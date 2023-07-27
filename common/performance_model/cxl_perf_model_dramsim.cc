#include "cxl_perf_model_dramsim.h"
#include "simulator.h"
#include "config.h"
#include "config.hpp"
#include "stats.h"
#include "shmem_perf.h"
#include "hooks_manager.h"

#if 0
#define MYTRACE_ENABLED
   extern Lock iolock;
#include "core_manager.h"
#include "simulator.h"
#define MYTRACE(...)                                    \
{                                                       \
    ScopedLock l(iolock);                               \
    fflush(f_trace);                                    \
    fprintf(f_trace, "[%d CXL] ", m_cxl_id);            \
    fprintf(f_trace, __VA_ARGS__);                      \
    fprintf(f_trace, "\n");                             \
    fflush(f_trace);                                    \
}
#else
#define MYTRACE(...) {}
#endif

CXLPerfModelDramSim::CXLPerfModelDramSim(cxl_id_t cxl_id, UInt64 transaction_size /* in bits */):
    CXLPerfModel(cxl_id, transaction_size),
    m_queue_model(NULL),
    m_cxl_bandwidth(8 * Sim()->getCfg()->getFloat("perf_model/cxl/memory_expander_" + itostr((unsigned int)cxl_id) + "/bandwidth")),
    m_total_queueing_delay(SubsecondTime::Zero()),
    m_total_access_latency(SubsecondTime::Zero()),
    m_dramsim(NULL),
    m_dramsim_channels(Sim()->getCfg()->getInt("perf_model/dram/dramsim/channles_per_contoller"))
{
    Sim()->getHooksManager()->registerHook(HookType::HOOK_ROI_BEGIN, CXLPerfModelDramSim::ROIstartHOOK, (UInt64)this);
    Sim()->getHooksManager()->registerHook(HookType::HOOK_ROI_END, CXLPerfModelDramSim::ROIendHOOK, (UInt64)this);
    
    
    m_cxl_latency =
        SubsecondTime::FS() *
        static_cast<uint64_t>(
            TimeConverter<float>::NStoFS(Sim()->getCfg()->getFloat(
                "perf_model/cxl/memory_expander_" + itostr((unsigned int)cxl_id) + "/cxl_latency")));
    m_ddr_access_cost = SubsecondTime::FS() *
        static_cast<uint64_t>(
            TimeConverter<float>::NStoFS(Sim()->getCfg()->getFloat(
                "perf_model/cxl/memory_expander_" + itostr((unsigned int)cxl_id) + "/ddr_latency")));
    m_queue_model = QueueModel::create(
        "cxl-queue", cxl_id,
        Sim()->getCfg()->getString("perf_model/cxl/queue_type"),
        m_cxl_bandwidth.getRoundedLatency(transaction_size));

    registerStatsMetric("cxl", cxl_id, "total-access-latency", &m_total_access_latency);
    registerStatsMetric("cxl", cxl_id, "total-queueing-delay", &m_total_queueing_delay);

    /* Intialize DRAMsim3 */
    m_dramsim = (DRAMsimCntlr**)malloc(sizeof(DRAMsimCntlr*) * m_dramsim_channels);
   for (UInt32 ch_id = 0; ch_id < m_dramsim_channels; ch_id++){
      m_dramsim[ch_id] = new DRAMsimCntlr(cxl_id, ch_id, true);
   }

#ifdef MYTRACE_ENABLED
    std::ostringstream trace_filename;
    trace_filename << "cxl_perf_" << (int)m_cxl_id << ".trace";
    f_trace = fopen(trace_filename.str().c_str(), "w+");
    std::cerr << "Create CXL perf trace " << trace_filename.str().c_str() << std::endl;
#endif // MYTRACE_ENABLED
}

CXLPerfModelDramSim::~CXLPerfModelDramSim()
{
    if (m_queue_model)
    {
        delete m_queue_model;
        m_queue_model = NULL;
    }

    if (m_dramsim)
    {
      for (UInt32 ch_id = 0; ch_id < m_dramsim_channels; ch_id++){
         delete m_dramsim[ch_id];
      }
      free(m_dramsim);
      m_dramsim = NULL;
    }
}

SubsecondTime CXLPerfModelDramSim::getAccessLatency(SubsecondTime pkt_time, UInt64 pkt_size, core_id_t requester, IntPtr address, CXLCntlrInterface::access_t access_type, ShmemPerf *perf)
{
    // pkt_size is in 'bits'
    // m_dram_bandwidth is in 'Bits per clock cycle'
    if ((!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
        return SubsecondTime::Zero();
    
    SubsecondTime processing_time = m_cxl_bandwidth.getRoundedLatency(pkt_size);

    // Compute Queue Delay
    SubsecondTime queue_delay;
    queue_delay = m_queue_model->computeQueueDelay(pkt_time, processing_time, requester);

    SubsecondTime access_latency = queue_delay + processing_time + m_cxl_latency + m_ddr_access_cost;

    uint32_t dramsim_ch_id = (address/m_transaction_size_bytes) % m_dramsim_channels;
    IntPtr ch_addr = (address/m_transaction_size_bytes) / m_dramsim_channels * m_transaction_size_bytes;
    m_dramsim[dramsim_ch_id]->addTrans(pkt_time + queue_delay + processing_time + m_cxl_latency, ch_addr, access_type == CXLCntlrInterface::WRITE);


    switch(access_type){
        case CXLCntlrInterface::READ:
            MYTRACE("R ==%s== @ %016lx", itostr(pkt_time + queue_delay + processing_time).c_str(), address);
            perf->updateTime(pkt_time);
            perf->updateTime(pkt_time + queue_delay, ShmemPerf::CXL_QUEUE);
            perf->updateTime(pkt_time + queue_delay + processing_time, ShmemPerf::CXL_BUS);
            perf->updateTime(pkt_time + queue_delay + processing_time + m_cxl_latency + m_ddr_access_cost, ShmemPerf::CXL_DEVICE);
            break;
        case CXLCntlrInterface::WRITE:
            MYTRACE("W ==%s== @ %016lx", itostr(pkt_time + queue_delay + processing_time).c_str(), address);
            break;
        default:
            LOG_PRINT_ERROR("Unrecognized CXL access Type: %u", access_type);
            break;
    }

    // Update Memory Counters
    m_num_accesses++;
    m_total_access_latency += access_latency;
    m_total_queueing_delay += queue_delay;

    return access_latency;
}


void CXLPerfModelDramSim::dramsimStart(){
   for (UInt32 ch_id = 0; ch_id < m_dramsim_channels; ch_id++){
      m_dramsim[ch_id]->start();
   }
}

void CXLPerfModelDramSim::dramsimEnd(){
   for (UInt32 ch_id = 0; ch_id < m_dramsim_channels; ch_id++){
      m_dramsim[ch_id]->stop();
   }  
}
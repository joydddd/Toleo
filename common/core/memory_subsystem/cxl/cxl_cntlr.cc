#include "cxl_cntlr.h"
#include "config.h"
#include "config.hpp"
#include "stats.h"
#include "shmem_perf.h"

#if 1
#  define MYLOG_ENABLED
   extern Lock iolock;
#  include "core_manager.h"
#  include "simulator.h"
#  define MYLOG(...) if (enable_trace){                                                   \
   ScopedLock l(iolock);                                                                  \
   fflush(f_trace);                                                                       \
   fprintf(f_trace, "[%s] %d%cdr %-25s@%3u: ",                                            \
   itostr(getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_SIM_THREAD)).c_str(),      \
         getMemoryManager()->getCore()->getId(),                                          \
         Sim()->getCoreManager()->amiUserThread() ? '^' : '_', __FUNCTION__, __LINE__);   \
   fprintf(f_trace, __VA_ARGS__); fprintf(f_trace, "\n"); fflush(f_trace); }
#else
#  define MYLOG(...) {}
#endif

CXLCntlr::CXLCntlr(MemoryManagerBase* memory_manager, ShmemPerfModel* shmem_perf_model, UInt32 cache_block_size, CXLAddressTranslator* cxl_address_tranlator, std::vector<bool>& cxl_connected):
    CXLCntlrInterface(memory_manager, shmem_perf_model, cache_block_size, cxl_address_tranlator),
    m_cxl_connected(cxl_connected),
    m_reads(NULL),
    m_writes(NULL),
    f_trace(NULL),
    enable_trace(false)
{
     m_reads = (UInt64*) malloc(sizeof(UInt64)*cxl_connected.size());
     memset(m_reads, 0, sizeof(UInt64)*cxl_connected.size());
     m_writes = (UInt64*) malloc(sizeof(UInt64)*cxl_connected.size());
     memset(m_writes, 0, sizeof(UInt64)*cxl_connected.size());
     m_cxl_perf_models = (CXLPerfModel**) malloc(sizeof(CXLPerfModel*)*cxl_connected.size());
     memset(m_cxl_perf_models, 0, sizeof(CXLPerfModel*) * cxl_connected.size());
     for (cxl_id_t cxl_id = 0; cxl_id < cxl_connected.size(); ++cxl_id) {
         if (cxl_connected[cxl_id]) {
             registerStatsMetric("cxl", cxl_id, "reads", &m_reads[cxl_id]);
             registerStatsMetric("cxl", cxl_id, "writes", &m_writes[cxl_id]);
             String cxl_id_str = itostr((unsigned int)cxl_id);
             ComponentBandwidth cxl_banchwidth =
                 8 * Sim()->getCfg()->getFloat("perf_model/cxl/memory_expander_" +
                                           cxl_id_str + "/bandwidth");  // Convert bytes to bits
             SubsecondTime cxl_access_cost = SubsecondTime::FS() *
                        static_cast<uint64_t>(TimeConverter<float>::NStoFS(
                            Sim()->getCfg()->getFloat(
                                "perf_model/cxl/memory_expander_" + cxl_id_str +
                                "/latency")));     // Operate in fs for higher
                                                   // precision before converting
                                                   // to uint64_t/SubsecondTime
             /* Create CXL perf model */;
             m_cxl_perf_models[cxl_id] = new CXLPerfModel(cxl_id, cxl_banchwidth, cxl_access_cost, cache_block_size * 8);
                                                                                                   /* convert from bytes to bits*/
         }
     }

#ifdef MYLOG_ENABLED
   std::ostringstream trace_filename;
   trace_filename << "cxl_cntlr_" << memory_manager->getCore()->getId()
                  << ".trace";
   f_trace = fopen(trace_filename.str().c_str(), "w+");
   std::cerr << "Create CXL cntlr trace " << trace_filename.str().c_str() << std::endl;
#endif // MYLOG_ENABLED
}


CXLCntlr::~CXLCntlr()
{
   if (m_reads) free(m_reads);
   if (m_writes) free(m_writes);

#ifdef MYLOG_ENABLED
   fclose(f_trace);
#endif // MYLOG_ENABLED
}


boost::tuple<SubsecondTime, HitWhere::where_t> CXLCntlr::getDataFromCXL(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf){
   cxl_id_t cxl_id = m_address_translator->getHome(address);
   IntPtr local_address = m_address_translator->getLinearAddress(address);
   UInt64 pkt_size = getCacheBlockSize() * 8; /* Byte to bits */

   LOG_ASSERT_ERROR(m_cxl_connected[cxl_id], "CXL %d is not connected", cxl_id);
   SubsecondTime cxl_latency = m_cxl_perf_models[cxl_id]->getAccessLatency(now, pkt_size, requester, local_address, READ, perf);

   ++m_reads[cxl_id];
   MYLOG("[%d]R @ %08lx latency %s", requester, address, itostr(cxl_latency.getNS()).c_str());

   return boost::tuple<SubsecondTime, HitWhere::where_t>(cxl_latency, HitWhere::CXL);
}


boost::tuple<SubsecondTime, HitWhere::where_t>
CXLCntlr::putDataToCXL(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now){
   cxl_id_t cxl_id = m_address_translator->getHome(address);
   IntPtr local_address = m_address_translator->getLinearAddress(address);
   UInt64 pkt_size = getCacheBlockSize() * 8;/* Byte to bits */

   LOG_ASSERT_ERROR(m_cxl_connected[cxl_id], "CXL %d is not connected", cxl_id);
   SubsecondTime cxl_latency = m_cxl_perf_models[cxl_id]->getAccessLatency(now, pkt_size, requester, local_address, WRITE, &m_dummy_shmem_perf);

   ++m_writes[cxl_id];
   MYLOG("[%d]W @ %08lx latency %s", requester, address, itostr(cxl_latency.getNS()).c_str());

   return boost::tuple<SubsecondTime, HitWhere::where_t>(cxl_latency, HitWhere::CXL);
}

void CXLCntlr::enablePerfModel() 
{
   for (cxl_id_t cxl_id = 0; cxl_id < m_cxl_connected.size(); cxl_id++){
      if (m_cxl_connected[cxl_id]){
         assert(m_cxl_perf_models[cxl_id]);
         m_cxl_perf_models[cxl_id]->enable();
      }
   }
   enable_trace = true;
}

void CXLCntlr::disablePerfModel() 
{
   for (cxl_id_t cxl_id = 0; cxl_id < m_cxl_connected.size(); cxl_id++){
      if (m_cxl_connected[cxl_id]){
         assert(m_cxl_perf_models[cxl_id]);
         m_cxl_perf_models[cxl_id]->disable();
      }
   } 
   enable_trace = false;
}
#include "scheduler_round_robin.h"
#include "simulator.h"
#include "core_manager.h"
#include "performance_model.h"
#include "config.hpp"
#include "os_compat.h"

// Round-robin scheduler.
// Each thread has is pinned to a specific core (m_thread_affinity).
// Cores are handed out to new threads in round-robin fashion.
// If multiple threads share a core, they are time-shared with a configurable quantum

SchedulerRoundRobin::SchedulerRoundRobin(ThreadManager *thread_manager)
   : SchedulerDynamic(thread_manager)
   , m_quantum(SubsecondTime::NS(Sim()->getCfg()->getInt("scheduler/round_robin/quantum")))
   , m_next_core(0)
   , m_last_periodic(SubsecondTime::Zero())
   , m_core_thread_running(Sim()->getConfig()->getApplicationCores(), INVALID_THREAD_ID)
   , m_quantum_left(Sim()->getConfig()->getApplicationCores(), SubsecondTime::Zero())
{
}

core_id_t SchedulerRoundRobin::threadCreate(thread_id_t thread_id)
{
   if (m_thread_info.size() <= (size_t)thread_id)
      m_thread_info.resize(m_thread_info.size() + 16);

   core_id_t core_id = m_next_core;

   ++m_next_core;
   if (m_next_core >= (core_id_t)Sim()->getConfig()->getApplicationCores())
      m_next_core = 0;

   m_thread_info[thread_id].core_affinity = core_id;

   // The first thread scheduled on this core can start immediately, the others have to wait
   if (m_core_thread_running[core_id] == INVALID_THREAD_ID)
   {
      m_thread_info[thread_id].core_running = core_id;
      m_core_thread_running[core_id] = thread_id;
      m_quantum_left[core_id] = m_quantum;
      return core_id;
   }
   else
   {
      m_thread_info[thread_id].core_running = INVALID_CORE_ID;
      return INVALID_CORE_ID;
   }
}

void SchedulerRoundRobin::threadYield(thread_id_t thread_id)
{
   core_id_t core_id = m_thread_info[thread_id].core_running;

   if (core_id != INVALID_CORE_ID)
   {
      Core *core = Sim()->getCoreManager()->getCoreFromID(core_id);
      SubsecondTime time = core->getPerformanceModel()->getElapsedTime();

      m_quantum_left[core_id] = SubsecondTime::Zero();
      reschedule(time, core_id, false);

      if (core_id != m_thread_info[thread_id].core_affinity
          && m_core_thread_running[m_thread_info[thread_id].core_affinity] == INVALID_THREAD_ID)
      {
         // We have just been moved to a different core, and that core is free. Schedule us there now.
         reschedule(time, m_thread_info[thread_id].core_affinity, false);
      }
   }
}

bool SchedulerRoundRobin::threadSetAffinity(thread_id_t calling_thread_id, thread_id_t thread_id, size_t cpusetsize, const cpu_set_t *mask)
{
   if (!mask)
   {
      // No mask given: free to schedule anywhere. We don't support anywhere, so just leave the thread where it is.
      return true;
   }

   core_id_t core_id = INVALID_CORE_ID;
   for(unsigned int cpu = 0; cpu < 8 * cpusetsize; ++cpu)
   {
      if (CPU_ISSET_S(cpu, cpusetsize, mask))
      {
         if (core_id != INVALID_CORE_ID)
         {
            LOG_PRINT_WARNING_ONCE("This scheduler only allows sched_setaffinity() with a single core. Call to sched_setaffinity() with multiple cores specified is ignored.");
            return false;
         }
         core_id = cpu;
      }
   }

   LOG_ASSERT_ERROR(core_id != INVALID_CORE_ID, "No valid core found in sched_setaffinity() mask");
   LOG_ASSERT_ERROR(core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(), "Invalid core %d found in sched_setaffinity() mask", core_id);

   // Next time, schedule this thread on core core_id
   m_thread_info[thread_id].core_affinity = core_id;

   if (thread_id == calling_thread_id)
      threadYield(thread_id);

   return true;
}

bool SchedulerRoundRobin::threadGetAffinity(thread_id_t thread_id, size_t cpusetsize, cpu_set_t *mask)
{
   if (cpusetsize*8 < Sim()->getConfig()->getApplicationCores())
   {
      // Not enough space to return result
      return false;
   }

   CPU_ZERO_S(cpusetsize, mask);
   CPU_SET_S(m_thread_info[thread_id].core_affinity, cpusetsize, mask);

   return true;
}

void SchedulerRoundRobin::threadStart(thread_id_t thread_id, SubsecondTime time)
{
}

void SchedulerRoundRobin::threadStall(thread_id_t thread_id, ThreadManager::stall_type_t reason, SubsecondTime time)
{
   // If the running thread becomes unrunnable, schedule someone else
   if (m_thread_info[thread_id].core_running != INVALID_CORE_ID)
      reschedule(time, m_thread_info[thread_id].core_running, false);
}

void SchedulerRoundRobin::threadResume(thread_id_t thread_id, thread_id_t thread_by, SubsecondTime time)
{
   // If our core is currently idle, schedule us now
   if (m_core_thread_running[m_thread_info[thread_id].core_affinity] == INVALID_THREAD_ID)
      reschedule(time, m_thread_info[thread_id].core_affinity, false);
}

void SchedulerRoundRobin::threadExit(thread_id_t thread_id, SubsecondTime time)
{
   // If the running thread becomes unrunnable, schedule someone else
   if (m_thread_info[thread_id].core_running != INVALID_CORE_ID)
      reschedule(time, m_thread_info[thread_id].core_running, false);
}

void SchedulerRoundRobin::periodic(SubsecondTime time)
{
   SubsecondTime delta = time - m_last_periodic;

   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (delta > m_quantum_left[core_id])
      {
         reschedule(time, core_id, true);
      }
      else
      {
         m_quantum_left[core_id] -= delta;
      }
   }

   m_last_periodic = time;
}

void SchedulerRoundRobin::reschedule(SubsecondTime time, core_id_t core_id, bool is_periodic)
{
   if (m_core_thread_running[core_id] != INVALID_THREAD_ID
       && Sim()->getThreadManager()->getThreadState(m_core_thread_running[core_id]) == Core::INITIALIZING)
   {
      // Thread on this core is starting up, don't reschedule it for now
      return;
   }

   thread_id_t new_thread_id = INVALID_THREAD_ID;
   thread_id_t first_thread_id = (m_core_thread_running[core_id] == INVALID_THREAD_ID) ? 0 : (m_core_thread_running[core_id] + 1);

   for(UInt64 idx = 0; idx < m_threads_runnable.size(); ++idx)
   {
      thread_id_t thread_id = (first_thread_id + idx) % m_threads_runnable.size();
      if ((m_thread_info[thread_id].core_affinity == core_id) && (m_threads_runnable[thread_id] == true))
      {
         // Unless we're in periodic(), don't pre-empt threads currently running on a different core
         if (is_periodic
             || m_thread_info[thread_id].core_running == INVALID_CORE_ID
             || m_thread_info[thread_id].core_running == core_id)
         {
            new_thread_id = thread_id;
            break;
         }
      }
   }

   if (m_core_thread_running[core_id] != new_thread_id)
   {
      // If a thread was running on this core, and we'll schedule another one, unschedule the current one
      if (m_core_thread_running[core_id] != INVALID_THREAD_ID)
      {
         m_thread_info[m_core_thread_running[core_id]].core_running = INVALID_CORE_ID;
         moveThread(m_core_thread_running[core_id], INVALID_CORE_ID, time);
      }

      // Set core as running this thread *before* we call moveThread(), otherwise the HOOK_THREAD_RESUME callback for this
      // thread might see an empty core, causing a recursive loop of reschedulings
      m_core_thread_running[core_id] = new_thread_id;

      // If we found a new thread to schedule, move it here
      if (new_thread_id != INVALID_THREAD_ID)
      {
         // If thread was running somewhere else: let that core know
         if (m_thread_info[new_thread_id].core_running != INVALID_CORE_ID)
            m_core_thread_running[m_thread_info[new_thread_id].core_running] = INVALID_THREAD_ID;
         // Move thread to this core
         m_thread_info[new_thread_id].core_running = core_id;
         moveThread(new_thread_id, core_id, time);
      }
   }

   m_quantum_left[core_id] = m_quantum;
}

void SchedulerRoundRobin::printState()
{
   printf("thread state:");
   for(thread_id_t thread_id = 0; thread_id < (thread_id_t)Sim()->getThreadManager()->getNumThreads(); ++thread_id)
   {
      char state;
      switch(Sim()->getThreadManager()->getThreadState(thread_id))
      {
         case Core::INITIALIZING:
            state = 'I';
            break;
         case Core::RUNNING:
            state = 'R';
            break;
         case Core::STALLED:
            state = 'S';
            break;
         case Core::SLEEPING:
            state = 's';
            break;
         case Core::WAKING_UP:
            state = 'W';
            break;
         case Core::IDLE:
            state = 'I';
            break;
         case Core::BROKEN:
            state = 'B';
            break;
         case Core::NUM_STATES:
         default:
            state = '?';
            break;
      }
      if (m_thread_info[thread_id].core_running != INVALID_CORE_ID)
      {
         printf(" %c@%d", state, m_thread_info[thread_id].core_running);
      }
      else
      {
         printf(" %c_%d", state, m_thread_info[thread_id].core_affinity);
      }
   }
   printf("  --  core state:");
   for(core_id_t core_id = 0; core_id < (core_id_t)Sim()->getConfig()->getApplicationCores(); ++core_id)
   {
      if (m_core_thread_running[core_id] == INVALID_THREAD_ID)
         printf(" __");
      else
         printf(" %2d", m_core_thread_running[core_id]);
   }
   printf("\n");
}

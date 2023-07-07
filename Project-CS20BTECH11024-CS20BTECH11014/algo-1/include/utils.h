#ifndef UTILS_H
#define UTILS_H

#include <atomic>
#include <mutex>

#ifdef SHARED_MEMORY_LOG
#define DEBUG(x) x
#else
#define DEBUG(x) 42
#endif

enum class TransStatus { ALIVE, ABORT, COMMIT };
enum class Operation { READ, WRITE };

inline unsigned getTime(std::chrono::system_clock::time_point StartTime) {
  auto CurrentTime = std::chrono::system_clock::now();
  auto TimeElapsed = CurrentTime - StartTime;
  return TimeElapsed.count();
}

class mylock {
  std::mutex m_mutex;

public:
  std::atomic<int> m_tid;
  bool isLocked() const { return m_tid > 0; }

  bool has_lock(int TId) const { return (!isLocked() || (m_tid == TId)); }
  void lock(int TId) {
    m_mutex.lock();
    m_tid = TId;
  }

  void unlock() {
    m_tid = -1;
    m_mutex.unlock();
  }

  bool try_lock(int TId) {
    bool Status = m_mutex.try_lock();
    if (Status)
      m_tid = TId;
    return Status;
  }
};
#endif
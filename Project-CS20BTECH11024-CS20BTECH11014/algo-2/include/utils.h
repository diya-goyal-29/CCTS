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

#endif
#ifndef PARALLEL_THREADPOOL_H
#define PARALLEL_THREADPOOL_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <functional>

namespace parallel {

typedef struct job_t Job;

class ThreadPool {
public:
  ThreadPool(const unsigned int num_threads);
  virtual ~ThreadPool();

private:
  unsigned int num_threads_;
  std::vector<std::thread> worker_threads_;

  std::queue<Job*> job_queue_;

  std::condition_variable cv_job_queue_;
  std::mutex mutex_get_job_;

  bool is_stopped_;

  void runThread();

public:
  void addJob(void (*function)(void *), void *arg);
};

}  // namespace parallel

#endif // PARALLEL_THREADPOOL_H
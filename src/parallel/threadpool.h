#ifndef PARALLEL_THREADPOOL_H
#define PARALLEL_THREADPOOL_H

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace parallel {

typedef struct job_t {
  void (*function_)(void *);
  void *arg_;

  job_t() : function_(NULL), arg_(NULL){};

  job_t(void (*function)(void *), void *arg) : function_(function), arg_(arg){};

  ~job_t(){};

} Job;

class ThreadPool {
public:
  ThreadPool(const unsigned int num_threads=std::thread::hardware_concurrency());
  virtual ~ThreadPool();

private:
  std::atomic<unsigned int> num_processed_;
  unsigned int num_running_threads_;
  bool is_stopped_;

  std::vector<std::thread> workers_;

  std::queue<std::unique_ptr<Job>> job_queue_;
  std::mutex queue_mutex_;
  std::condition_variable cv_job_added_;
  std::condition_variable cv_job_finished_;

  static void runThread(ThreadPool* pool_);

public:
  void addJob(void (*function)(void *), void *arg);
  void wait();
};

}  // namespace parallel

#endif // PARALLEL_THREADPOOL_H
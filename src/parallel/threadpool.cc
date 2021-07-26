#include "parallel/threadpool.h"

#include <stdio.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace parallel {

typedef struct job_t {
  void (*function_)(void *);
  void *arg_;

  job_t() : function_(NULL), arg_(NULL){};

  job_t(void (*function)(void *), void *arg) : function_(function), arg_(arg){};

  ~job_t(){};

} Job;


ThreadPool::ThreadPool(const unsigned int num_threads)
    : num_threads_(num_threads), is_stopped_(false) {
  worker_threads_.reserve(num_threads_);
  for (unsigned int i = 0; i < num_threads_; i++) {
    worker_threads_[i] = std::thread(runThread, (void *)this);
  }
}

ThreadPool::~ThreadPool() {
  is_stopped_ = true;
  cv_job_queue_.notify_all();
  for (auto &thread : worker_threads_) {
    thread.join();
  }
}

void ThreadPool::runThread() {
  Job* job;
  while (true) {
    { // Critical section
      std::unique_lock<std::mutex> lock(mutex_get_job_);
      cv_job_queue_.wait(lock, [this]() {
        return (!this->job_queue_.empty()) || (this->is_stopped_);
      });
      if (is_stopped_ && job_queue_.empty()) {
        break;
      }

      job = job_queue_.front();
      job_queue_.pop();
    } // End of critical section

    job->function_(job->arg_);
  }
}

void ThreadPool::addJob(void (*function)(void *), void *arg) {

  if (is_stopped_) {
    throw std::runtime_error("ThreadPool is already stopped.\n");
  }

  { // Critical section
    std::unique_lock<std::mutex> lock(mutex_get_job_);

    std::unique_ptr<Job> pJob(new Job(function, arg));
    job_queue_.push(pJob.get());
  } // End of critical section

  cv_job_queue_.notify_one();
}

} // namespace parallel
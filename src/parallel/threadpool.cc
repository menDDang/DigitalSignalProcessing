#include "parallel/threadpool.h"

#include <stdio.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace parallel {


ThreadPool::ThreadPool(const unsigned int num_threads) 
    : num_running_threads_(0)
    , num_processed_(0)
    , is_stopped_(false) {
  
  workers_.resize(num_threads);
  for (unsigned int i = 0; i < num_threads; i++) {
    workers_[i] = std::thread(runThread, this);
    //workers_.emplace_back(std::bind(&ThreadPool::runThread, this));
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    is_stopped_ = true;
    cv_job_added_.notify_all();
  }

  for (auto& thread : workers_) {
    thread.join();
  } 
}

void ThreadPool::runThread(ThreadPool* pool_) {
  ThreadPool *pool = (pool_);
  void (*fn)(void*);
  void* arg;
  while (true) {
    { // Critical section
      std::unique_lock<std::mutex> lock(pool->queue_mutex_);
      pool->cv_job_added_.wait(
        lock, [pool](){ return pool->is_stopped_ || !pool->job_queue_.empty(); });
      
      if (pool->is_stopped_) {
        break;
      }
      if (!pool->job_queue_.empty()) {
        pool->num_running_threads_++;
        
        auto job = pool->job_queue_.front().get();
        fn = job->function_;
        arg = job->arg_;
        pool->job_queue_.pop();
      }
    } // end of critictal section

    fn(arg);
    pool->num_processed_++;
      
    { // Critical section
      std::unique_lock<std::mutex> lock(pool->queue_mutex_);
      pool->num_running_threads_--;
      pool->cv_job_finished_.notify_one();
    } // end of critictal section
  }
}

void ThreadPool::addJob(void (*function)(void *), void *arg) {
  if (arg == NULL) {
    throw std::runtime_error("ThreadPool::addJob() - `job` is NULL.\n");
  }

  if (is_stopped_) {
    throw std::runtime_error("ThreadPool is already stopped.\n");
  }

  { // Critical section
    std::unique_lock<std::mutex> lock(queue_mutex_);

    std::unique_ptr<Job> job(new Job(function, arg));
    job_queue_.push(std::move(job));
    cv_job_finished_.notify_one();
  } // End of critical section
}

void ThreadPool::wait() {
  
  {  // Critical section
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_job_finished_.wait(lock, 
      [this]() { return job_queue_.empty() && (num_running_threads_ == 0); }
    );
  }  // End of critical section
}

} // namespace parallel
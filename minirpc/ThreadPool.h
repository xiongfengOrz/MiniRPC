// Copyright 2016,xiong feng.

#ifndef MINIRPC_THREADPOOL_H_
#define MINIRPC_THREADPOOL_H_

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "Task.h"


#include <deque>
#include <vector>
#include <string>
#include <memory>



namespace minirpc
{

class ThreadPoolWorker;

class ThreadPool  
{
 friend class ThreadPoolWorker;

 public:
  explicit ThreadPool(const std::string& nameArg, int maxSize);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  void start(int numThreads);
  void stop();

  const std::string& name() const
  { 
     return name_; 
  }

  size_t queueSize() const;
  void run(const Task* f);
  //  void run(Task&& f);


 private:
  bool isFull() const;
  void runInThread();
  std::shared_ptr<Task> take();

 private:
  std::string name_;
  mutable Mutex mutex_;
  Condition notEmpty_;
  Condition notFull_; 
  std::vector<std::unique_ptr<Thread>> threads_;
  std::deque<std::shared_ptr<Task>> queue_;
  size_t maxQueueSize_;
  bool running_;
  std::unique_ptr<ThreadPoolWorker> threadPoolWorker_;
};

}// namespace minirpc

#endif //  MINIRPC_THREADPOOL_H_


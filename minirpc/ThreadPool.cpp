// Copyright 2016,xiong feng.

#include "ThreadPool.h"

#include <assert.h>
#include <stdio.h>

#include <memory>
#include <string>

namespace minirpc
{

class ThreadPoolWorker : public ThreadWorker
{
 public:
  explicit ThreadPoolWorker(ThreadPool* threadpool)
    :threadpool_(threadpool)
    {}
  ~ThreadPoolWorker()
    {}
  void run() override;
  
 private:
  ThreadPool* threadpool_;
};



ThreadPool::ThreadPool(const std::string& nameArg = std::string("ThreadPool"),
                      int maxSize = 1)
  : name_(nameArg),
    mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    maxQueueSize_(maxSize),
    running_(false)
{
  threadPoolWorker_.reset(new ThreadPoolWorker(this));
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    threads_.emplace_back(new Thread(threadPoolWorker_.get()));
    threads_[i]->start();
  }
}

void ThreadPool::stop()
{
  {
    MutexLock lock(mutex_);
    running_ = false;
    notEmpty_.notifyAll();
  }
  for (auto& thread : threads_)
  {
    thread->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLock lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(const Task* task)
{
  if (threads_.empty())
  {
    task->handle();
  }
  else
  {
    MutexLock lock(mutex_);
    while (isFull())
    {
      notFull_.wait();
    }
    queue_.push_back(std::shared_ptr<Task>(const_cast<Task*>(task)));
    notEmpty_.notify();
  }
}

/*
void ThreadPool::run(Task&& task)
{
}
*/

std::shared_ptr<Task> ThreadPool::take()
{
  MutexLock lock(mutex_);
  while (queue_.empty() && running_) // must while
  {
    notEmpty_.wait();
  }
  std::shared_ptr<Task> task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool::isFull() const
{
  MutexLock lock(mutex_);
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}


void ThreadPoolWorker::run()
{
  while (threadpool_->running_)
  {
    std::shared_ptr<Task> task = threadpool_->take();
    if (task)
    {
      task->handle();
    }
  }
}

} // namespace minirpc



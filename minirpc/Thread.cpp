// Copyright 2016,xiong feng.

#include <pthread.h>
#include "Thread.h"

using namespace minirpc;

Thread::Thread(ThreadWorker *thread_worker)
  : thread_worker_(thread_worker) 
{
}

Thread::~Thread() 
{
}

void Thread::start() 
{
  pthread_attr_t thread_attr;

  if (pthread_attr_init(&thread_attr) != 0) 
  {
    return;
  }

  if (pthread_create(&pthread_, &thread_attr, threadMain, this) != 0) 
  {
    return;
  }
}

int Thread::join()
{
  return pthread_join(pthread_, nullptr);
}


void* Thread::threadMain(void* arg) 
{
  Thread *thread = static_cast<Thread*>(arg);
  thread->thread_worker_->run();
  return nullptr;
}

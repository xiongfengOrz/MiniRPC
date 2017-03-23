// Copyright 2016,xiong feng.

#ifndef MINIRPC_CONDITION_H_
#define MINIRPC_CONDITION_H_

#include "Mutex.h"

#include <pthread.h>

namespace minirpc
{

class Condition
{
 public:
  explicit Condition(Mutex& mutex)
    : mutex_(mutex)
  {
     pthread_cond_init(&pcond_, nullptr);
  }

  ~Condition()
  {
     pthread_cond_destroy(&pcond_);
  }

  Condition(const Condition&)= delete;
  Condition& operator=(const Condition&)= delete;

  void wait()
  {
    MutexLock lock(mutex_);
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
  }


  void notify()
  {
     pthread_cond_signal(&pcond_);
  }

  void notifyAll()
  {
     pthread_cond_broadcast(&pcond_);
  }

 private:
  Mutex& mutex_;
  pthread_cond_t pcond_;
};

}  // namespace minirpc
#endif  // MINIRPC_CONDITION_H_


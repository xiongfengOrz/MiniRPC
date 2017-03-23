// Copyright 2016,xiong feng.

#ifndef MINIRPC_MUTEX_H_
#define MINIRPC_MUTEX_H_

#include <assert.h>
#include <pthread.h>

namespace minirpc
{
class Mutex
{
 public:
  Mutex()
  {
    if(pthread_mutex_init(&pthread_mutex_, nullptr) != 0 )
    {
      return;
    }
  }

  ~Mutex()
  {
    if(pthread_mutex_destroy(&pthread_mutex_) != 0 )
    {
      return;
    }
  }

  void lock()const
  {
    pthread_mutex_lock(&pthread_mutex_);
  }

  bool tryLock()const
  {
    return(pthread_mutex_trylock(&pthread_mutex_) == 0);
  }

  void unLock()const
  {
    pthread_mutex_unlock(&pthread_mutex_);
  }

  pthread_mutex_t* getPthreadMutex()
  {
    return &pthread_mutex_;
  }

 private:
  mutable pthread_mutex_t pthread_mutex_;

};


class MutexLock
{
 public:
  explicit MutexLock(Mutex& mutex)
	  : mutex_(mutex)
  {
    mutex_.lock();
  }

  ~MutexLock()
  {
    mutex_.unLock();
  }

 private:
  Mutex& mutex_;
};

}

#endif  // MINIRPC_MUTEX_H_


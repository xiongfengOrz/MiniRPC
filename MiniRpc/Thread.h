// Copyright 2016,xiong feng.

#ifndef MINIRPC_THREAD_H_
#define MINIRPC_THREAD_H_

namespace minirpc
{

class ThreadWorker
{
 public:
  virtual ~ThreadWorker() {}
  virtual void run() = 0;
};

class Thread
{
 public:
  explicit Thread(ThreadWorker *ThreadWorker);
  ~Thread();

  void start();
  int join();

 private:
  static void* threadMain(void* arg);

 private:
  pthread_t pthread_;
  ThreadWorker *thread_worker_;
};

}  // namespace minirpc
#endif  // MINIRPC_THREAD_H_


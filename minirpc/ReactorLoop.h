// Copyright 2016,xiong feng.

#ifndef MINIRPC_REACTORLOOP_H_ 
#define MINIRPC_REACTORLOOP_H_

#include "Event.h"
#include "Task.h"

#include <memory>

// !!! 一定要注意，reactorLoop是实现的唯一持有者，一旦其析构，
// 实现也会退出，哪怕是在另一个线程中，这样设计比较合乎直观理解
// to do 更改设计，让用户选择新建线程传入ReactorLoop

//using namespace minirpc;

static const int kMaxPollWaitTime = 10;
static const int kEpollFdCount = 1024;

namespace minirpc
{

//object sematic
class ReactorLoop
{
 public:
  //you nerver should init reactorloop in different thread!!!!
  ReactorLoop();
  ~ReactorLoop();

  ReactorLoop(const ReactorLoop&) = delete;
  ReactorLoop& operator=(const ReactorLoop&) = delete;

  void startLoop();
  void stopLoop();

  //thread
  void startInOtherThread(Condition *monitor);
  void stopInOtherThread();

  //event_callback
  void addEvent(Event* events);
  void ModifyEvent(Event* events);
  void deleteEvent(Event* events);
  bool searchEvent(Event* events);

  void wakeUp();
  void runTask(std::shared_ptr<Task> handler);
  bool isInLoopThread() const;
  class Impl;

 private:
  ::std::unique_ptr<Impl> impl_;
};

}// namespace minirpc

#endif  //  MINIRPC_ReactorLoop_H_


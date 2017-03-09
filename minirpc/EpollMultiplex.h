// Copyright 2016,xiong feng.

#ifndef MINIRPC_EPOLLMULTIPLEX_H_
#define MINIRPC_EPOLLMULTIPLEX_H_

#include "IOMultiplex.h"
#include "ReactorLoop.h"

#include <sys/epoll.h>

#include <vector>

namespace minirpc
{

class EPollIOMultiplex : public IOMultiplex
{

 public:
  EPollIOMultiplex();  //when use pimpl, is difficult to pass reactorloop in
  virtual ~EPollIOMultiplex();

  virtual void poll(int timeoutMs, EventList* activeEvents) override;
  virtual void addEvent(Event* Event) override;
  virtual void deleteEvent(Event* Event) override;
  virtual void ModifyEvent(Event* channel) override;


 private:
  static const int kInitEventListSize = 16;
  static const char* operationToString(int op);

  void fillActiveEvents(int numEvents, EventList* activeEvents) const;
  void update(int operation, Event* Event);

  int epollfd_;
  std::vector<struct epoll_event> events_;
};

}// namespace minirpc
#endif  // MINIRPC_EPOLLMULTIPLEX_H_


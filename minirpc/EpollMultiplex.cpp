// Copyright 2016,xiong feng.

#include "Event.h"
#include "EpollMultiplex.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include "glog/logging.h"


using namespace minirpc;
 
 
static const int kNew = -1;
static const int kAdded = 1;
static const int kDeleted = 2;
 

EPollIOMultiplex::EPollIOMultiplex()
	: epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	  events_(kInitEventListSize)
{
  if (epollfd_ < 0)
  {
    LOG(FATAL) << "EPollIOMultiplex::EPollIOMultiplex";
  }
}

EPollIOMultiplex::~EPollIOMultiplex()
{
  ::close(epollfd_);
}

void EPollIOMultiplex::poll(int timeoutMs, EventList* activeEvents)
{
  LOG(INFO) << "fd total count " << Events_.size();
  int numEvents = ::epoll_wait(epollfd_,&*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
  int savedErrno = errno;
   
  if (numEvents > 0)
  {
  	LOG(INFO) << numEvents << " events happended";
  	fillActiveEvents(numEvents, activeEvents);
  	if (static_cast<size_t>(numEvents) == events_.size()) // implicit?
  	{
  	  events_.resize(events_.size() * 2);
  	}
  }
  else if (numEvents == 0)
  {
        LOG(INFO) << "nothing happended";
  }
  else
  {
  	if (savedErrno != EINTR)
  	{
  	  errno = savedErrno;
  	  LOG(ERROR) << "EPollIOMultiplex::poll()";
  	}
  }
  return;
}

void EPollIOMultiplex::fillActiveEvents(int numEvents,EventList* activeEvents) const
{
  for (int i = 0; i < numEvents; ++i)
  {
    Event* Event = static_cast<minirpc::Event*>(events_[i].data.ptr);
    Event->set_revents(events_[i].events);
    activeEvents->push_back(Event);
  }
}

void EPollIOMultiplex::addEvent(Event* Event)
{
  const int index = Event->index();
  //LOG(INFO) << "fd = " << Event->fd()<< " rflags = " << Event->events() << " index = " << index;
  if (index == kNew || index == kDeleted)
  {
  	int fd = Event->fd();
  	if (index == kNew)
  	{
          assert(Events_.find(fd) == Events_.end());
          Events_[fd] = Event;
  	}
  	else 
  	{
  	  assert(Events_.find(fd) != Events_.end());
  	  assert(Events_[fd] == Event);
  	}
  
  	Event->set_index(kAdded);
  	update(EPOLL_CTL_ADD, Event);
  }
  else
  {
  	//LOG(INFO)<< "update event "<<Event;
  	if (Event->isNoneEvent())
  	{
  	  update(EPOLL_CTL_DEL, Event);
  	  Event->set_index(kDeleted);
  	}
  	else
  	{
  	  update(EPOLL_CTL_MOD, Event);
  	}
  }
}

void EPollIOMultiplex::deleteEvent(Event* Event)
{

  //int fd = Event->fd();
  //LOG(INFO) << "Delete fd = " << fd;
  int index = Event->index();
  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, Event);
  }
  Event->set_index(kNew);
}

void EPollIOMultiplex::update(int operation, Event* Event)
{
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = Event->events();
  event.data.ptr = Event;
  int fd = Event->fd();
  LOG(INFO) << "epoll_ctl op = " << operationToString(operation)
  	<< " fd = " << fd << " event = { " << Event->eventsToString() << " }";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
  	if (operation == EPOLL_CTL_DEL)
  	{
  	  LOG(ERROR) << "epoll_ctl op =" 
	  	<< operationToString(operation) << " fd =" << fd;
  	}
  	else
  	{
  	  LOG(FATAL) << "epoll_ctl op =" 
	  	<< operationToString(operation) << " fd =" << fd;
  	}
  }
}

const char* EPollIOMultiplex::operationToString(int op)
{
  switch (op)
  {
  case EPOLL_CTL_ADD:
    return "ADD";
  case EPOLL_CTL_DEL:
  	return "DEL";
  case EPOLL_CTL_MOD:
  	return "MOD";
  default:
  	return "Unknown Operation";
  }
}

void EPollIOMultiplex::ModifyEvent(Event* channel)
{
  return;
}


#ifndef MINIRPC_IOMULTPLEX_H_
#define MINIRPC_IOMULTPLEX_H_

#include "ReactorLoop.h"
#include "Event.h"

#include <map>
#include <vector>

namespace minirpc
{

class IOMultiplex
{
 public:
  typedef std::vector<Event*> EventList;

  IOMultiplex();
  virtual ~IOMultiplex();

  IOMultiplex(const IOMultiplex&) = delete;
  IOMultiplex& operator=(const IOMultiplex&) = delete;

  virtual void poll(int timeoutMs, EventList* activeEvents) = 0;

  virtual void addEvent(Event* Event) = 0;

  virtual void deleteEvent(Event* Event) = 0;

  virtual void ModifyEvent(Event* events) = 0;

  virtual bool searchEvent(Event* Event) const;

 protected:
  typedef std::map<int, Event*> EventMap;
  EventMap Events_;
};

}  //  namespace minirpc
#endif  // MINIRPC_IOMULTPLEX_H_


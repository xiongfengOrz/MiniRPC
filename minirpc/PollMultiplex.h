#ifndef MINIRPC_POLLMULTIPLEX_H_
#define MINIRPC_POLLMULTIPLEX_H_

#include "IOMultiplex.h"

#include <poll.h>
#include <vector>

namespace minirpc
{

class PollPoller : public IOMultiplex
{
 public:
  PollPoller();
  virtual ~PollPoller();

  void poll(int timeoutMs, EventList* activeChannels) override;
  virtual void addEvent(Event* channel) override;
  virtual void deleteEvent(Event* channel) override;
  virtual void ModifyEvent(Event* channel) override;

 private:
  void fillActiveChannels(int numEvents,
		  EventList* activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;
};

}  //  namespace minirpc
#endif  // MINIRPC_POLLMULTIPLEX_H


#include "PollMultiplex.h"
#include "Event.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

#include "glog/logging.h"

using namespace minirpc;

PollPoller::PollPoller() = default;

PollPoller::~PollPoller() = default;

void PollPoller::poll(int timeoutMs, EventList* activeChannels)
{

  int numEvents = ::poll(&*pollfds_.begin(),
		  pollfds_.size(), timeoutMs);
  int savedErrno = errno;

  if (numEvents > 0)
  {
    LOG(INFO) << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);
  }
  else if (numEvents == 0)
  {
    LOG(INFO) << " nothing happended";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG(ERROR) << "PollPoller::poll()";
    }
  }
  return;
}

void PollPoller::fillActiveChannels(int numEvents, 
		EventList* activeChannels) const
{
  for (auto pfd = pollfds_.begin(); pfd != pollfds_.end() 
		  && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      auto ch = Events_.find(pfd->fd);
      Event* channel = ch->second;
      channel->set_revents(pfd->revents);
      activeChannels->push_back(channel);
    }
  }
}

void PollPoller::addEvent(Event* channel)
{

  LOG(INFO) << "fd = " << channel->fd()
	  << " events = " << channel->events();
  if (channel->index() < 0)
  {
    // a new one, add to pollfds_
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    Events_[pfd.fd] = channel;
  }
  else
  {
    // update existing one
    int idx = channel->index();
    struct pollfd& pfd = pollfds_[idx];
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent())
    {
      pfd.fd = -channel->fd() - 1;
    }
  }
}

void PollPoller::deleteEvent(Event* channel)
{

  LOG(INFO) << "fd = " << channel->fd();
  int idx = channel->index();
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  if (static_cast<size_t>(idx) == pollfds_.size() - 1)
  {
    pollfds_.pop_back();
  }
  else
  {
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd - 1;
    }
    Events_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
}

void PollPoller::ModifyEvent(Event* channel)
{
  return;
}


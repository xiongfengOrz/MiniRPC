#include "Event.h"

#include <sstream>
#include <assert.h>
#include <poll.h>
#include <memory>

#include "glog/logging.h"

using namespace minirpc;

static const int NoFlag = 0;
static const int ReadFlag = POLLIN | POLLPRI;
static const int WriteFlag = POLLOUT;

class Event::Impl
{
 public:
  Impl(int fd_);
  ~Impl();

  void add();
  void del();
  void handleEvent();

  void setReadHandler(EventHandler* handler);
  void setWriteHandler(EventHandler* handler);
  void setCloseHandler(EventHandler* handler);
  void setErrorHandler(EventHandler* handler);

  int fd() const;
  int events() const;
  void set_revents(int revt);

  bool isNoneEvent() const;

  void enableReading();
  void disableReading();
  void enableWriting();
  void disableWriting();
  void disableAll();
  bool isWriting() const;
  bool isReading() const;

  bool isAdded() const;

  // for IOMultiplex
  int index();
  void set_index(int idx);

  //debug
  std::string reventsToString() const;
  std::string eventsToString() const;
 private:
  //debug
  std::string eventsToString(int fd, int ev) const;

 private:
  //ReactorLoop* loop_;
  int  fd_;
  int  flags_;
  int  rflags_; // it's the received event types of epoll or poll
  int  index_; // used by IOMultiplex.
  bool logHup_;

  bool eventHandling_;
  bool addedToLoop_;

  std::shared_ptr<EventHandler> readHandler_;
  std::shared_ptr<EventHandler> writeHandler_;
  std::shared_ptr<EventHandler> closeHandler_;
  std::shared_ptr<EventHandler> errorHandler_;
};

Event::Impl::Impl(int fd__)
        :fd_(fd__),
        flags_(0),
        rflags_(0),
        index_(-1),
        logHup_(true),
        eventHandling_(false),
        addedToLoop_(false)
{
}

Event::Impl::~Impl()
{
  LOG(INFO)<< "delete event fd "<< fd_;
}

void  Event::Impl::setReadHandler(EventHandler* handler)
{
  flags_ |= ReadFlag;
  readHandler_ = std::shared_ptr<EventHandler>(handler);
}

void Event::Impl::setWriteHandler(EventHandler* handler)
{
  flags_ |= WriteFlag;
  writeHandler_ = std::shared_ptr<EventHandler>(handler);
}

void Event::Impl::setCloseHandler(EventHandler* handler)
{
  closeHandler_ = std::shared_ptr<EventHandler>(handler);
}

void Event::Impl::setErrorHandler(EventHandler* handler)
{
  errorHandler_ = std::shared_ptr<EventHandler>(handler);
}

int Event::Impl::fd() const
{
  return fd_;
}

int Event::Impl::events() const
{
  return flags_;
}
void Event::Impl::set_revents(int revt)
{
  rflags_ = revt;
}

bool Event::Impl::isNoneEvent() const
{
  return flags_ == NoFlag;
}

void Event::Impl::enableReading()
{
  flags_ |= ReadFlag;
  add();
}

void Event::Impl::disableReading()
{
  flags_ &= ~ReadFlag;
  add();
}

void Event::Impl::enableWriting()
{
  flags_ |= WriteFlag;
  add();
}

void Event::Impl::disableWriting()
{
  flags_ &= ~WriteFlag;
  add();
}

void Event::Impl::disableAll()
{
  flags_ = NoFlag;
  del();
}

bool Event::Impl::isWriting() const
{
  return flags_ & WriteFlag;
}

bool Event::Impl::isReading() const
{
  return flags_ & ReadFlag;
}

// for IOMultiplex
int Event::Impl::index()
{
  return index_;
}

void Event::Impl::set_index(int idx)
{
  index_ = idx;
}

void Event::Impl::add()
{
  if (addedToLoop_) return;
  addedToLoop_ = true;
}

void Event::Impl::del()
{
  if (!addedToLoop_) return;
  addedToLoop_ = false;
}

bool Event::Impl::isAdded() const
{
  return addedToLoop_;
}

void Event::Impl::handleEvent()
{
  eventHandling_ = true;
  //LOG(INFO) << eventsToString(fd_,rflags_);
  if ((rflags_ & POLLHUP) && !(rflags_ & POLLIN))
  {
    if (logHup_)
    {
      LOG(WARNING) << "fd = " << fd_ << " Event::handle_event() POLLHUP";
    }
    if (closeHandler_)
      closeHandler_->handlePacket();
  }

  if (rflags_ & POLLNVAL)
  {
    LOG(WARNING) << "fd = " << fd_ << " Event::handle_event() POLLNVAL";
  }

  if (rflags_ & (POLLERR | POLLNVAL))
  {
    if (errorHandler_)
      errorHandler_->handlePacket();
  }
  if (rflags_ & (POLLIN | POLLPRI | POLLRDHUP))
  {
    if (readHandler_)
      readHandler_->handlePacket();
  }
  if (rflags_ & POLLOUT)
  {
    if (writeHandler_)
      writeHandler_->handlePacket();
  }
  eventHandling_ = false;
}

std::string Event::Impl::reventsToString() const
{
  return eventsToString(fd_, rflags_);
}

std::string Event::Impl::eventsToString() const
{
  return  eventsToString(fd_, flags_);
}


std::string Event::Impl::eventsToString(int fd, int ev) const
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}




//Event
Event::Event(int fd)
        : impl_( new Impl(fd) )
{
}

Event::~Event() = default;

void Event::handleEvent()
{
  impl_->handleEvent();
}

void Event::add()
{
  impl_->add();
}
void Event::del()
{
  impl_->del();
}

bool Event::isAdded() const
{
  return impl_->isAdded();
}

void Event::setReadHandler(EventHandler* handler)
{
  impl_->setReadHandler(handler);
}
void Event::setWriteHandler(EventHandler* handler)
{
  impl_->setWriteHandler(handler);
}
void Event::setCloseHandler(EventHandler* handler)
{
  impl_->setCloseHandler(handler);
}
void Event::setErrorHandler(EventHandler* handler)
{
  impl_->setErrorHandler(handler);
}

int Event::fd() const
{
  return impl_->fd();
}

int Event::events() const
{
  return impl_->events();
}

void Event::set_revents(int revt)
{
  impl_->set_revents(revt);
}

bool Event::isNoneEvent() const
{
  return impl_->isNoneEvent();
}

void Event::enableReading()
{
  impl_->enableReading();
}

void Event::disableReading()
{
  impl_->disableReading();
}

void Event::enableWriting()
{
  impl_->enableWriting();
}

void Event::disableWriting()
{
  impl_->disableWriting();
}

void Event::disableAll()
{
  impl_->disableAll();
}

bool Event::isWriting() const
{
  return impl_->isWriting();
}

bool Event::isReading() const
{
  return impl_->isReading();
}

// for IOMultiplex
int Event::index()
{
  return impl_->index();
}

void Event::set_index(int idx)
{
  impl_->set_index(idx);
}

//debug
std::string Event::reventsToString() const
{
  return impl_->reventsToString();
}

std::string Event::eventsToString() const
{
  return impl_->eventsToString();
}




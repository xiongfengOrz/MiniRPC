// Copyright 2016,xiong feng.

#include "RpcServer.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include <string>
#include "glog/logging.h"

#include "RpcConnection.h"
#include "IterNetFunc.h"
#include "IterNetAddress.h"
#include "ReactorLoop.h"
#include "Event.h"
#include "Mutex.h"
#include "Task.h"
#include "Thread.h"
#include "EpollMultiplex.h"
#include "IterNetFunc.h"
#include "RpcConnectionFactory.h"


using namespace minirpc;
using std::string;

class AcceptTask : public Task
{
 public:
  explicit AcceptTask(RpcServer::Impl *impl)
    : impl_(impl)
  {
  }

  virtual ~AcceptTask()
  {
  }

  std::string taskName()
  {
    return "AcceptTask";
  }

  void handle() const override;

 private:
  RpcServer::Impl *impl_;
};


class acceptEventHandler:public EventHandler
{
 public:
  explicit acceptEventHandler(RpcServer::Impl* impl)
    :impl_(impl)
  {
  }

  virtual ~acceptEventHandler()
  {
  }

  void handlePacket() override;

 private:
  RpcServer::Impl* impl_;
};




class RpcServer::Impl
{
 public:
  Impl(std::vector<ReactorLoop*> loop, const string &host,
                  int port, ServerMessageHandlerFactory *factory);
  ~Impl();
  void handleAccept();
  void addAcceptEvent();
  void start();

 private:
  std::vector<ReactorLoop*> loop_;
  NetAddress listen_address_;
  std::unique_ptr<Event> acceptEvent_;
  int listen_fd_;
  int idleFd_;
  bool listenning_;
  bool started_;
  std::shared_ptr<RpcConnectionFactory> manager_;
  std::shared_ptr<AcceptTask> acceptTask_;
  size_t round_robin_;
};


RpcServer::Impl::Impl(std::vector<ReactorLoop*> loop, const string &host,
                int port, ServerMessageHandlerFactory *factory)
  : loop_(loop),
    listen_address_(host, port),
    listen_fd_(0),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
    listenning_(false),
    manager_(new RpcConnectionFactory(factory)),
    acceptTask_(new AcceptTask(this)),
    round_robin_(0)

{
}


RpcServer::Impl::~Impl()
{
  LOG(INFO) << "RpcServer " << this << " die  ";
  loop_[0]->deleteEvent(acceptEvent_.get());
  ::close(listen_fd_);
  ::close(idleFd_);
}

void RpcServer::Impl::handleAccept()
{
  int fd = -1;
  bool result = false;
  while (true)
  {
    struct sockaddr_in address;
    result = NetFunc::Accept(listen_fd_, &address, &fd, &idleFd_);
    if (result == false)
    {
      return;
    }
    if (fd == 0)
    {
      break;
    }
    NetAddress client_address(address);
    LOG(INFO) << "accept connection from " << client_address.DebugString()
            << ", fd: " << fd;
    std::shared_ptr<RpcConnection> connection = manager_->getConnection(fd);
    LOG(INFO) << "connection " << connection 
            << "use count" << connection.use_count();
    connection->setAddress(client_address);
    auto size = loop_.size();
    round_robin_ = (round_robin_+1)%(size);
    if (size > 1 && round_robin_ == 0)
      round_robin_++;
    connection->setReactorLoop(loop_[round_robin_]);
    connection->start();
  }
}

void RpcServer::Impl::start()
{
  listen_fd_ = NetFunc::Listen(listen_address_);
  acceptEvent_.reset(new Event(listen_fd_));
  LOG(INFO) << "create listen fd " << listen_fd_ << " for "
          << listen_address_.DebugString();
  acceptEvent_->setReadHandler(new acceptEventHandler(this));
  acceptEvent_->disableWriting();
  loop_[0]->runTask(acceptTask_);
}

void RpcServer::Impl::addAcceptEvent()
{
  LOG(INFO)<< "Add accept event to  loop" << loop_[0];
  loop_[0]->addEvent(acceptEvent_.get());
}

void acceptEventHandler::handlePacket()
{
  impl_->handleAccept();
}

void AcceptTask::handle() const
{
  impl_->addAcceptEvent();
}


//////////////////RpcServer///
RpcServer::RpcServer(std::vector<ReactorLoop*> loop, const string &host,
                int port, ServerMessageHandlerFactory *factory)
  //: impl_{ std::make_unique<Impl>() }  //only c++14
  : impl_( new Impl(loop, host, port, factory) )
{
}

RpcServer::~RpcServer()
{
}


void RpcServer::start()
{
  impl_->start();
}



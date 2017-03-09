// Copyright 2016,xiong feng.

#include "RpcConnection.h"

#include <map>
#include <list>
#include <string>
#include <memory>
#include "glog/logging.h"

#include "Event.h"
#include "ReactorLoop.h"
#include "Buffer.h"

using namespace minirpc;

class closeConnection :public Task
{
 public:
  explicit closeConnection(RpcConnection::Impl* impl)
    :impl_(impl)
  {
  }
    ~closeConnection()
  {
  }

  std::string taskName()
  {
    return "closeConnection";
  }

  void handle()const override;

 private:
  RpcConnection::Impl* impl_;
};


class writeConnection :public Task
{
 public:
  writeConnection(RpcConnection::Impl* impl, uint32_t opcode,
                  const ::google::protobuf::Message *message)
    :impl_(impl),
     opcode_(opcode),
     message_(message)
  {
  }
    virtual ~writeConnection()
  {
  }

  std::string taskName()
  {
    return "writeConnection";
  }
  void handle() const override;

 private:
  RpcConnection::Impl* impl_;
  uint32_t opcode_;
  const ::google::protobuf::Message *message_;
};



class readEventHandler:public EventHandler
{
 public:
  explicit readEventHandler(RpcConnection::Impl* impl)
    :impl_(impl)
  {
  }

  virtual ~readEventHandler()
  {
  }

  void handlePacket()  override;

 private:
  RpcConnection::Impl* impl_;
};


class writeEventHandler:public EventHandler
{
 public:
  explicit writeEventHandler(RpcConnection::Impl* impl)
    :impl_(impl)
  {
  }

  virtual ~writeEventHandler()
  {
  }

  void handlePacket() override;

 private:
  RpcConnection::Impl* impl_;
};



class RpcConnection::Impl
{
 public:
  Impl(RpcConnection *connection, RpcConnectionFactory *manager,
         ReactorLoop* loop, int fd);

  ~Impl();

  void setFd(int fd);
  int  getFd();

  void setAddress(const NetAddress &address);
  void setReactorLoop(ReactorLoop* loop);
  void setHandler(MessageHandler *handler);


  void sendPacket(uint32_t opcode,
                  const ::google::protobuf::Message *message);
  void sendTreadSafe(uint32_t opcode,
                  const ::google::protobuf::Message *message);
  void start();
  void close();

  void handleCloseTreadSafe();
  void handleRead();
  void handleWrite();

 private:
  RpcConnection *connection_;
  RpcConnectionFactory *connection_manager_;
  ReactorLoop *loop_;
  ReadMessageState state_;
  int fd_;
  NetAddress address_;

  std::unique_ptr<Event> event_;
  MessageHeader messageHeader_;
  Buffer input_buffer_;
  Buffer output_buffer_;
  std::shared_ptr<MessageHandler> message_handler_;
};


RpcConnection::Impl::Impl(RpcConnection *connection,
                              RpcConnectionFactory *manager,
                              ReactorLoop* loop, int fd)
  : connection_(connection),
    connection_manager_(manager),
    loop_(loop),
    state_(READ_HEADER),
    fd_(fd)
{

}

RpcConnection::Impl::~Impl()
{
  //handleCloseTreadSafe();
  LOG(INFO)<< "delete rpc connection "<<connection_;
  //unique ptr event will delete another times
}


void RpcConnection::Impl::start()
{

  event_.reset(new Event(fd_));
  event_->setReadHandler(new readEventHandler(this));
  event_->setWriteHandler(new writeEventHandler(this));
  event_->disableWriting();
  loop_->addEvent(event_.get());

}

void RpcConnection::Impl::close()
{
  if (fd_ > 0)
  {
    input_buffer_.clear();
    output_buffer_.clear();
    state_ = READ_HEADER;
    event_->disableAll();
    LOG(INFO) << "delete event from rpc_connection "<<event_->eventsToString();
    loop_->deleteEvent(event_.get());
    connection_manager_->putConnection(connection_);//error will delete twice
    ::close(this->fd_);
    fd_ = -1;
  }
}

void RpcConnection::Impl::handleRead()
{
  int buffer_read = input_buffer_.Read(fd_);
  if (buffer_read == -1)
  {
    LOG(ERROR) << "recv message from "<<address_.DebugString() << " error";
    close();
    return;
  }
  else if (buffer_read == 0)
  {
    LOG(INFO) << "recv message from "<<address_.DebugString() << " close";
    close();
    return;
  }
  //LOG(INFO)<< " Handle Read "<< fd_;
  while (!input_buffer_.isReadComplete())
  {
    uint32_t result = decodeMessage(&input_buffer_, &messageHeader_, &state_);
    if (result == kRecvMessageNotCompleted)
    {
      return;
    }

    if (!message_handler_->handlePacket(messageHeader_, &input_buffer_))
    {
      LOG(ERROR) << "handle message from "<<address_.DebugString() << " error";
      close();
    }
  }
  input_buffer_.clear();
  return;
}

void RpcConnection::Impl::handleWrite()
{
  if (output_buffer_.isReadComplete())
  {
    LOG(INFO) << "write complete";
    event_->disableWriting();
    loop_->addEvent(event_.get());
    return;
  }
  //LOG(INFO) <<  "RpcConnection send message";
  uint32_t result = write(&output_buffer_, fd_);
  if (result == kSendMessageError)
  {
    LOG(ERROR) << "send message to " << address_.DebugString() << " error";
    close();
  }

  if (output_buffer_.isReadComplete())
  {
    LOG(INFO) << "write complete";
    event_->disableWriting();
    loop_->addEvent(event_.get());
  }
  return;
}


void readEventHandler::handlePacket()
{
  impl_->handleRead();
}

void writeEventHandler::handlePacket()
{
  impl_->handleWrite();
}

void RpcConnection::Impl::handleCloseTreadSafe()
{
  closeConnection *closeTask = new closeConnection(this);
  loop_->runTask(std::shared_ptr<Task>(closeTask));
  //loop_->wakeUp();
}

void closeConnection::handle() const
{
  impl_->close();
}


void RpcConnection::Impl::sendPacket( uint32_t opcode,
                const ::google::protobuf::Message *message)
{
  //to do
  //may be we can write here directly if output_buffer_ is empty()?
  //no
  LOG(INFO) << " send packet ";
  encodeMessageToBuffer(opcode, message, &output_buffer_);
  if (!event_->isWriting())
  {
    //LOG(INFO) << " set writing ";
    event_->enableWriting();
    loop_->addEvent(event_.get());
  }
  //LOG(INFO) << " packet sended";
}

void RpcConnection::Impl::sendTreadSafe(uint32_t opcode,
                const ::google::protobuf::Message *message)
{
  writeConnection *sendTask = new writeConnection(this,opcode,message);
  loop_->runTask(std::shared_ptr<Task>(sendTask));
  // loop_->wakeUp();
}

void  writeConnection::handle() const
{
  impl_->sendPacket(opcode_, message_);
}


void RpcConnection::Impl::setFd(int fd)
{
  fd_ = fd;
}

int RpcConnection::Impl::getFd()
{
  return fd_;
}

void RpcConnection::Impl::setAddress(const NetAddress &address)
{
  address_ = address;
}

void RpcConnection::Impl::setReactorLoop(ReactorLoop* loop)
{
  loop_ = loop;
}

void RpcConnection::Impl::setHandler(MessageHandler *handler)
{
  message_handler_ = std::shared_ptr<MessageHandler>(handler);
}


/////////////////////////////////////////////////////////////

RpcConnection::RpcConnection(RpcConnectionFactory *manager,
                      ReactorLoop* loop, int fd)
  : impl_( new Impl(this, manager, loop, fd) )
{
}

RpcConnection::~RpcConnection()
{
}

void RpcConnection::setFd(int fd)
{
  impl_->setFd(fd);
}

void RpcConnection::setAddress(const NetAddress &address)
{
  impl_->setAddress(address);
}

void RpcConnection::setReactorLoop(ReactorLoop* loop)
{
  impl_->setReactorLoop(loop);
}


void RpcConnection::setHandler(MessageHandler *handler)
{
  impl_->setHandler(handler);
}


void RpcConnection::sendPacket(uint32_t opcode,
                const ::google::protobuf::Message *message)
{
  impl_->sendPacket(opcode, message);
}

void RpcConnection::sendTreadSafe(uint32_t opcode,
                const ::google::protobuf::Message *message)
{
  impl_->sendTreadSafe(opcode, message);
}


void RpcConnection::start()
{
  impl_->start();
}


void RpcConnection::close()
{
  impl_->close();
}

void RpcConnection::handleCloseTreadSafe()
{
  impl_->handleCloseTreadSafe();
}

int RpcConnection::getFd()
{
  return impl_->getFd();
}




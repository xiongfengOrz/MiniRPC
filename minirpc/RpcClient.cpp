#include "Task.h"
#include "RpcClient.h"
#include "IterNetAddress.h"
#include "RpcConnectionFactory.h"
#include "IterNetFunc.h"
#include "glog/logging.h"

#include <vector>
#include <memory>
#include <sys/socket.h>

using namespace minirpc;
using std::string;

static const uint32_t kMaxTryConnectTime = 5;

class ConnectTask : public Task
{
 public:
  explicit ConnectTask(RpcClient::Impl *impl)
    : impl_(impl)
  {
  }

  virtual ~ConnectTask()
  {
  }

  std::string taskName()
  {
    return "ConnectTask";
  }

  void handle() const override;

 private:
  RpcClient::Impl *impl_;
};


class RpcClient::Impl
{
 public:
  Impl(ReactorLoop* loop, const string &host,
		  int port, ServerMessageHandlerFactory *factory);
  ~Impl();

  bool connect();
  void close();
  void sendPacket(uint32_t opcode,
		  const ::google::protobuf::Message *message);

  void setHandler(MessageHandler *handler);
  void connectToServer();
  void pushTask(Task *task);

  bool isConnected()
  {
    return isConnected_;
  }

 private:
  void saveSendMessage(uint32_t opcode,
		  const ::google::protobuf::Message *message);

 private:
  int fd_; // owed by connection
  bool isConnected_;
  uint32_t connectTimes_;
  NetAddress serverAddress_;
  ReactorLoop* loop_;
  std::shared_ptr<ConnectTask> connectTask_;
  std::shared_ptr<MessageHandler> handler_;
  ReadMessageState state_;

  std::shared_ptr<RpcConnectionFactory> manager_;
  std::shared_ptr<RpcConnection> connection_;
  std::vector<uint32_t> saveOpcode;
  std::vector<::google::protobuf::Message *> saveMessage;//not safe, to fix
};

RpcClient::Impl::Impl(ReactorLoop* loop, const string &host,
		int port, ServerMessageHandlerFactory *factory)
  : fd_(-1),
    isConnected_(false),
    connectTimes_(0),
    serverAddress_(host, port),
    loop_(loop),
    connectTask_(new ConnectTask(this)),
    state_(READ_HEADER) ,
    manager_(new RpcConnectionFactory(factory))
{
}

RpcClient::Impl::~Impl()
{
}

void RpcClient::Impl::close()
{
  manager_->close();
  LOG(INFO) << "close connection";
  connection_.reset();
}


bool RpcClient::Impl::connect()
{

  loop_->runTask(connectTask_);
  return true;
}

void RpcClient::Impl::connectToServer()
{
  LOG(INFO)<< "Start connect to server";
  fd_ = NetFunc::Connect(serverAddress_);
  if (fd_ > 0)
  {
    isConnected_ = true;
    LOG(INFO) << "create connection fd " << fd_
	    << " for " << serverAddress_.DebugString();

    connection_ = manager_->getConnection(fd_);
    connection_->setAddress(serverAddress_);
    connection_->setReactorLoop(loop_);
    connection_->start();
    decltype(saveOpcode.size()) i;
    for (i = 0 ; i < saveOpcode.size() ; ++i)
    {
       sendPacket(saveOpcode[i], saveMessage[i]);
    }
    saveOpcode.clear();
    saveMessage.clear();
    /*
    uint32_t result = write(&output_buffer_, fd_);
    if (result == kSendMessageError) {
      LOG(ERROR) << "send message to "
        << serverAddress_.DebugString() << " error";
      Close();
    }
    */
    return;
  }
  ++connectTimes_;
  if (connectTimes_ > kMaxTryConnectTime)
  {
    LOG(ERROR) << "try connect to " << serverAddress_.DebugString()
	    << kMaxTryConnectTime << "times, now give up and quit";
    close();
    return;
  }

  LOG(ERROR) << "try connect to "<< serverAddress_.DebugString()
	  << ", " << connectTimes_ << " times fail, now try again...";
  loop_->runTask(std::shared_ptr<Task>(new ConnectTask(this)));
}


void RpcClient::Impl::sendPacket(uint32_t opcode,
		const ::google::protobuf::Message *message)
{
  if (isConnected_)
  {
    connection_->sendPacket(opcode, message);
  }
  else
  {
    saveSendMessage(opcode, message);
  }
}

void RpcClient::Impl::saveSendMessage(uint32_t opcode,
		const ::google::protobuf::Message *message)
{
  saveOpcode.push_back(opcode);
  saveMessage.push_back(const_cast<::google::protobuf::Message *>(message));
}



void RpcClient::Impl::setHandler(MessageHandler *handler)
{
  handler_ = std::shared_ptr<MessageHandler>(handler);
}



void ConnectTask::handle() const
{
  impl_->connectToServer();
}

void RpcClient::Impl::pushTask(Task *task)
{
  loop_->runTask(std::shared_ptr<Task>(task));
}


//interface
RpcClient::RpcClient(ReactorLoop* loop,const string &host, int port,
		     ServerMessageHandlerFactory *factory)
	//: impl_{ std::make_unique<Impl>() }  //only c++14
	: impl_( new Impl(loop, host, port, factory) )
{
}

RpcClient::~RpcClient()
{
}

bool RpcClient::connect()
{
  return impl_->connect();
}

void RpcClient::close()
{
  impl_->close();
}

void RpcClient::sendPacket(uint32_t opcode,
		const ::google::protobuf::Message *message)
{
  impl_->sendPacket(opcode, message);
}

void RpcClient::setHandler(MessageHandler *handler)
{
  impl_->setHandler(handler);
}

bool RpcClient::isConnected()
{
  return impl_->isConnected();
}

void RpcClient::pushTask(Task *task)
{
  impl_->pushTask(task);
}


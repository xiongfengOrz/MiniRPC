#include "RpcChannel.h"

#include <sys/time.h>
#include <arpa/inet.h>  // htonl, ntohl
#include <unistd.h>
#include <map>
#include <list>
#include <memory>
#include <functional> //hash
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "glog/logging.h"

#include "messageHandler.h"
#include "Task.h"
#include "RpcConnection.h"
#include "IterNetFunc.h"
#include "IterNetAddress.h"
#include "ProtobufHelper.h"
#include "Condition.h"


using namespace minirpc;
using std::string;

uint32_t kMethodTimeout = 5000;

struct MessageResponse
{
  ::google::protobuf::RpcController* controller;
  ::google::protobuf::Message* response;
  ::google::protobuf::Closure* done;
  uint32_t action_time;
};


class CallMethodTask : public Task
{
 public:
  CallMethodTask(uint32_t opcode, ::google::protobuf::Message* request,
		  RpcChannel::Impl *impl)
    : opcode_(opcode),
      request_(request),
      impl_(impl)
  {
  }

  virtual ~CallMethodTask()
  {
  }

  void handle() const override;

  std::string taskName()
  {
    return "CallMethodTask";
  }

 private:
  uint32_t opcode_;
  ::google::protobuf::Message* request_;
  RpcChannel::Impl *impl_;
};


/////////////////////handler factory/////


class RpcChannelMessageHandler : public ServerMessageHandler
{
 public:
  RpcChannelMessageHandler(RpcConnection *channel,
                           RpcChannel::Impl* impl)
    : ServerMessageHandler(channel),
      impl_(impl)
  {
  }

  virtual ~RpcChannelMessageHandler()
  {
  }

  bool handlePacket(const MessageHeader &header,
		  Buffer* buffer) override;

 private:
  RpcChannel::Impl* impl_;
};


class RpcChannelMessageHandlerFactory: public ServerMessageHandlerFactory
{
 public:
  explicit RpcChannelMessageHandlerFactory(RpcChannel::Impl* impl)
     :impl_(impl)
  {
  }

  virtual ~RpcChannelMessageHandlerFactory()
  {
  }

  ServerMessageHandler* CreateHandler(RpcConnection *connection) override
  {
    return new RpcChannelMessageHandler(connection, impl_);
  }

 private:
  RpcChannel::Impl* impl_;
};

class  RpcChannel::Impl : public ServerMessageHandler
{
 public:
  Impl(ReactorLoop* loop, const string &host, int port);
  ~Impl();

  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done);

  bool handlePacket(const MessageHeader &header, Buffer* buffer);

  MessageResponse* GetMessageResponse();

  void FreeMessageResponse(MessageResponse *response);

  void SendPacket(uint32_t opcode,
		  const ::google::protobuf::Message *message);

  bool connect();

  void close();

  void PushTask(Task *callback);

 private:
  typedef std::list<MessageResponse*> MessageResponseList;
  typedef std::map<uint32_t, MessageResponseList> MessageResponseMap;
  MessageResponseMap message_response_map_;
  MessageResponseList free_response_list_;
  RpcChannelMessageHandlerFactory* factory_;
  RpcClient client_; // this own client
};

RpcChannel::Impl::Impl(ReactorLoop* loop, const string &host, int port)
  : factory_(new RpcChannelMessageHandlerFactory(this)),
    client_(loop, host, port, factory_)
{
}

RpcChannel::Impl::~Impl()
{
}

void RpcChannel::Impl::close()
{
  client_.close();
}

bool RpcChannel::Impl::connect()
{
  return client_.connect();
}

void RpcChannel::Impl::CallMethod(
		    const ::google::protobuf::MethodDescriptor* method,
                    ::google::protobuf::RpcController* controller,
                    const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
                    ::google::protobuf::Closure* done)
{
  MessageResponse *message_response = GetMessageResponse();
  message_response->controller = controller;
  message_response->response = response;
  message_response->done = done;
  message_response->action_time = kMethodTimeout;
  uint32_t opcode = std::hash<std::string>()(method->full_name());
  ::google::protobuf::Message* save_request = request->New();
  save_request->CopyFrom(*request);
  LOG(INFO) << "call service: " << method->full_name()
	  << ", opcode: " << opcode  << ", request: "
	  << save_request->DebugString();
  message_response_map_[opcode].push_back(message_response);

  CallMethodTask *callback_method =
	  new CallMethodTask(opcode,save_request,this);
  PushTask(callback_method);

  //LOG(INFO) << "push timeout task for " << opcode;
}

void RpcChannel::Impl::SendPacket(uint32_t opcode,
		const ::google::protobuf::Message *message)
{
  if (client_.isConnected())
  {
    client_.sendPacket(opcode, message);
  }
  else
  {
    LOG(INFO) << "not connect to server currently";
    auto iter = message_response_map_.find(opcode);
    if (iter == message_response_map_.end())
    {
      LOG(ERROR) << "cannot find handler for opcode: " << opcode;
      return;
    }
    MessageResponse *response = iter->second.front();
    iter->second.pop_front();
    response->controller->SetFailed("server unaviable");
    response->done->Run();
    FreeMessageResponse(response);
  }
}

MessageResponse* RpcChannel::Impl::GetMessageResponse()
{
  if (free_response_list_.empty())
  {
    return new MessageResponse();
  }
  MessageResponse *message_response = free_response_list_.front();
  free_response_list_.pop_front();
  return message_response;
}

void RpcChannel::Impl::FreeMessageResponse(MessageResponse *response)
{
  free_response_list_.push_back(response);
}


bool RpcChannel::Impl::handlePacket(const MessageHeader &header,
		Buffer* buffer)
{
  auto iter = message_response_map_.find(header.opcode);
  if (iter == message_response_map_.end())
  {
    LOG(ERROR) << "cannot find handler for opcode: " << header.opcode;
    return false;
  }
  MessageResponse *response = iter->second.front();
  iter->second.pop_front();
  if (!buffer->deserializeMessage(response->response, header.length))
  {
    LOG(ERROR) << "DeserializeToMessage " << header.opcode << " error";
    FreeMessageResponse(response);
    return false;
  }

  response->done->Run();
  FreeMessageResponse(response);
  return true;
}

bool RpcChannelMessageHandler::handlePacket(const MessageHeader &header
		, Buffer* buffer)
{
  return impl_->handlePacket(header, buffer);
}


void RpcChannel::Impl::PushTask(Task *task)
{
  client_.pushTask(task);
}

void CallMethodTask::handle() const
{
  LOG(INFO) << "request: " << request_->DebugString();
  impl_->SendPacket(opcode_, request_);
  delete request_;
}



//////////////////////////interface/////////////////////////////////

RpcChannel::RpcChannel(ReactorLoop* loop, const string &host, int port)
  : impl_( new Impl(loop,host, port) )//RpcClient()，
{
}

RpcChannel::~RpcChannel()
{
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                         ::google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done)
{
  impl_->CallMethod(method, controller, request, response, done);
}

bool RpcChannel::connect()
{
  return impl_->connect();
}

void RpcChannel::close()
{
  impl_->close();
}




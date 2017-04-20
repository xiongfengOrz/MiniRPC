// Copyright 2016,xiong feng.

#include "RpcServiceServer.h"

#include <memory>
#include <vector>
#include <string>

#include "messageHandler.h"
#include "glog/logging.h"
#include "messageHandler.h"
#include "Task.h"
#include "RpcConnection.h"
#include "IterNetFunc.h"
#include "IterNetAddress.h"
#include "ProtobufHelper.h"
#include "Condition.h"
#include "RpcServiceManager.h"


using namespace minirpc;
using std::string;


class RpcServerMessageHandler : public ServerMessageHandler
{
 public:
  explicit RpcServerMessageHandler(RpcConnection *connection)
    : ServerMessageHandler(connection)
  {
  }

  virtual ~RpcServerMessageHandler() {}
  bool handlePacket(const MessageHeader &header, Buffer* buffer);

 public:
  RpcServiceManager *method_manager_;
};

class RpcServerMessageHandlerFactory: public ServerMessageHandlerFactory
{
 public:
  explicit RpcServerMessageHandlerFactory(RpcServiceManager *method_manager)
    : method_manager_(method_manager)
  {
  }

  virtual ~RpcServerMessageHandlerFactory()
  {
  }

  virtual ServerMessageHandler* CreateHandler(RpcConnection *connection)
  {
    RpcServerMessageHandler *handler = new RpcServerMessageHandler(connection);
    handler->method_manager_ = method_manager_;
    return handler;
  }

 public:
  RpcServiceManager *method_manager_;
};


class RpcServiceServer::Impl
{
 public:
  Impl(std::vector<ReactorLoop*> loop, const string &host, int port);
  ~Impl();

  void RegisterService(::google::protobuf::Service *service);
  void start();
  void run();

 private:
  RpcServiceManager method_manager_;
  RpcServerMessageHandlerFactory* factory_;
  RpcServer rpc_server_;
};

RpcServiceServer::Impl::Impl(std::vector<ReactorLoop*> loop, const string &host, int port)
  : factory_( new RpcServerMessageHandlerFactory(&method_manager_)),
    rpc_server_(loop, host, port, factory_)
{
}

RpcServiceServer::Impl::~Impl()
{
}

void RpcServiceServer::Impl::RegisterService(::google::protobuf::Service *service)
{
  method_manager_.RegisterService(service);
}

bool RpcServerMessageHandler::handlePacket(const MessageHeader &header, Buffer* buffer)
{
  return method_manager_->handlePacket(header, buffer, connection_);
}

void RpcServiceServer::Impl::start()
{
  rpc_server_.start();
}

void RpcServiceServer::Impl::run()
{
  start();
}

//interface

RpcServiceServer::RpcServiceServer(std::vector<ReactorLoop*> loop,
                const string &host, int port)
  :   impl_ (new Impl(loop, host, port))
{
}

RpcServiceServer::~RpcServiceServer()
{
}

void RpcServiceServer::RegisterService(::google::protobuf::Service *service)
{
  impl_->RegisterService(service);
}

void RpcServiceServer::start()
{
  impl_->start();
}

void RpcServiceServer::run()
{
  impl_->run();
}

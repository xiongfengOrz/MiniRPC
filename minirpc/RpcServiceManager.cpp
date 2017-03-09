// Copyright 2016,xiong feng.

#include "RpcServiceManager.h"

#include <map>
#include <functional> //hash
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/common.h>

#include "glog/logging.h"
#include "Callback.h"

using namespace minirpc;

class RpcMethod
{
 public:
  RpcMethod(::google::protobuf::Service *service,
            const ::google::protobuf::Message *request,
            const ::google::protobuf::Message *response,
            const ::google::protobuf::MethodDescriptor *method)
    : service_(service),
      request_(request),
      response_(response),
      method_(method)
  {
  }

 public:
  ::google::protobuf::Service *service_;
  const ::google::protobuf::Message *request_;
  const ::google::protobuf::Message *response_;
  const ::google::protobuf::MethodDescriptor *method_;
};

class RpcServiceManager::Impl
{
 public:
  Impl();
  ~Impl();

  void RegisterService(::google::protobuf::Service *service);
  bool handlePacket(const MessageHeader &header, Buffer* buffer,
          RpcConnection *connection);

 private:
  typedef std::map<uint32_t, RpcMethod*> RpcMethodMap;
  RpcMethodMap rpc_method_map_;
};

RpcServiceManager::Impl::Impl()
{
}

RpcServiceManager::Impl::~Impl()
{
  for (auto iter = rpc_method_map_.begin(); iter != rpc_method_map_.end();  ++iter)
  {
    RpcMethod *rpc_method = iter->second;
    delete rpc_method;
  }
}

void RpcServiceManager::Impl::RegisterService(::google::protobuf::Service *service)
{
  const ::google::protobuf::ServiceDescriptor *descriptor = service->GetDescriptor();
  for (int i = 0; i < descriptor->method_count(); ++i)
  {
    const ::google::protobuf::MethodDescriptor *method = descriptor->method(i);
    const ::google::protobuf::Message *request = &service->GetRequestPrototype(method);
    const ::google::protobuf::Message *response = &service->GetResponsePrototype(method);

    RpcMethod *rpc_method = new RpcMethod(service, request, response, method);
    uint32_t opcode = std::hash<std::string>()(method->full_name());
    LOG(INFO) << "register service: " << method->full_name()<< ", opcode: " << opcode;
    rpc_method_map_[opcode] = rpc_method;
  }
}

class HandleServiceEntry
{
 public:
  HandleServiceEntry(const ::google::protobuf::MethodDescriptor *method,
                     ::google::protobuf::Message *request,
                     ::google::protobuf::Message *response,
                     RpcConnection *connection)
    : method_(method),
      request_(request),
      response_(response),
      connection_(connection)
  {
  }
 public:
  const ::google::protobuf::MethodDescriptor *method_;
  ::google::protobuf::Message *request_;
  ::google::protobuf::Message *response_;
  RpcConnection *connection_;
};

static void HandleServiceDone(HandleServiceEntry *entry)
{
  uint32_t opcode = std::hash<std::string>()(entry->method_->full_name());
  entry->connection_->sendTreadSafe(opcode, entry->response_);
  delete entry->request_;
  delete entry->response_;
  delete entry;
}

bool RpcServiceManager::Impl::handlePacket(const MessageHeader &header, Buffer* buffer,
                       RpcConnection *connection)
{
  uint32_t opcode = header.opcode;
  RpcMethod *rpc_method = rpc_method_map_[opcode];
  if (rpc_method == nullptr)
  {
    LOG(ERROR) << "opcode " << header.opcode << " not registered";
    return false;
  }
  const ::google::protobuf::MethodDescriptor *method = rpc_method->method_;
  ::google::protobuf::Message *request = rpc_method->request_->New();
  ::google::protobuf::Message *response = rpc_method->response_->New();
  if (!buffer->deserializeMessage(request, header.length))
  {
    delete request;
    delete response;
    LOG(ERROR) << "DeserializeToMessage " << header.opcode << " error";
    return false;
  }
  HandleServiceEntry *entry =
          new HandleServiceEntry(method, request, response, connection);
  ::google::protobuf::Closure *done =
          ::google::protobuf::NewCallback(&HandleServiceDone, entry);

  rpc_method->service_->CallMethod(method, nullptr, request, response, done);
  return true;
}

RpcServiceManager::RpcServiceManager()
    : impl_( new Impl() )
{
}

RpcServiceManager::~RpcServiceManager()
{
}

void RpcServiceManager::RegisterService(::google::protobuf::Service *service)
{
  impl_->RegisterService(service);
}

bool RpcServiceManager::handlePacket(const MessageHeader &header, Buffer* buffer,
									RpcConnection *connection)
{
  return impl_->handlePacket(header, buffer, connection);
}


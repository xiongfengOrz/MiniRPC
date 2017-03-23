#ifndef MINIRPC_RPC_CHANNEL_H_
#define MINIRPC_RPC_CHANNEL_H_

#include "RpcClient.h"
#include <string>
#include <memory>
#include <google/protobuf/service.h>

namespace minirpc
{
class ReactorLoop;

class RpcChannel : public ::google::protobuf::RpcChannel
{
 public:
  RpcChannel(ReactorLoop* loop, const std::string &host, int port);
  virtual ~RpcChannel();

  bool connect();
  void close();

  virtual void CallMethod(const ::google::protobuf::MethodDescriptor* method,
		         ::google::protobuf::RpcController* controller,
                          const ::google::protobuf::Message* request,
                          ::google::protobuf::Message* response,
                          ::google::protobuf::Closure* done);

  class Impl;
 private:
  std::unique_ptr<Impl> impl_;
};

};
#endif // MINIRPC_RPC_CHANNEL_H_

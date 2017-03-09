// Copyright 2016,xiong feng.

#ifndef __EVENTRPC_RPC_METHOD_MANAGER_H_
#define __EVENTRPC_RPC_METHOD_MANAGER_H_

#include "Buffer.h"
#include "DataHead.h"
#include "RpcConnection.h"

#include <google/protobuf/service.h>
#include <memory>

namespace minirpc
{

class RpcServiceManager
{
 public:
  RpcServiceManager();
  ~RpcServiceManager();

  void RegisterService(::google::protobuf::Service *service);
  bool handlePacket(const MessageHeader &header,
                    Buffer* buffer,
                    RpcConnection *connection);
  class Impl;
 private:
  std::unique_ptr<Impl> impl_;
};

}
#endif  //  __EVENTRPC_RPC_METHOD_MANAGER_H_

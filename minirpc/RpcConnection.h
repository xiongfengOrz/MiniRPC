// Copyright 2016,xiong feng.

#ifndef MINIRPC_RPCCONNECTION_H_
#define MINIRPC_RPCCONNECTION_H_

#include <arpa/inet.h>
#include <inttypes.h>
#include <sys/types.h>

#include <memory>

#include <google/protobuf/service.h>
#include <google/protobuf/message.h>

#include "IterNetAddress.h"
#include "Buffer.h"
#include "ProtobufHelper.h"
#include "DataHead.h"
#include "ReactorLoop.h"
#include "RpcConnectionFactory.h"
#include "messageHandler.h"


namespace minirpc
{
class ReactorLoop;
class RpcConnection
{
 public:
  RpcConnection(RpcConnectionFactory* connection_manager = nullptr,
                  ReactorLoop* loop = nullptr,int fd = -1);
  ~RpcConnection();

  RpcConnection(const RpcConnection&) = delete;
  RpcConnection& operator=(const RpcConnection&) = delete;

  void setFd(int fd);
  int getFd();
  void setAddress(const NetAddress &address);

  //should pass new ptr
  void setHandler(MessageHandler *handler);
  void setReactorLoop(ReactorLoop* loop);
  void sendPacket(uint32_t opcode, const ::google::protobuf::Message *message);
  void sendTreadSafe(uint32_t opcode, 
                  const ::google::protobuf::Message *message);
  void handleCloseTreadSafe();

  void start();
  void close();
  class Impl;

 private:
  ::std::unique_ptr<Impl> impl_;
};
} // namespace minirpc
#endif // MINIRPC_RPCCONNECTION_H_


// Copyright 2016,xiong feng.

#ifndef MINIRPC_RPCSERVER_H_
#define MINIRPC_RPCSERVER_H_

#include <functional>
#include <vector>
#include <memory>
#include <string>

#include "Event.h"
#include "ReactorLoop.h"

namespace minirpc
{

class RpcServer
{
 public:   
  RpcServer(std::vector<ReactorLoop*> loop, const std::string &host, int port,
                  ServerMessageHandlerFactory *factory);
  ~RpcServer();
  RpcServer(const RpcServer&) = delete;
  RpcServer& operator=(const RpcServer&) = delete;
  
  void start();
  class Impl;

 private:
	std::unique_ptr<Impl> impl_;
};

}

#endif  // MINIRPC_RPCSERVER_H_


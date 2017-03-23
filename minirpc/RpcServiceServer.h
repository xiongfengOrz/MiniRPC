// Copyright 2016,xiong feng.

#ifndef MINIRPC_RPCSERVICESERVER_H_
#define MINIRPC_RPCSERVICESERVER_H_

#include "RpcServer.h"

#include <google/protobuf/service.h>
#include <vector>
#include <memory>
#include <string>
#include "Thread.h"


namespace minirpc
{

class ReactorLoop;
class RpcServiceServer : public ThreadWorker
{
 public:
  RpcServiceServer(std::vector<ReactorLoop*> loop, const std::string &host, int port);
  virtual ~RpcServiceServer();

  void RegisterService(::google::protobuf::Service *service);
  void start();
  void run() override;
  class Impl;

 private:
  std::unique_ptr<Impl> impl_;
};

}
#endif  //  MINIRPC_RPCSERVICESERVER_H


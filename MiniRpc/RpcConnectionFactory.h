// Copyright 2016,xiong feng.

#ifndef MINIRPC_RPCCONNECTIONFACTORY_H_
#define MINIRPC_RPCCONNECTIONFACTORY_H_

#include <list>
#include <vector>
#include <unordered_map>
#include <memory>

#include "messageHandler.h"
#include "DataHead.h"


namespace minirpc
{

class ServerMessageHandlerFactory;
class RpcConnection;


class RpcConnectionFactory
{
 public:
  // must new first before pass in
  explicit RpcConnectionFactory(ServerMessageHandlerFactory *factory);
  ~RpcConnectionFactory();

  RpcConnectionFactory(const RpcConnectionFactory&) = delete;
  RpcConnectionFactory& operator=(const RpcConnectionFactory&) = delete;

  // must new first before pass in
  void setHandlerFactory(ServerMessageHandlerFactory *factory);

  std::shared_ptr<RpcConnection> getConnection(int fd);
  void putConnection(RpcConnection* connection);
  void close();

 private:
  typedef std::vector<std::shared_ptr<RpcConnection>> ConnectionList;
  typedef std::unordered_map<int, std::shared_ptr<RpcConnection>> ConnectionSet;
  ConnectionList connectionList_;
  ConnectionSet useList_;
  std::unique_ptr<ServerMessageHandlerFactory> factory_;
};

}  //  namespace minirpc
#endif  //  MINIRPC_RPCCONNECTIONFACTORY_H_


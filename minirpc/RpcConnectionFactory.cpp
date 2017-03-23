// Copyright 2016,xiong feng.

#include "RpcConnectionFactory.h"

#include <memory>

#include "RpcConnection.h"
#include "glog/logging.h"

using namespace minirpc;

RpcConnectionFactory::RpcConnectionFactory(ServerMessageHandlerFactory *factory)
{
  factory_.reset(factory);
}

RpcConnectionFactory::~RpcConnectionFactory()
{
  //close();
  connectionList_.clear();
  useList_.clear();
}

void RpcConnectionFactory::close()
{
  for(auto con : connectionList_)
    con->handleCloseTreadSafe();
  for (auto it = useList_.begin(); it != useList_.end(); ++it)
    it->second->handleCloseTreadSafe();
}


void RpcConnectionFactory::setHandlerFactory(ServerMessageHandlerFactory *factory)
{
  factory_.reset(factory);
}


std::shared_ptr<RpcConnection> RpcConnectionFactory::getConnection(int fd)
{
  std::shared_ptr<RpcConnection> connection (new RpcConnection(this));
  ServerMessageHandler *handler = factory_->CreateHandler(connection.get());//new
  connection->setHandler(handler);
  connection->setFd(fd);
  useList_[fd] = connection;
  return connection;
}


void RpcConnectionFactory::putConnection(RpcConnection *connection)
{
  LOG(INFO)<< "put connection "<< connection;
  useList_.erase(connection->getFd());
}




#ifndef MINIRPC_MESSAGEHANDLER_H_
#define MINIRPC_MESSAGEHANDLER_H_

#include "DataHead.h"
#include "Buffer.h"
#include "Condition.h"

namespace minirpc
{

class RpcConnection;

class  EventHandler
{
 public:
  virtual ~EventHandler() {}
  virtual void handlePacket() = 0;
 protected:
  EventHandler() {}
};

class MessageHandler
{
 public:
  virtual ~MessageHandler() {}
  virtual bool handlePacket(const MessageHeader &header,
		  Buffer* buffer) = 0;
 protected:
  MessageHandler() {}
};

class ClientMessageHandler : public MessageHandler
{
 public:
  ClientMessageHandler()
  {}
  virtual ~ClientMessageHandler()
  {}
  virtual bool HandlePacket(const MessageHeader &header,
		  Buffer* buffer) = 0;
 protected:
  friend class ClientMessageHandlerFactory;
  explicit ClientMessageHandler(RpcConnection *connection)
        : connection_(connection)
  {}
 private:
  RpcConnection *connection_;
};

class ServerMessageHandler : public MessageHandler
{
 public:
  ServerMessageHandler()
  {}
  virtual ~ServerMessageHandler()
  {}
  virtual bool handlePacket(const MessageHeader &header,
		  Buffer* buffer) = 0;
 protected:
  friend class ServerMessageHandlerFactory;
  explicit ServerMessageHandler(RpcConnection *connection)
      	: connection_(connection)
  {}
 protected:
  RpcConnection *connection_;
};

class ClientMessageHandlerFactory
{
 public:
  ClientMessageHandlerFactory(Condition *monitor)
  	:monitor(monitor)
  {}
  virtual ~ClientMessageHandlerFactory()
  {}
  virtual ServerMessageHandler* CreateHandler
	  (RpcConnection *connection) = 0;
 protected:
  Condition *monitor;
};

class ServerMessageHandlerFactory
{
 public:
  ServerMessageHandlerFactory()
  {}
  virtual ~ServerMessageHandlerFactory()
  {}
  virtual ServerMessageHandler* CreateHandler
	  (RpcConnection *connection) = 0;
};

}  //  namespace minirpc

#endif // MINIRPC_MESSAGEHANDLER_H_

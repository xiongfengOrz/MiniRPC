#include <iostream>
#include <stdio.h>
#include <string>
#include "minirpc/IterNetFunc.h"
#include "minirpc/RpcServer.h"
#include "minirpc/IterNetAddress.h"
#include "minirpc/ReactorLoop.h"
#include "minirpc/Event.h"
#include "glog/logging.h"
#include "minirpc/Mutex.h"
#include "minirpc/Task.h" 
#include "minirpc/Thread.h"
#include "minirpc/EpollMultiplex.h"
#include "minirpc/IterNetFunc.h"
#include "minirpc/Buffer.h"
#include "minirpc/RpcConnectionFactory.h"
#include "minirpc/Condition.h"
#include "minirpc/DataHead.h"
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include "DataHead.h"
#include "minirpc/RpcConnection.h"

#include "echo.pb.h"
#include "echo_opcode.h"


using namespace minirpc;
 

class EchoServerMessageHandler : public ServerMessageHandler 
{
 public:
  EchoServerMessageHandler(RpcConnection *connection)
    : ServerMessageHandler(connection) 
  {
  }

  virtual ~EchoServerMessageHandler() 
  {
  }

  bool handlePacket(const MessageHeader &header, Buffer* buffer) override
  {
    if (header.opcode != CMSG_ECHO) 
    {
      LOG(ERROR) << "opcode error: " << header.opcode;
      return false;
    }
    echo::EchoRequest request;
    if (!buffer->deserializeMessage(&request, header.length)) 
	  {
      LOG(ERROR) << "deserializeMessage error: " << header.length;
      return false;
    }
    LOG(INFO) << "request: " << request.message();
    echo::EchoResponse response;
    if(request.message()=="hello")
      response.set_response("world");
    else
      response.set_response(request.message());
    connection_->sendPacket(SMSG_ECHO, &response);
    return true;
  }
};

class EchoServerMessageHandlerFactory: public ServerMessageHandlerFactory 
{
 public:
  EchoServerMessageHandlerFactory() 
  {
  }
  virtual ~EchoServerMessageHandlerFactory() 
  {
  }
  ServerMessageHandler* CreateHandler(RpcConnection *connection) 
  {
    return new EchoServerMessageHandler(connection);
  }
};


int main(int argc, char *argv[]) 
{
  Mutex mutex;
  Condition monitor(mutex);
  ReactorLoop loop;
  loop.startInOtherThread(&monitor);
  EchoServerMessageHandlerFactory *factory=new EchoServerMessageHandlerFactory();
  std::string host="127.0.0.1";
  int port=22203;
  monitor.wait();
  return 0;
}

 

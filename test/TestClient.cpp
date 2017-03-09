#include <sys/socket.h>
#include <unistd.h>
#include <memory>
#include <string>
#include "minirpc/Task.h"
#include "minirpc/RpcClient.h"
#include "minirpc/IterNetAddress.h"
#include "minirpc/RpcConnectionFactory.h"
#include "minirpc/IterNetFunc.h"
#include "glog/logging.h"
#include "minirpc/Condition.h"
#include "minirpc/DataHead.h"
#include "echo.pb.h"
#include "echo_opcode.h"

using namespace minirpc;

static const uint32_t kMaxConnection = 80;
static uint32_t kCount = 0;	

class EchoClientMessageHandler : public ServerMessageHandler 
{
 public:
  EchoClientMessageHandler(RpcConnection *channel,
                           Condition *monitor)
    : ServerMessageHandler(channel),
      monitor_(monitor) 
  {
  }

  virtual ~EchoClientMessageHandler() 
  {
  }

  bool handlePacket(const MessageHeader &header, Buffer* buffer) override
  {
    if (header.opcode != SMSG_ECHO) 
	  {
       LOG(ERROR) << "opcode error: " << header.opcode;
      return false;
    }
    echo::EchoResponse response;
    if (!buffer->deserializeMessage(&response, header.length)) 
	  {
      LOG(ERROR) << "deserializeMessage error: " << header.opcode;
      monitor_->notify();
      return false;
    }
    ++kCount;
    LOG(INFO) << "response: " << response.response() << ", count: " << kCount;
    if (kCount == kMaxConnection) 
	  {
      monitor_->notify();
    }
    return true;
  }
 private:
  Condition *monitor_;
};


class EchoClienMessageHandlerFactory: public ServerMessageHandlerFactory 
{
 public:
  EchoClienMessageHandlerFactory(Condition *monitor)
     :monitor_(monitor)  
  {
  }
  virtual ~EchoClienMessageHandlerFactory() 
  {
  }
  ServerMessageHandler* CreateHandler(RpcConnection *connection) override
  {
    return new EchoClientMessageHandler(connection,monitor_);
  }
 private:
  Condition* monitor_;
};



int main(int argc, char *argv[]) 
{
  ReactorLoop loop;
	Mutex mutex,loop_mutex;
	Condition monitor(mutex);
	Condition loop_monitor(loop_mutex);
	EchoClienMessageHandlerFactory* factory=new EchoClienMessageHandlerFactory(&monitor);
	std::string host="127.0.0.1";
	int port=22203;
  loop.startInOtherThread(&loop_monitor);
	loop_monitor.wait();
  RpcClient client(&loop,host, port,factory);
	LOG(INFO)<<" start connect";
  client.connect();
  echo::EchoRequest request;
  request.set_message("hello");
  LOG(INFO)<<" send packet";
  client.sendPacket(CMSG_ECHO, &request);
	monitor.wait();
	LOG(INFO) << "out of wait";
	return 0;
}


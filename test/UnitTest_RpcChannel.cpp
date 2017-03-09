#include "RpcChannel.h"
#include "RpcController.h"
#include <sys/socket.h>
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

void echo_done(echo::EchoResponse* resp, Condition *monitor) 
{
  LOG(INFO) << "response: " << resp->response();
  monitor->notify();
}

int main(int argc, char *argv[]) 
{
  google::InitGoogleLogging(argv[0]);
  ReactorLoop loop;
  Mutex mutex,loop_mutex;
  Condition monitor(mutex);
  Condition loop_monitor(loop_mutex);
  std::string host="127.0.0.1";
  int port=22828;
  loop.startInOtherThread(&loop_monitor);
  loop_monitor.wait();
  echo::EchoRequest request;
  request.set_message("hello");
  LOG(INFO) << "request: " << request.DebugString();
  RpcController rpc_controller;
  RpcChannel rpc_channel(&loop,host, port);
  rpc_channel.connect();
  
  echo::EchoService::Stub stub(&rpc_channel);
  echo::EchoResponse response;
  stub.Echo(&rpc_controller, &request, &response,
            ::google::protobuf::NewCallback(::echo_done, &response, &monitor));
			
  monitor.wait();   
  if (rpc_controller.Failed()) 
  {
    LOG(INFO) << "response = " << rpc_controller.ErrorText();
  }

  stub.Echo(&rpc_controller, &request, &response,
            ::google::protobuf::NewCallback(::echo_done, &response, &monitor));
			
  monitor.wait();   
  if (rpc_controller.Failed()) 
  {
    LOG(INFO) << "response = " << rpc_controller.ErrorText();
  }
  LOG(INFO) << "exit program";
  return 0;
}

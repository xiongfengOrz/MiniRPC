#include "RpcServiceServer.h"
#include "RpcServiceManager.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
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
#include "minirpc/Thread.h"

#include "echo.pb.h"
#include "echo_opcode.h"


using namespace minirpc;


class EchoServiceImpl : public echo::EchoService 
{
 public:
  EchoServiceImpl() 
  {
  };

  virtual void Echo(::google::protobuf::RpcController* controller,
                    const ::echo::EchoRequest* request,
                    ::echo::EchoResponse* response,
                    ::google::protobuf::Closure* done) 
 {
    LOG(INFO) << "request: " << request->message();

    response->set_response(request->message());
    if (done) 
	{
      done->Run();
    }
  }
  virtual void Dummy(::google::protobuf::RpcController* controller,
                     const ::echo::DummyRequest* request,
                     ::echo::DummyResponse* response,
                     ::google::protobuf::Closure* done) 
  {
    LOG(INFO) << "dummy request: " << request->message();
    if (done) 
	{
      done->Run();
    }
  }
};


int main(int argc, char *argv[]) 
{
  google::InitGoogleLogging(argv[0]);
  Mutex mutex1,mutex2;
  Condition monitor1(mutex1);
	Condition monitor2(mutex2);
	Mutex mutex3,mutex4;
  Condition monitor3(mutex3);
	Condition monitor4(mutex4);
	std::vector<ReactorLoop*> loop;
	ReactorLoop* loop1_=new ReactorLoop();
  loop1_->startInOtherThread(&monitor1);
	loop.push_back(loop1_);
	ReactorLoop* loop2_=new ReactorLoop();
  loop2_->startInOtherThread(&monitor2);
	loop.push_back(loop2_);  
  ReactorLoop* loop3_=new ReactorLoop();
  loop3_->startInOtherThread(&monitor3);
	loop.push_back(loop3_);
	ReactorLoop* loop4_=new ReactorLoop();
  loop4_->startInOtherThread(&monitor4);
	loop.push_back(loop4_);
    
  monitor1.wait();
	monitor2.wait();
	monitor3.wait();
	monitor4.wait();
  std::string host="127.0.0.1";
  int port=22829;
	::google::protobuf::Service *service = new EchoServiceImpl();
	RpcServiceServer server(loop,host, port);
	server.RegisterService(service);
	server.start();
  //just stop in here
  int n;
  std::cin>> n;
  return 0;
}

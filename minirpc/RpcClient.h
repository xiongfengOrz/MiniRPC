#ifndef MINIRPC_RPCCLIENT_H_
#define MINIRPC_RPCCLIENT_H_

#include "Buffer.h"
#include "DataHead.h"
#include "messageHandler.h"
#include "ReactorLoop.h"
#include "Event.h"
#include "RpcConnection.h"

#include <inttypes.h>
#include <sys/types.h>
#include <string>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>


namespace minirpc
{
class RpcChannel;

class RpcClient
{
 public:
  friend class RpcChannel;
  RpcClient(ReactorLoop* loop, const std::string &host, int port,
  				ServerMessageHandlerFactory *factory);

  ~RpcClient();

  RpcClient(const RpcClient&) = delete;
  RpcClient& operator=(const RpcClient&) = delete;

  bool connect();
  void close();

  //thread safe
  void sendPacket(uint32_t opcode,
		  const ::google::protobuf::Message *message);
  void setHandler(MessageHandler *handler);
  bool isConnected();

  class Impl;

 private:
  void pushTask(Task *task);

 private:
  ::std::unique_ptr<Impl> impl_;
};

}
#endif // MINIRPC_RPCCLIENT_H_


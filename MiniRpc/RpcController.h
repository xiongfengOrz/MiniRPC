// Copyright 2016,xiong feng.

#ifndef MINIRPC_RPC_CONTROLLER_H_
#define MINIRPC_RPC_CONTROLLER_H_

#include <string>
#include <google/protobuf/service.h>

namespace minirpc
{

class RpcController : public ::google::protobuf::RpcController
{
 public:
  RpcController();
  virtual ~RpcController();

  // client-end methods
  virtual void Reset();
  virtual bool Failed() const;
  virtual std::string ErrorText() const;
  virtual void StartCancel();

  // server-end methods
  virtual void SetFailed(const std::string& reason);
  virtual bool IsCanceled() const;
  virtual void NotifyOnCancel(::google::protobuf::Closure* callback);

 private:
  std::string m_fail_reason;
};

}
#endif // MINIRPC_RPC_CONTROLLER_H_

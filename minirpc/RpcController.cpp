// Copyright 2016,xiong feng.

#include "RpcController.h"

using namespace minirpc;

RpcController::RpcController()
{}

RpcController::~RpcController()
{}

// client-end methods
void RpcController::Reset()
{}

bool RpcController::Failed() const
{
  return !m_fail_reason.empty();
}

std::string RpcController::ErrorText() const
{
  return m_fail_reason;
}

void RpcController::StartCancel()
{}

// server-end methods
void RpcController::SetFailed(const std::string& reason)
{
  m_fail_reason = reason;
}

bool RpcController::IsCanceled() const
{
  return false;
}

void RpcController::NotifyOnCancel(::google::protobuf::Closure* callback)
{}


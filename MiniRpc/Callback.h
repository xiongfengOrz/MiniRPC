// Copyright 2016,xiong feng.

#ifndef MINIRPC_CALLBACK_H_
#define MINIRPC_CALLBACK_H_

namespace minirpc
{

struct Callback
{
 public:
  Callback()
  {}
  virtual ~Callback()
  {}
  virtual void Run() = 0;
};

}  //  namespace minirpc
#endif  //  MINIRPC_CALLBACK_H_

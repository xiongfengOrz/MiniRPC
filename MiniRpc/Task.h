// Copyright 2016,xiong feng.

#ifndef TINYRPC_TASK_H_
#define TINYRPC_TASK_H_

#include <string>

namespace minirpc
{
class Task
{
 public:
  Task()
  {}
  virtual ~Task()
  {}
  virtual void handle() const = 0;
  virtual std::string taskName()
  {
    return "";
  }
};

}  // namespace minirpc
#endif  // TINYRPC_TASK_H_


// Copyright 2016,xiong feng.

#ifndef MINIRPC_DATAHEAD_H_
#define MINIRPC_DATAHEAD_H_

#include <inttypes.h>
#include <sys/types.h>


namespace minirpc
{

enum ReadMessageState
{
  READ_HEADER,
  READ_MESSAGE,
};

struct MessageHeader
{
  uint32_t opcode;
  uint32_t length;
} __attribute__((packed));

}  //  namespace minirpc
#endif  // MINIRPC_DATAHEAD_H_


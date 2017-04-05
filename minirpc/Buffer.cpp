// Copyright 2016,xiong feng.

#include "Buffer.h"

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <sys/types.h>
#include "glog/logging.h"

using namespace minirpc;

template <class Type>
static Type DeserializeBufferToValue(Buffer *input)
{
  Type result;
  memcpy(reinterpret_cast<char*>(&result),
                 input->firstReadData(), sizeof(Type));
  input->readLeap(sizeof(Type));
  return result;
}

template <class Type>
static void SerializeBufferFromValue(const Type &value, Buffer *buffer)
{
  memcpy(reinterpret_cast<char*>(buffer->firstWriteData()),
                 &value, sizeof(Type));
  buffer->writeLeap(sizeof(Type));
}

static bool Recv(int fd, void *buf, size_t count, int *length)
{
  int ret = 0, errno_copy = 0;
  bool first_recv = true;
  *length = 0;
  while ((ret > 0 && count > 0) || first_recv)
  {
    first_recv = false;
    ret = ::recv(fd, reinterpret_cast<char*>(buf) + (*length), 
		    count, MSG_DONTWAIT);
    errno_copy = errno;
    if (ret == 0)
    {
      // socket has been closed
      // LOG(INFO) << "recv close: " << strerror(errno_copy);
      *length = 0;
      return true;
    }
    if (ret > 0)
    {
      count -= ret;
      *length += ret;
      if (count == 0)
          {
        return true;
      }
      continue;
    }
    if (errno_copy == EINTR)
    {
      continue;
    }
    if (errno_copy == EAGAIN || errno_copy == EWOULDBLOCK)
    {
      return true;
    }

    LOG(ERROR) << "recv error: " << strerror(errno_copy);
    return false;
  }

  return true;
}


static bool Send(int fd, const void *buf, size_t count, int *length)
{
  int ret = 0, errno_copy = 0;
  bool first_send = true;
  *length = 0;
  while ((ret > 0 && count > 0) || first_send )
  {
    first_send = false;
    ret = ::send(fd, ((char*)(buf) + (*length)), count,
		    MSG_NOSIGNAL | MSG_DONTWAIT);
    errno_copy = errno;
    if (ret > 0)
    {
      count -= ret;
      *length += ret;
      continue;
    }
    if (ret == 0)
    {
      return true;
    }
    if (errno_copy == EINTR)
    {
      continue;
    }
    if (errno_copy == EAGAIN || errno_copy == EWOULDBLOCK)
    {
      return true;
    }
    LOG(ERROR) << "send error: " << strerror(errno_copy);
    return false;
  }

  return true;
}


Buffer::Buffer()
  : read_index_(0),
    write_index_(0)
{
  buffer_.reserve(buffer_size_);
}

Buffer::~Buffer()
{
}

int Buffer::Read(int fd)
{
  int to_read = 0;
  if (ioctl(fd, FIONREAD, &to_read) == -1)
  {
    LOG(ERROR) << "ioctl error: " << strerror(errno);
    return -1;
  }
  if (static_cast<uint32_t>(to_read) > (buffer_.capacity() - write_index_))
  {
    resize(to_read);
  }
  int32_t length = 0;
  //LOG(INFO) << "Read count "<<to_read;
  if (!Recv(fd, firstWriteData(), to_read, &length))
  {
    return -1;
  }
  write_index_ += length;
  return length;
}

int Buffer::Write(int fd)
{
  int send_length = 0, length = 0;
  LOG(INFO)<< "Write index "<<write_index_ <<" Read index "<< read_index_;
  while (true)
  {

    if (!Send(fd, firstReadData(), write_index_ - read_index_, &length))
    {
      return -1;
    }
    read_index_ += length;
    send_length += length;
    if (read_index_ == write_index_)
    {
      break;
    }
  }
  return send_length;
}

void Buffer::resize(int resize)
{
  if (resize < buffer_size_)
  {
    resize = buffer_size_;
  }
  buffer_.resize(buffer_.capacity() + resize);
}

void Buffer::readLeap(int size)
{
  if (size + read_index_ < buffer_.capacity())
  {
    read_index_ += size;
  }
}

void Buffer::writeLeap(int size)
{
  if (size + write_index_ < buffer_.capacity())
  {
    write_index_ += size;
  }
}

void Buffer::serializeuint32_t(uint32_t value)
{

  value = ::htonl(value);
  SerializeBufferFromValue<uint32_t>(value, this);
}

uint32_t Buffer::deserializeuint32_t()
{
  uint32_t value = DeserializeBufferToValue<uint32_t>(this);
  return ::ntohl(value);
}

void Buffer::serializeMessage(const ::google::protobuf::Message *message)
{

  message->SerializeToArray(firstWriteData(), message->ByteSize());
  writeLeap(message->ByteSize());
}

bool Buffer::deserializeMessage(::google::protobuf::Message *message, 
		uint32_t length)
{
  bool result = message->ParseFromArray(firstReadData(), length);
  readLeap(length);
  return result;
}



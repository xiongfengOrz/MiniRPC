// Copyright 2016,xiong feng.

#ifndef MINIRPC_BUFFER_H_
#define MINIRPC_BUFFER_H_

#include <vector>

#include <google/protobuf/message.h>


namespace minirpc
{

class Buffer // copyable
{
 static const int buffer_size_ = 1024;

 public:
  Buffer();
  ~Buffer();

  int Read(int fd);
  int Write(int fd);

  void clear()
  {
    buffer_.clear();
    read_index_ = write_index_ = 0;
  }

  char* content()
  {
    return &(buffer_[0]);
  }

  char* firstReadData()
  {
    return &(buffer_[read_index_]);
  }

  char* firstWriteData()
  {
    return &(buffer_[write_index_]);
  }

  size_t size()
  {
    return (write_index_ - read_index_);
  }

  void readLeap(int size);

  void writeLeap(int size);

  size_t endPosition() const
  {
    return write_index_;
  }

  bool isReadComplete() const
  {
    return (read_index_ == write_index_);
  }

  void serializeuint32_t(uint32_t value);

  uint32_t deserializeuint32_t();

  void serializeMessage(const ::google::protobuf::Message *message);

  bool deserializeMessage(::google::protobuf::Message *message,
		  uint32_t length);
  void resize(int resize);

 private:
  std::vector<char> buffer_;
  size_t read_index_;
  size_t write_index_;
};

}//  namespace minirpc
#endif  // MINIRPC_BUFFER_H_


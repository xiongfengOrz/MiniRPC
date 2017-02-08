#ifndef MINIRPC_ITERNETADDRESS_H_
#define MINIRPC_ITERNETADDRESS_H_

#include <arpa/inet.h>
#include <string>

namespace minirpc
{

class NetAddress
{
 public:
  NetAddress();
  NetAddress(const std::string &host, int port);
  explicit NetAddress(const struct sockaddr_in &address);

  const struct sockaddr_in* address() const
  {
    return &address_;
  }

  const std::string& DebugString() const;
 private:
  void Init();

 private:
  struct sockaddr_in address_;
  std::string debug_string_;
};

} // namespace minirpc
#endif  // MINIRPC_ITERNETADDRESS_H_


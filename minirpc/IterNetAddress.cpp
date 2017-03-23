#include "IterNetAddress.h"
#include <string.h>
#include <sstream>
#include "glog/logging.h"

using namespace minirpc;
using std::string;

NetAddress::NetAddress()
{
}

NetAddress::NetAddress(const string &host, int port)
{
  bzero(&address_, sizeof(address_));
  address_.sin_family = AF_INET;
  inet_pton(AF_INET, host.c_str(), &(address_.sin_addr));
  address_.sin_port = htons(port);
  Init();
}

NetAddress::NetAddress(const struct sockaddr_in &address)
  : address_(address)
{
  Init();
}

void NetAddress::Init()
{
  char buffer[30];
  inet_ntop(AF_INET, &address_.sin_addr, buffer, sizeof(buffer));
  std::stringstream stream;
  stream << ntohs(address_.sin_port);
  std::string port = stream.str();
  debug_string_ = std::string(buffer) + ":" + port;
}

const string& NetAddress::DebugString() const
{
  return debug_string_;
}



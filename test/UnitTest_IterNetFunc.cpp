#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "minirpc/IterNetFunc.h"
#include "glog/logging.h"

using namespace minirpc;

int main(int argc, char *argv[]) 
{
  NetAddress listen_address_("127.0.0.1", 22202);
  int listen_fd_ = NetFunc::Listen(listen_address_);
  LOG(INFO)<<"LISTEN_FD "<< listen_fd_;
  struct sockaddr_in address;
  int idleFd_=6,fd=7;
  bool result = NetFunc::Accept(listen_fd_, &address, &fd, &idleFd_);
  if (result == false) 
  {
	LOG(ERROR)<< "Accept error";
  }
  if (fd == 0) 
  {
	LOG(ERROR)<< "Accept fd=0";
  }
  return 0;
}

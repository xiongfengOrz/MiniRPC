#ifndef MINIRPC_ITERNETFUNC_H_
#define MINIRPC_ITERNETFUNC_H_

#include  "IterNetAddress.h"
#include <sys/socket.h>
#include <string>

struct sockaddr_in;

namespace minirpc
{

class NetFunc
{
 public:
  static int Connect(const NetAddress &address);

  static int Listen(const NetAddress &address);

  static bool Accept(int listen_fd, struct sockaddr_in *addr,
		  int *fd, int *idleFd);

  static bool SetNonBlocking(int fd);
};

}  // namespace minirpc
#endif  //  MINIRPC_ITERNETFUNC_H_


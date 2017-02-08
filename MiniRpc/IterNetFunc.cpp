#include "IterNetFunc.h"

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include <string.h>
#include "glog/logging.h"


using namespace minirpc;


static const uint32_t kConnectTimeoutSecond = 5;


static bool is_connected(int fd, const fd_set *read_events, 
		const fd_set *write_events)
{
  int error_save = 0;
  socklen_t length = sizeof(error_save);
  errno = 0;
  if (!FD_ISSET(fd, read_events) &&!FD_ISSET(fd, write_events))
  {
    return false;
  }
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error_save, &length)< 0)
  {
    return false;
  }
  errno = error_save;
  return (error_save == 0);
}

int NetFunc::Connect(const NetAddress &address)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
    LOG(ERROR) << "create socket error: " << strerror(errno);
    return -1;
  }
  if (!NetFunc::SetNonBlocking(fd))
  {
    LOG(ERROR) << "SetNonBlocking fail";
    ::close(fd);
    return -1;
  }
  const struct sockaddr_in *addr = address.address();
  int ret = ::connect(fd, (struct sockaddr *)(addr), sizeof(*addr));
  if (ret == 0)
  {
    return fd;
  }
  // time-out
  fd_set read_events, write_events, exception_events;
  struct timeval tv;
  FD_ZERO(&read_events);
  FD_SET(fd, &read_events);
  write_events = read_events;
  exception_events = read_events;
  tv.tv_sec = kConnectTimeoutSecond;
  tv.tv_usec = 0;
  int result = ::select(fd + 1, &read_events, &write_events,
		  &exception_events, &tv);
  if (result < 0)
  {
    LOG(ERROR) << "select fail: " << strerror(errno);
    ::close(fd);
    return -1;
  }
  if (result == 0)
  {
    LOG(ERROR) << "connect time out";
    ::close(fd);
    return -1;
  }
  if (is_connected(fd, &read_events, &write_events))
  {
    LOG(INFO) << "connect to " << address.DebugString()
      << " success";
    return fd;
  }
  LOG(ERROR) << "connect time out";
  ::close(fd);
  return -1;
}

int NetFunc::Listen(const NetAddress &address)
{
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
  {
    LOG(ERROR) << "create socket error: " << strerror(errno);
    return -1;
  }

  int one = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			  &one, sizeof(one)) < 0)
  {
    return -1;
  }
  if (!NetFunc::SetNonBlocking(fd))
  {
    return -1;
  }

  const struct sockaddr_in *addr = address.address();
  if (::bind(fd, (struct sockaddr *)(addr), sizeof(*addr)) < 0)
  {
    LOG(ERROR) << "bind socket to " << address.DebugString()
	    << " error: " << strerror(errno);
    return -1;
  }

  if (::listen(fd, 10000) < 0)
  {
    LOG(ERROR) << "listen socket to " << address.DebugString()
	    << " error: " << strerror(errno);
    return -1;
  }

  return fd;
}

bool NetFunc::Accept(int listen_fd,struct sockaddr_in *addr, 
		int *fd, int *idleFd)
{
  int accept_fd = 0, errno_copy = 0;
  socklen_t length = sizeof(*addr);

  while (true)
  {
    accept_fd = ::accept(listen_fd, (struct sockaddr *)(addr),
		    &length);
    if (accept_fd > 0)
    {
      break;
    }
    errno_copy = errno;
    if (accept_fd < 0)
    {
      if (errno_copy == EINTR)
      {
        continue;
      }
      else if (errno_copy == EAGAIN)
      {
        *fd = 0;
        return true;
      }
      else if (errno_copy == EMFILE)
      {
        ::close(*idleFd);
        *idleFd = ::accept(listen_fd, NULL, NULL);
        ::close(*idleFd);
        *idleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
      }
      else
      {
        LOG(ERROR) << "Fail to accept, "<< " error: "
		<< strerror(errno_copy);
        return false;
      }
    }
  }

  if (!NetFunc::SetNonBlocking(accept_fd))
  {
    ::close(accept_fd);
    return false;
  }


  *fd = accept_fd;
  return true;
}



bool NetFunc::SetNonBlocking(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
  {
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) ==-1 )
  {
    return false;
  }
  return true;
}



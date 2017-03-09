#include "minirpc/ReactorLoop.h"
#include "glog/logging.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <time.h>


using namespace minirpc;



void testFunc1()
{
  LOG(INFO)<< "time out";
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", static_cast<pid_t>(::syscall(	getpid())), static_cast<pid_t>(::syscall(SYS_gettid)));
  ReactorLoop loop;
  struct timespec ts = { 0, 0 };
  ts.tv_sec = static_cast<time_t>(5);
  ts.tv_nsec = 0;
  ::nanosleep(&ts, nullptr);

  loop.wakeUp();

  ts.tv_sec = static_cast<time_t>(3);
  ts.tv_nsec = 0;
  ::nanosleep(&ts, nullptr);
  
}


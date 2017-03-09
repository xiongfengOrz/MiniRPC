#include "minirpc/ReactorLoop.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

using namespace minirpc;
 
//to fix 通过测试发现的一个问题，如果我像下面这样编写程序，那么main函数中的loop变量其实会重用testFunc1中的空间，
//就会导致两个loop的地址一致，程序中的判断就会有问题
//

void testFunc1()
{
  //printf("threadFunc(): pid = %d, tid = %d\n", static_cast<pid_t>(::syscall(	getpid())), static_cast<pid_t>(::syscall(SYS_gettid)));
   
  ReactorLoop loop;  
  //loop.startInOtherThread();	
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", static_cast<pid_t>(::syscall(	getpid())), static_cast<pid_t>(::syscall(SYS_gettid)));

  testFunc1();
   
  ReactorLoop loop;
  loop.startLoop();// main thread will stick in there because loop

  
}

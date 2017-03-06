#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "RpcChannel.h"
#include "RpcController.h"
#include <sys/socket.h>
#include <memory>
#include <string>
#include "minirpc/Task.h"
#include "minirpc/RpcClient.h"
#include "minirpc/IterNetAddress.h"
#include "minirpc/RpcConnectionFactory.h"
#include "minirpc/IterNetFunc.h"
#include "glog/logging.h"
#include "minirpc/Condition.h"
#include "minirpc/DataHead.h"
#include "echo.pb.h"
#include "echo_opcode.h"

using namespace minirpc;

/* values */
volatile int timerexpired=0;
int speed=0;
int failed=0;
long long bytes=0;
int EACH_CLIENT_TIMES=30;
int clients=1;
int benchtime=30;
int mypipe[2];


static const struct option long_options[]=
{
 {"time",required_argument,NULL,'t'},
 {"help",no_argument,NULL,'?'},
 {"version",no_argument,NULL,'V'},
 {"proxy",required_argument,NULL,'p'},
 {"clients",required_argument,NULL,'c'},
 {NULL,0,NULL,0}
};

/* prototypes */
static void benchcore();
static int bench(void);
static void echo_done(echo::EchoResponse* resp, Condition *monitor) ;


static void alarm_handler(int signal)
{
   timerexpired=1;
}

static void usage(void)
{
   fprintf(stderr,
  "webbench [option]... URL\n"

  "  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
  "  -p|--proxy <server:port> Use proxy server for request.\n"
  "  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
  "  -?|-h|--help             This information.\n"
  "  -V|--version             Display program version.\n"
  );
};

int main(int argc, char *argv[])
{
 int opt=0;
 int options_index=0;

 if(argc==1)
 {
    usage();
    return 2;
 }

 while((opt=getopt_long(argc,argv,"912Vfrt:p:c:?h",long_options,&options_index))!=EOF )
 {
  switch(opt)
  {
   case  0 : break;
   case 't': EACH_CLIENT_TIMES=atoi(optarg);break;
   case ':':
   case 'h':
   case '?': usage();return 2;break;
   case 'c': clients=atoi(optarg);break;
  }
 }



 if(clients==0) clients=1;
 if(benchtime==0) benchtime=60;

 /* print bench info */
 printf("\nBenchmarking: ");
 printf("\n");
 if(clients==1) 
   printf("1 client");
 else
   printf("%d clients",clients);

 printf(", running %d sec", benchtime);

 printf(", running %d times", EACH_CLIENT_TIMES);

 printf(".\n");
 return bench();
}

static void echo_done(echo::EchoResponse* resp, Condition *monitor)
{
  //LOG(INFO) << "response: " << resp->response();
  if(resp->response()!="hello"){
     LOG(ERROR) << "response error";
     failed++;
  }

  monitor->notify();
}

/* vraci system rc error kod */
static int bench(void)
{
  int i,j;
  long long k;
  pid_t pid=0;
  FILE *f;

  /* create pipe */
  if(::pipe(mypipe))
  {
    LOG(ERROR)<< "pipe error";
    return 3;
  }

  /* fork childs */
  for(i=0;i<clients;i++)
  {
     pid=::fork();
     if(pid <= (pid_t) 0)
     {
       ::sleep(1); /* make childs faster */
       break;
     }
  }

  if( pid< (pid_t) 0)
  {
    fprintf(stderr,"problems forking worker no. %d\n",i);
    LOG(ERROR)<< "fork error";
    return 3;
  }

  if(pid== (pid_t) 0)
  {
    /* I am a child */
   benchcore();
     /* write results to pipe */
   f=fdopen(mypipe[1],"w");
   if(f==NULL)
   {
     LOG(ERROR)<<"open pipe for writing failed.";
     return 3;
   }
   /* fprintf(stderr,"Child - %d %d\n",speed,failed); */
   fprintf(f,"%d %d %lld\n",speed,failed,bytes);
   fclose(f);
   return 0;
  }
  else
  {
    f=fdopen(mypipe[0],"r");
    if(f==NULL)
    {
      LOG(ERROR)<<"open pipe for reading failed.";
      return 3;
    }
    ::setvbuf(f,NULL,_IONBF,0);
    speed=0;
    failed=0;
    bytes=0;

    while(1)
    {
      pid=fscanf(f,"%d %d %lld",&i,&j,&k);
      if(pid<2)
      {
        fprintf(stderr,"Some of our childrens died.\n");
        break;
      }
      speed+=i;
      failed+=j;
      bytes+=k;
      if(--clients==0) break;
    }
    fclose(f);
    printf(" %d failed.\n",failed);
  }
  return i;
}

int time_substract(struct timeval *result, struct timeval *begin,struct timeval *end)

{
  if(begin->tv_sec > end->tv_sec)    
    return -1;
  if((begin->tv_sec == end->tv_sec) && (begin->tv_usec > end->tv_usec))    
    return -2;
  result->tv_sec  = (end->tv_sec - begin->tv_sec);
  result->tv_usec = (end->tv_usec - begin->tv_usec);
  if(result->tv_usec < 0)
  {
    result->tv_sec--;
    result->tv_usec += 1000000;
  }
  return 0;
}


void benchcore()
{
  int numbers=EACH_CLIENT_TIMES;
  struct sigaction sa;

  sa.sa_handler=alarm_handler;
  sa.sa_flags=0;
  if(sigaction(SIGALRM,&sa,NULL))
    exit(3);
  alarm(benchtime);//30s


  ReactorLoop loop;
  Mutex mutex,loop_mutex;
  Condition monitor(mutex);
  Condition loop_monitor(loop_mutex);
  std::string host="127.0.0.1";
  int port=22829;
  loop.startInOtherThread(&loop_monitor);
  loop_monitor.wait();

  echo::EchoRequest request;
  request.set_message("hello");

  RpcController rpc_controller;
  RpcChannel rpc_channel(&loop,host, port);
  rpc_channel.connect();
  echo::EchoService::Stub stub(&rpc_channel);
  struct timeval start,stop,diff;
  memset(&start,0,sizeof(struct timeval));
  memset(&stop,0,sizeof(struct timeval));
  memset(&diff,0,sizeof(struct timeval));

  gettimeofday(&start,0);
  while(numbers)
  {
    numbers--;
    if(timerexpired)
    {
      if(failed>0)
      {
        failed--;
      }
      return;
    }
    echo::EchoResponse response;
    stub.Echo(&rpc_controller, &request, &response,
            ::google::protobuf::NewCallback(::echo_done, &response, &monitor));
    monitor.wait();
    if (rpc_controller.Failed())
    {
    LOG(ERROR) << "response = " << rpc_controller.ErrorText();
    failed++;
    continue;
    }
  }
  gettimeofday(&stop,0);
  time_substract(&diff,&start,&stop);
  printf("Total time : %d s,%d us\n",(int)diff.tv_sec,(int)diff.tv_usec);
}

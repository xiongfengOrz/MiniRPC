#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtest/gtest.h>
#include <string>
#include <pthread.h>
#include "Thread.h"
#include "glog/logging.h"

namespace minirpc
{
class userThreadWorker : public ThreadWorker
{
public:
    userThreadWorker()=default;
    ~userThreadWorker()=default;
    void run()
    {
		LOG(INFO)<<"user thread run";
	}	
};
class ThreadTest : public testing::Test 
{
 public:
  void SetUp() 
  {
	  
  }

  void TearDown() 
  {
  }
};

TEST_F(ThreadTest, ThreadRunTest) 
{
  userThreadWorker user;
  Thread userThread(&user);
  userThread.start();
  
};
}

int main(int argc, char *argv[]) 
{
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

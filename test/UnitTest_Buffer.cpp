#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtest/gtest.h>
#include <list>
#include <string>
#include "Buffer.h"
#include "glog/logging.h"

namespace minirpc
{
class BufferTest : public testing::Test 
{
 public:
  void SetUp() 
  {
  }

  void TearDown() 
  {
  }
};

TEST_F(BufferTest, ConvertTest) 
{
}

TEST_F(BufferTest, SerializeTest) 
{
  
  Buffer buffer;
  uint32_t i = 100, output;
  ASSERT_TRUE(buffer.isReadComplete());
  buffer.serializeuint32_t(i);
  ASSERT_FALSE(buffer.isReadComplete());
  ASSERT_EQ(sizeof(uint32_t), buffer.endPosition());
  output = buffer.deserializeuint32_t();
  ASSERT_TRUE(buffer.isReadComplete());
  ASSERT_EQ(i, output);
  
};
}

int main(int argc, char *argv[]) 
{
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

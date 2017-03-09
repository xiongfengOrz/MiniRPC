#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtest/gtest.h>
#include <list>
#include <string>
#include "echo.pb.h"
#include "ProtobufHelper.h"
#include "Buffer.h"

using namespace std;
namespace minirpc
{
class MessageUtilityTest : public testing::Test 
{
 public:
  void SetUp() 
  {
  }

  void TearDown() 
  {
  }
};

TEST_F(MessageUtilityTest, TestDecodeEncode) 
{
  ::echo::EchoResponse response, result;
  Buffer content;
  MessageHeader header;
  response.set_response("test");
  ASSERT_TRUE(encodeMessageToBuffer(1, &response, &content));
  ASSERT_TRUE(decodeMessageHeader(&content, &header));
  ASSERT_EQ(header.length, response.ByteSize());
  ASSERT_EQ(1, header.opcode);
  ASSERT_TRUE(content.deserializeMessage(&result, header.length));
}

};

int main(int argc, char *argv[]) 
{
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

#ifndef MINIRPC_PROTOBUFHELPER_H_
#define MINIRPC_PROTOBUFHELPER_H_

#include "DataHead.h"
#include <google/protobuf/message.h>

namespace minirpc
{

static const uint32_t kSuccess = 0;
static const uint32_t kDecodeMessageError = 100;
static const uint32_t kRecvMessageNotCompleted = 101;
static const uint32_t kCannotFindMethodId = 102;
static const uint32_t kServiceNotRegistered = 103;
static const uint32_t kSendMessageNotCompleted = 104;
static const uint32_t kCannotFindRequestId = 105;
static const uint32_t kHandlePacketError = 106;
static const uint32_t kSendMessageError = 200;
static const uint32_t kRecvMessageError = 201;

class Buffer;
bool encodeMessageToBuffer(uint32_t opcode,
	const google::protobuf::Message *message, Buffer *output);

bool decodeMessageHeader(Buffer *input, MessageHeader *message_header);

uint32_t decodeMessage(Buffer *input,
	MessageHeader *header, ReadMessageState *state);

uint32_t write(Buffer *output, int fd);
}
#endif  // MINIRPC_PROTOBUFHELPER_H_


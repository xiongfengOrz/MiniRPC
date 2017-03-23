//#include "eventrpc/utility.h"
#include "glog/logging.h"

//#include "eventrpc/string_utility.h"
#include "Buffer.h"
#include "DataHead.h"
#include "ProtobufHelper.h"
//#include "eventrpc/message_utility.h"


namespace minirpc 
{

	bool encodeMessageToBuffer(uint32_t opcode, const google::protobuf::Message *message, Buffer *output)
	{
		if (output == nullptr)
		{
			return false;
		}
		output->serializeuint32_t(opcode);
		output->serializeuint32_t(message->ByteSize());
		LOG(INFO) << " message byte size "<<message->ByteSize();
		output->serializeMessage(message);
		return true;
	}

	bool decodeMessageHeader(Buffer *input, MessageHeader *message_header)
	{
		if (message_header == nullptr)
		{
			return false;
		}
		message_header->opcode = input->deserializeuint32_t();
		message_header->length = input->deserializeuint32_t();
		return true;
	}

	uint32_t decodeMessage(Buffer *input, MessageHeader *header, ReadMessageState *state) {
		while (true)
		{
			if (*state == READ_HEADER)
			{
				if (input->size() < sizeof(MessageHeader))
				{
					LOG(INFO) << "kRecvMessageNotCompleted";
					return kRecvMessageNotCompleted;
				}
				decodeMessageHeader(input, header);
				*state = READ_MESSAGE;
			}
			if (*state == READ_MESSAGE)
			{
				*state = READ_HEADER;
				if (input->size() < header->length)
				{
					LOG(INFO) << "kRecvMessageNotCompleted" << ", opcode: " << header->opcode
						<< ", size: " << header->length << ", buffer size:" << input->size();
					return kRecvMessageNotCompleted;
				}
				return kSuccess;
			}
		}
		LOG(FATAL) << "should not reach here";
		return kSuccess;
	}

	uint32_t write(Buffer *output, int fd)
	{
		if (output->Write(fd) == -1)
		{
			return kSendMessageError;
		}
		if (output->isReadComplete() == true)
		{
			output->clear();
			return kSuccess;
		}
		return kSendMessageNotCompleted;
	}
};


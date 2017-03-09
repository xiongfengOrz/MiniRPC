#include <string.h>
#include <sstream>
#include "minirpc/IterNetAddress.h"
#include "glog/logging.h"

using namespace minirpc;
int main(int argc, char *argv[]) 
{
	NetAddress listen_address_("127.0.0.1", 22202);
	LOG(INFO)<< listen_address_.DebugString();
	return 1;
}

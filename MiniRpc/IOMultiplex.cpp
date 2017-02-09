#include "IOMultiplex.h"
#include "Event.h"

using namespace minirpc;

IOMultiplex::IOMultiplex() = default;

IOMultiplex::~IOMultiplex() = default;

bool IOMultiplex::searchEvent(Event* Event) const
{
  auto it = Events_.find(Event->fd());
  return it != Events_.end() && it->second == Event;
}





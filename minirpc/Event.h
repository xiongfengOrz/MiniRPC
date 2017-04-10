// Copyright 2016,xiong feng.
#ifndef MINIRPC_EVENT_H_
#define MINIRPC_EVENT_H_

#include "messageHandler.h"

#include <functional>
#include <memory>
#include <string>

namespace minirpc
{

class Event
{
 public:
  explicit Event(int fd);
  ~Event();

  Event(const Event&) = delete;
  Event& operator=(const Event&) = delete;

  void handleEvent();
  void add();
  void del();
  bool isAdded() const;

  void setReadHandler(EventHandler* handler);
  void setWriteHandler(EventHandler* handler);
  void setCloseHandler(EventHandler* handler);
  void setErrorHandler(EventHandler* handler);

  int fd() const;
  int events() const;
  void set_revents(int revt);
  bool isNoneEvent() const;

  void enableReading();
  void disableReading();
  void enableWriting();
  void disableWriting();
  void disableAll();
  bool isWriting() const;
  bool isReading() const;

  // for IOMultiplex
  int index();
  void set_index(int idx);

  // for debug
  std::string reventsToString() const;
  std::string eventsToString() const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;

};

}  // namespace minirpc
#endif  // MINIRPC_EVENT_H_


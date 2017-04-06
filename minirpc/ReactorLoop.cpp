// Copyright 2016,xiong feng.

#include "ReactorLoop.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <signal.h>
#include <memory>
#include <vector>
#include <string>
#include <assert.h>
#include <type_traits>
#include <stdio.h>
#include <algorithm>

#include "Event.h"
#include "glog/logging.h"
#include "Mutex.h"
#include "Task.h"
#include "Thread.h"
#include "EpollMultiplex.h"


using namespace minirpc;


class wakeUpEventHandler:public EventHandler
{
 public:
  explicit wakeUpEventHandler(int wakeUpPipeFd)
    :wakeUpPipeFd_(wakeUpPipeFd)
  {
  }

  virtual ~wakeUpEventHandler()
  {
  }

  void handlePacket() override
  {
    //LOG(INFO)<<"handlePacket";
    uint64_t one = 1;
    ssize_t n = ::read(wakeUpPipeFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
      LOG(ERROR) << "ReactorLoop::handleRead() reads "
              << n << " bytes instead of 8";
    }
  }

 private:
  int wakeUpPipeFd_;
};

class StopTask : public Task
{
 public:
  explicit StopTask(ReactorLoop::Impl *impl)
    : impl_(impl)
  {
  }
  
  virtual ~StopTask()
  {
  }

  std::string taskName()
  {
    return "StopTask";
  }
  void handle() const override;

 private:
  ReactorLoop::Impl *impl_;
};



static pthread_key_t   key;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;
static const int kPollTimeMs = 5000;

static void threadLocalfree(void* p)
{
  LOG(INFO) << "destructor excuted in thread";
}

static void threadInit(void)
{
  pthread_key_create(&key, threadLocalfree);
}

static IOMultiplex* ioMultiplexFactory()
{
  return new EPollIOMultiplex();
}

static int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG(INFO) << "Failed in eventfd";
    abort();
  }
  return evtfd;
}


class ReactorLoop::Impl : public ThreadWorker
{
 public:
  Impl();
  ~Impl();

  void startLoop();
  void stopLoop();
  //thread
  bool isInLoopThread() const;
  void startInOtherThread(Condition *monitor);
  void stopInOtherThread();

  //Event  first test

  void addEvent(Event* events);
  void ModifyEvent(Event* events);
  void deleteEvent(Event* events);
  bool searchEvent(Event* events);

  //Task
  void wakeUp();
  void handleRead();
  void runTask(std::shared_ptr<Task> task);
  void handleWaitingTasks();
  void queueInLoop(std::shared_ptr<Task> task);
  size_t queueSize();

 private:
  void run();
  void printActiveEvents() const;
  void close();

 private:
  bool started_;
  bool stop_;
  bool isHandlingEvents_;
  bool isHandlingTasks_;
  bool blockInPoll_;
  int iteration_;
  int threadId_;
  Thread thread_;

  int wakeUpPipeFd_;
  std::unique_ptr<IOMultiplex> ioMul_;
  std::unique_ptr<Event> wakeUpEvents_;
  std::vector<Event*> activeEvents_;
  Mutex mutex_;
  std::vector<std::shared_ptr<Task>> waitingTasks_;
  std::string name_;
  Condition *monitor_;
  std::shared_ptr<StopTask> stoptask_; //not good  to fix
};

ReactorLoop::Impl::Impl()
    :  started_(false),
    stop_(false),
    isHandlingEvents_(false),
    isHandlingTasks_(false),
    blockInPoll_(false),
    iteration_(0),
    threadId_(static_cast<pid_t>(::syscall(SYS_gettid))),
    thread_(this),
    ioMul_(ioMultiplexFactory()),
    stoptask_(new StopTask(this))

{
  //init
  LOG(INFO) << "ReactorLoop " << this << " created  in thread " << threadId_;
  pthread_once(&init_done, threadInit);

  wakeUpPipeFd_ = createEventfd();
  LOG(INFO) << "wakeUpPipeFd_ " << wakeUpPipeFd_;
  wakeUpEvents_.reset(new Event(wakeUpPipeFd_));
  wakeUpEvents_->setReadHandler(new wakeUpEventHandler(wakeUpPipeFd_));
  addEvent(wakeUpEvents_.get());
}

ReactorLoop::Impl::~Impl()
{
}

void ReactorLoop::Impl::close()
{
  LOG(INFO) << "ReactorLoop " << this << " die with thread " << threadId_;
  started_ = false;
  deleteEvent(wakeUpEvents_.get());
  ::close(this->wakeUpPipeFd_);
}


bool ReactorLoop::Impl::isInLoopThread() const
{
  return threadId_ == static_cast<pid_t>(::syscall(SYS_gettid));
}

void ReactorLoop::Impl::startInOtherThread(Condition *monitor)
{
  if (started_ == true)
  {
    LOG(ERROR) << "Already start";
    return;
  }
  monitor_ = monitor;
  thread_.start();
}

void ReactorLoop::Impl::run()
{
  threadId_ = static_cast<pid_t>(::syscall(SYS_gettid));
  LOG(INFO)<<"ReactorLoop "<< this <<" started in othter thread "<< threadId_;
  startLoop();
}

void ReactorLoop::Impl::startLoop()
{
  if (started_ == true)
  {
    LOG(ERROR)<< "ReactorLoop " << this << " Already in Loop";
    return;
  }

  if (static_cast<pid_t>(::syscall(SYS_gettid)) != threadId_)
  {
    LOG(ERROR) << "Not the same thread with Loop";
    return;
  }
  started_ = true;
  stop_ = false;
  LOG(INFO) << "ReactorLoop " << this << " start looping";

  while (!stop_)
  {
    blockInPoll_ = true;
    LOG(INFO) << this << " ReacatorLoop wait in the poll ";
    monitor_->notify();
    ioMul_->poll(kPollTimeMs, &activeEvents_);
    blockInPoll_ = false;

    LOG(INFO) << this <<" return form poll";
    if (stop_) break;
    ++iteration_;
    printActiveEvents();
    isHandlingEvents_ = true;

    for (auto it = activeEvents_.begin(); it != activeEvents_.end(); ++it)
    {
       (*it)->handleEvent();
    }
    activeEvents_.clear();

    isHandlingEvents_ = false;
    handleWaitingTasks();
    if (stop_) break;
  }

  LOG(INFO) << "ReactorLoop " << this << " stop looping";
  close();
  return;
}

//thread safe, this is the safe way to stop
void ReactorLoop::Impl::stopInOtherThread()
{
  if (stop_ == true)
  {
    LOG(ERROR) << "Already stop";
    return;
  }

  runTask(stoptask_);
  return;
}

void ReactorLoop::Impl::stopLoop()
{
  if (started_ && static_cast<pid_t>(::syscall(SYS_gettid)) != threadId_)
  {
    LOG(ERROR) << "Try ReactorLoop unsafe stop ";
    return;
  }
  stop_ = true;
  return;
}

void StopTask::handle() const
{
  impl_->stopLoop();
}

void ReactorLoop::Impl::runTask(std::shared_ptr<Task> task)
{
  if (static_cast<pid_t>(::syscall(SYS_gettid)) == threadId_)
  {
    LOG(INFO) << "Run Task Directly ";
    task->handle();
  }
  else
  {
    LOG(INFO) << "Queue Task  "<< task->taskName();
    queueInLoop(task);
  }
}

void ReactorLoop::Impl::queueInLoop(std::shared_ptr<Task> task)
{
  {
    MutexLock lock(mutex_);
    waitingTasks_.push_back(task);
  }

  if (blockInPoll_)
  {
    //LOG(INFO) << "WakeUp ReactorLoop "<< this;
    wakeUp();
  }
}

size_t ReactorLoop::Impl::queueSize()
{
  MutexLock lock(mutex_);
  return waitingTasks_.size();
}



//Event

void ReactorLoop::Impl::addEvent(Event* events)
{
  //From Linux Man:
  //While one thread is blocked in a call to epoll_pwait(),
  //it is possible for another thread to add a file
  //descriptor to the waited-upon epoll instance.
  ioMul_->addEvent(events);
}

void ReactorLoop::Impl::ModifyEvent(Event* events)
{
  ioMul_-> ModifyEvent(events);
}

void ReactorLoop::Impl::deleteEvent(Event* events)
{
  //From Linux Man:
  //If a file descriptor being monitored is closed
  //in another thread, the result is unspecified.
  pid_t curr_thread = static_cast<pid_t>(::syscall(SYS_gettid));
  if ( threadId_ != curr_thread )
  {
    LOG(ERROR)<< "Can not delete event from another thread " << curr_thread;
    return;
  }

  ioMul_->deleteEvent(events);
}

bool ReactorLoop::Impl::searchEvent (Event* events)
{
  return ioMul_->searchEvent(events);
}


void ReactorLoop::Impl::wakeUp()
{
  LOG(INFO)<<"wake up ReactorLoop "<<this;
  uint64_t one = 1;
  ssize_t n =  ::write(wakeUpPipeFd_, &one, sizeof(one));
  if (n != sizeof(one))
  {
    LOG(ERROR) << "ReactorLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}


void ReactorLoop::Impl::handleWaitingTasks()
{
  isHandlingTasks_ = true;
  {
    MutexLock lock(mutex_);
    for (size_t i = 0; i < waitingTasks_.size(); ++i)
    {
        LOG(INFO)<<"Handle waiting task " <<waitingTasks_[i]->taskName();
      waitingTasks_[i]->handle();
    }
  }
  waitingTasks_.clear();
  isHandlingTasks_ = false;
}


void ReactorLoop::Impl::printActiveEvents() const
{
  for (auto it = activeEvents_.begin(); it != activeEvents_.end(); ++it)
  {
    const Event* ch = *it;
    LOG(INFO) << "{" << ch->reventsToString() << "} ";
  }
}


ReactorLoop::ReactorLoop()
  //: impl_{ std::make_unique<Impl>() }  //only c++14
  : impl_( new Impl() )
{
}

ReactorLoop::~ReactorLoop()
{
}

//event
void ReactorLoop::addEvent(Event* events)
{
  impl_->addEvent(events);
}

void ReactorLoop::ModifyEvent(Event* events)
{
  impl_->ModifyEvent(events);
}

void ReactorLoop::deleteEvent(Event* events)
{
  impl_->deleteEvent(events);
}

bool ReactorLoop::searchEvent(Event* events)
{
  return impl_->searchEvent(events);
}

void ReactorLoop::wakeUp()
{
  impl_->wakeUp();
}
void ReactorLoop::runTask(std::shared_ptr<Task> task)
{
  impl_->runTask(task);
}

void ReactorLoop::startLoop()
{
  impl_->startLoop();
}

void ReactorLoop::stopLoop()
{
  impl_->stopLoop();
}

bool ReactorLoop::isInLoopThread() const
{
  return impl_->isInLoopThread();
}

void ReactorLoop::startInOtherThread(Condition *monitor)
{
  impl_->startInOtherThread(monitor);
}


void ReactorLoop::stopInOtherThread()
{
  impl_->stopInOtherThread();
}


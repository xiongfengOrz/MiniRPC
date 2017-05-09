
# MiniRpc

**MiniRpc 是个人在学习网络编程之余不断修改完善的练习之作，现已完成全部基本功能，但是还有不少不足之处。如果这份代码能够帮助到你学习，请让我知道，我会很高兴的 :)**

作者： Xiong Feng


# 总览：
  - 使用Protobuf来实现序列化，同时实现了两种利用Protobuf的实现方式，其中一种尽量减少Protobuf对代码的侵入性，支持同步和异步调用模式
  - Server端采用线程池模式，IO线程仅accept新连接，连接到来后由Worker线程接管处理
  - Client端提供对于异常调用的超时机制， Server端提供简易的过载保护
  - 提供服务发现及客户端负载均衡
  - 提供长连接和短连接模式


 

# 待完善：
  - 添加完善测试用例，做到测试100%覆盖
  
# 性能测试：
>代码编写过程中并没有太关注性能，功能完成后进行了简单的短连接测试
>使用Sample目录下的Search RPC C/S进行Echo RPC调用的压测，相当于Worker线程工作在Request-Reply模式的情况。

### 运行环境
    CPU：24 x Intel(R) Xeon(R) CPU E5-2620 v2 @ 2.40GHz
    内存：16 GB
    网卡：千兆网卡
    Client/Server机器之间PING值： 0.05ms
    Worker线程数：8
	工作模式：同步模式
	业务数据：小包
	连接模式： 短连接

### 性能测试结果(qps)
#### 10K


# 如何编译
#### Protobuf准备
MiniRpc必须依赖的第三方库只有Protobuf。在在编译前，系统需安装ProtoBuf。
同时项目需要也需要安装gtest，glog，但是你可以选择去掉相应的代码，并不影响功能。


#### 编译环境
  - Linux.
  - GCC-4.8.4及以上版本
  - Scons
  

#### 编译安装方法
进入MiniRpc根目录。
项目没有使用Makefile，而是使用SCons。Scons使用Python来编写，虽然性能不及Makefile，但胜在简单易懂，在项目不大的时候，是个不错的选择。
直接运行SCons即可。Scons的使用可以看[这里](http://scons.org/)。
 
# 如何使用
#### 编写proto文件
下面是sample目录下的proto文件部分样例。

```c++
message EchoRequest
{
  required string message = 1;
};

message EchoResponse
{
  required string response = 1;
};

service EchoService
{
  rpc Echo(EchoRequest) returns (EchoResponse);
};

```
同时在程序中添加标志用来指示包的类型
```c++
const static uint32_t CLIENT_ECHO  = 0x1;
const static uint32_t SERVER_ECHO  = 0x2;
```

#### 补充自己的代码
有两种使用Rpc的方法，一种需要使用者编写更多的代码，但是优点是对Protobuf的依赖较少，更换序列化框架更加方便。
另一种利用了Protobuf的特性，使用起来更加方便，下面给出对Protobuf依赖较多的样例代码。
##### Server

```c++
class EchoServerMessageHandler : public ServerMessageHandler 
{
 public:
  EchoServerMessageHandler(RpcConnection *connection)
    : ServerMessageHandler(connection) 
  {
  }

  bool handlePacket(const MessageHeader &header, Buffer* buffer) override
  {
    if (header.opcode != CLIENT_ECHO) 
    {
      LOG(ERROR) << "opcode error: " << header.opcode;
      return false;
    }
    echo::EchoRequest request;
    if (!buffer->deserializeMessage(&request, header.length)) 
    {
      LOG(ERROR) << "deserializeMessage error: " << header.length;
      return false;
    }
    LOG(INFO) << "request: " << request.message();
    echo::EchoResponse response;
    response.set_response(request.message());
    connection_->sendPacket(SERVER_ECHO, &response);
    return true;
  }
};
```

##### Client 

```c++
class EchoClientMessageHandler : public ServerMessageHandler 
{
 public:
  EchoClientMessageHandler(RpcConnection *channel,
                           Condition *monitor)
    : ServerMessageHandler(channel),
      monitor_(monitor) 
  {
  }

  bool handlePacket(const MessageHeader &header, Buffer* buffer) override
  {
    if (header.opcode != SERVER_ECHO) 
    {
      LOG(ERROR) << "opcode error: " << header.opcode;
      return false;
    }
    echo::EchoResponse response;
    if (!buffer->deserializeMessage(&response, header.length)) 
    {
      LOG(ERROR) << "deserializeMessage error: " << header.opcode;
      monitor_->notify();
      return false;
    }
    LOG(INFO) << "response: " << response.response() << ", count: " << kCount;
    if (kCount == kMaxConnection) 
    {
      monitor_->notify();
    }
    return true;
  }
 private:
  Condition *monitor_;
};
```



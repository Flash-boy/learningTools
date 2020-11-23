## mudou网络库解析   
### 写在前面    
mudou网络库是我当时学习某个开源项目叫做[Flamingo](https://github.com/balloonwj/flamingo)，这是一个纯c++开发的即时通讯软件，当时我并不知道mudou网络库，只是当初想要对c++服务器开发做一个深入了解，看看人家服务器端项目怎么写的，于是找到了这个项目。这个项目主要包括flamingoserver服务端,flamingoclient客户端两部分。其中flamingoserver服务端里面就包含了mudou库。其中的net和base文件夹就是属于mudou源码的大部分。我这里讲解的是mudou源码中关于整个网络库的一部分，我也去[mudou源码](https://github.com/chenshou/mudou)看过，核心这里还是讲解到的，所以我建议先看完本库的源码再去github上看陈硕老师的源码集合。     
由于当时我看flamingo源码时也不知道这块是属于mudou源码，所以也没有查资料，当时自己也花了很多时间摸索如何去看flamingo源码（第一次看一个完整的源码也没有经验），后来看反复看了很久弄通了整个服务器模块的逻辑，整个flamingo服务器端包括：    
- 网络库底层逻辑
- 数据库逻辑   
- 上层业务逻辑    
这里的网络库底层逻辑就是这里的mudou网络库，上层业务就是实现flamingo即时通讯类似qq这样功能软件所有处理的业务，数据库当然就是连接数据库这块。     
我也是通过阅读这个源码才了解到mudou库，这也是我学习到的第一个开源项目。所有写一遍文章，来记录自己对mudou库的理解，可能不是很到位，我会在给一张建议阅读mudou网络库的代码顺序，我觉得一个好的阅读顺序是学习源码的好方式。      
```
1.首先是base文件夹    
Singleton.h->Timestamp.h/cpp->ConfigFileReader.h/cpp->AsyncLog.h/cpp->Buffer.h/cpp->Endian.h->Callbacks.h->Sockets.h/cpp->InetAddress.h/cpp->    
channel.h/cpp->Poller.h/cpp->PollPoller.h/cpp->EpollPoller.h/cpp->SelectPoller.h/cpp->    
Timer.h/cpp->TimerId.h->TimerQueue.h/cpp->      

```  
以上只是个人的阅读顺序，希望你能有一个大概的网络API概念，包括套接字，bind(),listen(),connect()等函数，知道是干什么用的。然后阅读源码时候你会发现大佬写的代码都是很规范很漂亮的，文件夹的命名和文件的命名都是很规范，都能给你一定信息。我会带着上述的代码阅读顺序，逐个解析各个文件的作用，包括函数，和如何和其他连接起来的。    
### 阅读源码    
> 我觉得看懂一个类的作用和实现首先需要看的是类声明，先看类的成员变量，再看类的函数声明，再看类的成员函数的实现(细节)，最后总结归纳出该类的作用和对外接口。所以我会一次解读每个文件中每个类。每个类的分析都会有，类声明分析、重要类函数实现分析、类作用和对外接口，三个部分。    



## Singleton.h   
**1.类声明分析**    
```
template<typename T>
class Singleton
{
public:
	static T& Instance()
	{
		//pthread_once(&ponce_, &Singleton::init);
		if (nullptr == value_)
		{
			value_ = new T();
		}
		return *value_;
	}

private:
	Singleton();
	~Singleton() = default;

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

	static void init()
	{
		value_ = new T();
		//::atexit(destroy);
	}

	static void destroy()
	{
		delete value_;
	}

private:
	//static pthread_once_t ponce_;
	static T*             value_;
};

//template<typename T>
//pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = nullptr;
```
**2.重要类函数实现分析**    
这个是看名字就知道是个单例模式的类，准确说是个模板类，由于类声明和实现都在一起所以在这里直接看各个函数的作用。这个模板类成员函数是静态，也只有一个静态成员，指向模板类型的指针，同时构造函数是私有的也就是不能直接构造这个类的对象。只能用类的静态成员函数。对外提供了一个实例化函数**Instance()** 只会返回至始至终在内存中的某个对象，如果该对象已经存在，再次调用只会返回该对象，如果还没有该对象则会new一个新的然后返回。    
**3.类作用及对外接口总结**    
作用：当使用该类去得某个对象的时候在程序中只会出现一个该类型对象。使用的方式是直接**Signleton\<ClassType>::Instance()** 就可以取得对象。以后会再使用该方式取的对象永远是那一个。      

## Timestamp.h和Timestamp.cpp      
**1.类声明分析**    
![Timestamp](../Pictures/mudou_base-Timestamp1.png)    
该类是一个保存自1970年1月1日到某一时间的微秒数，可以计算一下足够我们使用，因为是用一个64位的数表示。
**2.重要类函数实现分析**    
构造函数默认的是一个微妙数是0的我们用来表示无效时间，还有一个显式构造函数。  
```
Timestamp& operator+=(Timestamp lhs)   # 将lhs对象的时间数加到该对象上，并返回该对象   
Timestamp& swap(Timestamp that)        # 交换该对象和that对象的时间数   
string toString() const;               # 以  "秒数：微秒数"  的字符形式返回该对象表示的时间
string toFormattedString(bool showMicroseconds = true) const; #  以 "年月日 时:分:秒:w微秒" 的形式返回该对象所表示的时间(如果参数为true则显示微秒，否则不显示)
static Timestamp now();                # 返回调用该函数时的当前时间对象
static Timestamp invalid();            # 返回一个无效的时间对象
```    
**3.类作用及对外接口总结**    
该类就是一个表示时间的类，为网络库里面的定时器事件提供对象。对外一般用该对象保存时间，作为参数使用    

## ConfigFileReader.h和ConfigFileReader.cpp       
**1.类声明分析**    
![ConfigFileReader](../Pictures/mudou_config.png)   
读取配置文件然后将配置以map的形式保存到对象中。说白了就是能够解析配置文件   
**2.重要类函数实现分析**   
```
CConfigFileReader(const char* filename)     # 构造对象的时候会自动调用loadFile()函数，加载对应的配置文件并把配置文件的配置以key,value的形式保存在该类的map中
char* getConfigName(const char* name);      # 得到配置的某个key所对应的value   
int setConfigValue(const char* name, const char*  value);   # 设置对应的key，value的值，并且会调用writeFile()写入对应的文件中   
```
**3.类作用及对外接口总结**  
这个类就是读取配置文件，把配置文件的每一行的=两步以字符串的形式读入map中，支持"#"作为注释，也支持空string左右空白，这是通过trimSpace()函数实现的。   
对外的接口就是构造对象之后通过**getConfigName()** 获取相应的配置值，和**setConfigValue()** 设置相应的key,value。支持类似下面的配置文件     
![ConfigFileReader](../Pictures/mudou_config2.png)    
其中#号是注释，而且空格是忽略的向  "listenip":"0.0.0.0" 会以这样的形式保存在map中，每行只以第一个=号为分隔key，value。   

## AsyncLog.h和AsyncLog.cpp     
**1.类声明分析**   
![AsyncLog](../Pictures/mudou_AsyncLog1.png)   
![AsyncLog](../Pictures/mudou_AsyncLog2.png)    
可以看到又是一个静态类，其实静态类也是类似于单例模式，因为你只能有一组静态成员变量，可以看到里面的成员，其中m_hLogFile是一个打开文件句柄，，表示要写入日志到某个文件，m_bToFile是输出日志到文件还是控制台，m_bTruncateLongLog表示是否截断长日志，一般日志保存在文件中一行就表示写了一次日志信息，如果一行太长，这里是一行最长256个字节。m_nFileRollSize是表示一个日志文件最长为多少，这里默认为10MB如果超过了就换一个文件，m_listLinesToWrite是一个list双向链表保存着要写入的日志，因为，日志是采用异步的方式进行写入文件的，在主线程里，每条日志都是放入这个list中，然后会有一个单独的线程去将list中的日志写入文件中。init()函数需要使用前调用，然后就是3个output函数是供外部调用的。   
**2.重要类函数实现分析**   
![AsyncLog](../Pictures/mudou_AsyncLog3.png)    
我们可以看到日志分了好多级别，只有高于当前日志级别也就是m_ncurrentLevel才能正常输入，但是最高级别CRITICAL除外，外部调用的时候我们只需要使用LOGT(...)之类的函数，支持格式化，也就是%s,%d，之类的，这是因为利用了宏定义，还有va_list。   
- init()  
  初始化函数，这是我们能够正确使用LOGF(...)之前需要做的工作。在函数内部主要是如果有文件名或者正确的值，那么m_strFileName就有效，否则就是一个空的字符串。然后就是init()会保存当前的进程ID,然后就是新建一个线程用来处理m_listLinesTowrite中的日志信息写入对应的文件。新线程绑定的是writeThreadProc()内部私有函数，该函数就是不断的从m_listLinesToWrite链表中拿出数据写入对应的文件中，如果没有日志信息可写会睡眠该线程。    
- uninit()    
  结束整个日志工作的函数，一旦调用，会等待写日志线程结束，然后关闭存储日志文件句柄，整个异步日志类停止工作。    
- setLevel()   
  设置整个日志过程中的日志级别默认系统开始时日志时INFO级别     
- isRunning()    
  判断写日志线程是否工作，当进入writeThreadPrco()工作线程绑定函数时m_bRunning设为True    
- output()    
  有3种日志输出文件，一种是普通信息，另外一种是加入了调用该函数的函数文件名，和行号，最后一种是日志信息的二进制写入文件。内部首先会调用makeLinePrefix()私有函数生成日志信息前缀形如"[日志级别][生成该日志时间][调用该函数的线程ID]"这样的前缀，后面才是真正的日志信息。然后处理日志正文，是通过va_list去读取格式字符串，得到一个完整的日志正文信息，然后如果开启了日志截断，则日志正文最多256个字符，如果是写入文件末尾追加一个换行符号，也就是日志文件每一行代表一个日志信息。然后利用锁和条件变量加入该信息到m_listLinesToWrite中，同时唤醒工作线程。如果日志级别是FATAL，那么会在工作台显示该日志，然后将该日志同步写入日志文件，然后终止整个程序。   
  ![AsyncLog](../Pictures/mudou_AsyncLog4.png)   
  这是我写的一个demo代码，下面的结果     
  ![AsyncLog](../Pictures/mudou_AsyncLog5.png)    
  可以看到默认的日志等级是INFO,低于INFO的DEBUG级别并没有输出，同时截断了过长的日志。   
  ![AsyncLog](../Pictures/mudou_AsyncLog6.png)    
  可以看到有一个日志文件以"文件名.日期.进程ID.log"的形式保存日志文件。   
  
**3.类作用及对外接口总结**    
这就是一个普通的异步日志类文件，对外的接口就是调用init()函数，然后使用宏定义，也就是    
![AsyncLog](../Pictures/mudou_AsyncLog3.png)    
这里面的LOGI(...)等形式的函数就可以输出日志到对应的文件。里面的技术主要是一个异步日志，采用一个线程专门写日志到文件，在其他调用LOGI(...)函数中只是包装好日志信息到内存链表中，然后交给线程处理函数写日志到文件。     


## Buffer.h和Buffer.cpp        
**1.类声明分析**    
![buffer](../Pictures/mudou_buffer1.png)    
整个buffer其实就是由一个vector组成的空间，然后外加两个索引分别表示可读和可写     
**2.重要类函数实现分析**    
![buffer](../Pictures/mudou_buffer2.png)     
一个vector被readerIndex_,writerIndex_分隔成3部分，第一部分为前置保留的区域[0,readerIndex_)，然后就是[readerIndex_,writerIndex_)表示的可读部分，最后就是[writerIndex,size)可写部分。  
通过移动可读和可写索引来向buffer中写或者读数据。       
```
explicit Buffer(size_t initialSize = kInitialSize)
			: buffer_(kCheapPrepend + initialSize),
			readerIndex_(kCheapPrepend),
			writerIndex_(kCheapPrepend)   #显式构造函数，默认前置的空间大小是8字节，初始化空间为1024字节所以开始时buffer_大小为1032字节。    
const char* peek() const     #可读数据的指针   
const char* findString(const char* targetStr) const     #在可读区域查找字符串，找到返回，没找到返回nullptr    
const char* findCRLF() const     #在可读范围内查找"\r\n"    
const char* findCRLF(const char* start) const     #在start(必须在可读区域)到可读区域末尾查找"\r\n"     
bool retrieve(size_t len)     #读取一定数量字节     
std::string retrieveAsString(size_t len)    #以字符串的形式读取一定字节    
void append(const std::string& str)     #写入字符串     
void appendInt64(int64_t x)             #以网络字节序写入buffer一个Int64     
int64_t readInt64()                     #以从网络字节序读出一个Int64为主机字节序     
bool prepend(const void* /*restrict*/ data, size_t len)    #在可读区域前面写入数据    
void shrink(size_t reserve)             #调整整个buffer到合适大小   
int32_t Buffer::readFd(int fd, int* savedErrno)    #最重要的一个函数从fd中读数据到buffer中如果可写区域小于65536则开另外一个数组临时保存fd中的数据然后再写入buffer中，确保能读完不大于65536个自己的fd数据             
```
**3.类作用及对外接口总结**    
Buffer是整个mudou网络库用来存储数据的中间类，提供了很多适合网络编程的接口，向readFd()等，都是针对网络库设计的Buffer.其实也很简单就是一个vector，然后两个变量记录了索引，将整个区间可读，可写，还有前置区域都划分的很清楚。     

## Endian.h        
**1.类声明分析**    
```
inline uint64_t hostToNetwork64(uint64_t host64)    
inline uint32_t hostToNetwork64(uint32_t host32)       
inline uint16_t hostToNetwork64(uint16_t host16)    
inline uint64_t networkToHost64(uint64_t net64)     
inline uint32_t networkToHost64(uint32_t net32)          
inline uint16_t networkToHost16(uint16_t net16)        
```
**2.重要类函数实现分析**     
为不同主机和网络字节序，统一了接口，有64位，32位，16位，主机字节序和网络字节序的互换。确保网络编程数据的正常交换。    
**3.类作用及对外接口总结**    
根据不同的数据需要调用不同的函数，跟后续的Socket联合使用。    



## Callbacks.h       
**1.类声明分析**    
```
template<typename To, typename From>
inline To implicit_cast(From const& f)
{
    return f;
}                   #隐式转化函数

template<typename To, typename From>    
inline To down_cast(From* f)       #向下转化，内部是调用dynamic_cast   

emplate<typename To, typename From>
inline std::shared_ptr<To> down_pointer_cast(const std::shared_ptr<From> & f)         #指针向下转化，内部也是调用dynamic_cast    
```
**2.重要类函数实现分析**    
```

    class Buffer;
    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    typedef std::function<void()> TimerCallback;
    typedef std::function<void(const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void(const TcpConnectionPtr&)> CloseCallback;
    typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef std::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

    // the data has been read to (buf, len)
    typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

    void defaultConnectionCallback(const TcpConnectionPtr& conn);
    void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);
```    
主要定义了一些回调函数类型，主要是建立连接的ConnectionCallback,定时器回调函数类型TimerCallback,关闭连接回调类型closeCallback,写数据完成WriteCompleteCallback,还有最重要的收发数据回调MessageCallback,然后有两个默认的回调函数。一个是建立连接的时候需要调用，另外一个是收发数据时候需要调用。     

**3.类作用及对外接口总结**     
整个文件定义了一些回调函数别名，和一些类型转化，类型转化主要有两个，还有一些回调函数，主要是建立连接和消息处理的回调函数。    


## Sockets.h和Sockets.cpp       
**1.类声明分析**    
这个主要是对socket的封装     
```
explicit Socket(int sockfd) : sockfd_(sockfd)    #显示构造函数
SOCKET fd() const { return sockfd_; }            
bool getTcpInfoString(char* buf, int len) const; #得到Tcp连接信息 
void bindAddress(const InetAddress& localaddr)   #socketFd绑定某个地址
void listen();   							     #socket转化为监听套接字
int accept(InetAddress* peeraddr);     			 #accept()函数的包装，接收某个连接，返回连接套接字     
void shutdownWrite();                            #关闭套接字写
void setTcpNoDelay(bool on);    				 #设置连接属性
void setReuseAddr(bool on);     
void setReusePort(bool on);    
void setKeepAlive(bool on);     
private:
	const SOCKET sockfd_;           #唯一成员变量一个常量的普通socketFd
```
**2.重要类函数实现分析**    
```
SOCKET createOrDie();  #创建非阻塞fd,失败就结束进程(函数名的OrDie就是这个意思)
SOCKET createNonblockingOrDie(); #创建非阻塞fd，失败就结束进程

void setNonBlockAndCloseOnExec(SOCKET sockfd);  #设置socket属性为非阻塞和在子程序中关闭

void setReuseAddr(SOCKET sockfd, bool on);     #socket重复使用某个地址
void setReusePort(SOCKET sockfd, bool on);	   #socket重复使用某个端口    

SOCKET connect(SOCKET sockfd, const struct sockaddr_in& addr); #连接某个地址
void bindOrDie(SOCKET sockfd, const struct sockaddr_in& addr); #绑定某个地址到socket   
SOCKET accept(SOCKET sockfd, struct sockaddr_in* addr);   #接收连接到某个socket的连接   
int32_t read(SOCKET sockfd, void *buf, int32_t count);    #从套接字读数据
ssize_t readv(SOCKET sockfd, const struct iovec *iov, int iovcnt);#从套接字读数据到多个内存    
int32_t write(SOCKET sockfd, const void *buf, int32_t count);#向套接字写数据
void close(SOCKET sockfd);  #关闭套接字内部也是调用全局close()
void shutdownWrite(SOCKET sockfd); #停止套接字数据传输，内部调用shutdown()函数    

void toIpPort(char* buf, size_t size, const struct sockaddr_in& addr);#某个地址的ip和port转化为字符串    
void toIp(char* buf, size_t size, const struct sockaddr_in& addr);#某个地址的ip转化为字符串    
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);# 从字符串和port，生成一个地址     

int getSocketError(SOCKET sockfd);#得打某个套接字相关的错误   

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);#地址转化sockaddr和sockaddr_in之间的转化
struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr);

struct sockaddr_in getLocalAddr(SOCKET sockfd);#得到本地地址    
struct sockaddr_in getPeerAddr(SOCKET sockfd);#得到对端地址
bool isSelfConnect(SOCKET sockfd);#判断是不是自己连接自己   
```   
和网络Socket相关的一些函数。    
**3.类作用及对外接口总结**    
该类只是对普通socket的包装，然后就是对网络变成的bind(),listen(),connect(),accept()函数的封装。    



## InetAddress.h和InetAddress.cpp        
**1.类声明分析**     
```
explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
InetAddress(const std::string& ip, uint16_t port);
InetAddress(const struct sockaddr_in& addr);
std::string toIp() const;
std::string toIpPort() const;
uint16_t toPort() const;
const struct sockaddr_in& getSockAddrInet() const { return addr_; }
void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }
uint16_t portNetEndian() const { return addr_.sin_port; }
static bool resolve(const std::string& hostname, InetAddress* result); 
private:
    struct sockaddr_in      addr_;
```  
该类只是sockaddr_in的包装    
**2.重要类函数实现分析**   
3种构造函数都可以构造一个网络地址，可以用本地地址，也可以从ip.port构造，也可以从一个sockaddr_in地址构造，resolve()是解析主机名然后生成一个地址。  
**3.类作用及对外接口总结**    
这个类主要是对地址的封装，方便一些接口和对象的统一使用     

------------------------------------------------------------------------ 
------------------------------------------------------------------------

------------------------------------------------------------------------
现在才是整个网络库的基础，包括一些常用概念channel,EventLoop,Poller等。   

## Channel.h和Channel.cpp       
**1.类声明分析**    
![](../Pictures/mudou_Channel1.png)     
可以看到这里面的成员变量，其实channel是整个mudou网络库应该是最底层的一个抽象出来的类，这个类主要的是为了处理socket不同事件，例如可读，可写，或者错误之类的事件，channel，就好比一个通道，socket描述了一个连接的一方，一个channel就绑定了一个socket，然后进行相应的数据处理。      
所以可以看到某个channel肯定是绑定某个socket的，那么就必须有fd_,同时采用的是one thread,one loop 的模型，所以这个channel肯定是属于某个loop_,还有这个socket，需要加入poll，或者epoll模型中监听某个事件，所以有events_,然后就是结果所以有revents_,最后就是每个channel是要被上层功能所包含的，例如TcpConnection,那么该channel是否绑定到某个类上，就需要指针tie_,然后就是可读，可写，关闭，错误回调函数，其中可读的都是有时间戳为参数的。     
**2.重要类函数实现分析**   
```
Channel::Channel(EventLoop* loop, int fd__)    #构造函数，绑定某个fd,和属于某个loop     
update()         #在该loop中的poll模型中更新监控的事件   
enableReading()  #让该fd在poll模型中监控可读事件    
disableAll()     #让该fd移除对应的poll模型中监控事件，就是啥也不监控   
remove()         #从属于的poll模型中移除该channel绑定的事件   
handleEvent()    #根据poll模型中revent_调用对应的函数执行     
reventsToString() #把revents_转化为字符串，方便日志的记录
```
**3.类作用及对外接口总结**   
一个channel对应一个socket,属于某个loop,可以设置不同的待监控事件，根据结果，调用不同的设置的回调函数处理相应的事件发生，最主要的函数应该就是update()，它会调用loop中的poll模型中的更新channel函数去更新相应的channel,然后如果channel被激活，也就是revents_有事件，那么会调用handleEvent()函数去处理相应的事件。      

--------------------------------------------------------------------

## Poller.h和Poller.cpp       
下面提到的几个类都是以该类为基类，派生出来的，是为了常用的三种非阻塞IO模型，select,poll,epoll提供一些共同的接口函数也就是poll()函数。     
**1.类声明分析**    
![poller](../Pictures/mudou_Poller.png)     
可以看到这是一个有纯虚函数的抽象基类，是为了给后面介绍的三种IO模型做继承，然后提供了这个抽象基类的接口      
**2.重要类函数实现分析**     
```
Timestamp poll(int timeoutMs,ChannelList* activeChannels)   #主要调用函数，返回相应事件被触发的channel，也就是说最后得到的channel说明该channel对应的socketFd上有相应的事件需要处理。    
bool updateChannel(Channel* channel)   #在对应的poll模型中更新channel
void removerChannel(Channel* channel)  #在对应的poll模型中移除某个channel   
bool hasChannel(Channel* channel)      #在对应的poll模型中是否有某个channel    
```
**3.类作用及对外接口总结**     
以上是一个抽象基类，在每一个继承的类中，都会根据对应的模型重载相应的函数，最后对外的接口就是这几个函数。     

## PollPoller.h和PollPoller.cpp     
利用poll模型,和select模型类似也是轮询一些fd，然后将触发事件的fd结果保存        
**1.类声明分析**    
![poll](../Pictures/mudou_poll1.png)    
可以看到poll模型中有两个数据一个是vector,保存了polldf，还有一个是每一个fd所对应的Channel    
**2.重要类函数实现分析**     
```
PollPoller(EventLoop* loop)     #构造函数一个EventLoop含有一个poll   
fillActiveChannels(int numEvents,ChannelList* activeChannels) const #根据poll()函数中被触发的事件填充activeChannels    
updateChannel(Channel* channel)   #如果channel对应的索引是-1表明是一个新的channel,则直接加入到poll中的pollfds_和channels_中，否则就是一个存在的则更新它    
removeChannel(Channel* channel)  #移除某个已经在poll中的channel,该channel必须对应的是NoneEvent才能被移除，会在vector中将该channel与最后一个channel交换位置再移除，总是移除vector中最后一个，同时删除掉channels_中     
poll(int timeOutMs,ChannelList* activeChannels) #内部调用::poll()函数监控pollfds_中的socketFd,同时返回相应事件的通道和对应的时间     
```
**3.类作用及对外接口总结**     
Poll类是对poll模型的类的封装，其中的poll()函数正是对poll模型中的::poll()函数进行的封装处理。    

## EpollPoller.h和EpollPoller.cpp  
epoll模型是对poll模型的改进每个epoll对象控制着事件的加入和删除     
**1.类声明分析**     
![epoll](../Pictures/mudou_epoll1.png)       
可以看到也是有两个重要的数据成员events_,因为epoll模型是直接返回对应的事件所以相对的效率会高很多，不需要向poll那样对每一个event都要看是否触发，同样也有一个channels_,同时还有一个epollfd_，这是标识epoll对象的。    
**2.重要类函数实现分析**       
```
EpollPoller(EventLoop* loop)  #构造函数，属于某个loop，同时创建一个epoll对象  
updateChannel(Channel* channel)  # 更新某个channel
removeChannel(Channel* channel)  # 移除某个channel    
hasChannel(Channel* channel)     #epoll模型中是否有某个channel   
Timestamp poll(int timeoutMs,ChannelList* activeChannels) #调用内部epoll_wait()函数    
```
**3.类作用及对外接口总结**       
该类是对epoll模型的封装，使用和其他的模型一样，对外接口类似都是poll()函数内部执行::epoll_wait()功能     

## SelectPoller.h和SelectPoller.cpp    
select模型的封装实现        
**1.类声明分析**    
![select](../Pictures/mudou_select1.png)    
用的类和epoll类似只是某些细节上不一样   
**2.重要类函数实现分析**       
```
poll(int timeoutMs,channelList* activeChannels)  #内部调用select()函数轮询fd，将fd加入对应的可读，可写集合      
fillActiveChannels(int numEvents, ChannelList* activeChannels, fd_set& readfds, fd_set& writefds)    #判断可读可写事件中是否有某个channel，有就加入到激活channels中
```
**3.类作用及对外接口总结**       
利用select模型，去判断事件是否被触发，select模型是有两个可读，可写集合，轮询监控这两个集合     

--------------------------------------------------------------------

## Timer.h和Timer.cpp    
从前面我们就能推断一个loop中应该有一个poll模型，poll模型主要监控的是网络读写事件，我们还有一些其他事件同样是需要处理的，例如定时器事件，该类就表示一个定时器事件。      
**1.类声明分析**   
![timer](../Pictures/mudou_timer.png)     
定时器类主要就是在某个时刻触发某事件，该类支持某时刻触发某事件并且间隔多少事件，触发某事件多少次     
成员主要包括一个回调函数，过期时间和一个时间间隔，重复次数，还有一个序列号表示是当前类的多少个对象
**2.重要类函数实现分析**       
```
Timer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeatCount = -1);    #定时器构造函数-1，表示一直重复下去     
void run();                   #实际调用函数，表示跑定时器事件函数
```
**3.类作用及对外接口总结**    
该类是一个定时器类，一个对象表示一个定时器事件，一般只用该类创建对象，加入下面要分析的定时器队列类对象中     

## TimerId.h   
**1.类声明分析**   
![timerId](../Pictures/mudou_timerId.png)    
**2.重要类函数实现分析**    
通过序列号和一个定时器对象指针构造一个TimerId对象       
**3.类作用及对外接口总结**     
对Timer对象的一个封装，只是单纯的一层过渡，方便使用     

## TimerQueue.h和TimerQueue.cpp    
定时器队列类，每个loop中应当都有一个该对象,同时TimerQueue掌管着所以Timer对象的构造，析构    
**1.类声明分析**   
![TimerQueue](../Pictures/mudou_timerQueue.png)     
**2.重要类函数实现分析**    
```
TimerId addTimer(const TimerCallback& cb, Timestamp when, int64_t interval, int64_t repeatCount);   #添加定时器内部会new一个定时器对象，然后因为set集合的线程不安全，所以会调用addTimerInLoop()函数，在loop中insert一个对象到set中保证在线程安全    
removeTimer(TimerId timerId)   #从set中移除一个定时器内部也是调用removeTimerInLoop()     
cancelTimer(TimerId timerId,bool offf)  #将一个定时器对象设置为开/关，内部也会调用cancelTimerInLoop()     
doTimer()  #定时器事件，如果当前时间>=定时器对象定时时间，则调用Timer->run()函数做该定时器事件    
           #同时采用set是可以利用定时器事件开始时间作为set的key键，从最早时间开始直到当前时间都是要做定时器事件，加快效率
```
**3.类作用及对外接口总结**     
TimerQueue是一个定时器类，用set保存每一个定时器对象包括定时器对象，然后比较当前时间和定时器时间，判断是否做相应的事件。   
貌似remove定时器对象和定时器重复次数变为移除set的Timer没有delete？内存泄露？           

------------------------------------------------------------------------








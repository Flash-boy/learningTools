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
Singleton.h->Timestamp.h/cpp->ConfigFileReader.h/.cpp->
```  
以上只是个人的阅读顺序，希望你能有一个大概的网络API概念，包括套接字，bind(),listen(),connect()等函数，知道是干什么用的。然后阅读源码时候你会发现大佬写的代码都是很规范很漂亮的，文件夹的命名和文件的命名都是很规范，都能给你一定信息。我会带着上述的代码阅读顺序，逐个解析各个文件的作用，包括函数，和如何和其他连接起来的。    
### 阅读源码    
> 我觉得看懂一个类的作用和实现首先需要看的是类声明，先看类的成员变量，再看类的函数声明，再看类的成员函数的实现(细节)，最后总结归纳出该类的作用和对外接口。所以我会一次解读每个文件中每个类。每个类的分析都会有，类声明分析、重要类函数实现分析、类作用和对外接口，三个部分。    


## Singleton.h   
**1.类声明分析**    
**2.重要类函数实现分析**   
**3.类作用及对外接口总结**  

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

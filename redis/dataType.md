## redis对象的实现     
主要包括以下几个部分     
1.各种redis对象，包括string对象，list对象，set对象，hash对象，zset对象     
2.各种键对象的实现     

### 1.object.c      
- 设计思想   
  redis有各种数据结构，然而 Redis 并没有直接使用这些数据结构来实现键值对的数据库，而是在这些数据结构之上又包装了一层 RedisObject（对象），RedisObject 有五种对象：字符串对象、列表对象、哈希对象、集合对象和有序集合对象。通过不同类型的对象，Redis 可以在执行命令之前，根据对象的类型来判断一个对象是否可以执行给定的命令。我们可以针对不同的使用场景，为对象设置不同的实现，从而优化内存或查询速度。    

- 源码分析    
  
  **object.c源文件**   
  ![object](../Pictures/redis_object1.png)      
  object抽象出来了这么一个结构，我们看看各字段的含义    
  type:    
  type 记录了对象的类型，所有的类型如下，其中键一定是字符串对象，值是5种对象中的任一种      
  |类型常量|对象名称|
  |:----|:----|
  |OBJ_STRING|字符串对象|  
  |OBJ_LIST|列表对象|
  |OBJ_SET|集合对象|
  |OBJ_HASH|哈希对象|
  |OBJ_ZSET|有序集合对象|   
  ptr:      
  指向对象的底层实现数据结构      

  encoding:     
  encoding 表示 ptr 指向的具体数据结构，即这个对象使用了什么数据结构作为底层实现,encoding主要有以下几种     

  |编码常量|编码常量所对应的底层数据结构|
  |:----|:----|
  |OBJ_ENCODING_RAW|简单动态字符串|  
  |OBJ_ENCODING_EMBSTR|embstr编码的简单动态字符串|
  |OBJ_ENCODING_INT|long类型的整数|  
  |OBJ_ENCODING_QUICKLIST|quicklist对应的快速列表|  
  |OBJ_ENCODING_ZIPLIST|ziplist对应的压缩列表| 
  |OBJ_ENCODING_HT|dict对应的字典结构|
  |OBJ_ENCODING_INTSET|intset对应的整数集合|
  |OBJ_ENCODING_SKIPLIST|包含skiplist和dict两种结构|    
  
  每种对象类型可能底层实现有多种数据结构，下面是对象和数据结构之间的关系     

  |类型|编码|对象|
  |:----|:----|:----|
  |OBJ_STRING|OBJ_ENCODING_RAW|使用简单字符串实现的字符串对象|
  |OBJ_STRING|OBJ_ENCODING_EMBSTR|使用embstr编码简单字符串实现的字符串对象|
  |OBJ_STRING|OBJ_ENCODING_INT|使用整数值实现的字符串对象|  
  |OBJ_LIST|OBJ_ENCODING_QUICKLIST|使用快速列表实现的列表对象|
  |OBJ_LIST|OBJ_ENCODING_ZIPLIST|使用压缩列表实现的列表对象|
  |OBJ_SET|OBJ_ENCODING_HT|使用字典实现的集合对象|  
  |OBJ_SET|OBJ_ENCODING_INTSET|使用整数集合实现的集合对象|
  |OBJ_HASH|OBJ_ENCODING_ZIPLIST|使用压缩列表实现的哈希对象|
  |OBJ_ZSET|OBJ_ENCODING_SKIPLIST|使用跳跃表和字典两种结构实现的有序集合对象|
  |OBJ_ZSET|OBJ_ENCODING_ZIPLIST|使用压缩列表实现的有序集合对象|   

  refcount:    
  refcount 表示引用计数，由于 C 语言并不具备内存回收功能，所以 Redis 在自己的对象系统中添加了这个属性，当一个对象的引用计数为0时，则表示该对象已经不被任何对象引用，则可以进行垃圾回收了。    

  lru:    
  表示对象最后一次被命令程序访问的时间     

  ```
  ###############字符串对象##################
  robj *createObject(int type, void *ptr) #创建对象底层数据结构采用OBJ_ENCODING_RAW   
  robj *makeObjectShared(robj *o) #设置对象的refcount为OBJ_SHARED_REFCOUNT表示为共享对象    
  robj *createRawStringObject(const char *ptr, size_t len) #创建底层为OBJ_ENCODING_RAW的字符串对象    
  robj *createEmbeddedStringObject(const char *ptr, size_t len) #创建底层为OBJ_ENCODING_EMBSTR的字符串对象，embstr编码其实就是object结构和string内存连在一起，一次分配两个内存    
  robj *createStringObject(const char *ptr, size_t len) #如果字符串长度小于等于BJ_ENCODING_EMBSTR_SIZE_LIMIT则采用embstr编码的字符串，否则原生字符串对象     
  robj *createStringObjectFromLongLong(long long value) #某个long long值，先看是否可以用共享对象，再看能否变为OBJ_ENCODING_INT字符串，最后不行才将数抓为字符串，存入对象    
  robj *createStringObjectFromLongDouble(long double value, int humanfriendly) #long double转化为字符串构建字符串对象    
  robj *dupStringObject(const robj *o) #复制某个字符串对象    
  void incrRefCount(robj *o)  #如果不是共享对象增加refcount计数   
  void decrRefCount(robj *o)  #如果不是共享对象，减少refcount计数，减为0时释放ptr指针所指向的对象数据    
  robj *tryObjectEncoding(robj *o) #试着将OBJ_ENCODING_RAW或者OBJ_ENCODING_EMBSTR字符串对象编码为OBJ_ENCODING_INT实现的对象，或者把OBJ_ENCODING_RAW转化为OBJ_ENCODING_EMBSTR，或者最差将OBJ_ENCODING_RAW字符串的空余位置压缩    
  robj *getDecodedObject(robj *o) #试着把OBJ_ENCODING_INT实现的字符串，创建为一个新的以OBJ_ENCODING_RAW或者OBJ_ENCODING_EMBSTR实现的字符串对象    
  int compareStringObjects(robj *a, robj *b) #二进制方式比较字符串对象    
  int collateStringObjects(robj *a, robj *b) #strcoll方式比较字符串对象    
  size_t stringObjectLen(robj *o) #得到字符串对象长度       
  int getDoubleFromObject(const robj *o, double *target) # 从字符串中得到double值   
  char *strEncoding(int encoding) #将对象的encoding转化为字符串    
  ```    

- 总结    
  该object.c中的部分源码只是为了让我们对redisObjetc结构描述对象有一个完整的认识，后续的各种键值对象的实现在后续值对象中会提到     

   





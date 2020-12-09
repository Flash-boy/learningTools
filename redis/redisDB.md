## redis数据库的实现    
redis数据库的实现其实就是一个大字典，每个db都是一个大的字典存储着我们常说的key,value    

### 1.db.c    
db.c主要介绍了redis的数据库的操作命令，也就是最外层的一个大字典的key，value的操作。    

- 设计思想   
  redisDb结构表示了一个数据库，可以看到里面不仅仅只有一个dict,而是有多个dict，其中的主要字典是dict，其他还有处理设置键过期的key，还有处理列表阻塞命令等的key     
  ![db](../Pictures/redis_db1.png)    
  这里主要关注dict和expires表示的是主字典和存储过期key-when的字典      
  id表示当前数据库的索引，因为redis有一个数组表示多个数据库    
  avg_ttl表示的是当前数据库的key平均过期时间     

- 源码分析   
  **db.c源文件**     
  首先要讲一下redis存在一个淘汰策略，就是当内存中达到最大内存时如果开启了数据淘汰策略，也即是内存写满了的情况下到底该先删除哪些key。     
  LRU,最近最少使用，传统LRU需要一个哈希表和双向链表，但如果加入一个双向链表是需要消耗内存的，redis为了减少内存消耗采用了**近似LRU算法**。我们可以看到每个对象中都有一个lru数据，这个如果是在LRU淘汰策略下就表示一个24位的表示毫秒的时间戳。来表示最近访问时间。LRU近似算法如下：    
  
  1.首次淘汰：随机抽样选出【最多N个数据】放入【待淘汰数据池 evictionPoolEntry】   
  - 数据量N：由 redis.conf 配置的 maxmemory-samples 决定，默认值是5，配置为10 将非常接近真实LRU效果，但是更消耗CPU；     
  - samples：n.样本；v.抽样；     

  2.再次淘汰：随机抽样选出【最多N个数据】，只要数据比【待淘汰数据池 evictionPoolEntry】中的【任意一条】数据的 lru 小，则将该数据填充至 【待淘汰数据池】；      
  - evictionPoolEntry 的容容量是 EVPOOL_SIZE = 16；     
  - 详见 源码 中 evictionPoolPopulate 方法的注释；   
3.执行淘汰： 挑选【待淘汰数据池】中 lru 最小的一条数据进行淘汰；     

  ==============================================================
  另外一种淘汰策略是**LFU**,最不经常使用     
  如果一条数据仅仅是突然被访问（有可能后续将不再访问），在 LRU 算法下，此数据将被定义为热数据，最晚被淘汰。但实际生产环境下，我们很多时候需要计算的是一段时间下key的访问频率，淘汰此时间段内的冷数据。LFU 算法相比 LRU，在某些情况下可以提升 数据命中率，使用频率更多的数据将更容易被保留。      
  LFU 使用 Morris counter 概率计数器，仅使用几比特就可以维护 访问频率，Morris算法利用随机算法来增加计数，在 Morris 算法中，计数不是真实的计数，它代表的是实际计数的量级。     
  redis采用的是对象字段lru被分为两部分，前16位表示系统时间单位为分钟，后8位最大为255表示使用频率注意不是使用次数。访问频率会随着时间衰减，也就是前面的16位存储的时间与当前时间比较，相差越大衰减的越大。    
  
  LFU 的核心配置：

  - lfu-log-factor：counter 增长对数因子，调整概率计数器 counter 的增长速度，      
  - lfu-log-factor值越大 counter 增长越慢；lfu-log-factor 默认10。     
    lfu-decay-time：衰变时间周期，调整概率计数器的减少速度，单位分钟，默认1。N 分钟未访问，counter 将衰减 N/lfu-decay-time，直至衰减到0；     
    若配置为0：表示每次访问都将衰减 counter；

  ================================================================     
  ```
  void updateLFU(robj *val) #更新LFU，也就是上面提到的LFU淘汰策略       
  robj *lookupKey(redisDb *db, robj *key, int flags) #查找key，其中回调用更新LFU    
  void dbAdd(redisDb *db, robj *key, robj *val) #往数据库中加入一个key，value   
  void dbOverwrite(redisDb *db, robj *key, robj *val) #重写数据库某个key的值为新value    
  void setKey(redisDb *db, robj *key, robj *val) #上面两个函数的封装，数据库某个key对应为value    
  robj *dbRandomKey(redisDb *db) #返回一个随机的key对象   
  int dbSyncDelete(redisDb *db, robj *key) #同步删除key，value已经expires字典中的key   
  int dbDelete(redisDb *db, robj *key) #根据数据库的配置采用同步删除或者惰性删除   
  long long emptyDb(int dbnum, int flags, void(callback)(void*)) #清空某个索引的数据库，-1则清空所以数据库   
  int selectDb(client *c, int id) #切换到，某一数据库       
  ```   
  ```
  *********************************操作数据库命令API*******************************************
  int getFlushCommandFlags(client *c, int *flags)  #解析清空数据库的的操作flag      
  void flushdbCommand(client *c) #清空当前数据库  
  void flushallCommand(client *c) #清空所有数据库   
  void delGenericCommand(client *c, int lazy) #删除数据库某个key    
  void delCommand(client *c) #del直接删除key，value   
  void unlinkCommand(client *c) #惰性删除key对应的value   
  void existsCommand(client *c) #返回key在数据库中存在的个数    
  void selectCommand(client *c) #切换当前操作哪一个数据库    
  void randomkeyCommand(client *c) #返回随机的key   
  void keysCommand(client *c) #查找满足某个pattern的所有key   
  void scanGenericCommand(client *c, robj *o, unsigned long cursor)  #扫描数据库命令   
  void dbsizeCommand(client *c) #返回数据库的键值对个数   
  void lastsaveCommand(client *c) #返回lastsave的时间   
  void typeCommand(client *c) #返回键对应的值的类型   
  void shutdownCommand(client *c) #何种方式关闭服务器  
  void renameGenericCommand(client *c, int nx) #重命名某个key  
  void moveCommand(client *c) #移动某个key到某个数据中   
  void swapdbCommand(client *c) #交换两个数据库的数据   

  ```  

- 总结    
  |命令|使用方法|作用|
  |:----|:----|:------|
  |FLUSHDB|flushdb [ASYNC]|采用同步还是异步的方式清空当前数据库|   
  |FLUSHALL|flushall [ASYNC]|采用异步还是同步的方式清空所有数据库|   
  |DEL|del key [key ...]|直接删除数据库中对应的key，value|  
  |UNLINK|unlink key [key ...]|惰性删除key对应的value|   
  |EXISTS|exists key [key ...]|返回数据库中某些key存在的个数|   
  |SELECT|select index|切换到当前操作索引对应的数据库|   
  |RANDOMKEY|randomkey|返回数据库某个随机的key|   
  |KEYS|keys pattern|返回满足某一pattern的所以key|  
  |SCAN|scan cursor [MATCH pattern] [COUNT count]|扫描遍历当前数据库|   
  |DBSIZE|dbsize|返回当前数据库键值对的个数|  
  |LASTSAVE|lastsave|返回上次保存的时间|   
  |TYPE|type key|返回某个键对应的值的类型| 
  |SHUTDOWN| shutdown [NOSAVE|SAVE]|保存或者不保存的方式关闭数据库|   
  |RENAME|rename key newkey|重命名某个存在的key为newkey|   
  |RENAMENX|renamenx key newkey|在新的key不存在时才重命名|  
  |MOVE|move key db|移动某个key到数据库db中|   
  |SWAPDB|swapdb db1 db2|交换数据库的数据|   
  






  



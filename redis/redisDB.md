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

### notify功能    
redis的通知功能是基于订阅发布的所实现的，听起来好像一脸懵逼，让我慢慢到来。       
**为什么要有通知功能？**       
其实主要源于我们想要关注某个key，接下来有哪些事，例如某个客户端存储了一个key，value，然后它需要持续关注这个key，可能另外一个客户端修改了这个key，删除了这个key等等，那么该客户端就key订阅一个东西用来关注这个key的改动，当有改动，redis服务器就会推送信息给该客户端，这也就是所谓了键空间，主要的是关注某个key。        
还有另外一种情况，就是某个客户端想要关注对数据库的某些操作特别感兴趣，例如DEL命令，该客户端关注所以DEL命令的实施，所以该客户端对这个事件，这个动作感兴趣，也可以订阅一个东西来关注这个，当redis服务器DEL命令产生就可以向该客户端推送相关信息。这也就是所谓的键事件。     

- 设计思想    
  我们知道一个redis服务器可能有多个客户端，那么它是如何实现上面的功能的呢，这就不得不说pubsub机制了，还有一个channel，这里大概说一下，其实就是维护了一个字典和一个列表。当某个客户端订阅某个channel，其实就是一个字符串，只不过是特殊含义的字符串，也即是一开始说的键空间和键事件。都会加入到一个全局的专门用来处理通知功能的字典，字典的key就是这里的channel名字，value就是一个列表，表示订阅这个channle的客户端。redis服务器全局不断的维护这个表，然后每次命令都调用**notifyKeyspaceEvent(int type, char *event, robj *key, int dbid)**这个函数，实现发布，从而完成了一整个的通知机制。    

- 源码分析    
  **pubsub.c源文件**    
  该文件就是订阅发布的实现，其实就是维护了一个字典和一个列表，针对普通的channel,还有patter匹配模式的channel的实现   
  ``` 
  #################################pubsub低价API######################## 
  typedef struct pubsubPattern {
    client *client;
    robj *pattern;          
  } pubsubPattern;    
  #我们知道一个channel就是一个字符串，例如mychannel，而pattern其实就是一个带有匹配的例如my*这样的可以匹配任何my开头的channel，上面结构就表明了一个pattern    
  void freePubsubPattern(void *p) #释放这个pubsubPattern结构   
  int listMatchPubsubPattern(void *a, void *b) #判断两个pattern是否完全相等，包括客户端都要相同    
  int clientSubscriptionsCount(client *c) #客户端定义的channel和pattern总数    
  int pubsubSubscribeChannel(client *c, robj *channel) #客户端订阅一个channel,首先会加入c->pubsub_channels客户端自己的一个特殊字典中，然后会加入到全局的字典中这样整个服务器就可以知道哪些客户端订阅了哪些channel     
  int pubsubUnsubscribeChannel(client *c, robj *channel, int notify) #客户端取消订阅某个channel    
  int pubsubSubscribePattern(client *c, robj *pattern) #客户端订阅一个pattern，跟channel一样，不过是采用列表来保存pattern,客户端本地保存需要pattern不一样，全局列表因为pattern加入了客户端所以不同客户端肯定不一样     
  int pubsubUnsubscribePattern(client *c, robj *pattern, int notify) #取消订阅某个pattern    
  int pubsubUnsubscribeAllChannels(client *c, int notify) #取消订阅客户端所有的channel    
  int pubsubUnsubscribeAllPatterns(client *c, int notify) #取消订阅客户端所有的pattern   
  int pubsubPublishMessage(robj *channel, robj *message) #向某个channel发送消息，也是最重要的函数之一，首先会找到服务器全局的channels,找到该channel，然后向对应的客户端们发该消息，然后会找到全局的patten，找到匹配该channel的客户端发送消息      
  ```
  ```
  #############################PUBSUB命令API###############################
  void subscribeCommand(client *c) #客户端订阅一些channel 
  void unsubscribeCommand(client *c) #客户端取消订阅channel   
  void psubscribeCommand(client *c)   #客户端订阅pattern    
  void punsubscribeCommand(client *c)  #客户端取消订阅pattern   
  void publishCommand(client *c) #客户端向一个channel发布消息   
  void pubsubCommand(client *c) #pubsub命令可以实现一些关于channel,patter查看的子命令    
  ```
  **notify.c源文件**    
  该源文件就只有3个函数      
  ```
  int keyspaceEventsStringToFlags(char *classes) #判断某个字符串表示的属于哪一个事件是键空间还是键事件   
  sds keyspaceEventsFlagsToString(int flags) #和上面的相反，通过flag解析出对应的字符串   
  void notifyKeyspaceEvent(int type, char *event, robj *key, int dbid) #最重要的函数我们可以从好多命令中看到这个函数，因为我们的命令就是对键的一些操作，其中就会调用这个函数，将对应的事件通过channel发出去    
                                                                       #如果有客户端定义了该channel就会收到对应的消息        
                                                                       #__keyspace@<db>__:<key> <event>  键空间，如果某个客户端订阅了__keyspace@<db>__:<key> 这个channel,那么针对该键的各种操作如DEL，就会通知对应的客户端。（这里event就是各种操作例如DEL）     
                                                                       #__keyevent@<db>__:<event> <key>  键事件，如果有某个客户端订阅了__keyevent@<db>__:<event> 这个channel，那么出现该种动作的各个键都会通知客户端        
                                                                       #内部就是调用了pubsubPublishMessage()函数往某个channel发送消息     
  ```  
  这样不管你是关注键本身，还是动作(事件)本身都可以清楚的追踪，例如关注键，那么对键的各种操作你都会收到，如果关注的是事件，那么该事件操作的哪些键你都能知道。总而言之，redis很巧妙的利用了订阅发布这样一个机制实现了监控。但唯一的缺点是，当客户端消失一段时间又连上，期间发生的针对键的操作是无法追踪的。因为你客户端本身不在了，这个机制也就不会向你这个客户端发送任何东西。    
  

- 总结    
  |命令|使用方法|作用|
  |:----|:----|:------|
  |SUBSCRIBE|subscribe channel [channel ...]|客户端订阅channel|
  |UNSUBSCRIBE|unsubscribe channel [channel ...]|客户端取消订阅,没有channel参数，则表示取消所以channel|  
  |PSUBSCRIBE|psubscribe pattern [pattern ...]|客户端订阅pattern|   
  |PUNSUBSCRIBE|punsubscribe pattern [pattern ...]|客户端取消订阅pattern，没有参数表示取消所有pattern|   
  |PUBLISH|publish channel message|向某个channel上发布message|   
  |PUBSUB|pubsub subcommand [argument [argument ...]]|主要是一些查看当前数据库的订阅channel,pattern的命令|   
  |PUBSUB CHANNELS|pubsub channels [pattern] |查看当前数据库的符合pattern模式的channel,pattern可以为空，则显示所有channel| 
  |PUBSUB NUMSUB|pubsub numsub [[channel1] [channel2] ...]|查看订阅某个channel的客户端数|   
  |PUBSUB NUMPAT|pubsun numpat|查看服务器当前总的pattern数|     

### 2.RDB持久化     
因为Redis是内存数据库，它将自己的数据库状态储存在内存里面，所以如果不想办法将储存在内存中的数据库状态保存到磁盘里面，那么一旦服务器进程退出，服务器中的数据库状态也会消失不见，为了解决这个问题，Redis提供了RDB持久化功能，这个功能可以将Redis在内存中的数据库状态保存到磁盘里面，避免数据意外丢失，RDB持久化既可以手动执行，也可以根据服务器配置选项定期执行，该功能可以将某个时间点上的数据库状态保存到一个RDB文件。     

- 设计思想    
  RDB的实现是保存当前数据库存储的键值数据以特殊形式保存到二进制文件中，包括数据还有各种状态，然后在服务器下次启动时加载该文件恢复数据。       
  有两个Redis命令可以用于生成RDB文件，一个是SAVE，另一个是BGSAVE      
  SAVE：SAVE命令会阻塞Redis服务器进程，直到RDB文件创建完毕为止，在服务器进程阻塞期间，服务器不能处理任何命令请求        
  BGSAVE：和SAVE命令直接阻塞服务器进程的做法不同，BGSAVE命令会派生出一个子进程，然后由子进程负责创建RDB文件，服务器进程（父进程）继续处理命令请求      
  ![rdb](../Pictures/redis_rdb1.png)     
  redis在执行save或者bgsave命令时的状态     
  > **SAVE命令执行时服务器的状态**
  > - 当SAVE命令执行时，Redis服务器会被阻塞，所以当SAVE命令正在执行时，客户端发送的所有命令请求都会被拒绝
  > - 只有在服务器执行完SAVE命令、重新开始接受命令请求之后，客户端发送的命令才会 被处理


  > **BGSAVE命令执行时服务器的状态**
  > - 因为BGSAVE命令的保存工作是由子进程执行的，所以在子进程创建RDB文件的过程中，服务器仍然可以继续处理客户端的命令请求，但在BGSAVE命令执行期间，服务器处理SAVE、BGSAVE、BGREWRITEAOF三个命令的方式会和平时有所不同  
  > - ①在BGSAVE命令执行期间，客户端发送的SAVE命令会被服务器拒绝，服务器禁止 SAVE命令和BGSAVE命令同时执行是为了避免父进程（服务器进程）和子进程同时执行两个 rdbSave调用，防止产生竞争条件
  > - ②在BGSAVE命令执行期间，客户端发送的BGSAVE命令会被服务器拒绝，因为同 时执行两个BGSAVE命令也会产生竞争条件
  > - **③BGREWRITEAOF和BGSAVE两个命令不能同时执行：**
  > 1.如果BGSAVE命令正在执行，那么客户端发送的BGREWRITEAOF命令会被延迟到 BGSAVE命令执行完毕之后执行  
  > 2.如果BGREWRITEAOF命令正在执行，那么客户端发送的BGSAVE命令会被服务器拒绝    
  > - 因为BGREWRITEAOF和BGSAVE两个命令的实际工作都由子进程执行，所以这两个命令在操作方面并没有什么冲突的地方，不能同时执行它们只是一个性能方面的考虑,并发出两个子进程，并且这两个子进程都同时执行大量的磁盘写入操作，这怎么想都不会是一个好主意

  RBD文件的载入是在服务器启动的时候完成的。RDB文件的载入工作是在服务器启动时自动执行的，所以Redis并没有专门用于载入RDB文件的命令，只要Redis服务器在启动时检测到RDB文件存在，它就会自动载入RDB文件服务器在载入RDB文件期间，会一直处于阻塞状态，直到载入工作完成为止。      

  > AOF持久化对RDB持久化的影响：   
  > - 如果服务器开启了AOF持久化功能，那么服务器会优先使用AOF文件来还原数据库状 态，那么就不会使用RDB文件了
  > - 只有在AOF持久化功能处于关闭状态时，服务器才会使用RDB文件来还原数据库状态   


- 源码分析    
  这次我们直接从最上层的命令函数开始看起一个一个函数来解析，先看看一个RDB文件到底是怎样分布的     
  ![rdb](../Pictures/redis_rdb2.png)          
  ```
  **************************save命令实现函数*************************
  void saveCommand(client *c) {
    if (server.rdb_child_pid != -1) {
        addReplyError(c,"Background save already in progress");
        return;
    }
    rdbSaveInfo rsi, *rsiptr;  
    // 初始化rsi结构
    rsiptr = rdbPopulateSaveInfo(&rsi);  
    // 调用rdbsave()函数    
    if (rdbSave(server.rdb_filename,rsiptr) == C_OK) {
        addReply(c,shared.ok);
    } else {
        addReply(c,shared.err);
    }
  }
  ```   
  ```
  ***************************rdbsave()函数*******************
  int rdbSave(char *filename, rdbSaveInfo *rsi) {
    char tmpfile[256];
    char cwd[MAXPATHLEN]; /* Current working dir path for error messages. */
    FILE *fp;
    rio rdb;
    int error = 0;
    // 初始化一个临时文件    
    snprintf(tmpfile,256,"temp-%d.rdb", (int) getpid());
    fp = fopen(tmpfile,"w");
    if (!fp) {
        char *cwdp = getcwd(cwd,MAXPATHLEN);
        serverLog(LL_WARNING,
            "Failed opening the RDB file %s (in server root dir %s) "
            "for saving: %s",
            filename,
            cwdp ? cwdp : "unknown",
            strerror(errno));
        return C_ERR;
    }
    //将该文件绑定到一个rio对象，用于写     
    rioInitWithFile(&rdb,fp);  
    //实际写数据到磁盘     
    if (rdbSaveRio(&rdb,&error,RDB_SAVE_NONE,rsi) == C_ERR) {
        errno = error;
        goto werr;
    }
    //刷新到磁盘
    /* Make sure data will not remain on the OS's output buffers */
    if (fflush(fp) == EOF) goto werr;
    if (fsync(fileno(fp)) == -1) goto werr;
    if (fclose(fp) == EOF) goto werr;

    /* Use RENAME to make sure the DB file is changed atomically only
     * if the generate DB file is ok. */
    //将该临时文件重命名    
    if (rename(tmpfile,filename) == -1) {
        char *cwdp = getcwd(cwd,MAXPATHLEN);
        serverLog(LL_WARNING,
            "Error moving temp DB file %s on the final "
            "destination %s (in server root dir %s): %s",
            tmpfile,
            filename,
            cwdp ? cwdp : "unknown",
            strerror(errno));
        unlink(tmpfile);
        return C_ERR;
    }

    serverLog(LL_NOTICE,"DB saved on disk");
    server.dirty = 0;
    server.lastsave = time(NULL);
    server.lastbgsave_status = C_OK;
    return C_OK;

  werr:
    serverLog(LL_WARNING,"Write error saving DB on disk: %s", strerror(errno));
    fclose(fp);
    unlink(tmpfile);
    return C_ERR;
  }
  ```   
  int rdbSaveRio(rio *rdb, int *error, int flags, rdbSaveInfo *rsi) #实际的工作函数写内存数据到文件     
  **大致流程如下**    
  1.先写入 REDIS 魔法值，然后是 RDB 文件的版本( rdb_version )，额外辅助信息 ( aux )。辅助信息中包含了 Redis 的版本，内存占用和复制库( repl-id )和偏移量( repl-offset )等。    
  2.然后 rdbSaveRio 会遍历当前 Redis 的所有数据库，将数据库的信息依次写入。先写入 RDB_OPCODE_SELECTDB识别码和数据库编号，接着写入RDB_OPCODE_RESIZEDB识别码和数据库键值数量和待失效键值数量，最后会遍历所有的键值，依次写入。     
  3.在写入键值时，当该键值有失效时间时，会先写入RDB_OPCODE_EXPIRETIME_MS识别码和失效时间，然后写入键值类型的识别码，最后再写入键和值。    
  4.写完数据库信息后，还会把 Lua 相关的信息写入，最后再写入 RDB_OPCODE_EOF结束符识别码和校验值。 

  =================================================================

  rdbSaveRio在写键值时，会调用rdbSaveKeyValuePair 函数。该函数会依次写入键值的过期时间，键的类型，键和值。       
  根据对象的不同写入不同的值    
  ![rdb](../Pictures/redis_rdb3.png)      

- 总结    
  总之redis在进行rdb持久化的过程其实就是一个数据库某个时间节点的快照，将当前状态的数据库数据按一定的格式写入到某一个文件中，可以看到当数据库数据量十分大时没进行一次RDB持久化会相对耗时     

### 3.AOF持久化   
除了RDB持久化功能之外，Redis还提供了AOF(AppendOnly File)持久化功能。与RDB持久化通过保存数据库中的键值对来记录数据库状态不同，AOF持久化是通过保存Redis服务器所执行的写命令来记录数据库状态的。与RDB持久化相比，AOF持久化可能丢失的数据更少，但是AOF持久化可能会降低Redis的性能。    

- 设计思想     
  AOF持久化功能的实现可以分为命令追加、文件写入、文件同步(sync)三个步骤。AOF其实就是一个个命令的记录，等到需要的时候，再模拟一个客户端执行这些命令，那数据下次又可以在内存中生成。      

- 源码解析    
  **一：AOF持久化**    
  AOF持久化功能的实现可以分为命令追加、文件写人、文件同步(sync)三个步骤。    
  1.命令追加     
  开启了AOF快照功能后，当Redis服务器收到客户端命令时，会调用函数feedAppendOnlyFile。该函数按照统一请求协议对命令进行编码，将编码后的内容追加到AOF缓存server.aof_buf中。feedAppendOnlyFile代码如下：     
  ```
  void feedAppendOnlyFile(struct redisCommand *cmd, int dictid, robj **argv, int argc) {
    sds buf = sdsempty();
    robj *tmpargv[3];

    /* The DB this command was targeting is not the same as the last command
     * we appended. To issue a SELECT command is needed. */
    if (dictid != server.aof_selected_db) {
        char seldb[64];

        snprintf(seldb,sizeof(seldb),"%d",dictid);
        buf = sdscatprintf(buf,"*2\r\n$6\r\nSELECT\r\n$%lu\r\n%s\r\n",
            (unsigned long)strlen(seldb),seldb);
        server.aof_selected_db = dictid;
    }

    if (cmd->proc == expireCommand || cmd->proc == pexpireCommand ||
        cmd->proc == expireatCommand) {
        /* Translate EXPIRE/PEXPIRE/EXPIREAT into PEXPIREAT */
        buf = catAppendOnlyExpireAtCommand(buf,cmd,argv[1],argv[2]);
    } else if (cmd->proc == setexCommand || cmd->proc == psetexCommand) {
        /* Translate SETEX/PSETEX to SET and PEXPIREAT */
        tmpargv[0] = createStringObject("SET",3);
        tmpargv[1] = argv[1];
        tmpargv[2] = argv[3];
        buf = catAppendOnlyGenericCommand(buf,3,tmpargv);
        decrRefCount(tmpargv[0]);
        buf = catAppendOnlyExpireAtCommand(buf,cmd,argv[1],argv[2]);
    } else if (cmd->proc == setCommand && argc > 3) {
        int i;
        robj *exarg = NULL, *pxarg = NULL;
        /* Translate SET [EX seconds][PX milliseconds] to SET and PEXPIREAT */
        buf = catAppendOnlyGenericCommand(buf,3,argv);
        for (i = 3; i < argc; i ++) {
            if (!strcasecmp(argv[i]->ptr, "ex")) exarg = argv[i+1];
            if (!strcasecmp(argv[i]->ptr, "px")) pxarg = argv[i+1];
        }
        serverAssert(!(exarg && pxarg));
        if (exarg)
            buf = catAppendOnlyExpireAtCommand(buf,server.expireCommand,argv[1],
                                               exarg);
        if (pxarg)
            buf = catAppendOnlyExpireAtCommand(buf,server.pexpireCommand,argv[1],
                                               pxarg);
    } else {
        /* All the other commands don't need translation or need the
         * same translation already operated in the command vector
         * for the replication itself. */
        buf = catAppendOnlyGenericCommand(buf,argc,argv);
    }

    /* Append to the AOF buffer. This will be flushed on disk just before
     * of re-entering the event loop, so before the client will get a
     * positive reply about the operation performed. */
    if (server.aof_state == AOF_ON)
        server.aof_buf = sdscatlen(server.aof_buf,buf,sdslen(buf));

    /* If a background append only file rewriting is in progress we want to
     * accumulate the differences between the child DB and the current one
     * in a buffer, so that when the child process will do its work we
     * can append the differences to the new append only file. */
    if (server.aof_child_pid != -1)
        aofRewriteBufferAppend((unsigned char*)buf,sdslen(buf));

    sdsfree(buf);
  }
  ```   
  该函数中，首先判断本次命令的数据库索引dictid，是否与上次命令的数据库索引server.aof_selected_db相同，如果不同，则编码select命令；    
  如果命令为EXPIRE、PEXPIRE或者EXPIREAT，则调用catAppendOnlyExpireAtCommand将命令编码为PEXPIREAT命令的格式；    
  如果命令为setex或psetex，则先调用catAppendOnlyGenericCommand编码SET命令，然后调用catAppendOnlyExpireAtCommand编码PEXPIREAT命令；     
  其他命令直接用catAppendOnlyGenericCommand对命令进行编码；     
  如果server.aof_state为REDIS_AOF_ON，则说明开启了AOF功能，将编码后的buf追加到AOF缓存server.aof_buf中；            
  另外，如果server.aof_child_pid不是-1，说明有子进程在进行AOF重写，则调用aofRewriteBufferAppend将编码后的buf追加到AOF重写缓存server.aof_rewrite_buf_blocks中。         
  2.文件写入、文件同步
  为了提高文件的写入效率，在现代操作系统中，当用户调用write函数将数据写入到文件描述符后，操作系统通常会将写入数据暂时保存在一个内存缓冲区里面，等到缓冲区的空间被填满、或者超过了指定的时限之后，操作系统才真正地将缓冲区中的数据写入到磁盘里面。这种做法虽然提高了效率，但也为写入数据带来了安全问题，如果计算机发生宕机，那么保存在内存缓冲区里面的写入数据将会丢失。为此，操作系统提供了fsync同步函数，可以手动让操作系统立即将缓冲区中的数据写入到硬盘里面，从而确保写入数据的安全性。Redis服务器的主循环中，每隔一段时间就会将AOF缓存server.aof_buf中的内容写入到AOF文件中。并且根据同步策略的不同，而选择不同的时机进行fsync。同步策略通过配置文件中的appendfsync选项设置，总共有三种同步策略，分别是：        
  a：appendfsync  no  

  不执行fsync操作，完全交由操作系统进行同步。这种方式是最快的，但也是最不安全的。

  b：appendfsync  always

  每次调用write将AOF缓存server.aof_buf中的内容写入到AOF文件时，立即调用fsync函数。这种方式是最安全的，却也是最慢的。

  c：appendfsync  everysec

  每隔1秒钟进行一次fsync操作，这是一种对速度和安全性进行折中的方法。如果用户没有设置appendfsync选项的值，则使用everysec作为选项默认值。

  将AOF缓存server.aof_buf中的内容写入到AOF文件中。并且根据同步策略的不同，而选择不同的时机进行fsync。这都是在函数flushAppendOnlyFile中实现的，其代码如下：     
  ```
  void flushAppendOnlyFile(int force) {
    ssize_t nwritten;
    int sync_in_progress = 0;
    mstime_t latency;

    if (sdslen(server.aof_buf) == 0) return;

    if (server.aof_fsync == AOF_FSYNC_EVERYSEC)
        sync_in_progress = bioPendingJobsOfType(BIO_AOF_FSYNC) != 0;

    if (server.aof_fsync == AOF_FSYNC_EVERYSEC && !force) {
        /* With this append fsync policy we do background fsyncing.
         * If the fsync is still in progress we can try to delay
         * the write for a couple of seconds. */
        if (sync_in_progress) {
            if (server.aof_flush_postponed_start == 0) {
                /* No previous write postponing, remember that we are
                 * postponing the flush and return. */
                server.aof_flush_postponed_start = server.unixtime;
                return;
            } else if (server.unixtime - server.aof_flush_postponed_start < 2) {
                /* We were already waiting for fsync to finish, but for less
                 * than two seconds this is still ok. Postpone again. */
                return;
            }
            /* Otherwise fall trough, and go write since we can't wait
             * over two seconds. */
            server.aof_delayed_fsync++;
            serverLog(LL_NOTICE,"Asynchronous AOF fsync is taking too long (disk is busy?). Writing the AOF buffer without waiting for fsync to complete, this may slow down Redis.");
        }
    }
    /* We want to perform a single write. This should be guaranteed atomic
     * at least if the filesystem we are writing is a real physical one.
     * While this will save us against the server being killed I don't think
     * there is much to do about the whole server stopping for power problems
     * or alike */

    latencyStartMonitor(latency);
    nwritten = aofWrite(server.aof_fd,server.aof_buf,sdslen(server.aof_buf));
    latencyEndMonitor(latency);
    /* We want to capture different events for delayed writes:
     * when the delay happens with a pending fsync, or with a saving child
     * active, and when the above two conditions are missing.
     * We also use an additional event name to save all samples which is
     * useful for graphing / monitoring purposes. */
    if (sync_in_progress) {
        latencyAddSampleIfNeeded("aof-write-pending-fsync",latency);
    } else if (server.aof_child_pid != -1 || server.rdb_child_pid != -1) {
        latencyAddSampleIfNeeded("aof-write-active-child",latency);
    } else {
        latencyAddSampleIfNeeded("aof-write-alone",latency);
    }
    latencyAddSampleIfNeeded("aof-write",latency);

    /* We performed the write so reset the postponed flush sentinel to zero. */
    server.aof_flush_postponed_start = 0;

    if (nwritten != (ssize_t)sdslen(server.aof_buf)) {
        static time_t last_write_error_log = 0;
        int can_log = 0;

        /* Limit logging rate to 1 line per AOF_WRITE_LOG_ERROR_RATE seconds. */
        if ((server.unixtime - last_write_error_log) > AOF_WRITE_LOG_ERROR_RATE) {
            can_log = 1;
            last_write_error_log = server.unixtime;
        }

        /* Log the AOF write error and record the error code. */
        if (nwritten == -1) {
            if (can_log) {
                serverLog(LL_WARNING,"Error writing to the AOF file: %s",
                    strerror(errno));
                server.aof_last_write_errno = errno;
            }
        } else {
            if (can_log) {
                serverLog(LL_WARNING,"Short write while writing to "
                                       "the AOF file: (nwritten=%lld, "
                                       "expected=%lld)",
                                       (long long)nwritten,
                                       (long long)sdslen(server.aof_buf));
            }

            if (ftruncate(server.aof_fd, server.aof_current_size) == -1) {
                if (can_log) {
                    serverLog(LL_WARNING, "Could not remove short write "
                             "from the append-only file.  Redis may refuse "
                             "to load the AOF the next time it starts.  "
                             "ftruncate: %s", strerror(errno));
                }
            } else {
                /* If the ftruncate() succeeded we can set nwritten to
                 * -1 since there is no longer partial data into the AOF. */
                nwritten = -1;
            }
            server.aof_last_write_errno = ENOSPC;
        }

        /* Handle the AOF write error. */
        if (server.aof_fsync == AOF_FSYNC_ALWAYS) {
            /* We can't recover when the fsync policy is ALWAYS since the
             * reply for the client is already in the output buffers, and we
             * have the contract with the user that on acknowledged write data
             * is synced on disk. */
            serverLog(LL_WARNING,"Can't recover from AOF write error when the AOF fsync policy is 'always'. Exiting...");
            exit(1);
        } else {
            /* Recover from failed write leaving data into the buffer. However
             * set an error to stop accepting writes as long as the error
             * condition is not cleared. */
            server.aof_last_write_status = C_ERR;

            /* Trim the sds buffer if there was a partial write, and there
             * was no way to undo it with ftruncate(2). */
            if (nwritten > 0) {
                server.aof_current_size += nwritten;
                sdsrange(server.aof_buf,nwritten,-1);
            }
            return; /* We'll try again on the next call... */
        }
    } else {
        /* Successful write(2). If AOF was in error state, restore the
         * OK state and log the event. */
        if (server.aof_last_write_status == C_ERR) {
            serverLog(LL_WARNING,
                "AOF write error looks solved, Redis can write again.");
            server.aof_last_write_status = C_OK;
        }
    }
    server.aof_current_size += nwritten;

    /* Re-use AOF buffer when it is small enough. The maximum comes from the
     * arena size of 4k minus some overhead (but is otherwise arbitrary). */
    if ((sdslen(server.aof_buf)+sdsavail(server.aof_buf)) < 4000) {
        sdsclear(server.aof_buf);
    } else {
        sdsfree(server.aof_buf);
        server.aof_buf = sdsempty();
    }

    /* Don't fsync if no-appendfsync-on-rewrite is set to yes and there are
     * children doing I/O in the background. */
    if (server.aof_no_fsync_on_rewrite &&
        (server.aof_child_pid != -1 || server.rdb_child_pid != -1))
            return;

    /* Perform the fsync if needed. */
    if (server.aof_fsync == AOF_FSYNC_ALWAYS) {
        /* aof_fsync is defined as fdatasync() for Linux in order to avoid
         * flushing metadata. */
        latencyStartMonitor(latency);
        aof_fsync(server.aof_fd); /* Let's try to get this data on the disk */
        latencyEndMonitor(latency);
        latencyAddSampleIfNeeded("aof-fsync-always",latency);
        server.aof_last_fsync = server.unixtime;
    } else if ((server.aof_fsync == AOF_FSYNC_EVERYSEC &&
                server.unixtime > server.aof_last_fsync)) {
        if (!sync_in_progress) aof_background_fsync(server.aof_fd);
        server.aof_last_fsync = server.unixtime;
    }
  }
  ```    
  如果参数force置为0，并且fsync策略设置为everysec，并且有后台线程尚在进行fsync操作，因为Linux的write操作会被后台的fsync阻塞，因此需要延迟write操作。这种情况下，只需记住尚有缓存需要write，后续在serverCron中再次调用flushAppendOnlyFile函数时再进行write操作。如果force置为1，则不管是否有后台在fsync，都会进行write操作。     

  首先，如果server.aof_fsync为AOF_FSYNC_EVERYSEC，则查看是否有其他fsync任务正在执行，有则sync_in_progress为1，否则sync_in_progress为0；

  如果server.aof_fsync为AOF_FSYNC_EVERYSEC，并且参数force为0，并且后台有fsync任务正在执行，则需要延迟write操作，延迟策略是：

  a：若server.aof_flush_postponed_start为0，说明这是首次推迟write操作，将当前时间戳记录到server.aof_flush_postponed_start中，然后直接返回；

  b：若server.unixtime- server.aof_flush_postponed_start < 2，说明上次已经推迟了write操作，但是上次推迟时间距当前时间在2s以内，直接返回；

  c：不满足以上条件，说明上次推迟时间已经超过2s，则server.aof_delayed_fsync++，并且记录日志，不再等待fsync完成，下面直接开始进行写操作；   

  接下来进行单次写操作，调用write将server.aof_buf所有内容写入到server.aof_fd中。如果写入字节数不等于server.aof_buf总长度，则根据不同的情况写入不同的错误信息到日志中，并且，如果写入了部分数据，则调用ftruncate将这部分数据删除；

  写入失败的情况下，如果server.aof_fsync为AOF_FSYNC_ALWAYS，说明已经向客户端承诺数据必须同步到磁盘中，这种情况下，写入失败直接exit；

  如果server.aof_fsync不是AOF_FSYNC_ALWAYS，并且之前ftruncate失败的话，则将写入的字节数增加到当前AOF文件长度server.aof_current_size中，然后截取server.aof_buf为未写入的部分，然后返回，等待下次写入；  

  写入成功，则将写入字节数增加到server.aof_current_size中，然后重置缓存server.aof_buf；

  如果server.aof_fsync为AOF_FSYNC_ALWAYS，则调用aof_fsync确保数据确实写入磁盘，并且记录延迟时间；

  如果server.aof_fsync为AOF_FSYNC_EVERYSEC，并且server.unixtime大于server.aof_last_fsync，并且当前没有fsync任务，则调用aof_background_fsync增加后台fsync任务；最后更新server.aof_last_fsync为server.unixtime    

  **二：加载AOF文件**     
  Redis服务器启动时，如果AOF功能开启的话，则需要根据AOF文件的内容恢复到数据中。

  Redis加载AOF文件的方式非常巧妙，因为AOF中记录的是统一请求协议格式的客户端命令，因此Redis创建一个不带网络连接的伪客户端，通过伪客户端逐条执行AOF中的命令来恢复数据。主要实现是在函数loadAppendOnlyFile中，代码如下：    
  ```
  int loadAppendOnlyFile(char *filename) {
    struct client *fakeClient;
    FILE *fp = fopen(filename,"r");
    struct redis_stat sb;
    int old_aof_state = server.aof_state;
    long loops = 0;
    off_t valid_up_to = 0; /* Offset of latest well-formed command loaded. */

    if (fp == NULL) {
        serverLog(LL_WARNING,"Fatal error: can't open the append log file for reading: %s",strerror(errno));
        exit(1);
    }

    /* Handle a zero-length AOF file as a special case. An emtpy AOF file
     * is a valid AOF because an empty server with AOF enabled will create
     * a zero length file at startup, that will remain like that if no write
     * operation is received. */
    if (fp && redis_fstat(fileno(fp),&sb) != -1 && sb.st_size == 0) {
        server.aof_current_size = 0;
        fclose(fp);
        return C_ERR;
    }

    /* Temporarily disable AOF, to prevent EXEC from feeding a MULTI
     * to the same file we're about to read. */
    server.aof_state = AOF_OFF;

    fakeClient = createFakeClient();
    startLoading(fp);

    /* Check if this AOF file has an RDB preamble. In that case we need to
     * load the RDB file and later continue loading the AOF tail. */
    char sig[5]; /* "REDIS" */
    if (fread(sig,1,5,fp) != 5 || memcmp(sig,"REDIS",5) != 0) {
        /* No RDB preamble, seek back at 0 offset. */
        if (fseek(fp,0,SEEK_SET) == -1) goto readerr;
    } else {
        /* RDB preamble. Pass loading the RDB functions. */
        rio rdb;

        serverLog(LL_NOTICE,"Reading RDB preamble from AOF file...");
        if (fseek(fp,0,SEEK_SET) == -1) goto readerr;
        rioInitWithFile(&rdb,fp);
        if (rdbLoadRio(&rdb,NULL,1) != C_OK) {
            serverLog(LL_WARNING,"Error reading the RDB preamble of the AOF file, AOF loading aborted");
            goto readerr;
        } else {
            serverLog(LL_NOTICE,"Reading the remaining AOF tail...");
        }
    }

    /* Read the actual AOF file, in REPL format, command by command. */
    while(1) {
        int argc, j;
        unsigned long len;
        robj **argv;
        char buf[128];
        sds argsds;
        struct redisCommand *cmd;

        /* Serve the clients from time to time */
        if (!(loops++ % 1000)) {
            loadingProgress(ftello(fp));
            processEventsWhileBlocked();
        }

        if (fgets(buf,sizeof(buf),fp) == NULL) {
            if (feof(fp))
                break;
            else
                goto readerr;
        }
        if (buf[0] != '*') goto fmterr;
        if (buf[1] == '\0') goto readerr;
        argc = atoi(buf+1);
        if (argc < 1) goto fmterr;

        argv = zmalloc(sizeof(robj*)*argc);
        fakeClient->argc = argc;
        fakeClient->argv = argv;

        for (j = 0; j < argc; j++) {
            if (fgets(buf,sizeof(buf),fp) == NULL) {
                fakeClient->argc = j; /* Free up to j-1. */
                freeFakeClientArgv(fakeClient);
                goto readerr;
            }
            if (buf[0] != '$') goto fmterr;
            len = strtol(buf+1,NULL,10);
            argsds = sdsnewlen(NULL,len);
            if (len && fread(argsds,len,1,fp) == 0) {
                sdsfree(argsds);
                fakeClient->argc = j; /* Free up to j-1. */
                freeFakeClientArgv(fakeClient);
                goto readerr;
            }
            argv[j] = createObject(OBJ_STRING,argsds);
            if (fread(buf,2,1,fp) == 0) {
                fakeClient->argc = j+1; /* Free up to j. */
                freeFakeClientArgv(fakeClient);
                goto readerr; /* discard CRLF */
            }
        }

        /* Command lookup */
        cmd = lookupCommand(argv[0]->ptr);
        if (!cmd) {
            serverLog(LL_WARNING,"Unknown command '%s' reading the append only file", (char*)argv[0]->ptr);
            exit(1);
        }

        /* Run the command in the context of a fake client */
        fakeClient->cmd = cmd;
        cmd->proc(fakeClient);

        /* The fake client should not have a reply */
        serverAssert(fakeClient->bufpos == 0 && listLength(fakeClient->reply) == 0);
        /* The fake client should never get blocked */
        serverAssert((fakeClient->flags & CLIENT_BLOCKED) == 0);

        /* Clean up. Command code may have changed argv/argc so we use the
         * argv/argc of the client instead of the local variables. */
        freeFakeClientArgv(fakeClient);
        fakeClient->cmd = NULL;
        if (server.aof_load_truncated) valid_up_to = ftello(fp);
    }

    /* This point can only be reached when EOF is reached without errors.
     * If the client is in the middle of a MULTI/EXEC, log error and quit. */
    if (fakeClient->flags & CLIENT_MULTI) goto uxeof;

  loaded_ok: /* DB loaded, cleanup and return C_OK to the caller. */
    fclose(fp);
    freeFakeClient(fakeClient);
    server.aof_state = old_aof_state;
    stopLoading();
    aofUpdateCurrentSize();
    server.aof_rewrite_base_size = server.aof_current_size;
    return C_OK;

  readerr: /* Read error. If feof(fp) is true, fall through to unexpected EOF. */
    if (!feof(fp)) {
        if (fakeClient) freeFakeClient(fakeClient); /* avoid valgrind warning */
        serverLog(LL_WARNING,"Unrecoverable error reading the append only file: %s", strerror(errno));
        exit(1);
    }

  uxeof: /* Unexpected AOF end of file. */
    if (server.aof_load_truncated) {
        serverLog(LL_WARNING,"!!! Warning: short read while loading the AOF file !!!");
        serverLog(LL_WARNING,"!!! Truncating the AOF at offset %llu !!!",
            (unsigned long long) valid_up_to);
        if (valid_up_to == -1 || truncate(filename,valid_up_to) == -1) {
            if (valid_up_to == -1) {
                serverLog(LL_WARNING,"Last valid command offset is invalid");
            } else {
                serverLog(LL_WARNING,"Error truncating the AOF file: %s",
                    strerror(errno));
            }
        } else {
            /* Make sure the AOF file descriptor points to the end of the
             * file after the truncate call. */
            if (server.aof_fd != -1 && lseek(server.aof_fd,0,SEEK_END) == -1) {
                serverLog(LL_WARNING,"Can't seek the end of the AOF file: %s",
                    strerror(errno));
            } else {
                serverLog(LL_WARNING,
                    "AOF loaded anyway because aof-load-truncated is enabled");
                goto loaded_ok;
            }
        }
    }
    if (fakeClient) freeFakeClient(fakeClient); /* avoid valgrind warning */
    serverLog(LL_WARNING,"Unexpected end of file reading the append only file. You can: 1) Make a backup of your AOF file, then use ./redis-check-aof --fix <filename>. 2) Alternatively you can set the 'aof-load-truncated' configuration option to yes and restart the server.");
    exit(1);

  fmterr: /* Format error. */
    if (fakeClient) freeFakeClient(fakeClient); /* avoid valgrind warning */
    serverLog(LL_WARNING,"Bad file format reading the append only file: make a backup of your AOF file, then use ./redis-check-aof --fix <filename>");
    exit(1);
  }
  ```  
  函数中，置server.aof_state为REDIS_AOF_OFF，防止向该filename中写入新的AOF数据；因此，Redis在加载AOF文件时，AOF功能是关闭的。

  创建伪客户端，读取文件，根据统一请求协议的格式，将AOF文件内容恢复成客户端命令，然后调用lookupCommand查找命令处理函数，然后执行该函数以恢复数据；然后释放客户端命令；如果server.aof_load_truncated为True，则记录已读取的字节数到valid_up_to中，表示到valid_up_to个字节为止，AOF文件还是一切正常的；一直读下去，直到文件末尾，或者格式出错等；

  如果读到文件末尾并未出错，则关闭文件，释放伪客户端，恢复状态，调用stopLoading标记停止load过程，调用aofUpdateCurrentSize更新server.aof_current_size为AOF文件长度；返回REDIS_OK。

  如果读取文件时发生read错误，若还没读到文件末尾，则直接记录错误日志，然后退出；否则，如果读取中，应该还有数据的时候，却读到了文件末尾，则：

  如果server.aof_load_truncated为True，则调用truncate将AOF文件截断为valid_up_to，如果valid_up_to为-1，或者截断失败，则记录错误日志，然后exit退出；否则，截断成功，使描述符server.aof_fd的状态指向文件末尾，然后当做加载AOF成功处理；

  其他情况，一律记录日志，exit退出。    

  **三：AOF重写**   
  AOF持久化是通过保存执行的写命令来记录数据库状态的，随着服务器运行，AOF文件中的冗余内容会越来越多，文件的体积也会越来越大。

  为了解决AOF文件体积膨胀的问题，Redis提供了AOF文件重写功能。通过该功能，Redis服务器可以创建一个新的AOF文件来替代现有的AOF文件，新旧两个AOF文件所保存的数据库状态相同，但新AOF文件不会包含任何浪费空间的冗余命令，所以新AOF文件的体积通常会比旧AOF文件的体积要小得多。

  AOF文件重写并不需要对现有的AOF文件进行任何读取、分析或者写人操作，这个功能是通过读取服务器当前的数据库状态来实现的。   
  1：后台AOF重写

  为了防止重写AOF阻塞服务器，该过程在后台进行的。方法就是Redis服务器fork一个子进程，由子进程进行AOF的重写。任何时刻只能有一个子进程在进行AOF重写。

  注意，调用fork时，子进程的内存与父进程（Redis服务器）是一模一样的，因此子进程中的数据库状态也就是服务器此刻的状态。而此时父进程继续接受来自客户端的命令，这就会产生新的数据。

  为了使最终的AOF文件与数据库状态尽可能的一致，父进程处理客户端新到来的命令时，会将该命令缓存到server.aof_rewrite_buf_blocks中，并在合适的时机将server.aof_rewrite_buf_blocks中的内容，通过管道发送给子进程。这就是在之前介绍过的命令追加的实现函数feedAppendOnlyFile最后一步所进行的操作。

  父进程需要把缓存的新数据发给子进程，这就需要创建一系列用于父子进程间通信的管道，总共有3个管道：

  管道1用于父进程向子进程发送缓存的新数据。子进程在重写AOF时，定期从该管道中读取数据并缓存起来，并在最后将缓存的数据写入重写的AOF文件；

  管道2负责子进程向父进程发送结束信号。由于父进程在不断的接收客户端命令，但是子进程不能无休止的等待父进程的数据，因此，子进程在遍历完数据库所有数据之后，从管道1中执行一段时间的读取操作后，就会向管道2中发送一个"!"，父进程收到子进程的"!"后，就会置server.aof_stop_sending_diff为1，表示不再向子进程发送缓存的数据了；

  管道3负责父进程向子进程发送应答信号。父进程收到子进程的"!"后，会通过该管道也向子进程应答一个"!"，表示已收到了停止信号。

  子进程执行重写AOF的过程很简单，就是根据fork时刻的数据库状态，依次轮训Redis的server.dbnum个数据库，遍历每个数据库中的每个键值对数据，进行AOF重写工作。每当重写了10k的数据后，就会从管道1中读取服务器（父进程）缓存的新数据，并缓存到server.aof_child_diff中。

  子进程遍历完所有数据后，再次从管道1中读取服务器（父进程）缓存的新数据，读取一段时间后，向管道2中发送字符"!"，以使父进程停止发送缓存的新数据；然后从管道3中读取父进程的回应。最后，将server.aof_child_diff中的内容写入重写的AOF文件，最终完成了AOF重写的主要过程。   

  在子进程中调用rewriteAppendOnlyFile函数进行AOF重写，其代码如下：    
  ```
  int rewriteAppendOnlyFileRio(rio *aof) {
    dictIterator *di = NULL;
    dictEntry *de;
    size_t processed = 0;
    long long now = mstime();
    int j;

    for (j = 0; j < server.dbnum; j++) {
        char selectcmd[] = "*2\r\n$6\r\nSELECT\r\n";
        redisDb *db = server.db+j;
        dict *d = db->dict;
        if (dictSize(d) == 0) continue;
        di = dictGetSafeIterator(d);

        /* SELECT the new DB */
        if (rioWrite(aof,selectcmd,sizeof(selectcmd)-1) == 0) goto werr;
        if (rioWriteBulkLongLong(aof,j) == 0) goto werr;

        /* Iterate this DB writing every entry */
        while((de = dictNext(di)) != NULL) {
            sds keystr;
            robj key, *o;
            long long expiretime;

            keystr = dictGetKey(de);
            o = dictGetVal(de);
            initStaticStringObject(key,keystr);

            expiretime = getExpire(db,&key);

            /* If this key is already expired skip it */
            if (expiretime != -1 && expiretime < now) continue;

            /* Save the key and associated value */
            if (o->type == OBJ_STRING) {
                /* Emit a SET command */
                char cmd[]="*3\r\n$3\r\nSET\r\n";
                if (rioWrite(aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                /* Key and value */
                if (rioWriteBulkObject(aof,&key) == 0) goto werr;
                if (rioWriteBulkObject(aof,o) == 0) goto werr;
            } else if (o->type == OBJ_LIST) {
                if (rewriteListObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_SET) {
                if (rewriteSetObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_ZSET) {
                if (rewriteSortedSetObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_HASH) {
                if (rewriteHashObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_MODULE) {
                if (rewriteModuleObject(aof,&key,o) == 0) goto werr;
            } else {
                serverPanic("Unknown object type");
            }
            /* Save the expire time */
            if (expiretime != -1) {
                char cmd[]="*3\r\n$9\r\nPEXPIREAT\r\n";
                if (rioWrite(aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                if (rioWriteBulkObject(aof,&key) == 0) goto werr;
                if (rioWriteBulkLongLong(aof,expiretime) == 0) goto werr;
            }
            /* Read some diff from the parent process from time to time. */
            if (aof->processed_bytes > processed+AOF_READ_DIFF_INTERVAL_BYTES) {
                processed = aof->processed_bytes;
                aofReadDiffFromParent();
            }
        }
        dictReleaseIterator(di);
        di = NULL;
    }
    return C_OK;

  werr:
    if (di) dictReleaseIterator(di);
    return C_ERR;
  }
  ```  
  首先创建并打开临时文件temp-rewriteaof-<pid>.aof，然后用该文件的文件指针fp初始化rio结构的aof；然后初始化server.aof_child_diff，它用于缓存父进程发来的新数据；

  如果server.aof_rewrite_incremental_fsync为真，则设置aof的io.file.autosync为32M，也就是每写入文件32M数据，就进行一次fsync操作；

  然后，依次轮训Redis的server.dbnum个数据库，开始遍历数据库中的数据，进行AOF重写工作。

  首先是将"*2\r\n$6\r\nSELECT\r\n"以及当前的数据库索引写入aof中；

  然后利用字典迭代器di，从数据库的字典中依次取出键key，值对象o，以及超时时间expiretime，如果该key已经超时，则不再写入aof；然后根据值对象的类型调用不同的函数写入aof中：字符串对象，每次写入一个键值对，将命令"set key value"按照统一请求协议的格式写入aof中；列表对象调用rewriteListObject写入；集合对象调用rewriteSetObject写入；有序集合对象调用rewriteSortedSetObject写入；哈希对象调用rewriteHashObject写入。

  写入键值对后，如果该键设置了超时时间，则还写入一个PEXPIREAT命令；

  每当写入10k的数据后，就调用aofReadDiffFromParent，从管道中读取服务器（父进程）缓存的新数据，追加到server.aof_child_diff中；

  所有数据库的所有数据都重写完之后，先调用一次fflush和fsync操作，从而使aof文件内容确实写入磁盘。因父进程还在不断的发送新数据，这样可以使后续的fsync操作快一些；

  注意，在子进程中可以直接调用fsync，因为它不会阻塞Redis服务器，而在父进程中，调用fsync、unlink等可能阻塞服务器的函数时，需要小心调用，大多是通过后台线程完成的。

  接下来，再次调用aofReadDiffFromParent从父进程中读取累积的新数据，因为父进程从客户端接收数据的速度可能大于其向子进程发送数据的速度，所以这里最多耗时1s的时间进行读取，并且如果有20次读取不到数据时，直接就停止该过程；

  接下来，向管道server.aof_pipe_write_ack_to_parent中发送字符"!"，以使父进程停止发送缓存的新数据；然后从管道server.aof_pipe_read_ack_from_parent中，尝试读取父进程的回应"!"，这里读取的超时时间为5s；

  然后，最后一次调用aofReadDiffFromParent，读取管道中的剩余数据；并将server.aof_child_diff的内容写入到aof中；然后调用fflush和fsync，保证aof文件内容确实写入磁盘；然后fclose(fp)；

  最后，将临时文件改名为filename，并返回REDIS_OK。注意，这里的参数filename，其实也是一个临时文件，其值为temp-rewriteaof-bg-<pid>.aof，子进程之所以将重写的AOF文件记录到临时文件中，是因为此时父进程还在向旧的AOF文件中追加命令。当子进程完成AOF重写之后，父进程就会进行收尾工作，用新的重写AOF文件，替换旧的AOF文件。    

  2：AOF重写收尾工作

  子进程执行完AOF重写后退出，父进程wait得到该子进程的退出状态后，进行AOF重写的收尾工作：

  首先将服务器缓存的，剩下的新数据写入该临时文件中，这样该AOF临时文件就完全与当前数据库状态一致了；然后将临时文件改名为配置的AOF文件，并且更改AOF文件描述符，该过程中，为了避免删除操作会阻塞服务器，会使用后台线程进行close操作。

  该过程由函数backgroundRewriteDoneHandler实现，代码如下：    
  ```
  void backgroundRewriteDoneHandler(int exitcode, int bysignal) {
    if (!bysignal && exitcode == 0) {
        int newfd, oldfd;
        char tmpfile[256];
        long long now = ustime();
        mstime_t latency;

        serverLog(LL_NOTICE,
            "Background AOF rewrite terminated with success");

        /* Flush the differences accumulated by the parent to the
         * rewritten AOF. */
        latencyStartMonitor(latency);
        snprintf(tmpfile,256,"temp-rewriteaof-bg-%d.aof",
            (int)server.aof_child_pid);
        newfd = open(tmpfile,O_WRONLY|O_APPEND);
        if (newfd == -1) {
            serverLog(LL_WARNING,
                "Unable to open the temporary AOF produced by the child: %s", strerror(errno));
            goto cleanup;
        }

        if (aofRewriteBufferWrite(newfd) == -1) {
            serverLog(LL_WARNING,
                "Error trying to flush the parent diff to the rewritten AOF: %s", strerror(errno));
            close(newfd);
            goto cleanup;
        }
        latencyEndMonitor(latency);
        latencyAddSampleIfNeeded("aof-rewrite-diff-write",latency);

        serverLog(LL_NOTICE,
            "Residual parent diff successfully flushed to the rewritten AOF (%.2f MB)", (double) aofRewriteBufferSize() / (1024*1024));

        /* The only remaining thing to do is to rename the temporary file to
         * the configured file and switch the file descriptor used to do AOF
         * writes. We don't want close(2) or rename(2) calls to block the
         * server on old file deletion.
         *
         * There are two possible scenarios:
         *
         * 1) AOF is DISABLED and this was a one time rewrite. The temporary
         * file will be renamed to the configured file. When this file already
         * exists, it will be unlinked, which may block the server.
         *
         * 2) AOF is ENABLED and the rewritten AOF will immediately start
         * receiving writes. After the temporary file is renamed to the
         * configured file, the original AOF file descriptor will be closed.
         * Since this will be the last reference to that file, closing it
         * causes the underlying file to be unlinked, which may block the
         * server.
         *
         * To mitigate the blocking effect of the unlink operation (either
         * caused by rename(2) in scenario 1, or by close(2) in scenario 2), we
         * use a background thread to take care of this. First, we
         * make scenario 1 identical to scenario 2 by opening the target file
         * when it exists. The unlink operation after the rename(2) will then
         * be executed upon calling close(2) for its descriptor. Everything to
         * guarantee atomicity for this switch has already happened by then, so
         * we don't care what the outcome or duration of that close operation
         * is, as long as the file descriptor is released again. */
        if (server.aof_fd == -1) {
            /* AOF disabled */

            /* Don't care if this fails: oldfd will be -1 and we handle that.
             * One notable case of -1 return is if the old file does
             * not exist. */
            oldfd = open(server.aof_filename,O_RDONLY|O_NONBLOCK);
        } else {
            /* AOF enabled */
            oldfd = -1; /* We'll set this to the current AOF filedes later. */
        }

        /* Rename the temporary file. This will not unlink the target file if
         * it exists, because we reference it with "oldfd". */
        latencyStartMonitor(latency);
        if (rename(tmpfile,server.aof_filename) == -1) {
            serverLog(LL_WARNING,
                "Error trying to rename the temporary AOF file %s into %s: %s",
                tmpfile,
                server.aof_filename,
                strerror(errno));
            close(newfd);
            if (oldfd != -1) close(oldfd);
            goto cleanup;
        }
        latencyEndMonitor(latency);
        latencyAddSampleIfNeeded("aof-rename",latency);

        if (server.aof_fd == -1) {
            /* AOF disabled, we don't need to set the AOF file descriptor
             * to this new file, so we can close it. */
            close(newfd);
        } else {
            /* AOF enabled, replace the old fd with the new one. */
            oldfd = server.aof_fd;
            server.aof_fd = newfd;
            if (server.aof_fsync == AOF_FSYNC_ALWAYS)
                aof_fsync(newfd);
            else if (server.aof_fsync == AOF_FSYNC_EVERYSEC)
                aof_background_fsync(newfd);
            server.aof_selected_db = -1; /* Make sure SELECT is re-issued */
            aofUpdateCurrentSize();
            server.aof_rewrite_base_size = server.aof_current_size;

            /* Clear regular AOF buffer since its contents was just written to
             * the new AOF from the background rewrite buffer. */
            sdsfree(server.aof_buf);
            server.aof_buf = sdsempty();
        }

        server.aof_lastbgrewrite_status = C_OK;

        serverLog(LL_NOTICE, "Background AOF rewrite finished successfully");
        /* Change state from WAIT_REWRITE to ON if needed */
        if (server.aof_state == AOF_WAIT_REWRITE)
            server.aof_state = AOF_ON;

        /* Asynchronously close the overwritten AOF. */
        if (oldfd != -1) bioCreateBackgroundJob(BIO_CLOSE_FILE,(void*)(long)oldfd,NULL,NULL);

        serverLog(LL_VERBOSE,
            "Background AOF rewrite signal handler took %lldus", ustime()-now);
    } else if (!bysignal && exitcode != 0) {
        /* SIGUSR1 is whitelisted, so we have a way to kill a child without
         * tirggering an error conditon. */
        if (bysignal != SIGUSR1)
            server.aof_lastbgrewrite_status = C_ERR;
        serverLog(LL_WARNING,
            "Background AOF rewrite terminated with error");
    } else {
        server.aof_lastbgrewrite_status = C_ERR;

        serverLog(LL_WARNING,
            "Background AOF rewrite terminated by signal %d", bysignal);
    }

  cleanup:
    aofClosePipes();
    aofRewriteBufferReset();
    aofRemoveTempFile(server.aof_child_pid);
    server.aof_child_pid = -1;
    server.aof_rewrite_time_last = time(NULL)-server.aof_rewrite_time_start;
    server.aof_rewrite_time_start = -1;
    /* Schedule a new rewrite if we are waiting for it to switch the AOF ON. */
    if (server.aof_state == AOF_WAIT_REWRITE)
        server.aof_rewrite_scheduled = 1;
  }
  ```   
  如果子进程执行失败，或者被信号杀死，则标记server.aof_lastbgrewrite_status为REDIS_ERR，然后记录日志错误信息；

  如果子进程执行AOF重写成功，则首先打开子进程进行AOF重写的临时文件temp-rewriteaof-bg-<pid>.aof，打开的描述符是newfd；

  然后调用aofRewriteBufferWrite，将服务器缓存的剩下的新数据写入该临时文件中，这样该AOF临时文件就完全与当前数据库状态一致了；

  接下来要做的就是将临时文件改名为配置的AOF文件，并且更改AOF文件描述符，该过程中要保证close或rename不会阻塞服务器。有以下两种可能的场景：

  a：AOF功能被禁用，将临时文件改名为配置的AOF文件，当该文件已经存在时会被删除，删除过程可能阻塞服务器；

  b：AOF功能被启用，在将临时文件改名为配置的AOF文件后，会关闭原来的AOF文件描述符，关闭后该文件的描述符引用计数为0，因此会直接删除该文件，这就有可能会阻塞服务器；

  为了避免删除操作会阻塞服务器（可能由于场景1的rename，也可能由于场景2的close），这里使用后台线程进行处理。首先通过打开配置的AOF文件，使场景1转换成场景2。rename操作之后的close操作，将会执行unlink操作。

  首先，如果server.aof_fd为-1，说明AOF功能被禁用，尝试打开配置的AOF文件，描述符为oldfd；否则，置oldfd为-1；

  然后执行rename操作，将临时文件改名为配置的AOF文件，改名成功后，如果server.aof_fd为-1，说明AOF功能被禁用，这种情况直接关闭临时文件的描述符newfd；

  如果server.aof_fd不为-1，将AOF文件描述符server.aof_fd置为newfd，如果server.aof_fsync为AOF_FSYNC_ALWAYS，则直接调用fsync；如果server.aof_fsync为AOF_FSYNC_EVERYSEC，则调用aof_background_fsync由后台线程执行fsync操作；

  然后，置server.aof_selected_db为-1，保证后续添加到AOF中的内容含有SELECT命令；然后调用aofUpdateCurrentSize更新server.aof_current_size；然后释放并重置AOF缓存server.aof_buf；

  如果oldfd不是-1，则将关闭原AOF配置文件的任务放入任务队列中，以使后台线程执行，关闭后，原AOF配置文件就会被删除；

  最后，执行清理工作，调用aofClosePipes关闭重写AOF时使用的管道；调用aofRewriteBufferReset重置重写AOF缓存；删除重写AOF临时文件；设置server.aof_child_pid为-1等；   

- 总结    
  Redis的AOF持久化是从命令的角度去实现的，保存的AOF文件可以阅读，AOF持久化对数据要求准确的任务十分有用    












  
  
  






  



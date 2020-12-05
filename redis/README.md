## redis源码解析      
### 写在前面     
当我在写此文档的时候，我还只是看了一部分redis的代码，只看了redis里面的最底层的数据结构的实现，此时我就感叹，作者真的是太厉害了，每次但代码都在惊叹为何能写出如此优雅的代码，虽然redis代码是基于c实现的，但代码到处充满了对象的思想，层层抽象封装。也在此想写一些文档记录字节阅读redis源码的过程，也方便日后回顾源码。       
网上不外乎有一些人写的源码分析，但我觉得还不够详细，里面的许多实现细节没有讲清楚，最让人烦的是我觉得，阅读源码，必须有图，包括整个数据结构内存的分布，然后有一些表进行归纳总结，所以本人在阅读相关代码过程中也会制作相应的图方便理解。事实上每个源码开头都有相应的函数的实现，但是是英文的，我觉得这些注释对理解作者的设计很有帮助。      
### 文档布局         
我是按照网上某位博主叫做[鹿野](https://www.cnblogs.com/breka/articles/9914787.html)推荐的源码阅读顺序去阅读redis源码，所以我也是按照他所推荐的分成7个阶段来阅读整个源码。每个阶段有一个md文件来阐述，按先后顺序包括以下:           

- [dataStruct.md](./dataStruct.md) 主要是最底层数据结果实现，包括:     
  1.内存分配zmalloc.h和zmalloc.c  
  2.简单动态字符串(simple dynamic string)sds.h和sds.c   
  3.双端通用链表(A generic doubly linked list)adlist.h和adlist.c     
  4.哈希表字典(Hash Tables)dict.h和dict.c    
  5.跳跃表(zskiplist)server.h中的zskiplist结构和t_zset.c中的zsl开头函数    
  6.基数估算算法hyperloglog.c 

- [memoryEncode.md](./memoryEncode.md) 主要是redis内存编码结构的实现，包括:     
  1.整数集合数据结构 intset.h和intset.c    
  2.压缩列表数据结构 ziplist.h和ziplist.c    
  3.快速列表数据结构 quicklist.h和quicklist.c      
  
- [dataType.md](./dataType.md) 主要是redis对象的实现包括:    
  1.string对象,list对象,set对象,hash对象,zset对象,object.c    
  2.字符串对象的命令实现 t_string.c  
  3.列表对象的命令实现 t_list.c    
  4.哈希对象的命令实现 t_hash.c   
  5.集合对象的命令实现 t_set.c   
   

      
  

### 如何利用该文档     
该文档只是帮助你理解redis源码，最好是一般看源码一边结合该文档，按照上述的文档布局的先后顺序去阅读相关代码，相信能够很快的帮助你大致掌握整个redis源码的实现。另外我所阅读的源码是redis-4.0.11,可以去[官网](https://redis.io/download)下载，也可以在终端通过以下方式获得           
```
$ wget https://download.redis.io/releases/redis-4.0.11.tar.gz
$ tar xzf redis-4.0.11.tar.gz  
```    

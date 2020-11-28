## redis数据结构实现源码解析    
整个redisd的数据类型包括各种对象的实现都是基于该篇所讲的数据结构所实现的，作者充分考虑的应用需要和节约内存所以实现了许多数据结构，里面的许多细节很令人佩服我将依次分析各个数据结构的底层实现，并会插以图和表的形式方便理解。     

### 1.zmalloc.h和zmalloc.c     
redis为了方便内存管理和使用，自己对malloc,free函数进行了封装。     

- 设计思想     
  我们知道传统的malloc,free，当我们得到一个动态分配的内存的指针，实际上并不知道该指针指向的内存是多少，也就是说给你一个指针，你知道它指向某个动态分配内存空间，但你不知道具体分配了多少，redis为了清楚每个动态内存的指针，所以采取了"[prefix][user really use]"的形式封装了malloc。prefix是一个固定的size_t类型大小，记录了你指针分配的内存大小，也就是上面的2段总和，最后的"user really use"是暴露给外界的也就是跟正常一样用户申请大小。例如我们用zmalloc函数申请一个10字节大小的空间，下面是真正的内存布局。     
  ![zmalloc](../Pictures/redis_zmalloc1.png)       
  当我们申请10字节内存时，实际申请了sizeof(size_t)+10字节的内存，然后将10保存在size_t中，然后对外返回指针p,当我们拿到了指针p，往前读size_t字节所对应的值就是p所指向的10字节大小，也就知道了任意一个由redis动态分配的内存指针所指向的字节数。      
  但在有些平台本身就是实现了该功能，所以我们经常能够看到这样的宏定义     
  ![zmalloc](../Pictures/redis_zmalloc2.png)     
  其中就是某些平台可以通过某些自带的函数实现相应的功能，所以定义了一个宏HAVE_MALLOC_SIZE,表示如果该平台是否实现了这一功能，Linux下是没有该功能的。     

- 源码分析     
  头文件有以下函数声明          
  ![zmalloc](../Pictures/redis_zmalloc3.png)    

  zmalloc.c文件函数实现细节     
  ![zmalloc](../Pictures/redis_zmalloc4.png)    
  这两个宏定义是为了更新一个zmalloc中维护的全局变量**used_memory**，该变量表示当前redis通过自带malloc函数从内存中分配总的内存空间。另外redis的used_memory记载的内存总是以sizeof(long)长度字节对齐，也就是每一次分配得到的内存都是long类型长度的整数倍。这可能是因为malloc函数底层实现估计也是以long长度对齐分配给用户，所以used_memory更可能是记载从内存实际得到的内存。            
  ```
  static void zmalloc_default_oom(size_t size) #out of memeory的默认处理函数，会退出程序     
  void *zmalloc(size_t size) #对::malloc()函数的封装，申请内存失败会调用默认的out of memory处理函数     
  void *zcalloc(size_t size) #对::calloc(num,size)函数的封装，calloc表示申请num块，size大小的连续内存，并置为0     
  void *zrealloc(void *ptr, size_t size) #对::realloc()函数的封装，realloc会尝试在原来内存位置扩大或者缩小内存，若不能，则会重新分配一块内存，并把旧内存数据拷贝到新内存处。     
  size_t zmalloc_size(void *ptr) #返回的是该指针分配大小，对应着全局静态变量used_memory中加入每一次分配内存大小     
  void zfree(void *ptr) #释放对应的内存，同时也会更新全局静态变量used_memory,从中减去相应内存     
  char *zstrdup(const char *s)复制一个C风格字符串，最后返回由redis管理的指针     
  size_t zmalloc_used_memory(void) #返回全局静态变量used_memory的值，也就是整个内存分配的大小    
  void zmalloc_set_oom_handler(void (*oom_handler)(size_t)) #设置默认的out of memory处理函数   
  #下面是在linux下有个记录程序各种状态的文件/proc/       
  size_t zmalloc_get_rss(void) #rss表示某个进程的常驻内存集(resident set size)，返回该进程所占内存大小    
  float zmalloc_get_fragmentation_ratio(size_t rss) #内存碎片率，表示rss/used_memory大小     
  size_t zmalloc_get_smap_bytes_by_field(char *field, long pid) #获取/proc/pid/smaps中某一field字段的所表示字节大小    
  size_t zmalloc_get_private_dirty(long pid) #获取rss中已改写的私有页面页面大小     
  ```    
  zrealloc()的原理图，假设新分配一个20字节大小的原指针为p的内存。其中realloc()底层会尝试在realptr分配所需字节，失败就重开一块内存，并把原指针指向的内存复制到新内存中，如图所示。   
  ![zmalloc](../Pictures/redis_zmalloc5.png)     

- 总结    
  整个zmalloc，只是对原有的malloc，free的包装，最重要的就是加了一个PREFIX表示该指针所指向的分配的动态内存大小，这也是整个redis的内存基础，后续所有的内存都会基于此来构造各种数据结构。      

### 2.sds.h和sds.c     




  
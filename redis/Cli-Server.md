## 客户端与服务端的代码实现     
### 事件驱动机制的实现     
redis用到的事件驱动机制是自己实现的EventLoop，就是一个循序，在里面不断的处理文件事件和定时器事件，redis简单粗暴的就处理这两种事件，文件事件是和不同客户端的通信，定时器事件主要是设定的一些定时事件包括持久化机制。   
- 设计思想     
  redis的事件驱动机制主要处理的就是文件事件和定时器事件，文件事件主要指和客户端你的交换，定时器事件是指redis服务器设定的一些事件，向=像RDB和AOF重写。redis采用的是EventLoop机制实现的，顶层的文件事件是看不同的服务器平台采用不同的IO复用模型，有select,epoll,evport,kqueue。分别对应的ae_select.c,ae_epoll.c,ae_evport.c和ae_kqueue.c四个源文件，里面实现了一系列函数，函数名都一样，只是基于的模型不一样，统一了接口。由于本机器上实现的是epoll所以后面我会以ae_epoll.c源文件解析    

- 源码解析       
  **ae_epoll.c源文件**     
  ```
  typedef struct aeApiState {
  int epfd;
  struct epoll_event *events;
  } aeApiState;
  ```    
  我么知道epoll模型都会有一个epfd，表明一个创建的epoll模型，然后会向这个epfd中加入或者删除，修改，监听某个描述符的某个事件，redis的事件驱动就封装了这样一个结构，epfd表示用到的epoll模型。events则是一个数组，里面用于保存触发的事件       
  ```
  static int aeApiCreate(aeEventLoop *eventLoop)   #创建一个在eventlool里面的epoll模型   
  static int aeApiResize(aeEventLoop *eventLoop, int setsize) #调整epoll模型最大接收触发事件的尺寸   
  static void aeApiFree(aeEventLoop *eventLoop)  #释放epoll模型    
  static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) #往epoll模型中加入某个描述符需要监听的事件   
  static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int delmask)  #epoll模型中某个描述符取消监听某个事件   
  static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) #epoll模型调用epoll_wait()返回触发事件的结果   
  static char *aeApiName(void) #返回所用的顶层IO复用模型的名字   
  ```    
  其他3个IO复用模型都有相同的接口，然后根据使用redis的不同平台所用的模型是不一样的，在编译的时候就决定了    
  ![eventloop](../Pictures/redis_eventloop1.png)     
  这里可以看到有使用优先级的情况，我们这里所用的是epoll模型    

  **ae.h和ae.c源文件**    
  ![eventloop](../Pictures/redis_eventloop2.png)   
  可以看到四个数据结构分别是     
  1.aeFileEvent:指的是文件事件，也就是描述符顶层epoll模型需要关注的事件，mask包括可读可写，还有一个BARRIER是在写之后读，redis默认的如果一个文件描述符既有可读可写，先读后写。然后就是两个事件处理函数可读，可写处理，还有一个数据clientData表明加入事件处理函数的参数    
  2.aeTimeEvent:指的是定时器事件，id事件的唯一标识，when_sec,when_ms，表明何时该事件处理，一个是事件处理回调函数timePro，还有一个是事件处理完毕回调函数finalizerProc,同样的有一个clientData表示参数，prev,next指针可以看出定时器事件是用一个双向链表拼接的   
  3.aeFiredEvent:这个主要是文件事件被触发之后保存fd和mask数据结构    
  4.aeEventLoop:整个主结构，eventLoop的核心结构，maxfd是当前被处理的最大的文件描述符，setsize是当前最大数量的文件描述符能被同时追踪。timeEventNextId是定时器事件的唯一标识符Id的生成，lastTime是上一次执行一次EventLoop的时间，events就是注册需要监听的事件数组，fired就是触发的事件数组，timeEventHead指向定时器事件的头部指针，stop表明eventloop是否停止，apidata就是封装的epoll模型，beforesleep，aftersleep是在每一次eventloop是否需要调用函数做相应的事情。    

  ```
  aeEventLoop *aeCreateEventLoop(int setsize)  #创建一个eventloop并初始化好，包括为aeFileEvent和aeFiredEvent分配数组空间    
  int aeGetSetSize(aeEventLoop *eventLoop) #得到该eventloop处理的最大监听连接数    
  int aeResizeSetSize(aeEventLoop *eventLoop, int setsize) #调整最大连接数，不会破坏以前已经加入的受监听的描述符   
  void aeDeleteEventLoop(aeEventLoop *eventLoop) #释放整个eventloop    
  void aeStop(aeEventLoop *eventLoop)  #停止整个事件循环   
  int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData) #向该eventloop中加入某个描述符需要监听的事件，以及事件发生的处理函数    
  void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask) #取消监听某个文件事件     
  int aeGetFileEvents(aeEventLoop *eventLoop, int fd)  #得到某个描述符需要监听的事件   
  static void aeGetTime(long *seconds, long *milliseconds) #得到当前时间，加入到seconds和milliseconds中   
  static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) #向当前时间加入milliseconds，其实就是定时器事件定时多少秒用到，相对当前多少秒加入到sec和ms中   
  long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc)#创建一个定时器事件，总是加入到定时器事件的表头   
  int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id) #删除某个定时器事件   
  static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop) #搜寻定时器事件中最早一个事件  
  static int processTimeEvents(aeEventLoop *eventLoop)  #比当前时间早的定时器事件都执行处理    
  int aeProcessEvents(aeEventLoop *eventLoop, int flags) #复杂的一个函数，处理事件，根据flags不同有不同的效果
  ###
  首先我们知道redis主要处理两类事件一个是文件事件一个是定时器事件，文件事件就是客户端的连接，定时器事件就是定时执行持久化操作。我么还知道epoll模型可以等待一段时间返回，或者立即返回，然后看
  是否有事件被触发了，还可以一直阻塞知道有事件触发。    
  ###
  int aeProcessEvents(aeEventLoop *eventLoop, int flags)
  {
    int processed = 0, numevents;
    // 如果既没有文件事件也没有定时器事件就直接返回，一般是不可能出现的，虽然没有客户端连接，但也有持久化这个定时器操作  
    /* Nothing to do? return ASAP */
    if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) return 0;

    /* Note that we want call select() even if there are no
     * file events to process as long as we want to process time
     * events, in order to sleep until the next time event is ready
     * to fire. */
     // 如果有文件事件或者说有定时器事件但需要等待一段时间，才会进入这个函数，否则直接处理定时器事件    
    if (eventLoop->maxfd != -1 ||
        ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
        int j;
        aeTimeEvent *shortest = NULL;
        struct timeval tv, *tvp;
        // 如果有定时器事件，而且需要等待一段时间，那么redis到底等待多少时间呢，也即是说epoll模型要阻塞多长时间呢，其实这个是由定时器最早发生的一个事件距离当前时间决定的    
        // 因为定时器事件必须在一定时间执行，为了尽可能的监听到客户端，这个等待时间就定在最早一个定时器要发生的时间，在这段时间我们可以尽最大可能监听客户端    
        if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT))
            shortest = aeSearchNearestTimer(eventLoop);
        // 当有定时器事件时我们就epoll阻塞事件就是最早开始的定时器事件时间距离当前时间
        if (shortest) {
            long now_sec, now_ms;

            aeGetTime(&now_sec, &now_ms);
            tvp = &tv;

            /* How many milliseconds we need to wait for the next
             * time event to fire? */
            long long ms =
                (shortest->when_sec - now_sec)*1000 +
                shortest->when_ms - now_ms;

            if (ms > 0) {
                tvp->tv_sec = ms/1000;
                tvp->tv_usec = (ms % 1000)*1000;
            } else {
                tvp->tv_sec = 0;
                tvp->tv_usec = 0;
            }
        // 没有定时器事件，如果设置不等待就把阻塞时间tvp设置为0，epoll就不等待立即返回，否则就一直等待客户端指到有事件触发    
        } else {
            /* If we have to check for events but need to return
             * ASAP because of AE_DONT_WAIT we need to set the timeout
             * to zero */
            if (flags & AE_DONT_WAIT) {
                tv.tv_sec = tv.tv_usec = 0;
                tvp = &tv;
            } else {
                /* Otherwise we can block */
                tvp = NULL; /* wait forever */
            }
        }

        /* Call the multiplexing API, will return only on timeout or when
         * some event fires. */
        numevents = aeApiPoll(eventLoop, tvp);

        /* After sleep callback. */
        if (eventLoop->aftersleep != NULL && flags & AE_CALL_AFTER_SLEEP)
            eventLoop->aftersleep(eventLoop);
        // 依次处理文件事件，默认的是如果同时可读可写，先处理读事件再处理写事件，当设置的AE_BARRIER时，可以先处理写事件再处理读事件   
        for (j = 0; j < numevents; j++) {
            aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
            int mask = eventLoop->fired[j].mask;
            int fd = eventLoop->fired[j].fd;
            int fired = 0; /* Number of events fired for current fd. */

            /* Normally we execute the readable event first, and the writable
             * event laster. This is useful as sometimes we may be able
             * to serve the reply of a query immediately after processing the
             * query.
             *
             * However if AE_BARRIER is set in the mask, our application is
             * asking us to do the reverse: never fire the writable event
             * after the readable. In such a case, we invert the calls.
             * This is useful when, for instance, we want to do things
             * in the beforeSleep() hook, like fsynching a file to disk,
             * before replying to a client. */
            int invert = fe->mask & AE_BARRIER;

	    /* Note the "fe->mask & mask & ..." code: maybe an already
             * processed event removed an element that fired and we still
             * didn't processed, so we check if the event is still valid.
             *
             * Fire the readable event if the call sequence is not
             * inverted. */
            if (!invert && fe->mask & mask & AE_READABLE) {
                fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                fired++;
            }

            /* Fire the writable event. */
            if (fe->mask & mask & AE_WRITABLE) {
                if (!fired || fe->wfileProc != fe->rfileProc) {
                    fe->wfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            /* If we have to invert the call, fire the readable event now
             * after the writable one. */
            if (invert && fe->mask & mask & AE_READABLE) {
                if (!fired || fe->wfileProc != fe->rfileProc) {
                    fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            processed++;
        }
    }
    /* Check time events */
    // 最后处理定时器事件   
    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents(eventLoop);

    return processed; /* return the number of processed file/time events */
  }

  int aeWait(int fd, int mask, long long milliseconds) #阻塞某段时间等待某个描述符事件触发   
  void aeMain(aeEventLoop *eventLoop) #主循环函数每次循环都会调用aeProcessEvents(eventLoop, AE_ALL_EVENTS|AE_CALL_AFTER_SLEEP)，从这里可以看到flags=总是等待的需要等待一段时间的    
  char *aeGetApiName(void) #获得底层模型   
  void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep) #设置循环前函数   
  void aeSetAfterSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *aftersleep) #设置循环后函数   
  ```    

- 总结   
  redis的事件循环主要是处理文件事件和定时器事件，这两者redis是如何保证好的进行的呢？我们可以假设当前距离某个最早的定时器事件在3s之后发生，那么epoll模型会阻塞3秒看这3秒有没有事件需要处理，如果有，就直接处理我们知道此时还没有到达定时器事件。所以会处理文件事件。如果在3s内还没有文件事件需要处理，那么也就刚好到最早的定时器事件的时间了，最后就处理定时器事件。    




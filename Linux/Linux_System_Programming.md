# Linux系统编程学习笔记
参考书籍《Linux系统编程》
### open()系统调用
用于打开一个文件并获得一个文件描述符
- int open(const char* name, int flags);
- int open(const char* name, int flags, mode_t mode);

flags的值必须是以下之一：*O_RDONLY*, *O_WRONLY*, *O_RDWR*。flags可以和以下一个或多个值进行按位或运算。*O_APPEND*, *O_ASYNC*, *O_CREAT*, *O_DIRECT*, *O_DIRECTORY*, *O_EXCL*, *O_LARGEFILE*, *O_NOCTTY*, *O_NOFOLLOW*, *O_NONBLOCK*, *O_SYNC*, *O_TRUNC*。

有些需要注意的flags：
- O_ASYNC这个标志位仅用于终端和套接字，不能用于普通文件。当指定文件可写或者可读时产生一个信号(默认SIGIO)。
- O_CREAT这个标志位用于表明在文件不存在时由内核来创建。如果文件已存在，本标志无效，除非给出了O_EXCL标志。
- O_EXCL这个标志位和O_CREAT组合使用，如果指定文件已存在，则open()调用失败。用来防止文件创建时出现竞争。
- O_NOFOLLOW这个标志位说明如果指定文件是一个符号链接，open()调用会失败。如果name是/etc/ship/plank.txt，如果plank.txt是一个符号链接则该调用失败。然而如果etc或者ship是符号链接，只要plank.txt不是，那么调用成功。
- O_NONBLOCK这个标志位用于指明文件将在非阻塞模式下打开。open()调用不会，任何其他操作都不会使该进程在I/O中阻塞。这种情况可能只用于FIFO。

除非创建了新文件，否则mode参数会被忽略；而当O_CREAT给出时则需要。如果使用了O_CREAT却忘记提供mode参数，结果是未定义的。

当文件创建时，mode参数提供新建文件的权限。系统并不在该次打开文件时检查权限，所以你可以进行相反的操作，例如设置文件为只读权限，但却在打开文件后进行写操作。

mode参数有：
- USER:*S_IRWXU*, *S_IRUSR*, *S_IWUSR*, *S_IXUSR*
- GROUP:*S_IRWXG*, *S_IRGRP*, *S_IWGRP*, *S_IXGRP*
- OTHER:*S_IRWXO*, *S_IROTH*, *S_IWOTH*, *S_OXOTH*

实际上，最终写入磁盘的权限位还需让mode参数与用户文件创建的掩码做按位与操作后来确定。若umask为022，mode为0666，那么最终为0644。

### creat()系统调用
O_WRONLY|O_CREAT|O_TRUNC组合经常被使用，以至于专门有个系统调用来实现。
- int creat(const char* name, mode_t mode);

### read()系统调用
- sszie_t read(int fd, void* buf, size_t len);

读取至多len个字节到buf中，成功时返回写入buf中的字节数。出错时返回-1，并设置errno。

<u>ssize_t有符号整数，而size_t则是无符号整数，分别为int和unsigned int。</u>

### write()系统调用
- ssize_t write(int fd, const void* buf, size_t count);

从文件描述符fd引用文件的当前位置开始，将buf中至多count个字节写入文件中。不支持定位的文件(像字符设备)总是从“开头”开始写。

对于普通文件，除非发生一个错误，否则write()将保证写入所有的请求。所以对于普通文件，就不需要进行循环写了。然而对于其他类型——例如套接字大概得有个循环来保证你真的写入了所有请求的字节。

当一个write()调用返回时，内核已将所提供的缓冲区数据复制到了内核缓冲区中，但却没有保证数据已写到目的文件。

**追加模式可以避免多个进程对同一个文件进行写时存在竞争条件。它保证文件位置总是指向文件末尾，这样所有的写操作总是追加的，即便有多个写者。可以认为每个写请求之间的文件位置更新操作都是原子操作。**

### 同步I/O
- int fsync(int fd);

调用fsync()可以保证fd对应文件的脏数据回写到磁盘上。文件描述符fd必须是以写方式打开的。该调用回写数据以及建立的时间戳和inode中的其他属性等元数据。在驱动器确认数据已经全部写入之前不会返回。
- int fdatasync(int fd);

这个系统调用完成的事情和fsync()一样，区别在于它仅写入数据。该调用不保证元数据同步到磁盘上，故此可能快一些。一般来说这就够了。

### sync()系统调用
sync()系统调用可以用来对*磁盘*上的所有缓冲区进行同步，尽管其效率不高，但仍然被广泛使用。
- void sync(void);


sync()真正派上用场的地方是在功能sync的实现中。应用程序则使用fsync()和fdatasync()将文件描述符指定的数据同步到磁盘。

### 直接I/O
在open()中使用O_DIRECT标志会使内核最小化I/O管理的影响。使用该标志时，I/O操作将忽略页缓存机制，直接对用户空间缓冲区和设备进行初始化。所有的I/O将是同步的；操作在完成之前不会返回。在使用直接I/O时，请求长度，缓冲区对齐，和文件偏移必须是设备扇区大小(通常是512字节)的整数倍。

### 关闭文件
- int close(int fd);

close()调用解除了已打开的文件描述符的关联，并分离进程和文件的关联。

当最后一个引用某文件的文件描述符关闭后，在内核中表示该文件的数据结构就被释放了。当它释放时，与文件关联的inode的内存拷贝被清除。如果文件已经从磁盘上解除链接，但在解除前仍保持打开，它在被关闭且inode从内存中移除前就不会真的被删除。因而，对close()的调用可能会使某个已解除链接的文件最终从磁盘上被删除。

### 用lseek()查找
- off_t lseek(int fd, off_t pos, int origin);

origin可以为以下值：
1. SEEK_CUR 当前文件位置fd设置为当前值加上pos，pos可以为负值，零或正值。
2. SEEK_END 当前文件位置fd设置为当前文件长度加上pos，pos可以为负值，零或者正值。
3. SEEK_SET 当前文件位置fd设置为pos。一个为零的pos设置偏移量为文件起始。

管道、FIFO或套接字不能执行查找操作。

<u>若是对文件结尾后第n个字节开始写操作，则会在新旧长度之间建立新的空间，并由零来填充。这种填充方式称为“空洞”。在Unix风格的文件系统上，空洞不占用任何物理上的磁盘空间。这暗示着文件系统上所有文件的大小加起来可以超过磁盘的物理大小。带空洞的文件叫做“稀疏文件”。稀疏文件可以节省可观的空间并提升效率，因为操作那些空洞并不引发任何物理I/O。对空洞部分的读请求将返回相应数量的二进制零。</u>

### 定位读写
- ssize_t pread(int fd, void* buf, size_t count, off_t pos);
- ssize_t pwrite(int fd, const void* buf, size_t count, off_t pos);
**不像read和write，在调用完值，pread和pwrite不会修改文件位置。即fd指向文件中的位置。**

### 截短文件
- int ftruncate(int fd, off_t len);
- int truncate(const char* path, off_t len);
它们可以将文件“截短”到比原长度更长，类似于前面“查找到文件末尾之后”中查找加上写操作的结合。扩展出的字节将全部填充为零。这两个操作均不修改当前文件位置。

### I/O多路复用
Linux提供了三种I/O多路复用方案：select、poll和epoll。
#### select()系统调用
- int select(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
- FD_CLR(int fd, fd_set* set);从指定集合中移除一个文件描述符
- FD_ISSET(int fd, fd_set* set);测试一个文件描述符在不在给定集合中。
- FD_SET(int fd, fd_set* set);向指定集合中添加一个文件描述符
- FD_ZERO(fd_set* set);从指定集合中移除所有文件描述符。在每次使用select()之前，需要调用该宏

- int pselect(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timespec* timeout, const sigset_t* sigmask);

pselect与select的三点不同：
1. pselect的timeout参数使用了timespec结构，而不是timeval结构。timespec使用秒和纳秒，不是秒和毫秒，从理论上来说更精确。实际上，两者在毫秒精度上已经都不可靠了。
2. pselect调用并不修改timeout参数。这个参数在后续调用时也不需要重新初始化。
3. select调用没有sigmask参数。当这个参数被设置为零时，pselect的行为等同于select。
pselect中的sigmask是为了解决信号和等待文件描述符之间的竞争条件。假设一个信号处理程序设置了一个全局标记，进程在每次调用select前都要检查这个标记。现在，假如在检查标记和调用之间接收到信号，应用可能会阻塞，并不再相应该信号。pselect提供了一组可阻塞的信号，可以解决这个问题。阻塞的信号知道解除阻塞才被处理。一旦pselect返回，内核就恢复旧的信号掩码。

#### poll()系统调用
- int poll(struct pollfd* fds, unsigned int nfds, int timeout);
```C
struct pollfd{
  int fd; //文件描述符
  short events; //请求监视的事件
  short revents;  //返回监视到的事件
};
```
每个结构体的events字段是要监视的文件描述符事件的一组位掩码。用户设置这个字段。revents字段则是发生在该文件描述符上的事件的位掩码。内核在返回时设置这个字段。

合法事件：
- POLLIN：有数据可读
- POLLRDNORM：有正常数据可读
- POLLRDBAND：有优先数据可读
- POLLPRI：有高优先级数据可读
- POLLOUT：写操作不会阻塞
- POLLWRNORM：写正常数据不会阻塞
- POLLBAND：写优先数据不会阻塞
- POLLMSG：有一个SIGPOLL消息可用
可能在revents中返回：
- POLLER：给出文件描述符上有错误
- POLLHUP：文件描述符上有挂起事件
- POLLNVAL：给出的文件描述符非法
我们无需在每次调用时重新构建pollfd结构体。

- int ppoll(struct pollfd* fds, nfds_t nfds, const struct timespec* timeout, const sigset_t* sigmask);
ppoll是Linux专有调用。

poll与select：
- poll无需使用者计算最大的文件描述符值加一和传递该参数
- poll在应对较大值的文件描述符时更具效率。比如有50和900两个文件描述符，对select来说是要遍历900个bit
- 使用select，文件描述符集合会在返回时重新创建，这样的话之后每个调用都必须重新初始化它们。poll系统调用分离了输入(events)和输出(revent)，数组无需改变即可重用
- poll由于某些Unix系统不支持poll，所以select可移植性更好

### 标准I/O
实现了用户缓冲的I/O

#### 文件指针
标准I/O例程并不直接操作文件描述符。取而代之的是它们用自己唯一的标志符，即大家熟知的文件指针。文件指针有FILE类型的指针表示。

#### 打开文件
- FILE* fopen(const char* path, const char* mode);

mode可选字符串：
- r：打开文件用来读取。
- r+：打开文件用来读写。
- w：打开文件用来写入，如果文件存在，文件会被清空。如果不存在，则会被创建。
- w+：打开文件用来读写。如果文件存在，文件会被清空。如果不存在，则会被创建。
- a：打开文件用来追加模式的写入。如果文件不存在则会被创建。流被设置在文件的末尾。
- a+：打开文件从来追加模式的读写。如果文件不存在则会被创建。流被设置在文件的末尾。

- FILE* fdopen(int fd, const char* mode);将一个已经打开的文件描述符转成一个流。可以指定模式w和w+，但是它们不会截断文件。

#### 关闭流
- int fclose(FILE* stream);所有被缓冲但还没有被写出的数据会被先写出。
- int fcloseall(void);关闭所有的和当前进程相关联的流，包括标准输入、标准输出、标准错误。

### 从流中读取数据
- int fgetc(FILE* stream);从流中读取下一个字节并把该无符号字符强转为int返回。
- int ungetc(int c, FILE* stream);把c强转成一个无符号字符并放回流中。
- char* fgets(char* str, int size, FILE* stream);从流中读取size-1个字节的数据，并把数据存入str中。当所有字节读入时，**空字符** 被存入字符串末尾。当读到EOF或换行符时读入结束。如果读到了一个换行符，‘\n’被存入str。
- size_t fread(void* buf, size_t size, size_t nr, FILE* stream);从输入流中读取nr个数据，每个数据有size个字节，并将数据放入到buf所指向的缓冲区。读入元素的个数(不是读入字节的个数)被返回。这个函数通过返回一个比nr小的数表明读取失败或文件结束。

### 向流中写数据
对齐：处理器以特定的粒度(例如2,4,8,16字节)来访问内存。因为每个处理的地址空间都从0地址开始，进程必须从一个特定粒度的整数倍开始读取。因此，C变量的存储和访问都要是地址对齐的。访问不对齐的数据在不同的体系结构上有不同成都的性能损失。
- int fputc(int c, FILE* stream);将c表示的字节写入stream指向的流中。
- int fputs(const char* str, FILE* stream);往给定的流中写入一个完整的字符串。
- size_t fwrite(void* buf, size_t size, size_t nr, FILE* stream);把buf指向的nr个元素写入到stream中，每个元素长为size。
- int fseek(FILE* stream, long offset, int whence);whence可以为SEEK_SET、SEEK_CUR和SEEK_END。
- void rewind(FILE* stream);将位置重置到流的初始位置。
- long ftell(FILE* stream);返回当前流的位置。

### 清洗一个流
- int fflush(FILE* stream);stream指向的流中的所有未写入的数据会被清洗到内核中。如果stream为NULL，进程打开的所有流会被清洗掉。

### 错误和文件结束
- int ferror(FILE* stream);如果错误标志被设置，函数返回非零值，否则返回0。
- int feof(FILE* stream);当到达文件的结尾时，EOF标志会被设置。如果标志被设置了，函数返回非零值，否则返回0。
- void clearerr(FILE* stream);为流清空错误和文件结尾标志。

### 获得关联的文件描述符
- int fileno(FILE* stream);返回和流相关联的文件描述符。

### 控制缓冲
- 不缓冲：没有执行用户缓冲，数据直接提交到内核。
- 行缓冲：缓冲以行为单位执行。每当遇到换行符，缓冲区被提交到内核。
- 块缓冲：缓冲以块为单位执行。适用于文件。

- int setvbuf(FILE* stream, char* buf, int mode, size_t size);mode为以下值：
  1. *_IONBF* 无缓冲
  2. *_IOLBF* 行缓冲
  3. *_IOFBF* 块缓冲

### 线程安全
标志I/O函数本质上是线程安全的。在内部实现中，设置了一把锁，一个锁计数器，和为每个打开的流创建的所有者线程。一个线程要想执行任何I/O请求，必须首先获得锁而且称为所有者线程。

#### 手动文件加锁
- void flockfile(FILE* stream);等待流被加锁，然后获得锁，增加锁计数，成为流的所有者线程。
- void funlockfile(FILE* stream);减少与流相关的锁计数。
- int ftrylockfile(FILE* stream);该函数为flockfile的非堵塞版本。

## 高级文件I/O
### 散布/聚集I/O
散布聚集I/O是一种可以在单次系统调用中操作多个缓冲区的I/O方法，可以将单个数据流的内容写到多个缓冲区，或者把单个数据流读到多个缓冲区中。
- sszie_t readv(int fd, const struct iovec* iov, int count);从fd读取count个segment到iov描述的缓冲区中。
- ssize_t writev(int fd, const struct iovec* iov, int count);从iov描述的缓冲区中读取count个segment的数据并写入fd中。

每个iovec结构体描述一个独立的缓冲区，我们称其为段。
```C
struct iovec{
  void* iov_base;
  size_t iov_len;
};
```

### epoll
- int epoll_create(int size);创建一个epoll上下文。size参数告诉内核需要监听的文件描述符数目，但不是最大值。
- int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);可以向指定的epoll上下文中加入或删除文件描述符。

```C
struct epoll_event{
  __ u32 events;
  union{
    void* ptr;
    int fd;
    __ u32 u32;
    __ u64 u64;
  }data;
};
```

epoll_ctl()成功调用将关联epoll实例和epfd。参数op指定对fd要进行的操作。event参数描述epoll更具体的行为。

op有效值：
- EPOLL_CTL_ADD：把fd指定的文件添加到epfd指定的epoll实例监听集中，监听event中定义的事件。
- EPOLL_CTL_DEL：把fd指定的文件从epfd指定的epoll监听集中删除。
- EPOLL_CTL_MOD：使用event改变在已有fd上的监听行为。

epoll_event结构体中的events参数列出了在给定文件描述符上监听的事件。多个事件可以使用位或运算同时指定。以下为有效值：
- EPOLLERR：文件出错。即使没有设置，这个事件也是被监听的。
- EPOLLET：在监听文件上开启边沿触发。默认行为是水平触发。
- EPOLLHUP：文件被挂起。即使没有设置，这个事件也是被监听的。
- EPOLLIN：文件未阻塞，可读。
- EPOLLONESHOT：在一次事件产生并被处理之后，文件不再被监听。必须使用EPOLL_CTL_MOD指定新的事件，以便重新监听文件。
- EPOLLOUT：文件未阻塞，可写。
- EPOLLPRI：高优先级的带外数据可读。

epoll_event中的data字段由用户使用。确认监听事件之后，data会被返回给用户。通常将event.data.fd设定为fd，这样就可以知道哪个文件描述符触发事件。

- int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);等待给定epoll实例关联的文件描述符上的事件。

#### 边沿触发事件和水平触发事件
如果epoll_ctl()的参数event中的events项设置为EPOLLET，fd上的监听称为边沿触发，相反的称为水平触发。区别可用以下情况解释：
1. 生产者向管道写入1kb数据
2. 消费者在管道上调用epoll_wait()，等待pipe出现数据，从而可读。

对于水平触发的监听，在步骤2里对epoll_wait()的调用将立即返回，以表明pipe可读。对于边沿触发的监听，这个调用直到步骤1发生后才会返回。也就是说，即使调用epoll_wait()时管道已经可读，调用仍然会等待直到有数据**再次**写入，之后返回。边沿触发需要一个不同的方式来写程序，通常利用非阻塞I/O，并需要仔细检查EAGAIN。

### 存储映射
- void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);请求内核将fd表示的文件中从offset处开始的len个字节数据映射到内存中。如果包含了addr，表明优先使用addr为内存中的开始地址。访存权限由prot指定，flags指定了其他的操作行为。

prot参数描述了对内存区域所请求的访问权限。如果是PROT_NONE，此时映射区域无法访问，也就是以下标志位的比特位的或运算值：
- PROT_READ 页面可读
- PROT_WRITE  页面可写
- PROT_EXEC 页面可执行

flags参数描述了映射的类型和一些行为。其值为以下值按二进制或运算的值：
- MAP_FIXED 告诉mmap()把addr看做强制性要求，而不是建议。
- MAP_PRIVATE 映射区不共享。文件映射采用了写时拷贝，进程对内存的任何改变不影响真正的文件或者其他进程的映射。
- MAP_SHARED  和所有其他映射该文件的进程共享映射内存。对内存的写操作等效于写文件。读该映射区域会受到其他进程的写操作的影响。

mmap()调用操作页。addr和offset参数都必须按页大小对齐。也就是说，它们必须是页大小的整数倍。

- int munmap(void* addr, size_t len);取消mmap()映射。

mmap()优点：
- 使用read()或write()系统调用需要从用户缓冲区进行数据读写，而使用映射文件进行操作，可以避免多余的数据拷贝。
- 除了潜在的页错误，读写映射文件不会带来系统调用和上下文切换的开销。就像直接操作内存一样简单。
- 当多个进程映射同一个对象到内存中，数据在进程间共享。只读和写共享的映射在全体中都是共享的；私有可写的尚未进行写时拷贝的页是共享的。
- 在映射对象中搜索只需要一般的指针操作。而不必使用lseek()。

mmap()缺陷：
- 映射区域的大小通常是页大小的整数倍。因此，映射文件大小与页大小的整数倍之间有空间浪费。
- 存储映射区域必须在进程地址空间内。对于32位的地址空间，大量的大小各异的映射会导致大量的碎片出现，使得很难找到连续的大片空内存。
- 创建和维护映射以及相关的内核数据结构有一定的开销。通过上节提到的消除读写时的不必要拷贝的，这些开销可以忽略，对于大文件和频繁访问的文件更是如此。

- void* mremap(void* addr, size_t old_size, size_t new_size, unsigned long flags);扩大或减少已有映射的大小。
- int mprotect(const void* addr, size_t len, int prot);改变映射区域的权限。

- int msync(void* addr, size_t len, int flags);可以将mmap()生成的映射在内存中的任何修改回写到磁盘，达到同步内存中的映射和被映射的文件的目的。不调用msync()，无法保证在映射取消前，修改过的映射会被写回到硬盘。这一点与write()不同。

flags参数：
- MS_ASYNC  指定同步操作是异步发生的。更新操作由系统调度，msync()会立即返回，不用等待write()操作完成。
- MS_INVALIDATE 指定该块映射的其他所有拷贝都将失效。未来对该文件任意映射的操作将直接同步到磁盘。
- MS_SYNC 指定同步操作必须同步进行。

### 进程
空闲进程——当没有其他进程在运行时，内核所运行的进程——它的pid是0。在启动后，内核运行的第一个进程称为init进程，它的pid是1。

#### 获得进程ID和父进程ID
- pid_t getpid(void);
- pid_t getppid(void);

#### exec系列系统调用
execl()成功的调用不仅仅改变了地址空间和进程的映像，还改变了进程的一些属性：
- 任何挂起的信号都会丢失
- 捕捉的任何信号会还原为缺省的处理方式，因为信号处理函数以及不存在于地址空间中了
- 任何内存的锁定会丢失
- 多数线程的属性会还原到缺省值
- 多数关于进程的统计信息会复位
- 与进程内存相关的任何数据都会丢失，包括映射的文件
- 包括C语言库的一些特性等独立存在于用户空间的数据都会丢失

exec系列：
- int execl(const char* path, const char* arg, ...);
- int execlp(const char* file, const char* arg, ...);
- int execle(const char* path, const char* arg, ..., char* const envp[]);
- int execv(const char* path, char* const argv[]);
- int execvp(const char* file, char* const argv[]);
- int execve(const char* filename, cahr* const argv[], char* const envp[]);

字母l和v分别代表参数是以列表方式或者数组方式提供的。字母p意味着在用户的PATH环境变量中寻找可执行文件。e表示会提供给新进程以新的环境变量。无论是列表还是数组，最后都是以NULL结尾。

#### fork系统调用
- pid_t fork(void);

注意：
- 子进程的pid是新分配的，与父进程不同
- 子进程的ppid会设置为父进程的pid
- 子进程中的资源统计信息会清零
- 任何挂起的信号会清除，不会被子进程继承
- 任何文件所都不会被子进程所继承

#### vfork系统调用
- pid_t vfork(void);除了子进程必须要立刻执行一次对exec的系统调用，或者调用_exit()退出，对vfork()的成功调用所产生的结果和fork()是一样的。vfork()会挂起父进程直到子进程终止或者运行了一个新的可执行文件的映像。通过这种方式，vfork()避免了地址空间的按页复制。在这个过程中，父进程和子进程共享相同的地址空间和页表项。实际上vfork()只完成了一件事：复制内部的内核数据结构。因此，子进程也就不能修改地址空间中的任何内存。

#### 终止进程
- void exit(int status);对exit()的调用通常会执行一些基本的终止进程的步骤，然后通知内核终止这个进程。

在终止进程之前，C语言函数执行以下关闭进程的工作：
1. 以在系统中注册的逆序来调用有atexit()或on_exit()注册的函数
2. 关闭所有已打开的标准I/O流
3. 删除由tmpfile()创建的所有临时文件

这些步骤完成了在用户空间中所需要做的事情，这样exit()就可以调用_exit()来让内核来处理终止进程的剩余工作了。

#### atexit()
- int atexit(void (* function)(void));用于注册一些在进程结束时要调用的函数。

#### on_exit()
- int on_exit(void (* function)(int, void*), void* arg);工作方式和atexit()一样，只是注册的函数原型不同。

#### 等待终止的子进程
- pid_t wait(int* status);返回已终止子进程的pid，或者返回-1表示出错(没有创建过子进程)。如果status不是NULL，那么它包含了一些关于子进程的附加信息。与该函数相关的宏有：
  - int WIFEXITED(status);
  - int WIFSIGNALED(status);
  - int WIFSTOPPED(status);
  - int WIFCONTINUED(status);
  - int WEXITSTATUS(status);
  - int WTERMSIG(status);
  - int WSTOPSIG(status);
  - int WCOREDUMP(status);

- pid_t waitpid(pid_t pid, int* status, int options);

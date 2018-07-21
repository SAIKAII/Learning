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

#### 实际用户ID、有效用户ID和保存设置的用户ID
实际上，与进程相关的用户ID有四个而不是一个，它们是：实际用户ID、有效用户ID、保存设置的用户ID和文件系统用户ID。

实际用户ID是运行这个进程的那个用户的uid。

有效用户ID是当前进程所使用的用户ID。权限验证一般是使用这个值。初始时，这个ID等于实际用户ID。因为创建进程时，子进程会继承父进程的有效用户ID。更进一步的讲，exec系统调用不会改变有效用户ID。但是在exec调用过程中，实际用户ID和有效用户ID的主要区别出现了：通过setuid(suid)程序，进程可以改变自己的有效用户ID。准确的说，有效用户ID被设置为拥有此程序文件拥有者的用户ID。

保存设置的用户ID是进程原先的有效用户ID。当创建进程时，子进程会从父进程继承保存设置的用户ID。

有效用户ID的作用是：它是在检查进程权限过程中使用的用户ID。实际用户ID和保存设置的用户ID像是代理或者一个潜在的用户ID值，它的作用是允许非root进程在这些用户ID之间互相切换。实际用户ID是真正运行程序的有效用户id。保存设置的用户ID是在执行suid程序前的有效用户id。

### 会话和进程组
每个进程都属于某个进程组。进程组是由一个或多个相互间有关联的进程组成的，它的目的是为了进行作业控制。进程组的主要特征就是信号可以发送给进程组中的所有进程：这个信号可以使同一个进程组中的所有进程终止、停止或者继续运行。每个进程组都由进程组ID唯一标识，该ID就是组长进程的pid。

当有新的用户登陆计算机，登陆进程就会为这个用户创建一个新的会话。这个会话中只有用户的登陆shell一个进程。登陆shell作为会话首进程。会话首进程的pid就被作为会话的ID。一个会话就是一个或多个**进程组**的集合。会话中的进程组分为一个前台进程组和零个或多个后台进程组。

#### 创建会话
- pid_t setsid(void);假如调用进程不是某个进程组组长进程，调用setsid()会创建新的会话。调用进程就是这个会话的唯一进程，也是新会话的首进程，但是它没有控制终端。调用也同时在这个会话中创建一个进程组，调用进程成为了组长进程，也是进程组中的唯一进程。新会话ID和进程组ID被设置为调用进程的pid。

这对守护进程来说十分有用，因为它不想是任何已存在会话的成员，也不想拥有控制终端。

#### 守护进程
守护进程运行在后台，不与任何控制终端相关联。守护进程通常在系统启动时就运行，它们以root用户运行或者其他特殊的用户，并处理一些系统级的任务。

对于守护进程有两个基本要求：它必须是init进程的子进程，并且不与任何控制终端相关联。

进程称为守护进程的步骤：
1. 调用fork()，创建新的进程，它会是将来的守护进程。
2. 在守护进程的父进程中调用exit()。这保证了祖父进程确认父进程已经结束。还保证了父进程不再继续运行，守护进程不是组长进程。最后这一点是顺利完成以下步骤的前提。
3. 调用setsid()，使得守护进程有一个新的进程组和新的会话，两者都把它作为首进程。这也保证它不会与控制终端相关联。
4. 用chdir()将当前工作目录改为根目录。因为前面调用fork()创建了新进程，它所继承来的当前工作目录可能在文件系统中任何地方。而守护进程通常在系统启动时运行，同时不希望一些随机目录保持打开状态，造成阻止管理员卸载守护进程工作目录所在的那个文件系统。
5. 关闭所有的文件描述符。不需要继承任何打开的文件描述符，对于无法确认的文件描述符，让它们继续处于打开状态。
6. 打开0、1和2号文件描述符，把它们重定向到/dev/null。

- int daemon(int nochdir, int noclose);用于简化工作。如果nochdir非零，就不会改变工作目录。如果noclose非零，就不关闭所有打开的文件描述符。

### 进程调度
#### nice()
- int nice(int inc);成功调用nice()将在现有优先级上增加inc，并返回新值。只有拥有CAP_SYS_NICE能力(实际上，就是root所有的进程)才能够使用负值inc，减少友好度，增加优先级。因此，非root进程只能降低优先级(增加inc值)。

#### getpriority()和setpriority()
- int getpriority(int which, int who);
- int setpriority(int which, int who, int prio);

“which”的取值为PRIO_PROCESS、PRIO_PGRP或者PRIO_USER，对应地，“who”就说明了进程ID，进程组ID或者用户ID。当“who”是0的时候，分别是当前进程，当前进程组或者当前用户。

getpriority()返回指定进程中的最高优先级(nice值最小)，setpriority()则将所有进程的优先级都设为“prio”。同nice()一样，只有拥有CAP_SYS_NICE能力的进程能够提高一个进程的优先级(降低nice值)，更进一步地说，只有这样的进程才能调整不属于当前用户的进程的优先级。

### 资源限制
Linux提供了两个操作资源限制的系统调用。

```C
struct rlimit{
  rlim_t rlim_cur;  //soft limit
  rlim_t rlim_max;  //hard limit
};
```
- int getrlimit(int resource, struct rlimit* rlim);
- int setrlimit(int resource, const struct rlimit* rlim);

一般用类似RLIMIT_CPU的整数常量表示资源，rlimit结构表示实际限制。结构定义了两个上上限：软限制和硬限制。内核对进程强制施行软限制，但进程自身可以修改软限制，可以是0到硬限制之间的任意值。不具备CAP_SYS_RESOURCE能力的进程(比如，非root进程)，只能调低硬限制。

15种资源限制：
- RLIMIT_AS 进程地址空间上限，单位是字节。
- RLIMIT_CORE 内存转储文件大小的最大值，单位是字节。
- RLIMIT_CPU  一个进程可以使用的最长CPU时间，单位是秒。
- RLIMIT_DATA 进程数据段和堆的大小，单位是字节。
- RLIMIT_FSIZE  进程可以创建的最大文件，单位是字节。
- RLIMIT_LOCKS  进程可以拥有的文件锁的最大数量。
- RLIMIT_MENLOCK  不具有CAP_SYS_IPC能力的进程通过mlock()，mlockall()或者shmctl()能锁定的最多内存的字节数。
- RLIMIT_MSGQUEUE 用户可以在POXIS消息队列中分配的最多字节。
- RLIMIT_NICE 进程可以降低nice值(提升优先级)的最大值。
- RLIMIT_NOFILE 该值比进程可以打开的最多文件数大一。
- RLIMIT_NPORC  系统任意时刻允许的最多进程数。
- RLIMIT_RSS  进程可以驻留在内存中的最多页数。
- RLIMIT_RTPRIO 没有CAP_SYS_NICE能力的进程可以拥有的最大实时优先级。
- RLIMIT_SIGPENDING 用户消息队列中最多信号数。
- RLIMIT_STACK  栈的最大字节长度。

### 文件及其元数据
每个文件均对应一个inode，它是由文件系统中唯一数值编址。inode存储了与文件有关的元数据，例如文件的访问权限，最后访问时间，所有者，群组，大小以及文件数据的存储位置。

#### 一组stat函数
- int stat(const char* path, struct stat* buf);
- int fstat(int fd, struct stat* buf);
- int lstat(const char* path, struct stat* buf);

```C
struct stat{
  dev_t st_dev; //ID of device containing file
  ino_t st_ino; //inode number
  mode_t st_mode; //permissions
  nlink_t st_nlink; //number of hard links
  uid_t st_uid; //user ID of owner
  gid_t st_gid; //group ID of owner
  dev_t st_rdev;  //device ID
  off_t st_size;  //total size in bytes
  blksize_t st_blksize; //blocksize for filesystem I/O
  blkcnt_t st_blocks; //number of blocks allocated
  time_t st_atime;  //last access time
  time_t st_mtime;  //last modification time
  time_t st_ctime;  //last status change time
};
```

stat()返回由路径path参数指明的文件信息，而fstat()返回由文件描述符fd指向的文件信息。lstat()与stat()类似，但是对于符号链接，lstat()返回链接本身而非目标文件。

#### 权限
- int chmod(const char* path, mode_t mode);
- int fchmod(int fd, mode_t mode);

#### 所有权
- int chown(const char* path, uid_t owner, gid_t group);
- int lchown(const char* path, uid_t owner, gid_t group);
- int fchown(int fd, uid_t owner, gid_t group);

chown()和lchown()设置由路径path指定的文件的所有权。它们作用一样，除非文件是个符号链接：前者沿着符号链接并改变链接目标而不是链接本身的所有权，而lchown()并不沿着符号链接，因此只改变符号链接文件的所有权。

只有具有CAP_CHOWN能力的进程(通常是root进程)可能改变文件的所有者。文件所有者可能将文件所属组设置为任何用户所属组；具有CAP_CHOWN能力的进程能改变文件所属群组为任何值。

#### 扩展属性
扩展属性提供一种永久地把文件与键/值对相关联的机制。

### 目录
在Unix，目录是个简单的概念：它包含文件名的列表，每个文件名对应一个inode编号。每个文件名称为目录项，每个名字到inode的映射称为链接。

#### 获取当前工作目录
- char* getcwd(char* buf, size_t size);成功调用getcwd()会以绝对路径名形式复制当前工作目录至buf指向的长度size字节的缓冲区，并返回一个指向buf的指针。

#### 更改当前工作目录
当用户第一次登入系统时，进程login设置其当前工作目录为在/etc/passwd指定的主目录。
- int chdir(const char* path);
- int fchdir(int fd);

调用chdir()会更改当前工作目录为path指定的路径名，绝对或相对路径均可以。同样，调用fchdir()会更改当前工作目录为文件描述符fd指向的路径名，而fd必须是打开的目录。

#### 创建目录
- int mkdir(const char* path, mode_t mode);创建目录path，其权限位为mode(由当前umask修改)。

#### 移除目录
- int rmdir(const char* path);

#### 读取目录内容
- DIR* opendir(const char* name);创建name所指向目录的目录流。目录流比指向打开目录的文件描述符保存的内容多了一些，主要增加的是一些元数据和保存目录内容的缓冲区。
- int dirfd(DIR* dir);返回目录流dir的文件描述符。由于目录流函数只能在内部使用该文件描述符，程序只能调用那些不操作文件位置的系统调用。
- struct dirent* readdir(DIR* dir);从目录流读取目录项。成功调用会返回dir指向的下个目录项。结构dirent指向目录项。

```C
struct dirent{
  ino_t d_ino;  //inode number
  off_t d_off;  //offset to the next dirent
  unsigned short d_reclen;  //length of this record
  unsigned char d_type; //type of file
  char d_name[256]; //filename
};
```
- int closedir(DIR* dir);关闭由dir指向的目录流，包括目录的文件描述符。

#### 硬链接
- int link(const char* oldpath, const char* newpath);使用link()为存在文件创建新链接。注意：inode是有最大链接数的。包含newpath的设备会建立新目录项。dentry目录项:主要包含文件名字和索引节点号,即inode。他们是一一对应的。

#### 符号链接
符号链接的不同点在于它不增加额外的目录项，而是一种特殊的文件类型。该文件包含被称为符号链接指向的其他文件的路径名。
- int symlink(const char* oldpath, const char* newpath);

#### 解除链接
- int unlink(const char* pathname);从文件系统删除pathname。
- int remove(const char* path);为了简化对各种类型文件的删除，C语言提供该函数。成功调用remove()会从文件系统删除path，并返回0。如果path是个文件remove()调用unlink()；如果path是个目录，remove()调用rmdir()。

#### 复制
复制的步骤：
1. 打开src
2. 打开dst，如果它不存在则创建，且如果存在则长度截断为零
3. 将src数据块读至内存
4. 将该数据块写入dst
5. 继续操作直到src全部已读取且已写入dst
6. 关闭dst
7. 关闭src

如果复制目录，则通过mkdir()创建该目录和所有子目录；其中的每个文件之后单独复制。

#### 移动
- int rename(const char* oldpath, const char* newpath);将路径名oldpath重命名为newpath。文件内容和inode保持不变。

### 设备节点
设备节点是应用程序与设备驱动交互的特殊文件。当应用程序在设备节点上执行一般的Unix I/O，内核以不同于普通文件I/O的方式处理这些请求。内核将该请求转交给设备驱动。设备驱动处理这些I/O操作，并向用户返回结果。设备节点提供设备抽象，使应用程序不必了解特定设备或熟悉特别的接口。

内核如何识别哪些设备驱动该处理哪些请求呢？每个设备节点都具有两个数值属性，分别是主设备号和次设备号。主次设备号与对应的设备驱动映射表已载入内核。

### 带外通信
- int ioctl(int fd, int request, ...);从字面理解就是I/O控制的意思。

**request是特殊请求代码，该值由内核和进程预定义，它指明对文件fd执行何种操作。**

### 监视文件事件
Linux提供为监视文件接口inotify——利用它可以监控文件的移动，读取，写入，或删除操作。假设你正在编写一个类似GNOME's Nautilus的图形化文件管理器。如果文件已复制到目录而Nautilus正在显示目录内容，则该目录在文件管理器中的视图将会不一致。通过inotify，内核能在事件发生时通知(push)应用程序。

#### 初始化inotify
在使用inotify之前，进程必须对它初始化。
- int inotify_init(void);返回初始化实例指向的文件描述符。

#### 监视
进程初始化inotify之后，会设置监视。监视由监视描述符表示，由一个标准Unix路径和一个与之相关联的监视掩码组成。该掩码通知内核，该进程关心何种事件。

inotify可以监视文件和目录。当监视目录时，inotify会报告目录本身和该目录下所有文件(但不包括监视目录子目录下的文件——监视不是递归的)的事件。
- int inotify_add_watch(int fd, const char* path, uint32_t mask);

```C
struct inotify_event{
  int wd; //watch descriptor
  uint32_t mask;  //mask of events
  uint32_t cookie;  //unique cookie
  uint32_t len; //size of 'name' field
  char name[];  //null-ternminated name 为了对齐，这个可能会有null字符填充
};
```

#### 删除inotify监视
- int inotify_rm_watch(int fd, uint32_t wd);

#### 数组分配
- void* calloc(size_t nr, size_t size);与malloc不同的是，calloc将分配的区域全部用0进行初始化。

#### 调整已分配内存大小
- void* realloc(void* ptr, size_t size);它返回一个指向新空间的指针，当试图扩大内存块的时候返回的指针**可能**不再是ptr。如果realloc不能在已有的空间上增加到size大小，那么就会另外申请一块size大小的空间，将原本的数据拷贝到新空间中，然后再将旧的空间释放。

#### 对齐
- int posix_memalign(void** memptr, size_t alignment, size_t size);调用posix_memalign()，成功时返回size字节的动态内存，并保证是按照alignment进行对齐的。参数alignment必须是2的幂，以及void指针大小的倍数。返回的内存块的地址保存在memptr里，函数返回0。

非标准类型对齐规则：
- 一个结构的对齐要求和它的成员中最大的那个类型是一样的。例如，一个结构中最大的是以4字节对齐的32bit的整形，那么这个结构至少以4字节对齐。
- 结构体也引入了对填充的需求，以此来保证每一个成员都符合各自的对齐要求。

#### 匿名内存映射
对于较大的分配，glibc并不使用堆而是创建一个匿名内存映射来满足要求。匿名内存映射和基于文件的映射很相似，只是它并不基于文件-所以称之为“匿名”。实际上，一个匿名内存映射只是一块已经用0初始化的大的内存块，以供用户使用。因为这种映射的存储不是基于堆的，所以并不会在数据段内产生碎片。

好处：
- 无需关系碎片。当程序不再需要这块内存的时候，只要撤销映射，这块内存就直接归还给系统了。
- 匿名存储映射的大小是可调整的，可以设置权限，还能像普通的映射一样接受建议。
- 每个分配存在于独立的内存映射。没有必要再去管理一个全局的堆了。

缺点：
- 每个存储器映射都是页面大小的整数倍。所以，如果大小不是页面整数倍的分配会浪费大量的空间。对于较小的分配来说，空间的浪费更加显著，因为相对于使用的空间，浪费的空间将更大。
- 创建一个新的内存映射比从堆中返回内存的负载要大，因为使用堆几乎不涉及任何内核操作。越小的分配，这样的问题也越明显。

建议：比128KB小的分配由堆实现，相应地，较大的由匿名存储器映射来实现。

#### 使用malloc_usable_size()和malloc_trim()调优
- size_t malloc_usable_size(void* ptr);返回ptr指向的动态内存块的实际大小。动态存储器分配的可用空间可能比请求的大。
- int malloc_trim(size_t padding);强制glibc归还所有的可释放的动态内存给内核。调用成功时，数据段会尽可能地收缩，但是填充字节被保留下来。

#### 基于栈的分配
- void* alloca(size_t size);成功时返回一个指向size字节大小的内存指针。这块内存是在栈中的，当调用它的函数返回时，这块内存将被自动释放。

#### 内存锁定
Linux实现了请求页面调度，页面调度是说在需要时将页面从硬盘交换进来，当不再需要时再交换出去。
- int mlock(const void* addr, size_t len);锁定addr开始长度为len个字节的虚拟内存。成功调用会将所有包含[addr, addr+len)的物理内存**页**锁定。例如，一个调用只是指定了一个字节，包含这个字节的所有物理页都将被锁定。POSIX标准要求addr应该与页边界对齐。Linux并没有强制要求，如果真要这样做的时候，会悄悄的将addr向下调整到最近的页面。对于要求可移植到其他系统的程序需要保证addr是页对齐的。

一个由fork()产生的子进程并不从父进程处继承锁定的内存。然而，由于Linux的写时复制机制，子进程的页面被锁定在内存中直到子进程对它们执行写操作。

#### 锁定全部地址空间
- int mlockall(int flags);锁定一个进程在现有地址空间在物理内存中的所有页面。

flags参数：
- MCL_CURRENT 如果设置了该值，mlockall()会将所有**已被映射**的页面(包括栈，数据段，映射文件)锁定进程地址空间中。
- MCL_FUTURE  如果设置了该值，mlockall()会将所有未来映射的页面也锁定到进程地址空间中。

#### 内存解锁
- int munlock(const void* addr, size_t len);
- int munlockall(void);

#### 确定页面是否在内存中
- int mincore(void* start, size_t length, unsigned char* vec);用来确定一个给定范围内的内存是在物理内存中还是被交换到了硬盘中。

调用mincore()提供了一个向量，表明调用时刻映射中哪个页面是在物理内存中。函数通过vec来返回向量，这个向量描述start(必须页面对齐)开始长为length(不需要对齐)字节的内存中的页面的情况。vec的每个字节对应指定区域内的一个页面，第一个字节对应者第一个页面，然后依次对应。因此，vec必须足够大来装入(length-1+page size)/page size字节。如果那页面在物理内存中，对应字节的最低位是1，否则是0。

目前来说，这个系统调用只能用在以MAP_SHARED创建的基于文件的映射上。

### 基本信号管理
`typedef void (*sighandler_t)(int)`
- sighandler_t signal(int signo, sighandler_t handler);一次成功的调用会移除接收signo信号的当前操作，并以handler指定的新信号处理程序代替它。进程不能捕获SIGKILL和SIGSTOP，因此给这两个信号设置处理程序是没有意义的。

可以用signal()指示内核对当前的进程忽略某个指定信号，或重新设置该信号的默认操作。这可以通过使用特殊的参数值来实现：
- SIG_DFL 将signo所表示的信号设置为默认操作。
- SIG_IGN 忽略signo表示的信号。

signal()函数返回该信号先前的操作，这是一个指向信号处理程序，SIG_DFL或SIG_IGN的指针。

#### 执行与继承
当进程第一次执行时，所有的信号都被设为默认操作，除非父进程忽略它们；在这种情况下，新创建的进程也会忽略那些信号。换句话说，在新进程中任何父进程**捕获**的信号都被重置为默认操作，所有其他的信号保持不变。

### 发送信号
- int kill(pid_t pid, int signo);从一个进程向另一个进程发送信号。

通常的用法是，kill()给pid代表的进程发送信号signo。
- 如果pid是0，signo被发送给调用进程的进程组中的每个进程。
- 如果pid是-1，signo会向每个调用进程有权限发送信号的进程发出信号，调用进程自身和init除外。
- 如果pid小于-1，signo被发送给进程组-pid。

#### 权限
为了给另一个进程发送信号，发送的进程需要合适的权限。有CAP_KILL权限的进程能给任何进程发送信号。如果没有这种权限，发送进程的有效的或真正的用户ID必须等于接受进程的真正的或保存的用户ID。简单的说，用户能够给他或他自己的进程发送信号。

#### 给自己发送信号
- int raise(int signo);一种简单的进程给自己发送信号的方法。

#### 给整个进程组发送信号
- int killpg(int pgrp, int signo);给特定进程组的所有进程发送信号。

### 重入
当内核发送信号时，进程可能执行到代码的任何位置。例如，进程可能正执行一个重要的操作，如果被中断，进程将会处于不一致状态。

可重入函数是指可以安全的调用自身(或者从同一个进程的另一个线程)的函数。为了使函数可重入，函数决不能操作静态数据，必须只操作栈分配的数据或调用中提供的数据，不得调用任何不可重入的函数。

### 信号集
- int sigemptyset(sigset_t* set);
- int sigfillset(sigset_t* set);
- sigaddset(sigset_t* set, int signo);
- int sigdelset(sigset_t* set, int signo);
- int sigismember(const sigset_t* set, int signo);

### 阻塞信号
- int sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

sigprocmask()的行为取决于how的值，它是以下标识之一：
- SIG_SETMASK 调用进程的信号掩码变成set。
- SIG_BLOCK set中的信号被加入到调用进程的信号掩码中。
- SIG_UNBLOCK set中的信号被从调用进程的信号掩码中移除。

如果oldset是非空的，该函数将oldset设置为先前的信号集。

如果set是空的，该函数忽略how，并且不改变信号掩码，但仍然设置oldset的信号掩码。

#### 获取待处理信号
- int sigpending(sigset_t* set);

#### 等待信号集
- int sigsuspend(const sigset_t* set);

一般在获取已到达信号和在程序运行期间阻塞在临界区的信号时使用sigsuspend()。进程首先使用sigprocmask()来阻塞一个信号集，将旧的信号掩码保存在oldset中。退出临界区后，进程会调用sigsuspend()，将oldset赋给set。

### 高级信号管理
- int sigaction(int signo, const struct sigaction* act, struct sigaction* oldact);

```C
struct sigaction{
  void (*sa_handler)(int);  //信号处理程序或操作
  void (*sa_sigaction)(int, siginfo_t*, void*);
  sigset_t sa_mask; //阻塞的信号
  int sa_flags; //标志
  void (*sa_restorer)(void);  //已经过时且不符合POSIX标准
};

typedef struct siginfo_t{
  int si_signo; //信号编号
  int si_errno; //errno值
  int si_code;  //信号代码
  pid_t si_pid; //发送进程的PID
  uid_t si_uid; //发送进程的真实UID
  int si_status;  //退出值或信号
  clock_t si_utime; //用户时间消耗
  clock_t si_stime; //系统时间消耗
  sigval_t si_value;  //信号载荷值
  int si_int; //POSIX.1b信号
  void* si_ptr; //POSIX.1b信号
  void* si_addr;  //造成错误的内存位置
  int si_band;  //带事件
  int si_fd;  //文件描述符
};
```

### 分解时间
```C
struct tm{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
#ifdef _BSD_SOURCE
  long tm_gmtoff;
  const char* tm_zone;
#endif
};
```

### 获取当前时间
- time_t time(time_t* t);返回当前时间，以自从大纪元以来用秒计的流逝的秒数来表示。如果参数t非NULL，该函数也将当前时间写入到提供的指针t中。

### 精度更高的获取当前时间
- int gettimeofday(struct timeval* tv, struct timezone* tz);提供了微秒级精度支持。tz参数已经很古老，一般传NULL给tz参数。

### 一个获取指定时间源的时间的高级接口
- int clock_gettime(clockid_t clock_id, struct timespec* ts);该函数可以达到纳秒级精度。将clock_id指定的时间源的当前时间存储到ts中。

### 取得进程时间
```C
struct tms{
  clock_t tms_utime;  //user time consumed
  clock_t tms_stime;  //system time consumed
  clock_t tms_cutime; //user time consumed by children
  clock_t tms_cstime; //system time consumed by children
};
```
- clock_t times(struct tms* buf);取得正在运行的当前进程及其子进程的进程时间，进程时间以时钟报时信号表示。统计的时间分成用户和系统时间。用户时间是在用户空间执行代码所用的时间。系统时间是在内核空间执行所用的时间(例如进行系统调用或者发送一个页错误所消耗的时间)。每个子进程的耗时统计只在该子进程已经终结，且父进程对其调用了waitpid()之后才被包含进来。

### 设置当前时间
- int stime(time_t* t);调用需要发起者用于CAP_SYS_TIME权限。一般的，只有root用户才有该权限。

### 高精度定时
- int settimeofday(const struct timeval* tv, const struct timezone* tz);让tz传递NULL是不错的选择。

### 设置时间的一个高级接口
- int clock_settime(clockid_t clock_id, const struct timespec* ts);在大多数系统上，唯一可以设置的时间源是CLOCK_REALTIME。因此，这个函数比settimeofday()唯一优越之处在于提供了纳秒级精度。

### 时间格式转换
- char* asctime(const struct tm* tm);
- char* asctime_r(const struct tm* tm, char* buf);

返回一个指向静态分配的字符串的指针。之后对任何时间函数的调用都可能覆盖该字符串，asctime()不是线程安全的。asctime_t()才是线程安全，buf最少应有26个字符长度。

- time_t mktime(struct tm* tm);转换tm结构体为time_t。

- char* ctime(const time_t* timep);
- char* ctime_r(const time_t* timep, char* buf);将一个time_t转换为ASCII表示。

- struct tm* gmtime(const time_t* timep);
- struct tm* gmtime_r(const time_t* timep, struct tm* 0result);将time_t转换到tm结构体。

- struct tm* localtime(const time_t* timep);
- struct tm* localtime_r(const time_t* timep, struct tm* result);将给出的time_t表示为用户时区。

- double difftime(time_t time1, time_t time0);返回两个time_t值的差值，并转换到双精度浮点类型来表示相差的秒数。

### 调校时间
- int adjtime(const struct timeval* delta, struct timeval* olddelta);指示内核使用增量delta来逐渐调整时间，然后返回0。如果delta指定的时间是正值，内核将加速系统时钟直到修正彻底完成。如果delta指定时间是负值，内核将减缓系统时钟直到修正完成。

- int adjtimex(struct timex* adj);比adjtime)()采用的渐进调整方法更加强大和复杂的时钟调整算法。调用adjtimex()可以将内核中与时间相关的参数读取到adj指向的timex结构体中。系统调用可以选择性的根据该结构体的modes字段来额外设置某些参数。

```C
struct timex{
  int modes;  //mode selector
  long offset;  //time offset(usec)
  long freq;  //frequency offset(scaled ppm)
  long maxerror;  //maximum error(usec)
  long esterror;  //estimated error(usec)
  int status; //clock status
  constant; //PLL time constant
  long precision; //clock precision(usec)
  long tolerance; //clock frequency tolerance
  struct timeval time;  //current time
  long tick;  //usecs between clock ticks
};
```

modes字段由零或多个以下标志位按位或的结果：
ADJ_OFFSET  通过offset设置时间偏移量
ADJ_FREQUENCY 通过freq设置频率偏移量
ADJ_MAXERROR  通过maxerror设置最大错误值
ADJ_ESTERROR  通过esterror设置估计错误值
ADJ_STATUS  通过status设置时钟状态
ADJ_TIMECONST 通过constant设置锁相环时间常量
ADJ_TICK  通过tick设置时钟计时信号量
ADJ_OFFSET_SINGLESHOT 使用简单算法通过offset设置一次时间偏移量

如果modes是0，就没有设置值。只有拥有CAP_SYS_TIME权限的用户才能给modes赋非零值；任何用户均可设置mode为0，从而取得所有参数，但不能设置任何值。

### 睡眠和等待
- unsigned int sleep(unsigned int seconds);该调用返回未睡眠的秒数。
- void usleep(unsigned long usec);微秒级精度睡眠
- int nanosleep(const struct timespec* req, struct timespec* rem);纳秒级精度的睡眠。如果睡眠被打断，且rem不是NULL，函数把剩余睡眠时间放到rem中。

### 最高级睡眠函数
- int clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* req, struct timespec* rem);

### 定时器
定时器提供了在一定时间获取后通知进程的机制。定时器超时所需的时间叫做延迟(delay)，或者超时值(expiration)。

#### 简单闹钟alarm
- unsigned int alarm(unsigned int seconds);调用会在真实时间seconds秒之后将SIGALRM信号发给调用进程。如果先前的信号尚未处理，调用就取消该信号，并用新的来代替它，并返回先前的剩余秒数。如果seconds是0，就取消掉之前的信号，但不设置新的闹钟。

#### 间歇定时器
- int getitimer(int which, struct itimerval* value);
- int setitimer(int which, const struct itimerval* value, struct itimerval* ovalue);

间歇定时器和alarm()的操作方式相似，但它能够自动重启自身，并在以下三个独有的模式中之一下工作：
- ITIMER_REAL 测量真实时间。当指定的真实时间过去后，内核将SIGALRM信号发给进程。
- ITIMER_VIRTUAL  只在进程用户空间的代码执行时减少。当指定的进程时间过去后，内核将SIGVTALRM发给进程。
- ITIMER_PROF 在进程执行以及内核为进程服务时(例如完成一个系统调用)都会减少。当指定的时间过去后，内核将SIGPROF信号发给进程。这个模式一般和ITIMER_VIRTUAL共用，这样程序就能衡量进程消耗的用户时间和内核时间。
- ITIMER_REAL 衡量的时间和alarm()相同；另外两个模式对于剖析程序很有帮助。

itimeval结构体允许用户在定时器过期或终止的时限，如果设定了值，则在过期后重启定时器：

```C
struct itimerval{
  struct timeval it_interval; //next value
  struct timeval it_value;  //current value
};
```

#### 高级定时器
- int timer_create(clockid_t clockid, struct sigevent* evp, timer_t* timerid);创建一个与POSIX时钟clockid相关联的定时器，在timerid中存储一个唯一的定时器标记，并返回0。

```C
struct sigevent{
  union sigval sigev_value;
  int sigev_signo;
  int sigev_notify;
  void (*sigev_notify_function)(union sigval);
  pthread_attr_t* sigev_notify_attributes;
};
union sigval{
  int sival_int;
  void* sival_ptr;
};
```

进程在定时器过期时的行为通过sigev_notify来指定，必须是以下三个值之一：
- SIGEV_NONE  一个“空的”通知。
- SIGEV_SIGNAL  当定时器到期时，内核给进程发送一个由sigev_signo指定的信号。在信号处理程序中，si_value被设置为sigev_value。
- SIGEV_THREAD  当定时器到期时，内核产生一个新线程，并让其执行sigev_notify_function，将sigev_value作为它唯一的参数。该线程在这个函数返回时终止。如果sigev_notify_attributes不是NULL，pthread_attr_t结构体则定义了新线程的行为。

#### 设置定时器
- int timer_settime(time_t timerid, int flags, const struct itimerspec* value, struct itimerspec* ovalue);成功调用将设置timerid指定的定时器的过期时间为value。

#### 取得定时器的过期时间
- int timer_gettime(timer_t timerid, struct itimerspec* value);

#### 取得定时器的超时值
- int timer_getoverrun(timer_t timerid);返回定时器过期与实际定时器过期后通知进程间的多余时长。

#### 删除定时器
- int timer_delete(timer_t timerid);

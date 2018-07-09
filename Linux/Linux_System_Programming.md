# Linux系统编程学习笔记
参考书籍《Linux系统编程》
#### open()系统调用
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

#### creat()系统调用
O_WRONLY|O_CREAT|O_TRUNC组合经常被使用，以至于专门有个系统调用来实现。
- int creat(const char* name, mode_t mode);

#### read()系统调用
- sszie_t read(int fd, void* buf, size_t len);

读取至多len个字节到buf中，成功时返回写入buf中的字节数。出错时返回-1，并设置errno。

<u>ssize_t有符号整数，而size_t则是无符号整数，分别为int和unsigned int。</u>

#### write()系统调用
- ssize_t write(int fd, const void* buf, size_t count);

从文件描述符fd引用文件的当前位置开始，将buf中至多count个字节写入文件中。不支持定位的文件(像字符设备)总是从“开头”开始写。

对于普通文件，除非发生一个错误，否则write()将保证写入所有的请求。所以对于普通文件，就不需要进行循环写了。然而对于其他类型——例如套接字大概得有个循环来保证你真的写入了所有请求的字节。

当一个write()调用返回时，内核已将所提供的缓冲区数据复制到了内核缓冲区中，但却没有保证数据已写到目的文件。

**追加模式可以避免多个进程对同一个文件进行写时存在竞争条件。它保证文件位置总是指向文件末尾，这样所有的写操作总是追加的，即便有多个写者。可以认为每个写请求之间的文件位置更新操作都是原子操作。**

#### 同步I/O
- int fsync(int fd);

调用fsync()可以保证fd对应文件的脏数据回写到磁盘上。文件描述符fd必须是以写方式打开的。该调用回写数据以及建立的时间戳和inode中的其他属性等元数据。在驱动器确认数据已经全部写入之前不会返回。
- int fdatasync(int fd);

这个系统调用完成的事情和fsync()一样，区别在于它仅写入数据。该调用不保证元数据同步到磁盘上，故此可能快一些。一般来说这就够了。

#### sync()系统调用
sync()系统调用可以用来对*磁盘*上的所有缓冲区进行同步，尽管其效率不高，但仍然被广泛使用。
- void sync(void);


sync()真正派上用场的地方是在功能sync的实现中。应用程序则使用fsync()和fdatasync()将文件描述符指定的数据同步到磁盘。

#### 直接I/O
在open()中使用O_DIRECT标志会使内核最小化I/O管理的影响。使用该标志时，I/O操作将忽略页缓存机制，直接对用户空间缓冲区和设备进行初始化。所有的I/O将是同步的；操作在完成之前不会返回。在使用直接I/O时，请求长度，缓冲区对齐，和文件偏移必须是设备扇区大小(通常是512字节)的整数倍。

#### 关闭文件
- int close(int fd);

close()调用解除了已打开的文件描述符的关联，并分离进程和文件的关联。

当最后一个引用某文件的文件描述符关闭后，在内核中表示该文件的数据结构就被释放了。当它释放时，与文件关联的inode的内存拷贝被清除。如果文件已经从磁盘上解除链接，但在解除前仍保持打开，它在被关闭且inode从内存中移除前就不会真的被删除。因而，对close()的调用可能会使某个已解除链接的文件最终从磁盘上被删除。

#### 用lseek()查找
- off_t lseek(int fd, off_t pos, int origin);

origin可以为以下值：
1. SEEK_CUR 当前文件位置fd设置为当前值加上pos，pos可以为负值，零或正值。
2. SEEK_END 当前文件位置fd设置为当前文件长度加上pos，pos可以为负值，零或者正值。
3. SEEK_SET 当前文件位置fd设置为pos。一个为零的pos设置偏移量为文件起始。

<u>若是对文件结尾后第n个字节开始写操作，则会在新旧长度之间建立新的空间，并由零来填充。这种填充方式称为“空洞”。在Unix风格的文件系统上，空洞不占用任何物理上的磁盘空间。这暗示着文件系统上所有文件的大小加起来可以超过磁盘的物理大小。带空洞的文件叫做“稀疏文件”。稀疏文件可以节省可观的空间并提升效率，因为操作那些空洞并不引发任何物理I/O。对空洞部分的读请求将返回相应数量的二进制零。</u>

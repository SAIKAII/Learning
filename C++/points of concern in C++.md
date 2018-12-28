- 若某个派生类私有继承一个基类，那么该基类的指针(引用)无法指向该派生类的实例。
  ```C++
  class Person{...}
  class Student:private Person{...}
  void eat(const Person &p);
  Person p;
  Student s;
  eat(p); //正确可行
  eat(s); //错误
  ```
  如果类之间的继承关系是private，编译器不会自动将一个派生类对象转换为一个基类对象。

- virtual继承会增加大小、速度、初始化(赋值)复杂度等等成本。如果virtual base classes不带任何数据(Java中的接口类-interface)，将是最具有实用价值的情况。

- 当编译器开始解析template print2nd时，尚未确认C是什么东西。C++有个规则可以解析这一歧义状态：如果解析器在template中遭遇一个嵌套从属名称(C::const_iterator)，它便假设这名称不是个类型，除非你告诉它是。所以缺省情况下嵌套从属名称不是类型，而是被认为是C中的一个static成员变量。
  ```C++
  template<typename C>
  void print2nd(const C& container){
    if(container.size() >= 2){
      C::const_iterator iter(container.begin());  //错误，无法通过编译
      //typename C::const_iterator iter(container.begin())这样才是被认为是类型
    }
  }
  ```
  typename只被用来验明嵌套从属类型名称；其他名称不该有它存在。
  ```C++
  template<typename C>
  void f(const C& container,  //不允许使用"typename"
        typename C:: iterator iter);  //一定要有"typename"
  ```
  typename不可以出现在继承列表内的嵌套从属类型名称之前，也不可以在成员初始化列表中作为基类修饰符。
  ```C++
  template<typename T>
  class Derived: public Base<T>::Nested{
  public:
    explicit Derived(int x):Base<T>::Nested(x){
      typename Base<T>::Nested temp;
      ...
    }
  };
  ```

- 内联操作发送与预处理(预编译)阶段，即处理带“#”前缀的语句那个阶段。

- 带有继承关系的B，D两类型分别具现化某个模板，产生出来的两个具现体并不带有继承关系。如下：
  Top <- Middle <- Bottom
  ```C++
  template<typename T>
  class SmartPtr{
  public:
    explicit SmartPtr(T* realPtr);
    ...
  };
  SmartPtr<Top> pt1 = SmartPtr<Middle>(new Middle); //将SmartPtr<Middle>
                                                    //转换为SmartPtr<Top>
  SmartPtr<Top> pt2 = SmartPtr<Bottom>(new Bottom); //同上
  SmartPtr<const Top> pct2 = pt1;                   //非常量转换为常量
  ```
  以上都是不行的，如果把以上的模拟模板智能指针改成普通的指针倒是完全没问题。上面都是具现化SmartPtr模板，所以`SmartPtr<Top>`和`SmartPtr<Middle>`之间并没有什么继承关系，只是两个不相干的类而已，这就造成了看似派生类的对象却无法转换成基类对象的情况。

- 在template实参推导过程中从不将隐式类型转换函数(non-explicit的构造函数)纳入考虑。

- C++要求所有operator news(动态申请空间)返回的指针都有适当的对齐，如指针的地址必须是4的倍数或double的地址必须是8的倍数。如果没有奉行这个约束条件，可能导致运行期硬件异常。而在Intel x86体系结构上，若double是8字节对齐，其访问速度会快很多。但是编译器自带的operator new并不保证对动态分配而得的double采取8字节对齐。

- `Base *b = new Base`这段代码的执行步骤如下：
  1. new申请空间
  2. 调用Base默认构造函数
  3. 把地址赋予b对象指针

- free和delete函数仅仅把指针指向的内存释放掉了，然而指针还保存了该内存的地址。所以在释放掉之后，除非确定之后不会再去对该指针做任何操作，否则要记得把指针置NULL，防止后面操作的误判断。

- 目前所有编译器对于virtual function的实现法都是使用各个class专属的virtual table，大小固定，并且在程序执行前就构造好了。

- member functions虽然含在class的声明之内，却不出现在object之中。每一个non-inline member function只会诞生一个函数实体。至于每一个“拥有零个或一个定义”的inline function则会在其每一个使用者(模块)身上产生一个函数实体。

- C++对象模型：Nonstatic data members被配置于每一个class object之内，static data members则被存放在所有的class object之外。Static和nonstatic function members也被放在所有的class object之外。virtual functions则以两个步骤支持：
  1. 每一个class产生出一堆指向virtual functions的指针，放在表格之中。这个表格被称为virtual table(vtbl)。
  2. 每一个class object被添加了一个指针，指向相关的virtual table。通常这个指针被称为vptr。vptr的设定(setting)和重置(resetting)都由每一个class的constructor、destructor和copy assignment运算符自动完成。每一个class所关联的type_info object(用以支持runtime type identification, RTTI)也经由virtual table被指出来，通常是放在表格的第一个slot处。

- 什么情况下编译器会合成“有用的默认构造函数”：
  1. “带有Default Constructor”的Member Class Object的类，即类A与另一个带有Default Constructor的类组合，那么类A如果没有其他构造函数，编译器就会合成默认构造函数，在该函数中调用另一个类的默认构造函数。如果类A有构造函数，那么“调用代码”会被安插到已有的所有构造函数最开头处。
  2. “带有Default Constructor”的Base Class的类，同上。
  3. “带有一个Virtual Function”的Class的类，若该类没有构造函数，编译器会合成默认构造函数，不但执行上面**1**的操作，还会加上初始化一个隐式的vptr的操作，该vptr是指向vtbl的。
  4. “带有一个Virtual Base Class”的Class的类。<br/><br/>
  至于没有存在那四种情况而又没有声明任何constructor的classes，我们说它们拥有的是implicit trivial default constructors(隐式无用默认构造函数)，它们实际上并不会被合成出来。

- 构造函数的成员初始化列表
  - 必须使用成员初始化列表的情况：
    1. 当初始化一个reference member时
    2. 当初始化一个const member时
    3. 当调用一个base class的constructor，而它拥有一组参数时
    4. 当调用一个member class的constructor，而它拥有一组参数时

- 即使只是一个空的class(没有成员变量)，编译器也会为其插入一个char(1 byte)，为的是在内存中占到一个独一无二的地址。
  ```C++
  class X{};
  class Y : public virtual X{};
  class Z : public virtual X{};
  class A : public Y, public Z{};
  ```
这三个的通过sizeof得出的结果分别为1、8、8、16。造成Y、Z、A这样的大小，其中有地址对齐和特定编译器的原因。

- static data members被放置在程序的一个global data segment中，不会影响个别的class object的大小。

- nonstatic member function至少和一般的nonmember function有相同的效率。因为编译器内部已将"member函数实体"转换为对等的"nonmember函数实体"。

- virtual function table会包含的函数地址：
  - 这个class所定义的虚拟函数。它会改写(overriding)一个可能存在的base class virtual function函数实体
  - 继承自base class的函数实体。这时在derived class决定不改写virtual function时才会出现的情况
  - 一个pure_virtual_called()函数实体，它既可以扮演pure virtual function的空间保卫者角色，也可以当做执行期异常处理函数

- 单一继承下，每一个拥有virtual function的class都会有它自个的virtual table，但是并不占用class object的大小。该class的每一个object都会有一个vptr。

- 在多重继承之下，一个derived class内含n-1个**额外**的virtual tables，n表示其上一层base classes的数目(因此，单一继承将不会有额外的virtual tables)。针对每一个virtual tables，Derived对象中有对应的vptr。<br/>
有一点细节要注意，对于像`class Derived : public Base1, public Base2{...}`这样的继承，Derived会有两个vptr分别指向不同的vtbl，对于Base1的vptr，其指向的vtbl，内含的virtual function可不止Base1的，其中还有Base2的；而Base2的vptr则只有Base2的。<br/>
当然，以上只是其中一种实现方法，不同编译器可能就不同。比如有一种是把所有vtbl都连成一块，然后Base2的vptr也只是一个offset而已，这是为了加速动态共享函数库的符号名称链接。

- Constructor可能内带大量的隐藏码，因为编译器会扩充每一个constructor，扩充程度视class T的继承体系而定。一般而言编译器所做的扩充操作大约如下：
  1. 记录在member initialization list中的data members初始化操作会被放进constructor的函数本身，并以members的声明顺序为顺序。
  2. 如果有一个member并没有出现在member initialization list之中，但它有一个default constructor，那么该default constructor必须被调用。
  3. 在那之前，如果class object有virtual table pointers(s)，它(们)必须被设定初值，指向适当的virtual table(s)。
  4. 在那之前，所有上一层的base class constructors必须被调用，以base class的声明顺序为顺序(如果base class是多重继承下的第二或后继的base class，那么this指针必须有所调整)。
  5. 在那之前，所有virtual base class constructors必须被调用，从左到右，从最深到最浅。

- Destructor扩展顺序：
  1. destructor的函数本身首先被执行
  2. 如果class拥有member class objects，而后者拥有destructors，那么它们会以其声明顺序的相反顺序被调用
  3. 如果object内带一个vptr，则现在被重新设定，指向适当的base class的virtual table
  4. 如果有任何直接的(上一层)nonvirtual base classes拥有destructor，它们会以其声明顺序的相反顺序被调用
  5. 如果有任何virtual base classes拥有destructor，而当前讨论的这个class是最尾端(most-derived)的class，那么它们会以其原来的构造顺序的相反顺序被调用

- 第一种情况是创建一个临时对象并返回它（直接在外部返回值的存储单元中）。

  第二种则是发生三件事。首先创建tmp对象，其中包括构造函数的调用。然后拷贝构造函数把tmp拷贝到外部返回值的存储单元中。最后，当tmp在作用域的结尾时调用析构函数。
  ```C++
  return Integer(5);
  // 虽然最后的结果一样，但实际上面与下面是不同的
  Integer tmp(5);
  return tmp;
  ```

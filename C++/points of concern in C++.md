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

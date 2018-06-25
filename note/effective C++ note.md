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
  }
  ```

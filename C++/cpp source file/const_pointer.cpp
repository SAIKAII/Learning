int main(){
  int num_1 = 1;
  int num_2 = 2;
  const int *const_value = &num_1;
  const_value = &num_2;
  //*const_value = 4;   指针指向的对象的值不能更改
  int* const const_pointer = &num_1;
  //const_pointer = &num_2; 指针指向的地址不能更改
  *const_pointer = 4;
  const int* const const_value_const_pointer = &num_1;
  //const_value_const_pointer = &num_2; 指针指向的地址不能更改
  //*const_value_const_pointer = 4; 指针指向的对象的值不能更改
  return 0;
}

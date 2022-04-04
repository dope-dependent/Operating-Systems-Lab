#include"memlab.h"
#define MAX_ARR_SIZE 100



void A(_int x, _bool y){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _int arr = createArr("_int",size);
 
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%INT_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return;
} 
 
void B(_int x, _char y){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _char arr = createArr("_char",size);
 
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%CHAR_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
}
 
void C(_char x, _bool y){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _medium_int arr = createArr("_medium_int",size);
 
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%MEDIUM_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
} 
 
void D(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _bool arr = createArr("_bool",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%2,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
}
 
void E(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _int arr = createArr("_int",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr, rand()%INT_MAX, i);
 
 struct myType ret = {0};
 // printf("yahan\n");
 returnFromFunc(ret);
 return ;
} 
 
void F(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _bool arr = createArr("_bool",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%2,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
}
 
void G(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _char arr = createArr("_char",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%CHAR_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
} 
 
void H(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _int arr = createArr("_int",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%INT_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
}
 
void I(_bool y, _char x){
 initFunction();
 
 int size = rand()%MAX_ARR_SIZE+1;
 _int arr = createArr("_int",size);
 printf("counter of array %lld\n",arr.counter);
 
 for(int i=0;i<size;i++)
 assignArrConst(arr,rand()%INT_MAX,i);
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
} 
 
void J(_bool y, _char x){
 initFunction();
 
 struct myType ret = {0};
 returnFromFunc(ret);
 return ;
}
 
int main(){
 createMem(2500000);
 initFunction();
 _int v = createVar("_int");
 _bool x = createVar("_bool");
 _char z = createVar("_char");
 _medium_int p = createVar("_medium_int");
 A(v,x);
 B(v,z);
 C(z,x);
 D(x,z);
 E(x,z);
 F(x,z);
 G(x,z);
 H(x,z);
 I(x,z);
 J(x,z);
}
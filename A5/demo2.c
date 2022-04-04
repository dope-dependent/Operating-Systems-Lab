#include "memlab.h"

void populateFibonacci(_int arr, int k){

    initFunction();
    assignArrConst(arr,0,0);
    if(k>=2) assignArrConst(arr,1,1);

    for(int i=2;i<k;i++)
    {
        int x,y;
        x=getArrayEntry(arr, i-1);
        y=getArrayEntry(arr, i-2);
        printf("values at iteration %d are %d %d\n",i, y,x);
        assignArrConst(arr,x+y,i);
    }

    struct myType ret={0};

    returnFromFunc(ret);

    return;
}

_int fibonacciProduct(int k){
    //create an integer array

    initFunction();
    _int arr = createArr("_int",k);

    _int ans = createVar("_int");
    assignVarConst(ans,1);

    populateFibonacci(arr,k);

    for(int i=2;i<k;i++)
    {
        int num = getArrayEntry(arr, i);
        printf("number %d in the array is %d\n",i,num);
        int _ans = getVar(ans);
        printf("number %d returned by getvar\n",_ans);
        assignVarConst(ans, num*_ans);
    }

    return returnFromFunc(ans);
}


int main(){
    createMem(2500000);
    initFunction();
    int num;
    printf("Enter k: ");
    scanf("%d",&num);

    _int prod = fibonacciProduct(num);
    printVar(prod);

    // pthread_kill(gcID, SIGABRT);    
}
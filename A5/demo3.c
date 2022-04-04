#include "memlab.h"

int main(){
    createMem(25000);
    initFunction();

    _int x = createVar("_int");
    _char y = createVar("_char");
    _medium_int z = createVar("_medium_int");

        // assignVarConst(y,'b');
    sleep(1);    
    sleep(4);
    _int cv = createArr("_int",256);
    _char le = createVar("_char");
    assignVarConst(le, 'x');
    printVar(le);
    _int xz = createVar("_int");
    assignVarConst(xz, 1348924);
    printVar(xz);
    _medium_int ym = createVar("_medium_int");
    sleep(4);
    freeElem(cv);
    sleep(3);
    printVar(le);
    printVar(xz);
    // assignArrConst(cv,'b',255);
    struct myType null = {0};
    // returnFromFunc(x);
}
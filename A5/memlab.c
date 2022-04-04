#include "memlab.h"

void reset () {
    printf("\033[0m");
}

freeMem* sortedFreeMerge(freeMem* a, freeMem* b)
{
    freeMem* result = NULL;
    
    /* Base cases */
    if (a == NULL) return(b);
    else if (b==NULL) return(a);
    
    /* Pick either a or b, and recur */
    if (a->offset <= b->offset)
    {
        result = a;
        result->prev = sortedFreeMerge(a->prev, b);
    }
    else
    {
        result = b;
        result->prev = sortedFreeMerge(a, b->prev);
    }
    return(result);
}
 
freeMem* sortFreeMem(freeMem* A) {
    if (!A || !A->prev) return A;
    
    freeMem *head, *a, *b, *slow, *fast, *ret;
    head = A;
    
    slow = head;
    fast = head->prev;
    while (fast) {
        fast = fast->prev;
        if (fast) {
            slow = slow->prev;
            fast = fast->prev;
        }
    }
    a = head;
    b = slow->prev;
    slow->prev = NULL;
    
    a = sortFreeMem(a);
    b = sortFreeMem(b);
    
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    return sortedFreeMerge(a,b);
}

void sortFreeList(){
    *freeHead = sortFreeMem(*freeHead);
    freeMem *curr = *freeHead;
    while(curr){
        // printf("%d->",curr->offset);
        curr = curr->prev;
    }    

    // printf("\n\n");
}

usedMem* sortedUsedMerge(usedMem* a, usedMem* b)
{
    usedMem* result = NULL;
    /* Base cases */
    if (a == NULL)
        return(b);
    else if (b == NULL)
        return(a);
    
    /* Pick either a or b, and recur */
    if (a->offset >= b->offset)
    {
        result = a;
        result->prev = sortedUsedMerge(a->prev, b);
    }
    else
    {
        result = b;
        result->prev = sortedUsedMerge(a, b->prev);
    }
    return (result);
}
 
usedMem* sortUsedMem(usedMem* A) {
    if (!A || !A->prev) return A;
    
    usedMem *head, *a, *b, *slow, *fast, *ret;
    head = A;
    
    slow = head;
    fast = head->prev;
    while (fast) {
        fast = fast->prev;
        if (fast) {
            slow = slow->prev;
            fast = fast->prev;
        }
    }
    a = head;
    b = slow->prev;
    slow->prev = NULL;
    
    a = sortUsedMem(a);
    b = sortUsedMem(b);
    
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    return sortedUsedMerge(a,b);
}

void sortUsedList(){
    *usedHead = sortUsedMem(*usedHead);
    usedMem *curr = *usedHead;
    usedMem *prev = NULL;
    while (curr) {
        // printf("%d->", curr->offset);
        curr->next = prev;
        prev = curr;
        curr = curr->prev;
    }
    
    // printf("\n\n");
    curr = *usedHead;
    while (curr != NULL && curr->prev != NULL) {
        curr = curr->prev;
    }
    while (curr != NULL){
        // printf("%d->", curr->offset);
        curr = curr->next;
    } 

    // printf("\n\n");
}

void compact(){
    sortFreeList(*freeHead);
    sortUsedList(*usedHead);

    printf(BLUE"gc_run: "YELLOW"Garbage compaction started\n");
    struct timeval start, end; 
    gettimeofday(&start, NULL);
     pthread_mutex_unlock(&lock);

    struct freeMem *fm = *freeHead;
    struct usedMem *um = *usedHead;
    
    while (fm && um) {
        if (fm->offset <= um->offset) {
            memcpy(userMem + fm->offset, userMem + um->offset, PAGE_SIZE * 4);
            int temp = fm->offset;
            fm->offset = um->offset;
            um->offset = temp;
        }
        else break;
    }

    fm = *freeHead;
    while (fm) {
        // printf("%d->", fm->offset);
        fm = fm->prev;
    }
    // printf("\n\n");

    um = *usedHead;
    while (um) {
        // printf("%d->", um->offset);
        um = um->prev;
    }
    gettimeofday(&end, NULL);
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf(BLUE"compact: "YELLOW"Garbage compaction completed in %lds %ldus\n", seconds, micros);

}

void gc_run(){

    printf(BLUE"gc_run: "YELLOW"Garbage collection started\n");
    
    struct timeval start, end; 
    gettimeofday(&start, NULL);

    pageNode* pagex = *pageStackHead;

    while(pagex){
        pagex->markBit=false;
        pagex=pagex->prev;
    }

    for(int i=0;i<HASH;i++)
    {
        STNode* iter=symTable->table[i];
        while(iter){
            if(!iter->isArray)
                iter->pageIndex->markBit=true;
            else{
                arrayBook *bookMark=iter->book;
                while(bookMark){
                    bookMark->pageIndex->markBit=true;
                    bookMark=bookMark->next;
                }
            }
            iter=iter->next;
        }
    }

    pagex= *pageStackHead;

    while(pagex){
        if(!pagex->markBit){
            pageQueueCount++;
            pageQueue[*pageQueueHead]=pagex->index;
            (*pageQueueHead)++;
            
            if(pagex==*pageStackHead)
                *pageStackHead=(*pageStackHead)->prev;
            
            else{
                if(pagex->prev)
                pagex->prev->next=pagex->next;
                if(pagex->next)
                pagex->next->prev=pagex->prev;
            }

            int offset = pagex->pointer->offset;

            usedQueueCount++;
            usedQueue[*usedQueueHead]=pagex->pointer->index;
            (*usedQueueHead)++;

            usedMem *usedx=pagex->pointer;

            if(usedx==*usedHead)
                *usedHead=(*usedHead)->prev;
            else{
                if(usedx->prev)
                    usedx->prev->next=usedx->next;
                if(usedx->next)
                    usedx->next->prev=usedx->prev;
            }

            freeQueueCount--;
            int index=freeQueue[*freeQueueTail];
            (*freeQueueTail)++;

            printf(BLUE"gc_run: "YELLOW"Freeing memory at offset: %d\n", offset);reset();

            freeNodes[index].prev=*freeHead;
            freeNodes[index].offset=offset;

            *freeHead=freeNodes+index;

            pageQueue[*pageQueueHead]=pagex->index;
            (*pageQueueHead)++;
            pageQueueCount++;

            if(pagex->prev)
            pagex->prev->next=pagex->next;

            if(pagex->next)
            pagex->next->prev=pagex->prev;

           funcTable->funcs[pagex->funcIndex].numPages--;
            
        }
        pagex=pagex->prev;
    }

    gettimeofday(&end, NULL);
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
    printf(BLUE"gc_run: "YELLOW"Garbage collection completed in %lds %ldus\n", seconds, micros);
}

// Initialize garbage collector
void* gc_initialize(void* x){
    while(1){
        sleep(1);
        gc_run();
        compact();
    }
}

// OK
int createMem(int size) {
    /**************** 
        Space for:
            Global Stack
            Function information table
            Function page table stack
            Symbol table
            Free memory tracking
            Used memory tracking
            User demanded memory = X bytes
            no. of pages = ceil (X/512)            
            size of function info table = FUNC_SIZE               
            
            size of function page table stack : number of pages
                - pointer to the stack
                - number of pages amount of free nodes
                - circular queue of size number of free nodes
                - two pointers for the queue

            size of symbol table: HASH
                - number of free nodes: MAX_SYMS
                - circular queue of size MAX_SYMS
                - pointers to the queue
            
            size of Free memory tracking: number of fragments
                - head and tail pointers to the data structure
                - circular queue to store free nodes
                - pointers to the queue

            size of used memory tracking: number of fragments
                - head and tail pointers to the data structure
                - circular queue to store free nodes
                - pointers to the queue

    *****************/
    pthread_mutex_init(&lock, NULL);
    printf(GREEN"createMem: "YELLOW"Memory demanded is %d bytes\n", size);reset();
    int si = sizeof(int);
    int si16 = sizeof(int16_t);

    int sizeMem = 0;
    int numFragments = (size / (4 * PAGE_SIZE)) + 1;
    sizeMem+= sizeof(funcT);
    sizeMem+= 2*sizeof(pageNode**) + numFragments*sizeof(pageNode) + numFragments*sizeof(int) + 2*sizeof(int);
    sizeMem+= sizeof(ST) + MAX_SYMS*sizeof(STNode) + MAX_SYMS * si16 + 2 * si16;
    sizeMem+= numFragments*sizeof(freeMem) + sizeof(freeMem*) + numFragments * si + 2 * si;
    sizeMem+= numFragments*sizeof(usedMem) + 2*sizeof(usedMem*) + numFragments * si + 2 * si;
    sizeMem+= 4 * PAGE_SIZE * numFragments; // Memory needed by the user
    sizeMem+= 2*sizeof(globalStack**)+ MAX_SYMS*sizeof(globalStack) + MAX_SYMS * si16 + 2 * si16;    
    sizeMem+= MAX_SYMS * 10 * (sizeof(arrayBook) + si) + 2 * si;
    
    mem = malloc(sizeMem);
    if (mem == NULL) {
        printf(RED"createMem: "YELLOW"Unable to create memory\n");
        reset();
        return 0;
    }
    else {
        printf(GREEN"createMem: "YELLOW"%d bytes allocated in total\n", sizeMem);reset();
    }

    funcTable = (funcT*)mem;

    pageStackHead = (pageNode**)(funcTable+1);
    pageStackTail = (pageNode**)(pageStackHead+1);
    pageNodes = (pageNode*)(pageStackTail+1);
    pageQueue = (int*)(pageNodes+numFragments);
    pageQueueHead = (int*)(pageQueue+numFragments);
    pageQueueTail = (int*)pageQueueHead+1;
    

    symTable = (ST*)(pageQueueTail+1);
    symNodes = (STNode*)(symTable+1);
    symQueue = (int16_t*)(symNodes+MAX_SYMS);
    symQueueHead = (int16_t*)(symQueue+MAX_SYMS);
    symQueueTail = (int16_t*)(symQueueHead+1);

    freeHead = (freeMem**)(symQueueTail+1);
    freeNodes = (freeMem*)(freeHead+1);
    freeQueue = (int*)(freeNodes+numFragments);
    freeQueueHead = (int*)(freeQueue+numFragments);
    freeQueueTail = (int*)(freeQueueHead+1);

    usedHead = (usedMem**)(freeQueueTail+1);
    
    usedTail = (usedMem**)(usedHead+1);
    usedNodes = (usedMem*)(usedTail+1);
    usedQueue = (int*)(usedNodes+numFragments);
    usedQueueHead = (int*)(usedQueue+numFragments);
    usedQueueTail = (int*)(usedQueueHead+1);

    globalStackHead = (globalStack**)(usedQueueTail+1);
    globalStackTail = (globalStack**)(globalStackHead+1);
    globalStackNodes = (globalStack*)(globalStackTail+1);
    globalStackQueue = (int16_t*) (globalStackNodes+MAX_SYMS);
    globalStackQueueHead = (int16_t*) (globalStackQueue+MAX_SYMS);
    globalStackQueueTail = (int16_t*) (globalStackQueueHead+1);

    bookNodes = (arrayBook*) (globalStackQueueTail+1);
    bookQueue = (int*)  (bookNodes+MAX_SYMS*10);
    bookQueueHead = (int*) (bookQueue+MAX_SYMS*10);
    bookQueueTail = (int*) (bookQueueHead+1);

    userMem = (void*)(bookQueueTail+1);

    // numFragments = PAGE_SIZE*ceil((double)sizeMem/PAGE_SIZE);
    funcIndex = 0;
    globalCounter = 4;

    for(int i = 0; i < MAX_SYMS; i++) {
        symQueue[i]=i;
        symNodes[i].index=i;
    }
    *symQueueHead = *symQueueTail = 0;
    symQueueCount = MAX_SYMS;

    for(int i = 0; i < numFragments; i++)
    {
        pageQueue[i] = i;
        pageNodes[i].index = i;        
    }

    for (int i = 0; i < numFragments; i++) {
        freeNodes[i].index = i;
        freeQueue[i] = -1;
    }
    *pageQueueHead = *pageQueueTail = 0;
    pageQueueCount = numFragments;
    
    *freeQueueHead = *freeQueueTail = 0;
    freeQueueCount = 0;

    *freeHead = freeNodes;
    (*freeHead)->prev = NULL;
    (*freeHead)->offset = 0;
    for(int i = 1; i < numFragments; i++)
    {
        freeNodes[i].offset = (i << 8);
        freeNodes[i].prev = *freeHead;
        *freeHead = freeNodes + i;
        // printf("%ld\t%ld\n",sizeMem,(*freeHead)->offset);
    }    

    for(int i=0;i<numFragments;i++)
    {
        usedNodes[i].index=i;
        usedQueue[i]=i;
    }
    *usedQueueHead = *usedQueueTail = 0;
    usedQueueCount = numFragments;

    for(int i=0;i < MAX_SYMS;i++)
    {
        globalStackQueue[i]=i;
        globalStackNodes[i].index=i;
    }
    *globalStackQueueHead = *globalStackQueueTail = 0;
    globalStackQueueCount = MAX_SYMS;

    for(int i=0;i < MAX_SYMS * 10;i++)
    {
        bookQueue[i]=i;
        bookNodes[i].index=i;
    }
    (*bookQueueHead) = (*bookQueueTail) = 0;
    bookQueueCount = MAX_SYMS * 10;

    printf(GREEN"createMem: "YELLOW"Creating Garbage Collector\n");reset();
    pthread_create(&gcID, NULL, gc_initialize, NULL);    

    
    return 0;
}

// OK - Add Mutex Locks
int demandPage(){
    
    if (*freeHead == NULL) {
        printf(RED"demandPage: "YELLOW"No free memory!\n");reset();
        return 0;
    }
    else{
        int index = usedQueue[*usedQueueTail];
        // printf("accessing index %d\n",index);
        (*usedQueueTail)++;
        usedQueueCount--;
        usedNodes[index].prev=*usedHead;
        
        if(*usedHead)
        (*usedHead)->next=usedNodes+index;
        *usedHead=usedNodes+index;

        index=pageQueue[*pageQueueTail];
        (*pageQueueTail)++;
        pageQueueCount--;
        pageNodes[index].prev=*pageStackHead;

        if(*pageStackHead)
        (*pageStackHead)->next=pageNodes+index;
        *pageStackHead=pageNodes+index;

        // (*usedHead)->page = *pageStackHead;
        (*usedHead)->offset = (*freeHead)->offset;
        (*usedHead)->next=NULL;

        (*pageStackHead)->markBit=0;
        (*pageStackHead)->pointer=*usedHead;
        (*pageStackHead)->currOffset=0;
        (*pageStackHead)->funcIndex=funcIndex;
        (*pageStackHead)->next=NULL;

        freeQueue[*freeQueueHead]=(*freeHead)->index;
        freeQueueCount++;
        (*freeQueueHead)++;

        printf(GREEN"demandPage: "YELLOW"Fragment assigned at offset %d\n",(*freeHead)->offset);reset();
        funcTable->funcs[funcIndex-1].numPages++;
        *freeHead=(*freeHead)->prev;
        return 1;
    }
}

// OK
STNode *getSTEntry(struct myType data){
    unsigned long long counter = data.counter;
    int hashIndex = counter % HASH;
    STNode* curr = symTable->table[hashIndex];
    while(curr && curr->counter != counter) curr = curr->next;
    if (curr == NULL) {
        printf(RED"getSTEntry:"YELLOW" Invalid memory access\n");reset();
        exit(EXIT_FAILURE);
    }
    return curr;
}

int initFunction(){

    if(funcIndex==MAX_FUNC) {
        printf("MAX NUMBER OF FUNCTIONS REACHED\n");
        exit(EXIT_FAILURE);
    }

    (*funcTable).funcs[funcIndex].funcIndex=funcIndex;
    (*funcTable).funcs[funcIndex].numPages=0;
    funcIndex++;

    int index = globalStackQueue[*globalStackQueueTail];
    (*globalStackQueueTail)++;
    globalStackQueueCount--;

    // printf("first node extracting from index: %d\n",index);

    globalStackNodes[index].prev=*globalStackHead;
    if(*globalStackHead)
    (*globalStackHead)->next=globalStackNodes+index;

    *globalStackHead=globalStackNodes+index;
    (*globalStackHead)->stnode=NULL;
    (*globalStackHead)->next=NULL;
    return 1;
}

struct myType returnFromFunc(struct myType retData){
    while((*globalStackHead)->stnode){
        struct myType var = {(*globalStackHead)->stnode->counter};
        if(var.counter!=retData.counter) {
            freeElem(var);

        }

        else{
            if((*globalStackHead)->prev) 
            (*globalStackHead)->prev->next = NULL;
            globalStack *curr = (*globalStackHead);
            (*globalStackHead)=(*globalStackHead)->prev;

            curr->prev=NULL;
        }
       
    }
    globalStackQueue[*globalStackQueueHead]=(*globalStackHead)->index;
    (globalStackQueueCount)++;
    (*globalStackQueueHead)++;

    if((*globalStackHead)->prev) (*globalStackHead)->prev->next=NULL;
    (*globalStackHead)=(*globalStackHead)->prev;
    
    funcIndex--;   

    printf("\nCurrent func index= %d\n",funcIndex);

    struct myType ret;
    if(retData.counter){

        STNode* stnode = getSTEntry(retData);
        if(stnode->type==_INT)
        ret = createVar("_int");

        if(stnode->type==_CHAR)
        ret = createVar("_char");

        if(stnode->type==_BOOL)
        ret = createVar("_bool");

        if(stnode->type==_MEDIUM_INT)
        ret = createVar("_medium_int");

        assignVarVar(ret, retData);
        freeElem(retData);
        gc_run();
        return ret;
    }
    

    else{
        ret.counter=0;
        gc_run();
        return ret;
    }
}

// OK - Lock needs to be added
struct myType createVar(const char *type)
{
    int8_t typec = -1;
    if(!strcmp(type,"_int")) typec=_INT;    
    else if(!strcmp(type,"_char")) typec=_CHAR;    
    else if(!strcmp(type,"_bool")) typec=_BOOL;    
    else if(!strcmp(type,"_medium_int")) typec=_MEDIUM_INT;
    else {
        printf(RED"createVar: "YELLOW"Invalid type\n");reset();
        struct myType rv = {0};
        return rv;
    }

    if((funcTable)->funcs[funcIndex-1].numPages == 0 || (*pageStackHead)->currOffset==PAGE_SIZE)
    {
        printf(GREEN"createVar: "YELLOW"Demanding new page by func index : %d\n", funcIndex - 1);
        printf(GREEN"createVar: "YELLOW"Current function pages: %d\n",(funcTable)->funcs[funcIndex-1].numPages);
        reset();
        if(demandPage() == 0){
            printf(RED"createVar: "YELLOW"Cannot find free page!\n");reset();
            exit(EXIT_FAILURE);
        }
    }    

    int hashIndex = globalCounter % HASH;
    // printf("hash value: %ld\n",globalCounter);
    STNode *curr = NULL;

    if(symTable->table[hashIndex] != NULL){
        curr = symTable->table[hashIndex];
        while(curr->next != NULL) curr=curr->next;
    }

    if(symQueueCount == 0) {
        printf(RED"createVar: "YELLOW"Maximum variable limit reached\n");reset();
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&lock);
    int index = *symQueueTail;
    (*symQueueTail)++;
    symQueueCount--;
    

    if(curr) curr->next = symNodes + index;
    else symTable->table[hashIndex] = symNodes + index;
        
    

    curr = symNodes + index;
    pthread_mutex_unlock(&lock);

    curr->counter    = globalCounter;
    curr->functionID = funcIndex;
    curr->pageIndex  = *pageStackHead;
    curr->pageOffset = (*pageStackHead)->currOffset;
    curr->isArray    = false;
    curr->next       = NULL;


    pthread_mutex_lock(&lock);
    index = *globalStackQueueTail;
    (*globalStackQueueTail)++;
    globalStackQueueCount--;

    globalStackNodes[index].prev = *globalStackHead;
    (*globalStackHead)->next = globalStackNodes+index;

    *globalStackHead = globalStackNodes + index;

   
    (*globalStackHead)->stnode = curr;
    (*globalStackHead)->next   = NULL;

    curr->stackLoc = *globalStackHead;
    curr->size = 1;
    curr->type = typec;    
    
    (*pageStackHead)->currOffset++;

     pthread_mutex_unlock(&lock);

    printf(GREEN"createVar: "YELLOW"Page: %d Offset: %d\n", curr->pageIndex->index,curr->pageOffset);
    printf(GREEN"createVar: "YELLOW"Variable of type "BLUE"%s"YELLOW" created!\n", type);reset();
    struct myType ret = {globalCounter};
    globalCounter += 4;
    return ret;    
}

// TODO - Lock needs to be added
struct myType createArr(const char *type, unsigned size){
    int hashIndex = globalCounter % HASH;
    
    // TODO - Check this again
    int8_t typec = -1;
    int numWords = 0;
    if(!strcmp(type,"_int")) {
        typec=_INT;   
        numWords = size;
    } 
    else if(!strcmp(type,"_char")) {
        typec=_CHAR;   
        numWords= ceil(((double)size)/4);
    }
    else if(!strcmp(type,"_bool")) {
        typec=_BOOL;  
        numWords= ceil(((double)size)/32);
    }   
    else if(!strcmp(type,"_medium_int")) {
        typec=_MEDIUM_INT;
        numWords = size;
    }
    else {
        printf(RED"createArr: "YELLOW"Invalid type\n");reset();
        struct myType rv = {0};
        return rv;
    }
    if(symQueueCount == 0){
        printf(RED"createArr: "YELLOW"Maximum variable limit reached\n");reset();
        exit(EXIT_FAILURE);
    }
    
     pthread_mutex_lock(&lock);
    int index = symQueue[*symQueueTail];
    (*symQueueTail)++;
    symQueueCount--;

    // Get a new STNode
    STNode *curr;
    if (symTable->table[hashIndex] == NULL) 
        symTable->table[hashIndex] = symNodes + index;

    else {
        curr = symTable->table[hashIndex];
        while(curr->next!=0)
        curr=curr->next;
        curr->next=symNodes+index;
    }

    curr = symNodes + index;

    index = globalStackQueue[*globalStackQueueTail];
    (*globalStackQueueTail)++;
    globalStackQueueCount--;

    (*globalStackHead)->next=globalStackNodes+index;
    globalStackNodes[index].prev=*globalStackHead;
    *globalStackHead=globalStackNodes+index;

    (*globalStackHead)->next=NULL;
    (*globalStackHead)->stnode=curr;

     pthread_mutex_unlock(&lock);

    // Fill parameters */
    curr->counter    = globalCounter;
    curr->functionID = funcIndex;
    curr->pageIndex  = NULL;
    curr->stackLoc   = *globalStackHead;
    curr->pageOffset = -1;
    curr->size       = size;
    curr->isArray    = true;
    curr->type       = typec;
    curr->next       = NULL;

    arrayBook *iter=curr->book;


     pthread_mutex_lock(&lock);
    while(numWords) {

        if((funcTable)->funcs[funcIndex-1].numPages==0||(*pageStackHead)->currOffset==PAGE_SIZE)
        {
            printf(GREEN"createArr: "YELLOW"Array Demanding new page by func index : %d\n",funcIndex-1);reset();
            if(demandPage() == 0){
                printf(RED"createArr: "YELLOW"Cannot find a new page\n");reset();
                exit(EXIT_FAILURE);
            }
        }  

        index = bookQueue[*bookQueueTail];
        (*bookQueueTail)++;
        bookQueueCount--;

        if(curr->book == NULL) curr->book= bookNodes + index;
        if(iter) iter->next = bookNodes + index;
        iter = bookNodes + index;
        iter->pageIndex  = *pageStackHead;
        iter->pageOffset = (*pageStackHead)->currOffset;
        iter->next       = NULL;
        if(numWords > PAGE_SIZE - (*pageStackHead)->currOffset) {
            numWords -= PAGE_SIZE - (*pageStackHead)->currOffset;
            (*pageStackHead)->currOffset = PAGE_SIZE;
        }
        else {
            (*pageStackHead)->currOffset += numWords;
            numWords = 0;            
        } 
    }

     pthread_mutex_unlock(&lock);
    // Print all the array pages
    iter = curr->book;
    while (iter) {
        printf(GREEN"createArr: "YELLOW"Array assigned at page index: %d offset: %d\n", 
                iter->pageIndex->index,iter->pageOffset);
        reset();
        iter=iter->next;
    }
    globalCounter += size * 4;
    struct myType ret={curr->counter};
    return ret;
}

// OK - Lock needs to be added
int assignVarConst(struct myType name, int value){
    STNode* curr = getSTEntry(name);
     pthread_mutex_lock(&lock);
    void *word = userMem + curr->pageIndex->pointer->offset + curr->pageOffset * 4;
    printf(GREEN"assignVarConst: "YELLOW"Memory fragment : %d, ", curr->pageIndex->pointer->offset);reset();
    printf(YELLOW"Byte Number in fragment: %d\n", curr->pageOffset * 4);reset();
    memcpy(word, &value, 4);
     pthread_mutex_unlock(&lock);
    return 0;

}

// OK - Lock needs to be added
int assignVarVar(struct myType dest, struct myType src){    

    STNode* currDest = getSTEntry(dest);
     pthread_mutex_lock(&lock);
    void *wordDest= userMem+currDest->pageIndex->pointer->offset + currDest->pageOffset * 4;
    STNode* currSrc = getSTEntry(src);
    void *wordSrc= userMem+currSrc->pageIndex->pointer->offset+currSrc->pageOffset*4;
    
    if(currSrc->type != currDest->type) {
        printf(RED"assignVarVar: "YELLOW"Types don't match\n"); reset();
        return 0;
    }
    if(currDest->isArray || currSrc->isArray) {
        printf(RED"assignVarVar: "YELLOW"Cannot assign to or from arrays!\n"); reset();
        return 0;
    }
    printf(GREEN"assignVarVar: "YELLOW"Source Memory fragment : %d, ", currSrc->pageIndex->pointer->offset);reset();
    printf(YELLOW"Source Byte Number in fragment: %d\n", currSrc->pageOffset * 4);reset();
    printf(GREEN"assignVarVar: "YELLOW"Destination Memory fragment : %d, ", currDest->pageIndex->pointer->offset);reset();
    printf(YELLOW"Destination Byte Number in fragment: %d\n", currDest->pageOffset * 4);reset();
    memcpy(wordDest, wordSrc, 4);  

     pthread_mutex_unlock(&lock); 
}

// OK
void printVar(struct myType data) {
    STNode *curr = getSTEntry(data);
     pthread_mutex_unlock(&lock);
    void *word= userMem + curr->pageIndex->pointer->offset + curr->pageOffset*4;    
    if(curr->type==_INT) printf(GREEN"printVar: "YELLOW"Integer data: %d\n",*(int*)word);
    if(curr->type==_CHAR) printf(GREEN"printVar: "YELLOW"Character data: %c\n",*(char*)(word));
    if(curr->type==_MEDIUM_INT) {
        int mask = INT32_MAX-(1<<30)-(1<<29)-(1<<28);
        printf(GREEN"printVar: "YELLOW"Medium integer data: %d\n",(*(int*)word)&mask);
    }
    if(curr->type==_BOOL)
    {
        if((*(int*)word) & 1) printf(GREEN"printVar: "YELLOW"Boolean data: true\n");
        else printf(GREEN"printVar: "YELLOW"Boolean data: false\n");
    }
     pthread_mutex_unlock(&lock);
    reset();
}

// OK
int freeElem(struct myType data){
    STNode* curr = getSTEntry(data);
    printf(GREEN"freeElem: "YELLOW"Freeing memory at counter %lld\n", data.counter);reset();
    int hashIndex = data.counter % HASH;
    int index = curr->index;  
    
    // Free from symbol table 
    if(curr == (symTable)->table[hashIndex]) symTable->table[hashIndex] = curr->next;
    else{
        STNode* prev = symTable->table[hashIndex];
        while(prev->next->counter != data.counter) prev=prev->next;
        prev->next = curr->next;
    }
    curr->counter = 0;
    // Free the pages pointed to by the array
    if(curr->isArray){
        arrayBook *iter = curr->book;
        while(iter){
            bookQueue[*bookQueueHead] = iter->index;
            (*bookQueueHead)++;
            bookQueueCount++;
            iter = iter->next;
        }
    }
    
    globalStackQueue[*globalStackQueueHead] = curr->stackLoc->index;
    (*globalStackQueueHead)++;
    globalStackQueueCount++;
    // Pop from the stack
    if(curr->stackLoc->next) curr->stackLoc->next->prev = curr->stackLoc->prev;
    if(curr->stackLoc->prev) curr->stackLoc->prev->next = curr->stackLoc->next;
    if(curr->stackLoc == (*globalStackHead)) (*globalStackHead) = (*globalStackHead)->prev;
    // Pop from the queue
    symQueue[*symQueueHead]=index;
    (*symQueueHead)++;
    symQueueCount++;
    
    printf(GREEN"freeElem: "YELLOW"Freed memory at counter %lld\n", data.counter);reset();
}

// OK
int getSize(STNode *node) {
    switch(node->type) {
        case _INT: return 32;
        case _CHAR: return 8;
        case _BOOL: return 1;
        case _MEDIUM_INT: return 24;
    }
}

// TODO - Check function again
int getVar(struct myType data) {
    STNode* src = getSTEntry(data);
    if (src->isArray) {
        printf(RED"getVar: "YELLOW"Single variable expected, not array!\n");
        reset();
        exit(EXIT_FAILURE);
    } 
    // Copy the bits
    void *wordSrc = userMem + src->pageIndex->pointer->offset + src->pageOffset*4;
    // Get the size
    int size = getSize(src);
    if (size == 1) {
        unsigned char *cx = (unsigned char *)(wordSrc);
        return *cx & (1U << 7);
    }
    int rv;
    size /= 4;
    memcpy(&rv, wordSrc, size);    
    return rv;
}

int assignArrConst(struct myType name, int value, int index){
    STNode *curr = getSTEntry(name);
    if(index>=curr->size){
        printf("INVALID MEMORY ACCESS\n");
        exit(EXIT_FAILURE);
    }

    if(!curr->isArray){
        printf("REQUESTED VARIABLE NOT AN ARRAY\n");
        exit(EXIT_FAILURE);
    }

    int reqWord;

    if(curr->type==_INT||curr->type==_MEDIUM_INT)
        reqWord = index;

    else if(curr->type==_CHAR)
        reqWord = index/4;
    
    else if(curr->type==_BOOL)
        reqWord = index/32;

    arrayBook *iter = curr->book;
    void *word;

    while(iter){

        // printf("reqWord: %d\n",reqWord);
        if(reqWord<PAGE_SIZE-iter->pageOffset)
        {
            word = userMem + iter->pageIndex->pointer->offset + reqWord*4; 
            break;
        }

        else{
            reqWord-=PAGE_SIZE-iter->pageOffset;
            iter=iter->next;
        }
    }

    if(curr->type==_INT||curr->type==_MEDIUM_INT)
    {
        memcpy(word, &value,4);
        printf("word after assignment is %d\n", *((int*)word));
    }

    if(curr->type==_CHAR)
    {
        int offset = index%4;
        memcpy(word+offset,((void*)&(value)),1);
    }

    if(curr->type==_BOOL){
        int offset = index%32;
        int x=value<<offset;
        int mask = -1 & (!(1<<offset));
        int *wordp = (int*)word;
        *wordp = *wordp & mask;
        *wordp = *wordp | mask;
    }
}

int getArrayEntry(struct myType arr, int index){
    STNode *curr = getSTEntry(arr);
    if(index>=curr->size){
        printf("INVALID MEMORY ACCESS\n");
        exit(EXIT_FAILURE);
    }

    if(!curr->isArray){
        printf("REQUESTED VARIABLE NOT AN ARRAY\n");
        exit(EXIT_FAILURE);
    }

    int reqWord;

    if(curr->type==_INT||curr->type==_MEDIUM_INT)
        reqWord = index;

    else if(curr->type==_CHAR)
        reqWord = index/4;
    
    else if(curr->type==_BOOL)
        reqWord = index/32;

    arrayBook *iter = curr->book;
    void *word;

     pthread_mutex_lock(&lock);
    while(iter){

        // printf("reqWord: %d\n",reqWord);
        if(reqWord<PAGE_SIZE-iter->pageOffset)
        {
            printf("offset being accessed is %d\n",iter->pageIndex->pointer->offset + reqWord*4);
            
            word = userMem + iter->pageIndex->pointer->offset + reqWord*4; 
            break;
        }

        else{
            reqWord-=PAGE_SIZE-iter->pageOffset;
            iter=iter->next;
        }
    }

    if(curr->type==_INT||curr->type==_MEDIUM_INT)
    {
         pthread_mutex_unlock(&lock);
        return *((int*)word);
    }

    if(curr->type==_CHAR)
    {
        int offset = index%4;
        char c;
        memcpy((void*)&c,word+offset,1);
         pthread_mutex_unlock(&lock);

        return c;
    }

    if(curr->type==_BOOL){
        int offset = index%32;
        int x=1<<offset;
        
        int *wordp = (int*)word;
         pthread_mutex_unlock(&lock);
        
        if(x&(*wordp)) return 1;
        return 0;
    }
}


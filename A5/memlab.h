#ifndef MEMLAB_H
#define MEMLAB_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

/* To print colored output */
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"

/* Constants in the program */
#define HASH 113        // Number of rows in the Symbol Table (Hash Table)
#define FUNC_SIZE 17       
#define MAX_SYMS 1000   // Maximum total number of symbols
#define MAX_FUNC 100    // Maximum number of functions
#define PAGE_SIZE 128   // Page size in number of words, each word is 4 bytes

/* Data types */
#define _INT 1
#define _BOOL 2
#define _CHAR 3
#define _MEDIUM_INT 4

/* Max values */
#define INT_MAX 2147483647
#define CHAR_MAX 255
#define MEDIUM_MAX 16777215
#define BOOL_MAX 1

/* Threads */
pthread_t gcID;

/* Mutex Locks */
pthread_mutex_t lock;
pthread_mutex_t input;

// Memory
void *mem;      // All memory
void *userMem;  // Memory asked by the user


/* Struct for abstract data type used to represent a variable */
typedef struct myType{
    unsigned long long counter;
}_int, _char, _medium_int, _bool;

unsigned long long globalCounter;   /* Global counter, always incremented by 4 */

/* Pre-declaration of all the structs */
typedef struct pageNode pageNode;      // Page
typedef struct globalStack globalStack;// Global stack of variables
typedef struct arrayBook arrayBook;    // All Pages in an array
typedef struct STNode STNode;          // Symbol Table Node
typedef struct ST ST;                  // Symbol Table
typedef struct freeMem freeMem;        // Data Structure which is used to represent a free Memory frame
typedef struct usedMem usedMem;        // Data Structure which is used to represent a used Memory frame
typedef struct funcInfo funcInfo;      
typedef struct funcT funcT;


/* Symbol Table Node */
typedef struct STNode{
    unsigned long long counter; // Counter assigned to the variable
    int16_t functionID;         // The function the Symbol belongs to
    pageNode*  pageIndex;       // ONLY FOR NON-ARRAYS: The Page the symbol references
    globalStack* stackLoc;      // Pointer in the globalStack entry
    int8_t type;                // 1, 2, 3 or 4: defined above
    int16_t  pageOffset;        // Starting from which position in the page
    int size;                   // The size of the array (1 for non-arrays)
    bool isArray;               // Whether symbol is an array or not
    int16_t index;              // The index in the Symbol Table queue which the node references
    struct arrayBook *book;     // ONLY FOR ARRAYS: Linked list of all the pages in the array
    struct STNode *next;        // Pointer to the next symbol in the chain of the current row
}STNode;

STNode *symNodes;               // Pool of nodes to be used
int16_t *symQueue;              // Queue of indices which can be used
int16_t *symQueueHead;          // Head of the queue
int16_t *symQueueTail;          // Tail of the queue
int16_t symQueueCount;          // Number of symbols

//

/* 
    Symbol Table which has been made as a Hash Table of HASH rows
    Hashing has been done using chaining. 
*/
typedef struct ST{
    STNode *table[HASH];    // Symbol Table row points to the List of STNodes
}ST;

ST *symTable;       // Global Symbol Table

//

/* Data Structure used to represent a frame in free memory */
/* Represented as a singly linked list because we won't be removing
anything from the middle */
typedef struct freeMem{
    int offset;    // Offset from the start of the memory where this frame starts, like 0, 512, etc.
    int index;     // Index in the queue of freeMem nodes
    struct freeMem *prev;   // Previous freeMem node
}freeMem;

freeMem **freeHead;   // Header of the linked list
freeMem *freeNodes;   // The pool of nodes to be used
int *freeQueue;       // Indices queue
int *freeQueueHead;   // Head of the indices queue
int *freeQueueTail;   // Tail of the indices queue
int freeQueueCount;   // Number of free pages

//

/* Data Structure used to represent a frame in used memory */
/* Used memory is denoted as a doubly linked list to accomodate for the 
freeElem function */
typedef struct usedMem{
    int index;    // Index in the queue of usedMem nodes
    int offset;   // Offset from the start of the memory where this frame starts, like 0, 512, etc  
    struct usedMem *next;  // Next freeMem node
    struct usedMem *prev;  // Previous freeMem node
}usedMem;

usedMem **usedHead;
usedMem **usedTail;
usedMem *usedNodes;
int *usedQueue; 
int *usedQueueHead;
int *usedQueueTail;
int  usedQueueCount;     // Number of used pages

//

/* 
    Data structure for a page 
    Page Tables will be doubly linked lists of pages
*/
typedef struct pageNode{
    bool markBit;           // Mark bit for mark and sweep
    usedMem *pointer;       // Pointer to a used memory node
    int16_t currOffset;     
    int16_t funcIndex;      // Function this page belongs to
    int index;              // Index in the queue of pages
    struct pageNode *prev;  // Previous Page in the Table
    struct pageNode *next;  // Next Page in the Table
}pageNode;

pageNode **pageStackHead;  // Head of the linked list of pages
pageNode **pageStackTail;  // Tail of the linked list of pages
pageNode *pageNodes;       // Pool of pageNodes
int *pageQueue;            // Queue of page indices
int *pageQueueHead;        // Head of the queue
int *pageQueueTail;        // Tail of the queue
int  pageQueueCount;        // Pages in used currently

//

/* Data structure to denote a list of pages needed by an array */
typedef struct arrayBook{
    pageNode* pageIndex;    // The page it points to
    int16_t pageOffset;     
    int index;          
    struct arrayBook *next; // The next page of the array
}arrayBook;

arrayBook *bookNodes;
int *bookQueue;
int *bookQueueHead;
int *bookQueueTail;
int  bookQueueCount;

//

typedef struct funcInfo{
    int16_t funcIndex;
    int numPages;  
}funcInfo;

typedef struct funcT{
    funcInfo funcs[MAX_FUNC];
}funcT;

funcT *funcTable;       // Table of functions
int16_t funcIndex;      // Function index counter

//

/* The global stack of symbols as a doubly linked list */
typedef struct globalStack{
    STNode *stnode;             // Back pointer to the symbol table entry
    int16_t index;              // Index in the symbol queue
    struct globalStack *next;   // Next entry
    struct globalStack *prev;   // Previous entry
}globalStack;

globalStack **globalStackHead;  // Head of the queue
globalStack **globalStackTail;  // Tail of the queue
globalStack *globalStackNodes;  // All the nodes allocated for the globalStack
int16_t *globalStackQueue;      // Indices in global stack
int16_t *globalStackQueueHead;  // Head of the queue
int16_t *globalStackQueueTail;  // Tail of the queue
int16_t  globalStackQueueCount; // Count of the number of used elements

//

/* FUNCTION PROTOTYPES */
struct myType createVar(const char *type);

int createMem(int size);

int assignVarConst(struct myType name, int value);

int assignVarVar(struct myType dest, struct myType src);

struct myType createArr(const char *type, unsigned size);

int assignArrConst(struct myType name, int value, int index);

int freeElem(struct myType name);

int getVar(struct myType);

int getArrayEntry(struct myType, int index);

int initFunction();

void printVar(struct myType);

struct myType returnFromFunc(struct myType);

#endif


/*
    Group 26

    Seemant G. Achari   (19CS10045)
    Rajas Bhatt         (19CS30037)

    Assignment_4_26.cpp
*/

/*

To compile, use
    g++ -Wall -pthread Assignment_4_26.cpp -o Assignment_4_26

To run, use
    ./Assignment_4_26

To run in debug mode, use
    ./Assignment_4_26 -d

To run in good output mode, use
    ./Assignment_4_26 -g

To redirect to external file, use
    ./Assignment_4_26 -r 
    ./Assignment_4_26 -r <filename>

To redirect to external file with good output, use
    ./Assignment_4_26 -gr 
    ./Assignment_4_26 -gr <filename>

*/

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <sys/wait.h>
#include <fcntl.h>

#define READY 0
#define EXECUTING 1
#define DONE 2
#define UNDEFINED -1

#define MAX_CHILDREN 20
#define MAX_ROOT_PROCESS 500
#define MAX_TREE_SIZE 1500
#define MAX_COMP_TIME 250
#define MAX_JOB_ID 100000000
#define QUEUE_SIZE (MAX_TREE_SIZE + 1)
#define MIN_PROD_SLEEP 200
#define MAX_PROD_SLEEP 500

/* Maximum of 10 producers, maximum number of created jobs
    = 10 * (Max running time / Minimum sleep time of producer) = 10 * (20 / 0.2) = 1000
*/

using namespace std;

bool __DEBUG{false};    /* Enable or disable DEBUG mode */
bool __GOOD_OUTPUT{false}; /* Enable for good output */
bool __RED{false};  /* File redirection */
string __red_file{"output.txt"};

struct Node {
    unsigned int _job_id;
    double _comp_time;
    int _dep_jobs_freq;
    Node *_dep_jobs[MAX_CHILDREN];
    pthread_mutex_t _mutex_lock;
    int _status;
    Node *_parent;
};

pthread_mutex_t queue_mutex;
pthread_mutex_t print;
pthread_mutex_t pid_m, cid_m;

int prod = 0;
int cons = 0;

void *producer_routine(void *arg) {
    pthread_mutex_lock(&pid_m);
    int thread_id = prod;
    prod += 1;
    pthread_mutex_unlock(&pid_m);
    long int running_time = 10 + rand() % 11;    // Number of seconds
    /* Get all the variables */
    Node *jobs = (Node *)arg;

    int *out, *size, *queue, *in;
    void *q = (void *)((char *)arg + MAX_TREE_SIZE * sizeof(Node));
    in = (int *)q;
    out = (int *)q + 1; /* Read end */
    size = ((int *)q) + 2;
    queue = ((int *)q)+ 4; 
    if (__DEBUG) {
        cout << "Running time of " << thread_id << " " << running_time << "\n";   
    } 
    time_t start = time(NULL);

    while (time(NULL) - start < running_time) {
        // cout << *size << " " << *in << " " << *out << "\n";
        int sJob = rand() % MAX_TREE_SIZE;
        pthread_mutex_lock(&jobs[sJob]._mutex_lock);
        if (jobs[sJob]._status == READY && jobs[sJob]._dep_jobs_freq < MAX_CHILDREN) {
            pthread_mutex_lock(&queue_mutex);
            if (*size == 0 || *size == MAX_TREE_SIZE) {
                pthread_mutex_unlock(&queue_mutex);
                continue;
            }
            int cVert = *out;
            *out = (*out + 1) % QUEUE_SIZE;
            *size = *size - 1;
            pthread_mutex_unlock(&queue_mutex);

            pthread_mutex_lock(&jobs[queue[cVert]]._mutex_lock);

            jobs[sJob]._dep_jobs[jobs[sJob]._dep_jobs_freq++] = &jobs[queue[cVert]];
            jobs[queue[cVert]]._job_id = rand() % MAX_JOB_ID + 1;
            jobs[queue[cVert]]._dep_jobs_freq = 0;
            jobs[queue[cVert]]._comp_time = rand() % MAX_COMP_TIME + 1;
            jobs[queue[cVert]]._parent = &jobs[sJob];
            jobs[queue[cVert]]._status = READY;

            if (__DEBUG) {
                cout << "Thread : " << thread_id << "\n";
                cout << "New Job Details\n";
                cout << "Job ID : " << jobs[cVert]._job_id << "\n";
                cout << "Comp Time: " << jobs[cVert]._comp_time << "\n";
                cout << "Parent Index: " << sJob << "\n";
                cout << "Child Index : " << cVert << "\n"; 
            }
            if (__GOOD_OUTPUT) {
                pthread_mutex_lock(&print);
                cout << "Producer " << thread_id << " ==> ";
                cout << "Added job (" << jobs[queue[cVert]]._job_id << "," << queue[cVert] << ") to job "; 
                cout << "("<< jobs[sJob]._job_id << "," << sJob << ")\n";
                pthread_mutex_unlock(&print);
            }
            else {
                cout << "Added job (" << jobs[queue[cVert]]._job_id << "," << queue[cVert] << ") to job "; 
                cout << "("<< jobs[sJob]._job_id << "," << sJob << ")\n";
            }

            pthread_mutex_unlock(&jobs[queue[cVert]]._mutex_lock);
            pthread_mutex_unlock(&jobs[sJob]._mutex_lock);

            int sleepTime = MIN_PROD_SLEEP + (rand()) % (MAX_PROD_SLEEP - MIN_PROD_SLEEP + 1); 
            usleep(sleepTime * 1000);
        }
        else pthread_mutex_unlock(&jobs[sJob]._mutex_lock);
    }
    if (__GOOD_OUTPUT) {
        pthread_mutex_lock(&print);
        cout << "Producer " << thread_id << " ==> ";
        cout << "Exiting producer" << "\n";
        pthread_mutex_unlock(&print);
    }
    return NULL;
}

void *consumer_routine(void *arg) {
    /* Get consumer thread id */
    pthread_mutex_lock(&cid_m);
    int thread_id = cons;
    cons += 1;
    pthread_mutex_unlock(&cid_m);
    /* Get all the variables */
    Node *jobs = (Node *)arg;

    int *in, *size, *queue, *allOver, *out;
    void *q = (void *)((char *)arg + MAX_TREE_SIZE * sizeof(Node));
    in = (int *)q; /* Write end */
    out = ((int *)q) + 1;
    size = ((int *)q) + 2;
    queue = ((int *)q) + 4; 
    allOver = ((int *)q) + 3;

    bool found;
    do {
        found = false;
        for (int i = 0; i < MAX_TREE_SIZE; i++) {            
            pthread_mutex_lock(&jobs[i]._mutex_lock);
            if (jobs[i]._dep_jobs_freq == 0 && jobs[i]._status == READY) {
                found = true;
                /* Sleep for the specified period */
                jobs[i]._status = EXECUTING;
                if (__GOOD_OUTPUT) {
                    pthread_mutex_lock(&print);
                    cout << "Consumer " << thread_id << " ==> ";
                    cout << "Execution for (" << jobs[i]._job_id << "," << i << ") started\n";
                    pthread_mutex_unlock(&print);
                }
                else  {
                    cout << "Execution for (" << jobs[i]._job_id << "," << i << ") started\n";
                }
                pthread_mutex_unlock(&jobs[i]._mutex_lock);
                usleep(jobs[i]._comp_time * 1000);
                pthread_mutex_lock(&jobs[i]._mutex_lock);
                /* Remove this job from the children queue of the parent */
                if (jobs[i]._parent != NULL) {
                    pthread_mutex_lock(&jobs[i]._parent->_mutex_lock);
                    for (int j = 0; j < jobs[i]._parent->_dep_jobs_freq; j++) {
                        if (jobs[i]._parent->_dep_jobs[j] == &jobs[i]) {
                            for (int k = j; k < jobs[i]._parent->_dep_jobs_freq - 1; k++) {
                                jobs[i]._parent->_dep_jobs[k] = jobs[i]._parent->_dep_jobs[k + 1];
                            }
                        }
                    }
                    jobs[i]._parent->_dep_jobs_freq--;
                    pthread_mutex_unlock(&jobs[i]._parent->_mutex_lock);
                }
                if (__GOOD_OUTPUT) {
                    pthread_mutex_lock(&print);
                    cout << "Consumer " << thread_id << " ==> ";
                    cout << "Execution for (" << jobs[i]._job_id << "," << i << ") complete\n";
                    pthread_mutex_unlock(&print);
                }
                else cout << "Execution for (" << jobs[i]._job_id << "," << i << ") complete\n";
                jobs[i]._status = UNDEFINED;
                jobs[i]._parent = NULL;
                /* Put this into the empty process queue */  
                pthread_mutex_unlock(&jobs[i]._mutex_lock);

                pthread_mutex_lock(&queue_mutex);
                queue[*in] = i;
                *in = (*in + 1) % QUEUE_SIZE;
                *size = *size + 1;
                // cout << "Cons " << " " << *size << " " << *in << " " << *out << "\n";
                pthread_mutex_unlock(&queue_mutex);
                break;
            }
            else pthread_mutex_unlock(&jobs[i]._mutex_lock);
        }
    }while(found || *allOver == 0);
    
    if (__GOOD_OUTPUT) {
        pthread_mutex_lock(&print);
        cout << "Consumer " << thread_id << " ==> ";
        cout << "Exiting consumer" << "\n";
        pthread_mutex_unlock(&print);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(26);
    if (argc == 2 && !strcmp(argv[1], "-d")) __DEBUG = true;
    if (argc == 2 && !strcmp(argv[1], "-g")) __GOOD_OUTPUT = true;
    if (argc == 2 && !strcmp(argv[1], "-gr")) {
        __RED = true;
        __GOOD_OUTPUT = true;
    }
    if (argc == 3 && !strcmp(argv[1], "-gr")) {
        __RED = true;
        __GOOD_OUTPUT = true;
        __red_file = string(argv[2]);
    }
    if (argc == 2 && !strcmp(argv[1], "-r")) {
        __RED = true;
    }
    if (argc == 3 && !strcmp(argv[1], "-r")) {
        __RED = true;
        __red_file = string(argv[2]);
    }
    if (__DEBUG) cout << "Debug mode enabled!\n";
    if (__GOOD_OUTPUT) cout << "Good output mode enabled!\n";
    if (__RED) cout << "Reading mode enabled!\n";
    int P, y;
    cout << "Producer threads (P): ";
    cin >> P;
    if (__DEBUG) cout << "P = " << P << "\n";
    cout << "Consumer threads (y): ";
    cin >> y;
    if (__DEBUG) cout << "y = " << y << "\n";

    if (__RED) {
        freopen(__red_file.c_str(), "w", stdout);
    }

    /* Attributes of all the mutex locks */
    /* PTHREAD_PROCESS_SHARED so that we can share locks between the producers (created
    by process A) and the consumers (created by process B) */
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    /* Initialise all global mutex locks */
    pthread_mutex_init(&queue_mutex, &attr);
    pthread_mutex_init(&print, &attr);
    pthread_mutex_init(&pid_m, &attr);
    pthread_mutex_init(&cid_m, &attr);


    key_t key = 26;
    size_t alloc_size = MAX_TREE_SIZE * sizeof(Node) + (QUEUE_SIZE + 4) * sizeof(int);

    int shmid = shmget(key, alloc_size, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    void *mem = shmat(shmid, NULL, 0);

    Node *nodes = (Node *)mem;

    int numJobs = rand() % 201 + 300; /* Number of initial jobs between 300 and 500 */
    if (__DEBUG) cout << "numJobs = " << numJobs << "\n";
    /* Creation of the base tree */
    for (int i = 0; i < numJobs; i++) {
        nodes[i]._comp_time = 1 + rand() % MAX_COMP_TIME;
        nodes[i]._dep_jobs_freq = 0;
        nodes[i]._parent = NULL;
        nodes[i]._job_id = rand() % MAX_JOB_ID + 1;
        nodes[i]._status = READY;
        pthread_mutex_init(&nodes[i]._mutex_lock, &attr);
    }

    /* Space for the circular queue */
    int *in, *out, *size, *queue, *allOver;
    void *circular_queue = (void *)((char *)mem + MAX_TREE_SIZE * sizeof(Node));
    in = (int *)circular_queue; /* Write end */
    out = ((int *)circular_queue) + 1; /* Read end */
    size = ((int *)circular_queue) + 2;
    allOver = ((int *)circular_queue) + 3;
    queue = ((int *)circular_queue) + 4;
    *in = *out = *size = 0;
    *allOver = 0;
    for (int i = numJobs; i < MAX_TREE_SIZE; i++) {
        nodes[i]._status = UNDEFINED;
        pthread_mutex_init(&nodes[i]._mutex_lock, &attr);
        queue[*in] = i;
        *in = (*in + 1) % QUEUE_SIZE;
        *size = *size + 1;
    }

    /* Create P producer threads and y consumer threads*/
    pthread_t *pT = (pthread_t *)calloc(P, sizeof(pthread_t));
    pthread_t *cT = (pthread_t *)calloc(y, sizeof(pthread_t));
    
    int pid = fork();
    if (pid == 0) {
        /* Process B */
        for (int i = 0; i < y; i++) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_create(&cT[i], &attr, consumer_routine, mem);  
        }

        for (int i = 0; i < y; i++) {
            pthread_join(cT[i], NULL);
        }

        exit(EXIT_SUCCESS);
    }
    else {
        for (int i = 0; i < P; i++) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_create(&pT[i], &attr, producer_routine, mem);
        }
        for (int i = 0; i < P; i++) {
            pthread_join(pT[i], NULL);
        }
        cout << "ALL PRODUCERS HAVE EXITED" << endl;
        *allOver = 1;
        wait(NULL);
    }    
    
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&print);
    pthread_mutex_destroy(&pid_m);
    pthread_mutex_destroy(&cid_m);
    for (int i = 0; i < MAX_TREE_SIZE; i++) {
        pthread_mutex_destroy(&nodes[i]._mutex_lock);
    }
    int err = shmctl(shmid, IPC_RMID, NULL);
    if (err < 0) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    free (pT);
    free (cT);

           
    return 0; 
}
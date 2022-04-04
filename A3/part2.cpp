#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
#include<sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

using namespace std;

#define Q 4
#define N 100

struct block {
    double matrix[N][N];
    int matrixID;
    int status[8];
    int producerNumber;
    int toFill;
};

int main(){


    int np, nw, n;
    cin>>np>>nw>>n;

    int fid=open(".memory",O_CREAT|O_RDWR);

    int id=ftok(".memory",100);    

    int shmid=shmget(id, Q*sizeof(block)+4*sizeof(int)+sizeof(pthread_mutex_t)+sizeof(pthread_mutexattr_t),IPC_CREAT|0666);
     
    if(shmid<0){
        perror("\nError in shared memory allocation\n");
    }
    

    //attaching the shared memory to parent's memory space
    char *mem= (char*)shmat(shmid,NULL,0);

    //the first Q*sizeof(block) is for the queue
    int *jobs_created =(int*) (mem + Q*sizeof(block));

    int *head = jobs_created + 1;
    int *count = head+1;
    int *tail = count + 1;

    //the mutex lock needs to be in the shared memory so that changes made by one process
    //are reflected in the others as well
    pthread_mutex_t *blk = (pthread_mutex_t*) (tail+1); 

    pthread_mutexattr_t *attr=(pthread_mutexattr_t*)(blk+1);

    pthread_mutexattr_init(attr);

    //specifying that we are using mutex for inter-process synchronisation
    pthread_mutexattr_setpshared(attr,PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(blk, attr) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    *jobs_created=0; *head=0; *tail=0;

    block *queue= (block*) mem;
    
    for(int i=0;i<np;i++)
    {
        int x=fork();
        if(x==0){
            time_t t;
            srand((int)time(&t) % getpid());
            int producerNumber=i+1;
            while(true){

                block data;
                
                data.producerNumber=producerNumber;
                for(int i=0;i<8;i++) data.status[i]=0;
                data.toFill=-1;
                for(int i=0;i<N;i++)
                {
                    for(int j=0;j<N;j++)
                        data.matrix[i][j]=(rand()%19)-9;
                }
                usleep((rand()%3000+5)*1000);

                //lock before making changes in the shared memory
                pthread_mutex_lock(blk);

                //enter in the queue only if size is lesser than Q-1,
                //so that there is always space for 1 more job which can be added by the workers
                if(*jobs_created<n&&(*count)<Q-1)
                {
                    
                    cout<<"Job position in the circular queue: "<<*head<<endl;
                    cout<<"Job number: "<<(*jobs_created)+1<<endl;
                    data.matrixID=rand()%100000+1;
                    queue[*head]=data;
                    (*jobs_created)++;
                    cout<<"Producer number: "<<producerNumber<<endl;
                    cout<<"PID: "<<getpid()<<endl;
                    
                    cout<<"Matrix ID: "<<data.matrixID<<endl;
                    
                    cout<<endl;

                    (*count)++; (*head)=((*head)+1)%Q;
                    pthread_mutex_unlock(blk);
                }
                
                else {
                    pthread_mutex_unlock(blk);
                    while(*count>Q-2);  //wait for more space to be freed up
                }
                if(*jobs_created==n) break; 
                
            }

            exit(0);
        }
    }

    for(int i=0;i<nw;i++)
    {
        int x=fork();

        int workerID=i+1;    

        if(x==0){
            time_t t;
            srand((int)time(&t) % getpid());
            while(true){
                //sleep for some time
                usleep((rand()%3000+5)*1000);
                
                //see if the first two jobs are ready to be processed or not
                if((*count)>1&&queue[*tail].status[0]!=-1&&queue[((*tail)+1)%Q].status[0]!=-1){

                    int getStatus=0;
                    pthread_mutex_lock(blk);
                    for(getStatus=0;getStatus<8;getStatus++)
                    if(queue[*tail].status[getStatus]==0) break;

                    if(getStatus!=8){
                        queue[*tail].status[getStatus]++;
                        queue[((*tail)+1)%Q].status[getStatus]++;

                        //the first worker will create a new job and push it into the queue
                        if(getStatus==0){
                            
                            queue[*tail].toFill=*head;
                            queue[((*tail)+1)%Q].toFill=*head;

                            for(int k=0;k<8;k++)
                            queue[*head].status[k]=-1;

                            for(int i=0;i<N;i++)
                            for(int j=0;j<N;j++) queue[*head].matrix[i][j]=0;

                            queue[*head].toFill=-1;

                            queue[*head].matrixID=rand()%100000+1;
                            queue[*head].producerNumber=-1;

                            (*count)++;
                            (*head)=((*head)+1)%Q;
                        }

                        pthread_mutex_unlock(blk);
                        int toFill=queue[*tail].toFill;

                        cout<<endl<<"Worker "<<i<<" multiplying "<<queue[*tail].matrixID<<" and "<<queue[((*tail)+1)%Q].matrixID<<" at status "<<getStatus<<" at position "<<toFill <<endl;


                        int rstart_1, cstart_1,rstart_2, cstart_2, rstart_f, cstart_f;
                        if(getStatus==0)
                        {
                            rstart_1=0;   cstart_1=0;
                            rstart_2=0;   cstart_2=0;
                            rstart_f=0;   cstart_f=0;
                        }

                        else if(getStatus==1){
                            rstart_1=0;   cstart_1=N/2;
                            rstart_2=N/2;   cstart_2=0;
                            rstart_f=0;   cstart_f=0;
                        }

                        else if(getStatus==2){
                            rstart_1=0;   cstart_1=0;
                            rstart_2=0;   cstart_2=N/2;
                            rstart_f=0;   cstart_f=N/2;
                        }

                        else if(getStatus==3){
                            rstart_1=0;   cstart_1=N/2;
                            rstart_2=N/2;   cstart_2=N/2;
                            rstart_f=0;   cstart_f=N/2;
                        }

                        else if(getStatus==4){
                            rstart_1=N/2;   cstart_1=0;
                            rstart_2=0;   cstart_2=0;
                            rstart_f=N/2;   cstart_f=0;
                        }

                        else if(getStatus==5){
                            rstart_1=N/2;   cstart_1=N/2;
                            rstart_2=N/2;   cstart_2=0;
                            rstart_f=N/2;   cstart_f=0;
                        }

                        else if(getStatus==6){
                            rstart_1=N/2;   cstart_1=0;
                            rstart_2=0;   cstart_2=N/2;
                            rstart_f=N/2;   cstart_f=N/2;
                        }

                        else if(getStatus==7){
                            rstart_1=N/2;   cstart_1=N/2;
                            rstart_2=N/2;   cstart_2=N/2;
                            rstart_f=N/2;   cstart_f=N/2;        
                        }

                        for(int i=0;i<N/2;i++)
                        {
                            for(int j=0;j<N/2;j++)
                            {
                                int sum=0;
                                for(int k=0;k<N/2;k++)
                                sum+=queue[*tail].matrix[i+rstart_1][k+cstart_1]*queue[((*tail)+1)%Q].matrix[k+rstart_2][j+cstart_2];
                                pthread_mutex_lock(blk);                                //lock before writing in the memory
                                queue[toFill].matrix[rstart_f+i][cstart_f+j]+=sum;
                                pthread_mutex_unlock(blk);
                            }
                        }

                        queue[*tail].status[getStatus]++;
                        queue[((*tail)+1)%Q].status[getStatus]++;
                    }

                    else{
                        int i;
                        for(i=0;i<8;i++)
                        if(queue[*tail].status[i]!=2) break;
                        if(i==8){
                            int toFill=queue[*tail].toFill;
                            (*count)-=2;
                            (*tail)=((*tail)+2)%Q;
                            for(int i=0;i<8;i++)
                            queue[toFill].status[i]=0;

                            cout<<"\nGenerated matrix "<<queue[toFill].matrixID<<" at position "<<toFill<<endl;
                            
                        }
                    }

                    pthread_mutex_unlock(blk);


                }
                
                if((*jobs_created)==n&&(*count)==1) exit(0);

            }
            exit(0); 
        }
    }


    while(wait(NULL)>0); //wait till all children processes not exited

    pthread_mutexattr_destroy(attr);

    shmdt(mem);
    shmctl(shmid,IPC_RMID,NULL);
}

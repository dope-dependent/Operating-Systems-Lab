/*
    Group 26
    
    Rajas Bhatt (19CS30037)
    Seemant G. Achari (19CS10055)

    part1.cpp

*/
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/wait.h>

const int __DEBUG = 0;
const int __AUTOFILL = 0;

using namespace std;

/* Structure for passing parameters */
struct _process_data{
	double **A;
	double **B;
	double **C;
	int veclen;
	int i;
	int j;
};

/* Multiply a row of matrix A by a column 
of matrix B */
void* mult(void* data){
	_process_data *X = (_process_data*)data;
	double sum = 0;
	for (int i = 0; i < X->veclen; i++) {
        sum += (X->A)[X->i][i] * (X->B)[i][X->j];
    }
	(X->C)[X->i][X->j] = sum;
    return NULL;
}

int main()
{
	srand(time(0));

    int r1, c1, r2, c2;

	cin >> r1 >> c1 >> r2 >> c2;
    
    if (__DEBUG) {
        cout << "r1 : " << r1 << "\n";
        cout << "c1 : " << c1 << "\n";
        cout << "r2 : " << r2 << "\n";
        cout << "c2 : " << c2 << "\n";
    }

    key_t key1 = ftok(".", r1 * 10);
    key_t key2 = ftok(".", r1 * 100);
    if (__DEBUG) {
        cout << "key1 : " << key1 << "\n";
        cout << "key2 : " << key2 << "\n";
    }
    /* mem1 is the id for the combined array of double * */
    /* mem2 is the id for the combined space of A, B and C combined */
    int mem1 = shmget(key1, sizeof(double*) * (2 * r1 + r2), 0666 | IPC_CREAT);
    int mem2 = shmget(key2, sizeof(double) * (r1 * c1 + r2 * c2 + r1 * c2), 0666 | IPC_CREAT);
    if (mem1 < 0){
		perror("mem1");
		exit(EXIT_FAILURE);
	}
    if (mem2 < 0) {
        perror("mem2");
        exit(EXIT_FAILURE);
    }
    if (__DEBUG) {
        cout << "mem1 : " << mem1 << "\n";
        cout << "mem2 : " << mem2 << "\n";
    }
    /* Get pointers to the shared memory */
    double **memp = (double **)shmat(mem1, NULL, 0);    // Pointer to the array of double *
    double *memd = (double *)shmat(mem2, NULL, 0);      // Pointer to the array of doubles used to make A, B and C

    double **A = &memp[0];                      // Takes space from memp[0] to memp[r1 - 1]
    double **B = &memp[r1];                     // Takes space from memp[r1] to memp[r1 + r2 - 1]
    double **C = &memp[r1 + r2];                // Takes space from memp[r1 + r2] to memp[2 * r1 + r2 - 1]
    double *Amem = &memd[0];                    // Takes space from memd[0] to memd[r1 * c1 - 1]
    double *Bmem = &memd[r1 * c1];              // Takes space from memd[r1 * c1] to memd[r1 * c1 + r2 * c2 - 1]
    double *Cmem = &memd[r1 * c1 + r2 * c2];    // Takes space from memd[r1 * c1 + r2 * c2] to memd[r1 * c1 + r2 * c2 + r1 * c2 - 1]
    /* Fill in the matrix A */
    for(int i = 0; i < r1; i++)
    {
        A[i] = &Amem[i * c1];
        for (int j = 0; j < c1; j++) {
            if (!__AUTOFILL) cin >> A[i][j];
            else A[i][j] = rand() % 19 - 9;
        }
    }
    /* Fill in the matrix B */
    for (int i = 0; i < r2; i++)
    {
        B[i] = &Bmem[i * c2];
        for (int j = 0; j < c2; j++) {
            if (!__AUTOFILL) cin >> B[i][j];
            else B[i][j] = rand() % 19 - 9;
        }
    }
    /* Assign pointers to each row of matrix C */
    for (int i = 0; i < r1; i++)
    {
        C[i] = &Cmem[i * c2];
    }
	/* Compute each cell of C by forking new children
    and calculating C[i][j] using the mult function */
	for (int i = 0; i < r1; i++)
	{
		for(int j=0 ; j < c2; j++) 
        {
			int x = fork();
			if (x < 0){
				perror("fork");
				exit(EXIT_FAILURE);
			}
			if (x == 0){
				_process_data data = {A, B, C, c1, i, j}; 
				mult(&data);
				exit(EXIT_SUCCESS);
			}
		}
	}
	/* Wait for all child processes */
	while(wait(NULL)>0);
	
    /* Print the values of C */
	for(int i = 0; i < r1; i++) 
	{
		for(int j = 0; j < c2; j++) cout << C[i][j] << " ";
		cout << endl;
	}	

    /* Free up the shared memory */
    int err;
    err = shmctl(mem1, IPC_RMID, NULL);
    if (err < 0) {
        perror("shmctrl");
        exit(EXIT_FAILURE);
    }
    err = shmctl(mem2, IPC_RMID, NULL);
    if (err < 0) {
        perror("shmctrl");
        exit(EXIT_FAILURE);
    }

	return 0;
}
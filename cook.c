#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

// Define the P and V operations for semaphores
#define P(s) semop(s, &pop, 1)  // P(s) operation
#define V(s) semop(s, &vop, 1)  // V(s) operation

// Define the sembuf structures for P and V operations
struct sembuf pop, vop;

void cmain(int cook_id) {
    // Access shared memory
    key_t key = ftok("/", 1);
    int shmid = shmget(key, 2000 * sizeof(int), 0666);


    int *M = (int*) shmat(shmid, NULL, 0);
    
    // Access semaphores
    key_t mutex_key = ftok("/", 2);
    key_t cook_key = ftok("/", 3);
    key_t waiter_key = ftok("/", 4);
    key_t customer_key = ftok("/", 5);

    int mutex = semget(mutex_key, 1, 0666);
    int cook = semget(cook_key, 1, 0666);
    int waiter = semget(waiter_key, 5, 0666);
    int customer = semget(customer_key, 200, 0666);

    // Initialize the sembuf structures for P and V operations
    pop.sem_num = vop.sem_num = 0;  // Semaphore index (0 for single semaphore)
    pop.sem_flg = vop.sem_flg = 0;  // No flags
    pop.sem_op = -1;  // P(s) operation: decrement semaphore by 1
    vop.sem_op = 1;   // V(s) operation: increment semaphore by 1

    while (1) {
        
        //Wait until woken up by a waiter submitting a cooking request.
        struct sembuf cook_signal = {0, -1, 0};  // V(s) operation
        semop(cook, &cook_signal, 1);

        // Acquire mutex to access shared memory
        P(mutex);

        // Read a cooking request from the cooks' queue
        int front = M[1100];  // Front index of the cooks' queue
        int waiter_id = M[front];  
        int customer_id = M[front + 1];  
        int count_customer = M[front + 2];  

        // Update front of the cooks' queue
        M[1100] = front + 3;

        // Release mutex

        // Simulate preparing the food (5 minutes per person)
        int cooking_time = 5 * count_customer;  // Total cooking time
        printf("Cook %d Preparing food for customer %d (order from waiter %d, count %d) at time %d. Time: %d minutes.\n", cook_id, customer_id, waiter_id, count_customer, M[0],cooking_time);
        int current_time = M[0];
        V(mutex);
        usleep(cooking_time * 100000);  // Scale down to 100 ms per minute

        P(mutex);
        M[0] += cooking_time;
        M[200 * waiter_id + 100] = customer_id; 
        int ready_food = M[0]; 
        V(mutex);  // Release mutex

        // Signal the waiter that food is ready
        struct sembuf waiter_signal = {waiter_id, 1, 0};  // V(s) operation
        semop(waiter, &waiter_signal, 1);
        printf("Cook %d prepared order at time %d(waiter number %d, customer id %d, count %d).\n",cook_id, ready_food,  waiter_id, customer_id,  count_customer);
    }
}

int main(){

    //create shared memory
    int shmid;
    key_t key = ftok("/", 1);
    int *M;

    shmid = shmget(key, 2000 * sizeof(int), IPC_CREAT | 0666);
    M = (int*) shmat(shmid, NULL, 0);

    M[0] = 0;       //time
    M[1] = 10;      //empty tables
    M[2] = 0;       //waiter number
    M[3] = 0;       //orders pending for cook]

    //waiter will be woke by these two
    M[100] = 0;     //FR variable food is ready this is the customer id's food ready
    M[101] = 0;     //PO variable number of customers waiting to place order 

    M[102] = 104;
    M[103] = 104;

    M[300] = 0;
    M[301] = 0;

    M[302] = 304;
    M[303] = 304;

    M[500] = 0;
    M[501] = 0;

    M[502] = 504;
    M[503] = 504;

    M[700] = 0;
    M[701] = 0;

    M[702] = 704;
    M[703] = 704;

    M[900] = 0;
    M[901] = 0;

    M[902] = 904;
    M[903] = 904;

    //cooking queue
    M[1100] = 1102;     //Front of the cooking orders
    M[1101] = 1102;     //Back of the cooking order


    //creating 4 semaphores required 
    key_t mutex_key = ftok("/", 2);
    int mutex = semget(mutex_key, 1, IPC_CREAT | 0666);

    key_t cook_key  = ftok("/", 3);
    int cook = semget(cook_key, 1, IPC_CREAT | 0666);

    key_t waiter_key = ftok("/", 4);
    int waiter = semget(waiter_key, 5, IPC_CREAT | 0666);

    key_t customer_key = ftok("/", 5);
    int customer = semget(mutex_key, 200,  IPC_CREAT | 0666);


    //cinitialising the semaphores
    semctl(mutex, 0, SETVAL, 1);
    semctl(cook, 0, SETVAL, 0);
    for(int i = 0; i < 5; i++){
        semctl(waiter, i, SETVAL, 0);
    }
    for(int i = 0; i < 200; i++){
        semctl(customer, i, SETVAL, 0);
    }



    //create cook proccesses
    pid_t cook1, cook2;

    cook1 = fork();
    if(cook1 == 0) cmain(0);


    cook2 = fork();
    if(cook2 == 0) cmain(1);
    
    while(wait(NULL) > 0);

    return 0;

}
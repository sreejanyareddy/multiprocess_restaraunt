#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define P(s) semop(s, &pop, 1) 
#define V(s) semop(s, &vop, 1)

struct sembuf pop, vop;


void wmain(int waiter_id) {

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

    pop.sem_num = vop.sem_num = 0;  
    pop.sem_flg = vop.sem_flg = 0; 
    pop.sem_op = -1;  
    vop.sem_op = 1;   

    while (1) {
        
        struct sembuf waiter_wait = {waiter_id, -1, 0};  // wait
        semop(waiter, &waiter_wait, 1);

        P(mutex);  // Acquire mutex to access shared memory
        if (M[200 * waiter_id + 100] != 0) {  // FR (food ready) is set
            int customer_id = M[200 * waiter_id + 100];  // Customer ID for whom food is ready
            M[200 * waiter_id + 100] = 0;  // Reset FR

            
            V(mutex);  // Release mutex

            // Signal the customer that food is ready
            struct sembuf customer_signal_getfood = {customer_id, 1, 0};  // V(s) operation
            semop(customer, &customer_signal_getfood, 1);
            printf("Waiter %d serving food to customer %d at time %d.\n",waiter_id, customer_id, M[0]);
        }
        else{
            // Read customer details from the waiter's queue
            int front = M[200 * waiter_id + 102];  // Front index of the waiter's queue
            int customer_id = M[front];  // Customer ID
            int customer_count = M[front + 1];  // Number of individuals

            // Update front of the queue
            M[200 * waiter_id + 102] = front + 2;

            int current_time = M[0];
            V(mutex);
            usleep(100000);  // 1 minute in simulation time
            P(mutex);
            M[0] = current_time + 1;  
            struct sembuf customer_signal_order = {customer_id, 1, 0};       //signal
            semop(customer, &customer_signal_order, 1);
            printf("Waiter %d is taking order of customer %d at time %d\n", waiter_id, customer_id, M[0]);
            V(mutex);  // Release mutex


            // Write the order to the cooks' queue
            P(mutex);  // Acquire mutex to access shared memory
            int back = M[1101];  // Back index of the cooks' queue
            M[back] = waiter_id;  // Waiter ID
            M[back + 1] = customer_id;  // Customer ID
            M[back + 2] = customer_count;  // Customer count
            M[1101] = back + 3;  // Update back index
            V(mutex);  // Release mutex

            // Signal the cook that a new order is available
            struct sembuf cook_signal = {0, 1, 0};  // V(s) operation
            semop(cook, &cook_signal, 1);
        }
    }
}



int main(){
    pid_t waiters[5];
    for(int i = 0; i < 5; i++){
        waiters[i] = fork();
        if(waiters[i] == 0){
            //waiter i process  
            wmain(i);

        }
    }

    while(wait(NULL) > 0);
}
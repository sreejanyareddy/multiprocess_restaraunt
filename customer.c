#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

struct sembuf pop, vop;

void cmain(int customer_id, int arrival_time, int cnt){

    //accessing the shared memory variables
    int shmid;
    key_t key = ftok("/", 1);
    shmid = shmget(key, 2000 * sizeof(int), 0666);
    int *M;
    M = (int *) shmat(shmid, NULL, 0);

    key_t mutex_key = ftok("/", 2);
    int mutex = semget(mutex_key, 1, 0666);

    key_t cook_key  = ftok("/", 3);
    int cook = semget(cook_key, 1, 0666);

    key_t waiter_key = ftok("/", 4);
    int waiter = semget(waiter_key, 5, 0666);

    key_t customer_key = ftok("/", 5);
    int customer = semget(customer_key, 200, 0666);

    //defining the senbuf..
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;    //wait
    vop.sem_op = 1;     //signal


    P(mutex);
    //If the time is after 3:00pm, leave.
    if(arrival_time > 240){
        printf("Customer %d leaves(Late arrival)\n", customer_id);
        V(mutex);
        return;
    }
    
    //If no table is empty, leave.
    if(M[1] == 0){
        printf("Customer %d leaves(No free tables)\n", customer_id);
        V(mutex);
        return;
    }

    //Use an empty table.
    M[1]--;

    //Find the waiter to serve
    int waiter_id = M[2];
    M[2]++;
    M[2] = M[2] %5;

    //Write (customer_ID, customer_cnt) to that waiter’s queue.
    int b = M[200*waiter_id + 103]; //back in the queue
    M[b] = customer_id;
    M[b+1] = cnt;
    M[200*waiter_id + 103] = b+2;

    //Signal the waiter to take the order
    struct sembuf waiter_signal = {waiter_id, 1, 0};        //signal
    semop(waiter, &waiter_signal, 1);
    V(mutex);

    printf("%d\n",semctl(customer, customer_id, GETVAL));
    fflush(stdout);
    //Wait for the waiter to attend (signal from the waiter).
    struct sembuf customer_wait_order = {customer_id, -1, 0};       //wait
    semop(customer, &customer_wait_order, 1);
    printf("%d\n",semctl(customer, customer_id, GETVAL));
    fflush(stdout);
    P(mutex);
    //Place order (no real operation)
    printf("Customer %d placed order to %d at time %d\n", customer_id, waiter_id, M[0]);
    int ordered_time = M[0];
    V(mutex);

    printf("%d\n",semctl(customer, customer_id, GETVAL));
    fflush(stdout);
    //Wait for food to be served (signal from that waiter)
    struct sembuf customer_wait_getfood = {customer_id, -1, 0};
    semop(customer, &customer_wait_getfood, 1);
    int received_time = M[0];
    printf("Customer %d gets the food at time %d,(waiting time = %d)\n", customer_id, M[0], received_time-ordered_time);
    printf("%d\n",semctl(customer, customer_id, GETVAL));
    fflush(stdout);

    //Eat food (this takes 30 minutes irrespective of the count of individuals)
    int current_time = M[0];
    usleep(3000000);
    P(mutex);
    M[0] += 30;       //eats in 30 minutes
    current_time = M[0];
    //Free the table for future customers (if any).
    M[1]++;
    V(mutex);

    //Leave
    printf("Customer %d leaves at time %d\n", customer_id,current_time);


}

int main(){

    FILE *fptr = fopen("customers.txt", "r");
    if(fptr == NULL){
        perror("customers.tt doesn't exist\n");
        return 0;
    }

    int customer_id, arrival_time, cnt;

    while(fscanf(fptr, "%d %d %d", &customer_id, &arrival_time, &cnt) == 3){

        //creating the process for the party of customer
        pid_t pid = fork();
        if(pid == 0){
            cmain(customer_id, arrival_time, cnt);
        }

    }
    fclose(fptr);
    while(wait(NULL) > 0);

    return 0;

}
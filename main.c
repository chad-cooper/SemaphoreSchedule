//  main.c
//
//  Created by Chad Cooper on 11/7/17.
//  Copyright © 2017 Chad Cooper. All rights reserved.
//

#define _REENTRANT
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <dispatch/dispatch.h>

typedef struct Node {
    struct Node *next;
    char direction;
    int crossingTime;
} Baboon;

typedef struct Queue{
    size_t capacity; // the capacity of the buffer, which is 15 in this case
    size_t count;
    Baboon *head;
    Baboon *tail;
    int isRope;
    int isFull;
} Queue;



void node_init(Baboon* baboon, char dir, int crossTime);

// Queue initializer
void q_initialize(Queue *q, size_t capacity, int isRope);

// Add an item to the back fo the queue.
void enqueue(Queue *q, Baboon *bab);

// Remove the head of the queue.
void dequeue(Queue *q);


void *crossLeftToRight(void *void_crossingTime);
void *crossRightToLeft(void *void_crossingTime);
dispatch_semaphore_t rope;

// Establish wait_for_rope queue
Queue wait_for_rope;

// Establish rope entrance/exit queue
Queue crossing;

int NUM_EAST_BBNS = 0, NUM_WEST_BBNS = 0;

int main(int argc, const char * argv[]) {
    
    
    
    if(argc != 3 ){
        printf("Usage: %s filename", argv[0]);
        return 1;
    } else {
        
        q_initialize(&wait_for_rope, 1000, 0);
        q_initialize(&crossing, 3, 1);
        
        int crossingTime = atoi(argv[2]);

        rope = dispatch_semaphore_create(1);
        
        printf("The crossing time is: %d\n", crossingTime);
        
        // We assume argv[1] is a filename to open
        FILE *file = fopen(argv[1], "r" );
        
        /* fopen returns 0, the NULL pointer, on failure */
        if ( file == 0 )
        {
            printf( "Could not open file\n" );
        }
        else
        {
            
            
            int x;
            while  ( ( x = fgetc( file ) ) != EOF )
            {
                if (x == ',') { // ignore commas
                    continue;
                } else if(x == 'L'){
                    NUM_EAST_BBNS++;
                    Baboon* newBaboon = malloc(sizeof(struct Node));
                    node_init(newBaboon, 'E', crossingTime);
                    enqueue(&wait_for_rope, newBaboon);
                } else if(x == 'R'){
                    NUM_WEST_BBNS++;
                    Baboon* newBaboon = malloc(sizeof(struct Node));
                    node_init(newBaboon, 'W', crossingTime);
                    enqueue(&wait_for_rope, newBaboon);
                } else {
                    continue;
                }
                printf( "%c ", x );
            }
            
            fclose( file );
        }
        
        printf("\n");
    
    
        /* UNIX semaphore controls...
         
         sem_t sem1;
         sem_wait(&sem1);
         sem_post(&sem1);
         sem_init(&sem1, 1, value);
         sem_destroy(&sem1);
         ===========================
         OS X semaphore controls....
         
         dispatch_semaphore_t;
         semaphore = dispatch_semaphore_create(1);
         dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
         dispatch_semaphore_signal(semaphore);
         dispatch_release(semaphore);
         */
        
        
        
        // Useful commands for threads
        pthread_t eastBoundBaboons[1]; // process id for thread 1
        pthread_t westBoundBaboons[1]; // process id for thread 2
        pthread_attr_t attr[1]; // attribute pointer array
        
        // Required to schedule thread independently.
        pthread_attr_init(&attr[0]);
        pthread_attr_setscope(&attr[0], PTHREAD_SCOPE_SYSTEM);
        // end to schedule thread independently
        
        // Create the threads
        pthread_create(&eastBoundBaboons[0], &attr[0], crossLeftToRight, &crossingTime);
        pthread_create(&westBoundBaboons[0], &attr[0], crossRightToLeft, &crossingTime);
        
        
        
        // Wait for the threads to finish
        pthread_join(eastBoundBaboons[0], NULL);
        pthread_join(westBoundBaboons[0], NULL);
        
        dispatch_release(rope);
    }
    
    return 0;
}

void *crossLeftToRight(void *void_crossingTime){
    int* crossingTime = (int *)void_crossingTime;
    
    int totalCrossed = 0;
    
    while(totalCrossed < NUM_EAST_BBNS) {
        dispatch_semaphore_wait(rope, DISPATCH_TIME_FOREVER); // Ask to use the rope.
        
        int b = 0; // this will hold the total sleep time for baboons crossing
        
        // while the next baboon in the waiting queue is going in the same direction...
        while(wait_for_rope.head != NULL && wait_for_rope.head->direction == 'E'){
            
            if(crossing.isFull){
                dequeue(&crossing);
                totalCrossed++;
            } else {
                b++;
            }
            
            Baboon* crossingBaboon = malloc(sizeof(struct Node));
            *crossingBaboon = *wait_for_rope.head;
            crossingBaboon->next = NULL;
            enqueue(&crossing, crossingBaboon); // The head of the waiting queue gets onto the rope.
            dequeue(&wait_for_rope); // Dequeue the baboon that just entered the rope.
            
            //b++;
            
            
            printf("There are %zu baboons crossing the rope from Left to right.\n", crossing.count);
        }
        
        totalCrossed += b;
        
        printf("\n");
        
        while(b > 0){
            b--;
            printf("Eastward baboon crossed. %d left.\n", b);
            dequeue(&crossing);
        }
        //sleep(*crossingTime);
        
        dispatch_semaphore_signal(rope); // A baboon going the other way can access the rope.
        
    }
    
    
    pthread_exit(NULL);
}

void *crossRightToLeft(void *void_crossingTime){
    
    int* crossingTime = (int *)void_crossingTime;
    
    int totalCrossed = 0;
    
    while(totalCrossed < NUM_WEST_BBNS) {
        dispatch_semaphore_wait(rope, DISPATCH_TIME_FOREVER); // Ask to use the rope.
        
        int b = 0; // this will hold the total sleep time for baboons crossing
        
        // while the next baboon in the waiting queue is going in the same direction...
        while(wait_for_rope.head != NULL && wait_for_rope.head->direction == 'W'){
            
            if(crossing.isFull){
                dequeue(&crossing);
                totalCrossed++;
            } else {
        
            b++;
            }
            

            Baboon* crossingBaboon = malloc(sizeof(struct Node));
            *crossingBaboon = *wait_for_rope.head;
            crossingBaboon->next = NULL;
            enqueue(&crossing, crossingBaboon); // The head of the waiting queue gets onto the rope.
            
            dequeue(&wait_for_rope); // Dequeue the baboon that just entered the rope.
            
            //b++;

            printf("There are %zu baboons crossing the rope from Right to left.\n", crossing.count);
        }
        
        totalCrossed +=b;
        
        //printf("\n");
        while(b > 0){
            b--;
            printf("Westward baboon crossed. %d left.\n", b);
            dequeue(&crossing);
        }
        
        //sleep(*crossingTime);
        
        dispatch_semaphore_signal(rope); // A baboon going the other way can access the rope.
        
    }
    
    pthread_exit(NULL);
}



void node_init(Baboon* bab, char dir, int crossTime){
    bab->next = NULL;
    bab->direction = dir;
    bab->crossingTime = crossTime;
}

// Queue initializer
void q_initialize(Queue *q, size_t capacity, int isRope){
    q->capacity = capacity;
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
    q->isFull = 0;
    q->isRope = isRope;
}

// Add an item to the back of the queue.
void enqueue(Queue *q, Baboon *bab){
    
    while (q->isFull) {
        sleep(bab->crossingTime); // Wait for a baboon to exit the rope.
        printf("Waiting for baboon to exit rope.\n");
    }
    
    if (q->head == NULL && q->tail == NULL) { // Queue is empty
        q->head = bab;
        q->tail = bab;
    } else {
        q->tail->next = bab;
        q->tail = bab;
    }
    
    q->count++; // increase the queue count.
    if(q->count == q->capacity){
        q->isFull = 1;
    }
}

// Remove the head of the queue.
void dequeue(Queue *q){
    
    Baboon* temp = q->head;
    
    if(q->count-1 <= 0){ // There is only one item in the queue.
        q->head = NULL;
        q->tail = NULL;
    } else {
        q->head = q->head->next;
    }
    
    if(q->isRope){ // When a baboon is moved from waiting line to rope, its memory is not freed.
        sleep(temp->crossingTime);
    }
     free(temp);
    
    q->count--;
    q->isFull = 0;
    
    
}




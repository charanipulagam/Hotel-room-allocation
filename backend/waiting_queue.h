#ifndef WAITING_QUEUE_H
#define WAITING_QUEUE_H

#include "rooms.h"

typedef struct Customer
{
    int customerId;
    char name[50];
    char phone[20];
    char roomType[20];
    int guests;

    struct Customer *next;
} Customer;

typedef struct
{
    Customer *front;
    Customer *rear;
} WaitingQueue;

void initializeQueue(WaitingQueue *queue);

int isQueueEmpty(WaitingQueue *queue);

void enqueue(WaitingQueue *queue,
             int customerId,
             const char name[],
             const char phone[],
             const char roomType[],
             int guests);

Customer* dequeue(WaitingQueue *queue);

Customer* findWaitingCustomer(WaitingQueue *queue,
                              const char roomType[]);

Customer* allocateWaitingCustomer(WaitingQueue *queue,
                                  Room *root);

void displayQueue(WaitingQueue *queue);

void freeQueue(WaitingQueue *queue);

#endif
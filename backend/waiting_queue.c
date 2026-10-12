#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "waiting_queue.h"

void initializeQueue(WaitingQueue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

int isQueueEmpty(WaitingQueue *queue)
{
    return queue->front == NULL;
}

void enqueue(WaitingQueue *queue,
             int customerId,
             const char name[],
             const char phone[],
             const char roomType[],
             int guests)
{
    Customer *newCustomer;

    newCustomer = (Customer*)malloc(sizeof(Customer));

    if (newCustomer == NULL)
    {
        printf("Memory allocation failed.\n");
        return;
    }

    newCustomer->customerId = customerId;

    strcpy(newCustomer->name, name);
    strcpy(newCustomer->phone, phone);
    strcpy(newCustomer->roomType, roomType);

    newCustomer->guests = guests;
    newCustomer->next = NULL;

    if (queue->rear == NULL)
    {
        queue->front = newCustomer;
        queue->rear = newCustomer;
    }
    else
    {
        queue->rear->next = newCustomer;
        queue->rear = newCustomer;
    }
}

Customer* dequeue(WaitingQueue *queue)
{
    Customer *customer;

    if (isQueueEmpty(queue))
    {
        return NULL;
    }

    customer = queue->front;

    queue->front = queue->front->next;

    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }

    customer->next = NULL;

    return customer;
}

Customer* findWaitingCustomer(WaitingQueue *queue,
                              const char roomType[])
{
    Customer *current;

    current = queue->front;

    while (current != NULL)
    {
        if (strcmp(current->roomType, roomType) == 0)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

Customer* allocateWaitingCustomer(WaitingQueue *queue,
                                  Room *root)
{
    Customer *current;
    Customer *previous;
    Customer *allocatedCustomer;
    Room *room;

    current = queue->front;
    previous = NULL;

    while (current != NULL)
    {
        room = NULL;

        /*
         * Search for an available room
         * matching the customer's room type.
         */
        Room *availableRoom = root;

        /*
         * Check all rooms using the BST.
         * displayRooms() cannot be used here because
         * we need to find a matching available room.
         */

        while (availableRoom != NULL)
        {
            if (availableRoom->available == 1 &&
                strcmp(availableRoom->roomType,
                       current->roomType) == 0)
            {
                room = availableRoom;
                break;
            }

            /*
             * Continue searching through the BST.
             */
            if (current->roomType[0] != '\0')
            {
                if (availableRoom->left != NULL)
                {
                    availableRoom = availableRoom->left;
                }
                else if (availableRoom->right != NULL)
                {
                    availableRoom = availableRoom->right;
                }
                else
                {
                    break;
                }
            }
        }

        if (room != NULL)
        {
            allocatedCustomer = current;

            if (previous == NULL)
            {
                queue->front = current->next;
            }
            else
            {
                previous->next = current->next;
            }

            if (queue->rear == current)
            {
                queue->rear = previous;
            }

            allocatedCustomer->next = NULL;

            occupyRoom(root, room->roomNumber);

            printf("\nWaiting customer allocated successfully.\n");
            printf("Customer : %s\n",
                   allocatedCustomer->name);
            printf("Room Type: %s\n",
                   allocatedCustomer->roomType);
            printf("Room     : %d\n",
                   room->roomNumber);

            return allocatedCustomer;
        }

        previous = current;
        current = current->next;
    }

    return NULL;
}

void displayQueue(WaitingQueue *queue)
{
    Customer *current;

    if (isQueueEmpty(queue))
    {
        printf("Waiting queue is empty.\n");
        return;
    }

    current = queue->front;

    while (current != NULL)
    {
        printf("Customer ID: %d | Name: %s | Phone: %s | Room Type: %s | Guests: %d\n",
               current->customerId,
               current->name,
               current->phone,
               current->roomType,
               current->guests);

        current = current->next;
    }
}

void freeQueue(WaitingQueue *queue)
{
    Customer *current;

    while (!isQueueEmpty(queue))
    {
        current = dequeue(queue);
        free(current);
    }
}
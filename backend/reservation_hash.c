#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "reservation_hash.h"

void initializeHashTable(HashTable *hashTable)
{
    int i;

    if (hashTable == NULL)
    {
        return;
    }

    for (i = 0; i < TABLE_SIZE; i++)
    {
        hashTable->table[i] = NULL;
    }
}

/*
    Polynomial hash function.

    Each character of the reservation ID contributes
    to the final hash value.

    Example:
    GS-11708 -> one bucket
    GS-54321 -> another bucket

    The same ID will ALWAYS produce the same bucket.
*/
int hashFunction(const char reservationId[])
{
    unsigned int hash = 0;
    int i = 0;

    if (reservationId == NULL)
    {
        return 0;
    }

    while (reservationId[i] != '\0')
    {
        hash = hash * 31 + (unsigned char)reservationId[i];
        i++;
    }

    return (int)(hash % TABLE_SIZE);
}

int insertReservation(HashTable *hashTable,
                      const char reservationId[],
                      const char customerName[],
                      int roomNumber,
                      int bookingIndex)
{
    int index;
    Reservation *newReservation;

    if (hashTable == NULL ||
        reservationId == NULL ||
        customerName == NULL)
    {
        return -1;
    }

    /*
        Do not allow an empty reservation ID.
    */
    if (reservationId[0] == '\0')
    {
        return -1;
    }

    /*
        Check whether this reservation ID already exists.
    */
    if (searchReservation(hashTable, reservationId) != NULL)
    {
        printf("[HASH] Duplicate reservation ID: %s\n",
               reservationId);

        return 0;
    }

    /*
        Calculate the bucket.
    */
    index = hashFunction(reservationId);

    /*
        Allocate memory for the new hash-table node.
    */
    newReservation =
        (Reservation *)malloc(sizeof(Reservation));

    if (newReservation == NULL)
    {
        printf("[HASH] Memory allocation failed.\n");
        return -1;
    }

    /*
        Copy reservation ID safely.
    */
    strncpy(newReservation->reservationId,
            reservationId,
            sizeof(newReservation->reservationId) - 1);

    newReservation->reservationId[
        sizeof(newReservation->reservationId) - 1
    ] = '\0';

    /*
        Copy customer name safely.
    */
    strncpy(newReservation->customerName,
            customerName,
            sizeof(newReservation->customerName) - 1);

    newReservation->customerName[
        sizeof(newReservation->customerName) - 1
    ] = '\0';

    /*
        Store room number and the index of the
        corresponding Booking structure.
    */
    newReservation->roomNumber = roomNumber;
    newReservation->bookingIndex = bookingIndex;

    /*
        Insert at the beginning of the bucket's
        linked list.
    */
    newReservation->next = hashTable->table[index];

    hashTable->table[index] = newReservation;

    /*
        Debug information.
        This helps verify that the reservation
        really entered the hash table.
    */
    printf("[HASH INSERT] ID=%s | Bucket=%d | Room=%d | BookingIndex=%d\n",
           newReservation->reservationId,
           index,
           newReservation->roomNumber,
           newReservation->bookingIndex);

    return 1;
}

Reservation* searchReservation(HashTable *hashTable,
                                const char reservationId[])
{
    int index;
    Reservation *current;

    if (hashTable == NULL || reservationId == NULL)
    {
        return NULL;
    }

    if (reservationId[0] == '\0')
    {
        return NULL;
    }

    /*
        The SAME hash function used during insertion
        must be used during searching.
    */
    index = hashFunction(reservationId);

    current = hashTable->table[index];

    printf("[HASH SEARCH] ID=%s | Bucket=%d\n",
           reservationId,
           index);

    /*
        Traverse the linked list in this bucket.
    */
    while (current != NULL)
    {
        printf("[HASH CHECK] Stored ID=%s\n",
               current->reservationId);

        if (strcmp(current->reservationId, reservationId) == 0)
        {
            printf("[HASH FOUND] ID=%s | BookingIndex=%d | Room=%d\n",
                   current->reservationId,
                   current->bookingIndex,
                   current->roomNumber);

            return current;
        }

        current = current->next;
    }

    printf("[HASH NOT FOUND] ID=%s\n",
           reservationId);

    return NULL;
}

void displayReservations(HashTable *hashTable)
{
    int i;
    Reservation *current;

    if (hashTable == NULL)
    {
        return;
    }

    printf("\n========== RESERVATION HASH TABLE ==========\n");

    for (i = 0; i < TABLE_SIZE; i++)
    {
        current = hashTable->table[i];

        if (current == NULL)
        {
            continue;
        }

        while (current != NULL)
        {
            printf("Bucket %d | Reservation ID: %s | Customer: %s | Room: %d | Booking Index: %d\n",
                   i,
                   current->reservationId,
                   current->customerName,
                   current->roomNumber,
                   current->bookingIndex);

            current = current->next;
        }
    }

    printf("============================================\n");
}

void freeHashTable(HashTable *hashTable)
{
    int i;
    Reservation *current;
    Reservation *next;

    if (hashTable == NULL)
    {
        return;
    }

    for (i = 0; i < TABLE_SIZE; i++)
    {
        current = hashTable->table[i];

        while (current != NULL)
        {
            next = current->next;

            free(current);

            current = next;
        }

        hashTable->table[i] = NULL;
    }
}
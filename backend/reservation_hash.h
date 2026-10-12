#ifndef RESERVATION_HASH_H
#define RESERVATION_HASH_H

#define TABLE_SIZE 20

/*
    One node of a bucket's linked list (separate chaining).

    bookingIndex is the position of the full reservation record
    inside the Booking array (see reservation.h). The hash table
    is used as a fast INDEX: reservation ID -> position.
*/
typedef struct Reservation
{
    char reservationId[20];
    char customerName[50];
    int roomNumber;
    int bookingIndex;

    struct Reservation *next;
} Reservation;

typedef struct
{
    Reservation *table[TABLE_SIZE];
} HashTable;

void initializeHashTable(HashTable *hashTable);

int hashFunction(const char reservationId[]);

/*
    Returns:
        1  = inserted
        0  = a reservation with this ID already exists
       -1  = memory allocation failed
*/
int insertReservation(HashTable *hashTable,
                      const char reservationId[],
                      const char customerName[],
                      int roomNumber,
                      int bookingIndex);

Reservation* searchReservation(HashTable *hashTable,
                               const char reservationId[]);

void displayReservations(HashTable *hashTable);

void freeHashTable(HashTable *hashTable);

#endif
#ifndef RESERVATION_H
#define RESERVATION_H

#include "rooms.h"

/*
    Maximum number of bookings kept in the Booking array.
    Checked-out bookings stay in the array (active = 0),
    so this is the total number of bookings, not just active ones.
*/
#define MAX_BOOKINGS 200

typedef struct
{
    char reservationId[20];
    char customerName[50];
    char phone[20];
    char email[60];
    char roomType[20];
    char facility[20];
    char checkIn[16];
    char checkOut[16];
    char specialRequest[200];

    int roomNumber;
    int guests;
    int stayDays;
    int active;
} Booking;


/* bookRoom() return values */
#define BOOK_SUCCESS          1
#define BOOK_ROOM_NOT_FOUND  -1
#define BOOK_ROOM_OCCUPIED   -2
#define BOOK_STORAGE_FULL    -3
#define BOOK_INVALID_STAY    -4
#define BOOK_OCCUPY_FAILED   -5

/*
    Books the room described by *request.
    The caller fills in every field except "active".
    The reservations[] array must hold MAX_BOOKINGS entries.
*/
int bookRoom(Room *root,
             Booking reservations[],
             int *reservationCount,
             const Booking *request);


/* checkoutByIndex() return values */
#define CHECKOUT_SUCCESS      1
#define CHECKOUT_NOT_FOUND    0
#define CHECKOUT_NOT_ACTIVE  -1

/* Checks out the booking stored at reservations[index]. */
int checkoutByIndex(Room *root,
                    Booking reservations[],
                    int reservationCount,
                    int index);

/* Older function: finds an active booking by room number (linear search). */
int checkoutRoom(Room *root,
                 Booking reservations[],
                 int reservationCount,
                 int roomNumber);

void displayActiveReservations(Booking reservations[],
                               int reservationCount);

#endif
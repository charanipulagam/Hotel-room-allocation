#include <stdio.h>
#include <string.h>

#include "reservation.h"

int bookRoom(Room *root,
             Booking reservations[],
             int *reservationCount,
             const Booking *request)
{
    Room *room;

    room = searchRoom(root, request->roomNumber);

    if (room == NULL)
    {
        printf("Room does not exist.\n");
        return BOOK_ROOM_NOT_FOUND;
    }

    if (room->available == 0)
    {
        printf("Room %d is already occupied.\n", request->roomNumber);
        return BOOK_ROOM_OCCUPIED;
    }

    if (*reservationCount >= MAX_BOOKINGS)
    {
        printf("Reservation storage is full.\n");
        return BOOK_STORAGE_FULL;
    }

    if (request->stayDays < 1 || request->stayDays > 7)
    {
        printf("Stay must be between 1 and 7 days.\n");
        return BOOK_INVALID_STAY;
    }

    if (!occupyRoom(root, request->roomNumber))
    {
        printf("Unable to occupy room.\n");
        return BOOK_OCCUPY_FAILED;
    }

    reservations[*reservationCount] = *request;
    reservations[*reservationCount].active = 1;

    (*reservationCount)++;

    printf("Reservation successfully created.\n");

    return BOOK_SUCCESS;
}

int checkoutByIndex(Room *root,
                    Booking reservations[],
                    int reservationCount,
                    int index)
{
    if (index < 0 || index >= reservationCount)
    {
        return CHECKOUT_NOT_FOUND;
    }

    if (reservations[index].active == 0)
    {
        return CHECKOUT_NOT_ACTIVE;
    }

    reservations[index].active = 0;

    releaseRoom(root, reservations[index].roomNumber);

    printf("Room %d checked out successfully.\n",
           reservations[index].roomNumber);

    return CHECKOUT_SUCCESS;
}

int checkoutRoom(Room *root,
                 Booking reservations[],
                 int reservationCount,
                 int roomNumber)
{
    int i;

    for (i = 0; i < reservationCount; i++)
    {
        if (reservations[i].roomNumber == roomNumber &&
            reservations[i].active == 1)
        {
            reservations[i].active = 0;

            if (releaseRoom(root, roomNumber))
            {
                printf("Room %d checked out successfully.\n",
                       roomNumber);

                return 1;
            }

            return 0;
        }
    }

    printf("No active reservation found for Room %d.\n",
           roomNumber);

    return 0;
}

void displayActiveReservations(Booking reservations[],
                               int reservationCount)
{
    int i;
    int found = 0;

    for (i = 0; i < reservationCount; i++)
    {
        if (reservations[i].active == 1)
        {
            printf("Reservation ID: %s | Customer: %s | Room: %d | Guests: %d | Stay: %d days\n",
                   reservations[i].reservationId,
                   reservations[i].customerName,
                   reservations[i].roomNumber,
                   reservations[i].guests,
                   reservations[i].stayDays);

            found = 1;
        }
    }

    if (!found)
    {
        printf("No active reservations.\n");
    }
}
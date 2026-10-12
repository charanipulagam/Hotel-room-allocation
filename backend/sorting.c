#include <stdio.h>
#include "sorting.h"

void sortRoomsByPrice(Room rooms[], int count)
{
    int i;
    int j;

    Room temp;

    for (i = 0; i < count - 1; i++)
    {
        for (j = 0; j < count - i - 1; j++)
        {
            if (rooms[j].price > rooms[j + 1].price)
            {
                temp = rooms[j];
                rooms[j] = rooms[j + 1];
                rooms[j + 1] = temp;
            }
        }
    }
}

void displaySortedRooms(Room rooms[], int count)
{
    int i;

    for (i = 0; i < count; i++)
    {
        printf("Room: %d | Type: %s | Facility: %s | Price: %.2f\n",
               rooms[i].roomNumber,
               rooms[i].roomType,
               rooms[i].facility,
               rooms[i].price);
    }
}
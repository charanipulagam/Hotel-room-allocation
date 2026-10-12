#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rooms.h"


// Create a new room node
Room* createRoom(int roomNumber,
                 const char roomType[],
                 const char facility[],
                 float price)
{
    Room *newRoom = (Room*)malloc(sizeof(Room));

    if (newRoom == NULL)
    {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    newRoom->roomNumber = roomNumber;

    strcpy(newRoom->roomType, roomType);
    strcpy(newRoom->facility, facility);

    newRoom->price = price;
    newRoom->available = 1;

    newRoom->left = NULL;
    newRoom->right = NULL;

    return newRoom;
}


// Insert a room into the BST
Room* insertRoom(Room *root,
                 int roomNumber,
                 const char roomType[],
                 const char facility[],
                 float price)
{
    if (root == NULL)
    {
        return createRoom(roomNumber, roomType, facility, price);
    }

    if (roomNumber < root->roomNumber)
    {
        root->left = insertRoom(root->left,
                                roomNumber,
                                roomType,
                                facility,
                                price);
    }
    else if (roomNumber > root->roomNumber)
    {
        root->right = insertRoom(root->right,
                                 roomNumber,
                                 roomType,
                                 facility,
                                 price);
    }

    return root;
}


// Search for a room using room number
Room* searchRoom(Room *root, int roomNumber)
{
    if (root == NULL)
    {
        return NULL;
    }

    if (roomNumber == root->roomNumber)
    {
        return root;
    }

    if (roomNumber < root->roomNumber)
    {
        return searchRoom(root->left, roomNumber);
    }

    return searchRoom(root->right, roomNumber);
}


// Display rooms using inorder traversal
void displayRooms(Room *root)
{
    if (root == NULL)
    {
        return;
    }

    displayRooms(root->left);

    printf("Room: %d | Type: %s | Facility: %s | Price: %.2f | Status: %s\n",
           root->roomNumber,
           root->roomType,
           root->facility,
           root->price,
           root->available ? "Available" : "Occupied");

    displayRooms(root->right);
}


// Occupy a room
int occupyRoom(Room *root, int roomNumber)
{
    Room *room = searchRoom(root, roomNumber);

    if (room == NULL)
    {
        return 0;
    }

    if (room->available == 0)
    {
        return 0;
    }

    room->available = 0;

    return 1;
}


// Release a room
int releaseRoom(Room *root, int roomNumber)
{
    Room *room = searchRoom(root, roomNumber);

    if (room == NULL)
    {
        return 0;
    }

    room->available = 1;

    return 1;
}


// Free all BST nodes
void freeRooms(Room *root)
{
    if (root == NULL)
    {
        return;
    }

    freeRooms(root->left);
    freeRooms(root->right);

    free(root);
}
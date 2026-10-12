#ifndef ROOMS_H
#define ROOMS_H

typedef struct Room {
    int roomNumber;
    char roomType[20];
    char facility[20];
    float price;
    int available;

    struct Room *left;
    struct Room *right;
} Room;

Room* createRoom(int roomNumber,
                 const char roomType[],
                 const char facility[],
                 float price);

Room* insertRoom(Room *root,
                 int roomNumber,
                 const char roomType[],
                 const char facility[],
                 float price);

Room* searchRoom(Room *root, int roomNumber);

void displayRooms(Room *root);

int occupyRoom(Room *root, int roomNumber);

int releaseRoom(Room *root, int roomNumber);

void freeRooms(Room *root);

#endif
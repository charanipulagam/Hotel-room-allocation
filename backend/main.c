#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "rooms.h"
#include "waiting_queue.h"
#include "reservation_hash.h"
#include "sorting.h"
#include "reservation.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

#ifndef _WIN32
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 8192
#define REQUEST_SIZE 16384
#define JSON_SIZE 262144

/*
    ============================================================
                GRANDSTAY HOTEL C BACKEND SERVER
    ============================================================

    Frontend:
        HTML + CSS + JavaScript

                |
                | HTTP
                v

    Backend:
        This C program

                |
        -------------------
        |        |        |
       BST     Queue    Hashing
      Rooms   Waiting  Reservations

    ============================================================
*/


/* ============================================================
   SERVER STATE
   ============================================================ */

static Booking bookings[MAX_BOOKINGS];
static int bookingCount = 0;

static HashTable reservationTable;

static int nextReservationNumber = 10001;

static char jsonBody[JSON_SIZE];


/* ============================================================
   ROOM INITIALIZATION
   ============================================================ */

void initializeRooms(Room **root)
{
    int room;

    /* Floor 1 - Single */
    for (room = 101; room <= 110; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Single",
                           "AC",
                           1500);
    }

    for (room = 111; room <= 120; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Single",
                           "Non-AC",
                           1200);
    }

    /* Floor 2 - Double */
    for (room = 201; room <= 210; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Double",
                           "AC",
                           2200);
    }

    for (room = 211; room <= 220; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Double",
                           "Non-AC",
                           1800);
    }

    /* Floor 3 - Deluxe */
    for (room = 301; room <= 320; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Deluxe",
                           "Premium AC",
                           3000);
    }

    /* Floor 4 - Suite */
    for (room = 401; room <= 420; room++)
    {
        *root = insertRoom(*root,
                           room,
                           "Suite",
                           "Premium AC",
                           4500);
    }
}


/* ============================================================
   SEND DATA
   ============================================================ */

static int sendAll(SOCKET client,
                   const char *data,
                   int length)
{
    int total = 0;
    int sent;

    while (total < length)
    {
        sent = send(client,
                    data + total,
                    length - total,
                    0);

        if (sent <= 0)
        {
            return 0;
        }

        total += sent;
    }

    return 1;
}


/* ============================================================
   SEND HTTP RESPONSE
   ============================================================ */

void sendResponse(SOCKET client,
                  const char contentType[],
                  const char body[])
{
    char header[512];
    int headerLength;
    int bodyLength;

    bodyLength = (int)strlen(body);

    headerLength = snprintf(header,
                            sizeof(header),

                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: %s\r\n"
                            "Access-Control-Allow-Origin: *\r\n"
                            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                            "Access-Control-Allow-Headers: Content-Type\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n"
                            "\r\n",

                            contentType,
                            bodyLength);

    if (sendAll(client, header, headerLength))
    {
        sendAll(client, body, bodyLength);
    }
}


/* ============================================================
   SEND CORS RESPONSE FOR OPTIONS REQUEST
   ============================================================ */

void sendOptionsResponse(SOCKET client)
{
    const char response[] =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Access-Control-Max-Age: 86400\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    send(client,
         response,
         (int)strlen(response),
         0);
}


/* ============================================================
   JSON OUTPUT HELPERS
   ============================================================ */

typedef struct
{
    char *data;
    size_t size;
    size_t length;
} JsonBuf;


static void jsonInit(JsonBuf *jb,
                     char *data,
                     size_t size)
{
    jb->data = data;
    jb->size = size;
    jb->length = 0;
    data[0] = '\0';
}


static void jsonAppend(JsonBuf *jb,
                       const char *format,
                       ...)
{
    va_list args;
    int written;
    size_t space;

    if (jb->length + 1 >= jb->size)
    {
        return;
    }

    space = jb->size - jb->length;

    va_start(args, format);
    written = vsnprintf(jb->data + jb->length,
                        space,
                        format,
                        args);
    va_end(args);

    if (written < 0)
    {
        return;
    }

    if ((size_t)written >= space)
    {
        jb->length = jb->size - 1;
    }
    else
    {
        jb->length += (size_t)written;
    }
}


/* Appends a text value as a quoted JSON string. */
static void jsonAppendString(JsonBuf *jb,
                             const char *text)
{
    const unsigned char *p =
        (const unsigned char *)text;

    jsonAppend(jb, "\"");

    while (*p != '\0')
    {
        if (*p == '"')
        {
            jsonAppend(jb, "\\\"");
        }
        else if (*p == '\\')
        {
            jsonAppend(jb, "\\\\");
        }
        else if (*p == '\n')
        {
            jsonAppend(jb, "\\n");
        }
        else if (*p == '\r')
        {
            jsonAppend(jb, "\\r");
        }
        else if (*p == '\t')
        {
            jsonAppend(jb, "\\t");
        }
        else if (*p < 0x20)
        {
            jsonAppend(jb, "\\u%04x", *p);
        }
        else
        {
            jsonAppend(jb, "%c", *p);
        }

        p++;
    }

    jsonAppend(jb, "\"");
}


/* Appends one booking as a JSON object. */
static void jsonAppendBooking(JsonBuf *jb,
                              const Booking *b)
{
    jsonAppend(jb, "{\"id\":");
    jsonAppendString(jb, b->reservationId);

    jsonAppend(jb, ",\"customer\":");
    jsonAppendString(jb, b->customerName);

    jsonAppend(jb, ",\"phone\":");
    jsonAppendString(jb, b->phone);

    jsonAppend(jb, ",\"email\":");
    jsonAppendString(jb, b->email);

    jsonAppend(jb, ",\"room\":%d", b->roomNumber);

    jsonAppend(jb, ",\"roomType\":");
    jsonAppendString(jb, b->roomType);

    jsonAppend(jb, ",\"facility\":");
    jsonAppendString(jb, b->facility);

    jsonAppend(jb, ",\"guests\":%d", b->guests);

    jsonAppend(jb, ",\"checkIn\":");
    jsonAppendString(jb, b->checkIn);

    jsonAppend(jb, ",\"checkOut\":");
    jsonAppendString(jb, b->checkOut);

    jsonAppend(jb, ",\"stayDays\":%d", b->stayDays);

    jsonAppend(jb, ",\"specialRequest\":");
    jsonAppendString(jb, b->specialRequest);

    jsonAppend(jb, ",\"status\":");
    jsonAppendString(jb,
                     b->active ? "Confirmed" : "Checked Out");

    jsonAppend(jb, "}");
}


/* Sends {"success":false,"message":"..."} */
static void sendJsonError(SOCKET client,
                          const char *message)
{
    JsonBuf jb;
    char text[512];

    jsonInit(&jb, text, sizeof(text));

    jsonAppend(&jb,
               "{\"success\":false,\"message\":");

    jsonAppendString(&jb, message);

    jsonAppend(&jb, "}");

    sendResponse(client,
                 "application/json",
                 jb.data);
}


/* ============================================================
   JSON INPUT HELPERS
   ============================================================ */

static const char *skipSpaces(const char *p)
{
    while (*p == ' ' ||
           *p == '\t' ||
           *p == '\r' ||
           *p == '\n')
    {
        p++;
    }

    return p;
}


static const char *skipJsonString(const char *p)
{
    if (*p != '"')
    {
        return NULL;
    }

    p++;

    while (*p != '\0')
    {
        if (*p == '\\')
        {
            if (p[1] == '\0')
            {
                return NULL;
            }

            p += 2;
        }
        else if (*p == '"')
        {
            return p + 1;
        }
        else
        {
            p++;
        }
    }

    return NULL;
}


static const char *jsonFindValue(const char *json,
                                 const char *key)
{
    const char *p;
    const char *keyStart;
    const char *keyEnd;
    size_t keyLength;

    keyLength = strlen(key);

    p = skipSpaces(json);

    if (*p != '{')
    {
        return NULL;
    }

    p++;

    while (*p != '\0')
    {
        p = skipSpaces(p);

        if (*p == ',')
        {
            p++;
            continue;
        }

        if (*p != '"')
        {
            return NULL;
        }

        keyStart = p + 1;

        p = skipJsonString(p);

        if (p == NULL)
        {
            return NULL;
        }

        keyEnd = p - 1;

        p = skipSpaces(p);

        if (*p != ':')
        {
            return NULL;
        }

        p = skipSpaces(p + 1);

        if ((size_t)(keyEnd - keyStart) == keyLength &&
            strncmp(keyStart, key, keyLength) == 0)
        {
            return p;
        }

        if (*p == '"')
        {
            p = skipJsonString(p);

            if (p == NULL)
            {
                return NULL;
            }
        }
        else
        {
            while (*p != '\0' &&
                   *p != ',' &&
                   *p != '}')
            {
                p++;
            }
        }
    }

    return NULL;
}


static int jsonGetString(const char *json,
                         const char *key,
                         char *out,
                         size_t outSize)
{
    const char *p;
    size_t n = 0;
    char c;
    int k;

    p = jsonFindValue(json, key);

    if (p == NULL || *p != '"')
    {
        return 0;
    }

    p++;

    while (*p != '\0' && *p != '"')
    {
        c = *p;

        if (c == '\\')
        {
            p++;

            if (*p == '\0')
            {
                break;
            }

            switch (*p)
            {
                case 'n':
                    c = '\n';
                    break;

                case 't':
                    c = '\t';
                    break;

                case 'r':
                    c = '\r';
                    break;

                case '"':
                    c = '"';
                    break;

                case '\\':
                    c = '\\';
                    break;

                case '/':
                    c = '/';
                    break;

                case 'u':
                    c = '?';

                    for (k = 0;
                         k < 4 && p[1] != '\0';
                         k++)
                    {
                        p++;
                    }

                    break;

                default:
                    c = *p;
                    break;
            }
        }

        if (n + 1 < outSize)
        {
            out[n++] = c;
        }

        p++;
    }

    out[n] = '\0';

    return 1;
}


static int jsonGetInt(const char *json,
                      const char *key,
                      int *result)
{
    const char *p;
    char *end;
    long value;

    p = jsonFindValue(json, key);

    if (p == NULL)
    {
        return 0;
    }

    if (*p == '"')
    {
        p++;
    }

    value = strtol(p, &end, 10);

    if (end == p)
    {
        return 0;
    }

    *result = (int)value;

    return 1;
}


static int jsonGetRequiredString(const char *json,
                                 const char *key,
                                 char *out,
                                 size_t outSize)
{
    if (!jsonGetString(json,
                       key,
                       out,
                       outSize))
    {
        return 0;
    }

    return out[0] != '\0';
}


/* ============================================================
   DATE HELPERS
   ============================================================ */

static int isLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) ||
           (year % 400 == 0);
}


static int daysInMonth(int year,
                       int month)
{
    static const int days[] =
        {31, 28, 31, 30, 31, 30,
         31, 31, 30, 31, 30, 31};

    if (month == 2 && isLeapYear(year))
    {
        return 29;
    }

    return days[month - 1];
}


static long daysFromCivil(long y,
                          long m,
                          long d)
{
    long era;
    long yoe;
    long doy;
    long doe;

    if (m <= 2)
    {
        y -= 1;
    }

    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2)
          / 5 + d - 1;
    doe = yoe * 365 +
          yoe / 4 -
          yoe / 100 +
          doy;

    return era * 146097 +
           doe -
           719468;
}


static int parseDate(const char *text,
                     long *days)
{
    int year;
    int month;
    int day;
    char extra;

    if (sscanf(text,
               "%d-%d-%d%c",
               &year,
               &month,
               &day,
               &extra) != 3)
    {
        return 0;
    }

    if (year < 2000 ||
        year > 2100 ||
        month < 1 ||
        month > 12 ||
        day < 1 ||
        day > daysInMonth(year, month))
    {
        return 0;
    }

    *days = daysFromCivil(year,
                          month,
                          day);

    return 1;
}


/* ============================================================
   CREATE ROOM JSON
   ============================================================ */

void addRoomsToJson(Room *root,
                    char json[],
                    size_t jsonSize,
                    int *first)
{
    if (root == NULL)
    {
        return;
    }

    /*
        Inorder traversal of BST.
        This gives rooms in increasing room number.
    */

    addRoomsToJson(root->left,
                   json,
                   jsonSize,
                   first);

    {
        char roomJson[300];

        snprintf(roomJson,
                 sizeof(roomJson),

                 "%s"
                 "{\"roomNumber\":%d,"
                 "\"roomType\":\"%s\","
                 "\"facility\":\"%s\","
                 "\"price\":%.2f,"
                 "\"available\":%s}",

                 *first ? "" : ",",

                 root->roomNumber,
                 root->roomType,
                 root->facility,
                 root->price,
                 root->available ? "true" : "false");

        if (strlen(json) +
            strlen(roomJson) + 2 < jsonSize)
        {
            strcat(json, roomJson);
        }

        *first = 0;
    }

    addRoomsToJson(root->right,
                   json,
                   jsonSize,
                   first);
}


/* ============================================================
   HANDLE GET /rooms
   ============================================================ */

void handleRoomsRequest(SOCKET client,
                        Room *root)
{
    char json[BUFFER_SIZE * 4];
    int first = 1;

    strcpy(json, "{\"rooms\":[");

    addRoomsToJson(root,
                   json,
                   sizeof(json),
                   &first);

    strcat(json, "]}");

    sendResponse(client,
                 "application/json",
                 json);
}


/* ============================================================
   HANDLE GET /room/<number>
   ============================================================ */

void handleSingleRoomRequest(SOCKET client,
                             Room *root,
                             int roomNumber)
{
    Room *room;
    char json[500];

    room = searchRoom(root,
                      roomNumber);

    if (room == NULL)
    {
        strcpy(json,
               "{\"success\":false,"
               "\"message\":\"Room not found\"}");

        sendResponse(client,
                     "application/json",
                     json);

        return;
    }

    snprintf(json,
             sizeof(json),

             "{\"success\":true,"
             "\"roomNumber\":%d,"
             "\"roomType\":\"%s\","
             "\"facility\":\"%s\","
             "\"price\":%.2f,"
             "\"available\":%s}",

             room->roomNumber,
             room->roomType,
             room->facility,
             room->price,
             room->available ? "true" : "false");

    sendResponse(client,
                 "application/json",
                 json);
}


/* ============================================================
   HANDLE POST /reserve
   ============================================================ */

void handleReserveRequest(SOCKET client,
                          Room *root,
                          const char *body)
{
    Booking request;
    Room *room;
    JsonBuf jb;

    char checkInText[16];
    char checkOutText[16];

    int roomNumber = 0;
    int guests = 0;

    long checkInDays;
    long checkOutDays;
    long stay;

    int result;
    int index;
    int inserted;

    if (body == NULL ||
        body[0] == '\0')
    {
        sendJsonError(client,
                      "Request body is missing.");
        return;
    }

    memset(&request,
           0,
           sizeof(request));


    /* ---------- customer and reservation details ---------- */

    if (!jsonGetRequiredString(
            body,
            "customerName",
            request.customerName,
            sizeof(request.customerName)) ||

        !jsonGetRequiredString(
            body,
            "phone",
            request.phone,
            sizeof(request.phone)) ||

        !jsonGetRequiredString(
            body,
            "email",
            request.email,
            sizeof(request.email)) ||

        !jsonGetRequiredString(
            body,
            "checkIn",
            checkInText,
            sizeof(checkInText)) ||

        !jsonGetRequiredString(
            body,
            "checkOut",
            checkOutText,
            sizeof(checkOutText)) ||

        !jsonGetInt(body,
                    "roomNumber",
                    &roomNumber) ||

        !jsonGetInt(body,
                    "guests",
                    &guests))
    {
        sendJsonError(
            client,
            "Please fill in all required customer and reservation details.");

        return;
    }


    /* Optional field */

    jsonGetString(body,
                  "specialRequest",
                  request.specialRequest,
                  sizeof(request.specialRequest));


    if (guests < 1)
    {
        sendJsonError(
            client,
            "Please enter a valid number of guests.");

        return;
    }


    /* ---------- dates and 7-day limit ---------- */

    if (!parseDate(checkInText,
                   &checkInDays) ||

        !parseDate(checkOutText,
                   &checkOutDays))
    {
        sendJsonError(
            client,
            "Dates must be valid and written as YYYY-MM-DD.");

        return;
    }

    stay = checkOutDays -
           checkInDays;

    if (stay < 1)
    {
        sendJsonError(
            client,
            "Check-out date must be after check-in date.");

        return;
    }

    if (stay > 7)
    {
        sendJsonError(
            client,
            "Maximum allowed stay is 7 days.");

        return;
    }


    /* ---------- BST ---------- */

    room = searchRoom(root,
                      roomNumber);

    if (room == NULL)
    {
        sendJsonError(client,
                      "Room not found.");

        return;
    }

    if (!room->available)
    {
        sendJsonError(
            client,
            "This room is already occupied. Please select another available room.");

        return;
    }


    /* ---------- build booking ---------- */

    snprintf(request.roomType,
             sizeof(request.roomType),
             "%s",
             room->roomType);

    snprintf(request.facility,
             sizeof(request.facility),
             "%s",
             room->facility);

    snprintf(request.checkIn,
             sizeof(request.checkIn),
             "%s",
             checkInText);

    snprintf(request.checkOut,
             sizeof(request.checkOut),
             "%s",
             checkOutText);

    snprintf(request.reservationId,
             sizeof(request.reservationId),
             "GS-%d",
             nextReservationNumber);

    request.roomNumber = roomNumber;
    request.guests = guests;
    request.stayDays = (int)stay;


    /* ---------- book room ---------- */

    result = bookRoom(root,
                      bookings,
                      &bookingCount,
                      &request);

    if (result != BOOK_SUCCESS)
    {
        if (result == BOOK_ROOM_OCCUPIED)
        {
            sendJsonError(
                client,
                "This room is already occupied. Please select another available room.");
        }
        else if (result == BOOK_STORAGE_FULL)
        {
            sendJsonError(
                client,
                "Reservation storage is full.");
        }
        else
        {
            sendJsonError(
                client,
                "The reservation could not be created.");
        }

        return;
    }

    index = bookingCount - 1;


    /* ---------- hash table ---------- */

    inserted = insertReservation(
        &reservationTable,
        request.reservationId,
        request.customerName,
        roomNumber,
        index);

    if (inserted != 1)
    {
        bookingCount--;

        releaseRoom(root,
                    roomNumber);

        sendJsonError(
            client,
            "The reservation could not be stored.");

        return;
    }

    nextReservationNumber++;

    printf(
        "Reservation %s created: Room %d for %s (%d days)\n",
        request.reservationId,
        roomNumber,
        request.customerName,
        (int)stay);

    printf(
        "Stored in hash bucket %d\n",
        hashFunction(request.reservationId));


    /* ---------- reply ---------- */

    jsonInit(&jb,
             jsonBody,
             sizeof(jsonBody));

    jsonAppend(
        &jb,
        "{\"success\":true,\"reservation\":");

    jsonAppendBooking(
        &jb,
        &bookings[index]);

    jsonAppend(&jb,
               "}");

    sendResponse(client,
                 "application/json",
                 jb.data);
}


/* ============================================================
   HANDLE POST /checkout
   ============================================================ */

void handleCheckoutRequest(SOCKET client,
                           Room *root,
                           const char *body)
{
    char reservationId[20];
    Reservation *found;
    JsonBuf jb;
    int result;

    if (body == NULL ||
        !jsonGetRequiredString(
            body,
            "reservationId",
            reservationId,
            sizeof(reservationId)))
    {
        sendJsonError(
            client,
            "Reservation ID is required.");

        return;
    }

    found = searchReservation(
        &reservationTable,
        reservationId);

    if (found == NULL)
    {
        sendJsonError(
            client,
            "Reservation could not be found.");

        return;
    }

    result = checkoutByIndex(
        root,
        bookings,
        bookingCount,
        found->bookingIndex);

    if (result == CHECKOUT_NOT_ACTIVE)
    {
        sendJsonError(
            client,
            "This reservation is not currently active.");

        return;
    }

    if (result != CHECKOUT_SUCCESS)
    {
        sendJsonError(
            client,
            "Checkout failed.");

        return;
    }

    jsonInit(&jb,
             jsonBody,
             sizeof(jsonBody));

    jsonAppend(
        &jb,
        "{\"success\":true,"
        "\"message\":\"Checkout completed successfully.\","
        "\"reservation\":");

    jsonAppendBooking(
        &jb,
        &bookings[found->bookingIndex]);

    jsonAppend(&jb,
               "}");

    sendResponse(client,
                 "application/json",
                 jb.data);
}


/* ============================================================
   HANDLE GET /reservation/<id>
   ============================================================ */

void handleReservationLookup(
    SOCKET client,
    const char *reservationId)
{
    Reservation *found;
    JsonBuf jb;

    found = searchReservation(
        &reservationTable,
        reservationId);

    if (found == NULL)
    {
        sendJsonError(
            client,
            "Reservation not found");

        return;
    }

    jsonInit(&jb,
             jsonBody,
             sizeof(jsonBody));

    jsonAppend(
        &jb,
        "{\"success\":true,\"bucket\":%d,\"reservation\":",
        hashFunction(reservationId));

    jsonAppendBooking(
        &jb,
        &bookings[found->bookingIndex]);

    jsonAppend(&jb,
               "}");

    sendResponse(client,
                 "application/json",
                 jb.data);
}


/* ============================================================
   HANDLE GET /reservations
   ============================================================ */

void handleReservationsList(SOCKET client)
{
    JsonBuf jb;
    int i;

    jsonInit(&jb,
             jsonBody,
             sizeof(jsonBody));

    jsonAppend(
        &jb,
        "{\"success\":true,\"count\":%d,\"reservations\":[",
        bookingCount);

    for (i = 0;
         i < bookingCount;
         i++)
    {
        if (i > 0)
        {
            jsonAppend(&jb, ",");
        }

        jsonAppendBooking(
            &jb,
            &bookings[i]);
    }

    jsonAppend(&jb,
               "]}");

    sendResponse(client,
                 "application/json",
                 jb.data);
}


/* ============================================================
   HANDLE UNKNOWN REQUEST
   ============================================================ */

void handleUnknownRequest(SOCKET client)
{
    const char json[] =
        "{"
        "\"success\":false,"
        "\"message\":\"Unknown endpoint\""
        "}";

    sendResponse(client,
                 "application/json",
                 json);
}


/* ============================================================
   READ CONTENT LENGTH
   ============================================================ */

static int parseContentLength(
    const char *headers,
    int headerLength)
{
    const char name[] = "content-length:";
    int nameLength =
        (int)strlen(name);

    int i;
    int k;
    char c;

    for (i = 0;
         i + nameLength < headerLength;
         i++)
    {
        for (k = 0;
             k < nameLength;
             k++)
        {
            c = headers[i + k];

            if (c >= 'A' &&
                c <= 'Z')
            {
                c = (char)(c + 32);
            }

            if (c != name[k])
            {
                break;
            }
        }

        if (k == nameLength)
        {
            return atoi(
                headers +
                i +
                nameLength);
        }
    }

    return 0;
}


/* ============================================================
   READ FULL HTTP REQUEST
   ============================================================ */

static int readRequest(
    SOCKET client,
    char *buffer,
    int bufferSize)
{
    int total = 0;
    int received;
    int headerEnd = -1;
    int contentLength = 0;
    char *end;

    while (total < bufferSize - 1)
    {
        received = recv(
            client,
            buffer + total,
            bufferSize - 1 - total,
            0);

        if (received <= 0)
        {
            break;
        }

        total += received;

        buffer[total] = '\0';

        if (headerEnd < 0)
        {
            end = strstr(
                buffer,
                "\r\n\r\n");

            if (end != NULL)
            {
                headerEnd =
                    (int)(end - buffer) + 4;

                contentLength =
                    parseContentLength(
                        buffer,
                        headerEnd);
            }
        }

        if (headerEnd >= 0 &&
            total >=
                headerEnd +
                contentLength)
        {
            break;
        }
    }

    buffer[total] = '\0';

    return total;
}


static int startsWith(
    const char *text,
    const char *prefix)
{
    return strncmp(
               text,
               prefix,
               strlen(prefix)) == 0;
}


/* ============================================================
   PROCESS HTTP REQUEST
   ============================================================ */

void processRequest(
    SOCKET client,
    Room *root)
{
    static char buffer[REQUEST_SIZE];

    char requestLine[120];

    const char *body;

    int received;
    int i;

    received = readRequest(
        client,
        buffer,
        sizeof(buffer));

    if (received <= 0)
    {
        return;
    }


    /* Print only first request line. */

    i = 0;

    while (buffer[i] != '\0' &&
           buffer[i] != '\r' &&
           buffer[i] != '\n' &&
           i < (int)sizeof(requestLine) - 1)
    {
        requestLine[i] = buffer[i];
        i++;
    }

    requestLine[i] = '\0';

    printf("[request] %s\n",
           requestLine);


    /*
        Browser sends OPTIONS before some POST requests.
    */

    if (startsWith(buffer,
                   "OPTIONS"))
    {
        sendOptionsResponse(client);

        return;
    }


    /* Start of request body. */

    body = strstr(
        buffer,
        "\r\n\r\n");

    if (body != NULL)
    {
        body += 4;
    }


    /*
        GET /rooms
    */

    if (startsWith(
            buffer,
            "GET /rooms "))
    {
        handleRoomsRequest(
            client,
            root);

        return;
    }


    /*
        GET /room/101
    */

    if (startsWith(
            buffer,
            "GET /room/"))
    {
        int roomNumber;

        if (sscanf(
                buffer + 10,
                "%d",
                &roomNumber) == 1)
        {
            handleSingleRoomRequest(
                client,
                root,
                roomNumber);

            return;
        }
    }


    /*
        POST /reserve
    */

    if (startsWith(
            buffer,
            "POST /reserve "))
    {
        handleReserveRequest(
            client,
            root,
            body);

        return;
    }


    /*
        POST /checkout
    */

    if (startsWith(
            buffer,
            "POST /checkout "))
    {
        handleCheckoutRequest(
            client,
            root,
            body);

        return;
    }


    /*
        GET /reservations
    */

    if (startsWith(
            buffer,
            "GET /reservations "))
    {
        handleReservationsList(client);

        return;
    }


    /*
        GET /reservation/GS-10001
    */

    if (startsWith(
            buffer,
            "GET /reservation/"))
    {
        char reservationId[20];

        const char *p =
            buffer +
            strlen("GET /reservation/");

        size_t n = 0;

        while (*p != '\0' &&
               *p != ' ' &&
               *p != '?' &&
               *p != '\r' &&
               n < sizeof(reservationId) - 1)
        {
            reservationId[n++] = *p;
            p++;
        }

        reservationId[n] = '\0';

        handleReservationLookup(
            client,
            reservationId);

        return;
    }


    handleUnknownRequest(client);
}


/* ============================================================
   START SERVER
   ============================================================ */

int main()
{
    int port = DEFAULT_PORT;

#ifdef _WIN32

    WSADATA wsaData;

#endif

    SOCKET serverSocket;
    SOCKET clientSocket;

    struct sockaddr_in serverAddress;

#ifdef _WIN32

    int addressLength;

#else

    socklen_t addressLength;

#endif

    Room *root = NULL;


    /* ========================================================
       RENDER PORT SUPPORT

       Render provides PORT as an environment variable.
       Locally, the server continues using port 8080.
       ======================================================== */

#ifndef _WIN32

    {
        const char *envPort =
            getenv("PORT");

        if (envPort != NULL &&
            envPort[0] != '\0')
        {
            port = atoi(envPort);
        }
    }

#endif


    /* ========================================================
       INITIALIZE ROOMS AND HASH TABLE
       ======================================================== */

    initializeRooms(&root);

    initializeHashTable(
        &reservationTable);

    printf("========================================\n");
    printf("       GRANDSTAY HOTEL BACKEND\n");
    printf("========================================\n");

    printf("\n80 hotel rooms initialized successfully.\n");


    /* ========================================================
       INITIALIZE WINDOWS SOCKET
       ======================================================== */

#ifdef _WIN32

    if (WSAStartup(
            MAKEWORD(2, 2),
            &wsaData) != 0)
    {
        printf(
            "Winsock initialization failed.\n");

        freeRooms(root);

        return 1;
    }

#endif


    /* ========================================================
       CREATE SERVER SOCKET
       ======================================================== */

    serverSocket =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (serverSocket ==
        INVALID_SOCKET)
    {
        printf(
            "Unable to create server socket.\n");

#ifdef _WIN32

        WSACleanup();

#endif

        freeRooms(root);

        return 1;
    }


    /* ========================================================
       SERVER ADDRESS
       ======================================================== */

    memset(
        &serverAddress,
        0,
        sizeof(serverAddress));

    serverAddress.sin_family =
        AF_INET;

    /*
        INADDR_ANY means:
        - localhost when running locally
        - all network interfaces when running on Render
    */

    serverAddress.sin_addr.s_addr =
        htonl(INADDR_ANY);

    serverAddress.sin_port =
        htons((unsigned short)port);


    /* ========================================================
       BIND SOCKET
       ======================================================== */

    if (bind(
            serverSocket,
            (struct sockaddr*)&serverAddress,
            sizeof(serverAddress)) ==
        SOCKET_ERROR)
    {
        printf(
            "Unable to bind server to port %d.\n",
            port);

        closesocket(serverSocket);

#ifdef _WIN32

        WSACleanup();

#endif

        freeRooms(root);

        return 1;
    }


    /* ========================================================
       LISTEN
       ======================================================== */

    if (listen(
            serverSocket,
            10) == SOCKET_ERROR)
    {
        printf(
            "Unable to start listening.\n");

        closesocket(serverSocket);

#ifdef _WIN32

        WSACleanup();

#endif

        freeRooms(root);

        return 1;
    }


    printf("\n========================================\n");
    printf("SERVER STARTED SUCCESSFULLY\n");
    printf("========================================\n");

    printf("\nBackend URL:\n");
    printf("http://localhost:%d\n",
           port);

    printf("\nAvailable endpoints:\n");
    printf("GET  /rooms\n");
    printf("GET  /room/<number>\n");
    printf("POST /reserve\n");
    printf("POST /checkout\n");
    printf("GET  /reservations\n");
    printf("GET  /reservation/<id>\n");

    printf("\nWaiting for frontend requests...\n");


    /* ========================================================
       CONTINUOUS SERVER LOOP
       ======================================================== */

    while (1)
    {
        addressLength =
            (int)sizeof(serverAddress);

        clientSocket =
            accept(
                serverSocket,
                (struct sockaddr*)&serverAddress,
                &addressLength);

        if (clientSocket ==
            INVALID_SOCKET)
        {
            printf(
                "Unable to accept connection.\n");

            continue;
        }


        /*
            Process browser request.
        */

        processRequest(
            clientSocket,
            root);


        closesocket(clientSocket);
    }


    /* ========================================================
       CLEANUP
       ======================================================== */

    closesocket(serverSocket);

#ifdef _WIN32

    WSACleanup();

#endif

    freeHashTable(
        &reservationTable);

    freeRooms(root);

    return 0;
}
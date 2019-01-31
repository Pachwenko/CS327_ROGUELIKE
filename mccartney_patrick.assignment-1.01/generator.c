#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for uint8_t, etc
#include <math.h>
#include <time.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'

#define DUNGEONCOLS 80
#define DUNGEONROWS 21
#define NUMROOMS 8
#define ROOMDATA 4
#define MINROOMWIDTH 4
#define MINROOMHEIGHT 3
#define ROOM_SIZE_X 0
#define ROOM_SIZE_Y 1
#define ROOM_POS_X 2
#define ROOM_POS_Y 3

struct material {
    char value;
    uint8_t hardness;
};

void printDungeon(struct material **dungeon);
struct material **allocateDungeon();
void drawBlank(char** dungeon);
void freeDungeon(struct material **dungeon);
void createRooms(struct material **dungeon, int rooms[NUMROOMS][4]);
int isImmuteable(char value);
int isValid(char value);
int canPlaceRoom(struct material **dungeon, int x, int y, int width, int height);
void drawRoom(struct material **dungeon, int x, int y, int width, int height);
void createCooridors(struct material **dungeon, int roomNum, int rooms[NUMROOMS][ROOMDATA]);
void placeStairs(struct material **dungeon, int rooms[NUMROOMS][ROOMDATA]);

int main(int argc, char *argv[])
{
    srand(time(NULL));
    struct material **dungeon = allocateDungeon();
    drawBlank(dungeon);
    int rooms[NUMROOMS][ROOMDATA];
    createRooms(dungeon, rooms);
    createCooridors(dungeon, 0, rooms);
    placeStairs(dungeon, rooms);

    printDungeon(dungeon);
    freeDungeon(dungeon);
    return 0;
}

struct material **allocateDungeon()
{
    struct material **dungeon;
    //allocate pointers to the rows of our dungeon
    dungeon = malloc(DUNGEONROWS * sizeof(struct material *));

    if (dungeon == NULL)
    {
        fprintf(stderr, "Dungeon is null - out of memory\n");
        exit(0);
    }

    //now allocate enough columns for each row
    int i = 0;
    for (i = 0; i < DUNGEONROWS; i++)
    {
        dungeon[i] = malloc(DUNGEONCOLS * sizeof(char *));
        if (dungeon[i] == NULL)
        {
            fprintf(stderr, "Out of memory - allocating dungeolCols");
            exit(0);
        }
    }
    return dungeon;
}

void drawBlank(char** dungeon) {
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++)
    {
        for (col = 0; col < DUNGEONCOLS; col++)
        {
            dungeon[row][col] = ' ';
        }
    }
    for (row = 0; row < DUNGEONROWS; row++)
    {
        dungeon[row][0] = '|';
        dungeon[row][DUNGEONCOLS - 1] = '|';
    }
    for (col = 0; col < DUNGEONCOLS; col++)
    {
        dungeon[0][col] = '-';
        dungeon[DUNGEONROWS - 1][col] = '-';
    }
}

void printDungeon(struct material **dungeon)
{
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++)
    {
        for (col = 0; col < DUNGEONCOLS; col++)
        {
            printf("%c", dungeon[row][col]);
        }
        printf("\n");
    }
}

void freeDungeon(struct material **dungeon)
{
    int i = 0;
    for (i = 0; i < DUNGEONROWS; i++)
    {
        free(dungeon[i]);
    }
    free(dungeon);
}

int isImmuteable(char value)
{
    if (value == '-' || value == '|')
    {
        return 1;
    }
    return 0;
}

int isValid(char value)
{
    if (isImmuteable(value) || value == '.' || value == '#' || value == '<' || value == '>')
    {
        return 0;
    }
    return 1;
}

int canPlaceRoom(struct material **dungeon, int x, int y, int width, int height)
{
    if ((x + width + 1) > DUNGEONCOLS || (y + height + 1) > DUNGEONROWS)
    {
        return 0;
    }
    int row, col = 0;
    for (col = x; col < x + width; col++)
    {
        for (row = y; row < y + height; row++)
        {
            if (!(isValid(dungeon[row-1][col-1]) && isValid(dungeon[row][col-1])
                && isValid(dungeon[row-1][col]) && isValid(dungeon[row][col])
                && isValid(dungeon[row+1][col]) && isValid(dungeon[row][col+1])
                && isValid(dungeon[row+1][col+1])))
            {
                return 0;
            }
        }
    }
    return 1;
}

void createRooms(struct material **dungeon, int rooms[NUMROOMS][4])
{
    int numRooms = 0;
    while (numRooms < NUMROOMS)
    {
        int width = MINROOMWIDTH + (rand() % 7 + 1);
        int height = MINROOMHEIGHT + (rand() % 7 + 1);

        int x = (rand() % DUNGEONCOLS) + 1;
        int y = (rand() % DUNGEONROWS) + 1;
        if (canPlaceRoom(dungeon, x, y, width, height))
        {
            rooms[numRooms][ROOM_POS_X] = x;
            rooms[numRooms][ROOM_POS_Y] = y;
            rooms[numRooms][ROOM_SIZE_X] = width;
            rooms[numRooms][ROOM_SIZE_Y] = height;
            numRooms++;

            // could be refactored to draw all at once instead of one at a time
            drawRoom(dungeon, x, y, width, height);
        }
    }
}

void drawRoom(struct material **dungeon, int x, int y, int width, int height)
{
    // Attempting to follow the column first standard
    int col, row = 0;
    for (col = x; col < x + width; col++)
    {
        for (row = y; row < y + height; row++)
        {
            dungeon[row][col] = '.';
        }
    }
}

    /*
    A better implementation could keep track of which rooms were connected along the way
    which would require checking which '.' belongs to which room and only connecting that room once
    */
void createCooridors(struct material **dungeon, int roomNum, int rooms[NUMROOMS][ROOMDATA]) {
    // basic idea is to keep adding to x using the provided formula
    // when the origin is equal to the destination then go to
    // doing the same for the y values. Also, don't overrite rooms
    int x = rooms[roomNum][ROOM_POS_X];
    int y = rooms[roomNum][ROOM_POS_Y];

    int x1 = rooms[roomNum+1][ROOM_POS_X];
    int y1 = rooms[roomNum+1][ROOM_POS_Y];

    while (x != x1) {
        x += (x1-x) / abs(x1-x);
        if ((isValid(dungeon[y][x]))) {
            dungeon[y][x] = '#';
        }
    }

    while (y != y1) {
        y += (y1-y) / abs(y1-y);
        if ((isValid(dungeon[y][x]))) {
            dungeon[y][x] = '#';
        }
    }

    if (roomNum < NUMROOMS - 2) {
        createCooridors(dungeon, roomNum+1, rooms);
    }
}

void placeStairs(struct material **dungeon, int rooms[NUMROOMS][ROOMDATA]) {
    int upX = (rand() % rooms[0][ROOM_SIZE_X]) + rooms[0][ROOM_POS_X];
    int upY = (rand() % rooms[0][ROOM_SIZE_Y]) + rooms[0][ROOM_POS_Y];
    dungeon[upY][upX] = '<';

    int downX = (rand() % rooms[NUMROOMS-1][ROOM_SIZE_X]) + rooms[NUMROOMS-1][ROOM_POS_X];
    int downY = (rand() % rooms[NUMROOMS-1][ROOM_SIZE_Y]) + rooms[NUMROOMS-1][ROOM_POS_Y];
    dungeon[downY][downX] = '>';
}
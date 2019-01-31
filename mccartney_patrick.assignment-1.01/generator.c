#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for uint8_t, etc
#include <math.h>
#include <time.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'

#define DUNGEONCOLS 80
#define DUNGEONROWS 21
#define NUMROOMS 8 // this is the maximum number of rooms
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

struct room {
    //1 byte each, 4 bytes total and in order they go xpos, ypos, width, height
    uint8_t xpos;
    uint8_t ypos;
    uint8_t width;
    uint8_t height;
    uint8_t wasConnected; // rememnber not to write this to save file, this is only for generation
};

void printDungeon(struct material **dungeon);
struct material **allocateDungeon();
void drawBlank(struct material **dungeon);
void freeDungeon(struct material **dungeon);
void createRooms(struct material **dungeon, struct room rooms[NUMROOMS]);
int isImmuteable(char value);
int isValid(char value);
int canPlaceRoom(struct material **dungeon, int x, int y, int width, int height);
void drawRoom(struct material **dungeon, int x, int y, int width, int height);
void createCooridors(struct material **dungeon, int roomNum, struct room rooms[NUMROOMS]);
void placeStairs(struct material **dungeon, struct room rooms[NUMROOMS]);

int main(int argc, char *argv[])
{
    time_t seconds = time(0);
    printf("seed: %ld\n", seconds);
    srand(seconds);
    struct material **dungeon = allocateDungeon();
    drawBlank(dungeon);
    struct room rooms[NUMROOMS];
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
        dungeon[i] = malloc(DUNGEONCOLS * sizeof(struct material *));
        if (dungeon[i] == NULL)
        {
            fprintf(stderr, "Out of memory - allocating dungeolCols");
            exit(0);
        }
    }
    return dungeon;
}

void drawBlank(struct material **dungeon) {
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++)
    {
        for (col = 0; col < DUNGEONCOLS; col++)
        {
            dungeon[row][col].value = ' ';
        }
    }
    for (row = 0; row < DUNGEONROWS; row++)
    {
        dungeon[row][0].value = '|';
        dungeon[row][DUNGEONCOLS - 1].value = '|';
    }
    for (col = 0; col < DUNGEONCOLS; col++)
    {
        dungeon[0][col].value = '-';
        dungeon[DUNGEONROWS - 1][col].value = '-';
    }
}

void printDungeon(struct material **dungeon)
{
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++)
    {
        for (col = 0; col < DUNGEONCOLS; col++)
        {
            printf("%c", dungeon[row][col].value);
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
            if (!(isValid(dungeon[row-1][col-1].value) && isValid(dungeon[row][col-1].value)
                && isValid(dungeon[row-1][col].value) && isValid(dungeon[row][col].value)
                && isValid(dungeon[row+1][col].value) && isValid(dungeon[row][col+1].value)
                && isValid(dungeon[row+1][col+1].value)))
            {
                return 0;
            }
        }
    }
    return 1;
}

void createRooms(struct material **dungeon, struct room rooms[NUMROOMS])
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
            rooms[numRooms].xpos = x;
            rooms[numRooms].ypos = y;
            rooms[numRooms].width = width;
            rooms[numRooms].height = height;
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
            dungeon[row][col].value = '.';
        }
    }
}

    /*
    A better implementation could keep track of which rooms were connected along the way
    which would require checking which '.' belongs to which room and only connecting that room once

    also adding a bit of randomness would be cool
    */
void createCooridors(struct material **dungeon, int roomNum, struct room rooms[NUMROOMS]) {
    // basic idea is to keep adding to x using the provided formula
    // when the origin is equal to the destination then go to
    // doing the same for the y values. Also, don't overrite rooms
    int x = rooms[roomNum].xpos;
    int y = rooms[roomNum].ypos;

    int x1 = rooms[roomNum+1].xpos;
    int y1 = rooms[roomNum+1].ypos;

    while (x != x1) {
        x += (x1-x) / abs(x1-x);
        if ((isValid(dungeon[y][x].value))) {
            dungeon[y][x].value = '#';
        }
    }

    while (y != y1) {
        y += (y1-y) / abs(y1-y);
        if ((isValid(dungeon[y][x].value))) {
            dungeon[y][x].value = '#';
        }
    }

    if (roomNum < NUMROOMS - 2) {
        createCooridors(dungeon, roomNum+1, rooms);
    }
}

void placeStairs(struct material **dungeon, struct room rooms[NUMROOMS]) {
    uint8_t upX = (rand() % rooms[0].width) + rooms[0].xpos;
    uint8_t upY = (rand() % rooms[0].height) + rooms[0].ypos;
    dungeon[upY][upX].value = '<';

    uint8_t downX = (rand() % rooms[NUMROOMS-1].width) + rooms[NUMROOMS-1].xpos;
    uint8_t downY = (rand() % rooms[NUMROOMS-1].height) + rooms[NUMROOMS-1].ypos;
    dungeon[downY][downX].value = '>';
}
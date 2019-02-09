#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for uint8_t, etc
#include <math.h>
#include <time.h>

#define DUNGEONCOLS 80
#define DUNGEONROWS 21
#define MAXROOMS 8 // this is the maximum number of rooms
#define ROOMDATA 4
#define MINROOMWIDTH 4
#define MINROOMHEIGHT 3
#define ROOM_SIZE_X 0
#define ROOM_SIZE_Y 1
#define ROOM_POS_X 2
#define ROOM_POS_Y 3
#define NUMSTAIRS 2

struct material
{
    char value;
    uint8_t hardness;
};

struct room
{
    //1 byte each, 4 bytes total and in order they go xpos, ypos, width, height
    uint8_t xpos;
    uint8_t ypos;
    uint8_t width;
    uint8_t height;
};

struct stair
{
    uint8_t xpos;
    uint8_t ypos;
};

void printDungeon(struct material **dungeon);
struct material **allocateDungeon();
void drawBlank(struct material **dungeon);
void freeDungeon(struct material **dungeon);
void createRooms(struct material **dungeon, struct room rooms[MAXROOMS]);
int isImmuteable(char value);
int isValid(char value);
int canPlaceRoom(struct material **dungeon, int x, int y, int width, int height);
void drawRoom(struct material **dungeon, int x, int y, int width, int height);
void createCooridors(struct material **dungeon, int roomNum, struct room rooms[MAXROOMS]);
void placeStairs(struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS]);

int main(int argc, char *argv[])
{
    time_t seconds = time(0);
    printf("seed: %ld\n", seconds);
    srand(seconds);
    struct material **dungeon = allocateDungeon();
    drawBlank(dungeon);
    struct room rooms[MAXROOMS];
    createRooms(dungeon, rooms);
    createCooridors(dungeon, 0, rooms);
    struct stair stairs[NUMSTAIRS];
    placeStairs(dungeon, rooms, stairs);

    if (argc > 1)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            if (!(strcmp(argv[i], "--load")))
            {
                if (load(&d))
                {
                    fprintf(stderr, "Failed to load dungeon\n");

                    return 1;
                }

                render_dungeon(&d);
            }
            else if (!(strcmp(argv[i], "--save")))
            {

                // generate then save dungeon
                if (save(d))
                {
                    fprintf(stderr, "Failed to save dungeon\n");

                    return 1;
                }
                render_dungeon(&d);
            }
        }
    }

    printDungeon(dungeon);
    freeDungeon(dungeon);
    return 0;
}


int load(struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS])
{
    char *home = getenv("HOME");
    char *filepath = malloc(sizeof(home) + sizeof("/.rlg327/dungeon") + 1);
    strcpy(filepath, home);
    strcat(filepath, "/.rlg327/dungeon");

    FILE *f;
    if ((f = fopen(filepath, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file at %s, exiting\n", filepath);
        return -1;
    }

    char *filemarker;
    fread(&filemarker, sizeof("RLG327-S2019") - 1, 1, f);

    uint32_t version;
    fread(&version, sizeof(version), 1, f);
    version = be32toh(version);

    uint32_t size;
    fread(&size, sizeof(size), 1, f);
    size = be32toh(version);

    uint8_t pposx, pposy = 0;
    fread(&pposx, sizeof(uint8_t), 1, f);
    fread(&pposy, sizeof(uint8_t), 1, f);

    // read the dungeon!
    fread(dungeon, sizeof(uint8_t), 1680, f);

    uint16_t numRooms;
    fread(&numRooms, sizeof(uint16_t), 1, f);
    numRooms = be16toh(numRooms);
    memset(dungeon->rooms, 0, sizeof(dungeon->rooms) * numRooms); //copied from scaeffer's code

    int i;
    for (i = 0; i < numRooms; i++)
    {
        uint8_t xpos, ypos, width, height = 0;
        fread(&xpos, sizeof(uint8_t), 1, f);
        fread(&ypos, sizeof(uint8_t), 1, f);
        fread(&width, sizeof(uint8_t), 1, f);
        fread(&height, sizeof(uint8_t), 1, f);

        dungeon->num_rooms++;

        dungeon->roo = xpos; //OVERFLOW ERROR?
        dungeon->rooms[i].position[dim_y] = ypos;
        dungeon->rooms[i].size[dim_x] = width;
        dungeon->rooms[i].size[dim_y] = height;
    }

    uint16_t numUpStairs;
    fread(&numUpStairs, sizeof(uint16_t), 1, f);
    numUpStairs = be16toh(numUpStairs);

    for (i = 0; i < numUpStairs; i++)
    {
        uint8_t xpos, ypos;
        fread(&xpos, sizeof(uint8_t), 1, f);
        fread(&ypos, sizeof(uint8_t), 1, f);

        dungeon[ypos][xpos] = '<';
    }

    uint16_t numDownStairs;
    fread(&numDownStairs, sizeof(uint16_t), 1, f);
    numDownStairs = be16toh(numDownStairs);

    for (i = 0; i < numDownStairs; i++)
    {
        uint8_t xpos, ypos;
        fread(&xpos, sizeof(uint8_t), 1, f);
        fread(&ypos, sizeof(uint8_t), 1, f);

        dungeon[ypos][xpos] = '>';
    }

    fclose(f);
    free(filepath);
    return 0;
}


int save( struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS]) {
    uint8_t numUpStairs = 1;
    uint8_t numDownStairs = 1;
    char *home = getenv("HOME");
    char *filepath = malloc(sizeof(home) + sizeof("/.rlg327/dungeon") + 1);
    strcpy(filepath, home);
    strcat(filepath, "/.rlg327/dungeon");

    FILE *f;
    if ((f = fopen(filepath, "r")) == NULL)
    {
        fprintf(stderr, "Error opening file at %s, exiting\n", filepath);
        return -1;
    }

    // file marker to denote type of file
    char* filetype = "RLG327-S2019";
    if (!(fwrite(&filetype, sizeof(filetype), 1, f) == 1)) {
        fprintf(stderr, "Failed to write to filetype %s to %s\n", filetype, filepath);
        return -1;
    }

    uint32_t version = 0;
    if (!(fwrite(&version, sizeof(version), 1, f) == 1)) {
        fprintf(stderr, "Failed to write the version %d to %s\n", version, filepath);
        return -1;
    }

    // MAXROOMS may change
    uint32_t size = 12 + 4 + 4 + 2 + 1680 + (4 * MAXROOMS) + (numUpStairs * 2) + 1 + (numDownStairs * 2);
    if (!(fwrite(&size, sizeof(size), 1, f) == 1)) {
        fprintf(stderr, "Failed to write the size %d to %s\n", size, filepath);
        return -1;
    }


    //unsure if this will work or if we need to write each position 1 by 1
    uint8_t playerposition[2] = {0, 0};
    if (!(fwrite(&playerposition, sizeof(playerposition), 1, f) == 1)) {
        fprintf(stderr, "Failed to write the size %i, %i to %s\n", playerposition[0], playerposition[1], filepath);
        return -1;
    }


    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++) {
        for (col = 0; col < DUNGEONCOLS; col++) {
            if (!(fwrite(&dungeon[row][col].hardness, sizeof(dungeon[row][col].hardness), 1, f) == 1)) {
                fprintf(stderr, "Failed to write hardness %i %i to %s\n", row, col, filepath);
                return -1;
            }
        }
    }

    int i;
    for (i = 0; i < MAXROOMS; i++) {
        //TODO make sure thos writes in correct order
        if (!(fwrite(&rooms[i], sizeof(rooms[i]), 1, f) == 1)) {
            fprintf(stderr, "Failed to write a room to %s\n", filepath);
            return -1;
        }
    }

    if (!(fwrite(&numUpStairs, sizeof(numUpStairs), 1, f) == 1)) {
        fprintf(stderr, "Failed to write to numUpStairs %i to %s\n", numUpStairs, filepath);
        return -1;
    }

    //this implementation sucks but might work down the road nicely
    for (i = 0; i < numDownStairs; i++) {
        if (!(fwrite(&stairs[i], sizeof(stairs[i]), 1, f)) == 1) {
        fprintf(stderr, "Failed to write to a down stair %i %i to %s\n", stairs[i].xpos, stairs[i].ypos, filepath);
        }
    }


    if (!(fwrite(&numDownStairs, sizeof(numDownStairs), 1, f) == 1)) {
        fprintf(stderr, "Failed to write to numDownStairs %i to %s\n", numDownStairs, filepath);
        return -1;
    }

    for (i = numDownStairs; i < numUpStairs + numDownStairs; i++) {
        if (!(fwrite(&stairs[i], sizeof(stairs[i]), 1, f)) == 1) {
            fprintf(stderr, "Failed to write to an up stair %i %i to %s\n", stairs[i].xpos, stairs[i].ypos, filepath);
        }
    }

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

void drawBlank(struct material **dungeon)
{
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++)
    {
        for (col = 0; col < DUNGEONCOLS; col++)
        {
            dungeon[row][col].value = ' ';
            dungeon[row][col].hardness = 0;
        }
    }
    for (row = 0; row < DUNGEONROWS; row++)
    {
        dungeon[row][0].value = '|';
        dungeon[row][0].hardness = 255;
        dungeon[row][DUNGEONCOLS - 1].value = '|';
        dungeon[row][DUNGEONCOLS - 1].hardness = 255;
    }
    for (col = 0; col < DUNGEONCOLS; col++)
    {
        dungeon[0][col].value = '-';
        dungeon[0][col].hardness = 255;
        dungeon[DUNGEONROWS - 1][col].value = '-';
        dungeon[DUNGEONROWS - 1][col].hardness = 255;
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
            if (!(isValid(dungeon[row - 1][col - 1].value) && isValid(dungeon[row][col - 1].value) && isValid(dungeon[row - 1][col].value) && isValid(dungeon[row][col].value) && isValid(dungeon[row + 1][col].value) && isValid(dungeon[row][col + 1].value) && isValid(dungeon[row + 1][col + 1].value)))
            {
                return 0;
            }
        }
    }
    return 1;
}

void createRooms(struct material **dungeon, struct room rooms[MAXROOMS])
{
    int numRooms = 0;
    while (numRooms < MAXROOMS)
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
void createCooridors(struct material **dungeon, int roomNum, struct room rooms[MAXROOMS])
{
    // basic idea is to keep adding to x using the provided formula
    // when the origin is equal to the destination then go to
    // doing the same for the y values. Also, don't overrite rooms
    int x = rooms[roomNum].xpos;
    int y = rooms[roomNum].ypos;

    int x1 = rooms[roomNum + 1].xpos;
    int y1 = rooms[roomNum + 1].ypos;

    while (x != x1)
    {
        x += (x1 - x) / abs(x1 - x);
        if (dungeon[y][x].value == ' ')
        {
            dungeon[y][x].value = '#';
        }
    }

    while (y != y1)
    {
        y += (y1 - y) / abs(y1 - y);
        if (dungeon[y][x].value == ' ')
        {
            dungeon[y][x].value = '#';
        }
    }

    if (roomNum < MAXROOMS - 2)
    {
        createCooridors(dungeon, roomNum + 1, rooms);
    }
}

void placeStairs(struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS])
{
    uint8_t upX = (rand() % rooms[0].width) + rooms[0].xpos;
    uint8_t upY = (rand() % rooms[0].height) + rooms[0].ypos;
    dungeon[upY][upX].value = '<';

    uint8_t downX = (rand() % rooms[MAXROOMS - 1].width) + rooms[MAXROOMS - 1].xpos;
    uint8_t downY = (rand() % rooms[MAXROOMS - 1].height) + rooms[MAXROOMS - 1].ypos;
    dungeon[downY][downX].value = '>';
}

x

// int save(struct material **dungeon, struct room rooms[MAXROOMS])
// {
//     // Scaheffer's code does not provide a data structure for stairs, so make one now
//     uint8_t numUpStairs;
//     uint8_t numDownStairs;

//     // find the stairs and put them in memory so can write the bastards
//     int row, col = 0;
//     for (row = 0; row < DUNGEON_Y; row++)
//     {
//         for (col = 0; col < DUNGEON_X; col++)
//         {
//             if (dungeon.map[row][col] == ter_stairs_up)
//             {
//                 numUpStairs++;
//             }
//             else if (dungeon.map[row][col] == ter_stairs_down)
//             {
//                 numDownStairs++;
//             }
//         }
//     }

//     stair_t stairs[numUpStairs + numDownStairs];
//     int numstairs = 0;
//     for (row = 0; row < DUNGEON_Y; row++)
//     {
//         for (col = 0; col < DUNGEON_X; col++)
//         {
//             if (dungeon.map[row][col] == ter_stairs_up)
//             {
//                 stairs[numstairs].ypos = col;
//                 stairs[numstairs].xpos = row;
//                 stairs[numstairs].value = ter_stairs_up;
//             }
//             else if (dungeon.map[row][col] == ter_stairs_down)
//             {
//                 stairs[numstairs].ypos = col;
//                 stairs[numstairs].xpos = row;
//                 stairs[numstairs].value = ter_stairs_down;
//             }
//         }
//     }
//     // end retreiving stair info

//     char *home = getenv("HOME");
//     char *filepath = malloc(sizeof(home) + sizeof("/.rlg327/dungeon") + 1);
//     strcpy(filepath, home);
//     strcat(filepath, "/.rlg327/dungeon");
//     uint32_t version = 0;
//     uint32_t size = 1708 + (4 * dungeon.num_rooms) + (numUpStairs * 2) + (numDownStairs * 2);
//     uint8_t playerposition[2] = {0, 0};

//     FILE *f;
//     if ((f = fopen(filepath, "w")) == NULL)
//     {
//         fprintf(stderr, "Error opening file at %s, exiting\n", filepath);
//         return -1;
//     }

//     // file marker to denote type of file
//     char *filetype = "RLG327-S2019";
//     if (!(fwrite(&filetype, sizeof(filetype) - 1, 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write to filetype %s to %s\n", filetype, filepath);
//         return -1;
//     }

//     if (!(fwrite(&version, sizeof(version), 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write the version %d to %s\n", version, filepath);
//         return -1;
//     }

//     // MAXROOMS may change
//     if (!(fwrite(&size, sizeof(size), 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write the size %d to %s\n", size, filepath);
//         return -1;
//     }

//     //unsure if this will work or if we need to write each position 1 by 1
//     if (!(fwrite(&playerposition, sizeof(playerposition), 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write the size %i, %i to %s\n", playerposition[0], playerposition[1], filepath);
//         return -1;
//     }

//     for (row = 0; row < DUNGEON_Y; row++)
//     {
//         for (col = 0; col < DUNGEON_X; col++)
//         {
//             if (!(fwrite(&dungeon.hardness[row][col], sizeof(dungeon.hardness[row][col]), 1, f) == 1))
//             {
//                 fprintf(stderr, "Failed to write hardness %i %i to %s\n", row, col, filepath);
//                 return -1;
//             }
//         }
//     }

//     int i;
//     for (i = 0; i < dungeon.num_rooms; i++)
//     {
//         // TODO make sure thos writes in correct order
//         if (!(fwrite(&dungeon.rooms[i], sizeof(dungeon.rooms[i]), 1, f) == 1))
//         {
//             fprintf(stderr, "Failed to write a room to %s\n", filepath);
//             return -1;
//         }
//     }

//     if (!(fwrite(&numUpStairs, sizeof(numUpStairs), 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write to numUpStairs %i to %s\n", numUpStairs, filepath);
//         return -1;
//     }

//     //this implementation sucks but might work down the road nicely
//     for (i = 0; i < numUpStairs + numDownStairs; i++)
//     {
//         if (stairs[i].value == ter_stairs_up)
//         {
//             if (!((fwrite(&stairs[i].xpos, sizeof(stairs[i].xpos), 1, f)) == 1))
//             {
//                 fprintf(stderr, "Failed to write to a upward stair xpos %i to %s\n", stairs[i].xpos, filepath);
//             }
//             if (!((fwrite(&stairs[i].ypos, sizeof(stairs[i].ypos), 1, f)) == 1))
//             {
//                 fprintf(stderr, "Failed to write to a upward stair ypos %i to %s\n", stairs[i].xpos, filepath);
//             }
//         }
//     }

//     if (!(fwrite(&numDownStairs, sizeof(numDownStairs), 1, f) == 1))
//     {
//         fprintf(stderr, "Failed to write to numDownStairs %i to %s\n", numDownStairs, filepath);
//         return -1;
//     }

//     for (i = 0; i < numDownStairs + numDownStairs; i++)
//     {
//         if (stairs[i].value == ter_stairs_down)
//         {
//             if (!((fwrite(&stairs[i].xpos, sizeof(stairs[i].xpos), 1, f)) == 1))
//             {
//                 fprintf(stderr, "Failed to write to a down stair xpos %i to %s\n", stairs[i].xpos, filepath);
//             }
//             if (!((fwrite(&stairs[i].ypos, sizeof(stairs[i].ypos), 1, f)) == 1))
//             {
//                 fprintf(stderr, "Failed to write to a down stair ypos %i to %s\n", stairs[i].xpos, filepath);
//             }
//         }
//     }

//     free(filepath);
//     fclose(f);
//     return 0;
// }

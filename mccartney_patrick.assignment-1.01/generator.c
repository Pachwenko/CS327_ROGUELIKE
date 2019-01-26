#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'

#define DUNGEONCOLS 80
#define DUNGEONROWS 21
#define NUMROOMS 6
#define MINROOMWIDTH 4
#define MINROOMHEIGHT 3

void printDungeon(char **dungeon);
char **initializeDungeon();
void freeDungeon(char **dungeon);
void createRooms(char **dungeon);
int isImmuteable(char value);
int isValid(char value);
int canPlaceRoom(char **dungeon, int x, int y, int width, int height);

int main(int argc, char *argv[])
{
    char **dungeon = initializeDungeon();
    printDungeon(dungeon);
    //createRooms(*&dungeon);

    srand(time(NULL));

    freeDungeon(dungeon);
    return 0;
}

char **initializeDungeon()
{
    char **dungeon;
    //allocate pointers to the rows of our dungeon
    dungeon = malloc(DUNGEONROWS * sizeof(char *));

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
    return dungeon;
}

void printDungeon(char **dungeon)
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

void freeDungeon(char **dungeon)
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

void createRooms(char **dungeon)
{
    int rooms = 0;
    while (rooms < NUMROOMS)
    {
        // TODO: replace width and height with random values

        int width = MINROOMWIDTH;
        int height = MINROOMHEIGHT;

        int x = (rand() % DUNGEONCOLS) + 1;
        int y = (rand() % DUNGEONROWS) + 1;
        // check if the room can be placed
        if (canPlaceRoom(dungeon, x, y, width, height))
        {
            printf("Succes! you can place a damn room~\n");
            rooms++;

            // write the room!! to memory!
        }
    }
}

int canPlaceRoom(char **dungeon, int x, int y, int width, int height)
{
    int i, j = 0;
    for (i = 0; i < x; i++)
    {
        for (j = 0; j < y; j++)
        {
            if (!(isValid(dungeon[i][j])))
            {
                return 0;
            }
        }
    }
    return 1;
}

void placeRoom(char **dungeon, int x, int y, int width, int height)
{
    int i, j = 0;

    // TODO: is this the correct way, y - height decrese y val?
    for (j = y; j > y - height; j--)
    {
        for (i = x; i < x + width; i++)
        {
            dungeon[i][j] = '.';
        }
    }
}
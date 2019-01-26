#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'

#define DUNGEONROWS 21
#define DUNGEONCOLS 80
#define NUMROOMS 6
#define ROOMWIDTH 4
#define ROOMHEIGHT 3

void printDungeon(char** dungeon);
char** initializeDungeon();
void freeDungeon(char** dungeon);
int isImmuteable(char value);
void createRooms(char** dungeon);
int placeRoom(int width, int height);

int main(int argc, char* argv[]) {
    char** dungeon = initializeDungeon();
    printDungeon(*&dungeon);

    srand(time(NULL));

    freeDungeon(*&dungeon);
    return 0;
}

char** initializeDungeon() {
    char** dungeon;
    //allocate pointers to the rows of our dungeon
    dungeon = malloc(DUNGEONROWS * sizeof(char*));

    if (dungeon == NULL) {
        fprintf(stderr, "Dungeon is null - out of memory\n");
        exit(0);
    }

    //now allocate enough columns for each row
    int i = 0;
    for (i = 0; i < DUNGEONROWS; i++) {
        dungeon[i] = malloc(DUNGEONCOLS * sizeof(char*));
        if (dungeon[i] == NULL) {
            fprintf(stderr, "Out of memory - allocating dungeolCols");
            exit(0);
        }
    }

    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++) {
        for (col = 0; col < DUNGEONCOLS; col++) {
            dungeon[row][col] = ' ';
        }
    }
    for (row = 0; row < DUNGEONROWS; row++) {
        dungeon[row][0] = '|';
        dungeon[row][DUNGEONCOLS - 1] = '|';
    }
    for (col = 0; col < DUNGEONCOLS; col++) {
        dungeon[0][col] = '-';
        dungeon[DUNGEONROWS - 1][col] = '-';
    }
    return dungeon;
}

void printDungeon(char** dungeon) {
    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++) {
        for (col = 0; col < DUNGEONCOLS; col++) {
            printf("%c", dungeon[row][col]);
        }
        printf("\n");
    }
}

void freeDungeon(char** dungeon) {
    int i = 0;
    for (i = 0; i < DUNGEONROWS; i++) {
        free(dungeon[i]);
    }
    free(dungeon);
}

int isImmuteable(char value) {
    if (value == '-' || value == '|') {
        return 1;
    }
    return 0;
}

void createRooms(char** dungeon) {
    int rooms = 0;
    while (rooms < NUMROOMS) {
        // TODO: replace width and height with random values

        int width = ROOMWIDTH;
        int height = ROOMHEIGHT;
        if (!(placeRoom(width, height))) {
            fprintf(stderr, "Failed to create a room");
            exit;
        }
    }
}

int placeRoom(int width, int height) {

}
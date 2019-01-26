#include <stdio.h>
#include <stdlib.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'


#define DUNGEONCOLS 80
#define DUNGEONROWS 21
#define NUMROOMS 6
#define MINROOMWIDTH 4
#define MINROOMHEIGHT 3

void printDungeon(char** dungeon);
char** initializeDungeon();
void freeDungeon(char** dungeon);
void createRooms(char** dungeon);
int isImmuteable(char value);

int main(int argc, char* argv[]) {
    char** dungeon = initializeDungeon();
    printDungeon(*&dungeon);
    //createRooms(*&dungeon);

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
    int y, x = 0;
    for (y = 0; y < DUNGEONCOLS; y++) {
        dungeon[y] = malloc(DUNGEONROWS * sizeof(char*));
        if (dungeon[y] == NULL) {
            fprintf(stderr, "Out of memory - allocating dungeolCols");
            exit(0);
        }
    }

    for (y = 0; y < DUNGEONROWS; y++) {
        for (x = 0; x < DUNGEONCOLS; x++) {
            dungeon[y][x] = ' ';
        }
    }
    for (y = 0; y < DUNGEONROWS; y++) {
        dungeon[y][0] = '|';
        dungeon[y][DUNGEONCOLS - 1] = '|';
    }
    for (x = 0; x < DUNGEONCOLS; x++) {
        dungeon[0][x] = '-';
        dungeon[DUNGEONROWS - 1][x] = '-';
    }
    return dungeon;
}

void printDungeon(char** dungeon) {
    int x, y = 0;
    for (y = 0; y < DUNGEONROWS; y++) {
        for (x = 0; x < DUNGEONCOLS; x++) {
            printf("%c", dungeon[y][x]);
        }
        printf("\n");
    }
}

void freeDungeon(char** dungeon) {
    int y = 0;
    for (y = 0; y < DUNGEONCOLS; y++) {
        free(dungeon[y]);
    }
    free(dungeon);
}

void createRooms(char** dungeon) {
int row, col = 0;
    for (col = 0; col < DUNGEONROWS; col++) {
        for (row = 0; row < DUNGEONCOLS; row++) {
            printf("%c", dungeon[row][col]);
        }
        printf("\n");
    }
}

int isImmuteable(char value) {
    if (value == '|' || value == '-') {
        return 1;
    }
    return 0;
}
#include <stdio.h>
#include <stdlib.h>

// For reference, spaces are rocks ' ', floors are periods '.', corridors are hashes '#',
// upward staircase are less than '<', downward stairs are greater than '>'


#define dungeonX 80
#define dungeonY 21 //remember in C columns or Y comes first as to increase performance
#define numRooms 6
#define minRoomWidth 4
#define minRoomHeight 3

void printDungeon(char** dungeon);
char** initializeDungeon();
void freeDungeon(char** dungeon);

int main(int argc, char* argv[]) {
    char** dungeon = initializeDungeon();
    printDungeon(*&dungeon);

    freeDungeon(*&dungeon);
    return 0;
}

char** initializeDungeon() {
    char** dungeon;
    //allocate pointers to the rows of our dungeon
    dungeon = malloc(dungeonY * sizeof(char*));

    if (dungeon == NULL) {
        fprintf(stderr, "Dungeon is null - out of memory\n");
        exit(0);
    }

    //now allocate enough columns for each row
    int i = 0;
    for (i = 0; i < dungeonY; i++) {
        dungeon[i] = malloc(dungeonX * sizeof(char*));
        if (dungeon[i] == NULL) {
            fprintf(stderr, "Out of memory - allocating dungeolCols");
            exit(0);
        }
    }

    int row, col = 0;
    for (row = 0; row < dungeonY; row++) {
        for (col = 0; col < dungeonX; col++) {
            dungeon[row][col] = ' ';
        }
    }
    for (row = 0; row < dungeonY; row++) {
        dungeon[row][0] = '|';
        dungeon[row][dungeonX - 1] = '|';
    }
    for (col = 0; col < dungeonX; col++) {
        dungeon[0][col] = '-';
        dungeon[dungeonY - 1][col] = '-';
    }
    return dungeon;
}

void printDungeon(char** dungeon) {
    int row, col = 0;
    for (row = 0; row < dungeonY; row++) {
        for (col = 0; col < dungeonX; col++) {
            printf("%c", dungeon[row][col]);
        }
        printf("\n");
    }
}

void freeDungeon(char** dungeon) {
    int i = 0;
    for (i = 0; i < dungeonY; i++) {
        free(dungeon[i]);
    }
    free(dungeon);
}

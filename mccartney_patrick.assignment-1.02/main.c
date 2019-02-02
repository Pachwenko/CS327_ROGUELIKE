#include <stdio.h>
#include "generator.h"
#include <stdlib.h>

void generate(struct material **dungeon, struct room rooms[MAXROOMS]);
int save(char* filepath, struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS]);
int load(char* filepath, struct material **dungeon);


int main (int argc, char* argv[]) {
    time_t seconds = time(0);
    srand(seconds);
    printf("seed: %ld\n", seconds);
    struct material **dungeon = allocateDungeon();
    struct room rooms[MAXROOMS];
    struct stair stairs[NUMSTAIRS];
    generate(dungeon, rooms);
    placeStairs(dungeon, rooms, stairs);


    char* filepath = getenv("$HOME");
    strcat(filepath, ".rgl327/dungeon");
    printf("Filepath: %s", filepath);

    if (save(filepath, dungeon)) {
        return -1;
    }

    printDungeon(dungeon);
}

void generate(struct material **dungeon, struct room rooms[MAXROOMS])
{
    drawBlank(dungeon);
    createRooms(dungeon, rooms);
    createCooridors(dungeon, 0, rooms);
    placeStairs(dungeon, rooms);
}

int save(char* filepath, struct material **dungeon, struct room rooms[MAXROOMS], struct stair stairs[NUMSTAIRS]) {
    uint8_t numUpStairs = 1;
    uint8_t numDownStairs = 1;
    FILE* f;
    if (!(f = fopen(filepath, 'w'))) {
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
        fprintf(stderr, "Failed to write the size %li, %li to %s\n", playerposition[0], playerposition[1], filepath);
        return -1;
    }


    int row, col = 0;
    for (row = 0; row < DUNGEONROWS; row++) {
        for (col = 0; col < DUNGEONCOLS; col++) {
            if (!(fwrite(&dungeon[row][col].hardness, sizeof(dungeon[row][col].hardness), 1, f) == 1)) {
                fprintf(stderr, "Failed to write hardness %li %li to %s\n", row, col, filepath);
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
        fprintf(stderr, "Failed to write to numUpStairs %li to %s\n", numUpStairs, filepath);
        return -1;
    }

    //this implementation sucks but might work down the road nicely
    for (i = 0; i < numDownStairs; i++) {
        if (!(fwrite(&stairs[i], sizeof(stairs[i]), 1, f)) == 1) {
        fprintf(stderr, "Failed to write to a down stair %li %li to %s\n", stairs[i].xpos, stairs[i].ypos, filepath);
        }
    }


    if (!(fwrite(&numDownStairs, sizeof(numDownStairs), 1, f) == 1)) {
        fprintf(stderr, "Failed to write to numDownStairs %li to %s\n", numDownStairs, filepath);
        return -1;
    }

    for (i = numDownStairs; i < numUpStairs + numDownStairs; i++) {
        if (!(fwrite(&stairs[i], sizeof(stairs[i]), 1, f)) == 1) {
            fprintf(stderr, "Failed to write to an up stair %li %li to %s\n", stairs[i].xpos, stairs[i].ypos, filepath);
        }
    }

}





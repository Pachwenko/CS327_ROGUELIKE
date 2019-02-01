#include <stdio.h>
#include "generator.h"

void save(struct material **dungeon);
void generate(struct material **dungeon, struct room rooms[MAXROOMS]);


int main (int argc, char* argv[]) {
    time_t seconds = time(0);
    srand(seconds);
    printf("seed: %ld\n", seconds);
    struct material **dungeon = allocateDungeon();
    struct room rooms[MAXROOMS];

    generate(dungeon, rooms);
    printDungeon(dungeon);
    save(dungeon);
}

void generate(struct material **dungeon, struct room rooms[MAXROOMS])
{
    drawBlank(dungeon);
    createRooms(dungeon, rooms);
    createCooridors(dungeon, 0, rooms);
    placeStairs(dungeon, rooms);
}

void save(struct material **dungeon) {

}
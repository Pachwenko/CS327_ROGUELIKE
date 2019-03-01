#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "heap.h"

#define DUNGEON_X 80
#define DUNGEON_Y 21
#define INFINITY 214748364
#define NUM_MOBS 13
#define PC_SPEED 10
#define MAX_SPEED 20
#define MIN_SPEED 5
#define NUM_ATTRIBUTES 4
#define PC_SPEED 10
#define NPC_SMART   0x00000001
#define NPC_TELE    0x00000002
#define NPC_TUNNEL  0x00000004
#define NPC_ERRATIC 0x00000008


#define has_characteristic(character, bit) ((character)->characteristics & NPC_##bit)

#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (d->map[y][x])
#define hardnesspair(pair) (d->hardness[pair[dim_y]][pair[dim_x]])
#define hardnessxy(x, y) (d->hardness[y][x])
/* Returns random integer in [min, max]. */
#define rand_range(min, max) ((rand() % (((max) + 1) - (min))) + (min))

typedef enum __attribute__((__packed__)) terrain_type
{
  ter_debug,
  ter_wall,
  ter_wall_immutable,
  ter_floor,
  ter_floor_room,
  ter_floor_hall,
  ter_stairs,
  ter_stairs_up,
  ter_stairs_down,
  mob
} terrain_type_t;

typedef enum __attribute__((__packed__)) monster_type
{
  mob_human,
  mob_nonhuman,
  mob_giant,
  mob_dragon,
  player
} mob_type_t;

typedef enum __attribute__((__packed__)) attribute_type
{ // if none of
  intelligence,
  telepathy,
  tunneling,
  erratic
} attribute_type_t;

typedef struct corridor_path
{
  heap_node_t *hn;
  uint8_t pos[2];
  int32_t cost;
} corridor_path_t;

typedef enum dim
{
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int8_t pair_t[num_dims];

typedef struct room
{
  pair_t position;
  pair_t size;
} room_t;

typedef struct dungeon
{
  uint16_t num_rooms;
  room_t *rooms;
  terrain_type_t map[DUNGEON_Y][DUNGEON_X];
  /* Since hardness is usually not used, it would be expensive to pull it *
   * into cache every time we need a map cell, so we store it in a        *
   * parallel array, rather than using a structure to represent the       *
   * cells.  We may want a cell structure later, but from a performanace  *
   * perspective, it would be a bad idea to ever have the map be part of  *
   * that structure.  Pathfinding will require efficient use of the map,  *
   * and pulling in unnecessary data with each map cell would add a lot   *
   * of overhead to the memory system.                                    */
  uint8_t hardness[DUNGEON_Y][DUNGEON_X];
  pair_t pc;
} dungeon_t;

typedef struct monster
{
  pair_t loc;
  pair_t pc_loc;
  int characteristics;
  char symbol;
  uint8_t speed;
  heap_node_t *hn;
  uint64_t priority; // this is to avoid any overflow incase a game goes on for a very long time
} monster_t;

static int32_t corridor_path_cmp(const void *key, const void *with)
{
  return ((corridor_path_t *)key)->cost - ((corridor_path_t *)with)->cost;
}

// Modified dijkstra_corridor from rlg327.c
// Initialized to INT_MAX and adds all points to the heap as long as
// their map isnt of an immuteable type and hardness isnt 255
//
// Sets the distance from every point to the PC's position and stores it
// in the distmap array
static int tunneling_dijkstras(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized)
  {
    for (y = 0; y < DUNGEON_Y; y++)
    {
      for (x = 0; x < DUNGEON_X; x++)
      {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
        path[y][x].cost = INFINITY;
      }
    }
    initialized = 1;
  }

  path[d->pc[dim_y]][d->pc[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (mapxy(x, y) != ter_wall_immutable && hardnessxy(x, y) != 255)
      {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      }
      else
      {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    // bassically check if the current cost is more than at a nearby position
    // if so update the cost
    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y-1][x]
      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y][x-1]
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y][x+1]
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y+1][x]
      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y+1][x-1]
      path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y+1][x+1]
      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y-1][x-1]
      path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > p->cost + 1 + (hardnesspair(p->pos) / 85)))
    {
      // check [y-1][x+1]
      path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = p->cost + 1 + (hardnesspair(p->pos) / 85);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }
  }

  // set the distmap now that we have it saved
  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      distmap[y][x] = path[y][x].cost;
    }
  }
  return 0;
}

// Method which calls tunneling_dijkstra and outputs the results to
// standard output where X values represent infinity/unable to reach
// also stores distance map in the distmap array
static void print_tunneling_distmap(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
{
  if (tunneling_dijkstras(d, distmap))
  {
    fprintf(stderr, "Error creating tunneling monster's distmap");
  }

  int y, x;
  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (distmap[y][x] == INFINITY)
      {
        printf("X");
      }
      else if (y == d->pc[dim_y] && x == d->pc[dim_x])
      {
        printf("@");
      }
      else
      {
        printf("%d", distmap[y][x] % 10);
      }
    }
    printf("\n");
  }
}

// Modified dijkstra_corridor from rlg327.c
// Initialized to INT_MAX and adds all points to the heap as long as
// the hardness at that point is 0
//
// Sets the distance from every cooridor/room poisition to the PC's position
// and stores it in the distmap array
static int non_tunneling_dijkstras(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
{
  static corridor_path_t path[DUNGEON_Y][DUNGEON_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized)
  {
    for (y = 0; y < DUNGEON_Y; y++)
    {
      for (x = 0; x < DUNGEON_X; x++)
      {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
        path[y][x].cost = INFINITY;
      }
    }
    initialized = 1;
  }

  path[d->pc[dim_y]][d->pc[dim_x]].cost = 0;

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (d->hardness[y][x] == 0)
      {
        path[y][x].hn = heap_insert(&h, &path[y][x]);
      }
      else
      {
        path[y][x].hn = NULL;
      }
    }
  }

  while ((p = heap_remove_min(&h)))
  {
    p->hn = NULL;

    // bassically check if the current cost is more than at a nearby position
    // if so update the cost
    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > p->cost + 1))
    {
      // check [y-1][x]
      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > p->cost + 1))
    {
      // check [y][x-1]
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > p->cost + 1))
    {
      // check [y][x+1]
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > p->cost + 1))
    {
      // check [y+1][x]
      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost > p->cost + 1))
    {
      // check [y+1][x-1]
      path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > p->cost + 1))
    {
      // check [y+1][x+1]
      path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > p->cost + 1))
    {
      // check [y-1][x-1]
      path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > p->cost + 1))
    {
      // check [y-1][x+1]
      path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = p->cost + 1;
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
    }
  }

  // set the distmap now that we have it saved
  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      distmap[y][x] = path[y][x].cost;
    }
  }
  return 0;
}

// outputs the non-tunneling monster distmap to standard output and stores
// the distance values in the distmap array. Any infinite value or negative
// value is output as a "X".
static void print_nontunneling_distmap(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
{
  if (non_tunneling_dijkstras(d, distmap))
  {
    fprintf(stderr, "Error creating non-tunneling monster's distmap");
  }

  int y, x;
  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (d->map[y][x] == ter_wall || d->map[y][x] == ter_wall_immutable) {
        printf(" ");
      }
      else if (distmap[y][x] == INFINITY) {
        printf("X");
      }
      else if (y == d->pc[dim_y] && x == d->pc[dim_x])
      {
        printf("@");
      }
      else
      {
        printf("%d", distmap[y][x] % 10);
      }
    }
    printf("\n");
  }
}

// interface for creating distance maps for tunneling and non-tunneling
// monsters. Holds the distance map arrays. Only useful for assignment1.03
void generate_distmaps(dungeon_t *d)
{
  // 2d int array takes ~6kb of memory on the stack so it's not too bad
  int32_t distmap[DUNGEON_Y][DUNGEON_X];
  int32_t distmap2[DUNGEON_Y][DUNGEON_X];

  printf("\n");
  print_nontunneling_distmap(d, distmap2);
  printf("\n");
  print_tunneling_distmap(d, distmap);
}

void init_mobs(dungeon_t *d, monster_t *mobs, int num_monsters) {
  //first monster is the player
  mobs[0].pc_loc[dim_x] = d->pc[dim_x];
  mobs[0].pc_loc[dim_y] = d->pc[dim_y];
  mobs[0].loc[dim_x] = d->pc[dim_x];
  mobs[0].loc[dim_y] = d->pc[dim_y];
  mobs[0].speed = PC_SPEED;
  mobs[0].priority = 0;
  mobs[0].characteristics = 0;
  char symbols[] = "0123456789abcdef";
  mobs[0].symbol =  '@';

  int i = 0;
  for (i = 1; i < num_monsters; i++) {
    // randomize location, speed, type, and attributes
    mobs[i].pc_loc[dim_x] = UINT8_MAX;
    mobs[i].pc_loc[dim_y] = UINT8_MAX;
    mobs[i].speed = rand_range(5, 20);
    mobs[i].priority = 1000 / mobs[i].speed;
    mobs[i].characteristics = rand() % 16;
    mobs[i].symbol =  symbols[mobs[i].characteristics];

    uint8_t was_placed = 0;
    while (!was_placed) {
       mobs[i].loc[dim_x] = rand_range(1, DUNGEON_X - 1);
       mobs[i].loc[dim_y] = rand_range(1, DUNGEON_Y - 1);

      if (d->hardness[mobs[i].loc[dim_y]][mobs[i].loc[dim_x]] == 0 || has_characteristic(&mobs[i], TUNNEL)) {
        was_placed = 1;
      }
    }
  }

  int counter = 0;
  for (i = 0; i < num_monsters; i++) {
    if (has_characteristic(&mobs[i], TUNNEL)) {
      counter++;
    }
  }
  if (counter == num_monsters) {
    printf("ALL MOBS CAN TUNNEL, WATCH OUT\n");
  }
}

static int32_t monster_cmp(const void *key, const void *with)
{
  return (((monster_t *)key)->priority) - (((monster_t *)with)->priority);
}

void render(dungeon_t *d, monster_t *m, int num_monsters) {

  int i, y, x = 0;
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      char toPut;
        switch (d->map[y][x]) {
        case ter_wall:
          toPut = ' ';
          break;
        case ter_wall_immutable:
          toPut = '=';
          break;
        case ter_floor:
        case ter_floor_room:
          toPut = '.';
          break;
        case ter_floor_hall:
          toPut = '#';
          break;
        case ter_debug:
          toPut = '*';
          fprintf(stderr, "Debug character at %d, %d\n", y, x);
          break;
        case ter_stairs_up:
          toPut = '<';
          break;
        case ter_stairs_down:
          toPut = '>';
          break;
        default:
          toPut = ' ';
          break;
        }
      for (i = 0; i < num_monsters; i++) {
        if (m[i].loc[dim_y] == y && m[i].loc[dim_x] == x) {
          toPut = m[i].symbol;
        }
      }
      putchar(toPut);
    }
    putchar('\n');
  }
}
void end_game(int pc_won, dungeon_t *d, monster_t *mobs, heap_t *heap) {
  if (pc_won) {
    printf(
      "\n                                       o\n"
      "                                      $\"\"$o\n"
      "                                     $\"  $$\n"
      "                                      $$$$\n"
      "                                      o \"$o\n"
      "                                     o\"  \"$\n"
      "                oo\"$$$\"  oo$\"$ooo   o$    \"$    ooo\"$oo  $$$\"o\n"
      "   o o o o    oo\"  o\"      \"o    $$o$\"     o o$\"\"  o$      \"$  "
      "\"oo   o o o o\n"
      "   \"$o   \"\"$$$\"   $$         $      \"   o   \"\"    o\"         $"
      "   \"o$$\"    o$$\n"
      "     \"\"o       o  $          $\"       $$$$$       o          $  ooo"
      "     o\"\"\n"
      "        \"o   $$$$o $o       o$        $$$$$\"       $o        \" $$$$"
      "   o\"\n"
      "         \"\"o $$$$o  oo o  o$\"         $$$$$\"        \"o o o o\"  "
      "\"$$$  $\n"
      "           \"\" \"$\"     \"\"\"\"\"            \"\"$\"            \""
      "\"\"      \"\"\" \"\n"
      "            \"oooooooooooooooooooooooooooooooooooooooooooooooooooooo$\n"
      "             \"$$$$\"$$$$\" $$$$$$$\"$$$$$$ \" \"$$$$$\"$$$$$$\"  $$$\""
      "\"$$$$\n"
      "              $$$oo$$$$   $$$$$$o$$$$$$o\" $$$$$$$$$$$$$$ o$$$$o$$$\"\n"
      "              $\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
      "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"$\n"
      "              $\"                                                 \"$\n"
      "              $\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\"$\""
      "$\"$\"$\"$\"$\"$\"$\"$\n"
      "                                   You win!\n\n"
      );
  } else {
    printf(
      "\n\n\n\n                /\"\"\"\"\"/\"\"\"\"\"\"\".\n"
      "               /     /         \\             __\n"
      "              /     /           \\            ||\n"
      "             /____ /   Rest in   \\           ||\n"
      "            |     |    Pieces     |          ||\n"
      "            |     |               |          ||\n"
      "            |     |   A. Luser    |          ||\n"
      "            |     |               |          ||\n"
      "            |     |     * *   * * |         _||_\n"
      "            |     |     *\\/* *\\/* |        | TT |\n"
      "            |     |     *_\\_  /   ...\"\"\"\"\"\"| |"
      "| |.\"\"....\"\"\"\"\"\"\"\".\"\"\n"
      "            |     |         \\/..\"\"\"\"\"...\"\"\""
      "\\ || /.\"\"\".......\"\"\"\"...\n"
      "            |     |....\"\"\"\"\"\"\"........\"\"\"\"\""
      "\"^^^^\".......\"\"\"\"\"\"\"\"..\"\n"
      "            |......\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"......"
      "..\"\"\"\"\"....\"\"\"\"\"..\"\"...\"\"\".\n\n"
      "            You're dead.  Better luck in the next life.\n\n\n"
      );
  }
  d = NULL;
  free(mobs);
  heap_delete(heap);
  exit(0);
}

void move_monster(heap_t *heap, dungeon_t *d, monster_t *mob, int num_monsters, monster_t *all_mobs, int32_t tunneling[DUNGEON_Y][DUNGEON_X], int32_t non_tunneling[DUNGEON_Y][DUNGEON_X]) {
  // move in a straight line to the last known pc location
  // if last known pc location is null, just move randomly

  uint8_t xpos = 0;
  uint8_t ypos = 0;
  // move to last known pc location
  if (mob->pc_loc[dim_x] != UINT8_MAX && mob->pc_loc[dim_y] != UINT8_MAX) {
    if (has_characteristic(mob, TUNNEL)) {
      // when they are tunneling
      // move in a straight line to the pc's last known location
      if (d->pc[dim_x] > mob->loc[dim_x]) {
        xpos++;
      } else if (d->pc[dim_x] < mob->loc[dim_x]) {
        xpos--;
      }

      if (d->pc[dim_y] > mob->loc[dim_y]) {
        ypos++;
      } else if (d->pc[dim_y] < mob->loc[dim_y]) {
        ypos--;
      }

      mob->loc[dim_y] += ypos;
      mob->loc[dim_x] += xpos;
      d->map[mob->loc[dim_y]][mob->loc[dim_x]] = ter_floor_hall;
    } else {
      // when they are NOT tunneling
      // if the path is blocked then just do nothing for now
      if (mob->pc_loc[dim_x] > mob->loc[dim_x] && d->hardness[mob->loc[dim_y]][mob->loc[dim_x+1]] == 0) {
        xpos++;
      } else if (mob->pc_loc[dim_x] < mob->loc[dim_x] && d->hardness[mob->loc[dim_y]][mob->loc[dim_x-1]] == 0) {
        xpos--;
      }

      if (mob->pc_loc[dim_y] > mob->loc[dim_y] && d->hardness[mob->loc[dim_y+1]][mob->loc[dim_x]] == 0) {
        ypos++;
      } else if (mob->pc_loc[dim_y] < mob->loc[dim_y] && d->hardness[mob->loc[dim_y-1]][mob->loc[dim_x]] == 0) {
        ypos--;
      }
      if (xpos || ypos) {
        // only do something if they actually moved
        mob->loc[dim_y] += ypos;
        mob->loc[dim_x] += xpos;
        d->map[mob->loc[dim_y]][mob->loc[dim_x]] = ter_floor_hall;
      }
    }
  } else {
    // just move randomly cause they haven't seen the PC
    if (has_characteristic(mob, TUNNEL)) {
      xpos = rand_range(-1, 1);
      ypos = rand_range(-1, 1);
      if (xpos || ypos) {
        mob->loc[dim_y] += ypos;
        mob->loc[dim_x] += xpos;
        d->map[mob->loc[dim_y]][mob->loc[dim_x]] = ter_floor_hall;
      }
    } else {
      if (rand_range(0,1) && d->hardness[mob->loc[dim_y]][mob->loc[dim_x+1]] == 0) {
        xpos++;
      } else if (rand_range(0,1) && d->hardness[mob->loc[dim_y]][mob->loc[dim_x-1]] == 0) {
        xpos--;
      }

      if (rand_range(0,1) && d->hardness[mob->loc[dim_y+1]][mob->loc[dim_x]] == 0) {
        ypos++;
      } else if (rand_range(0,1) && d->hardness[mob->loc[dim_y-1]][mob->loc[dim_x]] == 0) {
        ypos--;
      }
      if (xpos || ypos) {
        mob->loc[dim_y] += ypos;
        mob->loc[dim_x] += xpos;
        d->map[mob->loc[dim_y]][mob->loc[dim_x]] = ter_floor_hall;
      }
    }
  }
  if (mob->loc[dim_y] == d->pc[dim_y] && mob->loc[dim_x] == d->pc[dim_x]) {
    end_game(0, d, all_mobs, heap);
  }

}

int event_sim(dungeon_t *d, monster_t *m, int num_monsters, int32_t tunneling[DUNGEON_Y][DUNGEON_X], int32_t non_tunneling[DUNGEON_Y][DUNGEON_X]) {
  heap_t h;
  uint32_t i;

  heap_init(&h, monster_cmp, NULL);

  for (i = 0; i < num_monsters; i++)
  {
      m[i].hn = heap_insert(&h, &m[i]);
  }

  monster_t *mob;
  while ((mob = (monster_t*) heap_remove_min(&h))) {
    if (mob->priority < 0 || mob->priority >= UINT64_MAX) {
      //TODO: add method to reset priorities and reinsert them into the queue
    }
    else if ((heap_peek_min(&h) == NULL) && mob->symbol == '@') {
      end_game(1, d, m, &h);
      return 0;
    }
    else if (mob->symbol == '@') {
      // do nothing, I'm lazy to make the PC do stuff
      render(d, m, num_monsters);
      // sleep for 250000 micro seconds
      mob->priority += 1000 / mob->speed;
      mob->hn = heap_insert(&h, mob);
      usleep(850000);
    } else {
      // now move that monster
      move_monster(&h, d, mob, num_monsters, m, tunneling, non_tunneling);

      // if mob is out obounds then kill it
      if (mob->loc[dim_x] > DUNGEON_X - 1 || mob->loc[dim_x] < 2) {
        mob->hn = NULL;
      } else if (mob->loc[dim_y] > DUNGEON_Y - 1 || mob->loc[dim_y] < 2) {
        mob->hn = NULL;
      } else {
        mob->priority += 1000 / mob->speed;
        mob->hn = heap_insert(&h, mob);
      }
    }
  }
  heap_delete(&h);
  return 0;
}

void init_distmaps(dungeon_t *d, int32_t tunneling[DUNGEON_Y][DUNGEON_X], int32_t non_tunneling[DUNGEON_Y][DUNGEON_X]) {
  tunneling_dijkstras(d, tunneling);
  non_tunneling_dijkstras(d, non_tunneling);
}

void start_routines(dungeon_t *d, int num_monsters) {
  int num_mobs = NUM_MOBS;
  if (num_monsters != 0) {
    num_mobs = num_monsters;
  }

  int32_t tunneling[DUNGEON_Y][DUNGEON_X];
  int32_t non_tunneling[DUNGEON_Y][DUNGEON_X];
  monster_t *mobs = malloc(sizeof(monster_t) * num_mobs);

  init_mobs(d, mobs, num_mobs);
  event_sim(d, mobs, num_mobs, tunneling, non_tunneling);
}
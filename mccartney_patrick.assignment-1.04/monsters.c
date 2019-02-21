#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "heap.h"

#define DUNGEON_X 80
#define DUNGEON_Y 21
#define INFINITY 214748364
#define NUM_MOBS 13
#define PC_SPEED 10
#define MAX_SPEED 20
#define MIN_SPEED 5
#define NUM_ATTRIBUTES 4

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
  ter_stairs_down
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
  char attributes[NUM_ATTRIBUTES];
  uint8_t speed;
  char type;
  heap_node_t *hn;
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
  pair_t pc;
  pc[dim_x] = d->pc[dim_x];
  pc[dim_y] = d->pc[dim_y];
  *mobs[0].loc = *pc;
  mobs[0].speed = 10;
  mobs[0].type = '@';

  int i, j = 0;
  for (i = 1; i < num_monsters; i++) {
    // randomize location, speed, type, and attributes
    pair_t location;
    location[dim_x] = rand_range(1, DUNGEON_X - 1);
    location[dim_y] = rand_range(1, DUNGEON_Y - 1);
    *mobs[i].loc = *location;
    *mobs[i].pc_loc = *pc;
    mobs[i].speed = rand_range(5, 20);

    //set the type to either a random number or a character
    if (rand_range(0,1)) {
      mobs[i].type = rand_range(0, 9);
    } else {
      mobs[i].type = (rand()%(90-65))+65; // between A and Z
    }

    //sets the monster's attributes to either a 0 or 1
    for (j = 0; j < NUM_ATTRIBUTES; j++) {
      int num = rand_range(0,1);
      if (num) {
        mobs[i].attributes[j] = '1';
      } else {
        mobs[i].attributes[j] = '0';
      }
    }
  }
}

static int32_t monster_cmp(const void *key, const void *with)
{
  return ((1000/((monster_t *)key)->speed) - (1000/((monster_t *)with)->speed));
}

int event_sim(dungeon_t *d, monster_t *m, int num_monsters) {
  // Every character (PC and NPCs) generates an event when it is created.
  // that event has a time, when it will occur
  // (based on the current game turn and the character’s speed)
  // and goes in a priority queue,prioritized on the event time.
  // Each character’s move event occurs every floor(1000/speed) turns
  // integer math, so this is just a normal division;
  //
  // Each time a character’s move event is removed from the queue, that character gets to move,
  // and a new move event is placed in the queue for its next turn.
  printf("nummobs = %d\n", num_monsters);

  heap_t h;
  uint32_t i;

  heap_init(&h, monster_cmp, NULL);

  for (i = 0; i < num_monsters; i++)
  {
      printf("Put in %c:%u, at %u, %u \n", m[i].type, m[i].speed, m[i].loc[dim_x], m[i].loc[dim_y]);
      m[i].hn = heap_insert(&h, &m[i]);
  }

  printf("\n\n");

  monster_t *mob;
  while ((mob = (monster_t*) heap_remove_min(&h))) {
    printf("Took out %c:%u, at %u, %u \n", mob->type, mob->speed, mob->loc[dim_x], mob->loc[dim_y]);
  }
  return 0;
}

void start_routines(dungeon_t *d, int num_monsters) {
  int num_mobs = NUM_MOBS;
  if (num_monsters != 0) {
    num_mobs = num_monsters;
  }

  monster_t mobs[num_mobs];
  init_mobs(d, mobs, num_mobs);
  event_sim(d, mobs, num_mobs);
}
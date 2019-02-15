#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "heap.h"

#define DUNGEON_X 80
#define DUNGEON_Y 21
#define MIN_ROOMS 6
#define MAX_ROOMS 10
#define ROOM_MIN_X 4
#define ROOM_MIN_Y 3
#define ROOM_MAX_X 20
#define ROOM_MAX_Y 15
#define SAVE_DIR ".rlg327"
#define DUNGEON_SAVE_FILE "dungeon"
#define DUNGEON_SAVE_SEMANTIC "RLG327-" TERM
#define DUNGEON_SAVE_VERSION 0U

#define mappair(pair) (d->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (d->map[y][x])
#define hardnesspair(pair) (d->hardness[pair[dim_y]][pair[dim_x]])
#define hardnessxy(x, y) (d->hardness[y][x])

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

static int32_t corridor_path_cmp(const void *key, const void *with)
{
  return ((corridor_path_t *)key)->cost - ((corridor_path_t *)with)->cost;
}

static int tunneling_dijkstras(dungeon_t *d, pair_t from)
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
      }
    }
    initialized = 1;
  }

  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      path[y][x].cost = INT_MAX;
    }
  }

  heap_init(&h, corridor_path_cmp, NULL);

  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (mapxy(x, y) != ter_wall_immutable)
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

    // make 4 more of these blocks for the other positions we check
    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost >
         p->cost + hardnesspair(p->pos)))
    {
      path[p->pos[dim_y] - 1][p->pos[dim_x]].cost =
          p->cost + hardnesspair(p->pos);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]]
                                               .hn);

      // store cost in your output array
      // actually store both maps in an array
      //
      //
      //
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] - 1].cost >
         p->cost + hardnesspair(p->pos)))
    {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
          p->cost + hardnesspair(p->pos);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] - 1]
                                               .hn);
    }
    if ((path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]][p->pos[dim_x] + 1].cost >
         p->cost + hardnesspair(p->pos)))
    {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
          p->cost + hardnesspair(p->pos);
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]]
                                           [p->pos[dim_x] + 1]
                                               .hn);
    }

    // check if path at y+1 and x exists in the heap
    // then compare to see if path at y+1 and x costs more than the current cost of p
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost >
         p->cost + hardnesspair(p->pos)))
    {

      // at y+1,  x set the cost equal to the current cost plus the hardness at that point
      path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost + hardnesspair(p->pos);

      // TODO Store it in an array such as distmap[DUNGEON_Y][DUNGEON_X]




      // delete it from the heap
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
    }
  }
  return 0;
}

static int tunneling_distmap(dungeon_t *d, int distmap[DUNGEON_Y][DUNGEON_X]) {
  int y, x;
  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      pair_t pos;
      pos[dim_y] = y;
      pos[dim_x] = x;
      distmap[y][x] = tunneling_dijkstras(d, pos);
      printf("%d", distmap[y][x] % 10);
    }
  }
  return 0;
}

void generate_distmaps(dungeon_t *d)
{
  // make all distmaps in data structures and print them accordingly
  // TODO call dijkstras on all corridors and store them in an array
  int distmap[DUNGEON_Y][DUNGEON_X];

  tunneling_distmap(d, distmap);

}

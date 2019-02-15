#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "heap.h"

#define DUNGEON_X 80
#define DUNGEON_Y 21

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
        path[y][x].cost = INT_MAX;
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

static void tunneling_distmap(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
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
      if (distmap[y][x] == INT_MAX)
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
        path[y][x].cost = INT_MAX;
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

static void nontunneling_distmap(dungeon_t *d, int32_t distmap[DUNGEON_Y][DUNGEON_X])
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
      else if (distmap[y][x] >= INT_MAX || distmap[y][x] < 0) {
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

void generate_distmaps(dungeon_t *d)
{
  // 2d int array takes ~6kb of memory on the stack so it's not too bad
  int32_t distmap[DUNGEON_Y][DUNGEON_X];
  int32_t distmap2[DUNGEON_Y][DUNGEON_X];

  printf("\n");
  nontunneling_distmap(d, distmap2);
  printf("\n");
  tunneling_distmap(d, distmap);
}
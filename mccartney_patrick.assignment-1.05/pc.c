#include <stdlib.h>
#include <ncurses.h>
#include "string.h"

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"

void pc_delete(pc_t *pc)
{
  if (pc)
  {
    free(pc);
  }
}

uint32_t pc_is_alive(dungeon_t *d)
{
  return d->pc.alive;
}

void place_pc(dungeon_t *d)
{
  d->pc.position[dim_y] = rand_range(d->rooms->position[dim_y],
                                     (d->rooms->position[dim_y] +
                                      d->rooms->size[dim_y] - 1));
  d->pc.position[dim_x] = rand_range(d->rooms->position[dim_x],
                                     (d->rooms->position[dim_x] +
                                      d->rooms->size[dim_x] - 1));
}

void config_pc(dungeon_t *d)
{
  memset(&d->pc, 0, sizeof(d->pc));
  d->pc.symbol = '@';

  place_pc(d);

  d->pc.speed = PC_SPEED;
  d->pc.alive = 1;
  d->pc.sequence_number = 0;
  d->pc.pc = calloc(1, sizeof(*d->pc.pc));
  d->pc.npc = NULL;
  d->pc.kills[kill_direct] = d->pc.kills[kill_avenged] = 0;

  d->character[d->pc.position[dim_y]][d->pc.position[dim_x]] = &d->pc;

  dijkstra(d);
  dijkstra_tunnel(d);
}

/**
 *
 *  Gets the next position for the pc
 *
 */
uint32_t pc_next_pos(dungeon_t *d, pair_t dir)
{
  dir[dim_y] = dir[dim_x] = 0;

  int input = getch();
  mvprintw(0,0, "you pressed: %c", input);
  switch (input)
  {
  case KEY_UP:
  case 8:
  case 'k':
    // move UP
    dir[dim_y]--;
    break;
  case KEY_PPAGE:
  case 9:
  case 'u':
    // move UP-RIGHT
    dir[dim_y]--;
    dir[dim_x]++;
    break;
  case KEY_RIGHT:
  case 6:
  case 'l':
    // move RIGHT
    dir[dim_x]++;
    break;
  case KEY_NPAGE:
  case 3:
  case 'n':
    // move DOWN-RIGHT
    dir[dim_y]++;
    dir[dim_x]++;
    break;
  case KEY_DOWN:
  case 2:
  case 'j':
    // move DOWN
    dir[dim_y]++;
    break;
  case KEY_END:
  case 1:
  case 'b':
    // move DOWN-LEFT
    dir[dim_y]++;
    dir[dim_x]--;
    break;
  case KEY_LEFT:
  case 4:
  case 'h':
    //move LEFT
    dir[dim_x]--;
  case KEY_HOME:
  case 7:
  case 'y':
    // move UP-LEFT
    dir[dim_y]--;
    dir[dim_x]--;
    break;
  case '<':
    // try to go down the stair (if it exists)
    break;
  case '>':
    // try to go up the stair (if it exists)
    break;
  case KEY_B2:
  case ' ':
  case '.':
    // rest for this turn
    break;
  case 'm':
    // create a loop untill player presses escape
    // display a list of monsters relative to the player
    // handle up and down arrows in this loop
    break;
  case 'Q':
    // quit the game
    d->pc.alive = 0;
    break;
  }

  

  return 0;
}

uint32_t pc_in_room(dungeon_t *d, uint32_t room)
{
  if ((room < d->num_rooms) &&
      (d->pc.position[dim_x] >= d->rooms[room].position[dim_x]) &&
      (d->pc.position[dim_x] < (d->rooms[room].position[dim_x] +
                                d->rooms[room].size[dim_x])) &&
      (d->pc.position[dim_y] >= d->rooms[room].position[dim_y]) &&
      (d->pc.position[dim_y] < (d->rooms[room].position[dim_y] +
                                d->rooms[room].size[dim_y])))
  {
    return 1;
  }

  return 0;
}

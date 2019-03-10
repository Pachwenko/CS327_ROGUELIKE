#include <stdlib.h>
#include <ncurses.h>
#include "string.h"

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"
#include "npc.h"

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
 *
 *  Checks if the PC is standing on a stair and generates a new dungeon is they are.
 *
 *
 *
 */
int enter_stairs(dungeon_t *d, char stair) {
  if ((d->map[d->pc.position[dim_y]][d->pc.position[dim_x]] == ter_stairs_up && stair == '<')
    || (d->map[d->pc.position[dim_y]][d->pc.position[dim_x]] == ter_stairs_down && stair == '>')) {
    // generate a new dungeon, but first delete all the old stuff.
    delete_dungeon(d);
    pc_delete(d->pc.pc);

    init_dungeon(d);
    gen_dungeon(d);
    config_pc(d);
    gen_monsters(d);
    return 0;
  }
  return 1;
}

int display_monsters(dungeon_t *d) {
  // clear the screen
  int input;
  while (1) {
    input = getch();
    if (input == 27) {
      break;
    }

    // check if input is up or down, keep displaying the current monsters
    // on lines x { 1-22}


  }
  return 0;
}


uint32_t pc_next_pos(dungeon_t *d, pair_t dir)
{
  dir[dim_y] = dir[dim_x] = 0;
  int input = getch();
  mvprintw(DUNGEON_Y+2, 0, "you pressed: %c", input);
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
    break;
  case KEY_HOME:
  case 7:
  case 'y':
    // move UP-LEFT
    dir[dim_y]--;
    dir[dim_x]--;
    break;
  case '<':
    // try to go down the stair (if it exists)
    enter_stairs(d, '<');
    break;
  case '>':
    // try to go up the stair (if it exists)
    enter_stairs(d, '>');
    break;
  case KEY_B2:
  case 5:
  case ' ':
  case '.':
    // rest for this turn
    break;
  case 'm':
    display_monsters(d);
    pc_next_pos(d, dir);
    break;
  case 'Q':
    // quit the game
    d->pc.alive = 0;
    break;
  default:
    // when a incorrect option is selected, try again
    pc_next_pos(d, dir);
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

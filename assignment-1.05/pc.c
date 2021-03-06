#include <stdlib.h>
#include <ncurses.h>
#include "string.h"

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"
#include "npc.h"

// to catch ctrl+c input, https://stackoverflow.com/questions/9750588/how-to-get-ctrl-shift-or-alt-with-getch-ncurses
#define CTRL(c) ((c) & 037)


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
int enter_stairs(dungeon_t *d, char stair)
{
  if ((d->map[d->pc.position[dim_y]][d->pc.position[dim_x]] == ter_stairs_up && stair == '<') || (d->map[d->pc.position[dim_y]][d->pc.position[dim_x]] == ter_stairs_down && stair == '>'))
  {
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

/**
 *
 *
 * Iterates though the dungeon and creates an array of npcs/monsters
 *
 */
character_t **getMonsters(dungeon_t *d)
{
  int index, y, x = 0;
  character_t **mobs = malloc(sizeof(character_t) * d->num_monsters);
  for (y = 0; y < DUNGEON_Y; y++)
  {
    for (x = 0; x < DUNGEON_X; x++)
    {
      if (d->character[y][x] && d->character[y][x]->npc)
      {
        mobs[index] = d->character[y][x];
        index++;
      }
    }
  }
  return mobs;
}

/**
 *
 * clears the screen and display monster positions from start to end
 * on line 1-21
 *
 */
void list_mobs(character_t *mobs[], int start, int end, dungeon_t *d)
{
  clear();
  int i;
  // displaying the current monsters on lines x { 1-22 }
  mvprintw(0,0, "           Number of monsters: %d", d->num_monsters - 1);
  int y = 1;
  int ymax = 22;
  for (i = start; i < end && mobs[i] && y < ymax; i++, y++)
  {
    int ydif = mobs[i]->position[dim_y] - d->pc.position[dim_y];
    int xdif = mobs[i]->position[dim_x] - d->pc.position[dim_x];
    char *ydir;
    char *xdir;
    if (ydif < 0) {
      ydir = "south";
      ydif = ydif * -1;
    } else {
      ydir = "north";
    }
    if (xdif < 0) {
      xdir = "west";
      xdif = xdif * -1;
    } else {
      xdir = "east";
    }
    mvprintw(y, 0, "           %c (%3d %5s, %3d %5s) #%d",
             mobs[i]->symbol, ydif, ydir, xdif, xdir, i);
  }
  refresh();
}


/**
 *
 *  Interface to use when displaying the list of monsters.
 *
 */
int display_monsters(dungeon_t *d)
{
  // first, get the monsters into an array we can use to display
  character_t **mobs = getMonsters(d);
  // display mob list starting at 0
  int listStart = 0;
  int listEnd = d->num_monsters;
  int allowScrolling = 0;
  if (d->num_monsters > 21)
  {
    listEnd = 21;
    allowScrolling = 1;
  }
  list_mobs(mobs, listStart, listEnd, d);
  int input;
  while (1)
  {
    input = getch();
    switch (input)
    {
    case KEY_UP:
    case 8:
    case 'k':
      // move the list backwards
      if (allowScrolling && listStart > 0) {
        mvprintw(23,0, "DOING SCROLL BACKWARDZ");
        listStart--;
        listEnd--;
        list_mobs(mobs, listStart, listEnd, d);
      }
      break;
    case 27:
    case CTRL('c'):
      render_dungeon(d);
      return 0;
      break;
    case KEY_DOWN:
    case 2:
    case 'j':
      // move the list forwards
      if (allowScrolling && listEnd < d->num_monsters) {
        mvprintw(23,0, "DOING SCROLL FORWARDZ");
        listStart++;
        listEnd++;
        list_mobs(mobs, listStart, listEnd, d);
      }
      break;
    }
  }
  render_dungeon(d);
  return 0;
}

/**
 *
 *
 * Code originally by Sheaffer, but modified to use NCurses instead of putchar
 *
 */
uint32_t pc_next_pos(dungeon_t *d, pair_t dir)
{
  dir[dim_y] = dir[dim_x] = 0;
  int input = getch();
  mvprintw(DUNGEON_Y + 2, 0, "you pressed: %c", input);
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

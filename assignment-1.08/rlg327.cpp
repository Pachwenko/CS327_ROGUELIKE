#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <fstream> //file streaming
#include <string>
#include <vector>

#include "dungeon.h"
#include "pc.h"
#include "npc.h"
#include "move.h"
#include "utils.h"
#include "io.h"

using namespace std;


const char *victory =
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
    "                                   You win!\n\n";

const char *tombstone =
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
    "            You're dead.  Better luck in the next life.\n\n\n";

void usage(char *name)
{
  fprintf(stderr,
          "Usage: %s [-r|--rand <seed>] [-l|--load [<file>]]\n"
          "          [-s|--save [<file>]] [-i|--image <pgm file>]\n"
          "          [-n|--nummon <count>]\n",
          name);

  exit(-1);
}


class dice {
  public:
  int base;
  int rolls;
  int sides;
};

class monster_desc {
  public:
  string name;
  char symbol;
  string color;
  string desc;
  dice speed;
  dice damage;
  dice hp;
  int rarity;
  string abilities;
};

/**
 *
 *  Reads the file and stores the needed information using the
 *  classes created. After doing so it reads all mobs and prints them
 *  to standard out in the order specified in the assignment pdf
 *
 */
int parse_monster_file()
{
  char *homepath = getenv("HOME");
  char *file_loc = strdup("/.rlg327/monster_desc.txt");
  char *filepath = (char *)malloc(strlen(homepath) + strlen(file_loc) + 1);
  strcat(filepath, homepath);
  strcat(filepath, file_loc);
  //printf("%s\n", filepath);

  ifstream f(filepath); // a input file stream
  string s;
  getline(f, s);
  if (s.compare("RLG327 MONSTER DESCRIPTION 1"))
  {
    fprintf(stderr, "First line of file is not \"RLG327 MONSTER DESCRIPTION 1\" Exiting\n");
    return 1;
  }

  int index, is_reading_newmob, got_name, got_symb, got_color, got_abil,
    got_speed, got_dam, got_hp, got_rrty, got_desc = 0;

  string first_token;
  monster_desc mobs[100];
  vector<monster_desc> mobz;

  while (f.is_open() && getline(f, s))
  {
    //cout << s << endl;
    int end_first_token = s.find_first_of(" \t")+1;
    first_token = s.substr(0, end_first_token-1);
    //cout << "first token is: " << first_token << endl;


    if (s.compare("BEGIN MONSTER") == 0 && !is_reading_newmob)
    {
      monster_desc newmob;
      mobz.push_back(newmob);
      // new mob so malloc space for another one
      //printf("began new monster\n");
      is_reading_newmob = 1;
    }
    else if (s.length() == 0)
    {
      //printf("newline\n");
    }
    else if (s.compare("END") == 0) {
      is_reading_newmob = 0;
      if (got_name && got_symb && got_color && got_abil && got_speed && got_dam
        && got_hp && got_rrty && got_desc) {
        index++;
      }
      got_name, got_symb, got_color, got_abil, got_speed, got_dam, got_hp,
          got_rrty, got_desc = 0;
    }
    else if (first_token.compare("NAME") == 0) {
      mobs[index].name = s.substr(end_first_token);
      got_name = 1;
      //cout << "mob name: " << mobs[index].name << endl;
    }
    else if (first_token.compare("SYMB") == 0) {
      mobs[index].symbol = s.back();
      got_symb = 1;
      //printf("symbol is: %c\n", s.back());
    }
    else if (first_token.compare("COLOR") == 0) {
      mobs[index].color = s.substr(end_first_token);
      got_color = 1;
      //cout << mobs[index].color << endl;
    }
    else if (first_token.compare("ABIL") == 0) {
      mobs[index].abilities = s.substr(end_first_token);
      got_abil = 1;
      //cout << mobs[index].abilities << endl;
    }
    else if (first_token.compare("SPEED") == 0) {
      // to parse, eat the first string and the following space
      // eat the first string up to the +, that is the base
      // eat the next string untill 'd', that is the number of rolls
      // eat the last number which is number of sides
      s = s.substr(end_first_token); // eat first token cause we want the dice data

      string rolls = s.substr(0, s.find_first_of("+"));
      s = s.substr(s.find_first_of("+")+1);
      mobs[index].speed.rolls = stoi(rolls);

      string base = s.substr(0, s.find_first_of("d"));
      s = s.substr(s.find_first_of("d")+1);
      mobs[index].speed.base = stoi(base);

      string sides = s;
      mobs[index].speed.sides = stoi(sides);

      got_speed = 1;
      //TODO: get speed dice
    }
    else if (first_token.compare("DAM") == 0) {
      s = s.substr(end_first_token); // eat first token cause we want the dice data

      string rolls = s.substr(0, s.find_first_of("+"));
      s = s.substr(s.find_first_of("+")+1);
      mobs[index].damage.rolls = stoi(rolls);

      string base = s.substr(0, s.find_first_of("d"));
      s = s.substr(s.find_first_of("d")+1);
      mobs[index].damage.base = stoi(base);

      string sides = s;
      mobs[index].damage.sides = stoi(sides);
      got_dam = 1;
      //TODO get damage dice
    }
    else if (first_token.compare("HP") == 0) {
      s = s.substr(end_first_token); // eat first token cause we want the dice data

      string rolls = s.substr(0, s.find_first_of("+"));
      s = s.substr(s.find_first_of("+")+1);
      mobs[index].hp.rolls = stoi(rolls);

      string base = s.substr(0, s.find_first_of("d"));
      s = s.substr(s.find_first_of("d")+1);
      mobs[index].hp.base = stoi(base);

      string sides = s;
      mobs[index].hp.sides = stoi(sides);

      got_hp = 1;
      //TODO get hp dice
    }
    else if (first_token.compare("RRTY") == 0) {
      string rarity = s.substr(end_first_token);
      mobs[index].rarity = stoi(rarity);
      got_rrty = 1;
      //cout << mobs[index].rarity << endl;
    }
    else if (first_token.compare("DESC") == 0) {
      // iterate untill hitting a line with only a period on it
      while (s.compare(".")) {
        string temp;
        getline(f, temp);
        if (temp.length() > 77) {
          fprintf(stderr, "Character desc is over 78 characters wide\n");
          break;
        } else {

          s = temp;
        }
        mobs[index].desc.append(s);
        mobs[index].desc.append("\n");
      }
      got_desc = 1;
    }
  }

  int i;
  for (i = 0; i < index; i++) {
    cout << mobs[i].name << endl;
    cout << mobs[i].desc;
    cout << mobs[i].symbol << endl;
    cout << mobs[i].color << endl;
    cout << mobs[i].speed.rolls << "+" << mobs[i].speed.base << "d" << mobs[i].speed.sides << endl;
    cout << mobs[i].abilities << endl;
    cout << mobs[i].hp.rolls << "+" << mobs[i].hp.base << "d" << mobs[i].hp.sides << endl;;
    cout << mobs[i].damage.rolls << "+" << mobs[i].damage.base << "d" << mobs[i].damage.sides << endl;
    cout << mobs[i].rarity << endl;
    printf("\n");
  }
  f.close();
  return 0;
}

int main(int argc, char *argv[])
{

  /* Read monster_desc.txt and exit */
  parse_monster_file();
  return 0;

  //   dungeon d;
  //   time_t seed;
  //   struct timeval tv;
  //   int32_t i;
  //   uint32_t do_load, do_save, do_seed, do_image, do_save_seed, do_save_image;
  //   uint32_t long_arg;
  //   char *save_file;
  //   char *load_file;
  //   char *pgm_file;

  //   /* Quiet a false positive from valgrind. */
  //   memset(&d, 0, sizeof (d));

  //   /* Default behavior: Seed with the time, generate a new dungeon, *
  //    * and don't write to disk.                                      */
  //   do_load = do_save = do_image = do_save_seed = do_save_image = 0;
  //   do_seed = 1;
  //   save_file = load_file = NULL;
  //   d.max_monsters = MAX_MONSTERS;

  //   /* The project spec requires '--load' and '--save'.  It's common  *
  //    * to have short and long forms of most switches (assuming you    *
  //    * don't run out of letters).  For now, we've got plenty.  Long   *
  //    * forms use whole words and take two dashes.  Short forms use an *
  //     * abbreviation after a single dash.  We'll add '--rand' (to     *
  //    * specify a random seed), which will take an argument of it's    *
  //    * own, and we'll add short forms for all three commands, '-l',   *
  //    * '-s', and '-r', respectively.  We're also going to allow an    *
  //    * optional argument to load to allow us to load non-default save *
  //    * files.  No means to save to non-default locations, however.    *
  //    * And the final switch, '--image', allows me to create a dungeon *
  //    * from a PGM image, so that I was able to create those more      *
  //    * interesting test dungeons for you.                             */

  //  if (argc > 1) {
  //     for (i = 1, long_arg = 0; i < argc; i++, long_arg = 0) {
  //       if (argv[i][0] == '-') { /* All switches start with a dash */
  //         if (argv[i][1] == '-') {
  //           argv[i]++;    /* Make the argument have a single dash so we can */
  //           long_arg = 1; /* handle long and short args at the same place.  */
  //         }
  //         switch (argv[i][1]) {
  //         case 'n':
  //           if ((!long_arg && argv[i][2]) ||
  //               (long_arg && strcmp(argv[i], "-nummon")) ||
  //               argc < ++i + 1 /* No more arguments */ ||
  //               !sscanf(argv[i], "%hu", &d.max_monsters)) {
  //             usage(argv[0]);
  //           }
  //           break;
  //         case 'r':
  //           if ((!long_arg && argv[i][2]) ||
  //               (long_arg && strcmp(argv[i], "-rand")) ||
  //               argc < ++i + 1 /* No more arguments */ ||
  //               !sscanf(argv[i], "%lu", &seed) /* Argument is not an integer */) {
  //             usage(argv[0]);
  //           }
  //           do_seed = 0;
  //           break;
  //         case 'l':
  //           if ((!long_arg && argv[i][2]) ||
  //               (long_arg && strcmp(argv[i], "-load"))) {
  //             usage(argv[0]);
  //           }
  //           do_load = 1;
  //           if ((argc > i + 1) && argv[i + 1][0] != '-') {
  //             /* There is another argument, and it's not a switch, so *
  //              * we'll treat it as a save file and try to load it.    */
  //             load_file = argv[++i];
  //           }
  //           break;
  //         case 's':
  //           if ((!long_arg && argv[i][2]) ||
  //               (long_arg && strcmp(argv[i], "-save"))) {
  //             usage(argv[0]);
  //           }
  //           do_save = 1;
  //           if ((argc > i + 1) && argv[i + 1][0] != '-') {
  //             /* There is another argument, and it's not a switch, so *
  //              * we'll save to it.  If it is "seed", we'll save to    *
  // 	     * <the current seed>.rlg327.  If it is "image", we'll  *
  // 	     * save to <the current image>.rlg327.                  */
  // 	    if (!strcmp(argv[++i], "seed")) {
  // 	      do_save_seed = 1;
  // 	      do_save_image = 0;
  // 	    } else if (!strcmp(argv[i], "image")) {
  // 	      do_save_image = 1;
  // 	      do_save_seed = 0;
  // 	    } else {
  // 	      save_file = argv[i];
  // 	    }
  //           }
  //           break;
  //         case 'i':
  //           if ((!long_arg && argv[i][2]) ||
  //               (long_arg && strcmp(argv[i], "-image"))) {
  //             usage(argv[0]);
  //           }
  //           do_image = 1;
  //           if ((argc > i + 1) && argv[i + 1][0] != '-') {
  //             /* There is another argument, and it's not a switch, so *
  //              * we'll treat it as a save file and try to load it.    */
  //             pgm_file = argv[++i];
  //           }
  //           break;
  //         default:
  //           usage(argv[0]);
  //         }
  //       } else { /* No dash */
  //         usage(argv[0]);
  //       }
  //     }
  //   }

  //   if (do_seed) {
  //     /* Allows me to generate more than one dungeon *
  //      * per second, as opposed to time().           */
  //     gettimeofday(&tv, NULL);
  //     seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  //   }

  //   srand(seed);

  //   io_init_terminal();
  //   init_dungeon(&d);

  //   if (do_load) {
  //     read_dungeon(&d, load_file);
  //   } else if (do_image) {
  //     read_pgm(&d, pgm_file);
  //   } else {
  //     gen_dungeon(&d);
  //   }

  //   /* Ignoring PC position in saved dungeons.  Not a bug. */
  //   config_pc(&d);
  //   gen_monsters(&d);

  //   io_display(&d);
  //   if (!do_load && !do_image) {
  //     io_queue_message("Seed is %u.", seed);
  //   }
  //   while (pc_is_alive(&d) && dungeon_has_npcs(&d) && !d.quit) {
  //     do_moves(&d);
  //   }
  //   io_display(&d);

  //   io_reset_terminal();

  //   if (do_save) {
  //     if (do_save_seed) {
  //        /* 10 bytes for number, plus dot, extention and null terminator. */
  //       save_file = (char *) malloc(18);
  //       sprintf(save_file, "%ld.rlg327", seed);
  //     }
  //     if (do_save_image) {
  //       if (!pgm_file) {
  // 	fprintf(stderr, "No image file was loaded.  Using default.\n");
  // 	do_save_image = 0;
  //       } else {
  // 	/* Extension of 3 characters longer than image extension + null. */
  // 	save_file = (char *) malloc(strlen(pgm_file) + 4);
  // 	strcpy(save_file, pgm_file);
  // 	strcpy(strchr(save_file, '.') + 1, "rlg327");
  //       }
  //     }
  //     write_dungeon(&d, save_file);

  //     if (do_save_seed || do_save_image) {
  //       free(save_file);
  //     }
  //   }

  //   printf("%s", pc_is_alive(&d) ? victory : tombstone);
  //   printf("You defended your life in the face of %u deadly beasts.\n"
  //          "You avenged the cruel and untimely murders of %u "
  //          "peaceful dungeon residents.\n",
  //          d.PC->kills[kill_direct], d.PC->kills[kill_avenged]);

  //   if (pc_is_alive(&d)) {
  //     /* If the PC is dead, it's in the move heap and will get automatically *
  //      * deleted when the heap destructs.  In that case, we can't call       *
  //      * delete_pc(), because it will lead to a double delete.               */
  //     character_delete(d.PC);
  //   }

  //   delete_dungeon(&d);

  //   return 0;
}

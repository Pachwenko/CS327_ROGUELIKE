#ifndef DISTMAPS_H
# define DISTMAS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "heap.h"

typedef struct dungeon dungeon_t;


void generate_distmaps(dungeon_t *d);


#endif
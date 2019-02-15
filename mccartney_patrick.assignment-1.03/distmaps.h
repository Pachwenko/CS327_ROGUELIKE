#ifndef DISTMAPS_H
# define DISTMAS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "heap.h"

// opaque reference
typedef struct dungeon dungeon_t;

// interface for creating distance maps for tunneling and non-tunneling
// monsters. Holds the distance map arrays. Only useful for assignment1.03
void generate_distmaps(dungeon_t *d);


#endif
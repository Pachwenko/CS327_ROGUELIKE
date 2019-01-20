#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define size 1024
/* Do not modify write_pgm() or read_pgm() */
int write_pgm(char *file, void *image, uint32_t x, uint32_t y)
{
  FILE *o;

  if (!(o = fopen(file, "w")))
  {
    perror(file);

    return -1;
  }

  fprintf(o, "P5\n%u %u\n255\n", x, y);

  /* Assume input data is correctly formatted. *
   * There's no way to handle it, otherwise.   */

  if (fwrite(image, 1, x * y, o) != (x * y))
  {
    perror("fwrite");
    fclose(o);

    return -1;
  }

  fclose(o);

  return 0;
}

/* A better implementation of this function would read the image dimensions *
 * from the input and allocate the storage, setting x and y so that the     *
 * user can determine the size of the file at runtime.  In order to         *
 * minimize complication, I've written this version to require the user to  *
 * know the size of the image in advance.                                   */
int read_pgm(char *file, void *image, uint32_t x, uint32_t y)
{
  FILE *f;
  char s[80];
  unsigned i, j;

  if (!(f = fopen(file, "r")))
  {
    perror(file);

    return -1;
  }

  if (!fgets(s, 80, f) || strncmp(s, "P5", 2))
  {
    fprintf(stderr, "Expected P6\n");

    return -1;
  }

  /* Eat comments */
  do
  {
    fgets(s, 80, f);
  } while (s[0] == '#');

  if (sscanf(s, "%u %u", &i, &j) != 2 || i != x || j != y)
  {
    fprintf(stderr, "Expected x and y dimensions %u %u\n", x, y);
    fclose(f);

    return -1;
  }

  /* Eat comments */
  do
  {
    fgets(s, 80, f);
  } while (s[0] == '#');

  if (strncmp(s, "255", 3))
  {
    fprintf(stderr, "Expected 255\n");
    fclose(f);

    return -1;
  }

  if (fread(image, 1, x * y, f) != x * y)
  {
    perror("fread");
    fclose(f);

    return -1;
  }

  fclose(f);

  return 0;
}


double convoluteX(int8_t array[size][size], int row, int col) {
  //calculates by top down, col col col, could do any order you like
  // Matrix is of the value:
  // [-1 0 +1]
  // [-2 0 +2]
  // [-1 0 +1]
  // therefore middle column will be 0, does not need to be calculated
  double col1 = (array[row - 1][col - 1] * -1.0)
    + (array[row][col - 1] * -2.0) + (array[row + 1][col] * -1.0);

  double col3 = (array[row - 1][col + 1] * 1.0) +
    (array[row][col + 1] * 2.0) + (array[row + 1][col + 1] * 1.0);

  return col1 + col3;
}

double convoluteY(int8_t array[size][size], int row, int col) {
  // calculated row by row, middle row sums to 0 so no need to calculate
  // Matrix is of the value:
  // [-1 -2 -1]
  // [ 0  0  0]
  // [+1 +2 +1]
  double row1 = (array[row-1][col-1] * -1.0) +
    (array[row-1][col] * -2.0) + (array[row-1][col+1] * -1.0);

  double row3 = (array[row+1][col-1] * 1.0) +
    (array[row+1][col] * 2.0) + (array[row+1][col+1] * 1.0);

  return row1 + row3;
}

int main(int argc, char *argv[])
{
  int8_t image[size][size];
  int8_t out[size][size];

  int debug = 1;

  if (debug)
  {
    printf("%s\n", argv[1]);
  }

  char *sobel = strdup(argv[1]);

  // remove .edge and concat the correct name for our output file
  sobel[strlen(sobel) - 4] = '\0';
  strcat(sobel, ".edge.pgm");

  if (debug)
  {
    printf("%s %li\n", sobel, strlen(sobel));
  }

  read_pgm(argv[1], image, 1024, 1024);



  int i, j, r, c = 0;
  for (i = 0; i < size; i++)
  {
    for (j = 0; j < size; j++)
    {
      out[i][j] = 0; // in another language we might not need to itialize all the values ourselves
    }
  }



  i = j = 0; // re-sets both i and j = 0
  double accumulator = 0.0;

  for (r = 1; i < size - 1; i++)
  {
    for (c = 1; j < size - 1; j++)
    {
      double Ox = convoluteX(image, i, j);

    }
  }

  write_pgm(sobel, image, 1024, 1024);

  return 0;
  /* Example usage of PGM functions */
  /* This assumes that motorcycle.pgm is a pgm image of size 1024x1024 */
  // read_pgm("motorcycle.pgm", image, 1024, 1024);

  /* After processing the image and storing your output in "out", write *
   * to motorcycle.edge.pgm.                                            */
  // write_pgm("motorcycle.edge.pgm", out, 1024, 1024);
}

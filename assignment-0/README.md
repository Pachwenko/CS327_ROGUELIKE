# Sobel Filter

Performs a sobel filter on a pgm file of size 1024x1024 which is grayscale.
The input file is given as a .pgm such as "./sobel picture.pgm" and the program
will then create a "picture.edge.pgm" file with the sobel filter effectively applied.

## Example Useage

```bash
# compile the program with make
make

# have a grayscale 1024x1024 image named "image.pgm"
# which is in the current directory next to the compiled program
./sobel image.pgm
```

# Dungeon Generator

Draws a 80x21 dungeon where the borders are considered immuteable, 8 rooms by default, all rooms connected to eachother by cooridors.
In the first room (index 0) there is a upward staircase and in the last room there is a downward staircase.

Useage:

```bash
make
./generator
```

## Number of rooms

Change the number of rooms by changing this line:

```C
#define MAXROOMS 8
```
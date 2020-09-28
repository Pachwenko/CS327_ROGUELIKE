# Notes about getting started on hw1.01

No requirements for looking nice.

80x21 where the border is immuteable (whats the best way to do that?)

6 rooms minimum of 3x4

## Step 1

Use a 2d character array, with y coordinate first e.g., char dungeon[y][x] or char dungeon [21][80]
Negative y going up, positive y going down. Due to historical reasons. This is better performance due to how memory is allocated.

Note: a char is a 1 byte integer

Rooms should be built with structs, but for this assignment an array is used. The y direction contains the room, the x direction contains properties of the rooms. "char rooms[MAX_ROOMS][4]" the 4 contains the top left corner and then the x size and y size.

Now, use random numbers to generate the location of the room. Change all the spaces to dots in that room. Create another room and do the same. Keep doing until you get to 6. Make sure to check if the location and size picked overlaps a room with a padding of 1 space.

## Step 2

Now that you have 6 rooms satisying the conditions (no overlap with 1 space of padding). Now generate only 2 rooms to start working on the coorridors.

The easiest way to connect them together is start at room 0 which connects to room 1. Take the top left corner of room 0 and 1. Use the y of 0 and x of 1, the y of 1 and x of 0 and use that as the midpoint. Assume there are always 2 coorridors. **
The x position you move is given by x1-x0/|x1-x0|** which always gives -1 or 1, add that to x0 as the position you draw the cooridor. Do the same for the y values.

## Step 3

Add stairs in a cooridor or room or right next to one of them (so it's accessible). Can be literally anywhere. Stairs can be walked over just like floors.

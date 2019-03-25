# Terminal Based Roguelike in C and C++

A roguelike game for Com S 327 at Iowa State University. Built in C and C++ with Ncurses.

Each folder represents the first to last assignment which halfway through transitions from C to C++. The last assignment in C is 1.05, all subequent assignments are in C++.
Each assignment builds on the last so the code may be viewed as a story through commit logs.

## Dependencies

### GCC and G++ To compile or make .c and .cpp files respectively

GCC-8 (for Ubuntu, sudo apt install gcc)
G++-8 (for Ubuntu, sudo apt install g++)

Alternatively, install the "build-essential" package.

### NCurses

For Ubuntu, sudo apt install libncurses5-dev

There are more if desired such as debugging packages and can be found with "apt-cache search libncurses".


## How to Build and Run

```bash
make
./rlg327
```
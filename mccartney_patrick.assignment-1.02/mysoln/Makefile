all: generator main

main: main.c
	gcc -ggdb3 -O0 -Q main.c -o main

generator: generator.c
	gcc -ggdb3 -O0 -Q generator.c -o generator -lm

#da was sugguested but removed since it created 15 files

clean:
	rm -f generator main *~

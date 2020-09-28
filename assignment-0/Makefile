# all: sobel
# all rule is unneeded for only one source file

# note to self, -lm links the math library
# Wall and Werror unused because of debugging and such (unused variables cause warnings)
sobel: sobel.c
	gcc -ggdb3 sobel.c -o sobel -lm


clean:
	rm -f sobel bigger_digger.edge.pgm motorcycle.edge.pgm *~

clean2:
	rm -f sobel *~

run: sobel.c
	gcc sobel.c -o sobel -lm
	./run.sh

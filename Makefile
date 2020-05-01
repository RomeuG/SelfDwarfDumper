all:
	gcc -ggdb3 -O0 src/main.c -o selfdwarfdumper -ldwarf

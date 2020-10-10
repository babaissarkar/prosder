prosder : prosder.c
	gcc `pkg-config --cflags cairo gtk+-3.0` -no-pie -g prosder.c `pkg-config --libs cairo gtk+-3.0` -lm -o prosder

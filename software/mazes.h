#ifndef _MAZES_H

#define NUM_MAZES 3

//textures
#define B 1 // bluestone
#define C 2 // colorstone
#define E 3 // eagle (end level)
#define G 4 // greystone
#define M 5 // mossy
#define P 6 // purplestone
#define R 7 // redbrick
#define W 8 // wood

#define O 0 // opening (floor)

typedef struct {
	int width;
	int height;
	int area;
	char name[20];
	short map[1296]; //make this largest area of any map
} maze_t;

#endif

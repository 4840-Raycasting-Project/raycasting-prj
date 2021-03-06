/*
Adapted from https://permadi.com/activity/ray-casting-game-engine-demo/

TODO:

1. Texture mapping

3. Bit shift operations for speedup
4. Other speedup optimizations

6. Thread to update the future hardware
7. usb controller input?
8. sprites?
9. Floor / ceiling color gradients or (texture -> harder)

*/

#include "framebuffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usbkeyboard.h"
#include <pthread.h>
#include <stdbool.h> 
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>


// size of tile (wall height) - best to make some power of 2
#define TILE_SIZE 64
#define WALL_HEIGHT 64
#define PROJECTIONPLANEWIDTH 1024
#define PROJECTIONPLANEHEIGHT 768		//VGA module from lab3 being built on
#define ANGLE60 PROJECTIONPLANEWIDTH
#define ANGLE30 (ANGLE60/2)
#define ANGLE15 (ANGLE30/2)
#define ANGLE90 (ANGLE30*3)
#define ANGLE180 (ANGLE90*2)
#define ANGLE270 (ANGLE90*3)
#define ANGLE360 (ANGLE60*6)
#define ANGLE0 0
#define ANGLE5 (ANGLE30/6)
#define ANGLE10 (ANGLE5*2)

//best to make this some power of 2
#define COLUMN_WIDTH 4


//some controller defaults
#define CONTROLLER_DEFAULT 0x7F

//number of levels
#define NUMLEVELS 3
 
//default selected level for the game
int levels = 0;

//game levels
char *level[NUMLEVELS] = {"1.MUDD", "2.CESPR", "3.PUPIN"}; 

// precomputed trigonometric tables
float fSinTable[ANGLE360+1];
float fISinTable[ANGLE360+1];
float fCosTable[ANGLE360+1];
float fICosTable[ANGLE360+1];
float fTanTable[ANGLE360+1];
float fITanTable[ANGLE360+1];
float fFishTable[ANGLE360+1];
float fXStepTable[ANGLE360+1];
float fYStepTable[ANGLE360+1];

// player's attributes
int fPlayerX = 100; int tmpPlayerX;
int fPlayerY = 160; int tmpPlayerY;
int fPlayerArc = ANGLE0;
int fPlayerDistanceToTheProjectionPlane = 277;
int fPlayerHeight = 32;
int fPlayerSpeed = 8;
int fProjectionPlaneYCenter = PROJECTIONPLANEHEIGHT / 2;

// movement flag
bool fKeyUp = false;
bool fKeyDown = false;
bool fKeyLeft = false;
bool fKeyRight = false;

// 2 dimensional map
static const uint8_t W = 1; // wall
static const uint8_t O = 0; // opening
static const uint8_t MAP_WIDTH = 12;
static const uint8_t MAP_HEIGHT = 12;

uint8_t *fMap;

struct libusb_device_handle *keyboard;
uint8_t endpoint_address;

//function signatures
void render();
void create_tables();
float arc_to_rad(float);
void handle_key_press(struct usb_keyboard_packet *, bool);
void game_start_stop();
void level_select();

bool up_pressed, down_pressed, left_pressed, right_pressed, enter_pressed;

pthread_t keyboard_thread;
void *keyboard_thread_f(void *);

int main() {
    
    fbopen();
    fb_clear_screen();

    create_tables();


    /* Open the keyboard */
    if ((keyboard = openkeyboard(&endpoint_address)) == NULL) {
   fprintf(stderr, "Did not find a keyboard\n");
        exit(1);
    }
	
	//start keyboard thread
   pthread_create(&keyboard_thread, NULL, keyboard_thread_f, NULL);

    /*basic logic implemented, need to work on levels maze*/
    level_select();

    game_start_stop();

	
    fb_clear_screen();

    usleep(20000);
	render();
	
	int map_size = MAP_HEIGHT * MAP_WIDTH;

    while(true) {

		//uint8_t keycode = get_last_keycode(packet.keycode);
		//char gameplay_key = get_gameplay_key(keycode);

		// rotate left
		if(left_pressed) {
			if((fPlayerArc -= ANGLE5) < ANGLE0)
				fPlayerArc += ANGLE360;
		}
		
		// rotate right
		else if(right_pressed) {
			if((fPlayerArc += ANGLE5) >= ANGLE360)
				fPlayerArc -= ANGLE360;
		}

			//  _____     _
			// |\ arc     |
			// |  \       y
			// |    \     |
		//            -
			// |--x--|  
			//
			//  sin(arc)=y/diagonal
			//  cos(arc)=x/diagonal   where diagonal=speed
		float playerXDir = fCosTable[fPlayerArc];
		float playerYDir = fSinTable[fPlayerArc];

		// move forward
		if(up_pressed) {
			
			tmpPlayerX = fPlayerX + (int)(playerXDir * fPlayerSpeed);
			tmpPlayerY = fPlayerY + (int)(playerYDir * fPlayerSpeed);
			
			int map_index = (tmpPlayerX / TILE_SIZE) + ((tmpPlayerY / TILE_SIZE) * MAP_HEIGHT);

			if(map_index < map_size && fMap[map_index] != W) {
				
				fPlayerX = tmpPlayerX;
				fPlayerY = tmpPlayerY;
			}
		}
		
		// move backward
		else if(down_pressed) {
			
			tmpPlayerX = fPlayerX - (int)(playerXDir * fPlayerSpeed);
			tmpPlayerY = fPlayerY - (int)(playerYDir * fPlayerSpeed);
			
			int map_index = (tmpPlayerX / TILE_SIZE) + ((tmpPlayerY / TILE_SIZE) * MAP_HEIGHT);
			
			if(map_index < map_size && fMap[map_index] != W) {
				
				fPlayerX = tmpPlayerX;
				fPlayerY = tmpPlayerY;
			}
		}

		//TODO only call if position changed
		render();
		
		usleep(20000);
    }

    fb_clear_screen(); 
	
    free(fMap);
	
    pthread_cancel(keyboard_thread);
    pthread_join(keyboard_thread, NULL);

    return 0;
}

void *keyboard_thread_f(void *ignored) {
	
	printf("keyboard thread called\n");

	int transferred;
	
	struct usb_keyboard_packet packet;
	char keystate[12];

	//uint8_t controller_packet [4][2] = {{0x7f,0x00},{0x7f,0xff},{0x00,0x7f},{0xff,0x7f}};
	
	while(true) {
		
		libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);

        if (transferred == sizeof(packet)) {

	      		sprintf(keystate, "%02x %02x %02x %02x %02x %02x %02x", packet.modifiers, packet.keycode[0],
		      	packet.keycode[1],packet.keycode[2],packet.keycode[3],packet.keycode[4],packet.keycode[5]);
	      		//fbputs(keystate, 6, 0,1);			//for debug

			//printf("%s\n",keystate);

			//if (packet.keycode[1] != CONTROLLER_DEFAULT || packet.keycode[2] != CONTROLLER_DEFAULT) { 

				//up_pressed 	= is_key_pressed(2,0x00, packet.keycode);
				//down_pressed 	= is_key_pressed(2,0xff, packet.keycode);
				//left_pressed 	= is_key_pressed(1,0x00, packet.keycode);
				//right_pressed 	= is_key_pressed(1,0xff, packet.keycode);

				up_pressed 	= is_key_pressed(0,0x52, packet.keycode);
				down_pressed 	= is_key_pressed(0,0x51, packet.keycode);
				left_pressed 	= is_key_pressed(0,0x50, packet.keycode);
				right_pressed 	= is_key_pressed(0,0x4f, packet.keycode);
				enter_pressed   = is_key_pressed(0,0x28, packet.keycode);
			//}
			/*else {
				up_pressed 	= false;
				down_pressed 	= false;
				left_pressed 	= false;
				right_pressed 	= false;
			}*/
		}
	}
  
	return NULL;
}

void render() {

    int verticalGrid;        // horizotal or vertical coordinate of intersection
    int horizontalGrid;      // theoritically, this will be multiple of TILE_SIZE
                                // , but some trick did here might cause
                                // the values off by 1
    int distToNextVerticalGrid; // how far to the next bound (this is multiple of
    int distToNextHorizontalGrid; // tile size)
    float xIntersection;  // x and y intersections
    float yIntersection;
    float distToNextXIntersection;
    float distToNextYIntersection;

    int xGridIndex;        // the current cell that the ray is in
    int yGridIndex;

    float distToVerticalGridBeingHit;      // the distance of the x and y ray intersections from
    float distToHorizontalGridBeingHit;      // the viewpoint

    int castArc, castColumn;

    castArc = fPlayerArc;
        // field of view is 60 degree with the point of view (player's direction in the middle)
        // 30  30
        //    ^
        //  \ | /
        //   \|/
        //    v
        // we will trace the rays starting from the leftmost ray
    castArc -= ANGLE30;
        // wrap around if necessary
    if(castArc < 0)
        castArc = ANGLE360 + castArc;

    for(castColumn=0; castColumn < PROJECTIONPLANEWIDTH; castColumn += COLUMN_WIDTH) {
        
        // ray is between 0 to 180 degree (1st and 2nd quadrant)
        // ray is facing down
        if(castArc > ANGLE0 && castArc < ANGLE180) {
                // truncuate then add to get the coordinate of the FIRST grid (horizontal
                // wall) that is in front of the player (this is in pixel unit)
                // ROUND DOWN
            horizontalGrid = (fPlayerY / TILE_SIZE) * TILE_SIZE + TILE_SIZE;

            // compute distance to the next horizontal wall
            distToNextHorizontalGrid = TILE_SIZE;

            float xtemp = fITanTable[castArc] * (horizontalGrid - fPlayerY);
                    // we can get the vertical distance to that wall by
                    // (horizontalGrid-GLplayerY)
                    // we can get the horizontal distance to that wall by
                    // 1/tan(arc)*verticalDistance
                    // find the x interception to that wall
            xIntersection = xtemp + fPlayerX;
        }
        
        // else, the ray is facing up
        else {

            horizontalGrid = (fPlayerY / TILE_SIZE) * TILE_SIZE;
            distToNextHorizontalGrid = -TILE_SIZE;

            float xtemp = fITanTable[castArc] * (horizontalGrid - fPlayerY);
            xIntersection = xtemp + fPlayerX;

            horizontalGrid--;
        }
        
        // LOOK FOR HORIZONTAL WALL
        if(castArc == ANGLE0 || castArc == ANGLE180)
            distToHorizontalGridBeingHit=__FLT_MAX__;//Float.MAX_VALUE;
        
        // else, move the ray until it hits a horizontal wall
        else {
            distToNextXIntersection = fXStepTable[castArc];

            while(true) {

                xGridIndex = (int)(xIntersection / TILE_SIZE);
                // in the picture, yGridIndex will be 1
                yGridIndex = (horizontalGrid / TILE_SIZE);

                if((xGridIndex >= MAP_WIDTH) ||
                    (yGridIndex >= MAP_HEIGHT) ||
                    xGridIndex < 0 || yGridIndex < 0) {

                    distToHorizontalGridBeingHit = __FLT_MAX__;
                    break;
                }
                else if ((fMap[yGridIndex*MAP_WIDTH + xGridIndex]) != O) {
                    distToHorizontalGridBeingHit  = (xIntersection-fPlayerX)*fICosTable[castArc];
                    break;
                }
                // else, the ray is not blocked, extend to the next block
                else {
                    xIntersection += distToNextXIntersection;
                    horizontalGrid += distToNextHorizontalGrid;
                }
            }
        }

        // FOLLOW X RAY
        if(castArc < ANGLE90 || castArc > ANGLE270) {

            verticalGrid = TILE_SIZE + (fPlayerX / TILE_SIZE) * TILE_SIZE;
            distToNextVerticalGrid = TILE_SIZE;

            float ytemp = fTanTable[castArc] * (verticalGrid - fPlayerX);
            yIntersection = ytemp + fPlayerY;
        }
        // RAY FACING LEFT
        else {
            
            verticalGrid = (fPlayerX/TILE_SIZE)*TILE_SIZE;
            distToNextVerticalGrid = -TILE_SIZE;

            float ytemp = fTanTable[castArc] * (verticalGrid - fPlayerX);
            yIntersection = ytemp + fPlayerY;

            verticalGrid--;
        }
        
        // LOOK FOR VERTICAL WALL
        if(castArc == ANGLE90 || castArc == ANGLE270) 
            distToVerticalGridBeingHit = __FLT_MAX__;
        
        else {
            
            distToNextYIntersection = fYStepTable[castArc];
        
            while(true) {
                // compute current map position to inspect
                xGridIndex = (verticalGrid / TILE_SIZE);
                yGridIndex = (int)(yIntersection / TILE_SIZE);

                if ((xGridIndex >= MAP_WIDTH) ||
                    (yGridIndex >= MAP_HEIGHT) ||
                    xGridIndex < 0 || yGridIndex < 0) {
                    distToVerticalGridBeingHit = __FLT_MAX__;
                    break;
                }
                else if ((fMap[yGridIndex * MAP_WIDTH + xGridIndex]) != O) {
                    distToVerticalGridBeingHit = (yIntersection - fPlayerY) * fISinTable[castArc];
                    break;
                }
                else  {
                    yIntersection += distToNextYIntersection;
                    verticalGrid += distToNextVerticalGrid;
                }
            }
        }

        //TODO replace below with a tuple system which will be decoded by software then hardware

        // DRAW THE WALL SLICE
        float scaleFactor;
        float dist;
        int topOfWall;   // used to compute the top and bottom of the sliver that
        int bottomOfWall;   // will be the staring point of floor and ceiling
		uint8_t wall_side; //0=x, 1=y
		uint8_t offset;
		
            // determine which ray strikes a closer wall.
            // if yray distance to the wall is closer, the yDistance will be shorter than
                // the xDistance
        if (distToHorizontalGridBeingHit < distToVerticalGridBeingHit) {
            
            // the next function call (drawRayOnMap()) is not a part of raycating rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToHorizontalGridBeingHit;
			wall_side = 0;
			offset = (int)xIntersection % TILE_SIZE;
        }
        // else, we use xray instead (meaning the vertical wall is closer than
        //   the horizontal wall)
        else {
            
            // the next function call (drawRayOnMap()) is not a part of raycasting rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToVerticalGridBeingHit;
			wall_side = 1;
			offset = (int)yIntersection % TILE_SIZE;
        }

        // correct distance (compensate for the fishbown effect)
        dist /= fFishTable[castColumn];
        // projected_wall_height/wall_height = fPlayerDistToProjectionPlane/dist;
        int projectedWallHeight = (int)(WALL_HEIGHT * (float)fPlayerDistanceToTheProjectionPlane / dist);
        bottomOfWall = fProjectionPlaneYCenter + (int)(projectedWallHeight * 0.5F);
        topOfWall = PROJECTIONPLANEHEIGHT - bottomOfWall;

        if(bottomOfWall >= PROJECTIONPLANEHEIGHT)
            bottomOfWall = PROJECTIONPLANEHEIGHT - 1;

        fb_draw_column(castColumn, topOfWall, COLUMN_WIDTH, projectedWallHeight, wall_side, offset);

        // TRACE THE NEXT RAY
        castArc += COLUMN_WIDTH;
        if (castArc >= ANGLE360)
            castArc -= ANGLE360;
    }
}

void game_start_stop(){

    	fb_clear_screen();
	for (int i=0;i<5;i++){
		usleep(500000);	
		fbputs("STARTING GAME",11,25,1);
		usleep(500000);	
    		fb_clear_screen();
	}
}

void level_select() {

	/*
 	 * CHOOSE DIFFICULTY LEVEL
	 * 1. MUDD
	 * 2. CEPSR
	 * 3. PUPIN
	 *

	char *temp_levels[5];
	
	
	 while (1) {

		
		if(up_pressed) {
			if (levels - 1 == -1)
				continue;
			levels--;
			printf("levels: %d\n",levels);
			for(int i=0;i<NUMLEVELS;i++){
				temp_levels[i] = level[i];
			}
		}
		else if(down_pressed) {
			if (levels + 1 == NUMLEVELS)
				continue;
			levels++;
			printf("levels: %d\n",levels);
			for(int i=0;i<NUMLEVELS;i++){
				temp_levels[i] = level[i];
			}
		}
		else if (enter_pressed)
			break;

		switch(levels){
			
			case 0:
				sprintf(temp_levels[0],"%s %s",">",level[0]);
				break;	
			case 1:
				sprintf(temp_levels[1],"%s %s",">",level[1]);
				break;	
			case 2:
				sprintf(temp_levels[2],"%s %s",">",level[2]);
				break;	
			case 3:
				sprintf(temp_levels[3],"%s %s",">",level[3]);
				break;	
			case 4:
				sprintf(temp_levels[4],"%s %s",">",level[4]);
				break;	
			default:
				break;
		}

		 fbputs(temp_level[0],10,25,levels == 0 ? 1 : 0);
		 fbputs(temp_level[1],11,25,levels == 1 ? 1 : 0);
		 fbputs(temp_level[2],12,25,levels == 2 ? 1 : 0);
		 fbputs(temp_level[3],13,25,levels == 3 ? 1 : 0);
		 fbputs(temp_level[4],14,25,levels == 4 ? 1 : 0);

		 usleep(100000);
 	 }*/

	int ctr = 0;
	 while (1) {
		
		if (up_pressed == 0 && down_pressed == 0) {
			ctr = 0;
		}
		if (ctr != 0 /*&& loop_start_flag!=0*/)
			continue;

		if(up_pressed) {
			if (levels - 1 == -1)
				continue;
			levels--;
			printf("levels: %d\n",levels);
			ctr++;
		}
		else if(down_pressed) {
			if (levels + 1 == NUMLEVELS)
				continue;
			levels++;
			printf("levels: %d\n",levels);
			ctr++;
		}
		else if (enter_pressed)
			break;

		 fbputs(level[0],10,25,levels == 0 ? 1 : 0);
		 fbputs(level[1],11,25,levels == 1 ? 1 : 0);
		 fbputs(level[2],12,25,levels == 2 ? 1 : 0);

		 //usleep(100000);
 	 }
}

void create_tables() {

    int i;
    float radian;

    for (i=0; i <= ANGLE360; i++) {
        // get the radian value (the last addition is to avoid division by 0, try removing
            // that and you'll see a hole in the wall when a ray is at 0, 90, 180, or 270 degree)
        radian = arc_to_rad(i) + (float)(0.0001);
        fSinTable[i] = (float)sin(radian);
        fISinTable[i] = (1.0F / (fSinTable[i]));
        fCosTable[i] = (float)cos(radian);
        fICosTable[i] = (1.0F / (fCosTable[i]));
        fTanTable[i] = (float)tan(radian);
        fITanTable[i] = (1.0F / fTanTable[i]);

        //  you can see that the distance between xi is the same
        //  if we know the angle
        //  _____|_/next xi______________
        //       |
        //  ____/|next xi_________   slope = tan = height / dist between xi's
        //     / |
        //  __/__|_________  dist between xi = height/tan where height=tile size
        // old xi|
        //                  distance between xi = x_step[view_angle];
        //
        //
        // facine left
        // facing left
        if (i >= ANGLE90 && i < ANGLE270) {

            fXStepTable[i] = (float)(TILE_SIZE / fTanTable[i]);
            if (fXStepTable[i] > 0)
                fXStepTable[i] = -1 * fXStepTable[i];
        }
        // facing right
        else {

            fXStepTable[i] = (float)(TILE_SIZE / fTanTable[i]);
            if(fXStepTable[i] < 0)
                fXStepTable[i] = -1 * fXStepTable[i];
        }

        // FACING DOWN
        if (i >= ANGLE0 && i < ANGLE180) {

            fYStepTable[i] = (float)(TILE_SIZE * fTanTable[i]);
            if (fYStepTable[i] < 0)
                fYStepTable[i] = -1 * fYStepTable[i];
        }
        // FACING UP
        else {
            fYStepTable[i] = (float)(TILE_SIZE * fTanTable[i]);
            if (fYStepTable[i] > 0)
                fYStepTable[i] = -1 * fYStepTable[i];
        }
    }

    for (i = -ANGLE30; i <= ANGLE30; i++) {

        radian = arc_to_rad(i);
        // we don't have negative angle, so make it start at 0
        // this will give range 0 to 320
        fFishTable[i + ANGLE30] = (float)(1.0F / cos(radian));
    }
	
	fMap = (uint8_t*)calloc((MAP_HEIGHT * MAP_WIDTH), sizeof(uint8_t));
	
	uint8_t fMapCopy[] =
		{
			W,W,W,W,W,W,W,W,W,W,W,W,
			W,O,O,O,O,O,O,O,O,O,O,W,
			W,O,O,O,O,O,O,O,O,O,O,W,
			W,O,O,O,O,O,O,O,W,O,O,W,
			W,O,O,W,O,W,O,O,W,O,O,W,
			W,O,O,W,O,W,W,O,W,O,O,W,
			W,O,O,W,O,O,W,O,W,O,O,W,
			W,O,O,O,W,O,W,O,W,O,O,W,
			W,O,O,O,W,O,W,O,W,O,O,W,
			W,O,O,O,W,W,W,O,W,O,O,W,
			W,O,O,O,O,O,O,O,O,O,O,W,
			W,W,W,W,W,W,W,W,W,W,W,W
		};
		
	for (i=0; i < (MAP_HEIGHT * MAP_WIDTH); i++)
		fMap[i] = fMapCopy[i];
}

  //*******************************************************************//
//* Convert arc to radian
//*******************************************************************//
float arc_to_rad(float arc_angle) {
    return ((float)(arc_angle*M_PI)/(float)ANGLE180);    
}


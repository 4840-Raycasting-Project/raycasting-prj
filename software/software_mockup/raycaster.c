/*
Adapted from https://permadi.com/activity/ray-casting-game-engine-demo/

TODO:
0. keyboard
1. Collision detection
2. Texture mapping
3. Bit shift operations for speedup
4. Other speedup optimizations
5. fogging / transparency effect to show distance in z axis

6. Thread to update the future hardware
7. usb controller input?
8. sprites?

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

// size of tile (wall height)
#define TILE_SIZE 64
#define WALL_HEIGHT 64
#define PROJECTIONPLANEWIDTH 1024
#define PROJECTIONPLANEHEIGHT 768
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

#define COLUMN_RESOLUTION 5

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
int fPlayerX = 100;
int fPlayerY = 160;
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

bool up_pressed, down_pressed, left_pressed, right_pressed;

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
	
	render();

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
			fPlayerX += (int)(playerXDir * fPlayerSpeed);
			fPlayerY += (int)(playerYDir * fPlayerSpeed);
		}
		
		// move backward
		else if(down_pressed) {
			fPlayerX -= (int)(playerXDir*fPlayerSpeed);
			fPlayerY -= (int)(playerYDir*fPlayerSpeed);
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
	
	int transferred;
	
	struct usb_keyboard_packet packet;
	
	while(true) {
		
		libusb_interrupt_transfer(keyboard, endpoint_address,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);

        if (transferred == sizeof(packet)) {
			
			up_pressed = is_key_pressed(0x52, packet.keycode);
			down_pressed = is_key_pressed(0x51, packet.keycode);
			left_pressed = is_key_pressed(0x50, packet.keycode);
			right_pressed = is_key_pressed(0x4F, packet.keycode);
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
    if (castArc < 0)
        castArc = ANGLE360 + castArc;

    for (castColumn=0; castColumn < PROJECTIONPLANEWIDTH; castColumn += COLUMN_RESOLUTION) {
        
        // ray is between 0 to 180 degree (1st and 2nd quadrant)
        // ray is facing down
        if (castArc > ANGLE0 && castArc < ANGLE180) {
                // truncuate then add to get the coordinate of the FIRST grid (horizontal
                // wall) that is in front of the player (this is in pixel unit)
                // ROUND DOWN
            horizontalGrid = (fPlayerY / TILE_SIZE) * TILE_SIZE  + TILE_SIZE;

            // compute distance to the next horizontal wall
            distToNextHorizontalGrid = TILE_SIZE;

            float xtemp = fITanTable[castArc] * (horizontalGrid-fPlayerY);
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
        if (castArc == ANGLE0 || castArc == ANGLE180)
            distToHorizontalGridBeingHit=__FLT_MAX__;//Float.MAX_VALUE;
        
        // else, move the ray until it hits a horizontal wall
        else {
            distToNextXIntersection = fXStepTable[castArc];

            while (true) {

                xGridIndex = (int)(xIntersection / TILE_SIZE);
                // in the picture, yGridIndex will be 1
                yGridIndex = (horizontalGrid / TILE_SIZE);

                if ((xGridIndex >= MAP_WIDTH) ||
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
        if (castArc < ANGLE90 || castArc > ANGLE270) {

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
        if (castArc == ANGLE90 || castArc == ANGLE270) 
            distToVerticalGridBeingHit = __FLT_MAX__;//Float.MAX_VALUE;
        
        else {
            
            distToNextYIntersection = fYStepTable[castArc];
        
            while (true) {
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
            // determine which ray strikes a closer wall.
            // if yray distance to the wall is closer, the yDistance will be shorter than
                // the xDistance
        if (distToHorizontalGridBeingHit < distToVerticalGridBeingHit) {
            
            // the next function call (drawRayOnMap()) is not a part of raycating rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToHorizontalGridBeingHit;
        }
        // else, we use xray instead (meaning the vertical wall is closer than
        //   the horizontal wall)
        else {
            
            // the next function call (drawRayOnMap()) is not a part of raycating rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToVerticalGridBeingHit;
        }

        // correct distance (compensate for the fishbown effect)
        dist /= fFishTable[castColumn];
        // projected_wall_height/wall_height = fPlayerDistToProjectionPlane/dist;
        int projectedWallHeight = (int)(WALL_HEIGHT * (float)fPlayerDistanceToTheProjectionPlane / dist);
        bottomOfWall = fProjectionPlaneYCenter + (int)(projectedWallHeight * 0.5F);
        topOfWall = PROJECTIONPLANEHEIGHT - bottomOfWall;

        if(bottomOfWall >= PROJECTIONPLANEHEIGHT)
            bottomOfWall = PROJECTIONPLANEHEIGHT - 1;

        //fOffscreenGraphics.drawLine(castColumn, topOfWall, castColumn, bottomOfWall);
       // fOffscreenGraphics.fillRect(castColumn, topOfWall, 5, projectedWallHeight);
        
        fb_draw_column(castColumn, topOfWall, COLUMN_RESOLUTION, projectedWallHeight);

        // TRACE THE NEXT RAY
        castArc += COLUMN_RESOLUTION;
        if (castArc >= ANGLE360)
            castArc -= ANGLE360;
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


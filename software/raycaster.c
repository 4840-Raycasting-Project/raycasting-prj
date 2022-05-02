/*
Adapted from https://permadi.com/activity/ray-casting-game-engine-demo/

TODO:

1. Textures for wall - global constants
2. blackout screen function
3. fbputchar -> tile buffer hardware char render
4. port in game logic and menu functionality start and complete a level
5. jumping demo

8. sprites?

*/

#include "column_decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "usbdevices.h"
#include <pthread.h>
#include <stdbool.h> 
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

// size of tile (wall height) - best to make some power of 2
#define TILE_SIZE 64
#define WALL_HEIGHT 64

//do not change - hardware precisely calibrated for these values - will crash system otherwise
#define PROJECTIONPLANEWIDTH 640 
#define PROJECTIONPLANEHEIGHT 480

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

//best to make this some power of 2 (with column decoder MUST be 1)
#define COLUMN_WIDTH 1

#define CONTROLLER_DPAD_DEFAULT 0x7F
#define CONTROLLER_BTN_DEFAULT 0xF

#define CHAR_NUM_ROWS 30
#define CHAR_NUM_COLS 80
 
columns_t columns;

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
int fPlayerDistanceToTheProjectionPlane = 677;
int fPlayerSpeed = 6;
int fProjectionPlaneYCenter = PROJECTIONPLANEHEIGHT / 2;

// movement flag
bool fKeyUp = false;
bool fKeyDown = false;
bool fKeyLeft = false;
bool fKeyRight = false;

// 2 dimensional map
static const uint8_t B = 1; // bluestone
static const uint8_t C = 2; // colorstone
static const uint8_t E = 3; // eagle
static const uint8_t G = 4; // greystone
static const uint8_t M = 5; // mossy
static const uint8_t P = 6; // purplestone
static const uint8_t R = 7; // redbrick
static const uint8_t W = 8; // wood


static const uint8_t O = 0; // opening
static const uint8_t MAP_WIDTH = 12;
static const uint8_t MAP_HEIGHT = 12;

uint8_t *fMap;

struct libusb_device_handle *keyboard;
struct libusb_device_handle *controller;
uint8_t endpoint_address_kb;
uint8_t endpoint_address_ctr;

//pthread_mutex_t kp_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t kp_cond = PTHREAD_COND_INITIALIZER;

//function signatures
void render();
void create_tables();
float arc_to_rad(float);
void handle_key_press(struct usb_keyboard_packet *, bool);
void put_char(char, uint8_t, uint8_t);
void clear_chars();
void set_blackout(bool);

bool up_pressed, down_pressed, left_pressed, right_pressed;
bool up_ctr_pressed, down_ctr_pressed, left_ctr_pressed, right_ctr_pressed;

bool jump_pressed, jump_pressed_ctr, is_jumping;
bool sched_jump_start;
int  jump_frame;

pthread_t keyboard_thread;
void *keyboard_thread_f(void *);

pthread_t controller_thread;
void *controller_thread_f(void *);

int column_decoder_fd;

int main() {
    
    //TODO way to clear screen

    static const char filename[] = "/dev/column_decoder";

	printf("Raycaster Userspace program started\n");

	if ( (column_decoder_fd = open(filename, O_RDWR)) == -1) {
		
		fprintf(stderr, "could not open %s\n", filename);
		return -1;
	}     

    create_tables();

    /* Open the keyboard */
    if ((keyboard = openkeyboard(&endpoint_address_kb)) == NULL) 
        fprintf(stderr, "Did not find a keyboard\n");
    
	/* Open the controller */
    if ((controller = opencontroller(&endpoint_address_ctr)) == NULL)
        fprintf(stderr, "Did not find a controller\n");
    
	if(controller == NULL && keyboard == NULL) 
		exit(1);
	
	//start keyboard and controller threads
	pthread_create(&keyboard_thread, NULL, keyboard_thread_f, NULL);
	pthread_create(&controller_thread, NULL, controller_thread_f, NULL);
	
	render();
	
	int map_size = MAP_HEIGHT * MAP_WIDTH;
	
	//reset column number in hardware for good measure
	if (ioctl(column_decoder_fd, COLUMN_DECODER_RESET_COL_NUM, 0x00)) {
		perror("ioctl(COLUMN_DECODER_RESET_COL_NUM) failed");
		exit(1);
	}
	
	set_blackout(false);
	clear_chars();
	
	int i = 0;
	
	put_char('a', 0, 0);

    while(true) {
		
		/*
		pthread_mutex_lock(&kp_mutex);
		while(!up_pressed && !down_pressed && !left_pressed && !right_pressed 
			&& !up_ctr_pressed && !down_ctr_pressed && !left_ctr_pressed && !right_ctr_pressed) {		
			pthread_cond_wait(&kp_cond,&kp_mutex);
		}
		*/
		
		//jump
		if(sched_jump_start) {
			
			is_jumping = true;
			jump_frame = 0;
			sched_jump_start = false;
		}
		
		if(is_jumping) {
			
			jump_frame++;
			
			fProjectionPlaneYCenter = (int) (PROJECTIONPLANEHEIGHT / 2) + (
			
				(.1 * (jump_frame - 32) * (jump_frame - 32)) - 100
			);
			
			if(fProjectionPlaneYCenter >= (PROJECTIONPLANEHEIGHT / 2)) {
				
				fProjectionPlaneYCenter = PROJECTIONPLANEHEIGHT / 2;
				is_jumping = false;
			}
		}
		
		// rotate left
		if(left_pressed || left_ctr_pressed) {
			if((fPlayerArc -= ANGLE5) < ANGLE0)
				fPlayerArc += ANGLE360;
		}
		
		// rotate right
		else if(right_pressed || right_ctr_pressed) {
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
		if(up_pressed || up_ctr_pressed) {
			
			tmpPlayerX = fPlayerX + (int)(playerXDir * fPlayerSpeed);
			tmpPlayerY = fPlayerY + (int)(playerYDir * fPlayerSpeed);
			
			int map_index = (tmpPlayerX / TILE_SIZE) + ((tmpPlayerY / TILE_SIZE) * MAP_HEIGHT);

			if(map_index < map_size && !fMap[map_index]) {
				
				fPlayerX = tmpPlayerX;
				fPlayerY = tmpPlayerY;
			}
		}
		
		// move backward
		else if(down_pressed || down_ctr_pressed) {
			
			tmpPlayerX = fPlayerX - (int)(playerXDir * fPlayerSpeed);
			tmpPlayerY = fPlayerY - (int)(playerYDir * fPlayerSpeed);
			
			int map_index = (tmpPlayerX / TILE_SIZE) + ((tmpPlayerY / TILE_SIZE) * MAP_HEIGHT);
			
			if(map_index < map_size && !fMap[map_index]) {
				
				fPlayerX = tmpPlayerX;
				fPlayerY = tmpPlayerY;
			}
		}
		
		//pthread_mutex_unlock(&kp_mutex);		
		
		//test black out every half second
		/*
		if((i % 60) > 30)
			set_blackout(true);
		else
			set_blackout(false);
		*/
		
		render();
		
		i++;
    }
	
	free(fMap);
	
	pthread_cancel(keyboard_thread);
    pthread_join(keyboard_thread, NULL);
	
	pthread_cancel(controller_thread);
    pthread_join(controller_thread, NULL);

    return 0;
}

void *keyboard_thread_f(void *ignored) {
	
	int transferred;
	
	bool prev_jump_state;
	
	struct usb_keyboard_packet packet;
	
	while(true) {
		
		libusb_interrupt_transfer(keyboard, endpoint_address_kb,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);

        if (transferred == sizeof(packet)) {
			
			prev_jump_state = jump_pressed;
			
			//pthread_mutex_lock(&kp_mutex);
			
			up_pressed =    is_key_pressed(0x52, packet.keycode);
			down_pressed =  is_key_pressed(0x51, packet.keycode);
			left_pressed =  is_key_pressed(0x50, packet.keycode);
			right_pressed = is_key_pressed(0x4f, packet.keycode);
			jump_pressed =  is_key_pressed(0x2c, packet.keycode);
			
			if(jump_pressed && !is_jumping && !prev_jump_state)
				sched_jump_start = true;
			
			//pthread_cond_signal(&kp_cond);
			//pthread_mutex_unlock(&kp_mutex);
		}
	}
  
	return NULL;
}

void *controller_thread_f(void *ignored) {
	
	int transferred;
	
	bool prev_jump_state_ctr;
	
	struct usb_keyboard_packet packet;
;	
	while(true) {
		
		libusb_interrupt_transfer(controller, endpoint_address_ctr,
			      (unsigned char *) &packet, sizeof(packet),
			      &transferred, 0);

        if (transferred == sizeof(packet)) {
			
			prev_jump_state_ctr = jump_pressed_ctr;

			//pthread_mutex_lock(&kp_mutex);
			
			if (packet.keycode[1] != CONTROLLER_DPAD_DEFAULT || packet.keycode[2] != CONTROLLER_DPAD_DEFAULT || packet.keycode[2] != CONTROLLER_BTN_DEFAULT) { 

				up_ctr_pressed 		= is_controller_key_pressed(2, 0x00, packet.keycode);
				down_ctr_pressed 	= is_controller_key_pressed(2, 0xff, packet.keycode);
				left_ctr_pressed 	= is_controller_key_pressed(1, 0x00, packet.keycode);
				right_ctr_pressed 	= is_controller_key_pressed(1, 0xff, packet.keycode);
				jump_pressed_ctr	= is_controller_key_pressed(3, 0x2f, packet.keycode);
				
				if(jump_pressed_ctr && !is_jumping && !prev_jump_state_ctr)
					sched_jump_start = true;
			}
			else {
				up_ctr_pressed 	    = false;
				down_ctr_pressed 	= false;
				left_ctr_pressed 	= false;
				right_ctr_pressed 	= false;
				jump_pressed_ctr	= false;
			}
			
			//pthread_cond_signal(&kp_cond);
			//pthread_mutex_unlock(&kp_mutex);
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
	
	uint8_t textureH, textureV, texture;
	uint8_t vblank = 0;

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
                else if (fMap[yGridIndex * MAP_WIDTH + xGridIndex]) {
					
					textureH = fMap[yGridIndex * MAP_WIDTH + xGridIndex] - 1;
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
                else if (fMap[yGridIndex * MAP_WIDTH + xGridIndex]) {
					
					textureV = fMap[yGridIndex * MAP_WIDTH + xGridIndex] - 1;
                    distToVerticalGridBeingHit = (yIntersection - fPlayerY) * fISinTable[castArc];
                    break;
                }
                else  {
                    yIntersection += distToNextYIntersection;
                    verticalGrid += distToNextVerticalGrid;
                }
            }
        }

        // DRAW THE WALL SLICE
        float dist;
        uint16_t topOfWall;   // used to compute the top and bottom of the sliver that
        uint16_t bottomOfWall;   // will be the staring point of floor and ceiling
		uint8_t wall_side; //0=x, 1=y
		uint8_t offset;
		
            // determine which ray strikes a closer wall.
            // if yray distance to the wall is closer, the yDistance will be shorter than
                // the xDistance
        if (distToHorizontalGridBeingHit < distToVerticalGridBeingHit) {
            
            // the next function call (drawRayOnMap()) is not a part of raycating rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToHorizontalGridBeingHit;
			texture = textureH;
			wall_side = 0;
			offset = (int)xIntersection % TILE_SIZE;
        }
        // else, we use xray instead (meaning the vertical wall is closer than
        //   the horizontal wall)
        else {
            
            // the next function call (drawRayOnMap()) is not a part of raycasting rendering part, 
            // it just draws the ray on the overhead map to illustrate the raycasting process
            dist = distToVerticalGridBeingHit;
			texture = textureV;
			wall_side = 1;
			offset = (int)yIntersection % TILE_SIZE;
        }

        // correct distance (compensate for the fishbown effect)
        dist /= fFishTable[castColumn];
		
		//0 distance makes no sense for below calcs
		if(dist == 0.0)
			dist = .00001;
				
        // projected_wall_height/wall_height = fPlayerDistToProjectionPlane/dist;
        int projectedWallHeight = (int)(WALL_HEIGHT * (float)fPlayerDistanceToTheProjectionPlane / dist);
		projectedWallHeight = projectedWallHeight > 32767 ? 32767 : projectedWallHeight;
		
		
        bottomOfWall = fProjectionPlaneYCenter + (int)(projectedWallHeight * 0.5F);
		bottomOfWall = bottomOfWall > 32767 ? 32767 : bottomOfWall;
		
        topOfWall = PROJECTIONPLANEHEIGHT - bottomOfWall;

        columns.column_args[castColumn].top_of_wall = topOfWall;
        columns.column_args[castColumn].wall_side = wall_side;
        columns.column_args[castColumn].texture_type = texture;
        columns.column_args[castColumn].wall_height = (short)projectedWallHeight;
        columns.column_args[castColumn].texture_offset = offset;

        // TRACE THE NEXT RAY
        castArc += COLUMN_WIDTH;
        if (castArc >= ANGLE360)
            castArc -= ANGLE360;
    }
	//usleep(16667);
	//wait for vblank to send columns
	while(true) {
		
		ioctl(column_decoder_fd, COLUMN_DECODER_READ_VBLANK, &vblank);
		
		if(vblank)
			break;
		
		usleep(50);
	}
	
    //send the columns to the driver
    if (ioctl(column_decoder_fd, COLUMN_DECODER_WRITE_COLUMNS, &columns)) {
		perror("ioctl(COLUMN_DECODER_WRITE_COLUMNS) failed");
		return;
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
	
	//types: B: bluestone, C: colorstone, E: eagle, G: greystone, M: mossy, P: purplestone, R: redbrick, W: wood
	
	uint8_t fMapCopy[] =
		{
			W,W,W,W,W,B,W,E,W,P,W,W,
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

/*supplemental functions dealing with screen color override and text*/

void put_char(char character, uint8_t row, uint8_t col) {
	
	char_tile_t char_tile = { character, row, col };
	
	if (ioctl(column_decoder_fd, COLUMN_DECODER_WRITE_CHAR, &char_tile)) {
		perror("ioctl(COLUMN_DECODER_WRITE_CHAR) failed");
		exit(1);
	}
}
	

void clear_chars() {
	
	for(int row=0; row<CHAR_NUM_ROWS; row++) {
		for(int col=0; row<CHAR_NUM_COLS; col++)
			put_char(' ', row, col);
	}
}


void set_blackout(bool blackout) {
	
	if(blackout) {
		if (ioctl(column_decoder_fd, COLUMN_DECODER_BLACKOUT_SCREEN, 0x00)) {
			perror("ioctl(COLUMN_DECODER_BLACKOUT_SCREEN) failed");
			exit(1);
		}			
	}
	else {
		if (ioctl(column_decoder_fd, COLUMN_DECODER_REMOVE_BLACKOUT_SCREEN, 0x00)) {
			perror("ioctl(COLUMN_DECODER_REMOVE_BLACKOUT_SCREEN) failed");
			exit(1);
		}	
	}
	
}


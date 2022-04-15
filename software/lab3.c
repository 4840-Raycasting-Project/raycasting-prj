/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include <stdlib.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_X 640
#define MAX_Y 480
#define MAX_SPEED 4

#define COLORS 9

int vga_ball_fd;

static const vga_ball_color_t colors[] = {
	{ 0xff, 0x00, 0x00 }, /* Red */
	{ 0x00, 0xff, 0x00 }, /* Green */
	{ 0x00, 0x00, 0xff }, /* Blue */
	{ 0xff, 0xff, 0x00 }, /* Yellow */
	{ 0x00, 0xff, 0xff }, /* Cyan */
	{ 0xff, 0x00, 0xff }, /* Magenta */
	{ 0x80, 0x80, 0x80 }, /* Gray */
	{ 0x00, 0x00, 0x00 }, /* Black */
	{ 0xff, 0xff, 0xff }  /* White */
};

/* Read and print the background color */
void print_initial_state(vga_ball_arg_t *vla) {
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_ATTR, vla)) {
      perror("ioctl(VGA_BALL_READ_ATTR) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla->background.red, vla->background.green, vla->background.blue);

  printf("%d %d %d\n",
	 vla->pos.pos_x, vla->pos.pos_y, vla->radius);	 
}

/* Set the background color */
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}

void set_ball_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.ball_color = *c;
  
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BALL_COLOR, &vla)) {
      perror("ioctl(VGA_BALL_SET_BALL_COLOR) failed");
      return;
  }
}

void set_position(const vga_ball_pos_t *pos)
{
  vga_ball_arg_t vla;
  vla.pos = *pos;

  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_POSITION, &vla)) {
      perror("ioctl(VGA_BALL_SET_POSITION) failed");
	   printf("failed position set ");
      return;
  }
}

void set_radius(const uint16_t radius)
{
  vga_ball_arg_t vla;
  vla.radius = radius;
  
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_RADIUS, &vla)) {
      perror("ioctl(VGA_BALL_SET_RADIUS) failed");
      return;
  }
}

void fade_radius(const uint16_t target_radius, const uint8_t speed) {
	
	//get initial radius
	vga_ball_arg_t vla;
	
	if (ioctl(vga_ball_fd, VGA_BALL_READ_ATTR, &vla)) {
		perror("ioctl(VGA_BALL_READ_ATTR) failed");
		return;
	}
	
	uint16_t cur_radius = vla.radius;

	if(cur_radius == target_radius)
		return;
	
	int8_t multiplier = cur_radius < target_radius ? 1 : -1;
	
	while(cur_radius != target_radius) {
			
		cur_radius = abs(target_radius - cur_radius) < speed 
			? target_radius 
			: cur_radius + (speed * multiplier);
			
		set_radius(cur_radius);

			
		usleep(20000);
	}
	
	return;
}

void fade_ball_color() {
	

}

void fade_bg_color() {
	
	
	
}

void bounce(char dir, ball_vector *bv) {
	
	short *vector = dir == 'x' ? &bv->speed_x : &bv->speed_y;
	
	*vector = *vector * -1;	
	
	//TODO fade colors
	
	//TODO ensure bg color different from last
	short bg_color_index;
	bg_color_index = rand() % COLORS;
	
	set_background_color(&colors[bg_color_index]);
	
	short ball_color_index;
	do {
		ball_color_index = rand() % COLORS;
	} while(ball_color_index == bg_color_index);
	
	set_ball_color(&colors[ball_color_index]);
}

void move(vga_ball_arg_t *vla, ball_vector *bv) {
	
	//establish boundaries
	int min_pos = 0 + vla->radius;
	int max_x_pos = 640 - vla->radius;
	int max_y_pos = 480 - vla->radius;	
	
	//draw ball to next pos, not further than edge
	vla->pos.pos_x += bv->speed_x;
	
	if(vla->pos.pos_x < min_pos)
		vla->pos.pos_x = min_pos;
	else if(vla->pos.pos_x > max_x_pos)
		vla->pos.pos_x = max_x_pos;
	
	if(vla->pos.pos_x == min_pos || vla->pos.pos_x == max_x_pos)
		bounce('x', bv);
	
	
	vla->pos.pos_y += bv->speed_y;
	
	if(vla->pos.pos_y < min_pos)
		vla->pos.pos_y = min_pos;
	else if(vla->pos.pos_y > max_y_pos)
		vla->pos.pos_y = max_y_pos;
	
	if(vla->pos.pos_y == min_pos || vla->pos.pos_y == max_y_pos)
		bounce('y', bv);
	
	vga_ball_pos_t position = {vla->pos.pos_x,vla->pos.pos_y};
	set_position(&position);
}


int main() {
	
	vga_ball_arg_t vla;
	int i;
	vga_ball_color_t bg_color = { 0x00, 0x00, 0x00 };
	vga_ball_color_t ball_color = { 0xff, 0xff, 0xff };
	static const char filename[] = "/dev/vga_ball";
  
	printf("VGA ball Userspace program started\n");

	if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
		
		fprintf(stderr, "could not open %s\n", filename);
		return -1;
	} 
	
	//reset position
	uint16_t pos_x = 320;
	uint16_t pos_y = 240;
	vga_ball_pos_t position = {pos_x,pos_y};
	set_position(&position);
	
	//reset background
	set_background_color(&bg_color);
	
	//reset ball color
	set_ball_color(&ball_color);
	
	printf("initial state: ");
	print_initial_state(&vla);	
	
	set_radius(32);
	//pulse radius for 3 seconds
	fade_radius(90, 2);
	fade_radius(32, 2);
	fade_radius(100, 2);
	fade_radius(32, 1);
	
	
	//TODO fade color on another thread

	//pick random x and y speed
	short x_speed = 0, y_speed = 0;
	
	while(!x_speed) {
		
		short multiplier = rand() % 2;
		if(!multiplier)
			multiplier = -1;
		
		x_speed = ((rand() % MAX_SPEED) + 1) * multiplier;	
	}
	
	while(!y_speed) {
		
		short multiplier = rand() % 2;
		if(!multiplier)
			multiplier = -1;
		
		y_speed = ((rand() % MAX_SPEED) + 1) * multiplier;	
	}	
	
	ball_vector bv = {
		speed_x: x_speed,
		speed_y: y_speed		
	};
	

	for(;;) {
		move(&vla, &bv);
		usleep(20000);
	}
  
	printf("VGA BALL Userspace program terminating\n");
	return 0;
}

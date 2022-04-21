#ifndef _COLUMN_DECODER_H
#define _COLUMN_DECODER_H

#include <linux/ioctl.h>
#include <asm/types.h>

typedef struct {

  short top_of_wall; //signed
  __u8 wall_side;
  __u8 texture_type;

  __u16 wall_height;
  __u8 texture_offset;

} column_arg_t;

typedef struct {
	column_arg_t column_args[640];
} columns_t;

#define COLUMN_DECODER_MAGIC 'q'

/* ioctls and their arguments */
#define COLUMN_DECODER_WRITE_COLUMNS _IOW(COLUMN_DECODER_MAGIC, 1, columns_t *)

/* #define VGA_BALL_READ_ATTR  _IOR(VGA_BALL_MAGIC, 5, vga_ball_arg_t *) */

#endif

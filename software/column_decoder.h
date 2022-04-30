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

typedef struct {
	
  __u8 row;
  __u8 col;
  __u8 char_val;
  
} char_tile_t;


#define COLUMN_DECODER_MAGIC 'q'

/* ioctls and their arguments */
#define COLUMN_DECODER_RESET_COL_NUM _IOW(COLUMN_DECODER_MAGIC, 1, __u8)
#define COLUMN_DECODER_WRITE_COLUMNS _IOW(COLUMN_DECODER_MAGIC, 2, columns_t *)
#define COLUMN_DECODER_WRITE_CHAR _IOW(COLUMN_DECODER_MAGIC, 3, char_tile_t *)
#define COLUMN_DECODER_BLACKOUT_SCREEN _IOW(COLUMN_DECODER_MAGIC, 4, __u8)
#define COLUMN_DECODER_REMOVE_BLACKOUT_SCREEN _IOW(COLUMN_DECODER_MAGIC, 5, __u8)

#define COLUMN_DECODER_READ_VBLANK _IOR(COLUMN_DECODER_MAGIC, 6, __u8)


#endif

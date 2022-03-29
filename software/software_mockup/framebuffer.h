#ifndef _FRAMEBUFFER_H
#  define _FRAMEBUFFER_H

#include <stdbool.h>
#include <stdint.h>


#define FBOPEN_DEV -1          /* Couldn't open the device */
#define FBOPEN_FSCREENINFO -2  /* Couldn't read the fixed info */
#define FBOPEN_VSCREENINFO -3  /* Couldn't read the variable info */
#define FBOPEN_MMAP -4         /* Couldn't mmap the framebuffer memory */
#define FBOPEN_BPP -5          /* Unexpected bits-per-pixel */

extern int fbopen(void);
extern void fb_clear_screen();
extern void fb_draw_column(int, int, int, int, uint8_t);
extern void fbputchar(char, int, int);
extern void fbputs(const char *, int, int);

int idiv_ceil (int, int);

#endif

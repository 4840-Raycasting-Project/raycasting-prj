/* * Device driver for the Raycasting Column Decoder
 *
 * A Platform device implemented using the misc subsystem
 *
 * Stephen A. Edwards
 * Columbia University
 * 
 * Modified by Adam Carpentieri AC4409
 * Columbia University Spring 2022
 *
 * References:
 * Linux source: Documentation/driver-model/platform.txt
 *               drivers/misc/arm-charlcd.c
 * http://www.linuxforu.com/tag/linux-device-drivers/
 * http://free-electrons.com/docs/
 *
 * "make" to build
 * insmod column_decoder.ko
 *
 * Check code style with
 * checkpatch.pl --file --no-tree column_decoder.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/types.h>
#include "column_decoder.h"

#define DRIVER_NAME "column_decoder"

/* Device registers */
#define WRITE_COLNUM_RESET(x) (x)
#define WRITE_COLS(x) ((x)+2)
#define WRITE_CHAR_S1(x) ((x)+4)
#define WRITE_CHAR_S2(x) ((x)+6)
#define WRITE_BLACKOUT(x) ((x)+8)
#define READ_VBLANK(x) (x)

/*
 * Information about our device
 */
struct column_decoder_dev {
	struct resource res; /* Resource: our register */
	void __iomem *virtbase; /* Where register can be accessed in memory */
	columns_t *columns;
} dev;


//ensure the next write will be for the first column
static void reset_col_num(void) {
	
	iowrite8(0x00, WRITE_COLNUM_RESET(dev.virtbase)); //value does not matter
}

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_columns(columns_t *columns)
{
	__u16 bits_to_send;
	__u16 i;
	__u32 scaling_factor;
	column_arg_t column_arg;	

	for(i=0; i<640; i++) {
		
		column_arg = columns->column_args[i];
		
		bits_to_send = 0x0000 | (column_arg.wall_side << 9);
		bits_to_send |= (column_arg.texture_type << 6);
		bits_to_send |= column_arg.texture_offset;
		iowrite16(bits_to_send, WRITE_COLS(dev.virtbase));
		
		iowrite16(column_arg.wall_height, WRITE_COLS(dev.virtbase));
		
		iowrite16(column_arg.top_of_wall, WRITE_COLS(dev.virtbase));
		
		//TODO figure out why it needs to be multipled by -1: otherwise texture in hardware is upside down
		scaling_factor = column_arg.wall_height ? (0x00000000 | ((64 << 25) / column_arg.wall_height)) * -1 : 0x00000000; 
		
		bits_to_send = (__u16) ((scaling_factor & 0xFFFF0000) >> 16);
		iowrite16(bits_to_send, WRITE_COLS(dev.virtbase));
		
		bits_to_send = (__u16) (scaling_factor & 0x0000FFFF);
		iowrite16(bits_to_send, WRITE_COLS(dev.virtbase));
	}
	
	//TODO copy column data manually
	//dev.columns = columns;
}


static void write_char(char_tile_t *char_tile) {
	
	__u16 s2_val;
  
	iowrite8(char_tile->char_val, WRITE_CHAR_S1(dev.virtbase));
	
	s2_val = char_tile->col << 5;
	s2_val |= char_tile->row;
	
	iowrite16(s2_val, WRITE_CHAR_S2(dev.virtbase));
}

static void blackout_screen(void) {	
	iowrite16(0x0001, WRITE_BLACKOUT(dev.virtbase));
}

static void remove_blackout_screen(void) {	
	iowrite16(0x0000, WRITE_BLACKOUT(dev.virtbase));
}

static __u8 read_vblank(void) {
	
	return (__u8) ioread16(READ_VBLANK(dev.virtbase));
}


/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long column_decoder_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	columns_t   columns;
	char_tile_t char_tile;
	__u8		vblank_val;

	switch (cmd) {
		
	case COLUMN_DECODER_RESET_COL_NUM:
		reset_col_num();
		break;		
		
	case COLUMN_DECODER_WRITE_COLUMNS:
	
		if (copy_from_user(&columns, (columns_t *) arg, sizeof(columns_t)))
			return -EACCES;
		
		write_columns(&columns);
		
		break;

	case COLUMN_DECODER_WRITE_CHAR:
	
		if (copy_from_user(&char_tile, (char_tile_t *) arg, sizeof(char_tile_t)))
			return -EACCES;
		
		write_char(&char_tile);
		
		break;			
		
	case COLUMN_DECODER_BLACKOUT_SCREEN:
		
		blackout_screen();
		
		break;
		
	case COLUMN_DECODER_REMOVE_BLACKOUT_SCREEN:
		remove_blackout_screen();
		
		break;
		
	case COLUMN_DECODER_READ_VBLANK:
	
		vblank_val = read_vblank();
		
		if (copy_to_user((__u8 *) arg, &vblank_val, sizeof(__u8)))
			return -EACCES;
		
		break;		

	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations column_decoder_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = column_decoder_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice column_decoder_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &column_decoder_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init column_decoder_probe(struct platform_device *pdev)
{

	int ret;

	/* Register ourselves as a misc device: creates /dev/column_decoder */
	ret = misc_register(&column_decoder_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&column_decoder_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int column_decoder_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&column_decoder_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id column_decoder_of_match[] = {
	{ .compatible = "csee4840,column_decoder-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, column_decoder_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver column_decoder_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(column_decoder_of_match),
	},
	.remove	= __exit_p(column_decoder_remove),
};

/* Called when the module is loaded: set things up */
static int __init column_decoder_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&column_decoder_driver, column_decoder_probe);
}

/* Called when the module is unloaded: release resources */
static void __exit column_decoder_exit(void)
{
	platform_driver_unregister(&column_decoder_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(column_decoder_init);
module_exit(column_decoder_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adam Carpentieri, Columbia University");
MODULE_DESCRIPTION("Raycasting Column Decoder driver");

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
#include <asm/types.h>
#include "column_decoder.h"


#define DRIVER_NAME "column_decoder"

/*
 * Information about our device
 */
struct column_decoder_dev {
	struct resource res; /* Resource: our register */
	void __iomem *virtbase; /* Where register can be accessed in memory */
	
	struct columns_t columns;
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_columns(columns_t *columns)
{
	__u16 bits_to_send = 0x0000;
	__u8 i;
	column_arg_t column_arg;

	for(i=0; i <640; i++) {
		
		column_arg = *(columns->column_args[i]);

		bits_to_send = 0x0000 & (column_arg.top_of_wall << 4);
		bits_to_send |= (column_arg.wall_side << 3);
		bits_to_send |= column_arg.texture_type;
		iowrite16(bits_to_send, dev.virtbase) );

		bits_to_send = 0x0000 & (column_arg.wall_height << 6);
		bits_to_send |= column_arg.texture_offset;
		iowrite16(bits_to_send, dev.virtbase) );
	}

	dev.columns = *columns;
}


/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long column_decoder_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	columns_t vla;

	switch (cmd) {
	case COLUMN_DECODER_WRITE_COLUMNS:
		if (copy_from_user(&vla, (columns_t *) arg,
				   sizeof(columns_t)))
			return -EACCES;
		write_columns(&vla);
		break;
/*
	case COLUMN_DECODER_READ_ATTR:
	  	vla.radius = dev.radius;
		if (copy_to_user((column_decoder_arg_t *) arg, &vla,
				 sizeof(column_decoder_arg_t)))
			return -EACCES;
		break;		
*/
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
        
	/* TODO Provide a bunch of blank columns */
	
	//write_columns(columns_t *columns);

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

#include "usbkeyboard.h"

#include <stdio.h>
#include <stdlib.h> 

/* References on libusb 1.0 and the USB HID/keyboard protocol
 *
 * http://libusb.org
 * http://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
 * http://www.usb.org/developers/devclass_docs/HID1_11.pdf
 * http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
 */

/*
 * Find and return a USB keyboard device or NULL if not found
 * The argument con
 * 
 */
struct libusb_device_handle *openkeyboard(uint8_t *endpoint_address) {
  libusb_device **devs;
  //struct libusb_device_handle *keyboard = NULL;
  struct libusb_device_handle *controller = NULL;
  struct libusb_device_descriptor desc;
  ssize_t num_devs, d;
  uint8_t i, k;
  
  /* Start the library */
  if ( libusb_init(NULL) < 0 ) {
    fprintf(stderr, "Error: libusb_init failed\n");
    exit(1);
  }

  /* Enumerate all the attached USB devices */
  if ( (num_devs = libusb_get_device_list(NULL, &devs)) < 0 ) {
    fprintf(stderr, "Error: libusb_get_device_list failed\n");
    exit(1);
  }

  /* Look at each device, remembering the first HID device that speaks
     the keyboard protocol */

  for (d = 0 ; d < num_devs ; d++) {
    libusb_device *dev = devs[d];
    if ( libusb_get_device_descriptor(dev, &desc) < 0 ) {
      fprintf(stderr, "Error: libusb_get_device_descriptor failed\n");
      exit(1);
    }

    /*
     * libusb_class_code enum
     * LIBUSB_CLASS_PER_INTERFACE = 0x00
     * others.. CLASS_AUDIO, PRINTER, SECURITY, VIDEO...
     * keyboard and controller: bDeviceClass            0 (Defined at Interface level)
    */

    /*temporary WA using the idVendor of device descriptor since keyboard has an overlapping interface with controller*/
    if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE && desc.idVendor == 0x0079) {

      struct libusb_config_descriptor *config;
      libusb_get_config_descriptor(dev, 0, &config);

      for (i = 0 ; i < config->bNumInterfaces ; i++)	       

	for ( k = 0 ; k < config->interface[i].num_altsetting ; k++ ) {

	  /* libusb_interface pointer array is part of the config_descriptor struct
 	   * there are bNumInterfaces # of these libusb_interface pointers
 	   * altsetting is a pointer to libusb_interface_descriptor. there is an array of them in libusb_interface struct
 	   * */
	  const struct libusb_interface_descriptor *inter =
	    config->interface[i].altsetting + k ;

	  /*
 	   * keyboard: 	
 	   * bInterfaceClass         3 Human Interface Device
 	   * bInterfaceSubClass      1 Boot Interface Subclass
 	   * bInterfaceProtocol      1 Keyboard
 	   *
 	   *
 	   * controller:
 	   * bInterfaceClass         3 Human Interface Device
 	   * bInterfaceSubClass      0 No Subclass
 	   * bInterfaceProtocol      0 None
 	   *
 	   * LIBUSB_CLAS_HID = 3
 	   * USB_HID_KEYBOARD_PROTOCOL = 1
 	   * USB_HID_CONTROLLER_PROTOCOL = 0
 	   * */

	  if ( inter->bInterfaceClass == LIBUSB_CLASS_HID &&
	       inter->bInterfaceSubClass ==  0 && 	
	       inter->bInterfaceProtocol == USB_HID_CONTROLLER_PROTOCOL) {

	    	    int r;

		    if ((r = libusb_open(dev, &controller)) != 0) {
		      fprintf(stderr, "Error: libusb_open failed: %d\n", r);
		      exit(1);
		    }

		    if (libusb_kernel_driver_active(controller,i))
		      libusb_detach_kernel_driver(controller, i);

		    libusb_set_auto_detach_kernel_driver(controller, i);

		    if ((r = libusb_claim_interface(controller, i)) != 0) {
		      fprintf(stderr, "Error: libusb_claim_interface failed: %d\n", r);
		      exit(1);
		    }

		    *endpoint_address = inter->endpoint[0].bEndpointAddress;
		    goto found;
	  }
	}
    }
  }

 //this is required when you are done with the devices. but make sure not to unreference the device you are about to open before actually opening it
 found:
  libusb_free_device_list(devs, 1);

  //return keyboard;
  return controller;
}

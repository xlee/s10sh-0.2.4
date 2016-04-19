#ifndef __LINUX_H__
#define __LINUX_H__

#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

struct usb_ctrltransfer {
	/* keep in sync with usbdevice_fs.h:usbdevfs_ctrltransfer */
	u_int8_t  bRequestType;
	u_int8_t  bRequest;
	u_int16_t wValue;
	u_int16_t wIndex;
	u_int16_t wLength;

	u_int32_t timeout;	/* in milliseconds */

	/* pointer to data */
	void *data;
};

struct usb_bulktransfer {
	/* keep in sync with usbdevice_fs.h:usbdevfs_bulktransfer */
	unsigned int ep;
	unsigned int len;
	unsigned int timeout;	/* in milliseconds */

	/* pointer to data */
	void *data;
};

struct usb_setinterface {
	/* keep in sync with usbdevice_fs.h:usbdevfs_setinterface */
	unsigned int interface;
	unsigned int altsetting;
};

#define IOCTL_USB_CONTROL	_IOWR('U', 0, struct usb_ctrltransfer)
#define IOCTL_USB_BULK		_IOWR('U', 2, struct usb_bulktransfer)
#define IOCTL_USB_RESETEP	_IOR('U', 3, unsigned int)
#define IOCTL_USB_SETINTF	_IOR('U', 4, struct usb_setinterface)
#define IOCTL_USB_SETCONFIG	_IOR('U', 5, unsigned int)
#define IOCTL_USB_CLAIMINTF	_IOR('U', 15, unsigned int)
#define IOCTL_USB_RELEASEINTF	_IOR('U', 16, unsigned int)
#define IOCTL_USB_RESET		_IO('U', 20)
#define IOCTL_USB_CLEAR_HALT	_IOR('U', 21, unsigned int)

#endif


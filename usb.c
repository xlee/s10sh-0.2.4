/* This file is part of s10sh
 *
 * Copyright (C) 2000 by Salvatore Sanfilippo <antirez@invece.org>
 * Copyright (C) 2001 by Salvatore Sanfilippo <antirez@invece.org>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * USB driver implementation
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#ifdef HAVE_USB_SUPPORT

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#ifdef __linux__
#include <asm/page.h>
#endif /* __linux__ */
#ifndef PAGE_SIZE
/* This should be ok in most archs: what matter is that the real
 * page size is equal or minor than 0x1000, not that it matches */
#define PAGE_SIZE 0x1000
#endif
#include "s10sh.h"
#include "param.h"
#include "custom.h"

/**************************
 *        USB API         *
 **************************/

/* WARNING: functions with "USB" prefix are s10sh USB api functions,
 *          functions with "usb" perfix are libusb functions
 */

/* USB settings */
static usb_dev_handle *cameraudh;
static int usb_timeout = 1000;
static int input_ep = 0x81;
static int output_ep = 0x02;
static int configuration = 1;
static int interface = 0;
static int alternate = 0;

typedef enum {
  NOCAMERA        =   0,       /* No camera found */
  CAMERA_FOUND    =   1,       /* Camera found */
  USB_INIT_DANGER = 255,       /* DANGER Product Guess - '-Z' command line only */
  USB_INIT_FAILED =  -1,       /* Unable to initialize USB */
} USB_INIT_RESULT;

#define USB_HEADER_SIZE  0x50
#define USB_COMMAND_SIZE 0x4c
#define USB_BUFFER_SIZE  (USB_HEADER_SIZE+USB_COMMAND_SIZE)

static void
print_buffer (unsigned char *p, int size)
{
  static unsigned char prev[USB_BUFFER_SIZE];
  int i;
  for (i = 0; i < size; i++) {
    if (i % 0x10 == 0)  printf ("\n0x%.4x ", i);
    printf (" %s0x%.2x", prev[i]==p[i]?" ":"*", p[i]);
    prev[i] = p[i];
  }
  printf ("\n");
}

typedef enum camera_class {
  canon_class5,
  canon_class6,
} camera_class;

static camera_class
get_camera_class (camera_type model)
{
  switch (model) {
  case IXUS_65:
  case EOS350D:
    return canon_class6;
  default:
    break;
  }
  return canon_class5;
}

static int
sync_size (camera_type model)
{
  switch (get_camera_class(model)) {
  case canon_class6:
    return 0x50;
  default:
    break;
  }
  return 0x10;
}


static USB_INIT_RESULT 
USB_camera_init(struct usb_device **camera_dev)
{
	struct usb_bus *bus;
	struct usb_device *dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	if (!usb_busses) {
		if (opt_debug)
			fprintf(stderr, "USB initialization failed\n");
		return USB_INIT_FAILED;
	}

	for (bus = usb_busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (opt_debug)
				printf("Found device %04X/%04X\n",
					dev->descriptor.idVendor,
					dev->descriptor.idProduct);
			switch(dev->descriptor.idVendor) {
			case VENDOR_ID_CANON:
				switch(dev->descriptor.idProduct) {
				case PRODUCT_ID_S10:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon S10 found\n");
					camera_model = S10;
					return CAMERA_FOUND;
				case PRODUCT_ID_S20:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon S20 found\n");
					camera_model = S20;
					return CAMERA_FOUND;
				case PRODUCT_ID_A20:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon A20 found\n");
					camera_model = A20;
					return CAMERA_FOUND;
                                        /* "Artem 'Zazoobr' Ignatjev" <timon@memphis.mephi.ru> */
				case PRODUCT_ID_A60:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon A60 found\n");
					camera_model = A60;
					return CAMERA_FOUND;
                                        /* Stephan Weitz <stephan@weitz-net.de> */
				case PRODUCT_ID_S30:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon S30 found\n");
					camera_model = S30;
					return CAMERA_FOUND;
				case PRODUCT_ID_S100_EU:
				case PRODUCT_ID_S100_US:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon S100 found\n");
					camera_model = S100;
					return CAMERA_FOUND;
                                        /* Sean_Welch@alum.wofford.org */
				case PRODUCT_ID_S400:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon S400 found\n");
					camera_model = S400;
					return CAMERA_FOUND;
                                        /* David Jones <drj@pobox.com> */
				case PRODUCT_ID_DIGITAL_IXUS_V3:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon Digital IXUS V3 found\n");
					camera_model = IXUS_V3;
					return CAMERA_FOUND;
				case PRODUCT_ID_G1:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon G1 found\n");
					camera_model = G1;
					return CAMERA_FOUND;
				case PRODUCT_ID_G3:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon G3 found\n");
					camera_model = G3;
					return CAMERA_FOUND;
                                        /* Matthew Dillon <dillon@apollo.backplane.com> */
				case PRODUCT_ID_10D:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon EOS-10D found\n");
                                        /* This camera believes the flash in drive "C:" instead of drive "D:" ... whatever */
					setdcimpath("C:\\DCIM");
					camera_model = EOS10D;
					return CAMERA_FOUND;
                                case PRODUCT_ID_EOS20D:
                                        *camera_dev = dev;
                                        if (opt_debug)
                                                printf("Canon EOS-20D found\n");
                                        camera_model = EOS20D;
					return CAMERA_FOUND;
				case PRODUCT_ID_DIG_V2:
					*camera_dev = dev;
					if (opt_debug)
						printf("Canon Digital V2\n");
					camera_model = DIG_V2;
					return CAMERA_FOUND;
                                case PRODUCT_ID_A75:
                                        *camera_dev = dev;
                                        /*user_init = 1;*/
                                        if (opt_debug)
                                                printf("Canon PowerShot A75\n");
                                        camera_model = A75;
					return CAMERA_FOUND;
                                case PRODUCT_ID_IXUS_65:
                                        *camera_dev = dev;
                                        /*user_init = 1;*/
                                        if (opt_debug)
                                                printf("Canon PowerShot IXUS_65\n");
                                        camera_model = IXUS_65;
					return CAMERA_FOUND;
                                case PRODUCT_ID_DRebel:
					*camera_dev = dev;
					/*user_init = 1;*/
					if (opt_debug)
						printf("Canon Digital Rebel\n");
					camera_model = DRebel;
					return CAMERA_FOUND;
                                case PRODUCT_ID_EOS350D:
					*camera_dev = dev;
					/*user_init = 1;*/
					if (opt_debug)
						printf("Canon 350D\n");
					camera_model = EOS350D;
					return CAMERA_FOUND;
				case PRODUCT_ID_NEXTDIGICAM1:
				case PRODUCT_ID_NEXTDIGICAM2:
				case PRODUCT_ID_NEXTDIGICAM3:
				case PRODUCT_ID_NEXTDIGICAM4:
				case PRODUCT_ID_NEXTDIGICAM5:
				case PRODUCT_ID_NEXTDIGICAM6:
					*camera_dev = dev;
					printf("Unsupported Canon digicam "
					       "found, S10sh will try to use "
					       "The s10, s20, s100, G1 "
					       "protocol. Cross your "
					       "fingers!\n");
					camera_model = NEW_CAMERA;
					return CAMERA_FOUND;
				default:
					if ( DANGER == 1 ) {
                                        	*camera_dev = dev;
                                               	printf("DANGER: Canon Product Guess: "
							"%04X/%04X.\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
                                        	return USB_INIT_DANGER;
                                        	break;
					} else {
						if (opt_debug)
							printf("Unknown Canon product"
							" ID: %04X\n",
							dev->descriptor.idProduct);
						break;
					}
				}
				break;
			default:
				if (opt_debug)
					printf("Unknown vendor ID: %04X\n",
						dev->descriptor.idVendor);
			}
		}
	}
	return NOCAMERA;
}

/* The following two functions are based on gpio library */
static int 
USB_write_control_msg(int value, char *buffer, int size)
{
	int retval;

	retval = usb_control_msg(cameraudh,
				USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_DIR_OUT,
				size > 1 ? 0x04 : 0x0c,
				value,
				0,
				buffer,
				size,
				usb_timeout);

	if (opt_debug && 0) {
		printf("WRITE CONTROL MSG, type %X, value %X, size %d: %s\n",
			USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_DIR_OUT,
			value, size, retval == -1 ? "FAILED" : "OK");
		if (retval != -1)
			dump_hex("DATA", buffer, size);
	}
	return retval;
}

static int 
USB_read_control_msg(int value, char *buffer, int size)
{
	int retval;
	retval = usb_control_msg(cameraudh,
				USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_DIR_IN,
				size > 1 ? 0x04 : 0x0c,
				value,
				0,
				buffer,
				size,
				usb_timeout);
	if (opt_debug) {
		printf("READ CONTROL MSG, value %X, size %d: %s\n",
			value, size, retval == -1 ? "FAILED" : "OK");
		if (retval != -1)
			dump_hex("DATA", buffer, size);
	}
        /* 050102 Attempt to get A75 totally working - A=Active C=ColdStart*/
	/* Apparently a second x58 bytes must be read from a ColdStart EOS camera, 20D?, can't verify*/
        if (opt_debug) {
                if (buffer[0] == 'A')
                        printf("USB Read Control MSG - buffer[0]=A - Camera was already active\n\n");
                else if (buffer[0] == 'C') {
                        printf("USB Read Control MSG - buffer[0]=C - Camera was woken up\n\n");
			/* *
			int xretval;
		        xretval = usb_control_msg(cameraudh,
                		                USB_TYPE_VENDOR|USB_RECIP_DEVICE|USB_DIR_IN,
                                		size > 1 ? 0x04 : 0x0c,
		                                value,
                		                0,
                                		buffer,
		                                0x58,
                		                usb_timeout);
                	printf("EOS 2nd INIT - READ CONTROL MSG, value %X, size %d: %s\n",
                        	value, size, xretval == -1 ? "FAILED" : "OK");
                	if (xretval != -1)
                        	dump_hex("DATA", buffer, size);
			* */
		} else {
			printf("USB Read Control MSG - buffer[0]=0x%x - Unknown camera response\n\n", buffer[0]);
			//print_buffer (buffer, USB_BUFFER_SIZE);
		}
        }
	return retval;
}

int USB_read(void *buffer, int size)
{
	int retval;

	retval = usb_bulk_read(cameraudh, input_ep, buffer, size, usb_timeout);
	if (opt_debug) {
		printf("USB READ: %s (%X)\n", retval == -1 ? "FAILED" : "OK", retval);
		if (retval != -1)
			dump_hex("DATA", buffer, size);
	}
	return retval;
}

int USB_write(void *buffer, int size)
{
	int retval;

	retval = usb_bulk_write(cameraudh, output_ep, buffer, size, usb_timeout);
	if (opt_debug) {
		printf("USB WRITE: %s (%X)\n", retval == -1? "FAILED" : "OK", retval);
		if (retval != -1)
			dump_hex("DATA", buffer, size);
	}
	return retval;
}

int USB_cmd(unsigned char cmd1, unsigned char cmd2, unsigned int cmd3, unsigned int serial, unsigned char *payload, int size)
{
	unsigned char buffer[4096];
	unsigned int aux;

	aux = size+0x10;

	memset(buffer, 0, 4096);
	*(unsigned int*)buffer = byteswap32(aux);
	*(unsigned int*)(buffer+4) = byteswap32(cmd3);
	buffer[0x40] = 0x02;
	buffer[0x44] = cmd1;
	buffer[0x47] = cmd2;
        if ( get_camera_class(camera_model) == canon_class6 ) {
                /* New style of protocol is picky about this byte. */
                buffer[0x46] = cmd3 == 0x202 ? 0x20 : 0x10;
        }
	*(unsigned int*)(buffer+0x48) = byteswap32(aux);
	*(unsigned int*)(buffer+0x4c) = byteswap32(serial);
	if (payload != NULL)
		memcpy(buffer+USB_HEADER_SIZE, payload, size);
	return USB_write_control_msg(0x10, buffer, USB_HEADER_SIZE+size);
}

void USB_initial_sync(void)
{
	struct usb_device *camera_dev;
        int retval;
        unsigned char buffer[4096];
	USB_INIT_RESULT init_val;

	usb_timeout = 500;
	init_val = USB_camera_init(&camera_dev);
	if (init_val == NOCAMERA) {
		if (opt_debug)
			printf("\n");
		printf("Camera not found, please press the shot button and\n"
			"  check that the camera is in PC mode, then retry.\n");
		if (!opt_debug)
			printf("Use the -D option to determine the PRODUCT ID and\n"
				" modify the source code to support this camera if appropriate.\n");
		if (opt_debug)
			printf("If you understand the DANGER to your CAMERA try the\n"
				" -Z command line option and FORCE the camera support.\n"
				"READ and UNDERSTAND the dangers in the README files FIRST!\n");
		exit(1);
	} else if (init_val == USB_INIT_FAILED) {
		printf("Fatal error initializing USB\n");
		exit(1);
	} else if (init_val == USB_INIT_DANGER) {
		printf("DANGER: This camera was not found specifically listed;\n"
			"       using the -Z override MAY DAMAGE YOUR CAMERA!\n");
	}

	cameraudh = usb_open(camera_dev);
	if (!cameraudh) {
		printf("usb_open() error, can't open the camera\n");
		exit(1);
	}

        retval = usb_set_configuration(cameraudh, configuration);
        if (retval == USB_ERROR) {
                printf("usb_set_configuration() error\n");
                exit(1);
        }

        retval = usb_claim_interface(cameraudh, interface);
        if (retval == USB_ERROR) {
                printf("usb_claim_interface() error\n");
                exit(1);
        }

        retval = usb_set_altinterface(cameraudh, alternate);
        if (retval == USB_ERROR) {
                printf("usb_set_altinterface() error\n");
                exit(1);
        }

        if (opt_debug)
                printf("USB: Camera successful open\n");

        while (USB_read_control_msg(0x55, buffer, 1) == -1);
        USB_read_control_msg(0x1, buffer, 0x58);
        USB_write_control_msg(0x11, buffer+0x48, sync_size(camera_model));
        USB_read(buffer, 0x44);
	usb_timeout = 3000;
}

char *USB_get_id(void)
{
	int retval;
	static char buffer[USB_BUFFER_SIZE];

        USB_cmd(0x01, 0x12, 0x201, 0x01, NULL, 0);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
	if (retval == -1) return NULL;
	firmware[1] = firmware[3] = firmware[5] = '.';
	firmware[0] = buffer[0x5b]+'0';
	firmware[2] = buffer[0x5a]+'0';
	firmware[4] = buffer[0x59]+'0';
	firmware[6] = buffer[0x58]+'0';
	firmware[7] = '\0';
	strncpy (camera_name, &buffer[0x5c], 0x20);
	if (get_camera_class(camera_model) != canon_class6) {
	  strncpy (camera_owner, &buffer[0x7c], 0x20);
	} else {
          USB_cmd(0x05, 0x12, 0x201, 0x01, NULL, 0);
          retval = USB_read(buffer, USB_BUFFER_SIZE);
	  if (retval == -1) return NULL;
	  strncpy (camera_owner, &buffer[0x54], 0x20);
	}
	return buffer+0x5c;
}

unsigned int *USB_body_id(void)
{
        int retval;
	unsigned int *value;
        static char buffer[USB_BUFFER_SIZE];

#if 1
        USB_cmd(0x1D, 0x12, 0x201, 0x01, NULL, 0);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;
#else
        unsigned char cmd1, cmd2;
	unsigned int cmd3;
	unsigned char cmd2s[] = { 0x11, 0x12, 0x21, 0x22 };
	unsigned char xxx[] = "    ";
	for (cmd3 = 0x201; cmd3 <= 0x203; cmd3++)
	for (cmd2 = 0; cmd2 < 4; cmd2++)
	for (cmd1 = 0x01; cmd1 <= 0x20; cmd1++) {
	  printf ("cmd1 = 0x%x, cmd2 = 0x%x, cmd3 = 0x%x\n", cmd1, cmd2s[cmd2], cmd3);
          USB_cmd(cmd1, cmd2s[cmd2], cmd3, 0x01, xxx, 4);
          memset (buffer, 0x0, USB_BUFFER_SIZE);
	  retval = USB_read(buffer, USB_BUFFER_SIZE);
	  if (retval != -1) {
	    dump_hex ("DATA", buffer, USB_BUFFER_SIZE);
	  }
        }
#endif
	memcpy(&value, buffer+0x54, 4);
	return value;
}

char *USB_set_owner(char *name)
{
        int retval;
        static char buffer[USB_BUFFER_SIZE];
        unsigned char xxxx[35];

	/*printf("The name is _%s_ and its _%d_ long.\n", name, strlen(name));*/
        
	memcpy(xxxx, name, strlen(name)+1);
        if (get_camera_class(camera_model) != canon_class6) {
		USB_cmd(0x05, 0x12, 0x201, 0x01, xxxx, strlen(name)+1);
        } else {
		USB_cmd(0x06, 0x12, 0x201, 0x01, xxxx, strlen(name)+1);
	}
	retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;
        return buffer+0x5c;
}

char *USB_control_camera(void)
{
        int retval, counter;
        static char buffer[USB_BUFFER_SIZE];
        unsigned char xxxx[0x20];

	unsigned char cntl_command = 0x13;	
        if (get_camera_class(camera_model) == canon_class6) {
		cntl_command = 0x25;
	}

	/* Control Init */
	memset(xxxx, 0, 0x20);
	xxxx[0] = 0x00;
        xxxx[1] = 0x00;
        USB_cmd(cntl_command, 0x12, 0x201, 0x01, xxxx, 0x18);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

        /* Set transfer mode */
	/* 0x09, 0x04, 0x03 or 0x09, 0x04, 0x02, 0x00, 0x00, 0x03 */
	memset(xxxx, 0, 0x20);
        xxxx[0] = 0x09;
        xxxx[1] = 0x04;
	xxxx[2] = 0x03;
	/*
	xxxx[2] = 0x02;
	xxxx[3] = 0x00;
	xxxx[4] = 0x00;
	xxxx[5] = 0x03;
	*/
        USB_cmd(0x13, 0x12, 0x201, 0x01, xxxx, 0x18);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

	/* Release Shutter */
	memset(xxxx, 0, 0x20);
	xxxx[0] = 0x04;
	xxxx[1] = 0x00;
        USB_cmd(cntl_command, 0x12, 0x201, 0x01, xxxx, 0x18);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

	/* Control Exit */
	/* Need a delay here to wait until the camera has finished */
	memset(xxxx, 0, 0x20);
        xxxx[0] = 0x01;
        xxxx[1] = 0x00;
	for (counter = 3; counter > 1; counter--) {
		fprintf(stderr, "%02d\b\b", counter);
		sleep(1);
		if (counter < 9) {
        		USB_cmd(cntl_command, 0x12, 0x201, 0x01, xxxx, 0x18);
        		retval = USB_read(buffer, USB_BUFFER_SIZE);
        		if (retval != -1)
                		counter = 0;
		}
	}

	mydisplay = 0;
	retval = camera_get_list(lastpath);
	mydisplay = 1;
        if (retval == -1) {
            printf("ls error\n");
	    return NULL;
        }

        int j, largest=0, current;
        char big[1024];
        char aux[1024];
        for (j = 0; j < dirlist_size; j++) {
               snprintf(aux, 5, "%s", dirlist[j]->name+4);
               current=atoi(aux);
              if (current > largest) {
                   largest=current;
                   snprintf(big, 1024, "%s", dirlist[j]->name);
              }
              /* printf("LenBig=%d, aux=%s, current=%d, largest=%d, file=%s\n", strlen(big), aux, current, largest, big);*/
	}
	if (strlen(big) == 12 ) 
		snprintf(buffer+0x5c, sizeof(big), "%s", big);
        else 
		snprintf(buffer+0x5c, sizeof(big), "UNKNOWN");
	return buffer+0x5c;
}

static char *USB_camera_pars(unsigned char cmd, unsigned char *payload, int size)
{
        int retval;
        static char buffer[USB_BUFFER_SIZE];
        unsigned char xxxx[0x40];
	
	unsigned char cntl_command = 0x13;	
        if (get_camera_class(camera_model) == canon_class6) {
		cntl_command = 0x25;
	}

	/* Control Init */
	memset(xxxx, 0, 0x20);
	xxxx[0] = 0x00;
        xxxx[1] = 0x00;
        USB_cmd(cntl_command, 0x12, 0x201, 0x01, xxxx, 0x18);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

        /* Set transfer mode */
	/* 0x09, 0x04, 0x03 or 0x09, 0x04, 0x02, 0x00, 0x00, 0x03 */
	memset(xxxx, 0, 0x20);
        xxxx[0] = 0x09;
        xxxx[1] = 0x04;
	xxxx[2] = 0x03;
	/*
	xxxx[2] = 0x02;
	xxxx[3] = 0x00;
	xxxx[4] = 0x00;
	xxxx[5] = 0x03;
	*/
        USB_cmd(0x13, 0x12, 0x201, 0x01, xxxx, 0x18);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

	/* Get release parameters */
	memset(xxxx, 0, size+5);
	xxxx[0] = cmd;
	xxxx[1] = 0x00;
	xxxx[2] = 0x00;
	xxxx[3] = 0x00;
	memcpy (&xxxx[4], payload, size);
        USB_cmd(cntl_command, 0x12, 0x201, 0x01, xxxx, size+4);
	retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;
	return buffer;
}

/* Get release parameters: 0A

Takes no parameters. Returns 0x34 bytes after the echo of the subcommand code. First 4-byte integer is 0x00000030, which is the count of remaining bytes. I assume that the 0x34 bytes are the same as is sent down by code 07, set release parameters.

Command: 0x58 bytes

0000 18 00 00 00 01 02 00 00 00 00 00 00 00 00 00 00 ................
0010 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0020 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0030 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0040 02 00 00 00 13 00 00 12 18 00 00 00 14 c4 12 00 ................
0050 0a 00 00 00 00 00 00 00                         ........
        

Response 0x8C bytes:

0000 00 00 00 00 01 03 00 00 00 00 00 00 00 00 00 00 ................
0010 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0020 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0030 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0040 02 00 00 00 13 00 00 22 4c 00 00 00 14 c4 12 00 ......."L.......
0050 00 00 00 00 0a 00 00 00 30 00 00 00 00 04 02 00 ........0.......
0060 00 00 00 00 04 01 01 ff 03 ff 01 30 00 ff 00 ff ...........0....
0070 00 00 00 7f 10 00 60 00 38 00 7c 00 18 18 18 ff ......`.8.|.....
0080 20 00 50 00 14 00 14 00 14 00 01 00             ..P.........
*/
	
char *USB_camera_getpars(int print)
{
        unsigned char xxxx[0x20];
	memset(xxxx, 0, 0x20);
	/* Release parameters */
	char *buffer = USB_camera_pars (0x0A, xxxx, 0x16);
	if (!buffer) 
		return NULL;

	//if (print) print_buffer (&buffer[0x58], USB_COMMAND_SIZE);
	if (print) print_buffer (buffer, USB_BUFFER_SIZE);
	
#if 0
	memset(xxxx, 0, 0x20);
	/* Extended release parameters size */
	buffer = USB_camera_pars (0x10, xxxx, 0x18);
	if (!buffer) 
		return NULL;

	int size = buffer[0x54];
	printf ("extended parameter size = 0x%x\n", size);
	memset(xxxx, 0, 0x20);
	/* Extended release parameters */
	buffer = USB_camera_pars (0x10, xxxx, 0x12);
	if (!buffer) 
		return NULL;

	if (print) print_buffer (buffer, USB_BUFFER_SIZE);
#endif
	
	return buffer;
}

char *USB_camera_getpar(char *name)
{
  char *buffer = USB_camera_getpars (0);
  char *value;
  if (!buffer) return NULL;
  
  switch (get_param_value (name, &value, &buffer[0x58])) {
    case PARAM_OK: return value;
    case PARAM_NO_SUCH_NAME:
      fprintf (stderr, "Illegal parameter name: %s\n", name);
      break;
    case PARAM_NO_SUCH_VALUE:
      fprintf (stderr, "Unexpected parameter value\n"); /* Internal error */
      break;    
    case PARAM_NOT_FOR_THIS_MODE:
      fprintf (stderr, "Unexpected parameter %s in mode %s\n", name, current_camera_mode);
      break;    
    case PARAM_NOT_SUPPORTED:
      fprintf (stderr, "Parameter %s is not supported for camera %s\n", name, camera_name);
      break;    
    case PARAM_CANNOT_BE_SET:
      fprintf (stderr, "Parameter %s cannot be set\n", name);
      break;    
  }
  return NULL;
}

/* Set release parameters: 07

Takes a payload of 0x38 bytes: 0x34 after subcommand code. Offset 14 in these bytes (18 from the start of the payload) controls the flash:

#define FLASH_OFF 0x00
#define FLASH_ON 0x01
#define FLASH_AUTO 0x02
        

Response: 0x5c bytes (failure):

0000:  00 00 00 00 01 03 00 00 00 00 00 00 00 00 00 00  ................
0010:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0020:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0030:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0040:  02 00 00 00 13 00 00 22 1c 00 00 00 2f 00 00 00  ......."..../...
0050:  00 00 00 00 07 01 00 ff 00 00 00 00              ............
        

(success):

0000:  00 00 00 00 01 03 00 00 00 00 00 00 00 00 00 00  ................
0010:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0020:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0030:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
0040:  02 00 00 00 13 00 00 22 1c 00 00 00 2f 00 00 00  ......."..../...
0050:  00 00 00 00 07 00 00 00 00 00 00 00              ............
*/

int USB_camera_setpar(char *name, char *value)
{
	char *buffer = USB_camera_getpars (0);
	if (!buffer) return 0;

	if (opt_debug) {
	  printf ("old buffer\n"); print_buffer(&buffer[0x58], USB_COMMAND_SIZE);
	}
	
	switch (set_param_value (name, value, &buffer[0x58])) {
	  case PARAM_OK: 
	    //print_buffer (&buffer[0x58], USB_COMMAND_SIZE);
	    break;
	  case PARAM_NO_SUCH_NAME:
	    fprintf (stderr, "Illegal parameter name: %s\n", name);
	    return 0;
	  case PARAM_NO_SUCH_VALUE:
	    fprintf (stderr, "Illegal parameter value: %s\n", value);
	    return 0;    
	  case PARAM_NOT_FOR_THIS_MODE:
	    fprintf (stderr, "Cannot set parameter %s in mode %s\n", name, current_camera_mode);
	    return 0;    
	  case PARAM_NOT_SUPPORTED:
	    fprintf (stderr, "Parameter %s is not supported for camera %s\n", name, camera_name);
	    return 0;    
	  case PARAM_CANNOT_BE_SET:
	    fprintf (stderr, "Parameter %s cannot be set\n", name);
	    return 0;    
	}

	if (opt_debug) {
		printf ("new buffer\n"); print_buffer(&buffer[0x58], USB_COMMAND_SIZE);
	}
	
	/* Fill in payload */
	unsigned char payload[0x34];
	memcpy (payload, &buffer[0x58], 0x34);
	
	buffer = USB_camera_pars (0x07 , payload, 0x34);
	if (!buffer) return 1;
	return 0;
}

char *USB_camera_getcustom(char *name)
{
        unsigned char xxxx[0x40];
	int index;
	char *buffer;
	char *value;
	
	switch (get_custom_index (name, &index)) {
	  case PARAM_OK: break;
	  case PARAM_NO_SUCH_NAME:
	    fprintf (stderr, "Illegal custom function name: %s\n", name);
	    return NULL;
	  case PARAM_NO_SUCH_VALUE:
	    fprintf (stderr, "Unexpected custom function value\n"); /* Internal error */
	    return NULL;    
	  case PARAM_NOT_FOR_THIS_MODE:
	    fprintf (stderr, "Unexpected custom function %s in mode %s\n", name, current_camera_mode);
	    return NULL;    
	  case PARAM_NOT_SUPPORTED:
	    fprintf (stderr, "Custom function %s is not supported for camera %s\n", name, camera_name);
	    return NULL;    
	  case PARAM_CANNOT_BE_SET:
	    fprintf (stderr, "Custom function %s cannot be set\n", name);
	    return NULL;    	  
	}
	
	memset(xxxx, 0, 0x40);
	xxxx[0] = 0x0a;
	xxxx[4] = index;
	do {
	  buffer = USB_camera_pars (0x0f, xxxx, 0x16);
	} while (!buffer);

        get_custom_value (name, &value, buffer[0x60]);
	printf ("Custom parameter %s = %s\n", name, value);
	//print_buffer(&buffer[0x58], USB_COMMAND_SIZE);
	return value;
}

int USB_camera_setcustom(char *name, char *value)
{
	int index;
	int val;
	switch (set_custom_value (name, value, &index, &val)) {
	  case PARAM_OK: break;
	  case PARAM_NO_SUCH_NAME:
	    fprintf (stderr, "Illegal custom function name: %s\n", name);
	    return 0;
	  case PARAM_NO_SUCH_VALUE:
	    fprintf (stderr, "Unexpected custom function value\n"); /* Internal error */
	    return 0;    
	  case PARAM_NOT_FOR_THIS_MODE:
	    fprintf (stderr, "Unexpected custom function %s in mode %s\n", name, current_camera_mode);
	    return 0;    
	  case PARAM_NOT_SUPPORTED:
	    fprintf (stderr, "Custom function %s is not supported for camera %s\n", name, camera_name);
	    return 0;    
	  case PARAM_CANNOT_BE_SET:
	    fprintf (stderr, "Custom function %s cannot be set\n", name);
	    return 0;    	  
	}

        unsigned char xxxx[0x20];
	char *buffer;
	
	memset(xxxx, 0, 0x20);
	xxxx[0] = 0x0a;
	xxxx[4] = index;
	xxxx[6] = index;
	xxxx[8] = val;
	do {
	  buffer = USB_camera_pars (0x0e, xxxx, 0x16);
	} while (!buffer);

	return 1;
}

int USB_focus(int focus)
{
        //unsigned char xxxx[0x20];
	//memset(xxxx, 0, 0x20);
	char *buffer = USB_camera_pars (focus ?0x02:0x03, NULL, 0x0);
	if (!buffer) 
		return 0;
        print_buffer (buffer, USB_BUFFER_SIZE);
	return 1;
}

int USB_shots(void)
{
        //unsigned char xxxx[0x20];
	//memset(xxxx, 0, 0x20);
	char *buffer = USB_camera_pars (0x0d, NULL, 0x0);
	if (!buffer) 
		return -1;
	return *(int*)&buffer[0x5c];
}

char *USB_get_disk(void)
{
	int retval;
	static char buffer[4096];

        if (opt_debug) printf ("USB_get_disk\n");
	if (get_camera_class(camera_model) != canon_class6) {
		USB_cmd(0x0a, 0x11, 0x202, 0x01, NULL, 0);
	} else {
		USB_cmd(0x0e, 0x11, 0x202, 0x01, NULL, 0);
	}
	USB_read(buffer, 0x40);
	memcpy(&retval, buffer+6, 4);
	USB_read(buffer, retval);

	return buffer;
}

#define BULK_TR_SIZE	0x1000 /* PAGE_SIZE */
unsigned char *USB_get_data(char *pathname, int reqtype, int *retlen)
{
	unsigned char buffer[4096*2];
	unsigned char *image;
	int aux = BULK_TR_SIZE;
	int size;
	int totalsize;
	int n_read = 0;
	int offset = 8;

	memset(buffer, 0, 4);
	buffer[0] = reqtype; /* select image or thumbnail */
	if (get_camera_class(camera_model) == canon_class6) {
          offset = 4;
        }
	*(unsigned int*)(buffer+4) = byteswap32(aux);
        memcpy(buffer+offset, pathname, strlen(pathname)+1);
	USB_cmd(0x01, 0x11, 0x202, 0x01, buffer, strlen(pathname)+offset+1);
        USB_read(buffer, 0x40);
	totalsize = byteswap32(*(unsigned int*)(buffer+6));
	if (totalsize == 0)
		return NULL;
	*retlen = totalsize;
	image = malloc(totalsize);
	if (!image) {
		perror("malloc");
		exit(1);
	}

	printf("Getting %s, %d bytes\n", pathname, totalsize);
	progressbar(PROGRESS_RESET, 0, 0);
       	while(1) {
               	size = (totalsize > BULK_TR_SIZE) ? BULK_TR_SIZE : totalsize;
               	USB_read(image+n_read, size);
               	totalsize -= size;
		n_read += size;
		progressbar(PROGRESS_PRINT, *retlen, n_read);
               	if (totalsize == 0) break;
       	}
	return image;
}

char *USB_setdate(void)
{
	#include <time.h>
	#include <sys/time.h>
	#include <err.h>

	int retval;
	char *RC="OK";
	static char buffer[USB_BUFFER_SIZE];
	unsigned char xxxx[100];
	/* */
	struct timeval tv[2];
	struct tm *tm1, *tm2;
	struct tm tm1_copy;
	time_t ta, tb, t1, t2, t3;
	int mydiff, dstoffset;

	if (gettimeofday(&tv[0], NULL))
		err(1, "gettimeofday");

	tm1 = localtime ((const time_t *) &(tv[0].tv_sec));
	tm1_copy = *tm1; /* struct copy */

	tm2 = gmtime ((const time_t *) &(tv[0].tv_sec));

	t1 = mktime (&tm1_copy);
	t2 = mktime (tm2);
	mydiff = t2-t1;

	/* No offset if standard/winter time. */
  	dstoffset = 0;
  	/* Adjust by 1 hour if it is Daylight/Summer time. */
  	if (tm1->tm_isdst == 1)
     		dstoffset = 3600;
	t3 = t1-mydiff+dstoffset;

	/* 
        if (dstoffset == 0)
		printf("USB_setdate - Standard/Winter Time\n");
	else
		printf("USB_setdate- Daylight/Summer Time\n");
	printf("Local Time1 %d, 0x%X, %s", t1, t1, ctime(&t1));
	printf("  GMT Time2 %d, 0x%X, %s", t2, t2, ctime(&t2));
	printf("CameraTime3 %d, 0x%X, %s", t3, t3, ctime(&t3));
	*/

	/* Get initial date from camera, save for later in case write fails*/	
	tb = USB_get_date();
	tb += GMT_offset;

	memcpy(xxxx, &t3, 4);
	xxxx[4] = 0;
	xxxx[5] = 0;
        xxxx[6] = 0;
        xxxx[7] = 0;
        xxxx[8] = 0;
        xxxx[9] = 0;
        xxxx[10] = 0;
        xxxx[11] = 0;
        USB_cmd(0x04, 0x12, 0x201, 0x01, xxxx, 12);
        retval = USB_read(buffer, USB_BUFFER_SIZE);
        if (retval == -1)
                return NULL;

        ta = USB_get_date();
        ta += GMT_offset;
	printf("Camera date before: %s", ctime(&tb));
        printf("Camera date after:  %s", ctime(&ta));
        return RC;
}

time_t USB_get_date(void)
{
	time_t curtime;
	unsigned char buffer[1024];

	USB_cmd(0x03, 0x12, 0x201, 0x01, NULL, 0);
	USB_read(buffer, 0x60);
	curtime = byteswap32(*(time_t*)(buffer+0x54));

	return curtime;
}

int USB_get_disk_info(char *disk, int *size, int *free)
{
	unsigned char buffer[1024];
	char diskstr[] = "X:\\";

	diskstr[0] = disk[0];
	USB_cmd(0x09, 0x11, 0x201, 0x01, diskstr, 4);
	USB_read(buffer, 0x5c);
	if (buffer[USB_HEADER_SIZE] != 0)
		return -1;
	*size = byteswap32(*(unsigned int*)(buffer+0x54));
	*free = byteswap32(*(unsigned int*)(buffer+0x58));
	return 0;
}

int USB_get_power_status(int *good, int *ac)
{
	unsigned char buffer[1024];

	USB_cmd(0x0a, 0x12, 0x201, 0x01, NULL, 0);
	USB_read(buffer, 0x58);
	if (*(buffer+0x54) == 0x06)
		*good = 1;
	else
		*good = 0;

	if (*(buffer+0x57) == 0x10)
		*ac = 1;
	else
		*ac = 0;

	return 0;
}

int USB_mkdir(char *pathname)
{
	unsigned char buffer[1024];
	unsigned char arg[1024];

	if (pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	USB_cmd(0x5, 0x11, 0x201, 0x01, pathname, strlen(pathname)+1);
	USB_read(buffer, 0x54);
	if (buffer[USB_HEADER_SIZE] == 0)
		return 0;
	else
		return -1;
}

int USB_rmdir(char *pathname)
{
	unsigned char buffer[1024];
	unsigned char arg[1024];

	if (pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	USB_cmd(0x6, 0x11, 0x201, 0x01, pathname, strlen(pathname)+1);
	USB_read(buffer, 0x54);
	if (buffer[USB_HEADER_SIZE] == 0)
		return 0;
	else
		return -1;
}

int USB_delete(char *pathname)
{
	unsigned char buffer[1024];
	unsigned char cmd = 0x0d;
	unsigned char response = 0x86;
	
	if (get_camera_class(camera_model) == canon_class6) {
	  cmd = 0x0a;
	  response = 0x00;
	}
	memcpy(buffer, lastpath, strlen(lastpath)+1);
	memcpy(buffer+strlen(lastpath)+1, pathname, strlen(pathname)+1);
	buffer[strlen(lastpath)] = '\\';
	USB_cmd(cmd, 0x11, 0x201, 0x01, buffer, strlen(lastpath)+1+
		strlen(pathname)+1);
	USB_read(buffer, 0x54);
	dump_hex ("DELETE", buffer, USB_BUFFER_SIZE);
	if (buffer[USB_HEADER_SIZE] == response)
		return 0;
	else
		return -1;
}

int USB_set_file_attrib(char *pathname, unsigned char newattrib)
{
	unsigned char buffer[1024];

	buffer[0] = newattrib;
	buffer[1] = buffer[2] = buffer[3] = 0x00;
	memcpy(buffer+4, lastpath, strlen(lastpath)+1);
	buffer[4+strlen(lastpath)] = '\\';
	memcpy(buffer+4+strlen(lastpath)+1, pathname, strlen(pathname)+1);
	USB_cmd(0x0e, 0x11, 0x201, 0x01, buffer, 4+strlen(lastpath)+1+
		strlen(pathname)+1);
	USB_read(buffer, 0x54);
	if (buffer[USB_HEADER_SIZE] == 0x86)
		return 0;
	else
		return -1;
}

int USB_upload(char *source, char *target)
{
	struct stat buf;
	unsigned char buffer[4096*2];
	char read_buffer[0x1400];
	unsigned int serial, datalen, offset;
	unsigned int len1, lenaux, aux;
	unsigned short saux;
	int fd, progress_bar = 0;
	char arg[1024];

	if (target == NULL) {
		char *p;
		p = strrchr(source, '/');
		if (p != NULL)
			target = p++;
		else
			target = source;
		p = strchr(target, '.');
		if (p != NULL && strchr(p+1, '.') != NULL) {
			printf("sorry, only one dot allowed in target filename\n");
			return -1;
		}
	}

	if (strlen(target) <= 2 || target[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, target);
		target = arg;
	}

	serial = 0x12345678;
	offset = 0;

	fd = open(source, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	if (fstat(fd, &buf) == -1) {
		perror("stat");
		printf("WARING: s10sh will not show the progress bar\n\n");
	} else {
		progress_bar = 1;
		progressbar(PROGRESS_RESET, 0, 0);
	}

	while(1) {
		datalen = read(fd, read_buffer, 0x1400);
		if (datalen == 0) {
			break;
		} else if (datalen == -1) {
			perror("read");
			return -1;
		}

		len1 = 0x1c+strlen(target)+1+datalen;

		memset(buffer, 0, 4);
		buffer[4] = 0x03;
		buffer[5] = 0x02;
		lenaux = len1+0x40;
		memcpy(buffer+6, &lenaux, 4);
		memset(buffer+10, 0, 0x36);
		USB_write_control_msg(0x10, buffer, 0x40);

		USB_read(buffer, 0x40);

		memcpy(buffer, &len1, 4);
		aux = 0x0403;
		memcpy(buffer+4, &aux, 4);
		memset(buffer+8, 0, 0x38);

		aux = 0x02;
		memcpy(buffer+0x40, &aux, 4);
		saux = 0x03;
		memcpy(buffer+0x44, &saux, 2);
		buffer[0x46] = 0x00;
		buffer[0x47] = 0x11;
		memcpy(buffer+0x48, &len1, 4);
		memcpy(buffer+0x4c, &serial, 4);
		aux = 0x02;
		memcpy(buffer+0x50, &aux, 4);
		memcpy(buffer+0x54, &offset, 4);
		memcpy(buffer+0x58, &datalen, 4);
		memcpy(buffer+0x5c, target, strlen(target)+1);
		memcpy(buffer+0x5c+strlen(target)+1, read_buffer, datalen);

		USB_write(buffer, len1+0x40);
		USB_read(buffer, 0x5c);
		offset += datalen;
		progressbar(PROGRESS_PRINT, buf.st_size, offset);
	}
	close(fd);
	printf("\n");
	return 0;
}

void USB_close(void)
{
	int retval;

	retval = usb_release_interface(cameraudh, interface);
	if (retval == USB_ERROR) {
		printf("usb_claim_interface() error\n");
		exit(1);
	}
	usb_close(cameraudh);
}

#endif /* HAVE_USB_SUPPORT */

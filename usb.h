/* This file is part of s10sh
 *
 * Copyright (C) 2000 by Salvatore Sanfilippo <antirez@invece.org>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#ifndef S10SH_USB_H
#define S10SH_USB_H

#include <usb.h>

#define VENDOR_ID_CANON         0x04A9
#define PRODUCT_ID_S10          0x3041	/* PowerShot S10 */
#define PRODUCT_ID_S20          0x3043	/* PowerShot S20 */
#define PRODUCT_ID_A20		0x304E  /* PowerShot A20 */
#define PRODUCT_ID_A60		0x3074	/* PowerShot A60 */
#define PRODUCT_ID_S30          0x3057  /* PowerShot S30 */
#define PRODUCT_ID_S100_US	0x3045	/* S100, aka. Digital Ixus, Elph */
#define PRODUCT_ID_S100_EU	0x3047	/* S100, aka. Digital Ixus, Elph */
#define PRODUCT_ID_G1		0x3048	/* PowerShot G1 */
#define PRODUCT_ID_G3		0x306E
#define PRODUCT_ID_S400         0x3075  /* PowerShot S400 */
#define PRODUCT_ID_DIGITAL_IXUS_V3 0x3070 /* Digital IXUS V3 */
#define PRODUCT_ID_10D		0x3083	/* EOS-10D */
#define PRODUCT_ID_DIG_V2	0x3065	/* Digital V2 */
#define PRODUCT_ID_DRebel       0x3084  /* Digital Rebel */
#define PRODUCT_ID_EOS20D       0x30EB  /* EOS-20D */
#define PRODUCT_ID_A75          0x30B5  /* PowerShot A75 */
#define PRODUCT_ID_IXUS_65      0x30FE  /* Digital IXUS 65 */
#define PRODUCT_ID_EOS350D      0x30EE  /* EOS-350D */

/* The Canon USB protocol of the S10, S20, S100, G1 is the same.
 * We can hope that the next cameras will adopt a compatible protocol
 * so we try this product-id-guessing joke. */

#define PRODUCT_ID_NEXTDIGICAM1	0x3049	/* The next canon camera? */
#define PRODUCT_ID_NEXTDIGICAM2 0x3050
#define PRODUCT_ID_NEXTDIGICAM3 0x3052
#define PRODUCT_ID_NEXTDIGICAM4 0x3053
#define PRODUCT_ID_NEXTDIGICAM5 0x3054
#define PRODUCT_ID_NEXTDIGICAM6 0x3055

/*
 * USB directions
 */
#define USB_DIR_OUT                     0
#define USB_DIR_IN                      0x80

/* USB ERROR -- for compatibility between libusb 1.0.0 and 1.0.1 */
#ifndef USB_ERROR
#define USB_ERROR			-1
#endif

/* libusb prototypes */
int usb_find_busses(void);

/* USB specific prototypes */
int USB_read(void *buffer, int size);
int USB_write(void *buffer, int size);
int USB_cmd(unsigned char cmd1, unsigned char cmd2, unsigned int cmd3, unsigned int serial, unsigned char *payload, int size);
void USB_initial_sync(void);
char *USB_get_id(void);
unsigned int *USB_body_id(void);
char *USB_set_owner(char *name);
char *USB_control_camera(void);
char *USB_camera_getpars(int print);
char *USB_camera_getpar(char *name);
int   USB_camera_setpar(char *name, char *value);
char *USB_camera_getcustom(char *name);
int   USB_camera_setcustom(char *name, char *value);
int   USB_focus(int start);
int   USB_shots(void);
char *USB_get_disk(void);
unsigned char *USB_get_data(char *pathname, int reqtype, int *retlen);
time_t USB_get_date(void);
char *USB_setdate(void);
int USB_get_disk_info(char *disk, int *size, int *free);
int USB_get_power_status(int *good, int *ac);
int USB_mkdir(char *pathname);
int USB_rmdir(char *pathname);
int USB_delete(char *pathname);
int USB_set_file_attrib(char *pathname, unsigned char newattrib);
int USB_upload(char *source, char *target);
void USB_close(void);
#endif /* S10SH_USB_H */

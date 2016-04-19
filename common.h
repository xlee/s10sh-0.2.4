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

#ifndef S10SH_COMMON_H
#define S10SH_COMMON_H

typedef enum {
  UNKOWN_CAMERA =   0,
  S10           =   1,       /* S10 found */
  S20           =   2,       /* S20 found */
  S30           =   3,       /* S20 found */
  S100		=   4,	     /* S100 (Digital Ixus) found */
  G1		=   5,	     /* G1 found */
  A20		=   6,	     /* A20 found */
  G2		=   7,	     /* G2 found */
  G3		=   8,	     /* G3 found */
  EOS10D	=   9,	     /* EOS-10D found */
  IXUS_V3      	=  10,       /* IXUS V3 found */
  A30          	=  11,       /* A30 found */
  A60          	=  12,       /* A60 found */
  S400         	=  13,       /* S400 (Digital Ixus) found */
  DIG_V2       	=  14,       /* Digital V2 */
  DRebel       	=  15,       /* Digital Rebel */
  EOS20D       	=  16,       /* EOS-20D */
  A75          	=  17,       /* PowerShot A75 */
  IXUS_65       =  18,       /* IXUS 65 */
  EOS350D       =  19,       /* EOS-350D */
  NEW_CAMERA   	= 100,	     /* Unsupported PowerShot found! */
} camera_type;

extern char        camera_name[];
extern camera_type camera_model;
extern char        camera_owner[];

unsigned long get_usec(void);
int camera_last_ls(void);
int camera_get_last_ls(int which);
int camera_get_list(char *pathname);
void dump_filename(struct canonfile *f);
int offset_from_GMT(void);
int camera_get_image(char *pathname, char *destfile);
int camera_get_thumb(char *pathname, char *destfile);
int view_thumb(char *pathname);
int view_all(void);
int camera_get_file_attr(char *name);
int camera_file_chmod(char *name, int action, int bits);
int camera_file_chmod_all(int action, int bits);
int camera_delete_all(int which);
int camera_close(void);
char *camera_get_id(void);
char *camera_set_owner(char *name);
char *camera_get_disk(void);
void camera_startup_initialization(void);
void camera_ping(void);
int camera_get_disk_info(char *disk, int *size, int *free);
int camera_get_power_status(int *good, int *ac);
void dump_hex(const char *msg, const unsigned char *buf, int len);

#endif

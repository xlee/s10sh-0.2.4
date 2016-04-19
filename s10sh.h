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

#include <time.h>

#define VERSION "0.2.2C Mitton"

/* file attributes */
#define ATTR_PROTECTED	(1<<0) /* file is protected */
#define ATTR_ITEMS	(1<<4) /* directory contains item not rec. entered */
#define ATTR_NEW	(1<<5) /* new means not downloaded */
#define ATTR_ENTERED	(1<<7) /* recursived entered directory */

/* chmod behaviour flags */
#define CHMOD_SET	1
#define CHMOD_CLEAR	0

/* getlastls behaviour flags */
#define WHICH_ALL	0
#define WHICH_NEW	1
#define WHICH_OLD	2

#define COMMANDARGS_MAX 32

#define TEMP_FILE_NAME "./s10sh_REMOVE_ME"

/* driver modes */
#define SERIAL_MODE	0
#define USB_MODE	1

/* directory listing recursion types */
#define DL_NO_RECURSION	0x00
#define DL_ONE_RECURSION 0x01
#define DL_FULL_RECURSION 0x02

/* global vars and structures */
struct canonfile {
	unsigned char type;
	unsigned int size;
	time_t date;
	char name[1024];
};

extern int opt_debug;
extern int opt_overwrite;
extern char prompt[1024];
extern struct canonfile *dirlist[1024];
extern int dirlist_size;
extern char lastpath[1024];
extern char cameraid[1024];
extern char firmware[8];
extern int mode, mydisplay, DANGER;
extern int use_lowers;
extern int GMT_offset;
extern int user_init;

#ifdef HAVE_USB_SUPPORT
#include "usb.h"
#endif
#include "serial.h"
#include "common.h"
#include "bar.h"

/* main.c function prototypes */
int command_parser(char *buffer, char *commandargs[], int argmax);
void progressbar(int op, int total, int done);
void signal_trap(int sid);
void show_help(void);
void do_cli_listall(void);
void do_cli_getall(int);
void do_cli_deleteall(void);
void show_usage(void);
void safe_exit(int exitcode);
void setdcimpath(const char *);

/* byte sex conversion */
int byteswap32(int val);

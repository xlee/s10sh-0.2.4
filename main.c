/* This file is part of s10sh
 *
 * Copyright (C) 2000 by Salvatore Sanfilippo <antirez@invece.org>
 * Copyright (C) 2001 by Salvatore Sanfilippo <antirez@invece.org>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * This file contains a function from gphoto's canon driver: dump_hex(),
 * I think it's fine to have the same debugging output.
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif	/* HAVE_READLINE */

#include "s10sh.h"
#include "param.h"
#include "custom.h"

#define NON_SERIAL \
 if (mode == SERIAL_MODE) { \
   printf("Not supported in SERIAL\n"); \
   continue; \
 }

int opt_debug = 0;
int opt_overwrite = 0;
char prompt[1024];
struct canonfile *dirlist[1024];
int dirlist_size = 0;
char lastpath[1024] = {'\0'};
char cameraid[1024];
char dcimpath[1024] = { "D:\\DCIM" };
char firmware[8];
int mode = USB_MODE; /* this is the default mode */
int use_lowers = 0;     /* write out files with upper case chars by default */
int GMT_offset = 0;
int user_init = 0;       /* Use custom initialization routine */
int user_init_time = 0;
int user_init_cap = 0;
int mydisplay = 1;       /* Used by usb.c to control ls screen display */
int DANGER = 0;		 /* Used by usb.c to bypass safe camera detection */

int main(int argc, char **argv)
{
#ifndef HAVE_READLINE
	char buffer[1024];
#endif
	char command[1024];
	char *command_argv[COMMANDARGS_MAX+1] = { NULL };
	int command_argc;
	int c;
	int cli_getallnew = 0, cli_getall = 0, cli_listall = 0;
        int cli_deleteall = 0, cli_test = 0;
	
	signal(SIGTERM, signal_trap);
	signal(SIGINT, signal_trap);
	signal(SIGQUIT, signal_trap);
	signal(SIGSEGV, signal_trap);
	signal(SIGFPE, signal_trap);

	/* Determine the difference in seconds we are from GMT depending upon
	** /etc/localtime or $TZ. This is used to "adjust" the date values we
	** find from the camera for images.
	*/
	GMT_offset = offset_from_GMT();
	
        while ((c = getopt(argc, argv, "d:DulgEhUas:Lni:tcZS")) != EOF) {
		switch(c) {
		case 'D':
			opt_debug = 1;
			printf("DEBUG mode enabled\n");
			break;
		case 'd':
                        serialdev = optarg;
                        break;
                case 'u':
#ifdef HAVE_USB_SUPPORT
			mode = USB_MODE;
			printf("USB mode enabled\n");
#else
			printf("This binary lacks the USB support\n");
			exit(1);
#endif
                        break;
		case 'S':
			mode = SERIAL_MODE;
			printf("SERIAL mode enabled\n");
			break;
		case 'g':
			cli_getall = 1;
			break;
		case 'n':
			cli_getallnew = 1;
			break;
		case 'l':
			cli_listall = 1;
			break;
                case 'L':
                        use_lowers = 1;
			break;
		case 'E':
			cli_deleteall = 1;
			break;
		case 'U':
			cli_test = 1;
			break;
		case 'a':
			opt_a50 = 1;
			printf("A50/Pro70 compatibility mode enabled\n");
			break;
		case 's':
			serial_change_speed(atoi(optarg));
			break;
		case 'i':
			user_init = atoi(optarg);
			break;
		case 't':
			user_init_time = 1;
			break;
		case 'c':
			user_init_cap = 1;
			break;
		case 'Z':
			DANGER = 1;
			break;
                case 'h':
                default:
			show_usage();
			exit(1);
                        break;
	 	}
	}

	printf(
	"S10sh -- version %s\n"
	"Copyright (C) 2000-2001 by Salvatore Sanfilippo <antirez@invece.org>\n"
	"S10sh is FREE SOFTWARE under the terms of the GNU public license\n"
	"\n", VERSION);

	camera_startup_initialization();
	
	/* Mitton */
	/* Change camera directory to highest numbered */
	if ( user_init == 1 ) {
   	   int retval;
	   mydisplay = 0;
	   retval = camera_get_list("dcim");
	   mydisplay = 1;
	   if (retval == -1) {
	      printf("ls error\n");
	   }
           /* */
           int j, largest=0, current;
           char big[1024];
           char aux[1024];
           for (j = 0; j < dirlist_size; j++) {
                   snprintf(aux, 4, "%s", dirlist[j]->name);
                   current=atoi(aux);
                   if (current > largest) {
                      largest=current;
                      snprintf(big, 1024, "%s", dirlist[j]->name);
                   }
                   /*printf(" current=%d, largest=%d, file=%s\n", current, largest, big);*/
           }
           retval = camera_get_list(big);
           if (retval == -1) {
              printf("ls error\n");
           }

	   /* Update camera time if difference greater than x minutes */
	   #include <err.h>
           struct timeval tv[2];
           struct tm *tm1;
           time_t t, t1;
           int mydiff;
	   int mylimit = 500; /* about -8 to +15 min*/
           if (gettimeofday(&tv[0], NULL))
                   err(1, "gettimeofday");

           tm1 = localtime ((const time_t *) &(tv[0].tv_sec));
	   t1 = mktime (tm1);

           t = USB_get_date();
           t += GMT_offset;

	   mydiff = abs(t1-t);
	   /* 
	   printf("user_init_1\n");
           printf("Local  %d, 0x%X, %s", t1, t1, ctime(&t1));
           printf("Camera %d, 0x%X, %s", t, t, ctime(&t));
	   printf("Diff   %d\n", mydiff); */
	   if (mydiff >= mylimit)
		user_init_time = 1;
        }

	/* Set the camera clock if user_init or -t on the command line */
	if ( user_init_time == 1 ) {
		if (mode == SERIAL_MODE) { \
			printf("Not supported in SERIAL\n"); \
		} else {
			char *rc = USB_setdate();
        		if (rc == NULL)
                	     printf("error setting the date\n");
		}
 	}

	/* Capture an image using the current camera settings */
	if ( user_init_cap == 1 ) {
		if (mode == SERIAL_MODE) { \
			printf("Not supported in SERIAL\n"); \
		} else {
                	char *id = USB_control_camera();
                	if (id == NULL)
                        	printf("error getting the ID\n");
                	else {
                        	printf("Capture = %s\n", id);
                	}
                }
	}

	/* CLI ACTIONS */
	if (cli_listall) {
		do_cli_listall();
		safe_exit(0);
	} else if (cli_getall) {
		do_cli_getall(WHICH_ALL);
		safe_exit(0);
	} else if (cli_getallnew) {
		do_cli_getall(WHICH_NEW);
		safe_exit(0);
	} else if (cli_deleteall) {
		do_cli_deleteall();
		safe_exit(0);
	} else if (cli_test) {
#ifdef HAVE_USB_SUPPORT
		opt_debug = 1;
		USB_upload(NULL, NULL);
#endif
		safe_exit(0);
	}

	while(1) {
		char *p, *cmd;

		snprintf(prompt, 1024, "[%s] %s> ", cameraid, lastpath);
#ifdef HAVE_READLINE
                p = readline(prompt);
                if (!p) continue;
                if (p[0] != '\0')
                        add_history(p);
                strncpy(command, p, 1024);
                free(p);
#else
                printf(prompt);
                if (fgets(command, 1024, stdin) == NULL)
                        continue;
                command[1023] = '\0';
                if ((p = strchr(buffer, '\n')) != NULL)
                        *p = '\0';
#endif /* HAVE_READLINE */

		/* free the old memory -- warning, the first time
		 * command_argv[0] must be NULL. */
		command_parser(NULL, command_argv, 0);

		/* split in words */
		command_argc = command_parser(command, command_argv,
                        COMMANDARGS_MAX);
		if (command_argc == 0)
			continue;
                cmd = command_argv[0]; /* cmd is argv[0] */

#define CHECK_ARGS(x) 	if (command_argc != x) \
			{ \
				printf("not enough arguments\n"); \
				continue; \
			}

		if (!strcmp(cmd, "close")) {
			camera_close();
		} else if (!strcmp(cmd, "quit") || !strcmp(cmd, "exit") || !strcmp(cmd, "bye")) {
			printf("bye!\n");
			safe_exit(0);
		} else if (!strcmp(cmd, "debug")) {
			opt_debug = !opt_debug;
			if (opt_debug) {
				printf("debug is on\n");
			} else {
				printf("debug is off\n");
			}
		} else if (!strcmp(cmd, "id")) {
			char *id = NULL;
			int *bid, inter;
			id = camera_get_id();
			if (id == NULL) {
				printf("error getting the ID\n");
			} else {
				printf("camera ID: %s\n", id);
				printf("firmware : %s\n", firmware);
				printf("Owner    : %s\n", camera_owner);
			}
			if (mode == USB_MODE) {
			   bid = USB_body_id();
			   if (bid == NULL) {
				printf("error getting the Bid\n");
			   } else {
				(int *)inter = bid;
				printf("Body id  : %d\n", inter);
			   }
			}
                } else if (!strcmp(cmd, "setowner")) {
			char *id = NULL;
			int *bid, inter;
			static char buffer[35];
			static char buffer1[35];
			/* */
			   int x=1;
			   strcpy(buffer, "");
			   for (x = 1; x < command_argc; x++) {
				if (x < command_argc-1)
					sprintf(buffer1, "%s ", command_argv[x]);
				else
					sprintf(buffer1, "%s", command_argv[x]);
				strcat(buffer, buffer1);
			   } 
			   if (strlen(buffer) >= 32) {
				printf("Error string too long %d.\n", strlen(buffer));
				continue;
			   }
			   /*printf("count = _%d_, length = _%d_, id = _%s_\n", command_argc-1, strlen(buffer), buffer);*/
			   id = camera_set_owner(buffer);
			   if (id == NULL) {
				printf("error setting the ID\n");
				continue;
			   }
			/* */
			id = NULL;
                        id = camera_get_id();
                        if (id == NULL) {
                                printf("error getting the ID\n");
                        } else {
                                printf("camera ID: %s\n", id);
                                printf("firmware : %s\n", firmware);
                                printf("Owner    : %s\n", id+32);
	                        if (mode == USB_MODE) {
        	                   bid = USB_body_id();
                	           if (bid == NULL) {
                        	        /*printf("error getting the Bid\n");*/
	                           } else {
        	                        (int *)inter = bid;
                	                printf("Body id  : %d\n", inter);
                        	   }
                       		 }
                        }
		} else if (!strcmp(cmd, "capture")) {
			NON_SERIAL;
			char *id = USB_control_camera();
			if (id == NULL) {
				/*printf("error getting the ID\n");*/
			} else {
				printf("Capture = %s\n", id);
			}
		} else if (!strcmp(cmd, "focus")) {
			NON_SERIAL;
			int id = USB_focus(1);
			if (id == 0) {
				/*printf("error getting the ID\n");*/
			} else {
			}
		} else if (!strcmp(cmd, "focus-stop")) {
			NON_SERIAL;
			int id = USB_focus(0);
			if (id == 0) {
				/*printf("error getting the ID\n");*/
			} else {
			}
		} else if (!strcmp(cmd, "shots")) {
			NON_SERIAL;
			int shots = USB_shots();
			if (shots < 0) {
				/*printf("error getting the ID\n");*/
			} else {
			   printf ("Shots left: %d\n", shots);
			}
		} else if (!strcmp(cmd, "getpars")) {
			char *id = USB_camera_getpars(1);
			if (id == NULL) {
				/*printf("error getting the ID\n");*/
			} else {
				//printf("Parameters = %s\n", id);
			}
		} else if (!strcmp(cmd, "getpar")) {
			NON_SERIAL;
			char *name = command_argv[1];
			char *val = USB_camera_getpar(name);
			if (!val) {
				/*fprintf(stderr, "cannot find parameter %s\n", name);*/
			} else {
				printf("Parameter %s = %s\n", name, val);
			}
		} else if (!strcmp(cmd, "setpar")) {
			NON_SERIAL;
			char *name = command_argv[1];
			char *value = command_argv[2];
			int val = USB_camera_setpar(name, value);
			if (!val) {
				/*fprintf(stderr, "cannot find parameter %s\n", name);*/
			} else {
				//printf("Parameter %s = 0x%x\n", name, val);
			}
		} else if (!strcmp(cmd, "getcustom")) {
			char *name = command_argv[1];
			char *id = USB_camera_getcustom(name);
			if (id == NULL) {
				/*printf("error getting the ID\n");*/
			} else {
				//printf("Custom = %s\n", id);
			}
		} else if (!strcmp(cmd, "setcustom")) {
			char *name  = command_argv[1];
			char *value = command_argv[2];
			int id = USB_camera_setcustom(name, value);
			if (id == 0) {
				/*printf("error getting the ID\n");*/
			} else {
				//printf("Custom = %d\n", id);
			}
		} else if (!strcmp(cmd, "disk")) {
			char *disk = NULL;
			disk = camera_get_disk();
			if (disk == NULL) {
				printf("error getting the disk\n");
			} else {
				printf("camera disk: %s\n", disk);
			}
		} else if (!strcmp(cmd, "ping")) {
			camera_ping();
		} else if (!strcmp(cmd, "diskinfo")) {
			int size, free, result = 0;
			CHECK_ARGS(2)
			result = camera_get_disk_info(command_argv[1],
				&size, &free);
			if (result == 0) {
				printf("Disk size: %d bytes (%d K)\n",
					size, size/1024);
				printf("available: %d bytes (%d K)\n",
					free, free/1024);
			} else {
				printf("Disk info unavailable for %c:\n",
					*command_argv[1]);
			}
		} else if (!strcmp(cmd, "ls") || !strcmp(cmd, "dir") || !strcmp(cmd, "cd")) {
			int retval;
			retval = camera_get_list(command_argv[1]);
			if (retval == -1) {
				printf("ls error\n");
			}
		} else if (!strcmp(cmd, "get")) {
			CHECK_ARGS(2);
			if (camera_get_image(command_argv[1], NULL) == -1) {
				printf("get error\n");
			} else {
				printf("get successful\n");
			}
		} else if (!strcmp(cmd, "tget")) {
			CHECK_ARGS(2);
			if (camera_get_thumb(command_argv[1], NULL) == -1) {
				printf("tget error\n");
			} else {
				printf("tget successful\n");
			}
		} else if (!strcmp(cmd, "view")) {
			CHECK_ARGS(2);
			view_thumb(command_argv[1]);
		} else if (!strcmp(cmd, "viewall")) {
			view_all();
		} else if (!strcmp(cmd, "lastls")) {
			camera_last_ls();
		} else if (!strcmp(cmd, "getlastls")|| !strcmp(cmd, "getall")) {
			camera_get_last_ls(WHICH_ALL);
		} else if (!strcmp(cmd, "getallold")) {
			camera_get_last_ls(WHICH_OLD);
		} else if (!strcmp(cmd, "getallnew")) {
			camera_get_last_ls(WHICH_NEW);
		} else if (!strcmp(cmd, "open")) {
			if (mode == SERIAL_MODE)
				serial_open();
			else
				printf("Not implemented with USB\n");
		} else if (!strcmp(cmd, "reopen")) {
			if (mode == SERIAL_MODE) {
				serial_close();
				serial_open();
			} else {
				printf("Not implemented with USB\n");
			}
		} else if (!strcmp(cmd, "getpkt")) {
			if (mode == SERIAL_MODE)
				serial_debug_getpkt();
			else
				printf("Not implemented with USB\n");
		} else if (!strcmp(cmd, "clear") | !strcmp(cmd, "cls")) {
			printf("\033[H\033[2J");
		} else if (!strcmp(cmd, "power")) {
			int good, ac;
			camera_get_power_status(&good, &ac);
			if (ac) {
				printf("The camera is using the AC adapter\n");
			} else {
				printf("The camera is using the battery\n");
				if (good)
					printf("battery level is HIGH\n");
				else
					printf("battery level is LOW\n");
			}
		} else if (!strcmp(cmd, "test")) {
			CHECK_ARGS(2);
			if (mode == SERIAL_MODE)
				serial_test_message(atoi(command_argv[1]));
			else
				printf("Not implemented with USB\n");
                } else if (!strcmp(cmd, "setdate")) {
			NON_SERIAL;
			char *rc = USB_setdate();
                	if (rc == NULL) {
                             printf("error setting the date\n");
                             continue;
                	}
		} else if (!strcmp(cmd, "date")) {
			time_t t;
			if (mode == SERIAL_MODE)
				t = serial_get_date();
#ifdef HAVE_USB_SUPPORT
			else
				t = USB_get_date();
#endif
			/* printf("Camera date: %s", ctime(&t)); */
                        t += GMT_offset;

			/* 050407 - Show whether daylight or standard time */
			struct timeval tv[2];
			struct tm *tm1;
			gettimeofday(&tv[0], NULL);
			tm1 = localtime ((const time_t *) &(tv[0].tv_sec));
			if (tm1->tm_isdst == 1)
				printf("Camera date (xDT): %s", ctime(&t));
			else
				printf("Camera date (xST): %s", ctime(&t));
		} else if (!strcmp(cmd, "speed")) {
			int old_speed = serial_speed;

			if (mode == SERIAL_MODE) {
				if (command_argc == 2) {
					serial_change_speed(atoi(command_argv[1]));
					if (old_speed != serial_speed) {
						serial_close();
						serial_open();
					}
				} else {
					serial_change_speed(0);
				}
			} else {
				printf("Not implemented with USB\n");
			}
		} else if (!strcmp(cmd, "help")) {
			if (command_argc != 2) show_help();
			else if (!strcmp(command_argv[1], "param")) param_help(0,23);
			else if (!strcmp(command_argv[1], "custom")) custom_help(0,23);
		} else if (!strcmp(cmd, "overwrite")) {
			opt_overwrite = !opt_overwrite;
			if (opt_overwrite)
				printf("overwrite mode ON\n");
			else
				printf("overwrite mode OFF\n");
		} else if (!strcmp(cmd, "mkdir")) {
			CHECK_ARGS(2);
			if (mode == SERIAL_MODE) {
				if (serial_mkdir(command_argv[1]) == 0)
					printf("mkdir successful\n");
				else
					printf("mkdir error\n");
			}
#ifdef HAVE_USB_SUPPORT
			else
			{
				if (USB_mkdir(command_argv[1]) == 0)
					printf("mkdir successful\n");
				else
					printf("mkdir error\n");
			}
#endif
		} else if (!strcmp(cmd, "rmdir")) {
			CHECK_ARGS(2);
			if (mode == SERIAL_MODE) {
				if (serial_rmdir(command_argv[1]) == 0)
					printf("rmdir successful\n");
				else
					printf("rmdir error\n");
			}
#ifdef HAVE_USB_SUPPORT
			else
			{
				if (USB_rmdir(command_argv[1]) == 0)
					printf("rmdir successful\n");
				else
					printf("rmdir error\n");
			}
#endif
		} else if (!strcmp(cmd, "rm") || !strcmp(cmd, "delete")) {
			CHECK_ARGS(2);
			if (mode == SERIAL_MODE) {
				if (serial_delete(command_argv[1]) == 0)
					printf("delete successful\n");
				else
					printf("delete error\n");
			}
#ifdef HAVE_USB_SUPPORT
			else
			{
				if (USB_delete(command_argv[1]) == 0)
					printf("delete successful\n");
				else
					printf("delete error\n");
			}
#endif
		} else if (!strcmp(cmd, "deleteall")) {
			camera_delete_all(WHICH_ALL);
		} else if (!strcmp(cmd, "deleteold")) {
			camera_delete_all(WHICH_OLD);
		} else if (!strcmp(cmd, "deletenew")) {
			camera_delete_all(WHICH_NEW);
		} else if (!strcmp(cmd, "protect")) {
			CHECK_ARGS(2);
			camera_file_chmod(command_argv[1],
				CHMOD_SET, ATTR_PROTECTED);
		} else if (!strcmp(cmd, "unprotect")) {
			CHECK_ARGS(2);
			camera_file_chmod(command_argv[1],
				CHMOD_CLEAR, ATTR_PROTECTED);
		} else if (!strcmp(cmd, "new")) {
			CHECK_ARGS(2);
			camera_file_chmod(command_argv[1],
				CHMOD_SET, ATTR_NEW);
		} else if (!strcmp(cmd, "old")) {
			CHECK_ARGS(2);
			camera_file_chmod(command_argv[1],
				CHMOD_CLEAR, ATTR_NEW);
		} else if (!strcmp(cmd, "protectall")) {
			camera_file_chmod_all(CHMOD_SET, ATTR_PROTECTED);
		} else if (!strcmp(cmd, "unprotectall")) {
			camera_file_chmod_all(CHMOD_CLEAR, ATTR_PROTECTED);
		} else if (!strcmp(cmd, "newall")) {
			camera_file_chmod_all(CHMOD_SET, ATTR_NEW);
		} else if (!strcmp(cmd, "oldall")) {
			camera_file_chmod_all(CHMOD_CLEAR, ATTR_NEW);
		} else if (!strcmp(cmd, "upload") || !strcmp(cmd, "put")) {
			int retval;
			if (command_argc == 2) {
#ifdef HAVE_USB_SUPPORT
				if (mode == USB_MODE)
					retval = USB_upload(command_argv[1], NULL);
				else
#endif
					retval = serial_upload(command_argv[1], NULL);
			} else if (command_argc == 3) {
#ifdef HAVE_USB_SUPPORT
				if (mode == USB_MODE)
					retval = USB_upload(command_argv[1], command_argv[2]);
				else
#endif
					retval = serial_upload(command_argv[1], command_argv[2]);
			} else {
				printf("usage: put <source> [target]\n");
				continue;
			}
			if (retval != -1)
				printf("upload successful\n");
			else
				printf("upload error\n");
		}

		else
		{
			printf("unknown command '%s', try 'help'\n", cmd);
		}
	}
	return 0; /* against warnings */
}

int command_parser(char *buffer, char *commandargs[], int argmax)
{
#define skip_char(x) while(*p == x) p++

        char *p = buffer, *d;
        char tmp[1024];
        int size;
        int argindex = 0;

        /* if buffer is a NULL pointer free commandargs memory */
        if (buffer == NULL) {
                for (; commandargs[argindex] != NULL; argindex++)
                        free(commandargs[argindex]);
                return argindex;
        }

        /* otherwise parse the command line */
        while(*p != '\0') {
                size = 0;
                d = tmp;
                skip_char(' ');

                while(*p != ' ') {
                        if (*p == '\0' || *p == '\n' || size >= 1023)
                                break;
                        *d++ = *p++;
                        size++;
                }

                if (size != 0) {
                        commandargs[argindex] = malloc(size+1);
                        if (commandargs[argindex] == NULL) {
                                perror("malloc");
                                exit(1);
                        }
                        strncpy(commandargs[argindex], tmp, size);
                        commandargs[argindex][size] = '\0';
                } else {
                        break;
                }

                argindex++;
                if (argindex >= argmax)
                        break;
        }
        commandargs[argindex] = NULL;
        return argindex;
}

void safe_exit(int exitcode)
{
	struct stat buf;
	if (mode == SERIAL_MODE)
		serial_send_switch_off();
#ifdef HAVE_USB_SUPPORT
	else
		USB_close();
#endif
	if (lstat(TEMP_FILE_NAME, &buf) != -1) {
		if (!S_ISLNK(buf.st_mode))
			unlink(TEMP_FILE_NAME);
		else
			printf("WARNING, %s is a symbolic link\n",
				TEMP_FILE_NAME);
	}
	exit(exitcode);
}

void signal_trap(int sid)
{
	printf("\n--> signal %d trapped, close the camera and exit\n", sid);
	safe_exit(sid);
}

void show_help(void)
{
	int j;
	char *helptext[] = {
"\ns10sh help\n",
"help                     show this help screen",
"help param               show help on parameters",
"help custom              show help on custom values",
"open                     open the camera",
"reopen                   close and open the camera",
"close                    close the connection with the camera",
"speed         [speed]    change the serial speed",
"quit                     close the camera and quit the program",
"ping                     ping four times the camera",
"clear                    clear the screen under some terminal types",
"id                       show the camera id",
"date                     show the internal date of the camera",
"setdate                  set the camera date to that of the computer",
"disk                     show the CF disk letter",
"diskinfo      <disk>     show disk information",
"ls | cd | dir <dir>      change to and list the specified directory",
"lastls                   show the last cached directory listing",
"get           <pathname> get the specified image",
"getall                   get all the files in the current directory",
"getallold                get all the old files in the current directory",
"getallnew                get all the new files in the current directory",
"tget          <pathname> get the specified image as thumbnail",
"view          <pathname> view the thumbnail using xv",
"viewall                  view all thumbnails in the current directory",
"power                    show informations about power status",
"overwrite                switch on/off the overwrite mode, when overwrite",
"                         mode is ON the old files will be overwritten with",
"                         the new files. Default ovewrite mode is OFF",
"mkdir         <dirname>  create a directory",
"rmdir         <dirname>  remove a directory",
"debug         (DEBUG)    turn the debug on/off",
"getpkt        (DEBUG)    wait for a packet from the camera",
"test <num>    (DEBUG)    send the specified request and wait for data",
"rm | delete   <filename> remove a file in the current path",
"deleteall                remove all files in the current directory",
"deleteold                remove downloaded files in the current directory",
"deletenew                remove new files in the current directory",
"protect       <filename> set the protected flag (in the current path)",
"unprotect     <filename> clear the protected flag (in the current path)",
"new           <filename> clear the downloaded flag (in the current path)",
"newall                   exec new against all files in the current directory",
"old           <filename> set the downloaded flag (in the current path)",
"oldall                   exec old against all files in the current directory",
"protectall               protect all files in the current directory",
"unprotectall             unprotect all files in the current directory",
"upload        <source> [dest]  upload a file (USB only)",
"setowner      string     change the owner string - WARNING NULL will clear!",
"capture                  take a picture using the current camera settings",
"getpars                  get the shooting parameters",
"getpar        <name>     get the shooting parameter value",
"setpar        <name> <value>   set the shooting parameter value",
"getcustom                get the custom function value",
"setcustom     <name> <value>  set the custom function value",
NULL };

	for (j = 0; helptext[j]; j++) {
		puts(helptext[j]);
		if (!(j % 23) && j != 0) {
			printf("--- MORE ---\n");
			getchar();
		}
	}
	/* j = param_help (j, 23);
	  j = param_help (j, 23);
	*/
}

/* endianess convertion */
#include "bytesex.h"

#if !defined BYTE_ORDER_LITTLE_ENDIAN && !defined BYTE_ORDER_BIG_ENDIAN
#error Please, define byte sex
#endif

int byteswap32(int val)
{
	typedef unsigned int u32; /* unsigned long is 64bit under ILP64 */
#ifdef BYTE_ORDER_BIG_ENDIAN
	u32 x = val;
	x = (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24);
	return x;
#endif
	return val;
}

void do_cli_getall(int which)
{
	int j, c;
	char *directory[1024];

	if (camera_get_list(dcimpath) == -1) {
		printf("Error listing %s\n", dcimpath);
		exit(1);
	}

	if (dirlist_size == 0) {
		printf("CF seems empty\n");
		exit(0);
	}

	c = 0;
	for (j = 0; j < dirlist_size; j++) {
		if (!(dirlist[j]->type & ATTR_ITEMS))
                  continue;
		directory[c] = malloc(strlen(dirlist[j]->name)+1);
		if (!directory[c]) {
			perror("malloc");
			exit(1);
		}
		memcpy(directory[c], dirlist[j]->name, strlen(dirlist[j]->name)+1);
		c++;
	}
	directory[c] = NULL;

	c = 0;
	while(directory[c]) {
		printf("---> %s\n", directory[c]);
		if (camera_get_list(directory[c]) == -1) {
			printf("Error listing %s\n", directory[c]);
			exit(1);
		}
		if (dirlist_size == 0) {
                    printf("skipping empty directory\n");
		} else if (camera_get_last_ls(which) == -1) {
			printf("camera_get_last_ls error\n");
			exit(1);
		}
		if (camera_get_list("..") == -1) {
			printf("Error performing cd ..\n");
			exit(1);
		}
		c++;
	}
}

void do_cli_listall(void)
{
	int j, c;
	char *directory[1024];

	if (camera_get_list(dcimpath) == -1) {
		printf("Error listing %s\n", dcimpath);
		exit(1);
	}

	if (dirlist_size == 0) {
		printf("CF seems empty\n");
		exit(0);
	}

	c = 0;
	for (j = 0; j < dirlist_size; j++) {
		if (!(dirlist[j]->type & ATTR_ITEMS))
				continue;
		directory[c] = malloc(strlen(dirlist[j]->name)+1);
		if (!directory[c]) {
			perror("malloc");
			exit(1);
		}
		memcpy(directory[c], dirlist[j]->name, strlen(dirlist[j]->name)+1);
		c++;
	}
	directory[c] = NULL;

	c = 0;
	while(directory[c]) {
		printf("---> %s\n", directory[c]);
		if (camera_get_list(directory[c]) == -1) {
			printf("Error listing %s\n", directory[c]);
			exit(1);
		}
		if (camera_get_list("..") == -1) {
			printf("Error performing cd ..\n");
			exit(1);
		}
		c++;
	}
}

void do_cli_deleteall(void)
{
	int j, c;
	char *directory[1024];

	printf("Are you sure? (y/N): ");
	fflush(stdout);
	if (getchar() != 'y')
		exit(0);

	if (camera_get_list(dcimpath) == -1) {
		printf("Error listing %s\n", dcimpath);
		exit(1);
	}

	if (dirlist_size == 0) {
		printf("CF seems empty\n");
		exit(0);
	}

	c = 0;
	for (j = 0; j < dirlist_size; j++) {
		if (!(dirlist[j]->type & ATTR_ITEMS))
				continue;
		directory[c] = malloc(strlen(dirlist[j]->name)+1);
		if (!directory[c]) {
			perror("malloc");
			exit(1);
		}
		memcpy(directory[c], dirlist[j]->name, strlen(dirlist[j]->name)+1);
		c++;
	}
	directory[c] = NULL;

	c = 0;
	while(directory[c]) {
		printf("---> %s\n", directory[c]);
		if (camera_get_list(directory[c]) == -1) {
			printf("Error listing %s\n", directory[c]);
			exit(1);
		}
		if (camera_delete_all(WHICH_ALL) == -1) {
			printf("camera_delete_all error\n");
			exit(1);
		}
		if (camera_get_list("..") == -1) {
			printf("Error performing cd ..\n");
			exit(1);
		}
#ifdef HAVE_USB_SUPPORT
		if (mode == USB_MODE)
			USB_rmdir(directory[c]);
		else
#endif
			serial_rmdir(directory[c]);
		c++;
	}
	if (camera_get_list("..") == -1) {
		printf("Error performing cd ..\n");
		exit(1);
	}
#ifdef HAVE_USB_SUPPORT
	if (mode == USB_MODE)
		USB_rmdir("DCIM");
	else
#endif
		serial_rmdir("DCIM");
}

void show_usage(void)
{
  printf(
         "s10sh -- Canon Digital Camera Software\n"
         "Version %s\n\n"
         "usage: s10sh -[DaugnlELhctZS] [-d <serialdevice> -i <value> -s <speed>]\n\n"
         "  -D                    enable debug mode\n"
#if __FreeBSD__
         "  -d <serialdevice>     set the serial device, default /dev/cuaa0\n"
#else
         "  -d <serialdevice>     set the serial device, default /dev/ttyS0\n"
#endif
         "  -a                    enable A50/Pro70 compatibility mode\n"
         "  -s <serialspeed>      set the serial speed (9600 19200 38400 57600 115200)\n"
         "  -u                    USB mode, now default; use -S for SERIAL\n"
	 "  -S                    SERIAL mode, default is now USB mode\n"
         "  -g                    non-interactive mode, get all images\n"
         "  -n                    non-interactive mode, get all new images\n"
         "  -l                    non-interactive mode, list all images\n"
         "  -E                    non-interactive mode, delete all images\n"
         "  -L                    write files using all lower-case characters\n"
	 "  -i <value>            set the user-init value\n"
	 "  -t                    set the camera to the current computer time\n"
         "  -c                    capture an image with the current camera settings\n"
	 "  -Z                    DANGER, this option bypasses SAFE camera detection routines\n" 
         "  -h                    show this help screen\n", VERSION
         );
}

void setdcimpath(const char *path)
{
  snprintf(dcimpath, sizeof(dcimpath), "%s", path);
  if (opt_debug) {
    printf ("dcimpath set to '%s'\n", dcimpath);
  }
}

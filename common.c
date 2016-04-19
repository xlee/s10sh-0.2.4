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
 * USB/SERIAL driver common functions and wrappers
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
#include <ctype.h>
#include <err.h>
#include "s10sh.h"

char        camera_name[0x21] = "Unknown camera model";
camera_type camera_model = UNKOWN_CAMERA;
char	    camera_owner[0x21] = "Unknown owner";

/* ripped from gphoto */
#define NIBBLE(_i)    (((_i) < 10) ? '0' + (_i) : 'A' + (_i) - 10)
void dump_hex(const char *msg, const unsigned char *buf, int len)
{
    int i;
    int nlocal;
    const unsigned char *pc;
    char *out;
    const unsigned char *start;
    char c;
    char line[100];

    if (!opt_debug)
	return;

    start = buf;

#if 0
    if (len > 160)
    {
        fprintf(stderr,"dump n:%d --> 160\n", len);
        len = 160;
    }
#endif

    fprintf(stderr,"%s: (%d bytes)\n", msg, len);
    while (len > 0)
    {
        sprintf(line, "%08x: ", buf - start);
        out = line + 10;

        for (i = 0, pc = buf, nlocal = len; i < 16; i++, pc++)
        {
            if (nlocal > 0)
            {
                c = *pc;

                *out++ = NIBBLE((c >> 4) & 0xF);
                *out++ = NIBBLE(c & 0xF);

                nlocal--;
            }
            else
            {
                *out++ = ' ';
                *out++ = ' ';
            }                   /* end else */

            *out++ = ' ';
        }                       /* end for */

        *out++ = '-';
        *out++ = ' ';

        for (i = 0, pc = buf, nlocal = len;
             (i < 16) && (nlocal > 0);
             i++, pc++, nlocal--)
        {
            c = *pc;

            if ((c < ' ') || (c >= 126))
            {
                c = '.';
            }

            *out++ = c;
        }                       /* end for */

        *out++ = 0;

        fprintf(stderr,"%s\n", line);

        buf += 16;
        len -= 16;
    }                           /* end while */
    printf("\n");
}                               /* end dump */

unsigned long get_usec(void)
{
        struct timeval tmptv;

        gettimeofday(&tmptv, NULL);
        return tmptv.tv_usec;
}

int camera_last_ls(void)
{
	int j;

	if (dirlist_size == 0) {
		printf("last ls is empty\n");
		return -1;
	}

	for (j = 0; j < dirlist_size; j++) {
		dump_filename(dirlist[j]);
	}

        if (opt_debug) {
          printf("last ls pathname was: %s\n", lastpath);
          printf("lastls successful\n");
        }
	return 0;
}

int camera_get_last_ls(int which)
{
	int j;

	if (dirlist_size == 0) {
		printf("last ls is empty\n");
		return -1;
	}

	for (j = 0; j < dirlist_size; j++) {
		char aux[1024];

		if (which == WHICH_NEW) {
			if (!(dirlist[j]->type & ATTR_NEW))
				continue;
		}
		else if (which == WHICH_OLD) {
			if (dirlist[j]->type & ATTR_NEW)
				continue;
		}

		snprintf(aux, 1024, "%s\\%s", lastpath, dirlist[j]->name);
		camera_get_image(aux, NULL);
		printf("\n");
	}
        if (opt_debug) {
          printf("getlastls successful\n");
        }
	return 0;
}

int camera_get_file_attr(char *name)
{
	int j;

	for (j = 0; j < dirlist_size; j++) {
		if (!strcasecmp(name, dirlist[j]->name))
			return dirlist[j]->type;
	}
	return -1; /* not found */
}

/* Determine the different (pos or negative) in seconds we are from GMT in this
** current timezone.
** 050407 Add in Standard/Daylight factor.
*/
int offset_from_GMT (void) {
  struct timeval tv[2];
  struct tm *tm1, *tm2;
  struct tm tm1_copy;
  time_t t1, t2;
  double diff = 0.0;
  int dstoffset;

  if (gettimeofday(&tv[0], NULL))
    err(1, "gettimeofday");

  tm1 = localtime ((const time_t *) &(tv[0].tv_sec));
  tm1_copy = *tm1; /* struct copy */

  tm2 = gmtime ((const time_t *) &(tv[0].tv_sec));

  t1 = mktime (&tm1_copy);
  t2 = mktime (tm2);

  /* No offset if standard/winter time. */
  dstoffset = 0;
  /* Adjust by 1 hour if it is Daylight/Summer time. */
  if (tm1->tm_isdst == 1)
     dstoffset = 3600;

  diff = difftime (t2, t1);
  diff -= dstoffset;

  return ((int)diff);
}

int camera_get_list(char *pathname)
{
	unsigned char *message = NULL;
	int message_size = 0;
	int message_offset = 0;
	unsigned char aux[1024];
	char arg[1024];
	unsigned char *pkt;
	int j, totbytes = 0, first_packet = 1;
	struct header hdr;
	unsigned char *p;

	if (pathname == NULL)
		pathname = lastpath;
	else if (!strcmp(pathname, "..")) {
		strncpy(arg, lastpath, 1024);
		p = strrchr(arg, '\\');
		if (!p)
			return -1;
		*p = '\0';
		pathname = arg;
	} else if (pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

        /* Skip "*.ctg" files */
        if ((pkt = strstr (pathname, ".CTG"))) {
          return 0;
        }
	switch(mode) {
	case SERIAL_MODE:
		aux[0] = DL_NO_RECURSION;
		memcpy(aux+1, pathname, strlen(pathname));
		memset(aux+1+strlen(pathname), 0, 3);

		serial_send_message_frag(MSG_TYPE_LIST_WITH_DATE, aux,
			4+strlen(pathname), 0);
		serial_send_eot();
		serial_get_ack();

		while(1) {
			unsigned char *newmem;

			pkt = serial_get_packet(&hdr); /* data */
			if (hdr.type == PKT_TYPE_EOT)
				break;

			p = hdr.data;
			if (first_packet && *(p+21) != 0x80) {
				serial_get_packet(&hdr); /* eot */
				serial_send_ack(ACK_ERROR_NONE);
				if (message) free(message);
				return -1;
			}

			message_size += hdr.len;
			newmem = realloc(message, message_size);
			if (!newmem) {
				perror("realloc");
				free(message);
				return -1;
			}
			message = newmem;
			memcpy(message+message_offset, hdr.data, hdr.len);
			message_offset += hdr.len;
			first_packet = 0;
		}
		serial_send_ack(ACK_ERROR_NONE);
		break;

#ifdef HAVE_USB_SUPPORT
	case USB_MODE:
		aux[0] = DL_NO_RECURSION;
		memcpy(aux+1, pathname, strlen(pathname));
		memset(aux+1+strlen(pathname), 0, 3);
		USB_cmd(0x0b, 0x11, 0x202, 0x01, aux, strlen(pathname)+4);
		USB_read(aux, 0x40);
		j = byteswap32(*(unsigned int*)(aux+6));
		if (j == 0)
			return -1;
		message = malloc(j);
		if (!message) {
			perror("malloc");
			exit(1);
		}
		USB_read(message, j);
		if (message[0] != 0x80) {
			free(message);
			return -1;
		}
		break;
#endif
	}

	p = message;
	if (mode == SERIAL_MODE)
		p += 31;
	else
		p += 10;

	/* skip the directory name */
	strncpy(lastpath, p, 1024);
	p += strlen(p) + 1;

	/* free the old directory list cache */
	for (j = 0; j < dirlist_size; j++)
		free(dirlist[j]);
	dirlist_size = 0;

	if (mydisplay == 1 )
		printf("\n");
	while(1) {
		if (!*(p+10)) break;
		dirlist[dirlist_size] = malloc(sizeof(struct canonfile));
		dirlist[dirlist_size]->type = *p;
		p += 2;
		memcpy(&dirlist[dirlist_size]->size, p, 4);
		dirlist[dirlist_size]->size = byteswap32(dirlist[dirlist_size]->size);
		/* sigbus on solaris + sun cc */
		/* dirlist[dirlist_size]->size = byteswap32(*(unsigned int*)p); */
		totbytes += dirlist[dirlist_size]->size;
		p += 4;
		memcpy(&dirlist[dirlist_size]->date, p, 4);
		dirlist[dirlist_size]->date = byteswap32(dirlist[dirlist_size]->date);

		/* "adjust" the date field so that things are printed according
		 * to one's timezone info. The 4-byte date field retreived from
		 * the camera is the time in seconds from the Epoch w.r.t. GMT!
		 * If we don't "adjust" this value accordingly, ctime(3) will
		 * make its own adjustments for the time zone and the wrong time/
		 * date is printed out (off by N hours).
		 */
		dirlist[dirlist_size]->date += GMT_offset;
		
		/* sigbus on solaris + sun cc */
		/* dirlist[dirlist_size]->date = byteswap32(*(unsigned int*)p); */
		p += 4;
		strncpy(dirlist[dirlist_size]->name, p, 1024);
		p += strlen(p);

		dump_filename(dirlist[dirlist_size]);

		p++;
		dirlist_size++;
	}
	if ( mydisplay == 1 )
		printf("        %d files      %d bytes\n\n", dirlist_size, totbytes);
	free(message);
	return 0;
}

/*
 * This routine is responsible for looking through a cached directory entry for
 * a particular filename. if a match is found, it returns the date from the canonfile
 * structure. Else, it'll return 0.
 *
 */
time_t get_date_for_image (char *pathname)
{
  time_t retval = 0;
  char *file;
  int j;
  
  file = strrchr(pathname, '\\');
  if (file == NULL) {
    file = pathname;
  }
  else {
    file++;
  }

  for (j = 0; j < dirlist_size; j++) {
    if (!strcmp (dirlist[j]->name, file)) {
      retval = dirlist[j]->date;
      break;
    }
  }
  
  return (retval);
  
}

int camera_get_image(char *pathname, char *destfile)
{
	time_t timestamp;
	int fd, len;
	unsigned char *image = NULL;
	char arg[1024];
	char lowerdestfile[1024];
	char orig_pathname[1024];
	char *ptr, *outfile;
	time_t imagedate;
	struct timeval tval[2];
	
	strncpy (orig_pathname, pathname, 1024);
	
	if (strlen(pathname) <= 2 || pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	if (destfile == NULL) {
		destfile = strrchr(pathname, '\\');
		if (!destfile) return -1;
		destfile++;
		/* Create a copy of the destfile name that is all lowercase */
		strncpy (lowerdestfile, destfile, strlen (destfile)+1);
		ptr = lowerdestfile;
		while (*ptr) {
		  *ptr = tolower(*ptr);
		  ptr++;
		}
	}

	timestamp = time(NULL);
	if (mode == SERIAL_MODE)
		image = serial_get_data(pathname, 0x00, &len);
#ifdef HAVE_USB_SUPPORT
	else
		image = USB_get_data(pathname, 0x00, &len);
#endif

	if (!image) {
		return -1;
	} else {
		timestamp = time(NULL) - timestamp;
		if (!timestamp)
			timestamp = 1;
		printf("\nDownloaded in %ld seconds, %ld bytes/s\n",
                       (long)timestamp, (long) len/timestamp);

		imagedate = get_date_for_image (orig_pathname);
		
		/* Decide which filename to use
		 */
		if (use_lowers) {
		  outfile = lowerdestfile;
		}
		else {
		  outfile = destfile;
		}
		
		if (opt_overwrite) {
		  fd = open(outfile, O_RDWR|O_CREAT|O_TRUNC, 0644);
		}
		else {
		  fd = open(outfile, O_RDWR|O_CREAT|O_EXCL, 0644);
		}

		if (fd == -1) {
			perror("===WARNING===> open");
			free(image);
			return -1;
		}

		write(fd, image, len);
		close(fd);
		printf("\n");

		/* If a non-zero result came back from get_date_for_image(),
		 * then set the atime and mtime values for the file
		 * using utimes(3).
		 */
		if (imagedate) {
		  tval[0].tv_sec = tval[1].tv_sec = imagedate;
		  tval[0].tv_usec = tval[1].tv_usec = 0;
		  utimes (outfile, tval);
		}

		free(image);
	}
	camera_file_chmod(pathname, CHMOD_CLEAR, ATTR_NEW);
	return 0;
}

int camera_get_thumb(char *pathname, char *destfile)
{
	time_t timestamp;
	int fd, len;
	unsigned char *image = NULL;
	char arg[1024];

	if (strlen(pathname) <= 2 || pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	if (destfile == NULL) {
		destfile = strrchr(pathname, '\\');
		if (!destfile) return -1;
		destfile++;
	}

	timestamp = time(NULL);
	if (mode == SERIAL_MODE)
		image = serial_get_data(pathname, 0x01, &len);
#ifdef HAVE_USB_SUPPORT
	else
		image = USB_get_data(pathname, 0x01, &len);
#endif

	if (!image) {
		return -1;
	} else {
		unsigned char *start;
		int tlen = 0;
		unsigned char *t = image;

		/* skip the first FFD8 */
		t += 2;

		/* search the FFD8 */
		while(t[0] != 0xFF || t[1] != 0xD8)
			t++;
		start = t;

		/* search the FFD9 */
		while(t[0] != 0xFF || t[1] != 0xD9) {
			t++;
			tlen++;
		}
		tlen += 2;

		fd = open(destfile, O_RDWR|O_CREAT|O_TRUNC, 0644);
		if (fd == -1) {
			perror("open");
			free(image);
			return -1;
		}
		write(fd, start, tlen);
		close(fd);
		printf("\n");
		timestamp = time(NULL) - timestamp;
		if (!timestamp)
			timestamp = 1;
		printf("Downloaded in %ld seconds,"
			" %ld bytes/s\n",
                       (long)timestamp, (long)len/timestamp);
		free(image);
	}
	return 0;
}

void dump_filename(struct canonfile *f)
{
		char *name, *ext, *p;
		char aux[1024];

	if (mydisplay == 1 )
	{
		p = strchr(f->name, '.');
		if (!p) {
			name = f->name;
			ext = "";
		} else {
			strcpy(aux, f->name);
			*(strchr(aux, '.')) = '\0';
			name = aux;
			ext = p+1;
		}

		printf("%c%c%c%c  ",
			(f->type & ATTR_PROTECTED) ? 'p' : '-',
			(f->type & ATTR_ITEMS) ? 'i' : '-',
			(f->type & ATTR_NEW) ? 'n' : '-',
			(f->type & ATTR_ENTERED) ? 'e' : '-');

		printf("%-8s%c%-3s  ", name, (f->type & 0x10) ? ' ' : '.', ext);
		if (f->type & 0x10) { /* directory? */
			printf("%-11s", "<DIR>");
		} else {
			if (f->size >= 1024)
				snprintf(aux, 1024, "%dk", f->size/1024);
			else
				snprintf(aux, 1024, "%d bytes", f->size);
			printf("%11.11s", aux);
		}
		printf("  %s", ctime(&f->date));
	}
}

int view_thumb(char *pathname)
{
	int result, childpid;
	char buffer[1024];

	buffer[0] = '\0';
	result = camera_get_thumb(pathname, TEMP_FILE_NAME);
	if (result == -1)
		return -1;
	childpid = fork();
	if (childpid == -1) {
		perror("fork");
		return -1;
	}

	if (childpid == 0) { /* child */
		execlp("xv", "xv", TEMP_FILE_NAME, "-geometry", "+200+100", NULL);
		perror("exec");
	} else { /* parent */
		printf("(d)elete (o)old (n)ew (g)et (q)uit (enter)nothing\n");
		while (fgets(buffer, 1024, stdin) == NULL);
		kill(childpid, 15);
	}
	return (int) buffer[0];
}

int view_all(void)
{
	int j;

	if (dirlist_size == 0) {
		printf("last ls is empty\n");
		return -1;
	}

	for (j = 0; j < dirlist_size; j++) {
		char c;
		char aux[1024];
		snprintf(aux, 1024, "%s\\%s", lastpath, dirlist[j]->name);
		c = view_thumb(aux);
		if (c == 'q' || c == 'Q')
			break;
		switch(c) {
		case 'd':
		case 'D':
#ifdef HAVE_USB_SUPPORT
			if (mode == USB_MODE)
				USB_delete(dirlist[j]->name);
			else
#endif
				serial_delete(dirlist[j]->name);
			break;
		case 'o':
		case 'O':
			camera_file_chmod(dirlist[j]->name,
				CHMOD_CLEAR, ATTR_NEW);
			break;
		case 'n':
		case 'N':
			camera_file_chmod(dirlist[j]->name,
				CHMOD_SET, ATTR_NEW);
			break;
		case 'g':
		case 'G':
			camera_get_image(dirlist[j]->name, NULL);
			break;
		}
	}
	return 0;
}

int camera_file_chmod(char *name, int action, int bits)
{
	int oldattr;
	char *p;

	/* get the filename from the path */
	p = strrchr(name, '\\');
	if (p != NULL)
		name = p+1;

	oldattr = camera_get_file_attr(name);
	if (oldattr == -1)
		return -1;

	if (action == CHMOD_SET)
		oldattr |= bits;
	else if (action == CHMOD_CLEAR)
		oldattr &= ~bits;

	if (mode == SERIAL_MODE) {
		return serial_set_file_attrib(name, oldattr);
	}
#ifdef HAVE_USB_SUPPORT
	else
	{
		return USB_set_file_attrib(name, oldattr);
	}
#endif
	return 0;
}

int camera_file_chmod_all(int action, int bits)
{
	int j;

	if (dirlist_size == 0) {
		printf("last ls is empty\n");
		return -1;
	}

	for (j = 0; j < dirlist_size; j++) {
		char aux[1024];
		int retval;

		snprintf(aux, 1024, "%s\\%s", lastpath, dirlist[j]->name);
		printf("chmod %s: ", aux);
		retval = camera_file_chmod(dirlist[j]->name, action, bits);
		if (retval == 0)
			printf("successful\n");
		else
			printf("ERROR\n");
	}
	printf("chmodall terminated\n");
	return 0;
}

int camera_delete_all(int which)
{
	int j;

	if (dirlist_size == 0) {
		printf("last ls is empty\n");
		return -1;
	}

	for (j = 0; j < dirlist_size; j++) {
		char aux[1024];
		int retval;

		if (which == WHICH_NEW) {
			if (!(dirlist[j]->type & ATTR_NEW))
				continue;
		}
		else if (which == WHICH_OLD) {
			if (dirlist[j]->type & ATTR_NEW)
				continue;
		}

		snprintf(aux, 1024, "%s\\%s", lastpath, dirlist[j]->name);
		printf("Removing %s: ", aux);
		if (dirlist[j]->type & ATTR_PROTECTED) {
			printf("PROTECTED, file skipped\n");
			continue;
		}

#ifdef HAVE_USB_SUPPORT
		if (mode == USB_MODE)
			retval = USB_delete(dirlist[j]->name);
		else
#endif
			retval = serial_delete(dirlist[j]->name);

		if (retval == 0)
			printf("successful\n");
		else
			printf("ERROR\n");
	}
	printf("delete terminated\n");
	return 0;
}

int camera_close(void)
{
	if (mode == SERIAL_MODE) {
		serial_close();
		printf("Now the camera should be OFF\n");
		return 0;
	} else {
		printf("Not implemented with USB\n");
		return -1;
	}
}

char *camera_get_id(void)
{
	if (mode == SERIAL_MODE) {
		char *name = serial_get_id();
		strcpy (camera_name, name);
	}
#ifdef HAVE_USB_SUPPORT
	else
		USB_get_id();
#endif
	return camera_name; /* avoid warnings */
}

char *camera_set_owner(char *name)
{
	if (mode == SERIAL_MODE) {
		printf("Not implemented with serial\n");
		return NULL;
	}
#ifdef HAVE_USB_SUPPORT
	else
		return USB_set_owner(name);
#endif
	return NULL; /* avoid warnings */
}


char *camera_get_disk(void)
{
	if (mode == SERIAL_MODE)
		return serial_get_disk();
#ifdef HAVE_USB_SUPPORT
	else
		return USB_get_disk();
#endif
	return NULL; /* avoid warnings */
}

void camera_startup_initialization(void)
{
	if (mode == SERIAL_MODE) {
		serial_initial_sync(serialdev);
		serial_timeout = 5;
		strncpy(cameraid, serial_get_id(), 1024);
		strncpy(lastpath, serial_get_disk(), 1024);
		camera_get_id ();
	} else if (mode == USB_MODE) {
#ifdef HAVE_USB_SUPPORT
		char *aux;
		USB_initial_sync();
		aux = camera_get_id();
		if (aux == NULL) {
			printf("USB protocol error, retry\n");
			exit(1);
		} else
			strncpy(cameraid, camera_name, 1024);
		aux = USB_get_disk();
		if (aux == NULL) {
			printf("USB protocol error, retry\n");
			exit(1);
		} else
			strncpy(lastpath, aux, 1024);
#endif
	}
}

void camera_ping(void)
{
	if (mode == SERIAL_MODE)
		serial_ping();
	else
		printf("Not implemented with USB\n");
}

int camera_get_disk_info(char *disk, int *size, int *free)
{
	if (mode == SERIAL_MODE)
		return serial_get_disk_info(disk, size, free);
#ifdef HAVE_USB_SUPPORT
	else
		return USB_get_disk_info(disk, size, free);
#endif
	return 0; /* avoid warnings */
}

int camera_get_power_status(int *good, int *ac)
{
	if (mode == SERIAL_MODE)
		return serial_get_power_status(good, ac);
#ifdef HAVE_USB_SUPPORT
	else
		return USB_get_power_status(good, ac);
#endif
	return 0; /* avoid warnings */
}

/* This file is part of s10sh
 *
 * Copyright (C) 2000 by Salvatore Sanfilippo <antirez@invece.org>
 * Copyright (C) 2001 by Salvatore Sanfilippo <antirez@invece.org>
 *
 * S10sh IS FREE SOFTWARE, UNDER THE TERMS OF THE GPL VERSION 2
 * don't forget what free software means, even if today is so diffused.
 *
 * SERIAL driver implementation
 *
 * ALL THIRD PARTY BRAND, PRODUCT AND SERVICE NAMES MENTIONED ARE
 * THE TRADEMARK OR REGISTERED TRADEMARK OF THEIR RESPECTIVE OWNERS
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "crc.h"		/* see crc.c, from gphoto's canon driver */
#include "s10sh.h"

int fd, pkt_sequence = 0;
unsigned char frag_sequence = 0;
int serial_timeout = 1;
int serial_u_timeout = 0;
unsigned char eot_sequence = 0;
unsigned char ack_sequence = 0;
struct termios backup, new;
int serial_speed = 115200;

#if __FreeBSD__
char *serialdev = "/dev/cuaa0";
#else
char *serialdev = "/dev/ttyS0";
#endif

int opt_a50 = 0; /* A50/Pro70 compatibility mode */

/* SERIAL COMMANDS STRINGS
   byte format: TDXXXX

   T: type byte
   O: direction byte from PC to camera
   I: direction byte from camera to PC
   XXXX: 4 belived random bytes
*/

unsigned char *msgtype_list[] =
{
        "\x01\x12\x22\x14\xf7\x8a\x00",         /* MSG_TYPE_CAMERA_ID */
        "\x01\x11\x21\x6a\x08\x79\x04",         /* MSG_TYPE_IMAGE */
        "\x01\x11\x21\xea\x0c\xb1\x02",         /* MSG_TYPE_THUMB */
        "\x04\x12\x00\x08\xd3\x9d\x00",         /* MSG_TYPE_SET_DATE */
        "\x05\x12\x00\xfc\xd2\x9d\x00",         /* MSG_TYPE_CH_OWNER */
        "\x09\x11\x21\xd8\xf7\x8a\x00",         /* MSG_TYPE_DISK_INFO */
        "\x0a\x11\x21\xdc\xf7\x8a\x00",         /* MSG_TYPE_GET_DISK */
        "\x0a\x12\x22\x70\xf6\x8a\x00",         /* MSG_TYPE_UNK_1 */
        "\x0b\x11\x21\x94\xf6\x8a\x00",         /* MSG_TYPE_LIST_WITHOUT_DATE */
        "\x0b\x11\x21\xa8\xf6\x8a\x00",         /* MSG_TYPE_LIST_WITH_DATE */
        "\x0d\x11\x21\x8c\xf4\x7b\x00",         /* MSG_TYPE_DELETE_IMG */
        "\x0a\x12\x22\x70\xf6\x8a\x00",         /* MSG_TYPE_POWER_STATUS */
        "\x03\x12\x12\x78\xf3\x64\x01",         /* MSG_TYPE_GET_DATE */
        "\x0e\x11\x00\x00\x00\x00\x00",         /* MSG_TYPE_SET_ATTRIB */
        "\x05\x11\x00\x00\x00\x00\x00",         /* MSG_TYPE_MKDIR */
        "\x06\x11\x00\x00\x00\x00\x00",         /* MSG_TYPE_RMDIR */
        "\x03\x11\x21\x00\x00\x00\x00",         /* MSG_TYPE_UPLOAD */
        NULL
};

int serial_flush_input(void)
{
	if (tcflush(fd,TCIFLUSH) < 0) {
		perror("tcflush input");
		return -1;
	}
	return 0;
}

int serial_flush_output(void)
{
	if (tcflush(fd,TCOFLUSH) == -1) {
		perror("tcflush output");
		return -1;
	}
	return 0;
}

int serial_init(char *device)
{
	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd == -1) {
		printf("Can't open the %s device\n", device);
		return -1;
	}

	if (tcgetattr(fd, &backup) == -1) {
		perror("tcgetattr");
		close(fd);
		return -1;
	}

	new = backup;

	new.c_cflag &= ~CSIZE;		/* clear the size bits */
	new.c_cflag |= CS8;		/* 8 bit data */
	new.c_cflag &= ~(CSTOPB);	/* 1 stop bit */
	new.c_cflag &= ~(CRTSCTS | PARENB | PARODD); /* no parity */

	/* raw mode */
	new.c_lflag = 0;		/* no local flags */
	new.c_oflag &= ~OPOST;		/* no output processing */
	new.c_iflag &= ~(BRKINT | IGNCR | INLCR | ICRNL |
		IXANY | IXON | IXOFF | INPCK | ISTRIP);
	new.c_iflag |= IGNPAR | IGNBRK;	/* ignore break conditions */

	new.c_cflag |= CLOCAL | CREAD;
	new.c_cc[VMIN] = 1;		/* return after one char */
	new.c_cc[VTIME] = 0;

	/* set speed */
	cfsetospeed(&new, B9600);
	cfsetispeed(&new, B9600);

	if (tcsetattr(fd, TCSANOW, &new) == -1) {
		perror("tcsetattr");
		close(fd);
		return -1;
	}

	(void) serial_flush_input();
	return 0;
}

int serial_change_serial_speed(int speed)
{
	cfsetospeed(&new, speed);
	cfsetispeed(&new, speed);

	if (tcsetattr(fd, TCSANOW, &new) == -1) {
		perror("tcsetattr");
		return -1;
	}
	if (opt_a50)
		sleep(1);
	return 0;
}

int serial_write(int fd, unsigned char *buffer, int size)
{
	int written = 0, i;

	if (opt_a50) {
		unsigned char *aux = buffer;

		for (i = 0; i < size; i++) {
			written = write(fd, aux, 1);
			if (written == -1)
				return -1;
			aux++;
			usleep(1);
		}
		dump_hex("WRITE", buffer, size);
	} else {
		while(size) {
			written = write(fd, buffer, size);
			if (written == -1)
				return -1;
			dump_hex("WRITE", buffer, written);
			size -= written;
			buffer += written;
		}
	}
	return 0;
}

int serial_send_frame(unsigned char *data, int len)
{
	int index = 0, j;
	unsigned char aux[4096];
	unsigned char buffer[4096];
	unsigned short cksum;

	/* add the checksum */
	cksum = canon_psa50_gen_crc(data, len);
	memcpy(aux, data, len);
	/* memcpy(aux+len, &cksum, sizeof(unsigned short)); */
	aux[len] = cksum & 0xff;
	aux[len+1] = cksum >> 8;
	len+=2;

	buffer[index] = 0xC0;
	index++;
	for (j = 0; j < len; j++) {
		switch(aux[j]) {
		case 0x7e:
		case 0xc0:
		case 0xc1:
			buffer[index] = 0x7e;
			index++;
			buffer[index] = aux[j] ^ 0x20;
			break;
		default:
			buffer[index] = aux[j];
			break;
		}
		index++;
	}
	buffer[index] = 0xC1;
	index++;
	return serial_write(fd, buffer, index);
}

int serial_send_pkt_message(unsigned char *pkt, unsigned short len, int morefrag)
{
	unsigned char buffer[4096];

	/* if morefrag is false this is the first message fragment */
	if (!morefrag)
		frag_sequence = 0;

	buffer[0] = frag_sequence++;
	buffer[1] = PKT_TYPE_MSG;
	/* memcpy(&buffer[2], &len, sizeof(unsigned short)); */
	buffer[2] = len & 0xff;
	buffer[3] = len >> 8;
	memcpy(&buffer[4], pkt, len);
	return serial_send_frame(buffer, len+4);
}

int serial_send_message_frag(int type, unsigned char *frag, unsigned short len, int morefrag)
{
	unsigned char buffer[4096];
	unsigned short totlen;
	unsigned char head[4];
	unsigned char mtype;

	if (type < 0 || type > 16) {
		fprintf(stderr, "INTERNAL BUG: serial_send_message_frag():"
				"message type %d not supported\n", type);
		safe_exit(1);
	}

	mtype = msgtype_list[type][0];
	memcpy(head, msgtype_list[type]+3, 4);

	buffer[0] = 0x02;
	buffer[1] = 0x00;
	buffer[2] = 0x00;
	buffer[3] = 0x00;
	buffer[4] = mtype;
	buffer[5] = 0x00;
	buffer[6] = 0x00;
	buffer[7] = msgtype_list[type][1]; /* direction: from PC to camera */
	totlen = len + 16;
	/* memcpy(&buffer[8], &totlen, sizeof(unsigned short)); */
	buffer[8] = totlen & 0xff;
	buffer[9] = totlen >> 8;
	buffer[10] = 0x00;
	buffer[11] = 0x00;
	memcpy(&buffer[12], head, 4);
	memcpy(&buffer[16], frag, len);
	return serial_send_pkt_message(buffer, totlen, morefrag);
}

int serial_send_ack(unsigned int ack_error)
{
	char ack[6];

	memcpy(ack, "\x00\x05\x00\x00\x00\x00", 6);
	ack[0] = ack_sequence;
	ack[2] = ack_error;
	if (ack_error == ACK_ERROR_NONE)
		ack_sequence++;
	return serial_send_frame(ack, 6);
}

int serial_send_switch_speed(void)
{
	int result;
	unsigned char *speedstr;

	switch(serial_speed) {
	case 9600:
		speedstr = SPEED_9600;
		break;
	case 19200:
		speedstr = SPEED_19200;
		break;
	case 38400:
		speedstr = SPEED_38400;
		break;
	case 57600:
		speedstr = SPEED_57600;
		break;
	case 115200:
	default:
		speedstr = SPEED_115200;
		break;
	}
	result = serial_write(fd, speedstr+1, speedstr[0]);

	/* sleep(1); */
	return result;
}

int serial_send_eot(void)
{
	char eot[6];

	memcpy(eot, "\x00\x04\x01\x00\x00\x00", 6);
	eot[0] = eot_sequence++;
	return serial_send_frame(eot, 6);
}

int serial_send_ping(void)
{
	char eot[6];

	memcpy(eot, "\x00\x04\x00\x00\x00\x00", 6);
	eot[0] = eot_sequence++;
	return serial_send_frame(eot, 6);
}

int serial_send_switch_off(void)
{
	int result;

        serial_write(fd, "\xC0\x00\x02\x55\x2C\xC1",6);
        result = serial_write(fd, "\xC0\x00\x04\x01\x00\x00\x00\x24\xC6\xC1",8);
	sleep(1);
	return result;
}

int serial_read(int fd, char *buffer, int size)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = serial_timeout;
	tv.tv_usec = serial_u_timeout;

	retval = select(fd+1, &rfds, NULL, NULL, &tv);
	if (retval) {
		return read(fd, buffer, size);
	}
	if (opt_debug)
		printf("READ TIMEOUT\n");
	return -1;
}

int serial_get_byte(void)
{
	static unsigned char buffer[1024];
	static int buffer_size = 0;
	static unsigned char *buffer_pointer;
	int n_read;

	if (buffer_size == 0) {
		n_read = serial_read(fd, buffer, 1024);
		if (n_read <= 0)
			return -1;
		buffer_size = n_read;
		buffer_pointer = buffer;
	}

	buffer_size--;
	buffer_pointer++;
	return *(buffer_pointer-1);
}

unsigned char *serial_get_frame(int *len)
{
	static unsigned char buffer[4096];
	int index = 0;
	int c;

	while((c = serial_get_byte()) != 0xC0) /* search the start */
		if (c == -1) return NULL;
	while((c = serial_get_byte()) != 0xC1) { /* C1 == end of the frame */
		if (c == -1) return NULL;
		if (c == 0x7e) {
			c = serial_get_byte();
			if (c == -1) return NULL;
			c ^= 0x20;
		}
		buffer[index] = c;
		index++;
		if (index == 4096) {
			fprintf(stderr, "FATAL ERROR serial_get_frame(): "
					"not enough buffer\n");
			exit(1);
		}
	}
	*len = index;
	dump_hex("RECV", buffer, index);

	/* the camera is no longer to PC mode? */
	if (index >= 13 && !memcmp(buffer, "\x00\x00\x10\x00\x02\x00\x00\x00\x02\x00\x04\x00\x10", 13))
		serial_nolonger_pcmode();
	return buffer;
}

unsigned char *serial_get_packet(struct header *hdr)
{
	unsigned char *frame;
	int framelen;

	frame = serial_get_frame(&framelen);
	if (frame == NULL)
		return NULL;

	hdr->seq = frame[SEQ_OFFSET];
	hdr->type = frame[TYPE_OFFSET];
	hdr->framelen = framelen;
	hdr->cksum = (frame[framelen-2] & 0xff) | (frame[framelen-1] << 8);

	if (canon_psa50_chk_crc(frame, framelen-2, hdr->cksum) == 0) {
		/* printf("BAD CRC RECEIVED\n"); */
		hdr->cksum_ok = 0;
	} else {
		hdr->cksum_ok = 1;
	}

	switch(hdr->type) {
	case PKT_TYPE_MSG:
		hdr->len = (frame[LEN_OFFSET] & 0xff)|(frame[LEN_OFFSET+1] <<8);
		hdr->data = &frame[DATA_OFFSET];
		break;
	case PKT_TYPE_EOT:
		hdr->eot_len = frame[EOTLEN_OFFSET];
		break;
	case PKT_TYPE_ACK:
		hdr->ack_err = frame[ACKERR_OFFSET];
		break;
	case PKT_TYPE_INIT:
		hdr->len = (frame[LEN_OFFSET] & 0xff)|(frame[LEN_OFFSET+1] <<8);
		hdr->data = &frame[DATA_OFFSET];
		break;
	default:
		fprintf(stderr, "ERROR: Unknown packet type received\n");
		break;
	}
	return frame;
}

int serial_get_ack(void)
{
	struct header hdr;
	unsigned char *frame;
	unsigned char eot_minus_one = eot_sequence-1;

	frame = serial_get_packet(&hdr);
	if (!frame) {
		printf("serial_get_packet() error waiting for ACK\n");
		return -1;
	}

	if (hdr.type != PKT_TYPE_ACK) {
		printf("Received a packet type %d waiting for ACK\n", hdr.type);
		return -2;
	}

	if (hdr.seq != eot_minus_one) {
		printf("Out of sequence ACK received, expected %d, got %d\n",
			eot_minus_one, hdr.seq);
		return -3;
	}

	return hdr.ack_err;
}

int serial_get_eot(void)
{
	struct header hdr;
	unsigned char *frame;

	frame = serial_get_packet(&hdr);
	if (!frame) {
		printf("serial_get_packet() error waiting EOT\n");
		return -1;
	}

	if (hdr.type != PKT_TYPE_EOT) {
		printf("Received a packet type %d waiting EOT\n", hdr.type);
		return -2;
	}

	if (hdr.seq != ack_sequence) {
		printf("Out of sequence ACK received, expected %d, got %d\n",
			ack_sequence, hdr.seq);
		return -3;
	}

	return 0;
}

int serial_initial_sync(char *device)
{
	struct header hdr;
	unsigned char *pkt;
	int len;

	int retry = 0;
	printf("Open %s: ", device);
	if (serial_init(device) == -1) {
		printf("failure\n");
		exit(1);
	}
	printf("OK\n");
	printf("Sync: "); fflush(stdout);

	/* set the delay between UUUU sequences */
	serial_timeout = 0;
	if (!opt_a50)
		serial_u_timeout = 400000;
	else
		serial_u_timeout = 900000;

	while(1) {
		write(fd, "UUUU", 4);
		pkt = serial_get_packet(&hdr);
		if (pkt) break;
		printf("."); fflush(stdout);
	}
	printf("\n");

	printf("::%s::\n", (char*)(pkt+26));
	pkt = serial_get_frame(&len); /* get the EOT */
	serial_send_ack(ACK_ERROR_NONE);

	/* set speed */
	printf("Switching to %d bound ", serial_speed);
	serial_timeout = 0;
	serial_send_switch_speed();
	serial_send_eot();
	pkt = serial_get_frame(&len);

	switch(serial_speed) {
	case 9600:
		serial_change_serial_speed(B9600);
		break;
	case 19200:
		serial_change_serial_speed(B19200);
		break;
	case 38400:
		serial_change_serial_speed(B38400);
		break;
	case 57600:
		serial_change_serial_speed(B57600);
		break;
	case 115200:
	default:
		serial_change_serial_speed(B115200);
		serial_change_serial_speed(B115200);
		break;
	}

	/* ping */
	pkt = NULL;
	while(!pkt) {
		printf("."); fflush(stdout);
		serial_send_ping();
		pkt = serial_get_frame(&len);
		if (!pkt) eot_sequence--;
		retry++;
		if (retry == 10) {
			printf("protocol error\n");
			exit(1);
		}
	}
	printf("OK\n");

	return 0;
}

/* The serial_get_id return the pointer the camera ID string, but you
   should be aware since the pointer is valid only after the call.
   Next camera requests will overwrite the camera ID.
   This also applied to some other serial_get_* function.
*/
char *serial_get_id(void)
{
	char *pkt;
	struct header hdr;
	static char buffer[1024];

	serial_send_message_frag(MSG_TYPE_CAMERA_ID, NULL, 0, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */

	strncpy(buffer, hdr.data+28, 1024);
	firmware[1] = firmware[3] = firmware[5] = '.';
	firmware[0] = *(hdr.data+27)+'0';
	firmware[2] = *(hdr.data+26)+'0';
	firmware[4] = *(hdr.data+25)+'0';
	firmware[6] = *(hdr.data+24)+'0';
	firmware[7] = '\0';

	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	return buffer;
}

char *serial_get_disk(void)
{
	char *pkt;
	struct header hdr;
	static char buffer[1024];

	serial_send_message_frag(MSG_TYPE_GET_DISK, NULL, 0, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */

	strncpy(buffer, hdr.data+20, 1024);

	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	return buffer;
}

void serial_ping(void)
{
	unsigned char *pkt;
	int len, c;
	unsigned long timestamp;

	for (c = 0; c < 4; c++) {
		timestamp = get_usec();
		serial_send_ping();
		pkt = serial_get_frame(&len);
		timestamp = get_usec() - timestamp;
		if (pkt) {
			printf("pong %d, %lu ms\n", c, timestamp);
		} else {
			printf("time out\n");
		}
	}
}

int serial_get_disk_info(char *disk, int *size, int *free)
{
	unsigned char *pkt;
	struct header hdr;
	char diskstr[] = "X:\\";

	diskstr[0] = disk[0];
	serial_send_message_frag(MSG_TYPE_DISK_INFO, diskstr, 4, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	if (*(hdr.data+16) == 0x87)
		return -1;
	*size = *(int*)(hdr.data+20);
	*size = byteswap32(*size);
	*free = *(int*)(hdr.data+24);
	*free = byteswap32(*free);
	return 0;
}

int serial_mkdir(char *pathname)
{
	unsigned char *pkt;
	struct header hdr;
	char arg[1024];

	if (strlen(pathname) <= 2 || pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	serial_send_message_frag(MSG_TYPE_MKDIR, pathname, strlen(pathname)+1, 0); 
	serial_send_eot(); 
	serial_get_ack(); 
	pkt = serial_get_packet(&hdr); /* data  */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE); 
	if (hdr.data[16] == 0x00)
		return 0;
	else
		return -1;
}

int serial_rmdir(char *pathname)
{
	unsigned char *pkt;
	struct header hdr;
	char arg[1024];

	if (strlen(pathname) <= 2 || pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	serial_send_message_frag(MSG_TYPE_RMDIR, pathname, strlen(pathname)+1, 0); 
	serial_send_eot(); 
	serial_get_ack(); 
	pkt = serial_get_packet(&hdr); /* data  */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE); 
	if (hdr.data[16] == 0x00)
		return 0;
	else
		return -1;
}

int serial_delete(char *pathname)
{
	unsigned char *pkt;
	struct header hdr;
	char arg[1024];

	if (strlen(pathname) <= 2 || pathname[1] != ':') {
		snprintf(arg, 1024, "%s\\%s", lastpath, pathname);
		pathname = arg;
	}

	serial_send_message_frag(MSG_TYPE_DELETE_IMG, pathname, strlen(pathname)+1, 0); 
	serial_send_eot(); 
	serial_get_ack(); 
	pkt = serial_get_packet(&hdr); /* data  */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE); 
	return 0;
	/* seems that the serial protocol lacks erorr handling in file
	   deletion... */
}

int serial_set_file_attrib(char *pathname, unsigned char newattrib)
{
	unsigned char buffer[1024];
	unsigned char *pkt;
	struct header hdr;

	buffer[0] = newattrib;
	buffer[1] = buffer[2] = buffer[3] = 0x00;
	memcpy(buffer+4, lastpath, strlen(lastpath)+1);
	buffer[4+strlen(lastpath)] = '\\';
	memcpy(buffer+4+strlen(lastpath)+1, pathname, strlen(pathname)+1);
	serial_send_message_frag(MSG_TYPE_SET_ATTRIB, buffer, 4+strlen(lastpath)+1+strlen(pathname)+1, 0); 
	serial_send_eot(); 
	serial_get_ack(); 
	pkt = serial_get_packet(&hdr); /* data  */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE); 
	return 0;
}

unsigned char *serial_get_data(char *pathname, int reqtype, int *retlen)
{
	char aux[1024];
	unsigned char *pkt;
	struct header hdr;
	int count, frag_count;
	unsigned char *image = NULL;
	int current_offset = 0;
	int n_read = 0;
	int last_current_offset = 0;
	int last_n_read = 0;
	int last_sequence_bad_crc = 0;
	int totlen = 0;
	int offset;
	int size;

	memset(aux, 0, 5);
	aux[0] = reqtype; /* set it to 0x01 for thumbnail 0x00 for image */
	aux[5] = (char) strlen(pathname)+1; /* includes the NUL term */
	aux[6] = aux[7] = 0x00;
	strncpy(aux+8, pathname, 256);
	*(aux+8+strlen(pathname)) = 0x00;

	serial_send_message_frag(MSG_TYPE_IMAGE, aux, 9+strlen(pathname), 0);
	serial_send_eot();
	serial_get_ack();

	count = frag_count = 0;
	while(1) {
		pkt = serial_get_packet(&hdr); /* data */
		if (!hdr.cksum_ok)
			last_sequence_bad_crc = 1;

		count++;
		frag_count++;

		if (hdr.type == PKT_TYPE_EOT) {
			/* error recovery */
			if (last_sequence_bad_crc) {
				printf("X");
				serial_send_ack(ACK_ERROR_RETRALL);
				current_offset = last_current_offset;
				n_read = last_n_read;
				last_sequence_bad_crc = 0;
				continue;
			} else {
				serial_send_ack(ACK_ERROR_NONE);
				last_current_offset = current_offset;
				last_n_read = n_read;
			}
			last_sequence_bad_crc = 0;

			if (n_read >= totlen) {
				*retlen = n_read;
				return image;
			}
			continue;
		}

		if (hdr.seq == 0) {
			if (*(hdr.data+16) != 0x00 && count == 1) {
				serial_get_eot();
				serial_send_ack(ACK_ERROR_NONE);
				return NULL;
			}

			totlen = byteswap32(*(unsigned int*)(hdr.data+20));
			offset = byteswap32(*(unsigned int*)(hdr.data+24));
			size = byteswap32(*(unsigned int*)(hdr.data+28));

			if (count == 1) {
				image = malloc(totlen);
				if (!image) {
					perror("malloc");
					safe_exit(1);
				}
				printf("Getting %s, %d bytes\n", pathname,
					totlen);
				progressbar(PROGRESS_RESET, 0, 0);
			}

			memcpy(image+current_offset, hdr.data+36, hdr.len-(36));
			current_offset += (hdr.len-(36));
			n_read += (hdr.len-(36));
		} else {
			memcpy(image+current_offset, hdr.data, hdr.len);
			current_offset += hdr.len;
			n_read += hdr.len;
		}
		progressbar(PROGRESS_PRINT, totlen, n_read);
	}
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
}

void serial_debug_getpkt(void)
{
	unsigned char *pkt;
	int len;

	pkt = serial_get_frame(&len);
	return;
}

void serial_nolonger_pcmode(void)
{
	printf("*** your camera is no longer in PC mode, exit\n");
	exit(1);
}

int serial_get_power_status(int *good, int *ac)
{
	unsigned char *pkt;
	struct header hdr;

	serial_send_message_frag(MSG_TYPE_POWER_STATUS, NULL, 0, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	if (*(hdr.data+20) == 0x06)
		*good = 1;
	else
		*good = 0;

	if (*(hdr.data+23) == 0x10)
		*ac = 1;
	else
		*ac = 0;

	return 0;
}

int serial_test_message(int msgtype)
{
	unsigned char *pkt;
	struct header hdr;

	serial_send_message_frag(msgtype, NULL, 0, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	return 0;
}

time_t serial_get_date(void)
{
	unsigned char *pkt;
	struct header hdr;

	serial_send_message_frag(MSG_TYPE_GET_DATE, NULL, 0, 0);
	serial_send_eot();
	serial_get_ack();
	pkt = serial_get_packet(&hdr); /* data */
	serial_get_eot();
	serial_send_ack(ACK_ERROR_NONE);
	return byteswap32(*(time_t*)(hdr.data+20));
}

void serial_change_speed(int speed)
{
	switch(speed) {
	case 0:
		printf("Current serial speed: %d\n", serial_speed);
		break;
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
		serial_speed = speed;
		break;
	default:
		printf(	"Invalid speed %d\n"
			"Try 9600, 19200, 38400, 57600, 115200.\n", speed);
		break;
	}
}

int serial_open(void)
{
	if (fd != -1)
		return -1;
	serial_send_switch_off();
	close(fd);
	pkt_sequence = 0;
	frag_sequence = 0;
	serial_timeout = 1;
	serial_u_timeout = 0;
	eot_sequence = 0;
	ack_sequence = 0;
	dirlist_size = 0;
	lastpath[0] = '\0';
	serial_initial_sync(serialdev);
	serial_timeout = 5;
	strncpy(cameraid, serial_get_id(), 1024);
	strncpy(lastpath, serial_get_disk(), 1024);
	return 0;
}

int serial_close(void)
{
	if (fd == -1)
		return -1;
	serial_send_switch_off();
	close(fd);
	fd = -1; /* -1 means not connected */
	return 0;
}

int serial_upload(char *source, char *target)
{
	struct stat buf;
	unsigned char buffer[4096*2];
	unsigned int offset, datalen, aux;
	char arg[1024];
	char read_buffer[1024];
	struct header hdr;
	unsigned char *pkt;
	int fd;
	int progress_bar = 0;

	if (target == NULL) {
		char *p;
		p = strrchr(source, '/');
		if (p != NULL)
			target = p+1;
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
		datalen = read(fd, read_buffer, 800);
		if (datalen == 0) {
			break;
		} else if (datalen == -1) {
			perror("read");
			return -1;
		}

		/* 02 00 00 00 */
		aux = byteswap32(0x02);
		memcpy(buffer+0x00, &aux, 4);

		/* offset */
		aux = byteswap32(offset);
		memcpy(buffer+0x04, &aux, 4);

		/* datalen */
		aux = byteswap32(datalen);
		memcpy(buffer+0x08, &aux, 4);

		memcpy(buffer+0x0c, target, strlen(target)+1);
		memcpy(buffer+0x0c+strlen(target)+1, read_buffer, datalen);

		serial_send_message_frag(MSG_TYPE_UPLOAD, buffer, 12+strlen(target)+1+datalen, 0); 
		serial_send_eot(); 
		serial_get_ack(); 
		pkt = serial_get_packet(&hdr); /* data  */
		serial_get_eot();
		serial_send_ack(ACK_ERROR_NONE);

		offset += datalen;
		progressbar(PROGRESS_PRINT, buf.st_size, offset);
	}
	close(fd);
	return 0;
}

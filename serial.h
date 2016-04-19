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

#ifndef S10SH_SERIAL_H
#define S10SH_SERIAL_H

struct header {
	unsigned char seq;
	unsigned char type;
	unsigned short len;
	unsigned char *data;
	unsigned short cksum;
	unsigned char cksum_ok;
	unsigned char ack_err;
	unsigned char eot_len;
	int framelen;
};

/* serial offsets */
#define SEQ_OFFSET	0
#define TYPE_OFFSET	1
#define EOTLEN_OFFSET	2
#define LEN_OFFSET	2
#define ACKERR_OFFSET	2
#define DATA_OFFSET	4

/* serial packet types */
#define PKT_TYPE_MSG	0x00
#define PKT_TYPE_EOT	0x04
#define PKT_TYPE_ACK	0x05
#define PKT_TYPE_INIT	0x06

/* serial commands */
#define MSG_TYPE_CAMERA_ID 0
#define MSG_TYPE_IMAGE 1
#define MSG_TYPE_THUMB 2
#define MSG_TYPE_SET_DATE 3
#define MSG_TYPE_CH_OWNER 4
#define MSG_TYPE_DISK_INFO 5
#define MSG_TYPE_GET_DISK 6
#define MSG_TYPE_UNK_1 7
#define MSG_TYPE_LIST_WITHOUT_DATE 8
#define MSG_TYPE_LIST_WITH_DATE 9
#define MSG_TYPE_DELETE_IMG 10
#define MSG_TYPE_POWER_STATUS 11
#define MSG_TYPE_GET_DATE 12
#define MSG_TYPE_SET_ATTRIB 13
#define MSG_TYPE_MKDIR 14
#define MSG_TYPE_RMDIR 15
#define MSG_TYPE_UPLOAD 16

/* ACK subtypes */
#define ACK_ERROR_NONE		0x00
#define ACK_ERROR_RETR1		0x01
#define ACK_ERROR_RETR2		0x02
#define ACK_ERROR_RETR3		0x03
#define ACK_ERROR_RETR4		0x04
#define ACK_ERROR_RETR5		0x05
#define ACK_ERROR_RETR6		0x05
#define ACK_ERROR_RETR7		0x07
#define ACK_ERROR_RETR8		0x08
#define ACK_ERROR_RETRALL	0xFF

/* serial speed changing commands */
#define SPEED_9600	"\x0F\xC0\x00\x03\x02\x02\x01\x10" \
			"\x00\x00\x00\x00\x7e\xe0\x39\xC1"
#define SPEED_19200	"\x0E\xC0\x00\x03\x08\x02\x01\x10" \
			"\x00\x00\x00\x00\x13\x1f\xC1"
#define SPEED_38400	"\x0E\xC0\x00\x03\x20\x02\x01\x10" \
			"\x00\x00\x00\x00\x5f\x84\xC1"
#define SPEED_57600	"\x0E\xC0\x00\x03\x40\x02\x01\x10" \
			"\x00\x00\x00\x00\x5e\x57\xC1"
#define SPEED_115200	"\x0E\xC0\x00\x03\x80\x02\x01\x10" \
			"\x00\x00\x00\x00\x4d\xf9\xC1"

extern int fd, pkt_sequence;
extern unsigned char frag_sequence;
extern int serial_timeout;
extern int serial_u_timeout;
extern unsigned char eot_sequence;
extern unsigned char ack_sequence;
extern struct termios backup, new;
extern int serial_speed;
extern char *serialdev;
extern int opt_a50;
extern unsigned char *msgtype_list[];

/* function prototypes */
int serial_write(int fd, unsigned char *buffer, int size);
int serial_send_frame(unsigned char *data, int len);
int serial_send_pkt_message(unsigned char *pkt, unsigned short len, int morefrag);
int serial_send_message_frag(int type, unsigned char *frag, unsigned short len, int morefrag);
int serial_send_ack(unsigned int ack_error);
int serial_get_ack(void);
int serial_send_switch_speed(void);
int serial_send_eot(void);
int serial_send_ping(void);
int serial_send_switch_off(void);
int serial_read(int fd, char *buffer, int size);
int serial_get_byte(void);
unsigned char *serial_get_frame(int *len);
unsigned char *serial_get_packet(struct header *hdr);
int serial_initial_sync(char *device);
char *serial_get_id(void);
char *serial_get_disk(void);
void serial_ping(void);
int serial_get_disk_info(char *disk, int *size, int *free);
void serial_debug_getpkt(void);
void serial_nolonger_pcmode(void);
int serial_get_power_status(int *good, int *ac);
int serial_test_message(int msgtype);
time_t serial_get_date(void);
void serial_change_speed(int speed);
int serial_flush_input(void);
int serial_flush_output(void);
int serial_init(char *device);
int serial_change_serial_speed(int speed);
unsigned char *serial_get_data(char *pathname, int reqtype, int *retlen);
int serial_open(void);
int serial_close(void);
int serial_mkdir(char *pathname);
int serial_rmdir(char *pathname);
int serial_upload(char *source, char *target);
int serial_delete(char *pathname);
int serial_set_file_attrib(char *pathname, unsigned char newattrib);

#endif /* S10SH_SERIAL_H */

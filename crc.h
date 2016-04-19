#ifndef CRC_H
#define CRC_H

unsigned short canon_psa50_gen_crc(const char *pkt,int len);
int canon_psa50_chk_crc(const char *pkt,int len,unsigned short crc);

#endif

/* Original code from the Linux C library */

#ifndef S10SH_BYTESEX_H
#define S10SH_BYTESEX_H

#if 	defined(__i386__) \
	|| defined(__alpha__) \
	|| (defined(__mips__) && (defined(MIPSEL) || defined (__MIPSEL__)))
#define BYTE_ORDER_LITTLE_ENDIAN
#elif 	defined(__mc68000__) \
	|| defined (__sparc__) \
	|| defined (__sparc) \
	|| defined (__PPC__) \
	|| (defined(__mips__) && (defined(MIPSEB) || defined (__MIPSEB__)))
#define BYTE_ORDER_BIG_ENDIAN
#else
# error can not find the byte order for this architecture, fix bytesex.h
#endif

#endif /* S10SH_BYTESEX_H */

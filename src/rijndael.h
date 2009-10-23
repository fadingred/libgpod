/* Rijndael Block Cipher - rijndael.h

   Written by Mike Scott 21st April 1999
   mike@compapp.dcu.ie

   Permission for free direct or derivative use is granted subject 
   to compliance with any conditions that the originators of the 
   algorithm place on its exploitation.  

*/
#ifndef RIJNDAEL_H
#define RIJNDAEL_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <glib.h>

#define u8 uint8_t /* 8 bits  */
#define u32 uint32_t       /* 32 bits */
#define u64 uint64_t

/* rotates x one bit to the left */

#define ROTL(x) (((x)>>7)|((x)<<1))

/* Rotates 32-bit word left by 1, 2 or 3 byte  */

#define ROTL8(x) (((x)<<8)|((x)>>24))
#define ROTL16(x) (((x)<<16)|((x)>>16))
#define ROTL24(x) (((x)<<24)|((x)>>8))


G_GNUC_INTERNAL void aes_set_key(u8 *key);
G_GNUC_INTERNAL void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
G_GNUC_INTERNAL void aes_encrypt(const u8 *initiv, u8 *inbuf, u8 *outbuf, unsigned long long len);

#endif

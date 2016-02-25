/**
 * This is ecc code taken from YAFFS.
 * There is no special reason to use this code specifically (or at all)
 * Any other code can be linked to nand.c for ecc code
 */

/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 *
 * yaffs_ecc.c: ECC generation/correction algorithms.
 *
 * Copyright (C) 2002 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 */

 /*
  * This code implements the ECC algorithm used in SmartMedia.
  *
  * The ECC comprises 22 bits of parity information and is stuffed into 3 bytes. 
  * The two unused bit are set to 1.
  * The ECC can correct single bit errors in a 256-byte page of data. Thus, two such ECC 
  * blocks are used on a 512-byte NAND page.
  *
  */

/* Table generated by gen-ecc.c
 * Using a table means we do not have to calculate p1..p4 and p1'..p4'
 * for each byte of data. These are instead provided in a table in bits7..2.
 * Bit 0 of each entry indicates whether the entry has an odd or even parity, and therefore
 * this bytes influence on the line parity.
 */

//const char *yaffs_ecc_c_version =
//    "$Id: yaffs_ecc.c,v 1.1 2007-08-07 14:57:16 aviadzuc Exp $";

#include <utils/yaffs_ecc.h>

static const unsigned char column_parity_table[] = {
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
};

static int yaffs_CountBits32(unsigned x)
{
	int r = 0;
	while (x) {
		if (x & 1)
			r++;
		x >>= 1;
	}
	return r;
}

/* Calculate the ECC for a 256-byte block of data */
void yaffs_ECCCalculate(const unsigned char *data, unsigned char *ecc)
{
	unsigned int i;

	unsigned int col_parity = 0;
	unsigned int line_parity = 0;
	unsigned int line_parity_prime = 0;
	unsigned int t;
	unsigned int b;

	for (i = 0; i < 256; i++) {		
		b = column_parity_table[*data++];
		col_parity ^= b;

		if (b & 0x01)	// odd number of bits in the byte
		{
			line_parity ^= i;
			line_parity_prime ^= ~i;
		}

	}

	ecc[2] = (~col_parity) | 0x03;

	t = 0;
	if (line_parity & 0x80)
		t |= 0x80;
	if (line_parity_prime & 0x80)
		t |= 0x40;
	if (line_parity & 0x40)
		t |= 0x20;
	if (line_parity_prime & 0x40)
		t |= 0x10;
	if (line_parity & 0x20)
		t |= 0x08;
	if (line_parity_prime & 0x20)
		t |= 0x04;
	if (line_parity & 0x10)
		t |= 0x02;
	if (line_parity_prime & 0x10)
		t |= 0x01;
	ecc[1] = ~t;

	t = 0;
	if (line_parity & 0x08)
		t |= 0x80;
	if (line_parity_prime & 0x08)
		t |= 0x40;
	if (line_parity & 0x04)
		t |= 0x20;
	if (line_parity_prime & 0x04)
		t |= 0x10;
	if (line_parity & 0x02)
		t |= 0x08;
	if (line_parity_prime & 0x02)
		t |= 0x04;
	if (line_parity & 0x01)
		t |= 0x02;
	if (line_parity_prime & 0x01)
		t |= 0x01;
	ecc[0] = ~t;

#ifdef CONFIG_YAFFS_ECC_WRONG_ORDER
	// Swap the bytes into the wrong order
	t = ecc[0];
	ecc[0] = ecc[1];
	ecc[1] = t;
#endif
}


/* Correct the ECC on a 256 byte block of data */

int yaffs_ECCCorrect(unsigned char *data, unsigned char *read_ecc,
		     const unsigned char *test_ecc)
{
	unsigned int d0, d1, d2;	/* deltas */

	d0 = read_ecc[0] ^ test_ecc[0];
	d1 = read_ecc[1] ^ test_ecc[1];
	d2 = read_ecc[2] ^ test_ecc[2];

	if ((d0 | d1 | d2) == 0)
		return 0; /* no error */

	if (((d0 ^ (d0 >> 1)) & 0x55) == 0x55 &&
	    ((d1 ^ (d1 >> 1)) & 0x55) == 0x55 &&
	    ((d2 ^ (d2 >> 1)) & 0x54) == 0x54) {
		/* Single bit (recoverable) error in data */

		unsigned byte;
		unsigned bit;

#ifdef CONFIG_YAFFS_ECC_WRONG_ORDER
		// swap the bytes to correct for the wrong order
		unsigned char t;

		t = d0;
		d0 = d1;
		d1 = t;
#endif

		bit = byte = 0;

		if (d1 & 0x80)
			byte |= 0x80;
		if (d1 & 0x20)
			byte |= 0x40;
		if (d1 & 0x08)
			byte |= 0x20;
		if (d1 & 0x02)
			byte |= 0x10;
		if (d0 & 0x80)
			byte |= 0x08;
		if (d0 & 0x20)
			byte |= 0x04;
		if (d0 & 0x08)
			byte |= 0x02;
		if (d0 & 0x02)
			byte |= 0x01;

		if (d2 & 0x80)
			bit |= 0x04;
		if (d2 & 0x20)
			bit |= 0x02;
		if (d2 & 0x08)
			bit |= 0x01;

		data[byte] ^= (1 << bit);

		return 1; /* Corrected the error */
	}

	if ((yaffs_CountBits32(d0) + 
	     yaffs_CountBits32(d1) + 
	     yaffs_CountBits32(d2)) ==  1) {
		/* Reccoverable error in ecc */

		read_ecc[0] = test_ecc[0];
		read_ecc[1] = test_ecc[1];
		read_ecc[2] = test_ecc[2];

		return 1; /* Corrected the error */
	}
	
	/* Unrecoverable error */

	return -1;

}
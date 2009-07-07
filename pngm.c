#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

typedef unsigned char u_char;
typedef int fd_t;

struct net32_s
{
	u_char a;
	u_char b;
	u_char c;
	u_char d;
};
typedef struct net32_s net32_t;

struct color48_s
{
	u_char r1;
	u_char r2;
	u_char g1;
	u_char g2;
	u_char b1;
	u_char b2;
};
typedef struct color48_s color48_t;

struct bkgd_chunk_s
{
	u_char name[4];
	color48_t color;
};
typedef struct bkgd_chunk_s bkgd_chunk_t;

#define from_net32(dst, src) \
dst = 0; \
dst += src.a; \
dst <<= 8; \
dst += src.b; \
dst <<= 8; \
dst += src.c; \
dst <<= 8; \
dst += src.d;

#define to_net32(dst, src) \
dst.d = src & 0xFF; \
dst.c = src >> 8 & 0xFF; \
dst.b = src >> 16 & 0xFF; \
dst.a = src >> 24 & 0xFF;


char *png_header = "\x89PNG\x0D\x0A\x1A\x0A";

static int process (char const *srcfn, char const *dstfn);
static size_t copy_bytes (FILE *dst, FILE *src,  size_t n);
unsigned long compute_crc (void *buf, int len);


int main (int argc, char const *argv[])
{
	if (argc != 3)
	{
		printf("%s\n", "Usage:\n  pngm src.png dst.png");
		return 1;
	}
	
	return process(argv[1], argv[2]);
}

int
process (char const *srcfn, char const *dstfn)
{
	FILE *src, *dst;
	
	src  = fopen(srcfn,  "rb");
	if (src == NULL)
	{
		perror("ERROR: Can't open src file for reading");
		return 2;
	}
	
	dst = fopen(dstfn, "wb");
	if (dst == NULL)
	{
		perror("ERROR: Can't open dst file for writing");
		return 3;
	}
	
	u_char *header = malloc(8);
	assert(header);
	u_char *name = malloc(5);
	assert(name);
	
	if (fread(header, 8, 1, src) < 1)
	{
		fprintf(stderr, "ERROR: Can't read header from %s: %s", srcfn, header);
		return 4;
	}
	
	if (memcmp(png_header, header, 8) != 0)
	{
		fprintf(stderr, "WARNING: Invalid header in %s: %s", srcfn, header);
	}
	
	fwrite(header, 8, 1, dst);
	
	net32_t size_net;
	net32_t crc_net;
	size_t size;
	unsigned long crc;
	
	for (;;)
	{
		if (fread(&size_net, sizeof(size_net), 1, src) < 1)
			break;
		from_net32(size, size_net);
		
		if (fread(name, 4, 1, src) < 1)
			break;
		name[4] = '\0';
		
		if (memcmp(name, "IDAT", 4) == 0)
		{
			bkgd_chunk_t bkgd;
			memcpy(bkgd.name, "bKGD", 4);
			color48_t bkgd_color = {0, 255, 0, 255, 0, 255};
			memcpy(&bkgd.color, &bkgd_color, sizeof(bkgd.color));
			
			net32_t bkgd_size_net;
			net32_t bkgd_crc_net;
			size_t bkgd_size = sizeof(bkgd.color);
			unsigned long bkgd_crc = compute_crc(&bkgd, sizeof(bkgd));
			
			printf("+ %s size: %6zd, crc: %10lu\n", bkgd.name, bkgd_size, bkgd_crc);
			
			to_net32(bkgd_size_net, bkgd_size);
			to_net32(bkgd_crc_net, bkgd_crc);
			
			fwrite(&bkgd_size_net, sizeof(bkgd_size_net), 1, dst);
			fwrite(bkgd.name, 4, 1, dst);
			fwrite(&bkgd.color, sizeof(bkgd.color), 1, dst);
			fwrite(&bkgd_crc_net, sizeof(bkgd_crc_net), 1, dst);
		}
		
		fwrite(&size_net, sizeof(size_net), 1, dst);
		fwrite(name, 4, 1, dst);
		copy_bytes(dst, src, size);
		
		if (fread(&crc_net, sizeof(crc_net), 1, src) < 1)
			break;
		fwrite(&crc_net, sizeof(crc_net), 1, dst);
		from_net32(crc, crc_net);
		
		
		// printf("  %s size: %-4zd (%-2d %-2d %-2d %-2d), crc: %u\n", name, size, size_net.a, size_net.b, size_net.c, size_net.d, crc);
		printf("  %s size: %6zd, crc: %10lu\n", name, size, crc);
	}
	
	return 0;
}


#define BUF_SIZE (4 * 1024)
static size_t
copy_bytes (FILE *dst, FILE *src,  size_t n)
{
	static u_char *buf = NULL;
	if (buf == NULL)
	{
		buf = malloc(BUF_SIZE);
		if (buf == NULL)
			return 0;
	}
	
	size_t total = n, copied;
	
	while (n > BUF_SIZE)
	{
		if ((copied = fread(buf, 1, BUF_SIZE, src)) < 1)
			goto DONE;
		fwrite(buf, 1, BUF_SIZE, dst);
		
		n -= copied;
	}
	
	if ((copied = fread(buf, 1, n, src)) < 1)
		goto DONE;
	fwrite(buf, 1, n, dst);
	
	DONE:
	return total - n;
}



// CRC implementation taken from w3c

/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void
make_crc_table (void)
{
	unsigned long c;
	int n, k;
	
	for (n = 0; n < 256; n++)
	{
		c = (unsigned long) n;
		for (k = 0; k < 8; k++)
		{
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below). */

static unsigned long
update_crc (unsigned long crc, unsigned char *buf, int len)
{
	unsigned long c = crc;
	int n;
	
	if (!crc_table_computed)
		make_crc_table();
	for (n = 0; n < len; n++)
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	
	return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long
compute_crc (void *buf, int len)
{
	return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}


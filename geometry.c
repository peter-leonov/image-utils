// thank Michael Petrov (http://michaelpetrov.com/)
// for this post http://www.64lines.com/jpeg-width-height
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

static char *RC_OK = "ok";
static char *RC_NEXT = "next";

char *
try_jpeg (u_char *data, size_t data_length, u_int *width, u_int *height)
{
	u_char type;
	size_t i, block_length;
	
	i = 0;
	
	if (data[i] != 0xff || data[i+1] != 0xd8)
	{
		return RC_NEXT;
	}
	
	i += 4;
	
	block_length = (data[i] << 8) + data[i+1];
	
	for (;;)
	{
		// printf("%ld: %ld\n", i, block_length);
		
		i += block_length;
		if (i + 4 >= data_length)
		{
			return "file is truncated";
		}
		
		if (data[i] != 0xff)
		{
			return "block has broken 0xFF mark";
		}
		
		type = data[i+1];
		
		if (type == 0xc0 || type == 0xc2)
		{
			if (i + 8 >= data_length)
			{
				return "file is truncated";
			}
			
			*height = (data[i+5] << 8) + data[i+6];
			*width = (data[i+7] << 8) + data[i+8];
			return RC_OK;
		}
		
		if (type == 0xd9)
		{
			break;
		}
		
		i += 2;
		
		block_length = (data[i] << 8) + data[i+1];
	}
	
	return "file is totally truncated";
}

int
process (char const *srcfn)
{
	int fd;
	char *rc;
	u_char *data;
	size_t data_length;
	struct stat fs;
	u_int height, width;
	
	fd = open(srcfn,  O_RDONLY);
	if (fd == -1)
	{
		perror("ERROR: Can't open the file for reading");
		return 1;
	}
	
	fstat(fd, &fs);
	
	data_length = fs.st_size;
	
	data = mmap(0, data_length, PROT_READ, MAP_SHARED, fd, 0);
	
	rc = try_jpeg(data, data_length, &width, &height);
	if (rc == RC_OK)
	{
		printf("%dx%d\n", width, height);
		return 0;
	}
	if (rc != RC_NEXT)
	{
		fprintf(stderr, "ERROR: %s\n", rc);
	}
	
	fprintf(stderr, "ERROR: File format is neither jpeg nor png.\n");
	
	return 6;
}

int main (int argc, char const *argv[])
{
	if (argc != 2)
	{
		printf("%s\n", "Usage:\n  jfifwh image.jpg");
		return 1;
	}
	
	return process(argv[1]);
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int
process (char const *srcfn)
{
	int fd;
	u_char *data, type;
	size_t data_length, i, block_length;
	u_int height, width;
	struct stat fs;
	
	fd = open(srcfn,  O_RDONLY);
	if (fd == -1)
	{
		perror("ERROR: Can't open the file for reading");
		return 2;
	}
	
	fstat(fd, &fs);
	
	data_length = fs.st_size;
	
	data = mmap(0, data_length, PROT_READ, MAP_SHARED, fd, 0);
	
	i = 0;
	
	if (data[i] != 0xff || data[i+1] != 0xd8)
	{
		fprintf(stderr, "ERROR: File has no proper Start Of Image block\n");
		return 3;
	}
	
	i += 4;
	
	block_length = (data[i] << 8) + data[i+1];
	
	for (;;)
	{
		// printf("%ld: %ld\n", i, block_length);
		
		i += block_length;
		if (i + 4 >= data_length)
		{
			fprintf(stderr, "ERROR: File is truncated\n");
			return 4;
		}
		
		if (data[i] != 0xff)
		{
			fprintf(stderr, "ERROR: Block has no 0xFF mark at %ld\n", i);
			return 5;
		}
		
		type = data[i+1];
		
		if (type == 0xc0 || type == 0xc2)
		{
			height = (data[i+5] << 8) + data[i+6];
			width = (data[i+7] << 8) + data[i+8];
			printf("%dx%d\n", width, height);
			return 0;
		}
		
		if (type == 0xd9)
		{
			break;
		}
		
		i += 2;
		
		block_length = (data[i] << 8) + data[i+1];
	}
	
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

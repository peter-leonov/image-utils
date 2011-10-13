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
	u_char *data;
	size_t data_length;
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
	
	if (((int32_t *) data)[0] != 0xe0ffd8ff)
	{
		fprintf(stderr, "ERROR: File has no JFIF for header\n");
		return 3;
	}
	
	return 0;
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

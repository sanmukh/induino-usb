#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

const char* dev="/dev/usbinduino0";

void send_cmd(int fd,char* cmd, int size)
{
	int retval;
	retval = write(fd,cmd,size);
	if (retval < 0)
		perror("Could not write");
}

void usage()
{
	printf("induino_write data\n");
}

int main(int argc, char* argv[])
{
	int fd;
	if (argc < 2) {
		usage();
		exit(1);
	}

	
	fd = open(dev, O_RDWR);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	send_cmd(fd, argv[1], strlen(argv[1]) + 1);
	printf("size of argv 1: %zu", strlen(argv[1]));
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

int lcd;
#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_SET _IOW(SCULL_IOC_MAGIC, 2, char *)
#define SCULL_READ _IOR(SCULL_IOC_MAGIC, 3, char *)
#define SCULL_READ_WRITE _IOWR(SCULL_IOC_MAGIC, 4, char *)

void test()
{
	int k, i, sum;
	char s[3],  user_msg[20];
	char *msg_to_copy = "User message!";

	memset(s, '2', sizeof(s));
	printf("test begin!\n");

	//k = write(lcd, s, sizeof(s));
	//printf("written = %d\n", k);
	
/*	
	printf("write hello test msg to kernel log\n");
	k = ioctl(lcd, SCULL_HELLO);
	printf("result = %d\n\n", k);


	// test writing using ioctl
	printf("ioctl write 'User message!' to device\n");
	k = ioctl(lcd, SCULL_SET, msg_to_copy);
	printf("result = %d\n\n", k);

	// test reading using ioctl
	printf("ioctl read from device\n");
	k = ioctl(lcd, SCULL_READ, user_msg);
	printf("result = %d\n", k);
	printf("user_msg = %s\n", user_msg);
*/
	
	// test read-write using ioctl
	strcpy(user_msg, msg_to_copy);
	printf("ioctl read-write from device\n");
	printf("user_msg = %s\n", user_msg);
	k = ioctl(lcd, SCULL_READ_WRITE, user_msg);
	printf("result = %d\n", k);
	printf("user_msg = %s\n", user_msg);

}

int main(int argc, char **argv)
{
	lcd = open("/dev/scull", O_RDWR);
	if (lcd == -1) {
		perror("unable to open lcd");
		exit(EXIT_FAILURE);
	}
	
	test();
	
	close(lcd);
	return 0;
}

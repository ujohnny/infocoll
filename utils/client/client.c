#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define NETLINK_INFOCOLL 31

/*
  1 bit - type
  8 bits - time in seconds
  8 bits - nanoseconds
  40 bits - payload
*/
#define PAYLOAD_SIZE 57 

uint64_t extract_uint64(unsigned char *str)
{
	uint64_t v = 0;
	int i;
	for (i = 0; i < 8; ++i) {
		v |= ((uint64_t) str[7-i]) << (i*8);
	}
	return v;
}

void process_data(struct nlmsghdr *nlh, FILE *fp) {
	unsigned char *payload = NLMSG_DATA(nlh);
	unsigned char type = payload[0];
	uint64_t time_sec = extract_uint64(payload+1),
		time_nsec = extract_uint64(payload+9),
		f1 = extract_uint64(payload+17),
		f2 = extract_uint64(payload+25),
		f3 = extract_uint64(payload+33),
		f4 = extract_uint64(payload+41),
		f5 = extract_uint64(payload+49);
	printf("%d\t" // type
			"%llu.%llu\t" // time
			"%llu\t" // f1
			"%llu\t" // f2
			"%llu\t" // f3
			"%llu\t" // f4
			"%llu\t\n" // f5
		   , type, time_sec, time_nsec, f1, f2, f3, f4, f5);
	fwrite(payload, 1, PAYLOAD_SIZE, fp);
	memset(payload, 0, PAYLOAD_SIZE);
}

int main(int argc, char **argv)
{

	struct sockaddr_nl src_addr;
	struct sockaddr_nl dst_addr;
	struct nlmsghdr *nlh; 
	struct iovec iov;
	struct msghdr msg;

	FILE *fp;
	int sock_fd;

	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		printf("./client <file>\n"
				"\tYou can specify file to write binary dump\n");
		return 0;
	}

	fp = argc == 2 ? fopen(argv[1], "wb") : NULL;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_INFOCOLL);
	if (sock_fd < 0) {
		printf("Device not mounted\n");
		return -1;
	}


	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); // self pid 

	bind(sock_fd, (struct sockaddr *) &src_addr, sizeof(src_addr));

	memset(&dst_addr, 0, sizeof(dst_addr));
	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0; // For Linux Kernel
	dst_addr.nl_groups = 0; // unicast 
	
	nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(PAYLOAD_SIZE));
	memset(nlh, 0, NLMSG_SPACE(PAYLOAD_SIZE));
	nlh->nlmsg_len = NLMSG_SPACE(PAYLOAD_SIZE);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	iov.iov_base = (void *) nlh;
	iov.iov_len = nlh->nlmsg_len;
		
	msg.msg_name = (void *) &dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(sock_fd, &msg, 0); // send msg to kernel with our pid 

	printf("type\t"
		   "time\t"
		   "info_1\t"
		   "info_2\t"
		   "info_3\t"
		   "info_4\t"
		   "info_5\n"); //header for output file

	/* Reading messages from kernel */

	do {
		recvmsg(sock_fd, &msg, 0);
		process_data(nlh, fp);
	} while (!(nlh->nlmsg_type == NLMSG_ERROR));
	
	close(sock_fd);

	if (fp) {
		fclose(fp);
	}

	return 0;
}

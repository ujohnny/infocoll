#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define NETLINK_INFOCOLL 31
#define MAX_PAYLOAD 41 /* maximum payload size*/

uint64_t extract_uint64(unsigned char *str)
{
	uint64_t v = 0;
	int i;
	for (i = 0; i < 8; ++i) {
		v |= ((uint64_t) str[7-i]) << (i*8);
	}
	return v;
}

void process_data(struct nlmsghdr *nlh) {
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
			"%llu\t\n", // f5
		   type, time_sec, time_nsec, f1, f2, f3, f4, f5);
	memset(payload, 0, MAX_PAYLOAD);
}

int main()
{
	int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_INFOCOLL);
	if(sock_fd < 0) {
		printf("Device not mounted\n");
		return -1;
	}

	struct sockaddr_nl src_addr;
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); // self pid 

	bind(sock_fd, (struct sockaddr*) &src_addr, sizeof(src_addr));

	struct sockaddr_nl dst_addr;
	memset(&dst_addr, 0, sizeof(dst_addr));
	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0; // For Linux Kernel
	dst_addr.nl_groups = 0; // unicast 
	

	struct nlmsghdr *nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	struct iovec iov;
	iov.iov_base = (void *) nlh;
	iov.iov_len = nlh->nlmsg_len;

	struct msghdr msg;
		
	msg.msg_name = (void *) &dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(sock_fd, &msg, 0); // send msg to kernel with our pid 

	printf("type\tlength\toffset\tinode\ttime\n"); //header for output file

	/* Reading messages from kernel */

	do {
		recvmsg(sock_fd, &msg, 0);
		process_data(nlh);
	} while (!(nlh->nlmsg_type == NLMSG_ERROR));
	
	close(sock_fd);
}

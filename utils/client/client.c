#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define NETLINK_INFOCOLL 31
#define MAX_PAYLOAD 41 /* maximum payload size*/

uint64_t extract_uint64(char *str)
{
	uint64_t v = 0;
	int i;
	for (i = 0; i < 8; ++i) {
		v |= ((uint64_t) str[7-i]) << (i*8);
	}
	return v;
}

void process_data(struct nlmsghdr *nlh) {
	char *payload = NLMSG_DATA(nlh);
	char type = payload[0];
	uint64_t length = extract_uint64(payload+1),
		offset = extract_uint64(payload+9),
		inode = extract_uint64(payload+17),
		time_sec = extract_uint64(payload+25),
		time_nsec = extract_uint64(payload+33); //TODO: fix incorrect nsec output
	printf("Rcvd msg: {type = %d, length = %llu, offset = %llu, inode = %llu, time = %llu.%llu}\n",
		   type, length, offset, inode, time_sec, time_nsec);
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
	src_addr.nl_pid = getpid(); /* self pid */

	bind(sock_fd, (struct sockaddr*) &src_addr, sizeof(src_addr));

	struct sockaddr_nl dst_addr;
	memset(&dst_addr, 0, sizeof(dst_addr));
	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0; /* For Linux Kernel */
	dst_addr.nl_groups = 0; /* unicast */
	

	struct nlmsghdr *nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	
	strcpy(NLMSG_DATA(nlh), "llambda!");

	struct iovec iov;
	iov.iov_base = (void *) nlh;
	iov.iov_len = nlh->nlmsg_len;

	struct msghdr msg;
		
	msg.msg_name = (void *) &dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	printf("Sending message to kernel\n");
	sendmsg(sock_fd, &msg, 0);
	printf("Waiting for message from kernel\n");

	/* Read message from kernel */

	do {
		recvmsg(sock_fd, &msg, 0);
		process_data(nlh);
	} while (!(nlh->nlmsg_type == NLMSG_ERROR));
	
	close(sock_fd);
}

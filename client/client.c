#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NETLINK_INFOCOLL 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/

int main()
{

	int sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_INFOCOLL);
	if(sock_fd<0)
		return -1;
	
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
	
	strcpy(NLMSG_DATA(nlh), "Hello");

	struct iovec iov;
	iov.iov_base = (void *) nlh;
	iov.iov_len = nlh->nlmsg_len;

	struct msghdr msg;
		
	msg.msg_name = (void *)&dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	printf("Sending message to kernel\n");
	sendmsg(sock_fd, &msg, 0);
	printf("Waiting for message from kernel\n");

	/* Read message from kernel */

	do {
		recvmsg(sock_fd, &msg, 0);
		printf("Rcvd msg: %s\n", NLMSG_DATA(nlh));
	} while (!nlh->nlmsgtype == NLMSG_ERROR) {
	
	close(sock_fd);
}

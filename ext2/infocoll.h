#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>

struct infocoll_datatype {
	struct sock *socket;
	int pid;
};

extern struct infocoll_datatype infocoll_data;

int infocoll_send_string(char *msg, int status)
{
	int msg_size = strlen(msg);
	
	struct sk_buff *skb_out = nlmsg_new(msg_size,0);

	struct nlmsghdr *nlh = nlmsg_put(skb_out, 0, 0, status, msg_size, 0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh), msg, msg_size);
	return nlmsg_unicast(infocoll_data.socket, skb_out, infocoll_data.pid);
}

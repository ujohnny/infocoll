#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/time.h>

struct infocoll_datatype {
	struct sock *socket;
	void *fs;
	int pid;
};

extern struct infocoll_datatype infocoll_data;

static DEFINE_SPINLOCK(infocoll_lock);

static int infocoll_send_string(char *str, int status) {
	if (infocoll_data.socket == NULL) {
		return -1;
	}
	
	spin_lock(&infocoll_lock);

	struct timespec time = CURRENT_TIME;
	char time_str[50];
	sprintf(time_str, "[ %lu.%lu ] ", time.tv_sec, time.tv_nsec);

	size_t str_len = strlen(str),
		time_len = strlen(time_str);

	char *msg = kmalloc(str_len + time_len + 1, GFP_ATOMIC);
	
	strcat(msg, time_str);
	strcat(msg, str);

	int msg_size = strlen(msg);

	struct sk_buff *skb_out = nlmsg_new(msg_size,0);

	struct nlmsghdr *nlh = nlmsg_put(skb_out, 0, 0, status, msg_size, 0);  
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh), msg, msg_size);

	int res =  nlmsg_unicast(infocoll_data.socket, skb_out, infocoll_data.pid);
	kfree(msg);
	spin_unlock(&infocoll_lock);
	return res;
}


static void infocoll_sock_init_callback(struct sk_buff *skb) {
	char *msg = "Hello from kernel";
	int msg_size = strlen(msg);
	
	printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

	struct nlmsghdr *nlh = (struct nlmsghdr*) skb->data;
	printk(KERN_INFO "Netlink received msg payload: %s\n",(char*) nlmsg_data(nlh));
	int pid = infocoll_data.pid = nlh->nlmsg_pid; /*pid of sending process */

	struct sk_buff *skb_out = nlmsg_new(msg_size,0);

	if (!skb_out) {
		printk(KERN_ERR "Failed to allocate new skb\n");
		return;
	}


	int res = infocoll_send_string(msg, NLMSG_DONE);

	if (res < 0) {
	    printk(KERN_INFO "Error while sending bak to user\n");
	}
}

static void infocoll_close_socket() {
	if (infocoll_data.socket == NULL) {
		return;
	}

	char *msg = "Goodbye from kernel";

	infocoll_send_string(msg, NLMSG_ERROR);

	netlink_kernel_release(infocoll_data.socket);
	infocoll_data.pid = -1;
	infocoll_data.socket = NULL;
}

static void strshift(char *s, size_t offset) {
	do {
		*s = s[offset];
	} while (*s++ != '\0');
}

static void infocoll_init_socket(char *data)
{
	if (!data) return;

	char *opt = "infocoll";
	const int nl_socket_id = 31;

	char *pos = strstr(data, opt),
		*s = pos;
	if (!pos) return;

	strshift(s, strlen(opt));

	if (pos == data && pos[0] == ',') {
		// removing leading comma
		strshift(pos, 1);
	} else if (pos[-1] == ',') {
		if (pos[0] == '\0') {
			// removing tailing comma
			pos[-1] = 0;
		} else if (pos[0] == ',') {
			// replacing two commas with one
			strshift(pos, 1);
		}
	}

	infocoll_data.socket = netlink_kernel_create(&init_net, nl_socket_id, 0, infocoll_sock_init_callback, NULL, THIS_MODULE);

	// TODO: remove this code
	if (!infocoll_data.socket) {
		printk(KERN_ALERT "Error creating socket.\n");
	} else {
		printk(KERN_INFO "Socket created successful.\n");
	}
}

static int infocoll_connected() {
	return (infocoll_data.socket == NULL) ? 0 : 1;
}

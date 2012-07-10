#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/time.h>

#define INFOCOLL_INIT 0
#define INFOCOLL_READ 1
#define INFOCOLL_WRITE 2
#define INFOCOLL_CLOSE 3

struct infocoll_datatype {
	struct sock *socket;
	void *fs;
	int pid;
};


extern struct infocoll_datatype infocoll_data;

// static DEFINE_SPINLOCK(infocoll_lock);

static void infocoll_write_to_buff(char *buff, uint64_t val)
{
	int i;
	for (i = 0; i < 8; ++i) {
		buff[7 - i] = val & 0xFF;
		val >>= 8;
	}
}

/*
 * infocoll_send sends a message to the client
 * This message has this structure:
 * struct infocoll_message {
 *   char type;             // type of action: INFOCOLL_INIT, INFOCOLL_READ,
 *                             INFOCOLL_WRITE or INFOCOLL_CLOSE
 *   uint64_t offset;       // action offset
 *   uint64_t length;       // number of bytes affected
 *   uint64_t inode;        // inode number
 *   uint64_t time_sec;     // seconds
 *   uint64_t time_nsec;    // nanoseconds
 * };
 */
static int infocoll_send(char type, ulong inode, size_t length
	, loff_t offset, int status)
{
	if (infocoll_data.socket == NULL) {
		return -1;
	}
	
//	spin_lock(&infocoll_lock);

	struct timespec time;
	getrawmonotonic(&time);
	long time_sec = time.tv_sec;
	long time_nsec = time.tv_nsec;

	const int size = 41; // 41 = 4*8 + 1

	struct sk_buff *skb_out = nlmsg_new(size, 0);
	struct nlmsghdr *nlh = nlmsg_put(skb_out, 0, 0, status, size, 0);  
	NETLINK_CB(skb_out).dst_group = 0;  /* unicast */

	char *payload = NLMSG_DATA(nlh);
	payload[0] = type;
	infocoll_write_to_buff(payload + 1, offset);
	infocoll_write_to_buff(payload + 9, length);
	infocoll_write_to_buff(payload + 17, inode);
	infocoll_write_to_buff(payload + 25, time_sec);
	infocoll_write_to_buff(payload + 33, time_nsec);

	int res =  nlmsg_unicast(infocoll_data.socket, skb_out, infocoll_data.pid);
//	spin_unlock(&infocoll_lock);
	return res;
}


static void infocoll_sock_init_callback(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = (struct nlmsghdr*) skb->data;
	printk(KERN_INFO "[ INFOCOLL ] Netlink received msg payload: %s\n",(char*) nlmsg_data(nlh));
	infocoll_data.pid = nlh->nlmsg_pid; /*pid of sending process */

	infocoll_send(INFOCOLL_INIT, 0, 0, 0, NLMSG_DONE);
}

static void infocoll_close_socket()
{
	if (infocoll_data.socket == NULL) {
		return;
	}

	infocoll_send(INFOCOLL_CLOSE, 0, 0, 0, NLMSG_ERROR);

	netlink_kernel_release(infocoll_data.socket);
	infocoll_data.pid = -1;
	infocoll_data.socket = NULL;
}

static void strshift(char *s, size_t offset)
{
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
	if (infocoll_data.socket) {
		printk(KERN_INFO "Infocoll started successful.\n");
	} else {
		printk(KERN_ALERT "Error while starting infocoll.\n");
	}
}

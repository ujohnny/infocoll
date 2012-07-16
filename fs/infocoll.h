#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/time.h>

#define INFOCOLL_BEGIN  0
#define INFOCOLL_END    1
#define INFOCOLL_OPEN   2
#define INFOCOLL_READ   3
#define INFOCOLL_WRITE  4
#define INFOCOLL_LSEEK  5
#define INFOCOLL_FSYNC  6
#define INFOCOLL_MKNOD  7
#define INFOCOLL_UNLINK 8
#define INFOCOLL_ALLOCATE 9
#define INFOCOLL_TRUNCATE 10
#define INFOCOLL_CLOSE  11

struct infocoll_datatype {
	struct sock *socket;
	void *fs;
	int pid;
};


extern struct infocoll_datatype infocoll_data;

static void infocoll_write_to_buff(unsigned char *buff, uint64_t val)
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
 *   char type;           // action type: INFOCOLL_INIT, INFOCOLL_READ, etc
 *   uint64_t time_sec;   // seconds
 *   uint64_t time_nsec;  // nanoseconds
 *   char[40] data;       // custom data
 * };
 */
static int infocoll_send(char type, char *data, int status)
{
	struct sk_buff *skb_out;
	struct nlmsghdr *nlh;
	unsigned char *payload;
	struct timespec time;
	const int size = 57; // 41 = 1 + 2*8 + 40

	if (infocoll_data.socket == NULL) {
		return -1;
	}
	
	getrawmonotonic(&time);

	skb_out = nlmsg_new(size, 0);
	nlh = nlmsg_put(skb_out, 0, 0, status, size, 0);  
	NETLINK_CB(skb_out).dst_group = 0;  /* unicast */

	payload = NLMSG_DATA(nlh);
	memset(payload, 0, size);
	payload[0] = type;
	infocoll_write_to_buff(payload + 1, time.tv_sec);
	infocoll_write_to_buff(payload + 9, time.tv_nsec);
	if (data) memcpy(payload+17, data, 40);

	return nlmsg_unicast(infocoll_data.socket, skb_out, infocoll_data.pid);
}


static void infocoll_sock_init_callback(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = (struct nlmsghdr *) skb->data;
	printk(KERN_INFO "[ INFOCOLL ] Netlink received msg payload: %s\n",(char *) nlmsg_data(nlh));
	infocoll_data.pid = nlh->nlmsg_pid; /*pid of sending process */

	infocoll_send(INFOCOLL_BEGIN, 0, NLMSG_DONE);
}

static void strshift(char *s, size_t offset)
{
	do {
		*s = s[offset];
	} while (*s++ != '\0');
}

static void infocoll_init_socket(char *data)
{
	char *pos, *s;
	char *opt = "infocoll";
	const int nl_socket_id = 31;

	if (!data) return;

	pos = strstr(data, opt);
	s = pos;
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

static void infocoll_close_socket(void)
{
	if (infocoll_data.socket == NULL) {
		return;
	}

	infocoll_send(INFOCOLL_END, 0, NLMSG_ERROR);

	netlink_kernel_release(infocoll_data.socket);
	infocoll_data.pid = -1;
	infocoll_data.socket = NULL;
}

#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#define NETLINK_INFOCOLL 31

/**
 * 1 bit - type
 * 8 bits - time in seconds
 * 8 bits - nanoseconds
 * 40 bits - payload
 */
#define PAYLOAD_SIZE 57 

/**
 * define file formats
 */
#define BINARY 1
#define TEXT 2

// moved here because of strange troubles with stack if they're placed in function
struct sockaddr_nl src_addr;
struct sockaddr_nl dst_addr;
struct nlmsghdr *nlh; 
struct iovec iov;
struct msghdr msg;

FILE *fp, *f_in, *f_out;

void sigint_handler(int signum) {
	if (fp)
		fclose(fp);
	if (f_in)
		fclose(f_in);
	if (f_out)
		fclose(f_out);
	exit(EXIT_SUCCESS);
}

/**
 * extract_uint64 - extract uint64_t from 8-bit char sequence
 * @str: input sequence
 */
uint64_t extract_uint64(unsigned char *str)
{
	uint64_t v = 0;
	int i;
	for (i = 0; i < 8; ++i) {
		v |= ((uint64_t) str[7-i]) << (i*8);
	}
	return v;
}

/**
 * convert_data - convert infocoll messages to string
 * @payload: input infocoll message
 * Wrapper for extract_uint64
 */
int convert_data(char *payload, uint64_t *time_sec, uint64_t *time_nsec, uint64_t *f1
				 , uint64_t *f2, uint64_t *f3, uint64_t *f4, uint64_t *f5) 
{
	*time_sec = extract_uint64(payload+1);
	*time_nsec = extract_uint64(payload+9);
	*f1 = extract_uint64(payload+17);
	*f2 = extract_uint64(payload+25);
	*f3 = extract_uint64(payload+33);
	*f4 = extract_uint64(payload+41);
	*f5 = extract_uint64(payload+49);
	return 0;
}

/**
 * convert_file - converts file with binary data to text
 * @f_in: input binary file
 * @f_out: output text file
 */
int convert_file(FILE *f_in, FILE *f_out) {
	char buffer[PAYLOAD_SIZE];

	print_header(f_out);

	while(fread(buffer, 1, PAYLOAD_SIZE, f_in)) {
		write_data_to_file(buffer, f_out, TEXT);
	}
	return 0;
}

/**
 * print_header - prints header into file for next processiong with R
 */
int print_header(FILE *file) {
	fprintf(file, "type\t"
			"time\t"
			"info_1\t"
			"info_2\t"
			"info_3\t"
			"info_4\t"
			"info_5\n");

}

/**
 * write_data_to_file - writes data in file with certain format
 * @payload: infocoll message
 */
int write_data_to_file(char *payload, FILE *fp, int file_format) {
	if (file_format == TEXT) { 
		unsigned char type = payload[0];

		uint64_t time_sec, time_nsec
			, f1, f2, f3, f4, f5;
		convert_data(payload, &time_sec, &time_nsec, &f1, &f2, &f3, &f4, &f5);

		fprintf(fp, "%d\t" // type
			   "%llu.%09llu\t" // time
			   "%llu\t" // f1
			   "%llu\t" // f2
			   "%llu\t" // f3
			   "%llu\t" // f4
			   "%llu\t\n" // f5
			   , type, time_sec, time_nsec, f1, f2, f3, f4, f5);
	}
	
	if (file_format == BINARY) {
		fwrite(payload, 1, PAYLOAD_SIZE, fp);
	}

	fflush(fp);
}

/**
 * extract_data_and_write - wrapper for write_data_to_file
 * @nlh: pointer to header of netlink message
 */
int extract_data_and_write(struct nlmsghdr *nlh, FILE *fp, int file_format) {

	unsigned char *payload = NLMSG_DATA(nlh);
	write_data_to_file(payload, fp, file_format);

	memset(payload, 0, PAYLOAD_SIZE);
	return 0;
}

/**
 * print_help - prints help if client runned with -h flag
 */
int print_help() {
	
	printf("./client [FLAG] [FILE(s)]\n");
	printf("flags:\n");
	printf("\t-c [INPUT_FILE] [OUTPUT_FILE]\n"
		   "\t\t convert from binary data to text\n");
	printf("\t-b [OUTPUT_FILE]\n"
		   "\t\tprint output data in binary format\n");
	printf("\t-t [OUTPUT_FILE]\n"
		   "\t\tprint output data in text format\n");
	return 0;

}

/**
 * start_logging - interaction with kernel
 * @fp: output file
 * @file_format: binary or text format
 * Connects to kernel, and receiving messages until NLMSG_ERROR received
 */
int start_logging(FILE* fp, int file_format) {

	int sock_fd;

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

	if (file_format == TEXT) {
		print_header(fp);
	}

	/* Reading messages from kernel */

	do {
		recvmsg(sock_fd, &msg, 0);
		extract_data_and_write(nlh, fp, file_format);
	} while (!(nlh->nlmsg_type == NLMSG_ERROR));
	
	close(sock_fd);
}

int main(int argc, char **argv)
{
	signal(SIGINT, sigint_handler);
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		print_help();
		return 0;
	}

	if (argc == 3) {
		int file_format;
		if (strcmp(argv[1], "-b") == 0) {
			fp = fopen(argv[2], "wb");
			file_format = BINARY;
		} else if (strcmp(argv[1], "-t") == 0) {
			fp = fopen(argv[2], "w");
			file_format = TEXT;
		} else {
			printf("incorrect option, use -h for help\n");
			return -1; 
		}
		
		if (!fp) {
			printf("Error opening file\n");
			return -2;
		}
		
		start_logging(fp, file_format);
		close(fp);
		return 0;
	} 

	if (argc == 4 && strcmp(argv[1], "-c") == 0) {
		f_in = fopen(argv[2], "rb");
		f_out = fopen(argv[3], "w");
		
		if (!(f_in && f_out)) {
			printf("Error opening file\n");
			return -3;
		}

		convert_file(f_in, f_out);
		
		fclose(f_in);
		fclose(f_out);
		return 0;
	} 
	
	printf("incorrect option, use -h for help\n");
	return -1;
}

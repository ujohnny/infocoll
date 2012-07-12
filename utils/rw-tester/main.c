#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
	if (argc == 1) {
		printf("Use: ./rw-tester <file> <operation> <offset> <length|string> <print>\n"
				"Where\n"
				"\tfile — path to the file to read from / write to\n"
				"\toperation — r read, rs sys_read or w write, ws sys_write\n"
				"\toffset — data offset\n"
				"\tlength — data length (will be generated randomly)\n"
				"\tstring — string to be written (for write mode only!)\n"
				"\tprint  — print data in ascii format (for read mode only)\n");
		return 0;
	}

	srand ( time(NULL) );

	char *fileName = argv[1];
	int isRead = strcmp(argv[2], "r") == 0,
		isSysRead = strcmp(argv[2], "rs") == 0,
		isWrite = strcmp(argv[2], "w") == 0,
		isSysWrite = strcmp(argv[2], "ws") == 0,
		printAscii = argc == 6;
	int offset = 0,
		length = -1;

	sscanf(argv[3], "%d", &offset);
	sscanf(argv[4], "%d", &length);

	if (isRead || isSysRead) {
		printf("Reading %d bytes from %s starting at %d\n", length, fileName, offset);
		char *buff = (char *) malloc(length + 1);

		if (isRead) {
			FILE *f = fopen(fileName, "rb");
			fseek(f, offset, SEEK_SET);
			fread(buff, 1, length, f);
			fclose(f);
		} else {	
			int file_desc = open(fileName, O_CREAT | O_RDWR);
			lseek(file_desc, offset, SEEK_SET);
			read(file_desc, buff, length);
			close(file_desc);
		}
		
		buff[length] = 0;
		if (printAscii) printf("%s\n", buff);
		free(buff);
	   
	}

	if (isWrite || isSysWrite) {
		char *buff;
		if (length == -1) {
			buff = argv[4];
			length = strlen(buff);
			printf("Writing '%s' to %s starting at %d\n", buff, fileName, offset);
		} else {
			buff = (char *) malloc(length);
			for (int i = 0; i < length; ++i) buff[i] = rand();
			printf("Writing %d bytes to %s starting at %d\n", length, fileName, offset);
		}

		if (isWrite) {
			FILE *f = fopen(fileName, "r+b");
			fseek(f, offset, SEEK_SET);
			fwrite(buff, 1, length, f);
			fclose(f);
		} else { 		
			int file_desc = open(fileName, O_CREAT | O_RDWR);
			lseek(file_desc, offset, SEEK_SET);
			write(file_desc, buff, length);
			close(file_desc);
		}
	}

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
	if (argc == 1) {
		printf("Use: ./rw-tester <file> <operation> <offset> <length|string> <print>\n"
				"Where\n"
				"\tfile — path to the file to read from / write to\n"
				"\toperation — r for read or w for write\n"
				"\toffset — data offset\n"
				"\tlength — data length (will be generated randomly)\n"
				"\tstring — string to be written (for write mode only!)\n"
				"\tprint  — print data in ascii format (for read mode only)\n");
		return 0;
	}

	srand ( time(NULL) );

	char *fileName = argv[1];
	int isRead = strcmp(argv[2], "r") == 0,
		isWrite = !isRead,
		printAscii = argc == 6;
	int offset = 0,
		length = -1;

	sscanf(argv[3], "%d", &offset);
	sscanf(argv[4], "%d", &length);

	if (isRead) {
		printf("Reading %d bytes from %s starting at %d\n", length, fileName, offset);

		FILE *f = fopen(fileName, "rb");
		fseek(f, offset, SEEK_SET);
		char *buff = (char *) malloc(length + 1);
		fread(buff, 1, length, f);
		buff[length] = 0;
		if (printAscii) printf("%s\n", buff);
		free(buff);
		fclose(f);
	}

	if (isWrite) {
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


		FILE *f = fopen(fileName, "r+b");
		fseek(f, offset, SEEK_SET);
		fwrite(buff, 1, length, f);
		fclose(f);
	}

	return 0;
}

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define THROW_ERR(msg) \
       syslog(LOG_ERR, msg);\
       exit(EXIT_FAILURE)

int main(int argc, char* argv[])
{
	openlog("writer.o", 0, LOG_USER);
	if (argc < 3) {
		THROW_ERR("Usage error: args: fname ftext\n");
	}

	char* fname = argv[1];
	char* ftext = argv[2];

	int fd;
	ssize_t nr;

	fd = creat(fname, 0644);
	if (fd == -1) {
		THROW_ERR("create error\n");
	}

	nr = write(fd, ftext, strlen(ftext));
	if (nr == -1) {
		THROW_ERR("write error\n");
	}
	closelog();
}


#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "read_line.h"

#define BUF_SIZE 32768 
#define FNAME "/var/tmp/aesdsocketdata"

struct files {
	int count;
	int* descriptors;
};

void* handleRequest(void* p)
{
	struct files* f = (struct files*)p;
	int fd;
	ssize_t nr, count;
	FILE *fp;
	char *buf = (char *) malloc(BUF_SIZE);
	if (buf == NULL) {
		syslog(LOG_ERR, "Failed to allocate memory for buf");
		exit(EXIT_FAILURE);
	}
	memset(buf, 0, BUF_SIZE);

	nr = readLine(f->descriptors[0], buf, BUF_SIZE);
	if (nr == -1) {
		syslog(LOG_ERR, "Failed to recieve.");
		exit(EXIT_FAILURE);
	}

	count = strlen(buf);
	fd = open(FNAME, O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		syslog(LOG_ERR, "Failed to open %s: %s", FNAME, strerror(errno));
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "Opened file %s", FNAME);
	nr = write(fd, buf, count);
	if (nr == -1) {
		syslog(LOG_ERR, "Failed to write %s to %s: %s", 
				buf, FNAME, strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if (nr != count) {
		syslog(LOG_ERR, "Possible failed write.");
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "Wrote msg to %s", FNAME);
	close(fd);

	fp = fopen(FNAME, "r");
	memset(buf, 0, BUF_SIZE);
	count = 0;
	while (fgets(buf, BUF_SIZE, fp) != NULL) {
		send(f->descriptors[0], buf, strlen(buf), 0); 
		syslog(LOG_INFO, "Sent %ld %s",
				count++, buf);
	}
	fclose(fp);

	free(buf);
	return NULL;
}

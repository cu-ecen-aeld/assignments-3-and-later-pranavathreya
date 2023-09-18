#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
	if (argc != 3) {
		syslog(LOG_ERR, "Usage: writer file text\n");
		exit(EXIT_FAILURE);
	}
	int fd;
	ssize_t nr;
	openlog(NULL, LOG_CONS, LOG_USER);

	fd = creat(argv[1], (O_RDWR | O_TRUNC));
	if (fd == -1) {
		syslog((LOG_CONS | LOG_ERR), "Error opening file: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	syslog((LOG_USER | LOG_DEBUG), "Writing %s to %s\n", argv[2], argv[1]);
	nr = write(fd, argv[2], strlen(argv[2]));
	chmod(argv[1], (S_IRUSR | S_IWUSR | S_IXUSR));  
	if (nr != strlen(argv[2])) {
		syslog(LOG_ERR, "Error writing bytes: %zd / %zd\n", nr, strlen(argv[2]));
		exit(EXIT_FAILURE);
	}
}

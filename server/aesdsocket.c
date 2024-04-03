#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include "read_line.h"

#define BUF_SIZE 32768 

typedef struct NameInfo
{
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];
} NameInfo;

void getAddressInfo(char* host, char* port,
		struct addrinfo** result)
{	
	int s;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	s = getaddrinfo(host, port, &hints,
			result);
	if (s != 0)
	{
		syslog(LOG_ERR, "getAddressInfo: getaddrinfo failed: %s\n",
			gai_strerror(s));
		exit(EXIT_FAILURE);
	}
}

void _getNameInfo(struct sockaddr* ai_addr, socklen_t ai_addrlen,
		NameInfo* nameInfo)
{
	int s;
	s = getnameinfo(ai_addr, ai_addrlen,
			nameInfo->host, NI_MAXHOST,
			nameInfo->serv, NI_MAXSERV, 0);
	if (s!=0) {
		syslog(LOG_ERR, "_getNameInfo: getnameinfo failed: %s\n", gai_strerror(s));
		syslog(LOG_ERR, "sa_family: %d\n sa_data: %s\n", ai_addr->sa_family,
				ai_addr->sa_data);
		exit(EXIT_FAILURE);
	}
}

int getSocket(int _bind, struct addrinfo *result)
{
	struct addrinfo *rp;
	int sfd;
	//NameInfo nameInfo;
	
	for (rp=result; rp!=NULL; rp=rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype,
			       	rp->ai_protocol);
		
		if (sfd == -1)
			continue;

		int err;
		if (_bind)
			err = bind(sfd, rp->ai_addr,
				       rp->ai_addrlen);
		else
			err = connect(sfd, rp->ai_addr,
					rp->ai_addrlen);
		if (!err)
			break;
	}

	if (rp == NULL)
	{
		syslog(LOG_ERR, "server: Failed to bind.\n");
		exit(EXIT_FAILURE);
	}
	
	//_getNameInfo(rp->ai_addr, rp->ai_addrlen, &nameInfo);
	//syslog(LOG_INFO, "getSocket: successfully %s to: %s:%s\n",
	//	       _bind ? "bound" : "connected",	nameInfo.host, nameInfo.serv);

	return sfd;
	
}

int bindOrConnectToAddress(char* host, char* port,
	       	int _bind)
{
	struct addrinfo *result; 
	int sfd;

	getAddressInfo(host, port, &result);	
	sfd = getSocket(_bind, result);	
	freeaddrinfo(result);
	
	return sfd;
}

static void signal_handler (int signo)
{
	if (signo == SIGINT)
		syslog(LOG_INFO, "Caught SIGINT, exiting");
	else if (signo == SIGTERM)
		syslog(LOG_INFO, "Caught SIGTERM, exiting");
	else {
		syslog(LOG_ERR, "Unexpected signal");
		exit(EXIT_FAILURE);
	}
	exit (EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int fd, nr, lfd, cfd;
	FILE *fp;
	char *fname = "/var/tmp/aesdsocketdata";
	char *hostname = "localhost";
	char *port = "9000";
	char *buf = (char *) malloc(BUF_SIZE);
	if (buf == NULL) {
		syslog(LOG_ERR, "Failed to allocate memory for buf");
		exit(EXIT_FAILURE);
	}
	struct sockaddr clientAddr;
	//struct sockaddr_in *clientAddr_in;
	socklen_t sl;
	pid_t pid;
	int count;

	openlog("aesdsocket", 0, LOG_USER);

	if (signal (SIGINT, signal_handler) == SIG_ERR) {
		syslog(LOG_ERR, "Cannot handle SIGIN!");
		exit(EXIT_FAILURE);
	}

	if (signal (SIGTERM, signal_handler) == SIG_ERR) {
		syslog(LOG_ERR, "Cannot handle SIGTERM!");
		exit(EXIT_FAILURE);
	}

	lfd = bindOrConnectToAddress(hostname, port, 1);
	if (argc > 1) {
		if (strcmp(argv[1], "-d") == 0) {
			pid = fork();
			if (pid == -1) {
				syslog(LOG_ERR, "Failed to fork(): %s", strerror(errno));
				exit(EXIT_FAILURE);
			}
			else if (pid != 0) {
				syslog(LOG_INFO, "Successfully forked: %d", pid);
				exit(EXIT_SUCCESS);
			}

			if (setsid() == -1) {
				syslog(LOG_ERR, "Failed to create new session and group: %s", 
						strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (chdir("/") == -1) {
				syslog(LOG_ERR, "Failed to changed to root dir: %s", 
						strerror(errno));
				exit(EXIT_FAILURE);
			}

			open("/dev/null", O_RDWR);
			dup(0);
			dup(1);
		}
	}

	if (listen(lfd, 0) == -1) {
		syslog(LOG_ERR, "Failure in listen(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "Listening on %s:%s", hostname, port);
	

	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		syslog(LOG_ERR, "Failed truncating %s: %s", fname, strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);
	syslog(LOG_INFO, "Successfully truncated %s", fname);
	
	for (;;) {
		cfd = accept(lfd, &clientAddr, &sl);
		//clientAddr_in = (struct socketaddr_in *) clientAddr;
		if (cfd == -1) {
			syslog(LOG_ERR, "Failure in accept(): %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "Accepted connection from %s\n", 
				clientAddr.sa_data);

		memset(buf, 0, BUF_SIZE);
		nr = readLine(cfd, buf, BUF_SIZE);
		if (nr == -1) {
			syslog(LOG_ERR, "Failed to recieve from %s", clientAddr.sa_data);
			exit(EXIT_FAILURE);
		}
		
		count = strlen(buf);
		fd = open(fname, O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
		if (fd == -1) {
			syslog(LOG_ERR, "Failed to open %s: %s", fname, strerror(errno));
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "Opened file %s", fname);

		nr = write(fd, buf, count);
		if (nr == -1) {
			syslog(LOG_ERR, "Failed to write %s to %s: %s", 
					buf, fname, strerror(errno));
			exit(EXIT_FAILURE);
		}
		else if (nr != count) {
			syslog(LOG_ERR, "Possible failed write.");
			exit(EXIT_FAILURE);
		}
		syslog(LOG_INFO, "Wrote msg to %s", fname);
		close(fd);

		fp = fopen(fname, "r");

		memset(buf, 0, BUF_SIZE);
		count = 0;
		while (fgets(buf, BUF_SIZE, fp) != NULL) {
			send(cfd, buf, strlen(buf), 0); 
			syslog(LOG_INFO, "Sent %d %s",
				       	count++, buf);
		}
		fclose(fp);
		close(cfd);
		syslog(LOG_INFO, "Closed connection from %s", clientAddr.sa_data);
	}



	free(buf);
	close(lfd);

	return 0;
}

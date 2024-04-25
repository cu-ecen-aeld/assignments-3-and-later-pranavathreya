#include "aesdsocket.h"
#include <pthread.h>

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

int getSocket(int _bind, struct addrinfo *result)
{
	struct addrinfo *rp;
	int sfd;
	
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

int main(int argc, char **argv)
{
	int fd;
	char *hostname;
	char *port;
	struct sockaddr clientAddr;
	socklen_t sl;
	pid_t pid;
	pthread_t thing1;
	struct files f;

	openlog("aesdsocket", 0, LOG_USER);

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

	if (argc > 2) {
		hostname = argv[2];
	}
	else {
		hostname = "0.0.0.0";
	}

	if (argc > 3) {
		port = argv[3];
	}
	else {
		port = "9000";
	}

	int lfd = bindOrConnectToAddress(hostname, port, 1);
	if (listen(lfd, 128) == -1) {
		syslog(LOG_ERR, "Failure in listen(): %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "Listening on %s:%s", hostname, port);
	

	fd = open(FNAME, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		syslog(LOG_ERR, "Failed truncating %s: %s", FNAME, strerror(errno));
		exit(EXIT_FAILURE);
	}
	close(fd);
	syslog(LOG_INFO, "Successfully truncated %s", FNAME);
	
	int cfd; /* connection fd */
	f.count = 1;
	f.descriptors = (int*) malloc(sizeof(int) * f.count);
	for (;;) {
		cfd = accept(lfd, &clientAddr, &sl);
		f.descriptors[0] = cfd;
		if (cfd == -1) {
			syslog(LOG_ERR, "Failure in accept(): %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		syslog(LOG_INFO, "Creating thread");
		pthread_create(&thing1, NULL, handleRequest, &f);
		syslog(LOG_INFO, "Created thread");
		pthread_join(thing1, NULL);
	}

	free(f.descriptors);
	return 0;
}

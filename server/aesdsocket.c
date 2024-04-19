#include "aesdsocket.h"

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

//void _getNameInfo(struct sockaddr* ai_addr, socklen_t ai_addrlen,
//		NameInfo* nameInfo)
//{
//	int s;
//	s = getnameinfo(ai_addr, ai_addrlen,
//			nameInfo->host, NI_MAXHOST,
//			nameInfo->serv, NI_MAXSERV, 0);
//	if (s!=0) {
//		syslog(LOG_ERR, "_getNameInfo: getnameinfo failed: %s\n", gai_strerror(s));
//		syslog(LOG_ERR, "sa_family: %d\n sa_data: %s\n", ai_addr->sa_family,
//				ai_addr->sa_data);
//		exit(EXIT_FAILURE);
//	}
//}

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

int main(int argc, char **argv)
{
	int fd;
	char *hostname;
	char *port;
	
	struct sockaddr clientAddr;
	socklen_t sl;
	pid_t pid;

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
	for (;;) {
		cfd = accept(lfd, &clientAddr, &sl);
		if (cfd == -1) {
			syslog(LOG_ERR, "Failure in accept(): %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		switch (fork()) {
			case -1:
				syslog(LOG_ERR, "Can't create child: %s", strerror(errno));
				close(cfd);
				break;	// Might be temporary, listen for next connection
			
			case 0: 				// Child
				close(lfd);			// Unneeded copy of lfd
				handleRequest(cfd, fd);
				exit(EXIT_SUCCESS);
			default: 				// Parent
				close(cfd);			// cfd copied over to child
				break;				// Listen for next
		}
	}

	return 0;
}

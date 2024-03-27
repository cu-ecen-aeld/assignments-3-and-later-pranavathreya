#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

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
		syslog(LOG_ERR, "sa_family: %d\nsa_data: %s\n", ai_addr->sa_family,
				ai_addr->sa_data);
		exit(EXIT_FAILURE);
	}
}

int getSocket(int _bind, struct addrinfo *result)
{
	struct addrinfo *rp;
	int sfd;
	NameInfo nameInfo;
	
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
	
	_getNameInfo(rp->ai_addr, rp->ai_addrlen, &nameInfo);
	syslog(LOG_INFO, "getSocket: successfully %s to: %s:%s\n",
		       _bind ? "bound" : "connected",	nameInfo.host, nameInfo.serv);

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

int main()
{
	struct sockaddr sa;
	socklen_t sl;

	int lfd = bindOrConnectToAddress("localhost", "9000", 1);

	int cfd; /* connection fd */
	for (;;) {
		cfd = accept(lfd, &sa, &sl);
		if (cfd == -1) {
			//syslog(LOG_ERR, "Failure in accept(): %s", strerror(errno));
			printf("%s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

	}

	return 0;
}

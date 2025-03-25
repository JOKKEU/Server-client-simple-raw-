#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>

#include <pthread.h>
#include <sched.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h> 

#include <linux/in.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <asm-generic/param.h>

#define true 				1
#define BUFFER_SIZE			8192

#ifndef PORT
	#define PORT 8080
#endif // PORT

#define LOG(...) 			fprintf(stdout, "" __VA_ARGS__)
#define MAX_PACKET_SIZE  		1024


struct _packet
{
	struct sockaddr_in*	packet_source;
	struct sockaddr_in*	client_packet_source;
	socklen_t		packet_source_len;
	socklen_t		client_packet_source_len;
	
	char*			buffer;
};


struct socket_param
{
	
	int			domain;
	int 			type;
	int 			protocol;
	
};


struct _client
{
	size_t			port;
	char			ip_server[INET_ADDRSTRLEN];
	struct socket_param*	sock_param;
	int			sockfd;
	
	char*			buffer_for_server;
};


extern int client(void);

#endif // __CLIENT_H__

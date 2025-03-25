#ifndef __SERVER__
#define __SERVER__

#include "gen.h"

#include <pthread.h>
#include <sched.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include <png.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h> 

#include <linux/in.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <asm-generic/param.h>

#define true 				1

#define MAX_PACKET_SIZE			1024
#define LOG(...) 			fprintf(stdout, "" __VA_ARGS__)


#define CLIENTS			 	5


#ifndef PORT

	#define PORT 			8080
	
#endif // PORT



#ifndef PING_BUFFER_SIZE
	#define PING_BUFFER_SIZE 	64
#endif // PING_BUFFER_SIZE


#ifndef BUFFER_ACCEPT_SIZE

	#define BUFFER_ACCEPT_SIZE 4096
	
#endif // BUFFER_ACCEPT_SIZE

#define SIZE_TIME_BUFFER 		80


struct _flags
{
	int 			stop_thread;
	int			stop_ot;
};

struct display_param
{
	int width;
	int height;
};



struct local_check
{
	struct icmp* 		lc_packet;
	struct sockaddr_in* 	lc_addr;
	
	struct timeval	 	timeout;
	
	
	char*			start_ip;
	int 			lc_sockfd;
	char			buffer[PING_BUFFER_SIZE];
	
	
	void			(*cleanup_lc)(struct local_check* lc);
};



struct data_in_packet
{
	ssize_t 		data_size;
	char* 			buffer_packet;
};


struct display_client
{
	int 			width;
	int 			height;
};

struct _packet
{

	struct sockaddr_in* 	source_packet;
	socklen_t		source_packet_len;	
	
	struct data_in_packet* 	data_in_pac;
	
	void (*cleanup_packet)(struct _packet*);
	
	
};

struct socket_param
{
	int 			domain;
	int 			type;
	int 			protocol;
	
	
	int 			backlog;
	int 			opt;
	
};

struct _server
{
	char			my_ip[INET_ADDRSTRLEN];
	char			interface[IFNAMSIZ];
	char*			buffer_for_accept;
	
	int 			all_online_client;
	int 			max_online_client;
	char**			all_client_ips;
	
	int 			max_my_online_client;
	int 			my_online_client;
	char** 			my_client_ips;
	char**			my_client_time;
	
	
	size_t 			start_device_count;
	unsigned short		port;
	size_t 			mtu;
	int 			sockfd;
	
	size_t 			chunk_size;
	
	struct socket_param* 	sock_param;
	struct _flags*		flags; 
	
	void (*cleanup_server)(struct _server*);
};



struct thread_data
{
	
	pthread_t		operation_thread;
	pthread_mutex_t		ot_mutex;
	void*			(*op_handler)(void*);
	
	pthread_mutex_t		lp_mutex;
	
	pthread_t 		thread_client_check;
	pthread_mutex_t 	mutex_client_check;
	pthread_mutex_t 	mutex_client_check_inner;
	
	void*			(*client_handler_func)(void*);
	
};	

struct data_for_thread
{
		
	struct _server* 	server;
	struct local_check* 	lc;	
	struct thread_data* 	thread_d;
	struct _packet*		packet;
	
};



extern int   server(void);
extern void  get_start_ip_for_local_check(struct local_check* __restrict lc, struct _server* __restrict server);
extern void  start_init_packet_data(struct _packet* __restrict pac, struct _server* __restrict server);
extern void  start_init_server(struct _server* server);
extern void  init_local_check(struct local_check* lc);
extern char* ping_local_net(struct _server* __restrict server, struct local_check* __restrict lc, struct thread_data* __restrict thread_d, char* ip_addr);

extern void all_clear(struct _server* __restrict server, struct local_check* __restrict lc, struct thread_data* __restrict thread_d, struct _packet* __restrict packet);
extern void* operation_clinets(void* data);
extern void* op_handler_console(void* data);

extern void set_nonblocking_mode(int fd);
extern void reset_terminal_mode(struct termios* orig_termios);
extern void remove_my_client_ips(struct _server* __restrict server, struct thread_data* __restrict thread_d, char* ip_addr);

extern void filling_my_client_ips(struct _server* __restrict server, struct thread_data* __restrict thread_d, struct _packet* __restrict packet);
extern void get_screenshot(struct _server* __restrict server, struct thread_data* __restrict thread_d, struct _packet* __restrict packet, int index, char* filename);
extern void my_clients_print(struct _server* server, struct thread_data* thread_d);
extern void add_client_in_my_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr);
extern void my_client_ips_realloc(struct _server* server, struct thread_data* thread_d);
#endif // __SERVER__













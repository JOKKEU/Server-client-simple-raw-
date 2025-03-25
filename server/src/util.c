#include "../hd/server.h"
// wlp0s20f3

static void cleanup_packet(struct _packet* packet);
static void cleanup_server(struct _server* server);



static char* handle_start_ip(char* start_ip)
{
	int count_dot = 3;	
	int count = 0;
		
	for (size_t index = 0; index < INET_ADDRSTRLEN; ++index)
	{
		if (count == count_dot) {start_ip[index] =  '\0'; break;}
		if (start_ip[index] == '.') {count++;}
	}
	
	return start_ip;
}

void get_start_ip_for_local_check(struct local_check* __restrict lc, struct _server* __restrict server)
{
	struct ifreq ifr_ip;
	struct ifreq ifr_mask;
	struct sockaddr_in lc_addr_for_ip;

	lc->lc_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (lc->lc_sockfd < 0)
	{
		perror("failed create socket");
		exit(EXIT_FAILURE);
	}
	
	strncpy(ifr_ip.ifr_name, server->interface, IFNAMSIZ);
	strncpy(ifr_mask.ifr_name, server->interface, IFNAMSIZ);
	
	if (ioctl(lc->lc_sockfd, SIOCGIFADDR, &ifr_ip) < 0)
	{
		perror("error ioctl for IP address");
		close(lc->lc_sockfd);
		exit(EXIT_FAILURE);
	}
	  
    	lc_addr_for_ip = 	*(struct sockaddr_in*)&ifr_ip.ifr_addr;
    	struct in_addr ip_addr;
    	ip_addr.s_addr = 	lc_addr_for_ip.sin_addr.s_addr;
    	
    	
    	if (ioctl(lc->lc_sockfd, SIOCGIFNETMASK, &ifr_mask) < 0)
	{
		perror("error ioctl for netmask");
		close(lc->lc_sockfd);
		exit(EXIT_FAILURE);
	}
    	

    	struct in_addr netmask;
	netmask.s_addr = 	((struct sockaddr_in *)&ifr_mask.ifr_addr)->sin_addr.s_addr;
    
   	struct in_addr network_addr;
    	network_addr.s_addr = 	lc_addr_for_ip.sin_addr.s_addr & netmask.s_addr;    	
	
    	strncpy(server->my_ip, inet_ntoa(ip_addr), sizeof(server->my_ip));
    	strncpy(lc->start_ip, inet_ntoa(network_addr), INET_ADDRSTRLEN);
    	
    	lc->start_ip = handle_start_ip(lc->start_ip);
    	close(lc->lc_sockfd);
}



static void print_clients_online(struct _server* server, struct thread_data* thread_d)
{
			
	for(size_t index = 0; index < server->max_online_client; ++index)
	{
		if (strcmp(server->all_client_ips[index], "0") == 0) {continue;}
		LOG("[%lu] client on: %s ip\n", index, server->all_client_ips[index]);
	}
	
}

void my_clients_print(struct _server* server, struct thread_data* thread_d)
{
			
	for(size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (strcmp(server->my_client_ips[index], "0") == 0) {continue;}
		LOG("[%lu] my client on: %s ip\ttime: %s\n", index, server->my_client_ips[index], server->my_client_time[index]);
	}
	
}

static int check_ip_in_arr(struct _server* server, char* ip_addr)
{
	for (size_t index = 0; index < server->my_online_client; ++index)
	{
		if (strcmp(server->my_client_ips[index], ip_addr) == 0)
		{
			return EXIT_SUCCESS;
		}
	}
	
	return EXIT_FAILURE;
}



static void remove_client(struct _server* server, struct thread_data* thread_d, size_t index)
{
	pthread_mutex_lock(&thread_d->mutex_client_check_inner);

	if (check_ip_in_arr(server, server->all_client_ips[index]) == 0)
	{
		strncpy(server->my_client_ips[index], "0", INET_ADDRSTRLEN);
		server->my_client_ips[index][INET_ADDRSTRLEN - 1] = '\0';
		server->my_online_client--;
		//LOG("removed all my client: %s (ip)\n", server->all_client_ips[index]);
		//LOG("(remove all) my online client: %lu\n", server->my_online_client);
	}
	//LOG("client removed (ip: %s) \n", server->all_client_ips[index]);
	strncpy(server->all_client_ips[index], "0", INET_ADDRSTRLEN);
	server->all_client_ips[index][INET_ADDRSTRLEN - 1] = '\0';
	server->all_online_client--;
	//LOG("client removed after (ip: %s) \n", server->all_client_ips[index]);
	//LOG("online client after removed: %lu\n", server->all_online_client);
	pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
	
}


void remove_my_client_ips(struct _server* __restrict server, struct thread_data* __restrict thread_d, char* ip_addr)
{
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if(strcmp(server->my_client_ips[index], ip_addr) == 0)
		{
			pthread_mutex_lock(&thread_d->mutex_client_check);
			
			strncpy(server->my_client_ips[index], "0", INET_ADDRSTRLEN);
			server->my_client_ips[index][INET_ADDRSTRLEN - 1] = '\0';
			server->my_online_client--;
			strncpy(server->my_client_time[index], "0", SIZE_TIME_BUFFER);
			server->my_client_time[index][SIZE_TIME_BUFFER - 1] = '\0';
			
			pthread_mutex_unlock(&thread_d->mutex_client_check);
		}
	}
	
	//LOG("removed my client: %s (ip)\n", ip_addr);
	//LOG("(remove) my online client: %lu\n", server->my_online_client);
}



static void add_client(struct _server* server, struct thread_data* thread_d, char* ip_addr)
{
	//LOG("get (ip: %s) \n", ip_addr);
	for (size_t index = 0; index < server->max_online_client; ++index)
	{
		pthread_mutex_lock(&thread_d->mutex_client_check_inner);
		if (server->all_client_ips[index] != NULL && strcmp(server->all_client_ips[index], ip_addr) == 0) 
		{
			//LOG("coincidence (ip: %s) \n", server->all_client_ips[index]);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			return;
		}
		pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
	}
	
	for (size_t index = 0; index < server->max_online_client; ++index)
	{
		if (server->all_client_ips[index] != NULL && strcmp(server->all_client_ips[index], "0") == 0)
		{
			pthread_mutex_lock(&thread_d->mutex_client_check_inner);
			strncpy(server->all_client_ips[index], ip_addr, INET_ADDRSTRLEN);
			server->all_client_ips[index][INET_ADDRSTRLEN - 1] = '\0';
			//LOG("Client added (ip: %s) \n", server->all_client_ips[index]);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			break;
		}
	}
	server->all_online_client++;
	//LOG("online client: %lu\n", server->all_online_client);	
}


static void set_time_on_client(struct _server* server, struct thread_data* thread_d, size_t index)
{
	time_t raw_time;
	time(&raw_time);
	
	struct tm* timeinfo;
	timeinfo = localtime(&raw_time);
	
	strftime(server->my_client_time[index], SIZE_TIME_BUFFER, "%Y-%m-%d %H:%M:%S", timeinfo);
}


void add_client_in_my_client_ips(struct _server* server, struct thread_data* thread_d, char* ip_addr)
{
	//LOG("get (ip: %s) \n", ip_addr);
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		pthread_mutex_lock(&thread_d->mutex_client_check_inner);
		if (server->my_client_ips[index] != NULL && strcmp(server->my_client_ips[index], ip_addr) == 0) 
		{
			//LOG("coincidence (ip: %s) \n", server->my_client_ips[index]);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			return;
		}
		pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
	}
	
	if (server->my_online_client >= server->max_my_online_client)
	{
		my_client_ips_realloc(server, thread_d);
	}
	
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		if (server->my_client_ips[index] != NULL && strcmp(server->my_client_ips[index], "0") == 0)
		{
			pthread_mutex_lock(&thread_d->mutex_client_check_inner);
			strncpy(server->my_client_ips[index], ip_addr, INET_ADDRSTRLEN);
			server->my_client_ips[index][INET_ADDRSTRLEN - 1] = '\0';
			set_time_on_client(server, thread_d, index);
			server->my_client_time[index][SIZE_TIME_BUFFER - 1] = '\0';
			//LOG("Client added (ip: %s) \n", server->my_client_ips[index]);
			pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
			break;
		}
	}
	server->my_online_client++;
	//LOG("online client: %lu\n", server->all_online_client);	
}


#define __CHECK_IP(_START, _MAX_INDEX)								\
for (_START; _START < _MAX_INDEX; ++index)							\
{												\
	snprintf(index_to_str, sizeof(index_to_str), "%d", index);				\
	snprintf(ip_for_check, INET_ADDRSTRLEN, "%s%s", dft->lc->start_ip, index_to_str);	\
	if (strcmp(ip_for_check, dft->server->my_ip) == 0) {continue;}				\
	ip_for_check[0] = '1';									\
	char* ip_addr = ping_local_net(dft->server, dft->lc, dft->thread_d, ip_for_check); 	\
	if (ip_addr == NULL)									\
	{continue;}										\
												\
	else if (dft->server->all_online_client == dft->server->max_online_client) 		\
	{											\
		pthread_mutex_lock(&dft->thread_d->mutex_client_check);				\
		all_client_ips_realloc(dft->server, dft->thread_d);				\
		pthread_mutex_unlock(&dft->thread_d->mutex_client_check); 			\
												\
	}											\
	pthread_mutex_lock(&dft->thread_d->mutex_client_check);					\
	add_client(dft->server, dft->thread_d, ip_addr);					\
	pthread_mutex_unlock(&dft->thread_d->mutex_client_check);				\
}												


static void all_client_ips_realloc(struct _server* server, struct thread_data* thread_d)
{
	size_t new_size = server->max_online_client + 5;
	char** temp_max_clients = (char**)malloc(sizeof(char*) * new_size);
	if (!temp_max_clients)
	{
		perror("error realloc all_client_ips");
		exit(EXIT_FAILURE);
	}	
	for(size_t index = 0; index < new_size; ++index)
	{
		temp_max_clients[index] = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
		if (!temp_max_clients[index])
		{
			perror("error realloc all_client_ips[index]");
			exit(EXIT_FAILURE);
		}
		
		memset(temp_max_clients[index], 0, INET_ADDRSTRLEN);
	}
	
	
	for (size_t index = server->max_online_client; index < new_size; ++index)
	{
		strncpy(temp_max_clients[index], "0", INET_ADDRSTRLEN);
		temp_max_clients[index][INET_ADDRSTRLEN - 1] = '\0';
	} 
	
	for (size_t index = 0; index < server->max_online_client; ++index)
	{
		pthread_mutex_lock(&thread_d->mutex_client_check_inner);
		strncpy(temp_max_clients[index], server->all_client_ips[index], INET_ADDRSTRLEN);
		pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
		temp_max_clients[index][INET_ADDRSTRLEN] = '\0'; 
	}
	
	for (size_t index = 0; index < server->max_online_client; ++index)
    	{
        	free(server->all_client_ips[index]);
    	}
    	
    	free(server->all_client_ips); 		
	server->all_client_ips = temp_max_clients;
	server->max_online_client = new_size;
}


void my_client_ips_realloc(struct _server* server, struct thread_data* thread_d)
{
	size_t new_size = server->max_my_online_client + 5;
	char** temp_max_clients = (char**)malloc(sizeof(char*) * new_size);
	if (!temp_max_clients)
	{
		perror("error realloc my_client_ips");
		exit(EXIT_FAILURE);
	}
	
	char** temp_my_client_time = (char**)malloc(sizeof(char*) * new_size);
	if (!temp_my_client_time)
	{
		perror("error realloc temp_my_client_time");
		exit(EXIT_FAILURE);
	}
			
	for(size_t index = 0; index < new_size; ++index)
	{
		temp_max_clients[index] = (char*)malloc(INET_ADDRSTRLEN * sizeof(char));
		if (!temp_max_clients[index])
		{
			perror("error realloc my_client_ips[index]");
			exit(EXIT_FAILURE);
		}
		temp_my_client_time[index] = (char*)malloc(sizeof(char) * SIZE_TIME_BUFFER);
		if (!temp_my_client_time[index])
		{
			perror("error realloc temp_my_client_time[index]");
			exit(EXIT_FAILURE);
		}	
	}
	
	
	for (size_t index = server->max_my_online_client; index < new_size; ++index)
	{
		strncpy(temp_max_clients[index], "0", INET_ADDRSTRLEN);
		temp_max_clients[index][INET_ADDRSTRLEN - 1] = '\0';
		strncpy(temp_my_client_time[index], "0", SIZE_TIME_BUFFER);
		temp_my_client_time[index][SIZE_TIME_BUFFER - 1] = '\0';
	} 
	
	
	for (size_t index = 0; index < server->max_my_online_client; ++index)
	{
		pthread_mutex_lock(&thread_d->mutex_client_check_inner);
		strncpy(temp_max_clients[index], server->my_client_ips[index], INET_ADDRSTRLEN);
		pthread_mutex_unlock(&thread_d->mutex_client_check_inner);
		temp_max_clients[index][INET_ADDRSTRLEN ] = '\0'; 
	}
	
	for (size_t index = 0; index < server->max_my_online_client; ++index)
    	{
        	free(server->my_client_ips[index]);
        	free(server->my_client_time[index]);
    	}
    	
    	free(server->my_client_ips);
	free(server->my_client_time);
			
	server->my_client_ips = temp_max_clients;
	server->max_my_online_client = new_size;
	server->my_client_time = temp_my_client_time;
}


void* operation_clinets(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	
	struct termios orig_termios, new_termios;
	
	tcgetattr(STDIN_FILENO, &orig_termios);
    	new_termios = orig_termios;
    	
    	set_nonblocking_mode(STDIN_FILENO);
    	
    	new_termios.c_lflag &= ~ICANON; 
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
	
	char ip_for_check[32];
	char index_to_str[16];
	char* ip_addr;
	
	while (true)
	{
		pthread_mutex_lock(&dft->thread_d->mutex_client_check);
		if (!dft->server->flags->stop_thread)
		{
			pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
			break;
		}
		pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
		
		if (dft->server->all_online_client != 0)
		{
			for (size_t index = 0; index < dft->server->max_online_client; ++index)
			{
				if (strcmp(dft->server->all_client_ips[index], "0") != 0)
				{
					ip_addr = ping_local_net(dft->server, dft->lc, dft->thread_d, dft->server->all_client_ips[index]);
				}
				else
				{
					continue;
				}
				
				
				pthread_mutex_lock(&dft->thread_d->mutex_client_check);
				if (!ip_addr)
				{
					remove_client(dft->server, dft->thread_d, index);
					pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
				}
				
				pthread_mutex_unlock(&dft->thread_d->mutex_client_check);
				
			}
		}	
			
		size_t index = 0;
		__CHECK_IP(index, 51)
		usleep(4000000);
		__CHECK_IP(index, 102)
		usleep(4000000);
		__CHECK_IP(index, 153)
		usleep(4000000);
		__CHECK_IP(index, 204)
		usleep(4000000);
		__CHECK_IP(index, 254)
		usleep(4000000);
		
		//my_clients_print(dft->server, dft->thread_d);
		filling_my_client_ips(dft->server, dft->thread_d, dft->packet);	
		
		//print_clients_online(dft->server, dft->thread_d);
	}
	
	return NULL;
}


static unsigned short check_sum(void* b, int len)
{
	unsigned short *buffer 	= 	b;
    	unsigned int sum 	= 	0;
    	unsigned short result;
	
	for (sum = 0; len > 1; len -= 2)
	{
		sum += *buffer++;
	}
	
	if (len == 1)
	{
		sum += *(unsigned char*)buffer;
	}
	
	sum = (sum >> 16) + (sum + 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
	
}


char* ping_local_net(struct _server* __restrict server, struct local_check* __restrict lc, struct thread_data* __restrict thread_d, char* ip_addr)
{
	pthread_mutex_lock(&thread_d->lp_mutex);
	ip_addr[0] = '1';
	//LOG("ip_handl: %s\n", ip_addr);
	pthread_mutex_lock(&thread_d->mutex_client_check);
	lc->lc_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	pthread_mutex_unlock(&thread_d->mutex_client_check);
	if (!lc->lc_sockfd)
	{
		perror("ping local net error");
		exit(EXIT_FAILURE);
	}
	
	memset(lc->lc_addr, 0, sizeof(lc->lc_addr));
	lc->lc_addr->sin_family = 	AF_INET;
	lc->lc_addr->sin_addr.s_addr = 	inet_addr(ip_addr);
	
	memset(lc->lc_packet, 0, sizeof(lc->lc_packet));
	lc->lc_packet->icmp_type = 	ICMP_ECHO;
	lc->lc_packet->icmp_code =	0;
	lc->lc_packet->icmp_id = 	getpid();
	lc->lc_packet->icmp_seq = 	1;
	lc->lc_packet->icmp_cksum = 	check_sum((void*)lc->lc_packet, sizeof(struct icmp));
	
	pthread_mutex_lock(&thread_d->mutex_client_check);
	if (sendto(lc->lc_sockfd, lc->lc_packet, sizeof(struct icmp), 0, (struct sockaddr*)lc->lc_addr, sizeof(struct sockaddr_in)) <= 0)
	{
		perror("sendto error");
		close(lc->lc_sockfd);
		exit(EXIT_FAILURE);
	}
	pthread_mutex_unlock(&thread_d->mutex_client_check);
	
	lc->timeout.tv_sec = 0; 
	lc->timeout.tv_usec = 50000; // 50 ms

	pthread_mutex_lock(&thread_d->mutex_client_check);
	if (setsockopt(lc->lc_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&lc->timeout, sizeof(lc->timeout)) < 0) 
	{
	    	perror("setsockopt failed");
	    	close(lc->lc_sockfd);
	    	exit(EXIT_FAILURE);
	}
	
	socklen_t addr_len = sizeof(lc->lc_addr);
	if (recvfrom(lc->lc_sockfd, lc->buffer, sizeof(lc->buffer), 0, (struct sockaddr*)lc->lc_addr, &addr_len) > 0)
	{
		close(lc->lc_sockfd);
		pthread_mutex_unlock(&thread_d->mutex_client_check);
		pthread_mutex_unlock(&thread_d->lp_mutex);
		return ip_addr;
	}
	
	pthread_mutex_unlock(&thread_d->mutex_client_check);
	pthread_mutex_unlock(&thread_d->lp_mutex);
	
	close(lc->lc_sockfd);
	return NULL;
}




void set_nonblocking_mode(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl get");
        	exit(EXIT_FAILURE);
	}
	
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void reset_terminal_mode(struct termios* orig_termios) 
{
    tcsetattr(STDIN_FILENO, TCSANOW, orig_termios);
}

void* op_handler_console(void* data)
{
	struct data_for_thread* dft = (struct data_for_thread*)data;
	struct termios orig_termios, new_termios;
	
	tcgetattr(STDIN_FILENO, &orig_termios);
    	new_termios = orig_termios;
    	
    	set_nonblocking_mode(STDIN_FILENO);
    	
    	new_termios.c_lflag &= ~ICANON; 
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    	
    	
    	char operation[256];
    	size_t index = 0;
	
	
	while (true)
	{
		LOG("1\n");
		memset(operation, 0, sizeof(operation)); 
		
		pthread_mutex_lock(&dft->thread_d->ot_mutex);
		if (!dft->server->flags->stop_ot)
		{
			pthread_mutex_unlock(&dft->thread_d->ot_mutex);
			break;
		}
		pthread_mutex_unlock(&dft->thread_d->ot_mutex);
		
		LOG("\nEnter help\n");
		fflush(stdout);
    		LOG(">:\n");
    		
    		while (true)
    		{
    			char c;
    			int n = read(STDIN_FILENO, &c, 1);
    			
    			if (n == -1)
    			{
    				if (errno == EAGAIN)
    				{
    					continue;
    				}
    				
    				else
    				{
    					perror("read error in console");
    					break;
    				}
    			}	
    			
    			if (c == '\n')
    			{
    				break;
    			}
    			
    			if (index < sizeof(operation) - 1)
    			{
    				operation[index++] = c;
    				
    			}
    		}
    		
    		operation[index] = '\0';
    		index = 0;
		LOG("2\n");
		LOG("\n\n");
		
		if (strcmp(operation, "help") == 0)
		{	
			LOG("[1] Server parameters\n");
    			LOG("[2] Print all ips online\n");
    			LOG("[3] Print my clients\n");
    			LOG("[screen [id] [filename]] get screen for client\n");
    			LOG("[clear] screen clear\n");
		}
		
		else if (strcmp(operation, "1") == 0)
		{
			LOG("Interface: %s\n", dft->server->interface);
			LOG("MTU: %lu\n", dft->server->mtu);
			LOG("My ip: %s\n", dft->server->my_ip);
			LOG("Port: %lu\n", dft->server->port);
		}
		
		else if (strcmp(operation, "2") == 0)
		{
			pthread_mutex_lock(&dft->thread_d->ot_mutex);
			print_clients_online(dft->server, dft->thread_d);
			pthread_mutex_unlock(&dft->thread_d->ot_mutex);
		}
		
		
		else if (strcmp(operation, "3") == 0)
		{
			pthread_mutex_lock(&dft->thread_d->ot_mutex);
			my_clients_print(dft->server, dft->thread_d);
			pthread_mutex_unlock(&dft->thread_d->ot_mutex);
		}
		
		else if (strcmp(operation, "screen") == 0)
		{
			pthread_mutex_lock(&dft->thread_d->ot_mutex);
			char id[4];
			memset(id, 0, sizeof(id)); 
			size_t index_id = 0;
			LOG("enter id client which is stored in '[]' (0 - 9999)\n");
			while (true)
	    		{
	    			char c;
	    			int n = read(STDIN_FILENO, &c, 1);
	    			
	    			if (n == -1)
	    			{
	    				if (errno == EAGAIN)
	    				{
	    					continue;
	    				}
	    				
	    				else
	    				{
	    					perror("read error in console");
	    					break;
	    				}
	    			}	
	    			
	    			if (c == '\n')
	    			{
	    				break;
	    			}
	    			
	    			if (index_id < sizeof(id) - 1)
	    			{
	    				id[index_id++] = c;
	    				
	    			}
	    			else
	    			{
	    				LOG("max index (0-9999)\n");
	    			}
	    		}
	    		
	    		
	    		char filename[40];
	    		memset(filename, 0, sizeof(filename));
	    		size_t index_filename = 0; 
	    		LOG("enter filename for save (40 chars)\n");
	    		while (true)
	    		{
	    			char c;
	    			int n = read(STDIN_FILENO, &c, 1);
	    			
	    			if (n == -1)
	    			{
	    				if (errno == EAGAIN)
	    				{
	    					continue;
	    				}
	    				
	    				else
	    				{
	    					perror("read error in console");
	    					break;
	    				}
	    			}	
	    			
	    			if (c == '\n')
	    			{
	    				break;
	    			}
	    			
	    			if (index_filename < sizeof(filename) - 1)
	    			{
	    				filename[index_filename++] = c;
	    				
	    			}
	    			else
	    			{
	    				LOG("max index (0-9999)\n");
	    			}
	    		}
	    		
	    		id[index_id] = '\0';
	    		index_id = 0;
	    		
	    		filename[40] = '\0';
	    		index_filename = 0; 
	    		
	    		int index_client = atoi(id);
	    		LOG("===================== WAIT =====================\n");
	    		pthread_mutex_lock(&dft->thread_d->lp_mutex);
	    		get_screenshot(dft->server, dft->thread_d, dft->packet, index_client, filename);
	    		pthread_mutex_unlock(&dft->thread_d->lp_mutex);
	  
	    		memset(operation, 0, sizeof(operation));
	    		LOG("screen saved\n");
	    		
		}
		
		else if (strcmp(operation, "clear") == 0)
		{
			system("clear");
		}
		pthread_mutex_unlock(&dft->thread_d->lp_mutex);
		
	}
	
	reset_terminal_mode(&orig_termios);
	return NULL;
}















#include "../hd/server.h"


static void cleanup_lc(struct local_check* lc);
static void cleanup_packet(struct _packet* packet);
static void cleanup_server(struct _server* server);

void start_init_packet_data(struct _packet* __restrict pac, struct _server* __restrict server)
{
        pac->source_packet = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        if (!pac->source_packet) 
        {
            perror("failed alloc source_packet");
            exit(EXIT_FAILURE);
        }
        
        pac->source_packet->sin_family = AF_INET;
        pac->source_packet->sin_port = htons(PORT);
        pac->source_packet->sin_addr.s_addr = htonl(INADDR_ANY);
        memset(pac->source_packet->sin_zero, 0, sizeof(pac->source_packet->sin_zero));
        pac->source_packet_len = sizeof(struct sockaddr_in);
        
        pac->data_in_pac = (struct data_in_packet*)malloc(sizeof(struct data_in_packet));
        if (!pac->data_in_pac) 
        {
            perror("failed alloc data_in_pac");
            free(pac->source_packet);
            exit(EXIT_FAILURE);
        }
        
        pac->data_in_pac->buffer_packet = (char*)malloc(server->mtu);
        if (!pac->data_in_pac->buffer_packet) 
        {
            perror("failed alloc buffer_packet");
            free(pac->data_in_pac); 
            free(pac->source_packet);
            exit(EXIT_FAILURE);
        }
        
        pac->data_in_pac->data_size = 0;
        pac->cleanup_packet = cleanup_packet;
        
}

void start_init_server(struct _server* server)
{

	LOG("enter network interface: ");
	scanf("%s", server->interface);
	
	
	//strncpy(server->interface, "wlp0s20f3", IFNAMSIZ);
	server->start_device_count = 	CLIENTS;
	server->max_online_client = 	CLIENTS;
	server->max_my_online_client =	CLIENTS;
	
	struct ifreq ifr_mtu_server;
	int temp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (temp_sockfd < 0) 
	{
		perror("socket mtu");
		exit(EXIT_FAILURE);
	}
	
	memset(&ifr_mtu_server, 0, sizeof(ifr_mtu_server));
    	strncpy(ifr_mtu_server.ifr_name, server->interface, IFNAMSIZ - 1);
    	ifr_mtu_server.ifr_name[IFNAMSIZ - 1] = '\0';  // Обеспечить нулевое завершение строки
	
	if (ioctl(temp_sockfd, SIOCGIFMTU, &ifr_mtu_server) < 0) 
	{
		fprintf(stderr, "ioctl mtu: %s\n", strerror(errno));
		close(temp_sockfd);
		exit(EXIT_FAILURE);
    	}
    	
    	server->mtu = ifr_mtu_server.ifr_mtu;
    	
	
	server->all_client_ips = (char**)malloc(sizeof (char*) * server->max_online_client);
	if (!server->all_client_ips) 
	{
		perror("failed alloc all_client_ips");
		exit(EXIT_FAILURE);
	}

	for (size_t index = 0; index < server->max_online_client; ++index)
	{

		server->all_client_ips[index] = (char*)malloc(sizeof (char) * INET_ADDRSTRLEN);
		if (!server->all_client_ips[index])
		{
			perror("online users[i] alloc error");
			for (size_t j = 0; j < index; ++j)
			{
				free(server->all_client_ips[j]);
			}
			
			free(server->all_client_ips);
			exit(EXIT_FAILURE);
		}

		strncpy(server->all_client_ips[index], "0", INET_ADDRSTRLEN);
	}
	
	server->my_client_ips = (char**)malloc(sizeof (char*) * server->max_online_client);
	if (!server->my_client_ips) 
	{
		perror("failed alloc my_client_ips");
		exit(EXIT_FAILURE);
	}

	for (size_t index = 0; index < server->max_online_client; ++index)
	{

		server->my_client_ips[index] = (char*)malloc(sizeof (char) * INET_ADDRSTRLEN);
		if (!server->my_client_ips[index])
		{
			perror("online users[i] alloc error");
			for (size_t j = 0; j < index; ++j)
			{
				free(server->my_client_ips[j]);
			}
			
			free(server->my_client_ips);
			exit(EXIT_FAILURE);
		}

		strncpy(server->my_client_ips[index], "0", INET_ADDRSTRLEN);
	}

	server->sockfd =		0;
	server->port = 			PORT;
	server->cleanup_server =	cleanup_server;
	
	
	
	server->sock_param = (struct socket_param*)malloc(sizeof (struct socket_param));
	if (!server->sock_param)
	{
		perror("failed alloc all_client_ips");
		exit(EXIT_FAILURE);
	}
	
	server->flags = (struct _flags*)malloc(sizeof (struct _flags));
	if (!server->flags)
	{
		perror("failed alloc flags");
		exit(EXIT_FAILURE);
	}
	
	server->buffer_for_accept = (char*)malloc(sizeof(char) * BUFFER_ACCEPT_SIZE);
	if (!server->buffer_for_accept)
	{
		perror("failed alloc buffer_for_accept");
		exit(EXIT_FAILURE);
	}
	
	server->my_client_time = (char**)malloc(sizeof(char*) * server->max_my_online_client);
	if (!server->my_client_time)
	{
		perror("failed alloc server->my_client_time");
		exit(EXIT_FAILURE);
	}
	
	for(size_t index = 0; index < server->max_my_online_client; ++index)
	{
		
		server->my_client_time[index] = (char*)malloc(sizeof(char) * SIZE_TIME_BUFFER);
		if (!server->my_client_time[index])
		{
			perror("failed alloc server->my_client_time[index]");
		
			for (size_t i = 0; i < server->max_my_online_client; ++i)
			{
				free(server->my_client_time[index]);
			}
			
			free(server->my_client_time);
			exit(EXIT_FAILURE);
		}
	}
	
	server->flags->stop_thread = 	true;
	server->flags->stop_ot = 	true;
	server->sock_param->domain = 	AF_INET;
	server->sock_param->type = 	SOCK_DGRAM;
	server->sock_param->protocol =	0;
	server->sock_param->opt = 	1;
	server->sock_param->backlog =	server->start_device_count;
	server->all_online_client = 	0;
	server->my_online_client =	0;
	server->chunk_size =		server->mtu;
	
}




void init_local_check(struct local_check* lc)
{
	
	
	lc->lc_packet = (struct icmp*)malloc(sizeof (struct icmp));
	if (!lc->lc_packet)
	{
		perror("lc_packet alloc error");
		exit(EXIT_FAILURE);
	}
	
	lc->lc_addr = (struct sockaddr_in*)malloc(sizeof (struct sockaddr_in));
	if (!lc->lc_addr)
	{
		perror("lc_addr alloc error");
		exit(EXIT_FAILURE);
	}
	
	lc->start_ip = (char*)malloc(INET_ADDRSTRLEN + 1);
    	if (!lc->start_ip)
    	{
    		perror("error start_ip");
		exit(EXIT_FAILURE);
    	}
	
	lc->cleanup_lc = 		cleanup_lc;
	lc->lc_sockfd = 		0;
	
	
}



static void cleanup_lc(struct local_check* lc)
{	
	if (lc)
	{
		
		if (lc->lc_packet)
		{
			free(lc->lc_packet);
		}
		
		if (lc->lc_addr)
		{
			free(lc->lc_addr);
		}
		
		
		
	}
	
}



static void cleanup_packet(struct _packet* packet)
{
        if (packet) 
        {
        	if (packet->source_packet)
        	{
        		free(packet->source_packet);
        	}
        	
        	if (packet->data_in_pac->buffer_packet)
        	{
        		free(packet->data_in_pac->buffer_packet);
        	}
        	
        	if (packet->data_in_pac)
        	{
        		free(packet->data_in_pac);
        	}
        	
 
        }
}


static void cleanup_server(struct _server* server)
{
	if (server)
	{
		if (server->all_client_ips)
		{
			free(server->all_client_ips);
		}
		
		if (server->sock_param)
		{
			free(server->sock_param);
		}
		
		if (server->my_client_ips)
		{
			free(server->my_client_ips);
		}
		
		if (server->my_client_time)
		{
			free(server->my_client_time);
		}

	}

}


void all_clear(struct _server* __restrict server, struct local_check* __restrict lc, struct thread_data* __restrict thread_d, struct _packet* __restrict packet)
{
	if (thread_d)
	{
		free(thread_d);
	}
	
	if (lc)
	{
		lc->cleanup_lc(lc);
	}
	
	if (packet)
	{
		packet->cleanup_packet(packet); 
	}
    		
    	if (server)
    	{
    		server->cleanup_server(server);  
    	}  
	
}



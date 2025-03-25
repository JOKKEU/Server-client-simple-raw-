#include "../hd/server.h"


#define _CLEAR 			\
	if (packet)		\
	{			\
		free(packet); 	\
	}			\
				\
	if (server)		\
	{			\
		free(server); 	\
	}			\


int server(void)
{
	struct termios orig_termios, new_termios;
	
	tcgetattr(STDIN_FILENO, &orig_termios);
    	new_termios = orig_termios;
    	
    	set_nonblocking_mode(STDIN_FILENO);
    	
    	new_termios.c_lflag &= ~ICANON;  
    	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


	
   	struct _packet* packet = (struct _packet*)malloc(sizeof(struct _packet));
   	if (!packet)
    	{
		perror("failed to allocate packet");
		return EXIT_FAILURE;
    	}
    	
    	struct _server* server = (struct _server*)malloc(sizeof(struct _server));
    	if (!server)
    	{
		perror("failed to allocate server");
		free(packet); 
		return EXIT_FAILURE;
   	}
    	start_init_server(server);
    	start_init_packet_data(packet, server);
    	
    	server->sockfd = socket(server->sock_param->domain, server->sock_param->type, server->sock_param->protocol);
    	if (server->sockfd < 0)
   	{
		perror("failed to create socket");
		_CLEAR
		return EXIT_FAILURE;
    	}


	struct local_check* lc = (struct local_check*)malloc(sizeof (struct local_check));
	if (!lc)
	{
		perror("failed to allocate lc");
		_CLEAR
		return EXIT_FAILURE;
	}
	init_local_check(lc);
	
    	if (bind(server->sockfd, (struct sockaddr*)packet->source_packet, packet->source_packet_len) < 0)
    	{
		perror("failed binding socket");
		close(server->sockfd);
		_CLEAR
		return EXIT_FAILURE;
    	}
    	
    	struct thread_data* thread_d = (struct thread_data*)malloc(sizeof (struct thread_data));
	if (!thread_d)
	{
		perror("failed to allocate thread_data");
		_CLEAR
		return EXIT_FAILURE;
	}
	
	struct data_for_thread dft;
	dft.server = server;
	dft.lc = lc;
	dft.thread_d = thread_d;
	dft.packet = packet;
	
	pthread_mutex_init(&thread_d->mutex_client_check, NULL);
	pthread_mutex_init(&thread_d->mutex_client_check_inner, NULL);
	thread_d->client_handler_func = operation_clinets;
	
	pthread_mutex_init(&thread_d->lp_mutex, NULL);

	
	pthread_mutex_init(&thread_d->ot_mutex, NULL);
	thread_d->op_handler = op_handler_console;	
	get_start_ip_for_local_check(lc, server);
	
	
	if (pthread_create(&thread_d->thread_client_check, NULL, thread_d->client_handler_func, &dft) != 0)
    	{
    		perror("failed thread_client create");
		_CLEAR
		return EXIT_FAILURE;
    	}
    	
	LOG("Thread: client hadnler started!\n");
    	if (pthread_create(&thread_d->operation_thread, NULL, thread_d->op_handler, &dft) != 0)
    	{
    		perror("failed operation_thread create");
		_CLEAR
		return EXIT_FAILURE;
    	}

    	LOG("Thread: operation hadnler started!\n");
    	LOG("Server started and listening on port %hu\n", server->port);
    	
    	
   
	pthread_join(thread_d->thread_client_check, NULL);
	pthread_mutex_destroy(&thread_d->mutex_client_check);
	pthread_mutex_destroy(&thread_d->mutex_client_check_inner);
	
	
	pthread_join(thread_d->operation_thread, NULL);
	pthread_mutex_destroy(&thread_d->ot_mutex);
	
	all_clear(server, lc, thread_d, packet);
	close(server->sockfd);
	_CLEAR
    	return EXIT_SUCCESS;
}





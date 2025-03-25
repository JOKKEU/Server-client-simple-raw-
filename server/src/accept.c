#include "../hd/server.h"



static void process_client_request(struct _server* __restrict server, struct _packet* __restrict packet, struct thread_data* __restrict thread_d, char* ip_addr)
{
	//LOG("ip for check: %s\n", ip_addr);
	//LOG("1\n");
    	char client_ip[INET_ADDRSTRLEN];

    	memset(server->buffer_for_accept, 0, BUFFER_ACCEPT_SIZE);
    	ssize_t recv_len;

    	inet_pton(AF_INET, ip_addr, &packet->source_packet->sin_addr); 

    	const char* message_for_start = "ready"; 
    	if (sendto(server->sockfd, message_for_start, strlen(message_for_start), 0, (struct sockaddr*)packet->source_packet, packet->source_packet_len) < 0)
    	{
        	//LOG("sendto error: %s\n", strerror(errno));
        	return;
    	}
    
    	struct timeval tv;
    	tv.tv_sec = 1;
    	tv.tv_usec = 0;

    	if (setsockopt(server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0)
    	{
        	//perror("setting timeout error");
        	return; 
    	}
    
    	memset(server->buffer_for_accept, 0, BUFFER_ACCEPT_SIZE);
    	recv_len = recvfrom
    	(
	    	server->sockfd,
	    	server->buffer_for_accept,
	    	BUFFER_ACCEPT_SIZE,
	    	0,
	    	(struct sockaddr*)packet->source_packet,
	    	&packet->source_packet_len
    	);
    
    	if (recv_len < 0) 
    	{
        	//LOG("recvfrom error: %s\n", strerror(errno));
        	remove_my_client_ips(server, thread_d, ip_addr);
        	memset(packet->source_packet->sin_zero, 0, sizeof(packet->source_packet->sin_zero));
       	 	return; 
    	}

    	server->buffer_for_accept[recv_len] = '\0';
    	inet_ntop(AF_INET, &packet->source_packet->sin_addr, client_ip, INET_ADDRSTRLEN);

    	if (strcmp(client_ip, ip_addr) != 0) 
    	{
    		memset(packet->source_packet->sin_zero, 0, sizeof(packet->source_packet->sin_zero));
        	return; 
    	}

    	//LOG("ip accept %s\n", ip_addr);

    	if (strcmp(server->buffer_for_accept, "ready\n") == 0 || strcmp(server->buffer_for_accept, "ready") == 0)
    	{
        	pthread_mutex_lock(&thread_d->mutex_client_check);
        	add_client_in_my_client_ips(server, thread_d, client_ip);
        	pthread_mutex_unlock(&thread_d->mutex_client_check);
    	}
    	
    	memset(packet->source_packet->sin_zero, 0, sizeof(packet->source_packet->sin_zero));
    	//LOG("2\n");
    
}


void filling_my_client_ips(struct _server* __restrict server, struct thread_data* __restrict thread_d, struct _packet* __restrict packet)
{
	if (server->my_online_client > 0)
	{
		for(size_t index = 0; index < server->my_online_client; ++index)
		{
			if (strcmp(server->all_client_ips[index], "0") == 0) {continue;}
			process_client_request(server, packet, thread_d, server->all_client_ips[index]);
		}
		
	}
	
	for(size_t index = 0; index < server->max_online_client; ++index)
	{
		if (strcmp(server->all_client_ips[index], "0") == 0) {continue;}
		process_client_request(server, packet, thread_d, server->all_client_ips[index]);
		//LOG("%s ip\n", server->all_client_ips[index]);
	}
	//LOG("(fill) my online client: %lu\n", server->my_online_client);
}



void get_screenshot(struct _server* __restrict server, struct thread_data* __restrict thread_d, struct _packet* __restrict packet, int index, char* filename)
{
	struct display_param disp_param;
    	Display *display;
    	Screen *screen;

    	display = XOpenDisplay(NULL);
    	if (display == NULL) 
    	{
		LOG("error open display\n");
		return;
    	}
    
    	char client_ip[INET_ADDRSTRLEN];
    	screen = DefaultScreenOfDisplay(display);
    	disp_param.width = screen->width;
    	disp_param.height = screen->height;
    	XCloseDisplay(display);
    
    	ssize_t bytes_recv = 0;
    	errno = 0;  
    	inet_pton(AF_INET, server->my_client_ips[index], &packet->source_packet->sin_addr);

    	const char* start_screen = "getscreen";
    	//LOG("Sending to %s:%lu\n", server->my_client_ips[index], htons(packet->source_packet->sin_port));
    	//LOG("Packet length: %zu\n", packet->source_packet_len);
    	//LOG("Data: %s\n", start_screen);

    	if (sendto(server->sockfd, start_screen, strlen(start_screen), 0, (struct sockaddr*)packet->source_packet, packet->source_packet_len) < 0) 
    	{
        	perror("send to param");
        	return;
    	}

    	inet_ntop(AF_INET, &packet->source_packet->sin_addr, client_ip, INET_ADDRSTRLEN);
    	struct timeval tv;
    	tv.tv_sec = 10;
    	tv.tv_usec = 0;

    	if (setsockopt(server->sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) 
    	{
        	LOG("Failed to set socket option\n");
        	return;
    	}

    	char buffer_for_param[10];
    	memset(buffer_for_param, 0, 10);

    	ssize_t param_recv = recvfrom(server->sockfd, buffer_for_param, sizeof(buffer_for_param), 0, 
                                   (struct sockaddr*)packet->source_packet, &packet->source_packet_len);

    	if (param_recv < 0) 
    	{
       		perror("recvfrom failed param");
        	return;
    	}

    	LOG("Received parameters from client\n");

    	usleep(1000000);
    	char width_client_str[5];
    	char height_client_str[5];
    	strncpy(width_client_str, buffer_for_param, 4);
    	width_client_str[4] = '\0';
    	strncpy(height_client_str, buffer_for_param + 4, 4);
    	height_client_str[4] = '\0';
  
    	int width_client = atoi(width_client_str);
    	int height_client = atoi(height_client_str);
    	
    	//LOG("width: %d\n", width_client);
    	//LOG("height: %d\n", height_client);

    	size_t buffer_size = width_client * height_client * 3 + 1; // RGB
    	//LOG("buffer size: %lu\n", buffer_size);

    	server->buffer_for_accept = malloc(buffer_size);
    	if (!server->buffer_for_accept) 
    	{
        	perror("failed alloc server->buffer_for_accept");
       		return;
    	}

    	size_t total_bytes_recv = 0;
    	while (total_bytes_recv < buffer_size) 
    	{
        	size_t chunk_size = buffer_size - total_bytes_recv > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : buffer_size - total_bytes_recv;

        	ssize_t recv = recvfrom(server->sockfd, server->buffer_for_accept + total_bytes_recv, chunk_size, 0,
                                (struct sockaddr*)packet->source_packet, &packet->source_packet_len);

        	if (recv < 0) 
        	{
            		if (errno == EAGAIN || errno == EWOULDBLOCK) 
            		{
                		//LOG("Timed out waiting for incoming data, retry\n");
                		goto next_stage;
            		}
            		
        	}

        	if (strcmp(client_ip, server->my_client_ips[index]) != 0) 
        	{
			LOG("Received data from unexpected client: %s\n", client_ip);
		    	continue; 
        	}

		total_bytes_recv += recv;
		//LOG("bytes recv: %lu\n", total_bytes_recv);

        	if (total_bytes_recv % (30 * MAX_PACKET_SIZE) == 0) 
        	{
           		usleep(150000);
        	}

        	usleep(100); 
    	}
next_stage:

    	//LOG("1\n");
    	

    	FILE* fp = fopen(filename, "wb");
    	if (!fp) 
    	{
		LOG("Unable to open file %s for writing\n", filename);
		free(server->buffer_for_accept);
		return;
    	}
    	//LOG("1\n");
   	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    	if (!png) 
    	{
		fclose(fp);
		LOG("Unable to create PNG write struct\n");
		free(server->buffer_for_accept);	
		return;
    	}
    	//LOG("1\n");
	png_infop info = png_create_info_struct(png);
	if (!info) 
    	{
		png_destroy_write_struct(&png, NULL);
		fclose(fp);
		LOG("Unable to create PNG info struct\n");
		free(server->buffer_for_accept);
		return;
    	}
	//LOG("1\n");
    	if (setjmp(png_jmpbuf(png))) 
   	{
		png_destroy_write_struct(&png, &info);
		fclose(fp);
		LOG("Error during PNG creation\n");
		free(server->buffer_for_accept);
		return;
    	}
	//LOG("1\n");
    	png_init_io(png, fp);
    	png_set_IHDR(png, info, width_client, height_client, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    	png_write_info(png, info);
	//LOG("1\n");
    	for (int y = 0; y < height_client; ++y) 
    	{
        	png_bytep row = (png_bytep)(server->buffer_for_accept + y * 3 * width_client);
        	png_write_row(png, row);
    	}
    	//LOG("1\n");
	png_write_end(png, NULL);
    	png_destroy_write_struct(&png, &info);
    	fclose(fp);
    	free(server->buffer_for_accept);
    	//LOG("1\n");

	return;
}

























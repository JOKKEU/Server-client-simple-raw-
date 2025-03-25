#include "../hd/client.h"



static void get_screen(struct _client* client, size_t* buffer_size, int* width, int* height)
{
	Display* display = XOpenDisplay(NULL);
	
	if (!display)
	{
		perror("failed create display");
		return;
	}
	
	Window root = DefaultRootWindow(display);
	XWindowAttributes attrs;
	XGetWindowAttributes(display, root, &attrs);
	
	Screen* screen;
	screen = DefaultScreenOfDisplay(display);
	*width = screen->width;
    	*height = screen->height;
    	
    	XImage* image = XGetImage(display, root, 0, 0, *width, *height, AllPlanes, ZPixmap);
    	if (!image)
    	{
    		LOG("Unable to get image\n");
    		return;
    	}
    	
    	free(client->buffer_for_server);
    	*buffer_size = (*width) * 3 * (*height);
    	client->buffer_for_server = (char*)malloc(*buffer_size);
    	if (!client->buffer_for_server)
    	{
    		perror("failed alloc buffer_for_server in get screen");
    		free(client->buffer_for_server);
    		return;
    		
    	}
    	
    	for (int y = 0; y < *height; ++y)
    	{
    		for (int x = 0; x < *width; ++x)
    		{
    			uint32_t pixel = XGetPixel(image, x, y);
            		client->buffer_for_server[(y * (*width) + x) * 3 + 0] = (pixel & image->red_mask) >> 16;   // R
            		client->buffer_for_server[(y * (*width) + x) * 3 + 1] = (pixel & image->green_mask) >> 8;  // G
            		client->buffer_for_server[(y * (*width) + x) * 3 + 2] = (pixel & image->blue_mask);        // B
    		}
    	}
    	
    	XDestroyImage(image);
    	XCloseDisplay(display);
    	
}



int client(void)
{
	
	struct _client* client = (struct _client*)malloc(sizeof (struct _client));
	if (!client)
	{
		perror("failed alloc client");
		return EXIT_FAILURE;
	}
	
	client->port = PORT;
	
	client->sock_param = (struct socket_param*)malloc(sizeof (struct socket_param));
	if (!client->sock_param)
	{
		perror("failed alloc sock_param");
		return EXIT_FAILURE;
	}
	
	client->sock_param->domain = 	AF_INET;
	client->sock_param->type = 	SOCK_DGRAM;
	client->sock_param->protocol = 	0;
	
	struct _packet* packet = (struct _packet*)malloc(sizeof (struct _packet));
	if (!packet)
	{
		perror("failed alloc packet");
		return EXIT_FAILURE;
	}
	
	packet->packet_source = (struct sockaddr_in*)malloc(sizeof (struct sockaddr_in));
	if (!packet->packet_source)
	{
		perror("failed alloc packet_source");
		return EXIT_FAILURE;
	}
	
	packet->client_packet_source = (struct sockaddr_in*)malloc(sizeof (struct sockaddr_in));
	if (!packet->packet_source)
	{
		perror("failed alloc client_packet_source");
		return EXIT_FAILURE;
	}
	
	packet->packet_source_len = sizeof(struct sockaddr_in);
    	packet->client_packet_source_len = sizeof(struct sockaddr_in);
	
	packet->buffer = (char*)malloc(sizeof (char) * BUFFER_SIZE);
	if (!packet->buffer)
	{
		perror("failed alloc packet_buffer");
		return EXIT_FAILURE;
	}
	
	client->buffer_for_server = (char*)malloc(sizeof (char) * BUFFER_SIZE);
	if (!client->buffer_for_server)
	{
		perror("failed alloc buffer_for_server");
		return EXIT_FAILURE;
	}
	
	
	memset(packet->client_packet_source, 0, packet->client_packet_source_len);
    	packet->client_packet_source->sin_family = AF_INET;
    	packet->client_packet_source->sin_port = htons(PORT);
    	packet->client_packet_source->sin_addr.s_addr = INADDR_ANY;
	
			
	client->sockfd = socket(client->sock_param->domain, client->sock_param->type, client->sock_param->protocol);
	if (client->sockfd < 0)
	{
		perror("failed create socket");
		return EXIT_FAILURE;
	}
	
	if (bind(client->sockfd, (const struct sockaddr*)packet->client_packet_source, packet->client_packet_source_len) < 0)
	{
		perror("failed bind socket");
        	close(client->sockfd);
        	return EXIT_FAILURE;
	}
	
	LOG("Client active\n");
	
	
	int width = 0;
	int height = 0;
	size_t buffer_size = 0;
	const char* request = "ready";
	const char* response = "ready";
	
	while (true)
	{
		ssize_t received_bytes = recvfrom(client->sockfd, packet->buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)packet->packet_source, &packet->packet_source_len);
		if (received_bytes < 0)
		{
			perror("failed receive bytes");
		    	break;
		}
		
		packet->buffer[received_bytes] = '\0';
		//LOG("Receive message: %s from %s\n", packet->buffer, inet_ntoa(packet->packet_source->sin_addr));
		if (strcmp(packet->buffer, request) == 0)
		{
			sendto(client->sockfd, response, strlen(response), 0, (struct sockaddr*)packet->packet_source, packet->packet_source_len);
			//LOG("Response 'ready' message: %s to %s\n", response, inet_ntoa(packet->packet_source->sin_addr));
		} 
		
		
		else if (strcmp(packet->buffer, "getscreen") == 0)
		{
			//LOG("Receive message: %s from %s\n", packet->buffer, inet_ntoa(packet->packet_source->sin_addr));
			get_screen(client, &buffer_size, &width, &height);
			
			char buffer_for_param[10];
			char str_width[5]; 
			char str_height[5];
			snprintf(str_width, sizeof(str_width), "%d", width);
			snprintf(str_height, sizeof(str_height), "%d", height);
			memset(buffer_for_param, 0, sizeof(buffer_for_param));
			snprintf(buffer_for_param, sizeof(buffer_for_param), "%04d%04d", width, height);
			//LOG("params: w = %04d | h = %04d\n", width, width);
			
			ssize_t param_send = sendto(client->sockfd, buffer_for_param, sizeof(buffer_for_param), 0, (struct sockaddr*)packet->packet_source, packet->packet_source_len);
			if (param_send < 0)
			{
				perror("sendto param failed");
				continue;
			}
			usleep(1000000);
			//LOG("param_sent: w = %04d | h = %04d\n", width, height);
			
			//size_t timer = 0;
			size_t bytes_sent = 0;
			while (bytes_sent < buffer_size)
			{
				size_t chunk_size = (buffer_size - bytes_sent > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : (buffer_size - bytes_sent);
				ssize_t sent = sendto(client->sockfd, client->buffer_for_server + bytes_sent, chunk_size, 0, (struct sockaddr*)packet->packet_source, packet->packet_source_len);
				
				if (sent < 0)
				{
					perror("sendto failed");
					break;
				}
				
				bytes_sent += sent;
				//LOG("bytes sent: %lu\n", bytes_sent);	
				if (bytes_sent % (30 * MAX_PACKET_SIZE) == 0)
				{
					usleep(250000);
				}
				usleep(100); // 0.1 millsec
			}
			
			//LOG("Response 'screenshot' buffer to %s\n", inet_ntoa(packet->packet_source->sin_addr));
		}
	}
		
	free(packet->buffer);
    	free(packet->packet_source);
    	free(packet->client_packet_source);
    	free(packet);
    	close(client->sockfd);
    	free(client);
	
	return EXIT_SUCCESS;
}

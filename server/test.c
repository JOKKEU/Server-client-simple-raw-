#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void send_ready_to_ip(const char *ip_address) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    // Создаем сокет
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса назначения
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip_address, &server_addr.sin_addr);

    // Отправляем сообщение "ready"
    const char *message = "getscreen";
    sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Закрываем сокет
    close(sockfd);
}

int main() {
    const char *target_ip = "192.168.5.106"; // Целевой IP адрес
    printf("Отправка сообщения на IP: %s\n", target_ip);
    send_ready_to_ip(target_ip);
    
    return 0;
}

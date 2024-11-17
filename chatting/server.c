#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 클라이언트로부터의 메시지를 처리하는 스레드 함수
void *receive_messages(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';  // null 문자로 문자열 끝내기
        printf("Client: %s\n", buffer);  // 클라이언트로부터 받은 메시지 출력
    }

    if (read_size == 0) {
        printf("Client disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(client_sock);
    return 0;
}

// 서버에서 클라이언트에게 메시지를 보내는 함수
void *send_messages(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char message[BUFFER_SIZE];

    while (1) {
        printf("Server: ");
        fgets(message, BUFFER_SIZE, stdin);  // 서버에서 메시지 입력
        message[strcspn(message, "\n")] = '\0';  // '\n' 제거

        if (send(client_sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            close(client_sock);
            return 0;
        }
    }
}

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_size = sizeof(struct sockaddr_in);
    pthread_t recv_thread, send_thread;

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created\n");

    // 서버 주소 구조체 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 소켓 바인딩
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }
    printf("Bind done\n");

    // 연결 대기
    listen(server_sock, 3);
    printf("Waiting for incoming connections...\n");

    // 클라이언트 연결을 기다리고 처리
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_size);
    if (client_sock < 0) {
        perror("Accept failed");
        close(server_sock);
        return 1;
    }
    printf("Connection accepted\n");

    new_sock = malloc(1);
    *new_sock = client_sock;

    // 클라이언트로부터 메시지 수신 스레드 생성
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)new_sock) < 0) {
        perror("Could not create receive thread");
        close(client_sock);
        return 1;
    }

    // 서버에서 클라이언트로 메시지 전송 스레드 생성
    if (pthread_create(&send_thread, NULL, send_messages, (void *)new_sock) < 0) {
        perror("Could not create send thread");
        close(client_sock);
        return 1;
    }

    // 스레드가 종료될 때까지 대기
    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(server_sock);
    return 0;
}

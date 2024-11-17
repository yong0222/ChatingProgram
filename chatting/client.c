#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// 서버로부터 메시지를 수신하는 스레드 함수
void *receive_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';  // null 문자로 문자열 끝내기
        printf("Server: %s\n", buffer);  // 서버로부터 받은 메시지 출력
    }

    if (read_size == 0) {
        printf("Server disconnected\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(sock);
    return 0;
}

// 클라이언트에서 서버로 메시지를 보내는 함수
void *send_messages(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char message[BUFFER_SIZE];

    while (1) {
        printf("Client: ");
        fgets(message, BUFFER_SIZE, stdin);  // 클라이언트에서 메시지 입력
        message[strcspn(message, "\n")] = '\0';  // '\n' 제거

        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            close(sock);
            return 0;
        }
    }
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    pthread_t recv_thread, send_thread;

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created\n");

    // 서버 주소 설정
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return 1;
    }
    printf("Connected to server\n");

    // 서버로부터 메시지 수신 스레드 생성
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&sock) < 0) {
        perror("Could not create receive thread");
        close(sock);
        return 1;
    }

    // 클라이언트에서 서버로 메시지 전송 스레드 생성
    if (pthread_create(&send_thread, NULL, send_messages, (void *)&sock) < 0) {
        perror("Could not create send thread");
        close(sock);
        return 1;
    }

    // 스레드가 종료될 때까지 대기
    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sock);
    return 0;
}

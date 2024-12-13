#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8000
#define BUFF_MAX 256
#define CLIENTS_COUNT 10

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fields_mutex = PTHREAD_MUTEX_INITIALIZER;

long long client_count = 0;

enum Status{
    IN_GAME, READY, ONLINE
};

enum Team {
    Black, White, NoTeam
};

typedef struct Client {
    int socket;
    enum Status status;
    enum Team team;
} Client;

Client clients[CLIENTS_COUNT];

char** black_field, **white_field;

int dividing(){
    int ready_count;
}

char** readline() {
    int buffer_size = BUFF_MAX;
    int cnt;
    char* buffer = malloc(buffer_size + 1);
    char c;
    while ((c = getchar()) != '\n'){
        if (cnt >= buffer_size) {
            buffer_size += BUFF_MAX;
            buffer = (char*)realloc(buffer, buffer_size + 1);
        }
        buffer[cnt++] = c;
    }
    buffer[cnt] = '\0';
    return buffer;
}

void* handle_user(void* arg) {
    Client* client = (Client*)arg;
    char* buffer = malloc(BUFF_MAX);
    int sz = 0;
    while ((sz = read(client -> socket, buffer, BUFF_MAX)) > 0){
        buffer[sz] = '\0';
        if (!strcmp(buffer, "ready")) {
            pthread_mutex_lock(&clients_mutex);
            client -> status = READY;
            pthread_mutex_unlock(&clients_mutex);
            break;
        }
    }

}

int check_online() {
    pthread_mutex_lock(&clients_mutex);
    int client_count = 0;
    int is_black = 0;
    int last_joined = 0;
    for (int i = 0; i < CLIENTS_COUNT; i++){
        if (clients[i].status == READY){
            last_joined = i;
            if (!is_black) {
                clients[i].team = Black;
                is_black = 1;
            } else {
                clients[i].team = White;
            }
            client_count++;
        }
    }
    if (client_count % 2 == 1){
        clients[last_joined].status = ONLINE;
    }
    pthread_mutex_unlock(&clients_mutex);

    return client_count >= 2;
}

void send_fields(Client** clients){
    for (int i = 0; clients[i] != NULL; i++){
        if (clients[i] -> team == Black){
            pthread_mutex_lock(&fields_mutex);
            for (int i = 0; black_field[i] != NULL; i++){
                send(clients[i] -> socket, black_field[i], strlen(black_field[i]), 0);
            }
            pthread_mutex_unlock(&fields_mutex);
        } else {
            pthread_mutex_lock(&fields_mutex);
            for (int i = 0; black_field[i] != NULL; i++){
                send(clients[i] -> socket, black_field[i], strlen(white_field[i]), 0);
            }
            pthread_mutex_unlock(&fields_mutex);
        }
    }
}

int start_game() {
    pthread_mutex_lock(&clients_mutex);
    int client_count = 0;
    Client** client_in_game = malloc(sizeof(Client*) * CLIENTS_COUNT);
    int j = 0;
    for (int i = 0; i < CLIENTS_COUNT; i++){
        if (clients[i].status == READY){
            client_count++;
            clients[i].status = IN_GAME;
            char* msg = clients[i].team == Black ? "your team is black \nStarting Game" : "your team is white \nStarting Game";
            client_in_game[j++] = &clients[i];
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
    client_in_game[j] = NULL;
    pthread_mutex_unlock(&clients_mutex);
    generate_fields(client_count);
    send_fields(client_in_game); 
    return;
}

void generate_fields(int k) {
    pthread_mutex_lock(&clients_mutex);
    int n = k * 6;
    white_field = (char**)malloc((n + 1) * sizeof(char*));
    black_field = (char**)malloc((n + 1) * sizeof(char*));
    for (int i = 0; i < n; i++){
        white_field[i] = malloc((n + 1) * sizeof(char));
        black_field[i] = malloc((n + 1) * sizeof(char));
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* check_start_command(){
    char* str = readline();
    if (!strcmp(str, "start")){
        if (check_online()) {
            start_game();
        } else {
            printf("not enough users with ready");
        }
    }
}


int main () {
    int server_fd;
    if (server_fd = socket(AF_INET, SOCK_STREAM, 0) == 0){
        perror("error with socket");
        exit(1);
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;
    // address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("error with binding socket to port");
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {     
        perror("error with listening");
        exit(1);
    }

    printf("server working successfully \n");

    while(1){
        int new_socket;
        int socket_len = sizeof(address); 
        if ((new_socket = accept(server_fd, (struct socketaddr*)&address, (socklen_t*)&socket_len)) < 0) {
            perror("error with accept connection");
            close(server_fd);
            exit(1);
        }

        if (client_count >= CLIENTS_COUNT){
            perror("to much clients");
            close(new_socket);
            continue;
        }
        
        clients[client_count].socket = new_socket;
        clients[client_count].status = ONLINE;
        clients[client_count].team = NoTeam;
        pthread_t thread;
        pthread_create(&thread, NULL, &handle_user, &clients[client_count]);
        client_count++;

    }


}
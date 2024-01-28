#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

#define URL_LEN 100
#define HOSTNAME_LEN 100
#define PATH_LEN 100
#define REQUEST_LEN 102400
#define RESPONSE_LEN 102400
#define PORT 80
#define BUFFER_SIZE 102400
#define SERVER_IP_LEN 100

// variable declaration
char url[URL_LEN], hostname[HOSTNAME_LEN], path[PATH_LEN], request[REQUEST_LEN], response[RESPONSE_LEN], buffer[BUFFER_SIZE], server_ip[SERVER_IP_LEN];
int sockfd, client_socket, packet_len, tot_len, ans;
struct sockaddr_in server_addr;
char** hyperlink;

void parse(void) {
    char *slash = strchr(url, '/');
    if (slash != NULL) {
        strncpy(hostname, url, slash - url);
        hostname[slash - url] = '\0';
        strcpy(path, slash);
    }
    else {
        strcpy(hostname, url);
        path[0] = '/';
    }
    // printf("%s\n", hostname);
    // printf("%s\n", path);
}
void create_socket(void) {
    // difference between PF_INET and AF_INET,
    // https://spyker729.blogspot.com/2010/07/afinetpfinet.html
    server_addr.sin_family = AF_INET; 
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(PORT);
    if((client_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("create_socket()");
        exit(EXIT_FAILURE);
    }
}
void connect_server(void) {
    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect_server()");
        exit(EXIT_FAILURE);
    }
}
void send_request(void) {
    printf("Sending HTTP request\n");
    if(send(client_socket, request, REQUEST_LEN, 0) == -1) {
        perror("send_request()");
        exit(EXIT_FAILURE);
    }
}
void receive_response(void) {
    printf("Receiving the response\n");
    memset(response, 0, sizeof(response)); // Initialize response to an empty string
    while((packet_len = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) { // Leave room for null terminator
        buffer[packet_len] = '\0'; // Null-terminate the received data
        strncat(response, buffer, packet_len);
        tot_len += packet_len;
    }
    if(packet_len == -1) {
        perror("receive_response()");
        exit(EXIT_FAILURE);
    }
}
void getlink(void) {
    hyperlink = (char**)malloc(1000 * sizeof(char*));
    for(int i = 0; i < 1000; i++) hyperlink[i] = (char*)malloc(1000 * sizeof(char));
    char *cursor = response;
    int i = 0;
    char *a_tag = NULL;
    char *href_tag = NULL;
    char *quote_start = NULL;
    char *quote_end = NULL;

    a_tag = strstr(cursor, "<a");
    while(a_tag != NULL) {
        href_tag = strstr(a_tag, "href=\"");
        if(href_tag != NULL) {
            quote_start = href_tag + strlen("href=\""); // Move the cursor to the start of the URL
            quote_end = strchr(quote_start, '\"'); // Find the end of the URL
            if(quote_end == NULL) {
                perror("getlink()");
                exit(EXIT_FAILURE);
            }
            strncpy(hyperlink[i], quote_start, quote_end - quote_start);
            hyperlink[i][quote_end - quote_start] = '\0'; // Ensure null termination
            cursor = quote_end;
            i++, ans++;
            a_tag = strstr(cursor, "<a");
        } 
        else a_tag = strstr(a_tag + 2, "<a");
    }
}
void hostname_to_ip(void) { 
    // Code reference https://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
    struct hostent *he;
    struct in_addr **addr_list;
    if ((he = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    addr_list = (struct in_addr **)he->h_addr_list;
    strcpy(server_ip, inet_ntoa(*addr_list[0]));
}
void free_memory(void) {
    for(int i = 0; i < 1000; i++) free(hyperlink[i]);
    free(hyperlink);
}

int main() {
    // read the input
    printf("Please enter the URL:\n");
    scanf("%s", url);

    // parse the input into hostname and path
    parse();

    puts("========== Socket ==========");

    // create socket (code reference--- slides p.7 TCP client.c)
    hostname_to_ip();
    create_socket();

    // connect (code reference--- slides p.7 TCP client.c)
    connect_server();

    // request
    memset(request, 0, sizeof(request));
    strcat(request, "GET ");
    strcat(request, path);
    strcat(request, " HTTP/1.1\r\nHost: ");
    strcat(request, hostname);
    strcat(request, "\r\nConnection: close\r\n\r\n");
    send_request();
    // printf("HTTP request:\n%s\n", request);
    
    // receive
    receive_response();

    puts("======== Hyperlinks ========");
    
    // get hyperlink
    //printf("HTTP response:\n%s\n", response);
    getlink();
    
    // print output
    for(int i = 0; i < ans; i++) 
        if(hyperlink[i] != NULL) 
            printf("%s\n", hyperlink[i]);

    puts("============================");

    // print ans
    printf("We have found %d hyperlinks\n", ans);

    // free memory
    free_memory();
    return 0;
}
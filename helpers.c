#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>   
#include <string.h>   
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>      
#include <arpa/inet.h>
#include "helpers.h"
#include "buffer.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

/*Extracts a string by ignoring the whitespace at the beginning and the 
  newline character at the end */
char* extract_string(char* buff){

    // Ignore frontal whitespace
    int i = 0;
    while(buff[i] == ' '){
        i++;
    }
    // Ignore the newline character
    return strtok(&buff[i], "\n");
}

/* Extracts the response code from a HTTP response*/
int extract_status_code(char* response){

    // Empty response
    if(response == NULL)
      return -1;
    
    char status[20], rest[BUFLEN];
    int ret = sscanf(response, "HTTP/1.1 %s %s", status, rest);

    // Parse error
    if(ret == 0)
       return -1;    
    int code = atoi(status);

    // Invalid status code
    if(code < 100 || code > 500)
       return -1;
    else 
       return code;
}

/*Parses the error message from a HTTP response */
void parse_error_msg(char *response){
   
   // Empty response
   if(response == NULL)
      return;
    
   // Extract payload
   const char delim[10] = "\r\n\r\n";
   char *ret;
   ret = strstr(response, delim);
   ret += 4;
   
   // Parse error on payload
   if(ret == NULL)
      return;

   // Print error
   printf("Server responded with: %s. Try again!\n\n", ret);
}

/*Parses the payload from a HTTP response */
JSON_Value* parse_payload(char *response){
   
   // Empty response
   if(response == NULL)
      return NULL;
    
   // Extract payload
   const char delim[10] = "\r\n\r\n";
   char *ret;
   ret = strstr(response, delim);
   ret += 4;
   
   // Error parsing payload
   if(ret == NULL)
      return NULL;
    
    // Convert the payload string into a JSON value
    JSON_Value *val = json_parse_string(ret);
    return val;
}

/*Parses the jwt token from a HTTP response*/
char* parse_jwt_token(char *response){
   
   // Empty response
   if(response == NULL)
      return NULL;
    
   // Extract payload
   const char delim[10] = "token";
   char *ret;
   ret = strstr(response, delim);
   ret += 8;
   
   // Parse error on payload
   if(ret == NULL)
      return NULL;
   
    char* tok;
    tok = strtok(ret, "\"}");
    return tok;
}

/* Shows the current error */
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/* Adds a line to a string message */
void compute_message(char *message, const char *line)
{
    strcat(message, line);
    strcat(message, "\r\n");
}

/* Opens a connection with server host_ip on port portno, returns a socket */
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag)
{
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}


/* Closes a server connection on socket sockfd */
void close_connection(int sockfd)
{
    close(sockfd);
}

/* Sends a message to a server */
void send_to_server(int sockfd, char *message)
{
    int bytes, sent = 0;
    int total = strlen(message);

    do
    {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

/* Receives and returns the message from a server */
char *receive_from_server(int sockfd)
{
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0){
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
        
        header_end = buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;
            
            int content_length_start = buffer_find_insensitive(&buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);
            
            if (content_length_start < 0) {
                continue;           
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length = strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t) header_end;
    
    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t) bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

/* Extracts and returns a JSON from a server response */
char *basic_extract_json_response(char *str)
{
    return strstr(str, "{\"");
}

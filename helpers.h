#include "parson.h"
#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000


#define DIE(condition, msg) \
	do { \
		if (condition) { \
			fprintf(stderr, "(%s:%d): %s\n", __FILE__, __LINE__, msg); \
			perror(""); \
			exit(1); \
		} \
	} while (0)
    
// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type, int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

// extracts a string by ignoring whitespaces and final '\n'
char* extract_string(char* buff);

// extracts the response code from a HTTP response
int extract_status_code(char* response);

// parses the error message from a HTTP response 
void parse_error_msg(char *response);

// parses the payload from a HTTP response
JSON_Value* parse_payload(char *response);

// parses the jwt token from a HTTP response
char* parse_jwt_token(char *response);
#endif

#include <stdio.h>   
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>   
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define SERV_NAME "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com"
#define SERV_PORT 8080
#define JSON_TYPE "application/json"
#define NR_COOKIES 10



int main(int argc, char *argv[]) {

    char *response = (char*) malloc(BUFLEN * sizeof(char));
    char *read_buff = (char*) malloc(BUFLEN * sizeof(char));
    char *message;
    int sockfd;

    // Allocate space for session cookies
    char **session_cookies = (char**) malloc(NR_COOKIES * sizeof(char*));
    for(int i=0; i < NR_COOKIES; i++){
            session_cookies[i] = (char*) calloc(BUFLEN, sizeof(char));
    }
    // Allocate space for the jwt token
    char* jwt_token = (char*) calloc(BUFLEN, sizeof(char));

    // Get the IP address associated with the server's name
    struct hostent *serv_data;
    serv_data = gethostbyname(SERV_NAME);
    char* serv_ip = inet_ntoa(*( struct in_addr*)(serv_data->h_addr_list[0]));
    

    while(1){

        // Read input from stdin
        fgets(read_buff,BUFLEN,stdin);

        // Extract command
        char* command = extract_string(read_buff);
        if(command == NULL)
           continue;
     
       // User inputs 'register' command 
       if(strcmp(command, "register") == 0){

            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");

            // Initialize a json object
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            char *serialized_string = NULL;

            // Read username
            printf("username=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            char* data = extract_string(read_buff);
            
            // Create a json "username" field
            json_object_set_string(root_object, "username", data);

            // Read password
            printf("password=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);

            // Create a json "password" field
            json_object_set_string(root_object, "password", data);
            
            // Convert json payload to string
            serialized_string = json_serialize_to_string_pretty(root_value);
            
            // Send POST to server
            message = compute_post_request(SERV_NAME, "/api/v1/tema/auth/register",
            JSON_TYPE, &serialized_string, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            free(message);
            
            // Receive response from server
            strcpy(response, receive_from_server(sockfd));
            
            // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                json_free_serialized_string(serialized_string);
                json_value_free(root_value);
                close_connection(sockfd);
                continue;
            }          
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nYOUR REGISTRATION WAS SUCCESSFUL!\n\n");
                printf("%s\n", response);
            } else {

                //Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }   
            // Free the json string
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
            
            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }
        

        // User inputs 'login' command 
        if(strcmp(command, "login") == 0){
            
            // Check if session cookies have previously been set
            if(session_cookies[0][0] != '\0'){
                printf("\nAlready logged in! Logout first!\n\n");
                continue;
            }

            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");

            // Initialize a json object
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            char *serialized_string = NULL;

            // Read username
            printf("username=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            char* data = extract_string(read_buff);
            
            // Create a json "username" field
            json_object_set_string(root_object, "username", data);

            // Read password
            printf("password=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);

            // Create a json "password" field
            json_object_set_string(root_object, "password", data);
            
            // Convert json payload to string
            serialized_string = json_serialize_to_string_pretty(root_value);
            
            // Send POST to server
            message = compute_post_request(SERV_NAME, "/api/v1/tema/auth/login",
            JSON_TYPE, &serialized_string, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            free(message);
            
            // Receive response from server
            strcpy(response, receive_from_server(sockfd));
            
            // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                json_free_serialized_string(serialized_string);
                json_value_free(root_value);
                close_connection(sockfd);
                continue;
            }       
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nYOUR LOG IN WAS SUCCESSFUL!\n\n");
                printf("%s\n", response);
                
                // Extract the cookies from the server response
                const char delim[20] = "Set-Cookie: ";
                char *ret;
                ret = strstr(response, delim);
                ret += strlen(delim);
                
                if(ret != NULL){

                    // Reset existing session cookies 
                    for(int i=0; i < NR_COOKIES; i++){
                        memset(session_cookies[i], 0, BUFLEN * sizeof(char));
                    }

                    // Extract the first cookie
                    char* tok = strtok(ret, ";");
                    strcpy(session_cookies[0], tok);

                    // Extract the second cookie
                    tok = strtok(NULL, "; ");
                    strcpy(session_cookies[1], tok);
                    
                    // Extract the third cookie
                    tok = strtok(NULL, " \r\n");
                    strcpy(session_cookies[2], tok);
                }
            } else {

                // Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }
            
            // Free the json string
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
            
            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }
        

        // User inputs 'enter_library' command 
        if(strcmp(command, "enter_library") == 0){
            
            // Check if the jwt token has already been set
            if(jwt_token[0] != '\0'){
                printf("\nYou already have access to the library!\n\n");
                continue;
            }       
            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");
            
            // Check if session cookies have been previously set 
            if(session_cookies[0][0] != '\0'){
                message = compute_get_request(SERV_NAME, "/api/v1/tema/library/access", 
                NULL, session_cookies, 3, NULL);
            } else {
                message = compute_get_request(SERV_NAME, "/api/v1/tema/library/access", 
                NULL, NULL, 0, NULL);
            }        

            // Send GET request to the server
            send_to_server(sockfd, message);
            free(message);
            strcpy(response, receive_from_server(sockfd));

	        // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                close_connection(sockfd);
                continue;
            }
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nYOU'RE IN! WELCOME TO THE LIBRARY!\n\n");
                printf("%s\n\n", response);
                strcpy(jwt_token, parse_jwt_token(response));

            } else {
                // Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }
            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }
        

        // User inputs 'get_books' command 
        if(strcmp(command, "get_books") == 0){
                        
            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");
            
            // Error in case a jwt_token has not been set yet
            if(jwt_token[0] == '\0'){
                printf("\nERROR! Not so fast! You need access to the library first!\n\n");
                continue;
            }      
            // Compute GET request
            message = compute_get_request(SERV_NAME, "/api/v1/tema/library/books", 
                  NULL, NULL, 0, jwt_token);    
            
           // Send GET request to the server
            send_to_server(sockfd, message);
            free(message);
            strcpy(response, receive_from_server(sockfd));

	        // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                close_connection(sockfd);
                continue;
            }
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nTHIS IS YOUR BOOK LIST:\n\n");
                JSON_Value* payload = parse_payload(response);
                printf("%s\n\n",json_serialize_to_string_pretty(payload));
                json_value_free(payload);

            } else {
                // Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }
            // Close the connection and continue reading commands
            close_connection(sockfd); 
            continue;
        }
        

        // User inputs 'get_book' command 
        if(strcmp(command, "get_book") == 0){
            
            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");
       
            // Read id
            printf("id=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            char* data = extract_string(read_buff);
            if(data == NULL){
                printf("\nERROR! The id is required. Try again!\n\n");
                continue;
            }
            int id = atoi(data);
            if(id <= 0){
                printf("\nERROR! The id must be an integer. Try again!\n\n");
                continue;
            }

            // Create the access route based on the book id   
            char path[40];
            sprintf(path, "/api/v1/tema/library/books/%s", data);

            // Error in case a jwt_token has not been set yet
            if(jwt_token[0] == '\0'){
                printf("\nERROR! Not so fast! You need access to the library first!\n\n");
                continue;
            }      
            // Compute GET request
            message = compute_get_request(SERV_NAME, path, 
                  NULL, NULL, 0, jwt_token);    
            
            // Send GET request to the server
            send_to_server(sockfd, message);
            free(message);
            strcpy(response, receive_from_server(sockfd));

	        // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                close_connection(sockfd);
                continue;
            }
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nHERE IS YOUR BOOK. ENJOY THE READ!\n\n");
                JSON_Value* payload = parse_payload(response);
                printf("%s\n\n",json_serialize_to_string_pretty(payload));
                json_value_free(payload);

            } else {
                // Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }
            // Close the connection and continue reading commands
            close_connection(sockfd); 
            continue;
        }


        // User inputs 'add_book' command 
        if(strcmp(command, "add_book") == 0){

            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");

            // Initialize a json object
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            char *serialized_string = NULL;
            
            // Read title
            printf("title=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            char* data = extract_string(read_buff);

             // Create a json "title" field
            json_object_set_string(root_object, "title", data);
                        
            // Read author
            printf("author=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);

            // Create a json "author" field
            json_object_set_string(root_object, "author", data);

            // Read genre
            printf("genre=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);

             // Create a json "genre" field
            json_object_set_string(root_object, "genre", data);

            // Read publisher
            printf("publisher=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);

             // Create a json "publisher" field
            json_object_set_string(root_object, "publisher", data);
            
            // Read page_count
            printf("page_count=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            data = extract_string(read_buff);
            if(data != NULL){
                int id = atoi(data);
                if(id <= 0){
                    printf("\nERROR! The page count must be an int. Try again!\n\n");
                    continue;
                }
            }
            // Create a json "page_count" field
            json_object_set_string(root_object, "page_count", data);

            // Convert json payload to string
            serialized_string = json_serialize_to_string_pretty(root_value);

            // Error in case a jwt_token has not been set yet
            if(jwt_token[0] == '\0'){
                printf("\nERROR! Not so fast! You need access to the library first!\n\n");
                continue;
            }      

            // Send POST to server
            message = compute_post_request(SERV_NAME, "/api/v1/tema/library/books",
            JSON_TYPE, &serialized_string, 1, NULL, 0, jwt_token);
            send_to_server(sockfd, message);
            free(message);
            
            // Receive response from server
            strcpy(response, receive_from_server(sockfd));
            
            // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                json_free_serialized_string(serialized_string);
                json_value_free(root_value);
                close_connection(sockfd);
                continue;
            }          
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nTHE BOOK WAS SUCCESSFULLY ADDED TO YOUR LIBRARY\n\n");
                printf("%s\n", response);
            } else {

                //Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }   
            // Free the json string
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
            
            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }

        // User inputs 'delete_book' command 
        if(strcmp(command, "delete_book") == 0){
            
            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");

            // Read id
            printf("id=");
            memset(read_buff,0, BUFLEN);
            fgets(read_buff,BUFLEN,stdin);
            char* data = extract_string(read_buff);
            if(data == NULL){
                printf("\nERROR! The id is required. Try again!\n\n");
                continue;
            }
            int id = atoi(data);
            if(id <= 0){
                printf("\nERROR! The id must be an integer. Try again!\n\n");
                continue;
            }
            // Create the access route based on the book id   
            char path[40];
            sprintf(path, "/api/v1/tema/library/books/%s", data);

            // Error in case a jwt_token has not been set yet
            if(jwt_token[0] == '\0'){
                printf("\nERROR! Not so fast! You need access to the library first!\n\n");
                continue;
            }

            // Send DELETE to server 
            message = compute_delete_request(SERV_NAME, path,
            JSON_TYPE, NULL, 0, jwt_token);
            send_to_server(sockfd, message);
            free(message);
            
            // Receive response from server
            strcpy(response, receive_from_server(sockfd));
            
            // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                close_connection(sockfd);
                continue;
            }          
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nTHE BOOK WAS SUCCESSFULLY DELETED FROM YOUR LIBRARY\n\n");
                printf("%s\n", response);
            } else {

                //Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }  
            
            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }
        
        // User inputs 'logout' command
        if(strcmp(command, "logout") == 0){

            // Open a connection to the server 
            sockfd = open_connection(serv_ip, SERV_PORT, AF_INET, SOCK_STREAM, 0); 
            DIE(sockfd < 0, "Invalid file descriptor");
            
            // Check if session cookies have been previously set 
            if(session_cookies[0][0] != '\0'){
                message = compute_get_request(SERV_NAME, "/api/v1/tema/auth/logout", 
                NULL, session_cookies, 3, NULL);
            } else {
                message = compute_get_request(SERV_NAME, "/api/v1/tema/auth/logout", 
                NULL, NULL, 0, NULL);
            }        
            // Send GET request to the server
            send_to_server(sockfd, message);
            free(message);
            strcpy(response, receive_from_server(sockfd));

	        // Examine the response status code
            int status_code = extract_status_code(response);
            
            // In case of an invalid code, continue 
            if(status_code == -1){
                printf("ERROR receiving response from server\n");
                close_connection(sockfd);
                continue;
            }
            // Check if the response was successful
            if(status_code/100 == 2){
                printf("\nSUCCESSFULLY LOGGED OUT.\n\n");
                printf("%s\n\n", response);

            } else {
                // Parse and print the error message received
                printf("\nERROR %d: ", status_code);
                parse_error_msg(response);
            }
            
            // Reset existing session cookies 
            for(int i=0; i < NR_COOKIES; i++){
                memset(session_cookies[i], 0, BUFLEN * sizeof(char));
            }
            
            // Reset the jwt token
            memset(jwt_token, 0, BUFLEN * sizeof(char));

            // Close the connection and continue reading commands
            close_connection(sockfd);
            continue;
        }
        
        // User inputs 'exit' command
        if(strcmp(command, "exit") == 0){
            break;
        }
        
        // User inputs an invalid command
        printf("ERROR: Not a valid command! ");
        printf("Make sure not to leave any trailing whitespace!\n\n");
        
    }
      
    // Free allocated data
     free(read_buff);
     free(response);
     for(int i=0; i < NR_COOKIES; i++){
            free(session_cookies[i]);
     }
     free(session_cookies);
     free(jwt_token);

     // Close the connection
     close_connection(sockfd);
     return 0;
}

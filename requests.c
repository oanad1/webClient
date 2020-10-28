#include <stdlib.h>     
#include <stdio.h>
#include <unistd.h>     
#include <string.h>    
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>      
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"


/* Computes a GET request given a host, an url, query parameters (optional), 
    a list of cookies (optional) and an authorization token (optional) */

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count, char* auth)
{
    char *message = calloc(BUFLEN, sizeof(char));    
    char *line = calloc(LINELEN, sizeof(char));     

    // Write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Add the Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    
    // Add the Authorization header
    if(auth != NULL){
        sprintf(line, "Authorization: Bearer %s", auth);
        compute_message(message, line);
    }
    // Add cookies, according to the protocol format
    if (cookies != NULL) {
        char* list = malloc(BUFLEN * cookies_count);
        
        strcpy(list, cookies[0]);
        if(cookies_count > 1){
            strcat(list, "; "); 
        }
        for(int i=1; i < cookies_count-1; i++){
            strcat(list, cookies[i]);
            strcat(list, "; ");
        }    
        if(cookies_count > 1){
            strcat(list, cookies[cookies_count-1]);
        } 
        sprintf(line,"Cookie: %s",list);
        compute_message(message, line);
        free(list);
    }
    // Add the final new line
    compute_message(message, "");
    
    // Free allocated data
    free(line);
    
    // Return the computed message
    return message;
}


/* Computes a POST request given a host, an url, the content type, a list of strings for
   body data, a list of cookies (optional) and an authorization token (optional) */

char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, 
                            char* auth)
{
    
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line); 
    
    // Add the Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Compute the size of the payload
    int data_len = 0;
    for(int i=0; i < body_data_fields_count; i++){
       data_len += strlen(body_data[i]);
    }
    // Add the Authorization header
    if(auth != NULL){
        sprintf(line, "Authorization: Bearer %s", auth);
        compute_message(message, line);
    } 
    // Add the Content Length header
    sprintf(line, "Content-Length: %d",  data_len);
    compute_message(message, line);

    // Add the Content Type header
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Add cookies, according to the protocol format
    if (cookies != NULL) {
        char* list = malloc(BUFLEN * cookies_count);
        
        strcpy(list, cookies[0]);
        if(cookies_count > 1){
            strcat(list, "; "); 
        }
        for(int i=1; i < cookies_count-1; i++){
            strcat(list, cookies[i]);
            strcat(list, "; ");
        }    
        if(cookies_count > 1){
            strcat(list, cookies[cookies_count-1]);
        } 
        sprintf(line,"Cookie: %s",list);
        compute_message(message, line);
        free(list);
    }
    //Add a new line at end of the headers
    compute_message(message, "");

    // Add the actual payload data
    strcpy(body_data_buffer, body_data[0]);
    for(int i = 1; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);
    }
    compute_message(message, body_data_buffer);
    
    // Free allocated memory
    free(line);
    free(body_data_buffer);
    
    // Return the computed message
    return message;
}


/* Computes a DELETE request given a host, an url, the content type, 
   a list of cookies (optional) and an authorization token (optional) */

char *compute_delete_request(char *host, char *url, char* content_type, 
                             char **cookies, int cookies_count, char* auth)
{
    
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Write the method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Add the Authorization header
    if(auth != NULL){
        sprintf(line, "Authorization: Bearer %s", auth);
        compute_message(message, line);
    }
    
    // Add the Content Type header
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Add the host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Add cookies, according to the protocol format
    if (cookies != NULL) {
        char* list = malloc(BUFLEN * cookies_count);
        
        strcpy(list, cookies[0]);
        if(cookies_count > 1){
            strcat(list, "; "); 
        }
        for(int i=1; i < cookies_count-1; i++){
            strcat(list, cookies[i]);
            strcat(list, "; ");
        }    
        if(cookies_count > 1){
            strcat(list, cookies[cookies_count-1]);
        } 
        sprintf(line,"Cookie: %s",list);
        compute_message(message, line);
        free(list);
    }
    // Add a new line at end of header
    compute_message(message, "");
    
    // Free allocated data
    free(line); 

    // Return the computed message
    return message;
}
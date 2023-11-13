#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct URI {
    char host[MAXLINE]; //hostname
    char port[MAXLINE]; //端口
    char path[MAXLINE]; //路径
};

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, struct URI *uri_);
void build_header(char *http_header, struct URI *uri_data, rio_t *client_rio);

// void get_filetype(char *filename, char *filetype);
// void serve_dynamic(int fd, char *filename, char *cgiargs);
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
	    clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	    doit(connfd);                                         
	    Close(connfd);                                          
    }
    return 0;
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int connfd) 
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char server[MAXLINE];

    rio_t rio, server_rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, connfd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  // size_t == 0 return;
        return;
    
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")) {               
        printf("Proxy does not implement this method");
        return;
    }      

    // parse uri
    struct URI *uri_ = (struct URI *)malloc(sizeof(struct URI));
    parse_uri(uri, uri_);

    //设置header
    build_header(server, uri_, &rio);

    
    // proxy try to connect server
    int serverfd = Open_clientfd(uri_->host, uri_->port);
    if (serverfd < 0) {
        printf("Connection failed");
        return;
    }
    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, server, strlen(server));

    size_t n;
    // response to client
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
    {
        printf("proxy forwarded %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
    //关闭服务器描述符
    Close(serverfd);
}
/* $end doit */

void parse_uri(char *uri, struct URI *uri_) {
    char *pos = strstr(uri, "//");
    if (!pos) {
        char *path_pos = strstr(uri, "/");
        if (path_pos) {
            strcpy(uri_->path, path_pos);
        }
        strcpy(uri_->port, "80");
        return;
    }
    else {
        char *port_pos = strstr(pos + 2, ":");
        if (port_pos) {
            int tmp;
            sscanf(port_pos + 1, "%d%s", &tmp, uri_->path);
            sprintf(uri_->port, "%d", tmp);
            *port_pos = '\0';
        } else {
            char *path_pos = strstr(pos + 2, "/");
            if (path_pos) {
                strcpy(uri_->path, path_pos);
                strcpy(uri_->port, "80");
                *path_pos = '\0';
            }
        }
        strcpy(uri_->host, pos + 2);
    }
    return;
}

void build_header(char *http_header, struct URI *uri_data, rio_t *client_rio)
{
    char *User_Agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
    char *conn_hdr = "Connection: close\r\n";
    char *prox_hdr = "Proxy-Connection: close\r\n";
    char *host_hdr_format = "Host: %s\r\n";
    char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
    char *endof_hdr = "\r\n";

    char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];
    sprintf(request_hdr, requestlint_hdr_format, uri_data->path);
    while (Rio_readlineb(client_rio, buf, MAXLINE) > 0)
    {
        if (strcmp(buf, endof_hdr) == 0)
            break; /*EOF*/

        if (!strncasecmp(buf, "Host", strlen("Host"))) /*Host:*/
        {
            strcpy(host_hdr, buf);
            continue;
        }

        if (!strncasecmp(buf, "Connection", strlen("Connection")) && !strncasecmp(buf, "Proxy-Connection", strlen("Proxy-Connection")) && !strncasecmp(buf, "User-Agent", strlen("User-Agent")))
        {
            strcat(other_hdr, buf);
        }
    }
    if (strlen(host_hdr) == 0)
    {
        sprintf(host_hdr, host_hdr_format, uri_data->host);
    }
    sprintf(http_header, "%s%s%s%s%s%s%s",
            request_hdr,
            host_hdr,
            conn_hdr,
            prox_hdr,
            User_Agent,
            other_hdr,
            endof_hdr);

    return;
}
/*
 * Created by Yuran Pereira.
 * GitHub:
 */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>

#include "client.h"

#define BUFLEN          65536
#define PORT            "9000"
#define SOCK_BACKLOG    128
#define OUTPUT_FILE     "/var/tmp/aesdsocketdata"

/*
 * This structure stores server information
 */
struct server {
    int                 sock_fd;
    int                 client_fd;
    unsigned long       port;
    struct addrinfo *   addrinfo;
    struct client *     client;
};

/**
 * Allocates an addrinfo structure.
 * Should only be called by the function that creates
 * the server.
 *
 * @port - the port on which to listen
 */
struct addrinfo *server_allocaddrinfo(char *port)
{
    struct addrinfo hints, *addrinfo;

    /* Setup hints and get addr info */
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    /* Get addr info */
    if (getaddrinfo(NULL, port, &hints, &addrinfo) != 0) {
        syslog(LOG_ERR, "Failed to allocate addrinfo.");
        return NULL;
    }

    return addrinfo;
}

/**
 * Frees the server's addrinfo structure
 */
void server_freeaddrinfo(struct addrinfo *addrinfo)
{
    freeaddrinfo(addrinfo);
}

/**
 * Creates a server that listen for connections on
 * a given port.
 *
 * Returns a pointer to a new server structure or NULL
 * on failure.
 * 
 * @port - The port on which to listen for connections
 */
struct server *server_create(char *port)
{
    struct addrinfo *addrinfo;
    struct server *server;
    int sock_fd;
    struct pollfd pfd[1];

    /* Allocate server */
    server = malloc(sizeof(struct server));
    if (!server) {
        syslog(LOG_ERR, "Failed to allocate server structure.");
        return NULL;
    }

    /* Allocate addrinfo */
    addrinfo = server_allocaddrinfo(port);
    if (!addrinfo) {
        syslog(LOG_ERR, "Failed to allocate addrinfo.");
        return NULL;
    }

    /* Open socket */
    sock_fd = socket(addrinfo->ai_family, addrinfo->ai_socktype, 
            addrinfo->ai_protocol);
    if (sock_fd < 0) {
        syslog(LOG_ERR, "Failed to create socket: %s", 
                strerror(errno));
        return NULL;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        syslog(LOG_ERR, "Failed to create socket: %s", 
                strerror(errno));

    /* Bind name to socket*/
    if (bind(sock_fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0) {
        syslog(LOG_ERR, "Failed to bind name to socket: %s", 
                strerror(errno));
        return NULL;
    }

    /* Wait for socket to bind */
    pfd[0].fd = sock_fd;
    pfd[0].events = POLL_IN | POLL_OUT;

    if (poll(pfd, 1, 2))
        syslog(LOG_ALERT, "Polled successfully");

    /* Listen */
    if (listen(sock_fd, SOCK_BACKLOG) != 0) {
        syslog(LOG_ERR, "Failed to listen: %s", strerror(errno));
        return NULL;
    }

    server->sock_fd = sock_fd;
    server->port = strtol(port, NULL, 10);
    server->addrinfo = addrinfo;

    return server;
}

/**
 * Terminate the server, close all open sockets
 * and free all allocations made by server_create.
 *
 * @server - The server structure to be destroyed
 */
void server_destroy(struct server *server)
{
    shutdown(server->sock_fd, SHUT_RDWR);
    server_freeaddrinfo(server->addrinfo);
    free(server);
}

/**
 * Allocates a new client and establishes a client-server connection.
 *
 * Returns the pointer to the newly created client structure.
 *
 * @server - the server structure
 */
struct client *server_connect(struct server *server)
{
    struct client *client = client_create();
    socklen_t addrlen = sizeof(struct sockaddr);

    /* Accept connection */
    client->sock_fd = accept(server->sock_fd, &client->sockaddr, &addrlen);
    if (client->sock_fd < 0) {
        syslog(LOG_ERR, "Could not accept connection %s", 
                strerror(errno));
        return NULL;
    }
    client->sockaddr_in = (struct sockaddr_in *)&client->sockaddr;

    return client;
}

/**
 * Disconnects the client from server and destroys
 * 
 * @client - the client to be disconnected
 */
void server_disconnect(struct client *client)
{
    shutdown(client->sock_fd, SHUT_RDWR);
    client_destroy(client);
}

/*
 * Reads data from socket until newline into buffer
 *
 * @sock_fd - socket file descriptor from which to read data
 * @buffer  - buffer on which to store the data received
 */
int socket_readline(int sock_fd, char *buffer)
{
    char ch;
    int len = 0;
     while(read(sock_fd, &ch, 1)) {
        if (ch == '\n' || ch == '\0') 
            break;

        if (len >= BUFLEN-1)
            break;
        buffer[len++] = ch;
    }
    return len;
}

/*
 * Writes data in buffer to a socket file descriptor
 *
 * @sock_fd - socket file descriptor
 * @buffer  - buffer from which to read data
 * @len     - length of data to write
 */
int socket_write(int sock_fd, char *buffer, size_t len)
{
    syslog(LOG_INFO, "sending %s", buffer);
    return write(sock_fd, buffer, len);
}

/**
 * Reads contents of the file into a dynamically allocated buffer.
 * Caller must ensure to free() the buffer after use.
 *
 * @file    - file pointer from which to read.
 * @len     - amount of data to read
 */
char *file_read(FILE *file, size_t len) 
{
    char *buffer = malloc(len);
    if (!buffer)
        return NULL;
    rewind(file);
    fread(buffer, 1, len, file);

    return buffer;
}

/*
 * Writes the content of a buffer into a file and ensures that
 * a newline is added to the end of the file.
 */
void file_writeline(FILE *file, char *buffer, size_t len)
{
    fwrite(buffer, 1, len, file);
    fwrite("\n", 1, 1, file);
}


/*
 * Get the size of a file
 */
size_t file_getsize(FILE *file)
{
    size_t size;

    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    return size;
}

/*
 * Clear all data from a given file
 */
void file_clean(FILE *file)
{
    ftruncate(fileno(file), 0);
}

/* 
 * This structure stores the data to be used by a signal
 * handler. It must be set using before the signal handler
 * is registered.
 */
struct sigaction_data {
    struct server *server;
    FILE *file;
}sigaction_data;

void signal_handler(int sig)
{
    syslog(LOG_ALERT, "Caught signal, exiting");

    file_clean(sigaction_data.file);
    shutdown(sigaction_data.server->client->sock_fd, SHUT_RDWR);
    server_disconnect(sigaction_data.server->client);
    server_destroy(sigaction_data.server);
    fclose(sigaction_data.file);
    closelog();
    exit(0);
}

int daemonize()
{
    pid_t pid = fork();

    if (pid > 0)
        exit(0);
    else if (pid == 0) {
        setsid();
        chdir("/");

        /* Redirect std*. */
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        freopen("/dev/null", "r", stdin);

        return 0;
    }
}

int main(int argc, char **argv)
{
    char data[BUFLEN], client_ip[64];
    char *args, *filedata;
    FILE *file;
    int log;
    size_t filesize, len;
    struct server *server;
    struct client *client;
    struct sigaction s_action;

    log = LOG_USER;
    if (argc > 1) 
        /* Daemonize process if -d is specified */
        if (!strcmp(argv[1], "-d"))
            log = LOG_DAEMON;

    /* Setup logging facilities */
    openlog(NULL, 0, log);

    /* Open output file to append */
    file = fopen(OUTPUT_FILE, "a+");
    if (!file) {
        syslog(LOG_ERR, "Failed to create output file: %s", 
                strerror(errno));
        closelog();
        goto ret_err;
    }

    /* Create a new server */
    server = server_create(PORT);
    if (!server) {
        goto ret_err_file;
    }

    if (log == LOG_DAEMON)
        daemonize();

    memset(data, 0, BUFLEN);

    /* Set the server variable to be used by the signal handler */
    sigaction_data.server = server;
    sigaction_data.file = file;

    /* Register signals to be handled */
    memset(&s_action, 0, sizeof(struct sigaction));
    s_action.sa_handler = signal_handler;

    sigaction(SIGTERM, &s_action, NULL);
    sigaction(SIGINT, &s_action, NULL);

    /* Connection loop */
    while (true) {
        /* Accept connection */
        client = server_connect(server);
        server->client = client;

        /* Log client IP */
        client_getip(client, client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        len = socket_readline(client->sock_fd, data);
        filesize = file_getsize(file);
        file_writeline(file, data, len);

        filesize = file_getsize(file);
        filedata = file_read(file, filesize);
        socket_write(client->sock_fd, filedata, filesize);
        free(filedata);

        /* Drop Connection */
        server_disconnect(client);

        /* Log disconnection */
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

ret_err_server:
    server_destroy(server);
ret_err_file:
    fclose(file);
ret_err:
    closelog();
    return -1;
}

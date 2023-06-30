/*
 * Created by Yuran Pereira.
 * GitHub: 
 */

/* Represents a connected client */
struct client {
    int                 sock_fd;
    struct sockaddr     sockaddr;
    struct sockaddr_in  *sockaddr_in;
};

/*
 * Allocate a new client structure 
 */
struct client *client_create()
{
    struct client *client;

    client = malloc(sizeof(struct client));
    if (!client) {
        syslog(LOG_ERR, "Failed to create client: %s", 
                strerror(errno));
        return NULL;
    }

    return client;
}

/*
 * Free a client structure 
 */
void client_destroy(struct client *client)
{
    free(client);
}

/**
 * Stores the string equivalent of a client's IP into a buffer.
 *
 * @client      - client structure
 * @ip_buffer   - buffer on which to store the IP string
 */
void client_getip(struct client *client, char *ip_buffer)
{
    inet_ntop(AF_INET, &client->sockaddr_in->sin_addr, ip_buffer, INET_ADDRSTRLEN);
}

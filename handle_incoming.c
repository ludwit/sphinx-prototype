#include "header.h"

/* Local helper functions */
int forward_pkg(char *recv_buf, char *forward_buf);
char add_surb_to_contacts(char * recv_buf);


void * handle_incoming(void *ptr) 
{
    struct sockaddr_in6 addr;
    char                recv_buf[SPX_PKT_SIZE];
    char                forward_buf[SPX_PKT_SIZE];
    char                alias;
    int                 list_port;
    int                 listenfd, connfd;

    /* Creates socket for listening */
    if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("spx: Error creating listening socket\nsocket");
        exit(EXIT_FAILURE);
    }

    /* Makes address reusable */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("spx: Address not reusable\nsetsockopt");
    }

    /* Saves listening address */
    list_port = *((int *) ptr);

    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_loopback;
    addr.sin6_port = htons(SPX_PORT + list_port);

    /* Binds socket to address */
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("spx: Error binding listening socket\nbind");
        exit(EXIT_FAILURE);
    }

    /* Listens for incoming connections */
    if (listen(listenfd, LISTENQ) < 0) {
        perror("spx: Error listening for connections\nlisten");
        exit(EXIT_FAILURE);
    }

    /* Endless loop */
    for ( ; ; ) {

        /* Resets recieving buffer */
        bzero(&recv_buf, sizeof(recv_buf));

        /* Resets forwarding buffer */
        bzero(&forward_buf, sizeof(forward_buf));

        /* Accepts incoming connection */
        if ( (connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) < 0) {
            perror("spx: Error accepting connection\naccept");
            close(connfd);
            continue;
        }

        /* Recieves incoming message */
        if (recv(connfd, &recv_buf, SPX_PKT_SIZE, 0) < 0) {
            perror("spx: Error recieving message\nrecv");
            close(connfd);
            continue;
        }

        /* Checks if recieved package is to be forwarded */
        if (isdigit(recv_buf[2])) {
            /* Yes, then forward it */

            /* Forwards package to next address */
            if (forward_pkg(&recv_buf[0], &forward_buf[0]) < 0) {
                fprintf(stderr, "spx: Error forwarding message\n");
                close(connfd);
                continue;
            }

        } else {
            /* No, then save SURB and print message */

            /* Saves SURB in contacts and creates alias */
            if ( (alias = add_surb_to_contacts(&recv_buf[0])) < 0) {
                fprintf(stderr, "spx: Error saving SURB\n");
                exit(EXIT_FAILURE);
            }
            
            /* Locks I/O mutex */
            if (pthread_mutex_lock(&io_mutex) < 0) {
                perror("spx: Error locking io_mutex\npthread_mutex_lock");
                exit(EXIT_FAILURE);
            }

            /* Prints message to stdout */
            if (printf("Message recieved: %sReply with alias: %s\n", &recv_buf[SPX_HEADER_SIZE], &alias) < 0) {
                perror("spx: Error printing recieved message\nprintf");
                close(connfd);
                continue;
            }

            /* Unlocks I/O mutex */
            if (pthread_mutex_unlock(&io_mutex) < 0) {
                perror("spx: Error unlocking io_mutex\npthread_mutex_unlock");
                exit(EXIT_FAILURE);
            }
        }

        /* Closes connection */
        close(connfd);
    }

    /* Never gets here */
    pthread_exit(NULL);
}

int forward_pkg(char *recv_buf, char *forward_buf)
{   
    int i;
    int dest_port;

    printf("message forwarded\n");

    /* Iterates through sphinx package header */
    for (i = 0; i < SPX_NUM_HOPS; i++) {

        /* Crosses out address in header of forward package at position i */
        forward_buf[i] = 'x';

        /* Checks for valid address in recieved package header at position i */
        if (isdigit(recv_buf[i])) {
            /* Yes, then forward package there */

            /* Saves destination address */
            dest_port = recv_buf[i] - '0';

            /* Copies recieved package to forward package from position i+1 */
            strcpy(&forward_buf[i+1], &recv_buf[i+1]);

            /* Sends forward package to destination */
            if (send_pkg_to(forward_buf, dest_port) < 0) {
                return -1;
            }

            /* Return indicating success */
            return 0;
        }
            /* No, then try next position */
    }

    /* Returns indicating failure as no vaild address was found */
    return -1;
}


char add_surb_to_contacts(char * recv_buf)
{   
    contact new_contact;

    /* Fills new contact */
    new_contact.alias = 'a' + contacts_iterator;
    strncpy(&new_contact.surb[0], &recv_buf[SPX_NUM_HOPS], SPX_SURB_SIZE);

    /* Locks contacts mutex */
    if (pthread_mutex_lock(&contacts_mutex) < 0) {
        perror("pthread_mutex_lock");
        return -1;
    }

    /* Saves new contact in contacts */
    contacts[contacts_iterator] = new_contact;

    /* Unocks contacts mutex */
    if (pthread_mutex_unlock(&contacts_mutex) < 0) {
        perror("pthread_mutex_unlock");
        return -1;
    }

    /* Increases iterator */
    contacts_iterator = (contacts_iterator + 1) % SPX_CONTACTS_SIZE;

    /* Returns alias indicting success */
    return new_contact.alias;
}
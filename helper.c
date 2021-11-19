#include "header.h"

int send_pkg_to(char *pkg_buf, int dest_port) {

    struct sockaddr_in6 addr;
    int                 sockfd;

    /* Creates socket for sending message */
    if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(SPX_PORT + dest_port);
    addr.sin6_addr = in6addr_loopback;

    /* Connects to destination */
    if ( (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    /* Sends message to destination */
    if ( (send(sockfd, pkg_buf, SPX_PKT_SIZE, 0)) < 0) {
        perror("send");
        close(sockfd);
        return -1;
    }

    printf("sent pkg to %d\n", dest_port);

    /* Closes connection */
    close(sockfd);

    /* Returns indicating success */
    return 0;
}

int char_to_num(char c) {
    return (int) c - '0';
}

char num_to_char(int n) {
    return (char) n + '0';
}
#include "header.h"

/* Local helper functions */
int get_address(char c);
int create_spx_pkg(char *in_buf, char *pkg_buf, int list_port, int dest_port);
void shuffle_array(int *arr, int arr_size);


void * handle_outgoing(void *ptr)
{
    fd_set rd_set;
    char   in_buf[SPX_INPUT_SIZE];
    char   pkg_buf[SPX_PKT_SIZE];
    int    list_port;
    int    dest_port;


    /* Saves listening port */
    list_port = *((int *) ptr);

    /* Initialises read set to include stdin */
    FD_ZERO(&rd_set);
    FD_SET(fileno(stdin), &rd_set);

    /* Endless loop */
    for ( ; ; ) {

        /* Resets input buffer */
        bzero(&in_buf, sizeof(in_buf));

        /* Resets package buffer */
        bzero(&pkg_buf, sizeof(pkg_buf));

        /* Blocks until keystroke from user */
        if (select(fileno(stdin)+1, &rd_set, NULL, NULL, NULL) < 0) {
            perror("spx: Error waiting for input\nselect");
            continue;
        }

        /* Locks I/O mutex */
        if (pthread_mutex_lock(&io_mutex) < 0) {
            perror("spx: Error locking io_mutex\npthread_mutex_lock");
            exit(EXIT_FAILURE);
        }

        /* Gets message and destination from stdin */
        if ( (fgets(in_buf, SPX_INPUT_SIZE, stdin)) == NULL && ferror(stdin)) {
            perror("spx: Message not sent\nfgets");
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        /* Checks if message is too long */
        if (strcspn(in_buf,"\n") == SPX_INPUT_SIZE - 1) {
            fprintf(stderr, "\nspx: Message not sent\ninput: Message too long\n");
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        /* Checks input for sufficient length to hold a message */
        if (strlen(in_buf) < 4) {
            fprintf(stderr, "spx: Message not sent\ninput: No message detected\n");
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        /* Checks if first character of input is a valid address */
        if ( (dest_port = get_address(in_buf[0])) < 0) {
            fprintf(stderr, "spx: Message not sent\n");
            pthread_mutex_unlock(&io_mutex);
            continue;
        }

        /* Unlocks I/O mutex */
        if (pthread_mutex_unlock(&io_mutex) < 0) {
            perror("spx: Error unlocking io_mutex\npthread_mutex_unlock");
            exit(EXIT_FAILURE);
        }

        /* Builds sphinx package and recieves address of first hop */
        dest_port = create_spx_pkg(&in_buf[0], &pkg_buf[0], list_port, dest_port);

        /* Sends package to destination */
        if (send_pkg_to(&pkg_buf[0], dest_port) < 0) {
            fprintf(stderr, "spx: Message not sent\n");
            continue;
        }
    }

    /* Never gets here */
    pthread_exit(NULL);
}



int create_spx_pkg(char *in_buf, char *pkg_buf, int list_port, int dest_port)
{
    contact reply_contact;
    int num_mixes;
    int i;
    int j;

    /* Calculates number of avalible mixes */
    if (list_port != dest_port) {
        num_mixes = NETWORK_SIZE - 2;
    } else {
        num_mixes = NETWORK_SIZE - 1;
    }

    /* Creates array for avalible mixes */
    int mixnodes[num_mixes];

    /* Fills array with addresses of avalible mixes */
    for (i = 0, j = 0; i < NETWORK_SIZE; i++) {
        if ( (i != list_port) && (i != dest_port) ) {
            mixnodes[j] = i;
            j++;
        }
    }

    /* Shuffles array of addresses for Reply-Block creation */
    shuffle_array(&mixnodes[0], num_mixes);

    /* Puts three addresses for Reply-Block in sphinx header */
    for (i = 0; i < SPX_NUM_HOPS; i++) {
        pkg_buf[SPX_NUM_HOPS + i] = num_to_char(mixnodes[i]);
    }

    /* Puts own destination address in Reply-Block */
    pkg_buf[SPX_HEADER_SIZE - 1] = num_to_char(list_port);

    /* Checks if destination address is alias */
    if (dest_port < NETWORK_SIZE) {
        /* No, then create sphinx header */

        /* Shuffles array of addresses for selcting hops */
        shuffle_array(&mixnodes[0], num_mixes);

        /* Puts second to last hop addresses in sphinx header */
        for (i = 0; i < SPX_NUM_HOPS - 1; i++) {
            pkg_buf[i] = num_to_char(mixnodes[i]);
        }

        /* Puts final destination address in sphinx header */
        pkg_buf[SPX_NUM_HOPS - 1] = num_to_char(dest_port);

    } else {
        /* Yes, then use SURB for header */

        /* Locks contacts mutex */
        if (pthread_mutex_lock(&contacts_mutex) < 0) {
            perror("pthread_mutex_lock");
            return -1;
        }

        /* Saves reply contact from contacts */
        reply_contact = contacts[dest_port - 'a'];

        /* Erases reply contact from contacts */
        bzero(&contacts[dest_port - 'a'], sizeof(reply_contact));

        /* Unocks contacts mutex */
        if (pthread_mutex_unlock(&contacts_mutex) < 0) {
            perror("pthread_mutex_unlock");
            return -1;
        }

        /* Copies all but first addresses from SURB to sphinx header */
        for (i = 0; i < SPX_NUM_HOPS; i++) {
            pkg_buf[i] = reply_contact.surb[i + 1];
        }

        /* Saves first hop address as destination */
        dest_port = reply_contact.surb[0];
    }
   
    /* Copies message to sphinx pakage body */
    strcpy(&pkg_buf[SPX_HEADER_SIZE], &in_buf[SPX_ADRESS_SIZE + SPX_INPUT_PADDING]);

    printf("pkg = %s", pkg_buf);

    /* Returns address of first hop */
    return mixnodes[SPX_NUM_HOPS - 1];
}

/* Shuffles array pseudo randomly */
void shuffle_array(int *arr, int arr_size) {
    int i;
    int j;
    int temp;

    for (i = 0; i < arr_size - 1; i++) {
        j = i + rand() / (RAND_MAX / (arr_size - i) + 1);
        temp = arr[j];
        arr[j] = arr[i];
        arr[i] = temp;
    }
}

int get_address(char c)
{   
    /* Checks if character is a number */
    if (isdigit(c)) {

        /* Checks if number is in network address range */
        if (char_to_num(c) < NETWORK_SIZE) {
            return char_to_num(c);
        } else {
            fprintf(stderr, "input: Address out of network range\n");
            return -1;
        }
    }

    /* Checks if character is an english alphabet lower case letter */
    if (c >= 'a' && c <= 'z') {
        
        /* Checks if character is alias in contacts */
        if (contacts[c - 'a'].alias == c) {
            return c;
        } else {
            fprintf(stderr, "input: Alias not found\n");
            return -1;
        }
    }

    /* Indicates wrong format was used */
    fprintf(stderr, "input: Invalid address format\n");
    return -1;
}
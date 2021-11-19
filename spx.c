#include "header.h" 

/* Defines contacts for storing SURBs as global variable */
contact contacts[SPX_CONTACTS_SIZE]; 

/* Defines number of contacts as global variable */
int contacts_iterator;

/* Defines contacts mutex as global variable */
pthread_mutex_t contacts_mutex;
 
/* Defines I/O mutex as global variable */
pthread_mutex_t io_mutex; 

/* Local helper function */
int set_terminal_noncanonical();


int main(int argc, char **argv)
{  
    pthread_t recv_thread, send_thread; 
    int  list_port;
    int       ret;
    
    /* Checks if only one argument was passed */
    if (argc != 2) {
        fprintf(stderr, "spx: Wrong number of arguments\nusage: spx <0-9>\n");
        exit(EXIT_FAILURE);
    }

    /* Checks if arguments first character is a digit */
    if (!isdigit(argv[1][0])) {
        fprintf(stderr, "spx: Invalid address value\nusage: spx <0-9>\n");
        exit(EXIT_FAILURE);
    }

    /* Makes user input avalible on keystroke */
    if (set_terminal_noncanonical() < 0) {
        exit(EXIT_FAILURE);
    }

    /* Initialises contacts iterator as zero */
    contacts_iterator = 0;

    /* Initialises mutex for contacts */
    if (pthread_mutex_init(&contacts_mutex, NULL) < 0) {
        perror("spx: Error initialising contacts mutex\npthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    /* Initialises mutex for terminal I/O */
    if (pthread_mutex_init(&io_mutex, NULL) < 0) {
        perror("spx: Error initialising I/O mutex\npthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    /* Creates thread for handling incoming messages */
    list_port = char_to_num(argv[1][0]);

    /* Creates thread for handling incoming messages */
    if ( (ret = pthread_create(&recv_thread, NULL, &handle_incoming, &list_port)) != 0) {
        errno = ret;
	    perror("spx: Error creating recieving thread\npthread_create");
        exit(EXIT_FAILURE);
    }

    /* Creates thread for handling outgoing messages */
    if ( (ret = pthread_create(&send_thread, NULL, &handle_outgoing, &list_port)) != 0) {
        errno = ret;
	    perror("spx: Error creating sending thread\npthread_create");
        exit(EXIT_FAILURE);
    }

    /* Waits for threads to finish */
    if ( (ret = pthread_join(send_thread, NULL)) != 0) {
        errno = ret;
	    perror("spx: Error joining sending thread\npthread_join");
        exit(EXIT_FAILURE);
    }

    if ( (ret = pthread_join(recv_thread, NULL)) != 0) {
        errno = ret;
	    perror("spx: Error joining recieving thread\npthread_join");
        exit(EXIT_FAILURE);
    }

    /* Destroys I/O mutex */
    if (pthread_mutex_destroy(&io_mutex) < 0) {
        perror("spx: Error destroying I/O mutex\npthread_mutex_destroy");
        exit(EXIT_FAILURE);
    }

    /* Never gets here */
    exit(EXIT_SUCCESS);
}

/* Makes user input avalible on keystroke */
/* Only supportet by UNIX like OS!!!      */
int set_terminal_noncanonical()
{
    struct termios  attr;

    bzero(&attr, sizeof(attr));

    /* Gets terminal attributs */
    if (tcgetattr(0, &attr) < 0) {
        perror("spx: Error getting terminal settings\ntcgetattr");
        return(-1);
    }

    /* Changes attributs to noncanonical */
    attr.c_lflag &= ~ICANON;

    /* Set terminal attributs */
    if (tcsetattr(0, TCSANOW, &attr) < 0) {
        perror("spx: Error setting terminal settings\ntcsetattr");
        return(-1);
    }

    /* Returns indicating success */
    return(0);
}
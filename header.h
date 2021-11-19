/* Libaries */
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>
#include	<sys/uio.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>
#include    <pthread.h> 
#include    <ctype.h>
#include    <sys/select.h>
#include    <sys/time.h>
#include    <termios.h>

/* Macros */
    /* Modifier */
#define SPX_PORT          30000
#define LISTENQ           10
#define SPX_NUM_HOPS      3
#define NETWORK_SIZE      5      /* >= SPX_NUM_HOPS + 2 */
#define SPX_MSG_SIZE      142
    /* Fixed */
#define SPX_CONTACTS_SIZE 26
#define SPX_ADRESS_SIZE   1
#define SPX_INPUT_PADDING 1
#define SPX_SURB_SIZE     SPX_NUM_HOPS + SPX_ADRESS_SIZE
#define SPX_HEADER_SIZE   SPX_NUM_HOPS + SPX_SURB_SIZE
#define SPX_PKT_SIZE      SPX_MSG_SIZE + SPX_HEADER_SIZE
#define SPX_INPUT_SIZE    SPX_MSG_SIZE + SPX_ADRESS_SIZE + SPX_INPUT_PADDING

/* Struct */
typedef struct contact {
    char alias;
    char surb[SPX_SURB_SIZE + 1];  /* + 1 is for Null termination */
} contact;

/* Global variables */
extern contact contacts[];
extern int contacts_iterator;
extern pthread_mutex_t contacts_mutex;
extern pthread_mutex_t io_mutex;

/* Thread functions */
void * handle_outgoing(void *ptr);
void * handle_incoming(void *ptr);

/* Global helper functions */
int send_pkg_to(char *pkg_buf, int dest_port);
int char_to_num(char c);
char num_to_char(int n);
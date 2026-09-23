/***************************************************************************
 *  socket_connections.c
 *
 *  Created: Aug, 30 2026 AD
 *
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************
 * TCP telnet connections for the eagle emulator
 */

#include <sys/types.h>
#if defined(_WIN32) || defined(_WIN64)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
    typedef SOCKET socket_t;
    typedef WSAPOLLFD pollfd_t;
    #define poll_sockets WSAPoll
    #define sockErrno WSAGetLastError()
#ifdef _WIN64
    #define FMT_SOCKET "%llu"
    #define FMT_SOCKET5 "%5llu"
#else
	#define FMT_SOCKET "%u"
    #define FMT_SOCKET5 "%5u"
#endif
#else
	#include <signal.h>
    #include <poll.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <unistd.h>
    typedef int socket_t;
    typedef struct pollfd pollfd_t;
    #define poll_sockets poll
    #include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#define sockErrno errno
	#define sockStrerror strerror
	#define FMT_SOCKET "%d"
	#define FMT_SOCKET5 "%5d"
	#define INVALID_SOCKET -1
#endif


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "socket_connections.h"
#include "sim.h"
#include "telnetDefs.h"
#include <assert.h>
#include "util.h"
#include "esc_sequences.h"
#include "vtparse/vtparse.h"
#include "charringbuffer.h"

//#define SOCK_DEBUG


static volatile int pollThreadTerminate;
static volatile int pollThreadRunning;
#ifndef _WIN32
/********************************************************
 Thread stuff for linux
 ********************************************************/
static pthread_mutex_t criticalSectionMutex;

#define ENTER_CRIT pthread_mutex_lock (&criticalSectionMutex);
#define LEAVE_CRIT pthread_mutex_unlock (&criticalSectionMutex);

static void initCriticalSections() {
	pthread_mutex_init (&criticalSectionMutex, NULL);
}

static void deInitCriticalSections() {
	pthread_mutex_destroy (&criticalSectionMutex);
}

static pthread_t pollThreadId;

static void *pollThread (void *ptr);


int sock_pollThreadStart () {
	//puts("start");
	int rc;
	pollThreadTerminate = 0;
	pollThreadRunning = 0;
	if ((rc = pthread_create(&pollThreadId, NULL, &pollThread, NULL)) != 0) {
		fprintf(stderr,"pthread_create() failed with %d (%s)",rc,strerror(rc));
		return -1;
	}
	return 0;
}


#else
/********************************************************
 Thread stuff for Windows
 ********************************************************/
static CRITICAL_SECTION criticalSection;

#define ENTER_CRIT EnterCriticalSection(&criticalSection);
#define LEAVE_CRIT LeaveCriticalSection(&criticalSection);

static void initCriticalSections() {
	InitializeCriticalSection (&criticalSection);
}

static void deInitCriticalSections() {
	DeleteCriticalSection (&criticalSection);
}

static DWORD pollThreadId;

static DWORD WINAPI pollThread(LPVOID ptr);


int sock_pollThreadStart () {
	pollThreadTerminate = 0;
	pollThreadRunning = 0;
	if (CreateThread(NULL,0,pollThread,NULL,0,&pollThreadId) == 0) return -1;
	return 0;
}

#endif

static void sock_poll();

#ifdef _WIN32
DWORD WINAPI pollThread(LPVOID ptr) {
#else
static void *pollThread (void *ptr) {
#endif // _WIN32
	//puts("pollThread started");
	pollThreadRunning++;
	while (!pollThreadTerminate) {
		usleep(10000);
		sock_poll();
	}
	pollThreadRunning--;
	//puts("pollThread terminated");
	return 0;
}

void sock_pollThreadEnd () {
	if (pollThreadRunning) {
		pollThreadTerminate = 1;
		while (pollThreadRunning)usleep(10000);
	}
}


// for holding escape sequences
#define OUT_BUF_SIZE 64

enum sockstat_t {
	STAT_CLOSED = 0,
	STAT_WAITCONN,
	STAT_OPEN,
	STAT_DATA_AVAILABLE
};

#define SEND_DATA_BUF_DEF_SIZE 32768
#define RECV_BUFFER_SIZE 32768

typedef struct {
	socket_t fd;
	enum sockstat_t status;
	int portNum;
	int revents;
	bool doTelnetInit;				// send telnet init sequence on connect
	bool doOutTranslation;			// translate evdt escape sequences to VT100
	bool doInTranslation;			// translate pressed keys to evdt codes
	bfseq_State_t outSeqSta;		// state for outgoing escape sequences
	int dumpIO_console;				// dump send and received chars to console
	int saveSendData;				// save send data to buffer
	int sendDataBufferSize;
	int sendDataBufferLen;
	char *sendDataBufferPos;
	char *sendDataBuffer;
	vtparse_t parser;				// parser for incoming key translation
	ring_buffer_t recvData;			// ringbuffer contains the translated keycodes
	char recvDataBuffer[RECV_BUFFER_SIZE];	// buffer used by ringbuffer
	int recvFlag;                   // currently bit 0=backspace toggle

} sock_t;

sock_t socks[SOCK_MAX];

static int sock_initialize_done;

// incoming key translation

#define CTRL_A "\001"
#define CTRL_B "\002"
#define CTRL_C "\003"
#define CTRL_D "\004"
#define CTRL_E "\005"
#define CTRL_F "\006"
#define CTRL_G "\007"
#define CTRL_H "\010"
#define CTRL_I "\011"
#define CTRL_J "\012"
#define CTRL_K "\013"
#define CTRL_L "\014"
#define CTRL_M "\015"
#define CTRL_N "\016"
#define CTRL_O "\017"
#define CTRL_P "\020"
#define CTRL_Q "\021"
#define CTRL_R "\022"
#define CTRL_S "\023"
#define CTRL_T "\024"
#define CTRL_U "\025"
#define CTRL_V "\026"
#define CTRL_W "\027"
#define CTRL_X "\030"
#define CTRL_Y "\031"
#define CTRL_Z "\032"
#define ALT "\035"
#define ESC "\033"



/* Sequence     seq   numParams   p1   p2
   ======================================
   ESC [1;2P     P        2       1    2
   ESC OP        O        1       'P'  0
*/

// F12 will toggle backspace between 0x08 (default for boss/ix shell) and 0x7f
#define FLAG_TOGGLE_BACKSPACE 0x0001

#define FLAG_TOGGLE_BACKSPACE_ACTIVE 0x0001

typedef struct {
	char seq;
	int numParams;
	char p1;
	char p2;
	char *bfSequence;
	int flags;
} vtToBfSeq_t;

/*
The following table summarizes all VED commands:

      CTRL           ESC CTRL        ALT            ALT ESC
                                      (drops mark first)
A   start of line  start of para   start of line   start of para
B   beginning      bottom          beginning       bottom
C   exit           backup file     ~               ~
D   delete char    delete words    ~               ~
E   end of line    end of para     end of line     end of para
F   forward char   forward word    forward char    forward word
G   get file       change file     edit macro file ~
H   back char      back word       back char       back word
I   tab            ~               tab             ~
J   save char      save word       jump to mark    switch mark
                                                   and cursor
K   kill line      kill para       kill selection  ~
L   adjust window  re-display      ~               ~
M   cr             change modes    sets mark       ~
N   down line      down para       down line       down para
O   open line      ~               open line       ~
P   up line        up para         up line         up para
Q   XON            ~               ~               ~
R   forward srch   reverse srch    forward srch    reverse srch
S   XOFF           ~               ~               ~
T   save line      save para       save selection  save selection
U   #=mult         mult=1          ~               ~
V   paste          paste and       replace select  replace select
                   clear buffer    with buffer     clear buffer
W   write file     write take buf  write selection ~
X   execute cmd    execute buf     exec selection  ~
Y   suspend        help            ~               ~
Z   re-search      rvrsd           re-search       rvrsd
@   where from     where from      where mark      where mark
    top            bot             from top        from bot
[   ESC            ~               ~               ~
]   ALT            ALT ESC         ~               ~
\   change buffer  clr present buf ~               ~
^   quote char     ~               ~               ~
_   undo delete    ~               undo mark       ~
DEL erase char*    erase word*     ~               ~

    *  note: The CTRL key is not a part of the "erase char" and
             "erase word" commands. Most terminal types will ignore
             the CTRL key if it is used with the DEL key, so ved
             will get the DEL code anyway. If the terminal does
             not ignore the CTRL key, then the usr must not
             combine CTRL with the DEL key.


*/

vtToBfSeq_t vtToBfTab_ESC[] = {
	{ 'O',1,'P',0, "\034" },	// ESC OP - F1 -> MB I   0x1c
	{ 'O',1,'Q',0, "\035" },	// ESC OP - F2 -> MB II  0x1d
	{ 'O',1,'R',0, "\036" },	// ESC OP - F3 -> MB III 0x1e
	{ 'O',1,'S',0, "\037" },	// ESC OP - F4 -> MB VI  0x1f
	{ 0,0,0,0,NULL }

};

vtToBfSeq_t vtToBfTab_CSI[] = {
	// cursor keys for the unos/bossix editor ved
	{ 'A',0,0,0, CTRL_P },	// cursor up
	{ 'B',0,0,0, CTRL_N },	// cursor down
	{ 'C',0,0,0, CTRL_F },	// cursor right
	{ 'D',0,0,0, CTRL_H },	// cursor left
	{ 'A',2,1,5, ESC CTRL_P },	// ctrl cursor up
	{ 'B',2,1,5, ESC CTRL_N },	// ctrl cursor down
	{ 'C',2,1,5, ESC CTRL_F },	// ctrl cursor right
	{ 'D',2,1,5, ESC CTRL_H },	// ctrl cursor left
	{ 'A',2,1,3, ESC CTRL_P },	// alt cursor up
	{ 'B',2,1,3, ESC CTRL_N },	// alt cursor down
	{ 'C',2,1,3, ESC CTRL_F },	// alt cursor right
	{ 'D',2,1,3, ESC CTRL_H },	// alt cursor left
	{ 'P',2,1,2, CTRL_P },	// shift cursor up
	{ 'B',2,1,2, CTRL_N },	// shift cursor down
	{ 'C',2,1,2, CTRL_P },	// shift cursor right
	{ 'D',2,1,2, CTRL_H },	// shift cursor left
	{ '~',1,5,0, CTRL_P  CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P CTRL_P},	// Pg Up - there is no page down in ved, only next paragraph, so go 22 lines down
	{ '~',1,6,0, CTRL_N  CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N CTRL_N},	// Pg Dn
	{ '~',2,5,5, "" },	// ctrl pg up
	{ '~',2,6,5, "" },	// ctrl pg dn
	{ '~',2,5,3, "" },	// alt pg up
	{ '~',2,6,3, "" },	// alt pg dn
	{ 'H',0,0,0, CTRL_A },	// home - start of line
	{ 'F',0,0,0, CTRL_E },	// end - end of line
	{ 'H',2,1,5, CTRL_B },	// ctrl home - start of file
	{ 'F',2,1,5, ESC CTRL_B },	// ctrl end - end of file
	{ 'H',2,1,5, ESC CTRL_A },	// alt home - start of para
	{ 'F',2,1,5, ESC CTRL_E },	// alt end - end of para
	{ '~',1,3,0, CTRL_D },      // del - delete char for basic and ved
	{ '~',2,3,5, ESC CTRL_D },      // ctrl del - delete word for ved
	{ '~',2,3,2, CTRL_K },      // shift del -kill line
	{ '~',1,2,0, CTRL_I },      // ins for basic

	{ '~',1,24,0,"",FLAG_TOGGLE_BACKSPACE },
	{ 0,0,0,0,NULL }

};


static void translateVtAndAdd (sock_t *sock, vtToBfSeq_t tab[], char seq, int numParams, char p1, char p2) {
	int i = 0;

	while (tab[i].bfSequence) {
		if (tab[i].seq == seq && tab[i].numParams == numParams) {
			int found = 1;
			if (numParams == 1 && tab[i].p1 != p1) found = 0;
			else if (numParams == 2 && tab[i].p2 != p2) found = 0;

			if (found) {
                if (tab[i].flags & FLAG_TOGGLE_BACKSPACE) {     // F12 toogles backspace
                    if (sock->recvFlag & FLAG_TOGGLE_BACKSPACE_ACTIVE) sock->recvFlag &= ~FLAG_TOGGLE_BACKSPACE;
                    else sock->recvFlag |= FLAG_TOGGLE_BACKSPACE;
                } else {
                	ENTER_CRIT
                    ring_buffer_queue_arr(&sock->recvData, tab[i].bfSequence, strlen(tab[i].bfSequence));
                    LEAVE_CRIT
#ifdef SOCK_DEBUG
                    printf("adding %d bytes to ringbuffer (",(int)strlen(tab[i].bfSequence));
                    char *p = tab[i].bfSequence;
                    while (*p) {
                        if (*p == 27) printf("ESC ");
                        else if (*p < 32) printf("0x%2x ",*p);
                        else printf("%c ",*p);

                        p++;
                    }
                    printf(")\n");
#endif
                    return;
                }
			}
		}
		i++;
	}
}

static void translateAndAdd (sock_t *sock, char *data, int size) {
	char *p = data;
	int processed;


#ifdef SOCK_DEBUG
    int i;
	printf("translateAndAdd %d bytes\n",size);
#endif
	if (size == 1)
		if (*data == 27) {		// single escape, this needs to be changed if serial connections will be added as this only works for a packet based connection
			ring_buffer_queue(&sock->recvData, 27);
			return;
		}

	while(size) {
		if (*p == 0x7f && (sock->recvFlag & FLAG_TOGGLE_BACKSPACE))
            ring_buffer_queue(&sock->recvData, 0x08);
		else if (*p == 0x08 && (sock->recvFlag % FLAG_TOGGLE_BACKSPACE)) ring_buffer_queue(&sock->recvData, 0x7f);
		else {
            processed = vtparse(&sock->parser, (unsigned char *)p, 1);
#ifdef SOCK_DEBUG
            //printf(" [%s] ", STATE_NAMES[sock->parser.state]);
#endif
            if (processed) {
#if 0
                printf("Received action %s %s\n", ACTION_NAMES[sock->parser.action],STATE_NAMES[sock->parser.state]);
                printf("%d Parameters for '%c' '%c': ", sock->parser.num_params,sock->parser.ch2,sock->parser.ch);
                for(i = 0; i < sock->parser.num_params; i++)
                    printf(" %d ", sock->parser.params[i]);

                printf("\n");
#endif
                switch (sock->parser.action) {
                    case VTPARSE_ACTION_PRINT:
                    case VTPARSE_ACTION_EXECUTE:
                        ring_buffer_queue(&sock->recvData, sock->parser.ch);
#ifdef SOCK_DEBUG
                        if (sock->parser.ch < 33 || sock->parser.ch > 126) printf("added 0x%02x\n",sock->parser.ch);
                        else printf("added '%c'\n",sock->parser.ch);
#endif
                        break;
                    case VTPARSE_ACTION_ESC_DISPATCH:	// this for F1..F4 (ESC OP, ESC OQ, ESC OR, ESC OS)
                        if (sock->parser.ch2 == 'O') translateVtAndAdd (sock, vtToBfTab_ESC, 'O',1,sock->parser.ch,0);
                        break;
                    case VTPARSE_ACTION_CSI_DISPATCH:	// this is for ESC[
                        translateVtAndAdd (sock, vtToBfTab_CSI, sock->parser.ch,sock->parser.num_params,sock->parser.params[0],sock->parser.params[1]);
                        break;
                    default:
                        break;

                }
            }
        }
        p++;
        size--;
	}
}

#ifdef _WIN32
static char * sockStrerrorLast;

char * sockStrerror (int errCode) {
	if (sockStrerrorLast) { LocalFree(sockStrerrorLast); sockStrerrorLast = NULL; }

	DWORD result = FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&sockStrerrorLast,0, NULL);

    if (result > 0 && sockStrerrorLast != NULL) {
		char *p = sockStrerrorLast;
		while (*p) {
			if (*p == '\r' || *p =='\n') *p = ' ';
			p++;
		}
		return sockStrerrorLast;
    } else {
    	//puts(" no err ");
      return "";
    }
}
#endif


// returns listenfd or -1 on error
socket_t setupListenSocket(int portNum) {
	socket_t listenfd;
    int res=0;
    int on;
    struct sockaddr_in6 serv_addr;
    char sendBuff [255];

    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(sendBuff, 0, sizeof(sendBuff));
	listenfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listenfd < 0) {
  	 	 fprintf(stderr,"socket failed with code " FMT_SOCKET ", errno: %d %s\n",listenfd, sockErrno,sockStrerror(sockErrno));
  	 	 return -1;
	}

	/****************************************************************************/
	/* The setsockopt() function is used to allow the local address to          */
	/* be reused when the server is restart_commanded before the required wait  */
	/* time expires.                                                            */
	/****************************************************************************/
    on=1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,(char *)&on,sizeof(on)) < 0) {
         fprintf(stderr,"setsockopt(SO_REUSEADDR) failed, sockErrno: %d %s\n",sockErrno,sockStrerror(sockErrno));
         return -1;
    }

    /*********************************************************************/
    /* on=1 would allow ipv6 only. Otherwise both, ipv4 and ipv6 clients */
    /* can connect                                                       */
	/*********************************************************************/
    on=0;
    res = setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on,sizeof(on));
    if (res < 0) {
  	 	 fprintf(stderr,"setsockopt(IPV6_V6ONLY = %d) failed with code %d, sockErrno: %d %s\n",on,res,sockErrno,sockStrerror(sockErrno));
	}

	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_port   = htons(portNum);
	/********************************************************************/
	/* Note: applications use in6addr_any similarly to the way they use */
	/* INADDR_ANY in IPv4.  A symbolic constant IN6ADDR_ANY_INIT also   */
	/* exists but can only be used to initialize an in6_addr structure  */
	/* at declaration time (not during an assignment).                  */
	/********************************************************************/
    serv_addr.sin6_addr   = in6addr_any;

    res = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (res < 0) {
  	 	 fprintf(stderr,"bind to port %d failed with code %d, sockErrno: %d %s\n",portNum, res, sockErrno, sockStrerror(sockErrno));
  	 	 close(listenfd);
  	 	 return -1;
	}

    res = listen(listenfd, 1);
    if (res < 0) {
  	 	 fprintf(stderr,"listen failed with code %d, sockErrno: %d %s\n",res, sockErrno,sockStrerror(sockErrno));
  	 	 close(listenfd);
  	 	 return -1;
	}

    return listenfd;
}


#include <stdio.h>
#include <string.h>

// Cross-platform headers
#ifdef _WIN32
    #include <winsock2.h>
    typedef int socklen_t;
#else
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
    typedef int SOCKET;
    #define INVALID_SOCKET -1
#endif

// Returns 1 on success (data received), 0 on timeout, -1 on error
int recvWithTimeout (socket_t client_sock, char *buffer, size_t buffer_size, int *bytes_received, int timeoutMs) {
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(client_sock, &read_fds);
    *bytes_received = 0;

    timeout.tv_sec = 0;
    timeout.tv_usec = timeoutMs * 1000;

    int activity = select((int)(client_sock + 1), &read_fds, NULL, NULL, &timeout);

    if (activity < 0) return -1;

    else if (activity == 0) {
        // timeout passed without the client sending anything
        return 0;
    }

    // 4. Data is ready to read
    if (FD_ISSET(client_sock, &read_fds)) {
        #ifdef _WIN32
            int res = recv(client_sock, buffer, (int)buffer_size, 0);
        #else
            ssize_t res = recv(client_sock, buffer, buffer_size, 0);
        #endif

        if (res > 0) {
            *bytes_received = (int)res;
            return 1; // Success
        } else if (res == 0) {
            return -1;  // lient disconnected
        } else {
            //perror("recv error");
            return -1;
        }
    }

    return -1;
}

void dumpBuffer (char *info, char *c, int len) {
	if (info) printf(info);
	for (int i=0; i<len; i++) {
		printf("%02x ",*(uint8_t *)c); c++;
	}
	printf("\n");
}

/* keep it simple: initialize telnet session, most important: disable remote echo

sample from DD-WRT and linux telnet:
SERVER CONNECTION ESTABLISHED
SERVER IAC DO 1 (ECHO)
SERVER IAC DO 31 (NAWS)
SERVER IAC WILL 1 (ECHO)
SERVER IAC WILL 3 (SGA)
CLIENT IAC WONT 1 (ECHO)
CLIENT IAC WILL 31 (NAWS)
CLIENT SUB 31 (NAWS) [4 bytes]:  [<0x00><0x50><0x00><0x18>]
CLIENT IAC DO 1 (ECHO)
CLIENT IAC DO 3 (SGA)
SERVER DATA:
 [<0x0D><0x0D><0x0A>
]
SERVER DATA: DD-WR

WILL SGA
DO   ECHO
WILL BINARY
DO   BINARY
*/

#define TELNET_RECV_TIMEOUT_MS 250

void telnetClientInit(socket_t fd) {
	int res;
	int dataSize;

        uint8_t myOptions [] = {
                        TELNET_IAC, TELNET_DO, TELNET_TELOPT_ECHO,		// client should not echo
                        TELNET_IAC, TELNET_DONT, TELNET_TELOPT_NAWS,	// window size, we should disable this
                        TELNET_IAC, TELNET_WILL,TELNET_TELOPT_ECHO,     // without that, linux telnet will show ^M on enter key
                        TELNET_IAC, TELNET_WILL,TELNET_TELOPT_SGA,
                        TELNET_IAC, TELNET_DO,  TELNET_TELOPT_SGA,		// for eagleemu, suppress CRLF on enter
                        TELNET_IAC, TELNET_DO,   TELNET_TELOPT_BINARY, 	// for eagleemu, suppress CRLF on enter
						TELNET_IAC, TELNET_WILL, TELNET_TELOPT_BINARY   // for eagleemu, suppress CRLF on enter
        };
        char reply[255];
#ifdef SOCK_DEBUG
        res = recvWithTimeout (fd, (char *)&reply, 255, &dataSize, TELNET_RECV_TIMEOUT_MS);
        if (res && (dataSize > 0)) dumpBuffer("got before sending opts: ",reply,dataSize);
#else
		recvWithTimeout (fd, (char *)&reply, 255, &dataSize, 50);
#endif

        res = send(fd, (void *)&myOptions, sizeof(myOptions), 0);
        if (res != sizeof(myOptions)) {
			printf("telnetClientInit: send returned %d, expected %d, errno %d %s\n",res,(int)sizeof(myOptions),sockErrno,sockStrerror(sockErrno));
        }
#ifdef SOCK_DEBUG
        res = recvWithTimeout (fd, (char *)&reply, 255, &dataSize, TELNET_RECV_TIMEOUT_MS);
        if (res && (dataSize > 0)) dumpBuffer("got after sending opts: ",reply,dataSize);
#else
		recvWithTimeout (fd, (char *)&reply, 255, &dataSize, TELNET_RECV_TIMEOUT_MS);
#endif

        // FIXME: select with timeout or some other not that stupid solutions
        //usleep(100000);   // Avoid getting the answer as keypresses, telnet needs some time

#ifdef SOCK_DEBUG
        // we should get at least one packet from the client with answers
        res = recv(fd, (void *)&reply, sizeof(reply), 0);
		printf("telnetClientInit response for fd " FMT_SOCKET ", size %d (IGNORED, ASSUMED TO BE OK)\n",fd,res);
#else
        // we should get at least one packet from the client with answers
        recv(fd, (void *)&reply, sizeof(reply), 0);
#endif
/*
        ESP_LOGD(TAG, "telnetClientInit response for fd %d, size %d (IGNORED, ASSUMED TO BE OK)",fd,rc);
        uint8_t *c;

        if (rc > 0) {
                c = &reply[0];
                for (int i=0; i<rc; i++) {
                        printf("%02x ",*c); c++;
                }
                printf("\n");
        }
*/
}

void sock_setupListen (int portNum, bool doClose) {
	if (!sock_initialize_done) return;
	assert(portNum >= 0 && portNum < SOCK_MAX);
	if (doClose) { close(socks[portNum].fd); socks[portNum].fd = -1; }

	int newFd = setupListenSocket(socks[portNum].portNum);
	if (newFd < 0) {
		//fprintf(stderr,"setupListenSocket failed with sockErrno %d %s\n",sockErrno,sockStrerror(sockErrno));
		socks[portNum].fd = -1;
		socks[portNum].status = STAT_CLOSED;
	} else {
		socks[portNum].fd = newFd;
		socks[portNum].status = STAT_WAITCONN;
	}
}

#define RECV_BUFSIZE 128

// check for incomping connections or data on all open ports and set the status field for each connection
static void sock_poll() {
	if (!sock_initialize_done) return;
	pollfd_t *pfds;
	int i,fd,numFds=0,res;
	int portIdx[SOCK_MAX];
	char recvBuffer[RECV_BUFSIZE];

	for (i=0;i<SOCK_MAX;i++) portIdx[i] = -1;

	// we can only poll an open socket
	for (i=0;i<SOCK_MAX;i++) {
		if (socks[i].status != STAT_CLOSED) numFds++;
	}
	if (!numFds) return;
	// allocate the array of fd's for poll
	pfds = calloc(numFds, sizeof(struct pollfd));
	if (pfds == NULL) return;

	fd=0;
	for (i=0;i<SOCK_MAX;i++) {
		if (socks[i].status != STAT_CLOSED) {
			pfds[fd].fd = socks[i].fd;
			pfds[fd].events = POLLIN; // | POLLRDHUP;
			portIdx[fd] = i;
			fd++;
		}
	}
	res = poll_sockets(pfds,numFds,0);
	if (res < 0) {
//#ifdef SOCK_DEBUG
		fprintf(stderr,"poll returned %d, sockErrno: %d %s\n",res,sockErrno,sockStrerror(sockErrno));
//#endif
		return;
	}

	for(i=0;i<numFds;i++) {
		//if (pfds[i].revents != 0) printf("%d revents: 0x%08x ",i,pfds[i].revents);
		socks[portIdx[i]].revents = pfds[i].revents;
		if (pfds[i].revents && POLLIN) {						// on Linux i only get POLLIN even if the telnet client is already terminated
			if (socks[portIdx[i]].status == STAT_WAITCONN) {	// socket is listening, we need to accept the connection
				int newFd = accept(pfds[i].fd, (struct sockaddr*)NULL, NULL);
				if (newFd < 0) {
					fprintf(stderr,"accept failed with sockErrno %d %s\n",sockErrno,sockStrerror(sockErrno));
					// close the listen socket and try to create it again
					sock_setupListen (portIdx[i],true);
				} else {	// accept was ok
					// close the listen socket and set the port socket to the connected one
					close(socks[portIdx[i]].fd);
					socks[portIdx[i]].fd = newFd;
					socks[portIdx[i]].status = STAT_OPEN;
					#ifndef _WIN32
					int flags = fcntl( newFd, F_GETFL);
					flags |= O_NONBLOCK;
					fcntl(newFd, F_SETFL, flags);
					#else
					ULONG mode = 1;
					ioctlsocket(newFd, FIONBIO, &mode);
					#endif
#ifdef SOCK_DEBUG
					printf("%d is connected\n",portIdx[i]);
#endif
					if (socks[portIdx[i]].doTelnetInit) telnetClientInit(newFd);

					char buff[255];
					char device[30];
					if (portIdx[i] == 0) strcpy(device,"scc0");
					else if (portIdx[i] == 1) strcpy(device,"scc1");
					else if (portIdx[i] < 6) sprintf(device,"fw1:%d",portIdx[i]-2);
					else if (portIdx[i] < 10) sprintf(device,"fw2:%d",portIdx[i]-6);
					else if (portIdx[i] < 14) sprintf(device,"fw3:%d",portIdx[i]-10);
					else sprintf(device,"fw4:%d",portIdx[i]-14);

					/* set window caption (of cause, does not work with windows telnet) */
					sprintf(buff,"\033]0;%s" \
								"\007\033[2J\033[H\033[1mWelcome to eagleemu on %s" \
								"\033[m\r\nF12 toggles backspace between 0x08 and 0x7f\r\n\n",device,device);
					send(newFd,buff,strlen(buff),0);
				}

			} else
			if (socks[portIdx[i]].status == STAT_OPEN || socks[portIdx[i]].status == STAT_DATA_AVAILABLE) {
				// receive the data here to get the full escape sequence and check for errors
				int rc = recv(pfds[i].fd, (char *)&recvBuffer, sizeof(recvBuffer), 0); //MSG_DONTWAIT);
				if (rc > 0) {
					socks[portIdx[i]].status = STAT_DATA_AVAILABLE;
					/* This is an ugly hack to fix Microsoft telnet

					   According to official Telnet protocol specifications (RFC 854), when binary transmission mode is negotiated,
					   the client should pass keys transparently. However, the Windows Telnet client defaults to a strict "New line mode".
					   When you press Enter (which maps to 0x0A or \n natively on Linux telnet), the Windows engine automatically expands
					   it to a carriage return + line feed (0x0D 0x0A)
					*/
					if (rc == 2)
						if (recvBuffer[0] == 0x0d && recvBuffer[1] == 0x0a)
							rc--;

#ifdef SOCK_DEBUG
					dumpBuffer("Received: ",(char *)&recvBuffer, rc);
#endif
					if (socks[i].doInTranslation) {
						translateAndAdd (&socks[portIdx[i]], recvBuffer, rc);
					} else {
						ENTER_CRIT
						ring_buffer_queue_arr(&socks[portIdx[i]].recvData, recvBuffer, rc);
						LEAVE_CRIT
					}
				} else {
					printf("port %d disconnected\n",i);
					// close the listen socket and try to create it again
					sock_setupListen (portIdx[i],true);
				}
			}
		}
	}
}


void dumpChar(char prefix, char data) {
	char tmpBuf[20];

	snprintf(tmpBuf,sizeof(tmpBuf)-1,"%c: 0x%02x %c ",prefix,data,data > ' ' && data < 127 ? data : ' ');
	printf("%s",tmpBuf);
	fflush(stdout);
}

void dumpStr(char prefix, char *data) {
	while (*data) {
		dumpChar(prefix,*data);
		data++;
	}
}

void sock_putchar(int portNum, char data) {
	if (portNum < 0 || portNum > SOCK_MAX-1) return;
	if (socks[portNum].status != STAT_DATA_AVAILABLE && socks[portNum].status != STAT_OPEN) return;

	ENTER_CRIT
	if (socks[portNum].sendDataBuffer) {	// save data send to terminal if enabled
		if (socks[portNum].sendDataBufferLen < socks[portNum].sendDataBufferSize) {
			*socks[portNum].sendDataBufferPos = data;
			socks[portNum].sendDataBufferPos++;
			socks[portNum].sendDataBufferLen++;
		}
	}
	LEAVE_CRIT

	if (socks[portNum].doOutTranslation) {		// if we have outgoing escape sequence translation
		char outSeqBuffer[OUT_BUF_SIZE];
												// bfseq_processChar will process the character and fill outSeqBuffer when we have a complete sequence

		int bufLen = bfseq_processChar (&socks[portNum].outSeqSta,data, outSeqBuffer, sizeof(outSeqBuffer));
		if (socks[portNum].dumpIO_console) dumpChar('s',data);
		if (bufLen) {
			send(socks[portNum].fd,(char *)&outSeqBuffer,bufLen,0); //MSG_DONTWAIT);
			//if (socks[portNum].dumpIO_console) dumpStr('s',outSeqBuffer);
		}

	} else {
		send(socks[portNum].fd,&data,1,0); //MSG_DONTWAIT);
		if (socks[portNum].dumpIO_console) dumpChar('s',data);
	}
}

void sock_putstr(int portNum, char * data) {
	while (*data) {
		sock_putchar(portNum, *data);
		data++;
	}
}


int sock_getchar(int portNum, char * data) {
	int rc;

	if (portNum < 0 || portNum > SOCK_MAX-1) return 0;
	ENTER_CRIT
	rc = ring_buffer_dequeue(&socks[portNum].recvData, data);
	LEAVE_CRIT
#ifdef SOCK_DEBUG
	//printf("sock_getchar %d retuning %d\n",portNum,rc);
#endif

	if (rc)
		if (socks[portNum].dumpIO_console) dumpChar('R',*data);
	return rc;
}

int sock_dataAvailable(int portNum) {
	int res;
	ENTER_CRIT
	res = ring_buffer_peek(&socks[portNum].recvData, NULL, 0);
	LEAVE_CRIT
	return res;
}


// init: setup any port to listen for a new connection, startingPortNumber can be 0 for the default
void sock_init(int startingPortNumber) {
	int i,portNum;

	if (startingPortNumber < 1)
		portNum = SOCK_DEFAULT_BASE_PORT;
	else
		portNum = startingPortNumber;

	for (i=0;i<SOCK_MAX;i++) {
		socks[i].fd = -1;
		socks[i].portNum = portNum;
		sock_setupListen (i, false);
		//if (i == 0 || (i > 1 && i < 6)) {
		if (i >= 0 && i < 6) {
			socks[i].doTelnetInit = true;  // for scc and first fourway as default, second fourway raw
			socks[i].doOutTranslation = true;
			socks[i].doInTranslation = true;
		}
		bfseq_init(&socks[i].outSeqSta);
		portNum++;
		ring_buffer_init(&socks[i].recvData, socks[i].recvDataBuffer, sizeof(socks[i].recvDataBuffer));
		vtparse_init(&socks[i].parser,NULL);
		socks[i].recvFlag = FLAG_TOGGLE_BACKSPACE_ACTIVE;
	}
}


// deinit: close all open sockets
void sock_deinit() {
	int i;

	for (i=0;i<SOCK_MAX;i++) {
		if (socks[i].fd > -1) {
//            printf("%d: closing socket %d ",i,socks[i].fd); fflush(stdout);
			close(socks[i].fd);
//            printf("\n");
			socks[i].fd = -1;
			socks[i].status = STAT_CLOSED;
		}
	}
}

char * status2txt(enum sockstat_t status) {
	switch(status) {
		case STAT_CLOSED: return "STAT_CLOSED";
		case STAT_WAITCONN: return "STAT_WAITCONN";
		case STAT_OPEN    : return "STAT_OPEN";
		case STAT_DATA_AVAILABLE: return "STAT_DATA_AVAILABLE";
		default: return "unknown";
	}
}


void printStatus(int port) {
	if (port < 0 || port > SOCK_MAX-1) return;
	printf("%7d" FMT_SOCKET5 " %08x %-20s %10d  %9d%3d%7d\n",port,socks[port].fd,socks[port].revents,status2txt(socks[port].status),socks[port].doTelnetInit,socks[port].doOutTranslation,socks[port].doInTranslation,socks[port].portNum);
}

void sock_showStatus (int numArgs, struct args_t *args) {
	int i;
	printf("portNum   fd revents  Status              Telnet init  Trans out in   port\n" \
	       "==========================================================================\n");
	if (numArgs == 1)
		if (args[0].isValue) {
			printStatus(args[0].value);
			return;
		}
	for (i=0; i<SOCK_MAX; i++)
		printStatus(i);
}

void sock_telnetInit (int numArgs, struct args_t *args) {
	int i;

	if (numArgs == 1) {		// set it for all ports
		if (!args[0].isValue || args[0].value < 0 || args[0].value > 1) {
			printf("argument 0 or 1 required\n");
			return;
		}
		for (i=0; i<SOCK_MAX; i++) socks[i].doTelnetInit = args[0].value;
	}
	if (numArgs == 2) {
		if (!args[0].isValue || args[0].value < 0 || args[0].value > SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].doTelnetInit = args[1].value;
	}
}

void sock_outTrans (int numArgs, struct args_t *args) {
	int i;

	if (numArgs == 1) {		// set it for all ports
		if (!args[0].isValue || args[0].value < 0 || args[0].value > 1) {
			printf("argument 0 or 1 required\n");
			return;
		}
		for (i=0; i<SOCK_MAX; i++) socks[i].doOutTranslation = args[0].value;
	}
	if (numArgs == 2) {
		if (!args[0].isValue || args[0].value < 0 || args[0].value > SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].doOutTranslation = args[1].value;
	}
}

void sock_inTrans (int numArgs, struct args_t *args) {
	int i;

	if (numArgs == 1) {		// set it for all ports
		if (!args[0].isValue || args[0].value < 0 || args[0].value > 1) {
			printf("argument 0 or 1 required\n");
			return;
		}
		for (i=0; i<SOCK_MAX; i++) socks[i].doInTranslation = args[0].value;
	}
	if (numArgs == 2) {
		if (!args[0].isValue || args[0].value < 0 || args[0].value > SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].doInTranslation = args[1].value;
	}
}

void sock_terminal(int numArgs, struct args_t *args) {
	if (numArgs == 2) {
		if (!args[0].isValue || args[0].value < 0 || args[0].value > SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].doInTranslation = args[1].value;
		socks[args[0].value].doOutTranslation = args[1].value;
		socks[args[0].value].doTelnetInit = args[1].value;
	}
}


void sock_dump(int numArgs, struct args_t *args) {
	int i;

	if (numArgs == 1) {		// set it for all ports
		if (!args[0].isValue || args[0].value < 0 || args[0].value > 1) {
			printf("argument 0 or 1 required\n");
			return;
		}
		for (i=0; i<SOCK_MAX; i++) socks[i].dumpIO_console = args[0].value;
	}
	if (numArgs == 2) {
		if (!args[0].isValue || args[0].value < 0 || args[0].value > SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].dumpIO_console = args[1].value;
	}
}

void sock_recSend (int numArgs, struct args_t *args) {
	int newBufSize = 0;
	int portNum;
	if (numArgs < 1 || !args[0].isValue) {
		printf("usage: dev sock savesend PortNum [BufferSize]\n");
		return;
	}
	portNum = args[0].value;
	if (portNum >= SOCK_MAX) {
		printf("invalid port number\n");
		return;
	}

	if (socks[portNum].sendDataBuffer) {
		ENTER_CRIT
		free(socks[portNum].sendDataBuffer);
		socks[portNum].sendDataBuffer = NULL;
		LEAVE_CRIT
		printf("recording disabled\n");
		return;
	}

	if (numArgs == 2)
		newBufSize = args[1].value;
	else
		newBufSize = SEND_DATA_BUF_DEF_SIZE;
	ENTER_CRIT
	socks[portNum].sendDataBuffer = calloc(1,newBufSize);
	if (!socks[portNum].sendDataBuffer) {
		LEAVE_CRIT
		printf("failed to allocate %d bytes\n",newBufSize);
		return;
	}
	socks[portNum].sendDataBufferSize = newBufSize;

	socks[portNum].sendDataBufferPos = socks[portNum].sendDataBuffer;
	socks[portNum].sendDataBufferLen = 0;
	LEAVE_CRIT
	printf("recording enabled for port %d, size: %d\n",portNum,newBufSize);
}


void dumpSendBuf (unsigned int addr, unsigned int endAddr, int lineLen, char * buf) {
	unsigned int data;
	int asciiLen = 0;
	int hexLen;
	char hexData[100];
	char ascii[17];

	printf("\n");
	hexData[0]=0; sprintf(hexData,"%08x: ",addr);
	hexLen = strlen(hexData);
	while (addr <= endAddr) {
		data = *buf; buf++;
		if ((data < ' ') | (data > 0x7e)) ascii[asciiLen++] = '.'; else ascii[asciiLen++] = data;
		ascii[asciiLen] = 0;
		hexData[hexLen++] = hexNibble(data >> 4);
		hexData[hexLen++] = hexNibble(data);
		hexData[hexLen++] = ' ';
		hexData[hexLen] = 0;
		if (asciiLen == 8) { hexData[hexLen++] = ' '; hexData[hexLen] = 0; }
		addr++;
		if (asciiLen == lineLen) {
			printf("%-59s  %s\n",hexData,ascii);
			asciiLen = 0;
			hexData[0]=0; sprintf(hexData,"%08x: ",addr);
			hexLen = strlen(hexData);
		}
	}
	if (asciiLen > 0) {
		printf("%-59s  %s\n",hexData,ascii);
	}
}

void sock_showRec (int numArgs, struct args_t *args) {
	int portNum = args[0].value;

	if (portNum >= SOCK_MAX) {
		printf("invalid port number\n");
		return;
	}
	if (!socks[portNum].sendDataBuffer || socks[portNum].sendDataBufferLen) {
		printf("no data available on port %d\n",portNum);
		return;
	}

	dumpSendBuf (0,socks[portNum].sendDataBufferLen-1,16,socks[portNum].sendDataBuffer);
}

void sock_help (int numArgs, struct args_t *args);

struct cmds_t sockCmds[] =
{
	{ "status"     , sock_showStatus,  0,0,1,"show socket connection status"},
	{ "telnetinit" , sock_telnetInit,  0,2,1,"do telnet init 0|1 for all ports or portNum 0|1"},
	{ "outtrans"   , sock_outTrans  ,  0,2,1,"do translate outgoing escape sequences to VT100 0|1 for all ports or portNum 0|1"},
	{ "intrans"    , sock_inTrans   ,  0,2,1,"do translate incoming key sequences from VT100 0|1 for all ports or portNum 0|1"},
	{ "terminal"   , sock_terminal  ,  0,2,1,"set telnetinit,outtrans and inttrans for the given port on or off"},
	{ "recsend"    , sock_recSend   ,  0,1,1,"record data send to terminal in buffer for the given port , 2nd optional parameter is buffer size"},
	{ "showrec"    , sock_showRec   ,  1,1,1,"show recorded data"},
	{ "dump"       , sock_dump      ,  0,2,1,"dump data to console 0|1 for all ports or portNum 0|1"},
	{ "?"          , sock_help,        0,0,0,"show this help"},
	{ "help"       , sock_help,        0,0,0,"show this help"},
	{ "",  NULL, 0,0,0,""}
};

void sock_help (int numArgs, struct args_t *args) {
	showHelp ("sock help commands",sockCmds,0);
}


int sock_dbgCmd(int numArgs, struct args_t * args) {
        return findAndExecCommand (args[0].txt,sockCmds,numArgs-1,&args[1]);
}


#ifndef _WIN32
void sock_initialize() {
	initCriticalSections();
	sock_initialize_done = 1;
	//signal(SIGPIPE, SIG_IGN);
}


void sock_deinitialize() {
	sock_pollThreadEnd ();
	deInitCriticalSections();
}
#else

// winsock init
void sock_initialize() {
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    initCriticalSections();

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        printf("WSAStartup failed with error: %d\n", err);
        return;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return;
    }

    sock_initialize_done = 1;
}

void sock_deinitialize() {
	if (!sock_initialize_done) return;
	sock_pollThreadEnd ();
	deInitCriticalSections();
	WSACleanup();
	sock_initialize_done = 0;
}

#endif // _WIN32


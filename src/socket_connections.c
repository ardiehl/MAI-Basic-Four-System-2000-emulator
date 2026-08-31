/***************************************************************************
 *  esc_sequencessocket_connections.c
 *
 *  Created: Aug, 30 2026 AD
 *  Changed:
 *  Armin Diehl <ad@ardiehl.de>
 ****************************************************************************
 * TCP telnet connections for the eagle emulator
	TODO: connect to a used port will be accepted ?
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include "socket_connections.h"
#include "sim.h"
#include "telnetDefs.h"
#include <assert.h>
#include "util.h"
#include "esc_sequences.h"

// for holding escape sequences
#define OUT_BUF_SIZE 64

enum sockstat_t {
	STAT_CLOSED = 0,
	STAT_WAITCONN,
	STAT_OPEN,
	STAT_DATA_AVAILABLE
};

#define SEND_DATA_BUF_DEF_SIZE 32768

struct sock_t {
	int fd;
	enum sockstat_t status;
	int portNum;
	int revents;
	bool doTelnetInit;			// send telnet init sequence on connect
	bool doOutTranslation;		// translate evdt escape sequences to VT100
	bool doInTranslation;		// translate pressed keys to evdt codes
	bfseq_State_t outSeqSta;	// state for outgoing escape sequences
	int dumpIO_console;			// dump send and received chars to console
	int saveSendData;			// save send data to buffer
	int sendDataBufferSize;
	int sendDataBufferLen;
	char *sendDataBufferPos;
	char *sendDataBuffer;
};

struct sock_t socks[SOCK_MAX];




// returns listenfd or -1 on error
int setupListenSocket(int portNum) {
	int listenfd;
    int res=0;
    int on;
    struct sockaddr_in6 serv_addr;
    char sendBuff [255];

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));
	listenfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (listenfd < 0) {
  	 	 fprintf(stderr,"socket failed with code %d, errno: %d %s\n",res, errno,strerror(errno));
  	 	 return -1;
	}

	/****************************************************************************/
	/* The setsockopt() function is used to allow the local address to          */
	/* be reused when the server is restart_commanded before the required wait  */
	/* time expires.                                                            */
	/****************************************************************************/
    on=1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,(char *)&on,sizeof(on)) < 0) {
         fprintf(stderr,"setsockopt(SO_REUSEADDR) failed, errno: %d %s\n",errno,strerror(errno));
         return -1;
    }

    /*********************************************************************/
    /* on=1 would allow ipv6 only. Otherwise both, ipv4 and ipv6 clients */
    /* can connect                                                       */
	/*********************************************************************/
    on=0;
    res = setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on,sizeof(on));
    if (res < 0) {
  	 	 fprintf(stderr,"setsockopt(IPV6_V6ONLY = %d) failed with code %d, errno: %d %s\n",on,res,errno,strerror(errno));
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
  	 	 fprintf(stderr,"bind to port %d failed with code %d, errno: %d %s\n",portNum, res, errno, strerror(errno));
  	 	 close(listenfd);
  	 	 return -1;
	}

    res = listen(listenfd, 1);
    if (res < 0) {
  	 	 fprintf(stderr,"listen failed with code %d, errno: %d %s\n",res, errno,strerror(errno));
  	 	 close(listenfd);
  	 	 return -1;
	}

    return listenfd;
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
*/
void telnetClientInit(int fd) {
        uint8_t myOptions [] = {
                        TELNET_IAC, TELNET_DO, TELNET_TELOPT_ECHO,		// client should no echo
                        TELNET_IAC, TELNET_DONT, TELNET_TELOPT_NAWS,	// window size, we should disable this
                        TELNET_IAC, TELNET_WILL,TELNET_TELOPT_ECHO,
                        TELNET_IAC, TELNET_WILL,TELNET_TELOPT_SGA,
                        TELNET_IAC, TELNET_DO,  TELNET_TELOPT_SGA,		// for eagleemu, suppress CRLF on enter
                        TELNET_IAC, TELNET_DO,   TELNET_TELOPT_BINARY, 	// for eagleemu, suppress CRLF on enter
						TELNET_IAC, TELNET_WILL, TELNET_TELOPT_BINARY   // for eagleemu, suppress CRLF on enter
        };
        uint8_t reply[255];

        write(fd, (void *)&myOptions, sizeof(myOptions));

        // we should get at least one packet from the client with answers
        int res = recv(fd, (void *)&reply, sizeof(reply), 0);
#ifdef SOCK_DEBUG
		printf("telnetClientInit response for fd %d, size %d (IGNORED, ASSUMED TO BE OK)\n",fd,res);
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
	assert(portNum >= 0 && portNum < SOCK_MAX);
	if (doClose) { close(socks[portNum].fd); socks[portNum].fd = -1; }

	int newFd = setupListenSocket(socks[portNum].portNum);
	if (newFd < 0) {
		fprintf(stderr,"setupListenSocket failed with errno %d %s\n",errno,strerror(errno));
		socks[portNum].fd = -1;
		socks[portNum].status = STAT_CLOSED;
	} else {
		socks[portNum].fd = newFd;
		socks[portNum].status = STAT_WAITCONN;
	}
}

// check for incomping connections or data on all open ports and set the status field for each connection
void sock_poll() {
	struct pollfd  *pfds;
	int i,fd,numFds=0,res;
	int portIdx[SOCK_MAX];

	for (i=0;i<SOCK_MAX;i++) portIdx[i] = -1;

	// we can only poll an open socket
	for (i=0;i<SOCK_MAX;i++) {
		if (socks[i].status != STAT_CLOSED) numFds++;
	}
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
	res = poll(pfds,numFds,0);
	if (res < 0) {
		fprintf(stderr,"poll returned %d, errno: %d %s\n",res,errno,strerror(errno));
		return;
	}

	for(i=0;i<numFds;i++) {
		//printf("%d revents: 0x%8x ",i,pfds[i].revents);
		socks[portIdx[i]].revents = pfds[i].revents;
		if (pfds[i].revents && POLLIN) {						// on Linux i only get POLLIN even if the telnet client is already terminated
			if (socks[portIdx[i]].status == STAT_WAITCONN) {	// socket is listening, we need to accept the connection
				int newFd = accept(pfds[i].fd, (struct sockaddr*)NULL, NULL);
				if (newFd < 0) {
					fprintf(stderr,"accept failed with errno %d %s\n",errno,strerror(errno));
					// close the listen socket and try to create it again
					sock_setupListen (portIdx[i],true);
				} else {	// accept was ok
					// close the listen socket and set the port socket to the connected one
					close(socks[portIdx[i]].fd);
					socks[portIdx[i]].fd = newFd;
					socks[portIdx[i]].status = STAT_OPEN;
#ifdef SOCK_DEBUG
					printf("%d is connected\n",portIdx[i]);
#endif
					if (socks[portIdx[i]].doTelnetInit) telnetClientInit(newFd);
#ifdef SOCK_DEBUG
					char buff[20];
					strcpy(buff,"Greetings\r\n");
					write(newFd,buff,strlen(buff));
#endif
				}

			} else
			if (socks[portIdx[i]].status == STAT_OPEN || socks[portIdx[i]].status == STAT_DATA_AVAILABLE) {
				char c;
				ssize_t rc = recv(pfds[i].fd, &c, 1, MSG_PEEK);		// we should have data but lets check for errors
				if (rc == 1) {
					socks[portIdx[i]].status = STAT_DATA_AVAILABLE;
				} else {
					fprintf(stderr,"sock_poll: recv after status POLLIN: %ld, errno: %d %s\n",rc,errno,strerror(errno));
					// close the listen socket and try to create it again
					sock_setupListen (portIdx[i],true);
				}
			}
		}
	}
	//printf("\n");
}

void dumpChar(char prefix, char data) {
	char tmpBuf[20];

	snprintf(tmpBuf,sizeof(tmpBuf)-1,"%c: 0x%02x %c ",prefix,data,data > ' ' && data < 127 ? data : ' ');
	printf(tmpBuf);
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

	if (socks[portNum].sendDataBuffer) {	// save data send to terminal if enabled
		if (socks[portNum].sendDataBufferLen < socks[portNum].sendDataBufferSize) {
			*socks[portNum].sendDataBufferPos = data;
			socks[portNum].sendDataBufferPos++;
			socks[portNum].sendDataBufferLen++;
		}
	}

	if (socks[portNum].doOutTranslation) {		// if we have outgoing escape sequence translation
		char outSeqBuffer[OUT_BUF_SIZE];
												// bfseq_processChar will process the character and fill outSeqBuffer when we have a complete sequence
		if (socks[portNum].dumpIO_console) dumpChar('s',data);
		int bufLen = bfseq_processChar (&socks[portNum].outSeqSta,data, outSeqBuffer, sizeof(outSeqBuffer));
		if (bufLen) {
			send(socks[portNum].fd,&outSeqBuffer,bufLen,MSG_DONTWAIT);
			//if (socks[portNum].dumpIO_console) dumpStr('s',outSeqBuffer);
		}

	} else {
		send(socks[portNum].fd,&data,1,MSG_DONTWAIT);
	}
}

void sock_putstr(int portNum, char * data) {
	while (*data) {
		sock_putchar(portNum, *data);
		data++;
	}
}


int sock_getchar(int portNum, char * data) {
	if (portNum < 0 || portNum > SOCK_MAX-1) return 0;
	if (socks[portNum].status != STAT_DATA_AVAILABLE) return 0;
	int res = recv(socks[portNum].fd,data,1,0);
	if (res != 1) {
		fprintf(stderr,"sock_getchar(%d), recv returned %d, errno: %d %s\n",portNum,res,errno,strerror(errno));
		socks[portNum].status = STAT_OPEN;
		return 0;
	}
	socks[portNum].status = STAT_OPEN;
#ifdef SOCK_DEBUG
	printf("%d retuning 1\n",portNum);
#endif

	if (socks[portNum].dumpIO_console) dumpChar('R',*data);
	return 1;
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
	}
}

// deinit: close all open sockets
void sock_deinit() {
	int i;

	for (i=0;i<SOCK_MAX;i++) {
		if (socks[i].fd > -1) {
			close(socks[i].fd);
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
	printf("%7d%5d %08x %-20s %10d  %9d%3d%7d\n",port,socks[port].fd,socks[port].revents,status2txt(socks[port].status),socks[port].doTelnetInit,socks[port].doOutTranslation,socks[port].doInTranslation,socks[port].portNum);
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
		if (!args[0].isValue || args[0].value < 0 || args[0].value >= SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
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
		if (!args[0].isValue || args[0].value < 0 || args[0].value >= SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
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
		if (!args[0].isValue || args[0].value < 0 || args[0].value >= SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
			printf("invalid arguments\n");
			return;
		}
		socks[args[0].value].doInTranslation = args[1].value;
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
		if (!args[0].isValue || args[0].value < 0 || args[0].value >= SOCK_MAX-1 || !args[1].isValue || args[1].value < 0 || args[1].value > 1) {
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
		free(socks[portNum].sendDataBuffer);
		socks[portNum].sendDataBuffer = NULL;
		printf("recording disabled\n");
		return;
	}

	if (numArgs == 2)
		newBufSize = args[1].value;
	else
		newBufSize = SEND_DATA_BUF_DEF_SIZE;

	socks[portNum].sendDataBuffer = calloc(1,newBufSize);
	if (!socks[portNum].sendDataBuffer) {
		printf("failed to allocate %d bytes\n",newBufSize);
		return;
	}
	socks[portNum].sendDataBufferSize = newBufSize;

	socks[portNum].sendDataBufferPos = socks[portNum].sendDataBuffer;
	socks[portNum].sendDataBufferLen = 0;
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
	if (!socks[portNum].sendDataBuffer) {
		printf("no data available on port %d\n",portNum);
		return;
	}

	dumpSendBuf (0,socks[portNum].sendDataBufferLen-1,16,socks[portNum].sendDataBuffer);
}

void sock_help (int numArgs, struct args_t *args);

struct cmds_t sockCmds[] =
{
	{ "status"     , sock_showStatus,  0,0,0,"show socket connection status"},
	{ "telnetinit" , sock_telnetInit,  0,2,0,"do telnet init 0|1 for all ports or portNum 0|1"},
	{ "outtrans"   , sock_outTrans  ,  0,2,0,"do translate outgoing escape sequences to VT100 0|1 for all ports or portNum 0|1"},
	{ "intrans"    , sock_inTrans   ,  0,2,0,"do translate incoming key sequences from VT100 0|1 for all ports or portNum 0|1"},
	{ "recsend"    , sock_recSend   ,  0,1,0,"record data send to terminal in buffer for the given port , 2nd optional parameter is buffer size"},
	{ "showrec"    , sock_showRec   ,  1,1,0,"show recorded data"},
	{ "dump"       , sock_dump      ,  0,2,0,"dump data to console 0|1 for all ports or portNum 0|1"},
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



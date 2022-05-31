#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

#define PORT "9034"   // luister poort

// sockaddr , IPv4 en IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    fd_set master;    // master beschrijving bestand lijst
    fd_set read_fds;  // temp beschrijving bestand lijst voor select()
    int fdmax;        // maximum beschrijving bestand nr

    int listener;     // luister socket beschrijving
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer voor client info
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    int yes=1;        // voor setsockopt() SO_REUSEADDR, beneden
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // leegmaken van masten en temp
    FD_ZERO(&read_fds);

    // socket en binden
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(::1, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // fout code als deze al gebruikt is
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // geen bind gelukt
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai);

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // extra listener toevoegen in master
    FD_SET(listener, &master);

    // grootste file monitoren
    fdmax = listener;

    // main loop
    for(;;) {
        read_fds = master; // kopieerd gegevens
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // zoek naar data om te lezen
        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // data gevonden
                if (i == listener) {
                    // handelen nieuwe connectie
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // toevoegen in master
                        if (newfd > fdmax) {    // kijken wat max is
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // handelen data voor een client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connectie voorbij
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // uit master gehaald
                    } else {
                        // data van client gekregen
                        for(j = 0; j <= fdmax; j++) {
                            // naar iedereen sturen
                            if (FD_ISSET(j, &master)) {
                                // buiten de listener en de client zelf
                                if (j != listener && j != i) {
                                    if (send(j, buf, nbytes, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                } // END data handeling client
            } // END 	nieuwe inkomende connectie
        } // END looping door files
    } // END for(;;)--einde van de oneindige loop

    return 0;
}

// extra info: bij server te laten runnen zorg dat de poorten in packet sendor( de client) disabled zijn (to be sure)
// anders kans op bind errors als deze al gebruikt zijn bij pakcet sender als server te gebruiken ipv client

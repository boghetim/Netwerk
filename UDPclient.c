#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7 //select minimal legacy support, needed for inet_pton, inet_ntop
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
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
#endif

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	//Step 1.0
	WSADATA wsaData;
	WSAStartup( MAKEWORD(2,0), &wsaData );

	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address = NULL;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	getaddrinfo( "::1", "24020", &internet_address_setup, &internet_address );

	//Step 1.2
	int internet_socket;
	internet_socket = socket( internet_address->ai_family, internet_address->ai_socktype, internet_address->ai_protocol );


	/////////////
	//Execution//
	/////////////

	//Step 2.1
  int i=0;
  while (i !=5)
  {
  char text[15];
	if(i==0){
  	sprintf(text,"pakket PostNL");
		sendto( internet_socket, text, 13, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
	else if(i==1){
		sprintf(text,"pakket DHL");
		sendto( internet_socket, text, 10, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
	else if(i==2){
		sprintf(text,"pakket Bpost");
		sendto( internet_socket, text, 12, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
	else if(i==3){
		sprintf(text,"pakket FedEx");
		sendto( internet_socket, text, 12, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
	else if(i==4){
		sprintf(text,"pakket DPD");
		sendto( internet_socket, text, 10, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
	else{
		sprintf(text,"pakket GLS");
		sendto( internet_socket, text, 12, 0, internet_address->ai_addr, internet_address->ai_addrlen );
		}
		i++;
  }

	////////////
	//Clean up//
	////////////

	//Step 3.2
	freeaddrinfo( internet_address );

	//Step 3.1
	close( internet_socket );

	//Step 3.0
	WSACleanup();

	return 0;
}

#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>



void print_ip_address( struct addrinfo * ip )
{
	void * ip_address;
	char * ip_version;
	char ip_string[INET6_ADDRSTRLEN];

	if( ip->ai_family == AF_INET )
	{ // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)ip->ai_addr;
		ip_address = &(ipv4->sin_addr);
		ip_version = "IPv4";
	}
	else
	{ // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ip->ai_addr;
		ip_address = &(ipv6->sin6_addr);
		ip_version = "IPv6";
	}

	inet_ntop( ip->ai_family, ip_address, ip_string, sizeof ip_string );
	printf( "%s -> %s\n", ip_version, ip_string );
}

int main( int argc, char * argv[] )
{
	WSADATA wsaData; //WSAData wsaData; //Could be different case
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 ) // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}

	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	internet_address_setup.ai_socktype = SOCK_STREAM;

	char ip_server[45] = "\0";
	char port_server[5] = "\0";
	printf("geef server IP in \n");
	scanf("%s\n",ip_server);
	fflush(stdin);
	printf("geef server Port in \n");
	scanf("%s\n",port_server);
	fflush(stdin);

	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( "ip_server", "port_server", &internet_address_setup, &result_head );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 2 );
	}
/* worden niet gebruikt
	struct sockaddr * internet_address;
	size_t internet_address_length;
*/

	//maken van socket
	int internet_socket;
	result_item = result_head; //take first of the linked list
	while( result_item != NULL ) //while the pointer is valid
	{
		internet_socket = socket( result_item->ai_family, result_item->ai_socktype, result_item->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			printf( "Connecting to " );
			print_ip_address( result_item );

			int connect_return;
			connect_return = connect( internet_socket, result_item->ai_addr, result_item->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				printf( "Connected\n" );
				break; //stop running through the linked list
			}
		}
		result_item = result_item->ai_next; //take next in the linked list
	}
	if( result_item == NULL )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 4 );
	}
	freeaddrinfo( result_head ); //free the linked list


	//data versturen
	int number_of_bytes_send = 0;
	char buffer_send[1000] = "\0";

	while(1)
	{
		printf("welke tekst wil je sturen \n");
		scanf("%s\n", buffer_send);
		fflush(stdin);
		if (strcmp(buffer_send, "/goodbye") ==0)
		{
		break;
		}
		number_of_bytes_send = send( internet_socket, buffer_send, strlen(buffer_send), 0 );
		if( number_of_bytes_send == -1 )
		{
			printf( "errno = %d\n", WSAGetLastError() ); //https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
			perror( "send" );
		}
	}





	//sluiten
	int shutdown_return;
	shutdown_return = shutdown( internet_socket, SD_SEND );
	if( shutdown_return == -1 )
	{
		printf( "errno = %d\n", WSAGetLastError() );
		perror( "shutdown" );
	}

	close( internet_socket );

	WSACleanup();

	return 0;
}

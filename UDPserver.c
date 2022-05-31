#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h>


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
	#include <time.h>

	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	execution( internet_socket );

	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;


	int getaddrinfo_return = getaddrinfo("192.168.0.237", "24020", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		//Step 1.2
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			//Step 1.3
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution( int internet_socket )
{
	//Step 2.1
	int number_of_bytes_received = 0;
	char buffer[1000];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	int a=0; //hoeveel pakketen er wordtn afgeleverd

	int pkt=0;
	int timeout = 10000;

	FILE * inputGegevens = fopen("UDP_ServerDB.csv", "w");
	if (inputGegevens==NULL)
	{
		printf("output file error \n");
		exit(1);
	}

	if (setsockopt(internet_socket, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)))
		{
			perror("Error");
		}
		clock_t begin_time;
		clock_t begin_packet;
		float tempMax=0.0;
		float tempMin=500.0;
		for (a=0; a<10; a++) //hoeveel pakketten je wilt ontvangen
			{
				if (a==0)
				{
					begin_time = clock(); //begin tijd vanaf 0 voor totale tijd
				}

				begin_packet = clock();	//begin voor tijd per pakket
				number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
				if( number_of_bytes_received == -1 )
				{
					perror( "recvfrom" );
				}
				else
				{
					buffer[number_of_bytes_received] = '\0';
					printf( "\n Pakket : %s\n", buffer );	//print pakket
					fwrite(&buffer, strlen(buffer), 1, inputGegevens);	//wegschrijven van info pakket
					fwrite("\n", sizeof(char), 1, inputGegevens);	//tabje
					pkt++;	//counter hoeveel pakketen er zijn gelukt
				}
				clock_t end_packet = clock();	//einde klok pakket
				float elapsed_packet = (double)(end_packet - begin_packet) / CLOCKS_PER_SEC;	//berekening pakket tijd
				printf("tijd voor pakket %d: %.4f sec\n", a+1, elapsed_packet);	//tijd ervan in seconden
				if (elapsed_packet > tempMax)
				{
					tempMax=elapsed_packet;
				}
				if (elapsed_packet < tempMin)
				{
					tempMin=elapsed_packet;
				}
		}
		clock_t end_time = clock();	//einde klok totaal
		float elapsed_time = (double)(end_time - begin_time) / CLOCKS_PER_SEC;	//berekening totaal tijd
		printf("\n totaal tijd alle pakketen: %.4f sec\n", elapsed_time);	//tijd ervan in seconden
		printf("er zijn %d pakketen aangekomen van de %d\n",pkt, a );	//hoeveel pakketen verloren
		float avg= elapsed_time/a;
		printf("per pakket is de max tijd: %.4f , de Min tijd: %.4f en de gemiddelde tijd %.4f", tempMax, tempMin, avg );

		//gegevens inladen
		char tmp[50] = "\0";

		sprintf(tmp, "totaal aangekomen pakketen: %d \n",pkt);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		sprintf(tmp, "totaal gevraagde pakketen : %d \n",a);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		sprintf(tmp, "totaal aantal tijd : %.4f \n",elapsed_time);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		sprintf(tmp, "max tijd tussen een pakket : %.4f \n",tempMax);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		sprintf(tmp, "min tijd tussen een pakket : %.4f \n",tempMin);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		sprintf(tmp, "gemiddelde tijd tussen een pakket : %.4f \n",avg);
		fwrite(&tmp, strlen(tmp), 1,inputGegevens);

		fclose(inputGegevens);

	//Step 2.2
	int number_of_bytes_send = 0;
	number_of_bytes_send = sendto( internet_socket, "Hello UDP world!", 16, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
	if( number_of_bytes_send == -1 )
	{
		perror( "sendto" );
	}
}

void cleanup( int internet_socket )
{
	//Step 3.1
	close( internet_socket );
	WSACleanup();
}


// extra info: bij server te laten runnen zorg dat de poorten in packet sendor( de client) disabled zijn (to be sure)
// anders kans op bind errors als deze al gebruikt zijn bij pakcet sender als server te gebruiken ipv client

//gcc -Wall -pedantic UDPserver.c -l ws2_32 -o UDPserver.exe

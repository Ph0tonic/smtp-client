/**
  * Description : Client SMTP implémenté sur la base d'un client TCP simple
  * Using : ./client_smtp expéditeur sujet FichierCorps serveurSmtp destinataire
  * Date : 3 juin 2017
  * Context : Cours Réseaux INF1j
  * Authors : Malik Fleury et Bastien Wermeille
  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SENDER       argv[1]
#define SUBJECT       argv[2]
#define BODY         argv[3]
#define SERVER       argv[4]
#define RECIPIENT    argv[5]
#define PORT         argv[6]

#define DEFAULTPORT "25"//587
#define MAXATTEMPT 3

typedef enum
{
	CONNECTION, HELO, FROM, TO, DATA, DATA2, QUIT, ERROR
} EtatsSmtp;

/* Function definition */
static FILE *tcp_connect(const char *hostname, const char *port);
void smtp_send(char* sender,char* recipient,char* subject,char* body,char* server,char* nPort);
void tcp_close(FILE *f);
int tcp_send(FILE *f, char *cmd);
int errorManager(char first, char second, char third);

/* Public function implementation */
//expéditeur, sujet, corps, serveur smtp, destinataire
int main(int argc, char *argv[])
{
	if(argc != 6 && argc !=7)
  {
    fprintf(stderr, "%s: Bad arguments.\n", argv[0]);
    return 1;
  }
  else
  {
    //Choice of the port, by default 25
    char* nPort = DEFAULTPORT;
    if(argc == 7)
      nPort = PORT;

    smtp_send(SENDER,RECIPIENT,SUBJECT,BODY,SERVER,nPort);
    return 0;
  }
}

int smtp_send(sender,recipient,subject,body,server,port)
{
  //Initial state
  EtatsSmtp smtpState=CONNECTION;

  //Basic stuff
  unsigned int attempt=0;
	int codeErr;

  //Check body message
	FILE *f = NULL;
	char buffer[1024];
	FILE *message = fopen(body,"r");
	if(message == NULL) return -1;

  //log
	fprintf(stdout, "Send from %s to %s with message %s : %s on %s :: %s\n",sender,recipient,subject,body,server,port);
	while(attempt < MAXATTEMPT)
	{
    //State machine
		switch(smtpState)
		{
			case CONNEXION:
				if((f = tcp_connect(server, port)))
				{
					printf("Connection ok\n");
					codeErr = tcp_send(f,"");
					(codeErr == 0) ? (smtpState = HELO) : (smtpState = ERROR);
				}
				else
				{
					perror("Error while conecting to the server\n");
					smtpState = ERROR;
				}
				break;
			case HELO:
				codeErr = tcp_send(f,"HELO client\r\n");
				(codeErr == 0) ? (smtpState = FROM) : (smtpState = ERROR);
				break;
			case FROM:
				sprintf(buffer,"MAIL FROM: <%s>\r\n",sender);
				codeErr = tcp_send(f,buffer);
				(codeErr == 0) ? (smtpState = TO) : (smtpState = ERROR);
				break;
			case TO:
				sprintf(buffer,"RCPT TO: <%s>\r\n",recipient);
				codeErr = tcp_send(f,buffer);
				(codeErr == 0) ? (smtpState = DATA) : (smtpState = ERROR);
				break;
			case DATA:
				codeErr = tcp_send(f,"DATA\r\n");
				(codeErr == 0) ? (smtpState = DATA2) : (smtpState = ERROR);//3
				break;
			case DATA2:
				sprintf(buffer,"Subject: %s\n",subject);
				fprintf(f,buffer);
				while(fgets(buffer,sizeof(buffer),message))
				{
					fprintf(f,buffer);
				}
				codeErr = tcp_send(f,"\r\n.\r\n");
				(codeErr == 0) ? (smtpState = QUIT) : (smtpState = ERROR);
				break;
			case QUIT:
				tcp_send(f,"QUIT\r\n");
				tcp_close(f);
				fclose(message);
				return 0;
				break;
			case ERROR:
				if(codeErr == -1)
				{
					tcp_close(f);
					fclose(message);
					perror("Error 5xx\n");
					return -1;
				}
				printf("New attempt in five minutes...\n");
				sleep(300);
				attempt++;
				smtpState = CONNECTION;
				break;
			default:
				perror("Unexpected error\n");
				tcp_close(f);
				fclose(message);
        return -1;
        break;
		}
		sleep(1);
	}

  //Close opened file and socket
	tcp_close(f);
	fclose(message);
}

//Send data to smtp server et get error code
int tcp_send(FILE *f, char *cmd)
{
	fprintf(f,cmd);
	fflush(f);
	char buffer[255];
	do
	{
		fgets(buffer, sizeof(buffer), f);
		puts(buffer);
	}while(buffer[0] < '2' || buffer[0] > '5');

	return errorManager(buffer[0],buffer[1],buffer[2]);
}

//Mangement of error
// -1 -> KO
//  0 -> OK
//  1 -> Try again
int errorManager(char first, char second, char third)
{
  switch (first){
		case '1':
		printf("The server does not respond\n");
			switch (second){
				case '0':
				printf("The server is unable to connect.\n");
				case '1':
				printf("Connection refused or inability to open an SMTP stream.\n");
				default:
				printf("Unexceptected error\n");
			}
		return -1;

	    case '2':
		printf("The server has completed the task successfully.\n");
		return 0;

	    case '3':
		printf("The server has understood the request, but requires further information to complete it.\n\n");
		return -1;

	    case '4':
		printf("The server has encountered a temporary failure.\n");
		return 1;

	    case '5':
	    printf("The server has encountered an error.\n");
		return 0;

		default:
	    printf("Thiserror seems impossible \n");
		return 0;
    	}
}

//Close tcp connection
void tcp_close(FILE *f)
{
	shutdown(fileno(f), SHUT_WR);
	if(fclose(f) == 0) f = NULL;
	else
	{
		perror("fclose(): failed: ");
	}
}

//Open tcp connection
static FILE *tcp_connect(const char *hostname, const char *port)
{
	FILE *f = NULL;

	int s;
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	hints.ai_family = AF_UNSPEC; /* IPv4 or v6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP */
	hints.ai_flags = 0;
	hints.ai_protocol = 0; /* any protocol */

	if((s = getaddrinfo(hostname, port, &hints, &result)))
	{
		fprintf(stderr, "getaddrinfo(): failed: %s.\n", gai_strerror(s));
	}
	else
	{
		/*  getaddrinfo() returns a list of address structures.
		*  Try each address until we successfully connect(2).
		*/
		for(rp = result; rp != NULL; rp = rp->ai_next)
		{
			char ipname[INET_ADDRSTRLEN];
			char servicename[6]; /* "65535\0" */
			/* socket creation */
			if((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
			{
				perror("socket() failed: ");
				continue; /* next loop */
			}

			if(!getnameinfo(rp->ai_addr, rp->ai_addrlen, ipname, sizeof(ipname), servicename, sizeof(servicename), NI_NUMERICHOST|NI_NUMERICSERV))
			{
				printf("Trying connection to host %s:%s ...\n", ipname, servicename);
			}

			if(connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
			{
				/* from now on, read(2), write(2), shutdown(2) and close(2)
				* can be called on that socket (file descriptor) s -- note that
				* as for every character device, write(2) might not write
				* all we want, and read(2) will return even if not all bytes
				* are received: this is one of the reason we will promote
				* the socket into a fully-equipped libc FILE *.
				*/
				break; /* end of loop */
			}
			else
			{
				perror("connect(): ");
			}

			close(s); /* err. ign. */
		}

		freeaddrinfo(result);

		if(rp == NULL)
		{
			fprintf(stderr, "Could not connect.\n");
		}
		else
		{
			/* associate the file descriptor to a C library file, to make
			* things easier for us
			*/
			if(!(f = fdopen(s, "r+")))
			{
				perror("fdopen() failed: ");
			}
		}
	}
	return f;
}

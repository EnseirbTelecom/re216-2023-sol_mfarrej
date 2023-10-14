#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>



int main(int argc, char *argv[]) {
	
	if (argc != 3) {
        fprintf(stderr, "Merci de rentrer deux arguments : <server_name> <server_port>\n");
        exit(EXIT_FAILURE);
    }
	
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	int ret = getaddrinfo(argv[1], argv[2], &hints, &result); // renvoie 0 si succès
	if (ret != 0) {
		perror("getaddrinfo");
      exit(EXIT_FAILURE);
	}
	
	int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (fd == -1) {
		perror("socket"); //affiche la valeur de errno 
		exit(EXIT_FAILURE);
	}
	
	ret = connect(fd, result->ai_addr, result->ai_addrlen);
	if (ret == -1) {
		perror("connect"); //affiche la valeur de errno 
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in addr_local_socket;
	socklen_t len = sizeof(addr_local_socket);
	getsockname(fd, (struct sockaddr *)&addr_local_socket, &len);
	ushort local_port = ntohs(addr_local_socket.sin_port);
	printf("Local socket port %u\n", local_port);
	
	struct pollfd fds[2]; // Un descr. pour l'entrée, l'autre (fd) pour la socket 
	memset(fds, 0, 2*sizeof(struct pollfd));
	fds[0].fd = 0; // stdin
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = fd; // socket
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	
	
	//char *str="Bonjour les T2! Comment ";
	printf("Message à envoyer : \n");
	fflush(stdout);
	
	int next_msg_size = 0;
	

	while (1) {
		
		int ret = poll(fds, 2, -1); // Call poll that awaits new events

		if (ret == -1) {
			perror("poll");
		   exit(EXIT_FAILURE);
		}

		if (fds[0].revents & POLLIN) { // Activité sur stdin
		 	// Lire stdin et envoyer au serveur
		   char buf[256];
		   if (fgets(buf, sizeof(buf), stdin) == NULL) {
 			   perror("fgets");
 			   exit(EXIT_FAILURE);
			}
		   
		   next_msg_size = strlen(buf);
		   
		   int sent = 0;
		   
		   while (sent!= sizeof(int)) { //envoi de la taille du message dans un premier temps
				int ret = write(fd, (char *)&next_msg_size+sent, sizeof(int)-sent); 
				if (ret == -1) {
					perror("write"); 
					exit(EXIT_FAILURE);
				}
	
				sent += ret;
			}
	
			sent = 0;
	
			while (sent!= next_msg_size) { //envoi de la taille du message dans un premier temps
				int ret = write(fd, buf+sent, next_msg_size-sent);
				if (ret == -1) {
					perror("write");  
					exit(EXIT_FAILURE);
				}
			
				sent += ret;
			}
			
			printf("Message envoyé. Vous pouvez envoyer un nouveau message :\n");
    		fflush(stdout);
			
		   if (strcmp(buf, "/quit\n") == 0) {
         	printf("Connexion fermée\n");
            break;
         }
		}

   	if (fds[1].revents & POLLIN) { // Activité sur la socket (message du serveur)
 	 	// Lire le message du serveur 
			int received = 0;
			char buf_in[next_msg_size + 1];
			buf_in[next_msg_size] = '\0';

			while (next_msg_size > received) {
				int ret = read(fd, (char *)buf_in + received, next_msg_size - received);

				if (ret == -1) {
					perror("read");
					exit(EXIT_FAILURE);
				}

				received += ret;
			}

			printf("\nReceived Msg from Server: %s\n", buf_in);
			fds[1].revents = 0;

      }
	}
	
	freeaddrinfo(result);
	close(fd);
	return 0;
	
}

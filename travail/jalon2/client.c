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

#include "common.h"
#include "msg_struct.h"


int valid_format(enum msg_type type, char *user_input) {
	switch (type) {

	   case NICKNAME_NEW:
	    	if (*(user_input + strlen("/nick")) != ' ') {
	    		printf("[Server] : Veuillez insérez un espace entre la commande et le pseudonyme\n");
	    		return -1;
	    	}
	    
	      if (strlen(user_input) <= strlen("/nick ")) {
	      // L'utilisateur n'a pas renseigné de nickname après "/nick "
	         printf("[Server] : Pseudonyme non renseigné, utilisez le format \"/nick <pseudonyme>\".\n");
            return -1;
         }

         if (strlen(user_input) - strlen("/nick ") > NICK_LEN+1) { // Prise en compte de \n
				printf("[Server] : Le pseudonyme est trop long. La longueur maximale est de %d caractères.\n", NICK_LEN);
            return -1;
         }
         
         break;

      case NICKNAME_INFOS:
			if (*(user_input + strlen("/whois")) != ' ' && *(user_input + strlen("/whois")) != '\n' ) {
	    		printf("[Server] : Veuillez insérez un espace entre la commande et le pseudonyme\n");
	    		return -1;
	    	}
			
			if (strlen(user_input) <= strlen("/whois ")) {
	         printf("[Server] : Pseudonyme non renseigné, utilisez le format \"/whois <pseudonyme>\".\n");
            return -1;
         }
			
			if (strlen(user_input) - strlen("/whois ") > NICK_LEN+1) { // Prise en compte de \n
				printf("[Server] : Le pseudonyme est trop long. La longueur maximale est de %d caractères.\n", NICK_LEN);
            return -1;
         }
         break;

		case NICKNAME_LIST:
			
			if ( strcmp(user_input, "/who\n") != 0 || (strncmp(user_input, "/who ", strlen("/who ")) && *(user_input + strlen("/who")) != ' ') ) {
			// Formats de commande acceptés : "/who", "/who ","/who <anything>"
			// Format non accepté : "/who<anything>" (sans espace)
	         printf("[Server] : Format incorrect. Formats de commande acceptés : \"/who\", \"/who \",\"/who <anything>\"\n");
            return -1;
         }
         // Aucune vérification de longueur n'est nécessaire ici
         break;

      case UNICAST_SEND:
	      
	      if (strcmp(user_input, "/msg\n") != 0 ) {
	      	printf("[Server] : Format de la commande : /msg <nickname> <message>\n");
	      	return -1;
	      }
	      
	      if ( *(user_input + strlen("/msg")) != ' ' )  {
	      	printf("[Server] : Veuillez insérer un espace après la commande /msg.\n");      	
	      	return -1;
	      }
	      
	      if (strlen(user_input+ strlen("/msg ")) > INFOS_LEN +1) { // Prise en compte de \n
	         printf("[Server] : L'ensemble <pseudonyme> <message> est trop long, la longueur maximale est %d caractères.\n", INFOS_LEN);
            return -1;
         }

	      char nickname[NICK_LEN];
	      char *user_input_ptr = user_input + strlen("/msg ");
	      int i = 0;
	      int space_counter = 0;
	   

	      while (space_counter !=1 || *user_input_ptr != '\n')	{

	      	if (*user_input_ptr == ' ') {
	      		space_counter +=1;
	      	}

	      	nickname[i] = *user_input_ptr;
	      	i+=1;
	      	user_input_ptr +=1;
	      	
	      }
	      
	      nickname[i] = '\0';
	      
	      if (space_counter == 0) {
	      	printf("[Server] : Message non renseigné. Veuillez utilisez le format /msg <username> <message>\n");
	      	return -1;
	      }
			
	      if (strlen(nickname) > NICK_LEN) {
	      	printf("[Server] : Le pseudonyme est trop long. La longueur maximale est de %d caractères.\n", NICK_LEN);
	      	return -1;
	      }
	      
         break;

      case BROADCAST_SEND:
			
			 if (strcmp(user_input, "/msgall\n") != 0 || ( strlen(user_input) <= strlen("/msgall ") && *(user_input + strlen("/msgall")) == ' ' && *(user_input + strlen("/msgall ")) == '\n' ) ) {
	      	printf("[Server] : Message non renseigné, utilisez le format \"/msgall <message>\".\n");
	      	return -1;
	      }
	      
	      if ( *(user_input + strlen("/msgall")) != ' ' ) {
	      	printf("[Server] : Veuillez insérer un espace après la commande /msgall.\n");  	
	      	return -1;
	      }
	      						
			if (strlen(user_input) - strlen("/msgall ") > INFOS_LEN + 1) { // Prise en compte de \n
				printf("[Server] : Le message est trop long. La longueur maximale est de %d caractères.\n", INFOS_LEN);
            return -1;
         }			
			
			break;

//	Tout ce qui ne correspond pas à ce qui précède est considéré comme ECHO_SEND
    }
   
   return 0; // Format et longueur valides
}





int prepare_message(struct message *msg, char *user_input, const char *nick_sender, char *payload) {
	
	//*payload[0] = '\0';
 	msg->infos[0] = '\0'; // Rempli si besoin
	enum msg_type type = 0;
	int ret = 999;
	int input_size = 0;
	
	if (strncmp(user_input, "/msgall", strlen("/msgall")) == 0) {
	   type = BROADCAST_SEND;
		ret = valid_format(type, (char *)user_input);
		if (ret == -1) {
			return -1;
		}
		
		input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
		strcpy(payload, user_input + strlen("/msgall "));
	}	    

   else if (strncmp(user_input, "/who", strlen("/who")) == 0){
   	type = NICKNAME_LIST;
   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
	}	
   
   else if (strncmp(user_input, "/msg", strlen("/msg")) == 0) { 
   	type = UNICAST_SEND;
   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   	input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
   	strcpy(payload, user_input + strlen("/msg "));
   }
   		
   else if (strncmp(user_input, "/nick", strlen("/nick")) == 0) { 
		type = NICKNAME_NEW;
   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   	input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
   	strcpy(payload, user_input + strlen("/nick "));
   }
   
   else if (strncmp(user_input, "/whois", strlen("/whois")) == 0) { 
		type = NICKNAME_INFOS;
   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   	input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
   	strcpy(payload, user_input + strlen("/whois "));
   }
   		
	else {
		type = ECHO_SEND; // pas d'infos extraites
   	input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
   	strcpy(payload, user_input);
   } 
//	Remplissage de la structure msg
   msg->type = type;	// Champ msg->type
   strcpy(msg->nick_sender, nick_sender); // Champ msg->nick_sender

  	msg->pld_len = strlen(payload);

   if (type == NICKNAME_NEW || type == NICKNAME_INFOS || type == UNICAST_SEND) {
    	strncpy(msg->infos, payload, INFOS_LEN);
   }
   
   return 0; // Format valide, structure msg remplie et payload stocké
}





int send2steps(int fd, struct message msg, const char *payload) {
	
	int sent = 0;
	
	while (sent != sizeof(struct message)) { 
	// Envoi de la structure dans un premier temps
   	int ret = write(fd, (char *)&msg + sent, sizeof(struct message) - sent);
      if (ret == -1) {
         return -1; // Erreur d'écriture
      }
   	
   	sent += ret;
    }
	
	
	if (msg.type != NICKNAME_LIST)	{
	//	Envoi des données du champ infos 
		sent = 0;
		while (sent != msg.pld_len) {
			int ret = write(fd, payload + sent, msg.pld_len - sent);
			if (ret == -1) {
				return -1; 
			}
			
			sent += ret;
		}	
	}

	return 0; // succès
}


int receive2steps(int fd, struct message *msg, char **payload) {

	int received = 0;
	
	while (received != sizeof(struct message)) {
	// Réception de la structure dans un premier temps	
		int ret = read(fd, (char *)msg + received, sizeof(struct message) - received);
		if (ret == -1)
			return -1; // Erreur de lecture
		
		received += ret;
   }
	
	received = 0;
	*payload = (char *)malloc(msg->pld_len);
	if (*payload == NULL) {
		return -2; // Pour différencier du retour d'erreur du read
	}
		
	while (received != msg->pld_len) {
	// Réception des données du champ infos
		int ret = read(fd, *payload + received, msg->pld_len - received);
		if (ret == -1){
		 	free(payload);
			return -1; // Erreur de lecture
		}
		received += ret;
	}
	
	return 0; // succès
}





int initialNickname(int fd, char **my_nickname) {
	char user_input[NICK_LEN]; // buffer qui sert à remplir le champ msg.infos au 1er envoi 
	user_input[0] = '\0'; 	
	
	*my_nickname = (char *)malloc(NICK_LEN);
	memset(*my_nickname, 0, NICK_LEN);
	
	struct message	msg;
	memset(&msg, 0, sizeof(struct message));
	
	while (1) {
		
		printf("[Server] : please login with /nick <your pseudo>\n");
		fflush(stdout);
		memset(*my_nickname, 0, NICK_LEN);

		if (fgets(user_input, NICK_LEN, stdin) == NULL) {
		     perror("fgets");
		     free(my_nickname);
		     exit(EXIT_FAILURE);
		}
		
		if (strncmp(user_input, "/nick", strlen("/nick")) == 0){
			printf("[Server] : Invalid format, please login with : /nick <your pseudo>");
			return -2;
		}
		
      if (*(user_input + strlen("/nick")) == '\n') {
      // L'utilisateur n'a pas renseigné de nickname après "/nick "
         printf("[Server] : Pseudonyme non renseigné, utilisez le format \"/nick <pseudonyme>\".\n");
         return -2;
      }
		
		if (*(user_input + strlen("/nick")) != ' ') {
	    		printf("[Server] : Veuillez insérez un espace entre la commande et le pseudonyme\n");
	    		return -2;
	   }    

      if (strlen(user_input) - strlen("/nick ") > NICK_LEN+1) { // Prise en compte de \n
			printf("[Server] : Le pseudonyme est trop long. La longueur maximale est de %d caractères.\n", NICK_LEN);
         return -2;
      }
		
		int input_size = strlen(user_input)-1; // retrait du \n
		user_input[input_size]='\0';
		
		
		// Remplissage de la structure msg
		msg.type = NICKNAME_NEW;	// Champ msg->type
   	msg.nick_sender[0] = '\0';  // Champ msg->nick_sender
		msg.pld_len = strlen(user_input + strlen("/nick "));
		strcpy(msg.infos, user_input + strlen("/nick "));
		
		
		// Envoi du pseudo
		int ret = send2steps(fd, msg, user_input + strlen("/nick "));
		if (ret == -1) {
			return -1; // Erreur d'envoi
		}
			
		char *payload = NULL;
			
		int res = receive2steps(fd, &msg, &payload);
		if (res == -1) {
			return -1;
			}
			
		if (strcmp(payload, "0")) {
			strcpy(*my_nickname, payload);
			printf("[Server] : Welcome to the chat %s !\n", *my_nickname);
		}					
		else if (strcmp(payload, "1"))
			printf("[Server] : Nickname already taken, please try again.\n");					
		else if (strcmp(payload, "2")) 
			printf("[Server] : Invalid nickname. Please try again using only alphanumeric characters.\n");
		free(payload);						
	}
}







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
		freeaddrinfo(result);
      exit(EXIT_FAILURE);
	}
	
	int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (fd == -1) {
		perror("socket"); 
		freeaddrinfo(result); 
		exit(EXIT_FAILURE);
	}
	
	ret = connect(fd, result->ai_addr, result->ai_addrlen);
	if (ret == -1) {
		perror("connect");
		close(fd); 
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	
	int received = 0;
	char buf_flag[1];
	while (received != sizeof(char)) {
	// Réception de la structure dans un premier temps	
		int ret = read(fd, buf_flag + received, sizeof(char) - received);
		if (ret == -1) {
			perror("read");
			close(fd);
			exit(EXIT_FAILURE); // Erreur de lecture
		}
		received += ret;
   }
	printf("Connecting to server ... done!\n"); //	N'afficher ça que si le tableau fds[max_conn] du serveur a de la place; le serveur m'envoie un flag pour me le notifier. (lecture d'un char)
	

	char* my_nickname = NULL; // stocke le pseudo de l'utilisateur

// Choix du pseudo avant le début de la boucle principale
	ret = initialNickname(fd, &my_nickname);
	if (ret == -1) {
		perror("write");
		close(fd);
		free(my_nickname);
		exit(EXIT_FAILURE);
	}
		
	struct pollfd fds[2]; // Un descr. pour l'entrée, l'autre (fd) pour la socket 
	memset(fds, 0, 2*sizeof(struct pollfd));
	fds[0].fd = 0; // stdin
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = fd; // socket
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	

	while (1) {
		
		int ret = poll(fds, 2, -1); // Call poll that awaits new events

		if (ret == -1) {
			perror("poll");
		   break;
		}

		if (fds[0].revents & POLLIN) { // Activité sur stdin
		 	// Lire stdin et envoyer au serveur
		   char buf[256];
		   if (fgets(buf, sizeof(buf), stdin) == NULL) {
 			   perror("fgets");
 			   break;
			}
		   
			struct message msg;
		   char payload[INFOS_LEN];
		   
		   // Remplissage msg
		   int res = prepare_message(&msg, buf, my_nickname, payload);
			
			
			if (res == 0) {
				send2steps(fds[1].fd, msg, payload);				
			}

		   printf("\nMessage envoyé. Vous pouvez envoyer un nouveau message :\n");
    		fflush(stdout);
			
		   if (strcmp(buf, "/quit\n") == 0) { // C'est un echo_send
         	printf("Connexion fermée\n");
            break;
         }
		}

   	if (fds[1].revents & POLLIN) { // Activité sur la socket (message du serveur)
 	 	// Lire le message du serveur 
			
			struct message msg;
			memset(&msg, 0, sizeof(struct message));
			
			char *payload = NULL;
			
			int res = receive2steps(fds[1].fd, &msg, &payload);
			if (res == -1) {
				perror("read"); 
				close(fds[1].fd);
				free(my_nickname);
				exit(EXIT_FAILURE);
			}
			
			int type = msg.type;
			
			switch (type) {
				case NICKNAME_NEW:
					if (strcmp(payload, "0")) {
						strcpy(my_nickname, payload);
						printf("[Server] : Nickname successfully changed to %s !\n", my_nickname);
					}					
					else if (strcmp(payload, "1"))
						printf("[Server] : Nickname already taken, please try again.\n");					
					else if (strcmp(payload, "2")) 
				 		printf("[Server] : Invalid nickname. Please try again using only alphanumeric characters.\n");					
					break;				

				case NICKNAME_LIST:
										
					char list[INFOS_LEN];
					strcpy(list, "[Server] : Online users are");
					
					int i = 0;
					char name[NICK_LEN];
					while (payload[i] != '.') {
						strcat(list, "\n							- ");
						while(payload[i] != ',') {
							name[i] = payload[i];
							i+=1; 
						}
						strcat(list, name);
						i += 1;
					}
					printf("%s\n", list);
					break;
					
				case NICKNAME_INFOS:
					printf("%s\n", payload);
					break;
				
				case UNICAST_SEND:
					if (strlen(payload) > 1) {
						printf("%s\n", payload); 
					}
					break;
					
				case ECHO_SEND:				
					printf("[Server] : %s\n", payload);
					break;
			}
			
			free(payload);
			fds[1].revents = 0;

      }
	}
	free(my_nickname);
	freeaddrinfo(result);
	close (fds[1].fd);
	printf("User disconnected\n");
	return 0;	
}

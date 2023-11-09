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
#include <ctype.h>

#include "common.h"
#include "msg_struct.h"

// Cette fonction valide le format des commandes utilisateur en fonction du type de message.
// Elle vérifie la correspondance de la commande avec le type de message attendu, la présence d'un éventuel pseudonyme et d'un éventuel message, et leurs format et longueur.
// Elle renvoie -1 en cas de format invalide et 0 sinon.
int valid_format(enum msg_type type, char *user_input) {
	
	switch (type) {

		case NICKNAME_NEW:
	
			if (strncmp(user_input, "/nick", strlen("/nick")) != 0){
				printf("[Server] : Invalid format. Please use \"/nick <your nickname>\".\n");
				return -1;
			}
			
			if ( (strcmp(user_input, "/nick") == 0) || (strcmp(user_input, "/nick ") == 0) ) {
				// L'utilisateur n'a pas renseigné de nickname après "/nick "
				printf("[Server] : Nickname not provided. Please use \"/nick <your nickname>\".\n");
				return -1;
		  	}
			
			if (*(user_input + strlen("/nick")) != ' ') {
				printf("[Server] : Please add a space between \"/nick\" and your username.\n");
		   		return -1;
		   	}    

		  	if (strlen(user_input) - strlen("/nick ") > NICK_LEN) {
				printf("[Server] : The provided username exceeds the maximum length of %d characters. Please try again.\n", NICK_LEN);
		  		return -1;
		  	}		
         
			// Vérification que le pseudo ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/nick ")); i++) {
				if (!isalnum( *(user_input + strlen("/nick ") + i) )) {
        	    // Parcours du pseudo : Si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid nickname. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}
         
      		break;

      	case NICKNAME_INFOS:
			
			if ( (strcmp(user_input, "/whois") == 0) || (strcmp(user_input, "/whois ") == 0) ) {
			// L'utilisateur n'a pas renseigné de nickname après "/whois "
			   	printf("[Server] : Nickname not provided. Please use \"/whois <nickname>\".\n");
		  		return -1;
		  	}
		
			if (*(user_input + strlen("/whois")) != ' ') {
				printf("[Server] : Please add a space between \"/whois\" and the username.\n");
		   		return -1;
		  	}    
		
			if (strlen(user_input) - strlen("/whois ") > NICK_LEN) {
				printf("[Server] : The provided username exceeds the maximum length of %d characters. Please try again.\n", NICK_LEN);
		  		return -1;
		  	}
		  	
			break;

		case NICKNAME_LIST:
			// Commandes acceptées : "/who", "/who ","/who <quelque chose>"
			if ( strcmp(user_input, "/who") == 0 || strcmp(user_input, "/who ") == 0 || strncmp(user_input, "/who ", strlen("/who ")) == 0 ) {
	    	    return 0;
         	} else {
        		printf("[Server] : Incorrect format. Please use one the accepted command formats : \"/who\", \"/who \",\"/who <anything>\"\n");
         		return -1;
			}
			
			break;
			
		case UNICAST_SEND:
			// Cas où l'utilisateur ne fournir aucun argument
			if ( (strcmp(user_input, "/msg") == 0) || (strcmp(user_input, "/msg ") == 0) ) {
			    printf("[Server] : No input arguments. Please use \"/msg <nickname> <message>\".\n");
			  	return -1;
			}
			// Cas où l'utilisateur colle les arguments à la commande
			if ( *(user_input + strlen("/msg")) != ' ' )  {
				printf("[Server] : A space should be added after \"/msg\". Please use \"/msg <nickname> <message>\".\n");     	
	      		return -1;
	      	}
	      
			// Contrôle de taille
			// Recherche de l'espace séparant le pseudo du message
	      	int idx_input = strlen("/msg "), idx_name = 0;
		  	int target_name_size = 0; 
		  	char name[NICK_LEN];
		  	while (user_input[idx_input] != ' ' && user_input[idx_input] != '\0') {
		  		name[idx_name] = user_input[idx_input];
				target_name_size += 1; 
				idx_input += 1;
				idx_name += 1;
			}
					
			// Cas de la donnée d'une chaîne sans aucun espace après "/msg "
			if (target_name_size > NICK_LEN){
				printf("[Server] : The provided nickname exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
	      		return -1;
			}
			
			if (user_input[idx_input] == ' ' && user_input[idx_input+1] == '\0'){
				idx_input += 1;  // Pour passer au message après l'espace
			}
			
			else if (user_input[idx_input] == '\0' || user_input[idx_input+1] == '\0'){ // Cas de l'absence de message 
				printf("[Server] : No message provided. Please use \"/msg <nickname> <message>\".\n");
				return -1;
			}	
			// Contrôle de taille du message
			if (strlen(user_input) - strlen("/msg ") - strlen(name) > INFOS_LEN) {
	        	printf("[Server] : The provided message exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
            	return -1;
         	}
		   
         	break;

		case BROADCAST_SEND:
			
			if ( (strcmp(user_input, "/msgall") == 0) || (strcmp(user_input, "/msgall ") == 0) ) {
			// L'utilisateur n'a pas renseigné de message
		    	printf("[Server] : Message not provided. Please use \"/msgall <message>\".\n");
		  		return -1;
			}
	      
			if (*(user_input + strlen("/msgall")) != ' ') {
				printf("[Server] : Please add a space between \"/msgall\" and the message.\n");
		   		return -1;
			} 
	      						
			if (strlen(user_input) - strlen("/msgall ") > INFOS_LEN) {
				printf("[Server] : The provided message exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
				return -1;
			}			
			
			break;
	}
    
			
   	return 0; // Format et longueur valides
}



// La fonction prepare_message remplit une structure message en fonction de la commande utilisateur et donc du type de message.
// Elle utilise la fonction valid_format pour effectuer des vérifications préliminaires sur le format de la commande. Ensuite, elle remplit le payload et la structure message, les rendant prêts à l'envoi.
// Elle renvoie -1 en cas de format invalide, -2 en cas d'erreur et 0 en cas de succès.
int prepare_message(struct message *msg, char *user_input, const char *nick_sender, char *payload) {
	
	msg->infos[0] = '\0'; // Rempli si besoin
	enum msg_type type = 0;
	int ret = -2;
		    
	if (strncmp(user_input, "/nick", strlen("/nick")) == 0) { 
		type = NICKNAME_NEW;
   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   	strcpy(payload, user_input + strlen("/nick "));
   	}
	
   	else if (strncmp(user_input, "/msgall", strlen("/msgall")) == 0) {
		type = BROADCAST_SEND;
		ret = valid_format(type, (char *)user_input);
		if (ret == -1) {
			return -1;
		}
		
		strcpy(payload, user_input + strlen("/msgall "));
	}
	
	else if (strncmp(user_input, "/msg", strlen("/msg")) == 0) { 
	   	type = UNICAST_SEND;
	   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   		strcpy(payload, user_input + strlen("/msg "));
  	}	   

   	else if (strncmp(user_input, "/whois", strlen("/whois")) == 0) { 
		type = NICKNAME_INFOS;
	   	ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
   		strcpy(payload, user_input + strlen("/whois "));
   	}
   
   	else if (strncmp(user_input, "/who", strlen("/who")) == 0){
   		type = NICKNAME_LIST;
   		ret = valid_format(type, user_input);
		if (ret == -1) {
			return -1;
		}
	}	
	
	else {
		type = ECHO_SEND; // pas d'infos extraites
   		strcpy(payload, user_input);
   	} 
   
	// Remplissage de la structure msg

   	msg->type = type;	// Champ msg->type
   	strcpy(msg->nick_sender, nick_sender); // Champ msg->nick_sender

  	msg->pld_len = strlen(payload);

   	if (type == NICKNAME_NEW || type == NICKNAME_INFOS || type == UNICAST_SEND) {
    	strncpy(msg->infos, payload, INFOS_LEN);
   	}
   
  	return 0; // Format valide, structure msg remplie et payload stocké
}




// Cette fonction permet de gérer un envoi à un serveur en deux temps : un premier message contenant la struct message suivi préremplie d'un second contenant les octets utiles.
// Elle renseigne aussi sur le statut de l'envoi en retournant "0" en cas de succès et "-1" en cas d'erreur. 
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
	//	Envoi du payload 
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

// Cette fonction permet de gérer la réception d'envoi par un serveur en deux temps : La réception de la struct message suivie des octets utiles.
// Elle renseigne aussi sur le statut de la lecture en retournant "0" en cas de succès, "-1" en cas d'erreur de lecture et "-2" en cas d'erreur lors de l'allocation dynamique de mémoire.
int receive2steps(int fd, struct message *msg, char **payload) {

	int received = 0;
	
	while (received != sizeof(struct message)) {
	// Réception de la structure dans un premier temps	
		int ret = read(fd, (char *)msg + received, sizeof(struct message) - received);
		if (ret == -1)
			return -1; // Erreur de lecture
					
		received += ret;
   	}
		
	int payload_size = msg->pld_len;

	*payload = (char *)malloc(payload_size);
	if (*payload == NULL) {
		return -2; // Pour différencier du retour d'erreur du read
	}
	memset(*payload, 0, payload_size);
	
	(*payload)[payload_size] = '\0';
	
	received = 0;
	while (received != payload_size) {
	// Réception des données du champ infos
		int ret = read(fd, *payload + received, payload_size - received);
		if (ret == -1){
		 	free(*payload);
			return -1; // Erreur de lecture
		}
		received += ret;
	}
	
	return 0; // succès
}




// Cette fonction gère l'attribution initiale du pseudo pour un nouvel utilisateur. 
// Elle utilise une boucle de vérification du format en appelant la fonction prepare_message puis envoie la demande de création de pseudonyme au serveur, qui répond avec un flag (0 en cas de succès, 1 si le pseudonyme renseigné est déjà utilisé)
// Elle renvoie -1 en cas d'erreur, 0 en cas de succès.
int initialNickname(int fd, char **my_nickname) {
	char user_input[512]; // buffer qui sert à remplir le champ msg.infos au 1er envoi 
	user_input[0] = '\0'; 	
	
	*my_nickname = (char *)malloc(NICK_LEN);
	if (*my_nickname == NULL) {
   	perror("malloc");
    	return -1;
  	}
	
	struct message	msg;
	memset(&msg, 0, sizeof(struct message));
	
	printf("[Server] : please login with /nick <your pseudo>\n");
	fflush(stdout);
	
	while (1) {

		memset(user_input, 0, strlen(user_input));

		if (fgets(user_input, sizeof(user_input), stdin) == NULL) {
		     perror("fgets");
		     free(my_nickname);
		     exit(EXIT_FAILURE);
		}
		
		user_input[strlen(user_input)-1]='\0'; // retrait du \n
		
		if (strncmp(user_input, "/nick", strlen("/nick")) != 0){
			printf("[Server] : Invalid format, please login with : /nick <your nickname>\n");
			continue;
		}
		
		if ( (strcmp(user_input, "/nick") == 0) || (strcmp(user_input, "/nick ") == 0) ) {
		// L'utilisateur n'a pas renseigné de nickname après "/nick "
	        printf("[Server] : Nickname not provided, use \"/nick <your nickname>\".\n");
	      	continue;
		}
		
		if (*(user_input + strlen("/nick")) != ' ') {
	    	printf("[Server] : Please add a space between \"/nick\" and your username.\n");
	   		continue;
	   	}    

      	if (strlen(user_input) - strlen("/nick ") > NICK_LEN) {
			printf("[Server] : Username is too long. Please use a username with a maximum length of %d characters.\n", NICK_LEN);
      		continue;
      	}

		int special_character_detected = 0;
		// Vérification que le pseudo ne contient pas un caractère spécial
		for (int i = 0; i < strlen(user_input + strlen("/nick ")); i++) {
			if (!isalnum( *(user_input + strlen("/nick ") + i) )) {
        	    // Parcours du pseudo : Si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
        		special_character_detected = 1;
            	printf("[Server] : Invalid nickname. Please try again using only alphanumeric characters without spaces.\n");
				break; 
			}
		}
		if (special_character_detected)
			continue;			
		
		// Remplissage de la structure msg
		msg.type = NICKNAME_NEW;
   		msg.nick_sender[0] = '\0';
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
		if (strcmp(payload, "0") == 0) {
			strcpy(*my_nickname, user_input + strlen("/nick "));
			printf("[Server] : Welcome to the chat %s !\n", *my_nickname);
			break;
		}					
		else if (strcmp(payload, "1") == 0)
			printf("[Server] : Nickname already taken, please try again.\n");					
			
		free(payload);						
	}
	return 0;
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
	printf("Connecting to server ... done!\n");
	

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
	
	int nicknew_count = 0; // compteur utilisé pour distinguer le choix initial du pseudo des changements de pseudos
	while (1) {
		
		int ret = poll(fds, 2, -1); // Call poll that awaits new events

		if (ret == -1) {
			perror("poll");
		   break;
		}
		
		char input_buffer[256];
		if (fds[0].revents & POLLIN) { // Activité sur stdin
		 	// Lire stdin et envoyer au serveur
		  	if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
 				perror("fgets");
 				break;
			}
		    input_buffer[strlen(input_buffer)-1]='\0'; // retrait du \n	   
		   	struct message msg;
		   	char payload[INFOS_LEN];
		   
		   	// Remplissage msg
		   	int res = prepare_message(&msg, input_buffer, my_nickname, payload);
			
			if (res == 0) {
				send2steps(fds[1].fd, msg, payload);				
			}
         	
         	// Cas d'une demande de déconnexion
		   	if (strcmp(payload, "/quit") == 0 || strcmp(payload, "/quit ") == 0) {
         		printf("[Server] : Request successfull. You have been disconnected.\n");
            	break;
         	}
         	
         	fds[0].revents = 0;
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
					if (strcmp(payload, "0") == 0) {
						nicknew_count += 1;
						
						if (nicknew_count >= 1){ // Pour différencier le premier choix de pseudo et les changements ultérieurs
						strcpy(my_nickname, input_buffer);
						printf("[Server] : Nickname successfully changed to %s !\n", my_nickname + strlen("/nick "));
						}
					}					
					else if (strcmp(payload, "1") == 0)
						printf("[Server] : Nickname already taken, please try again.\n");				
					else if (strcmp(payload, "2") == 0) 
				 		printf("[Server] : Invalid nickname. Please try again using only alphanumeric characters.\n");					
					break;				

				case NICKNAME_LIST:
										
					char list[NICK_LEN*MAX_CONN];
					strcpy(list, "[Server] : Online users are");
					
					int idx_payload = 0, idx_name = 0; 
					char name[NICK_LEN];
					while (payload[idx_payload] != '.') {
						strcat(list, "\n			- ");
						
						memset(name, 0, sizeof(name));
						idx_name = 0;
						while( payload[idx_payload] != ',' && payload[idx_payload] != '.' ) {
							name[idx_name] = payload[idx_payload];
							idx_name+=1;
							idx_payload+=1; 
						}
						//name[strlen(name)]='\0';
						strcat(list, name);
						// Pour passer à la position après le séparateur ", "
						if (payload[idx_payload] == ',')
							idx_payload += 2; 
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
				
				case BROADCAST_SEND:
					printf("%s\n", payload); 
					break;
					
				case ECHO_SEND:
					printf("%s\n", payload);
					break;
			}
			
			free(payload);
			fds[1].revents = 0;
		}
      
	}
	
	free(my_nickname);
	freeaddrinfo(result);
	close (fds[1].fd);
	printf("Have a nice day :)\n");
	return 0;	
}

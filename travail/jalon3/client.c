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


		/////////////////////////////////////////////////////////////////////
		//////////////////   JALON 3 : MULTICAST   //////////////////////////	
		
		case MULTICAST_CREATE:
			if ( (strcmp(user_input, "/create") == 0) || (strcmp(user_input, "/create ") == 0) ) {
			// L'utilisateur n'a pas renseigné de nickname après "/create "
			   	printf("[Server] : Channel name not provided. Please use \"/create <channel_name>\".\n");
		  		return -1;
		  	}
		
			if (*(user_input + strlen("/create")) != ' ') {
			// L'utilisateur n'a pas inséré d'espace après "/create"
				printf("[Server] : Please add a space between \"/create\" and the channel name.\n");
		   		return -1;
		  	}    
		
			if (strlen(user_input) - strlen("/create ") > CHANNEL_NAME_LEN) {
			// L'utilisateur a renseigné un nom de salon trop long
				printf("[Server] : The provided channel name exceeds the maximum length of %d characters. Please try again.\n", CHANNEL_NAME_LEN);
		  		return -1;
		  	}
			
			break;
			
			// Vérification que le channel_name ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/create ")); i++) {
				if (!isalnum( *(user_input + strlen("/create ") + i) )) {
        	    // Parcours du channel_name : si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid channel name. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}
		
			
		case MULTICAST_LIST:
			// Commandes acceptées : "/channel_list", "/channel_list ", "/channel_list <quelque chose>"
			if ( strcmp(user_input, "/channel_list") == 0 ||
				 strcmp(user_input, "/channel_list ") == 0 ||
				 strncmp(user_input, "/channel_list ", strlen("/channel_list ")) == 0 ) {
	    	    return 0;
         	} else {
        		printf("[Server] : Incorrect format. Please use one the accepted command formats : \"/channel_list\", \"/channel_list \",\"/channel_list <anything>\"\n");
         		return -1;
			}
			
			break;

			
		
		case MULTICAST_JOIN:
			
			if ( (strcmp(user_input, "/join") == 0) || (strcmp(user_input, "/join ") == 0) ) {
			// L'utilisateur n'a pas renseigné de salon après "/join "
			   	printf("[Server] : Channel name not provided. Please use \"/join <channel_name>\".\n");
		  		return -1;
		  	}
		
			if (*(user_input + strlen("/join")) != ' ') {
			// L'utilisateur n'a pas inséré d'espace après "/join"
				printf("[Server] : Please add a space between \"/join\" and the channel name.\n");
		   		return -1;
		  	}    
		
			if (strlen(user_input) - strlen("/join ") > CHANNEL_NAME_LEN) {
			// L'utilisateur a renseigné un nom de salon trop long
				printf("[Server] : The provided channel name exceeds the maximum length of %d characters. Please try again.\n", CHANNEL_NAME_LEN);
		  		return -1;
		  	}
			
			// Vérification que le channel_name ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/join ")); i++) {
				if (!isalnum( *(user_input + strlen("/join ") + i) )) {
        	    // Parcours du channel_name : si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid channel name. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}
			
			break;
			
	
		
		case MULTICAST_SEND:
			if (strlen(user_input) > INFOS_LEN) {
				printf("[Server] : The provided message exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
				return -1;
			}
			break;
		
		
		case MULTICAST_QUIT:
		
		/*  Tests implicitement vérifiés dans prepare_message normalement
			
			if ( (strcmp(user_input, "/quit") == 0) || (strcmp(user_input, "/quit ") == 0) ) {
			// L'utilisateur n'a pas renseigné de nom de salon après "/quit "
			   	printf("[Server] : Channel name not provided. Please use \"/join <channel_name>\".\n");
		  		return -1;
		  	}
		
		
		
			if (*(user_input + strlen("/quit")) != ' ') {
			// L'utilisateur n'a pas inséré d'espace après "/quit"
				printf("[Server] : Please add a space between \"/quit\" and the channel name.\n");
		   		return -1;
		  	}    
		
		*/////////////
			
			
			if (strlen(user_input) - strlen("/quit ") > CHANNEL_NAME_LEN) {
			// L'utilisateur a renseigné un nom de salon trop long
				printf("[Server] : The provided channel name exceeds the maximum length of %d characters. Please try again.\n", CHANNEL_NAME_LEN);
		  		return -1;
		  	}
			break;
	

			// Vérification que le channel_name ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/create ")); i++) {
				if (!isalnum( *(user_input + strlen("/create ") + i) )) {
        	    // Parcours du channel_name : si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid channel name. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}

		/////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////



	}
    
			
   	return 0; // Format et longueur valides
}


// Cette fonction prend le type de message en paramètre et renvoie la sous-chaîne correspondante !!!AVEC L'ESPACE!!!.
// Cela rend modulaire la préparation du payload dans prepare_message.
const char* get_command_string(enum msg_type type) {
    switch (type) {
		case NICKNAME_NEW:
            return "/nick ";
        case BROADCAST_SEND:
            return "/msgall ";
        case UNICAST_SEND:
            return "/msg ";
        case NICKNAME_INFOS:
            return "/whois ";
        case NICKNAME_LIST:
            return "/who ";
        default:
            return ""; // Pour le cas ECHO_SEND où il n'y a pas de préfixe
    }
}


// La fonction prepare_message remplit une structure message en fonction de la commande utilisateur et donc du type de message.
// Elle utilise la fonction valid_format pour effectuer des vérifications préliminaires sur le format de la commande. Ensuite, elle remplit le payload et la structure message, les rendant prêts à l'envoi.
// Elle renvoie -1 en cas de format invalide, -2 en cas d'erreur et 0 en cas de succès.
int prepare_message(struct message *msg, char *user_input, const char *nick_sender, char *payload, char *my_channel, int in_channel) {
	
	msg->infos[0] = '\0'; // Rempli si besoin
	enum msg_type type = 0;
	int ret = -2;
		    
	if (strncmp(user_input, "/nick", strlen("/nick")) == 0)
		type = NICKNAME_NEW;
	
   	else if (strncmp(user_input, "/msgall", strlen("/msgall")) == 0)
		type = BROADCAST_SEND;
	
	else if (strncmp(user_input, "/msg", strlen("/msg")) == 0) 
	   	type = UNICAST_SEND;	   

   	else if (strncmp(user_input, "/whois", strlen("/whois")) == 0) 
		type = NICKNAME_INFOS;
	   	
   
   	else if (strncmp(user_input, "/who", strlen("/who")) == 0)
   		type = NICKNAME_LIST;
	
	
	/////////////////////////////////////////////////////////////////////
	//////////////////   JALON 3 : MULTICAST   //////////////////////////

   	else if (strncmp(user_input, "/create", strlen("/create")) == 0)
   		type = MULTICAST_CREATE;	
	
   	else if (strncmp(user_input, "/channel_list", strlen("/channel_list")) == 0)
   		type = MULTICAST_LIST;	
	
   	else if (strncmp(user_input, "/join", strlen("/join")) == 0)
   		type = MULTICAST_JOIN;		
   	
   	else if ( strncmp(user_input, "/quit", strlen("/quit")) == 0 || strncmp(user_input, "/quit ", strlen("/quit ")) == 0 ) {// FAIRE LA DIFF ENTRE "/quit <CHANNNEL>" ET "/quit" 
   		if ( strcmp(user_input, "/quit") == 0 || strcmp(user_input, "/quit ") == 0 )
   			type = ECHO_SEND;
   		else if (strlen(user_input) > strlen("/quit ") && in_channel == 1)
   			if (strcmp(user_input+strlen("/quit "), my_channel) == 0 )
   				type = MULTICAST_QUIT;
   	}		
	
	else {
		if (in_channel == 1)
			type = MULTICAST_SEND;
		else
			type = ECHO_SEND;
	}
	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////




	ret = valid_format(type, user_input);
   	if (ret == -1) {
		return -1;
	}

	strcpy(payload, user_input + strlen(get_command_string(type)));
	
	// Remplissage de la structure msg
   	msg->type = type;	
   	strcpy(msg->nick_sender, nick_sender);
  	msg->pld_len = strlen(payload);

   	if (type == NICKNAME_NEW ||		// Tous les types dont le champ infos est non vide.
		type == NICKNAME_INFOS ||
		type == UNICAST_SEND ||
		type == MULTICAST_CREATE ||
		type == MULTICAST_JOIN ||
		type == MULTICAST_QUIT ) {
    
    	strncpy(msg->infos, payload, INFOS_LEN);
   	}
	
	else if (type == MULTICAST_SEND)
		strncpy(msg->infos, my_channel, INFOS_LEN);
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
	

	char *my_nickname = NULL; // stocke le pseudo de l'utilisateur
	int nicknew_count = 0; // compteur utilisé pour distinguer le choix initial du pseudo des changements de pseudos

	// Choix du pseudo avant le début de la boucle principale
	ret = initialNickname(fd, &my_nickname);
	if (ret == -1) {
		perror("write");
		close(fd);
		free(my_nickname);
		exit(EXIT_FAILURE);
	}
	
	
		

		
	/////////////////////////////////////////////////////////////////////
	//////////////////   JALON 3 : MULTICAST   //////////////////////////
	char *my_channel = NULL; // stocke le nom du salon de discussion
	my_channel = (char *)malloc(CHANNEL_NAME_LEN);
	if (my_channel == NULL) {
    	perror("malloc");
    	close(fd);
    	free(my_nickname);
    	exit(EXIT_FAILURE);
	}
	
	int in_channel = 0; // 1 si l'utilisateur est dans un salon, 0 sinon
	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////





	struct pollfd fds[2]; // Un descr. pour l'entrée, l'autre (fd) pour la socket 
	memset(fds, 0, 2*sizeof(struct pollfd));
	fds[0].fd = 0; // stdin
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = fd; // socket
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	

	
	//printf("%s> ", my_nickname);  // Affichage du pseudo de l'utilisateur avant chaque commande
	//fflush(stdout);
	
	while (1) {
		
		// Affichage du pseudo et salon de l'utilisateur avant chaque commande
		if (in_channel == 1){
			printf("%s[%s]> ", my_nickname, my_channel);  	
			fflush(stdout);
		}
		else {
			printf("%s> ", my_nickname);
			fflush(stdout);
		}
		
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
		   	
		   	/************** Fonctionnalité supplémentaire : l'utilisateur peut connaître son username*******/
			/*                                              via la commande /me                            */		   
		   	if (strcmp(input_buffer, "/me") == 0 || strcmp(input_buffer, "/me ") == 0) {
         		printf("[Server] : You are : %s\n", my_nickname);
         		continue;
         	}	
         	/***********************************************************************************************/
		   	
		   	struct message msg;
		   	char payload[INFOS_LEN];
		   
		   	// Remplissage msg
		   	int res = prepare_message(&msg, input_buffer, my_nickname, payload, my_channel, in_channel);
			
			if (res == 0) {
				send2steps(fds[1].fd, msg, payload);				
			}
         	
         	// Cas d'une demande de déconnexion.
		   	if ( strcmp(payload, "/quit") == 0  || strcmp(payload, "/quit ") == 0 ) {
         		printf("[Server] : Request successful. You have been disconnected.\n");
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
					break;				

				case NICKNAME_LIST:
					
					char list_nickname[NICK_LEN*MAX_CONN + 5];
					strcpy(list_nickname, "[Server] : Online users are");
					
					int idx_payload = 0, idx_name = 0; 
					char name_nickname[NICK_LEN];
					while (payload[idx_payload] != '.') {
						strcat(list_nickname, "\n			- ");
						
						memset(name_nickname, 0, sizeof(name_nickname));
						idx_name = 0;
						while( payload[idx_payload] != ',' && payload[idx_payload] != '.' ) {
							name_nickname[idx_name] = payload[idx_payload];
							idx_name+=1;
							idx_payload+=1; 
						}
						strcat(list_nickname, name_nickname);
						
						// Pour afficher son propre pseudo
						if (strcmp(name_nickname, my_nickname) == 0)
							strcat(list_nickname, " (me)");
						
						// Pour passer à la position après le séparateur ", "
						if (payload[idx_payload] == ',')
							idx_payload += 2; 
					}
					printf("%s\n", list_nickname);
					
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
			
				/////////////////////////////////////////////////////////////////////
				//////////////////   JALON 3 : MULTICAST   //////////////////////////

				case MULTICAST_CREATE:
				
					if (strcmp(payload, "0") == 0) {
						printf("You have created channel %s\n", my_channel + strlen("/create "));
						strcpy(my_channel, input_buffer);
						printf("You have joined %s\n", my_channel + strlen("/create "));		
					}					
					else if (strcmp(payload, "1") == 0)
						printf("Channel name already taken, please try again.\n");		
					else if (strcmp(payload, "2") == 0)
						printf("Server has reached maximum channel capacity. Please try again.\n");
					break;
			
				case MULTICAST_LIST:
					if (payload[0] == '\0')
				        printf("No channels available.\n");						
					else {
						char list_channel[CHANNEL_NAME_LEN*MAX_CHANNELS + 14];
						strcpy(list_channel, "List of created channels :");
						
						idx_payload = 0, idx_name = 0; 
						char name_channel[CHANNEL_NAME_LEN];
						while (payload[idx_payload] != '.') {
							strcat(list_channel, "\n			- ");
							
							memset(name_channel, 0, sizeof(name_channel));
							idx_name = 0;
							while( payload[idx_payload] != ',' && payload[idx_payload] != '.' ) {
								name_channel[idx_name] = payload[idx_payload];
								idx_name+=1;
								idx_payload+=1; 
							}
							strcat(list_channel, name_channel);
							if (strcmp(name_channel, my_nickname) == 0)
								strcat(list_channel, " (<--)");
							
							
							
							// Pour passer à la position après le séparateur ", "
							if (payload[idx_payload] == ',')
								idx_payload += 2; 
						}
						printf("%s\n", list_channel);
					}
					
					break;

				case MULTICAST_JOIN:
					if (strcmp(msg.nick_sender, my_nickname) == 0)
					{
						if (strcmp(payload, "0") == 0) {
							strcpy(my_channel, input_buffer);
							in_channel = 1;
							printf("INFO> You have joined %s\n", my_channel + strlen("/join "));		
						}
						else if (strcmp(payload, "1") == 0)		///////// à revoir avec serveur
							printf("INFO> You are already in this channel.\n");							
						else if (strcmp(payload, "2") == 0) 
					 		printf("INFO> This channel does not exist. Please try again\n");
						else if (strcmp(payload, "3") == 0) 
					 		printf("INFO> Channel has reached maximum number of users. Please try again.\n");
					}

					else 
						printf("INFO> %s has joined %s\n", msg.nick_sender, my_channel + strlen("/join "));
					break;
					
				case MULTICAST_SEND:
					if (strlen(payload) > 1) {
						printf("%s> : %s\n", msg.nick_sender, payload); 
					}
					break;
				
				case MULTICAST_QUIT:
					if (strcmp(msg.nick_sender, my_nickname) == 0) {
						
						if (strcmp(payload, "oh gosh") == 0)
							printf("You were the last user in this channel, %s has been destroyed\n", my_channel);				
						
						else if (strcmp(payload, "you're so big") == 0)
							printf("INFO> You have left %s\n", my_channel);
        		        
        		        // Réinitialisation
        		        in_channel = 0;
        		        my_channel[0] = '\0';
					}
					
					else						
						printf("INFO> : %s has quit %s\n", msg.nick_sender, my_channel);						
					
					break;
			
				/////////////////////////////////////////////////////////////////////
				/////////////////////////////////////////////////////////////////////						
			
			}
			
			free(payload);
			fds[1].revents = 0;
		}
      
	}
	
	free(my_nickname);
	free(my_channel);
	freeaddrinfo(result);
	close (fds[1].fd);
	printf("Have a nice day :)\n");
	return 0;	
}

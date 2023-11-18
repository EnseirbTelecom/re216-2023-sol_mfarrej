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
#include <fcntl.h>
#include <sys/stat.h>
#include <libgen.h>

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
			// Cas où l'utilisateur ne fournit aucun argument
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
		  	char name[256]; // Allocation d'une taille plus grande que la taille maximale permise pour prendre en compte le cas où l'utilisateur écrit un pseudo trop long en premier argument de la commande
		  	name[0] = '\0';
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
			name[target_name_size] = '\0';
	
			if (user_input[idx_input] == ' '){
				idx_input += 1;  // Pour passer au message après l'espace	
			}
			else if (user_input[idx_input] == '\0'){ // Cas de l'absence de message , sans espace après le pseudo
				printf("[Server] : No message provided. Please use \"/msg <nickname> <message>\".\n");
				return -1;
			}
			
			if (strlen(user_input) - strlen("/msg ") - strlen(name) -1 == 0){ // Cas de l'absence de message , avec espace après le pseudo
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
			
			// Vérification que le channel_name ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/create ")); i++) {
				if (!isalnum( *(user_input + strlen("/create ") + i) )) {
        	    // Parcours du channel_name : si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid channel name. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}
		
			break;
			
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
		
			if ( (strcmp(user_input, "/quit") == 0) || (strcmp(user_input, "/quit ") == 0) ) {
				// L'utilisateur n'a pas renseigné de nom de salon après "/quit "
			   	printf("[Server] : Channel name not provided. Please use \"/quit <channel_name>\".\n");
		  		return -1;
		  	}
		
			if (*(user_input + strlen("/quit")) != ' ') {
			// L'utilisateur n'a pas inséré d'espace après "/quit"
				printf("[Server] : Please add a space between \"/quit\" and the channel name.\n");
		   		return -1;
		  	}    
			
			
			if (strlen(user_input) - strlen("/quit ") > CHANNEL_NAME_LEN) {
			// L'utilisateur a renseigné un nom de salon trop long
				printf("[Server] : The provided channel name exceeds the maximum length of %d characters. Please try again.\n", CHANNEL_NAME_LEN);
		  		return -1;
		  	}

			// Vérification que le channel_name ne contient pas un caractère spécial
			for (int i = 0; i < strlen(user_input + strlen("/create ")); i++) {
				if (!isalnum( *(user_input + strlen("/create ") + i) )) {
        	    // Parcours du channel_name : si un caractère n'est ni un chiffre ni une lettre de l'alphabet, c'est un caractère spécial
            		printf("[Server] : Invalid channel name. Please try again using only alphanumeric characters without spaces.\n");
					return -1;
				}
			}

			break;
			
		case FILE_REQUEST:
			// Cas où l'utilisateur ne fournit aucun argument
			if ( (strcmp(user_input, "/send") == 0) || (strcmp(user_input, "/send ") == 0) ) {
			    printf("[Server] : No input arguments. Please use \"/send <nickname> <file_name>\".\n");
			  	return -1;
			}
			// Cas où l'utilisateur colle les arguments à la commande
			if ( *(user_input + strlen("/send")) != ' ' )  {
				printf("[Server] : A space should be added after \"/send\". Please use \"/send <nickname> <file_name>\".\n");     	
	      		return -1;
	      	}
	      
			// Contrôle de taille
			// Recherche de l'espace séparant le pseudo du nom de fichier
	      	idx_input = strlen("/send "), idx_name = 0;
		  	target_name_size = 0; 
		  	//char name[256]; // Allocation d'une taille plus grand que la taille maximale permise pour prendre en compte le cas où l'utilisateur écrit un pseudo trop long en premier argument de la commande
		  	name[0] = '\0';
		  	while (user_input[idx_input] != ' ' && user_input[idx_input] != '\0') {
		  		name[idx_name] = user_input[idx_input];
				target_name_size += 1; 
				idx_input += 1;
				idx_name += 1;
			}
			
			
			// Cas de la donnée d'une chaîne sans aucun espace après "/send "
			if (target_name_size > NICK_LEN){
				printf("[Server] : The provided nickname exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
	      		return -1;
			}
			name[target_name_size] = '\0';
	
			if (user_input[idx_input] == ' '){
				idx_input += 1;  // Pour passer au nom de fichier après l'espace	
			}
			else if (user_input[idx_input] == '\0'){ // Cas de l'absence de nom de fichier, sans espace après le pseudo
				printf("[Server] : No file name provided. Please use \"/send <nickname> <file_name>\".\n");
				return -1;
			}
			
			if (strlen(user_input) - strlen("/send ") - strlen(name) -1 == 0){ // Cas de l'absence de nom de fichier , avec espace après le pseudo
				printf("[Server] : No file name provided. Please use \"/send <nickname> <file_name>\".\n");
				return -1;	
			}
				
			// Contrôle de taille du message
			if (strlen(user_input) - strlen("/send ") - strlen(name) > INFOS_LEN) {
	        	printf("[Server] : The provided file name exceeds the maximum length of %d characters. Please try again.\n", INFOS_LEN);
            	return -1;
         	}
			
			break;
		
		case FILE_ACK:
			// le serveur envoie la struct avec le <file_name> dans le champ infos, et un flag en payload pour indiquer l'acquittement ou le non-acquittement.
			// 0 (ack), 1 (non ack)
			break;
		
		
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
        case MULTICAST_CREATE:
            return "/create ";
        case MULTICAST_JOIN:
            return "/join ";
        case MULTICAST_QUIT:
            return "/quit ";
        case MULTICAST_LIST:
            return "/channel_list ";
        case FILE_REQUEST:
        	return "/send ";
        default:
            return ""; // Pour le cas ECHO_SEND et MULTICAST_SEND où il n'y a pas de préfixe    
    }
}

// Cette fonction permet de vérifier l'existence du fichier voulant être envoyé dans le répertoire /home
int fileExists(const char* filePath) {
    return access(filePath, F_OK) != -1;
}

// La fonction prepare_message remplit une structure message en fonction de la commande utilisateur et donc du type de message.
// Elle utilise la fonction valid_format pour effectuer des vérifications préliminaires sur le format de la commande. Ensuite, elle remplit le payload et la structure message, les rendant prêts à l'envoi.
// Elle renvoie -1 en cas de format invalide, -2 en cas d'erreur d'allocation de mémoire, -3 en cas de demande d'envoi de fichier de l'utilisateur à lui-même, -4 en cas de fichier à envoyer introuvable et 0 en cas de succès.
int prepare_message(struct message *msg, char *user_input, const char *nick_sender, char *payload, char *my_channel, int in_channel, char **sent_file_path_name) {
	
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
   		if ( (strcmp(user_input, "/quit") == 0 && in_channel == 0) || (strcmp(user_input, "/quit ") == 0 && in_channel == 0) )
   			type = ECHO_SEND;
   		else 
   			type = MULTICAST_QUIT;
   	}		
	
	else if (strncmp(user_input, "/send", strlen("/send")) == 0)
   		type = FILE_REQUEST;
   		
   			
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
	
	// Préparation du payload : tout ce qui vient après "/command " dans user_input, sauf dans le cas type = FILE_REQUEST
	
	if (type == FILE_REQUEST) {
			char nickname_receiver[NICK_LEN]; // Création d'un buffer pour extraire le pseudo de l'utilisateur destinataire
			
			char path_name[FILE_NAME_LEN]; // Création d'un buffer pour extraire le nom du fichier
			*sent_file_path_name = (char*)malloc(sizeof(path_name)); // Pour stocker le chemin du fichier à envoyer 
			if (*sent_file_path_name == NULL) {
				return -2;
			}
			// Extraction de <nickname_receiver> 
			int i = strlen("/send "), j = 0;
			while (user_input[i] != ' ' && user_input[i] != '\0'){
				nickname_receiver[j] = user_input[i];
				i++;
				j++;
			}
			nickname_receiver[j] = '\0';
			
			// Vérification que l'identité du destinataire est différente de celle de l'expéditeur
			if (strcmp(nickname_receiver, nick_sender) == 0) 
				return -3;	
			
			// On saute l'espace entre <username> et <file_name>
			while (user_input[i] == ' ')
				i++;
			
			// Extraction de <file_name>
			j = 0;
			while (user_input[i] != '\0') {
				if (user_input[i] == '"')
					i++;
				path_name[j] = user_input[i];
				i++;
				j++;
			}	
			path_name[j] = '\0';
			strcpy(*sent_file_path_name, path_name);
			
			
			// Vérification de l'existence du fichier demandé
			if (fileExists(path_name)) { // Le fichier demandé existe
				// Remplissage du champ infos avec le <nickname_receiver>
				strcpy(msg->infos, nickname_receiver);
				// Remplissage du payload avec <file_name>
				strcpy(payload, path_name);
			}
			
			else	// Le fichier demandé n'existe pas
				return -4;			
	}
	
	else // Remplissage du payload dans tous les autres cas de type de message
		strcpy(payload, user_input + strlen(get_command_string(type)));
	
	// Remplissage de la structure msg
   	msg->type = type;	
   	strcpy(msg->nick_sender, nick_sender);
  	msg->pld_len = strlen(payload);

   	if (type == NICKNAME_NEW ||		// Tous les types dont le champ infos est non vide, sauf FILE_REQUEST
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
	
	if (msg.type != NICKNAME_LIST && msg.type != MULTICAST_LIST && msg.type != FILE_REJECT)	{
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

	*payload = (char *)malloc(payload_size+1);
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
	    	printf("[Server] : Please add a space between \"/nick\" and your nickname.\n");
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






void initializeConnection(int *fd, struct addrinfo **result, const char *server_name, const char *server_port){
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	int ret = getaddrinfo(server_name, server_port, &hints, result);
    if (ret == -1) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    *fd = socket((*result)->ai_family, (*result)->ai_socktype, (*result)->ai_protocol);
    if (*fd == -1) {
        perror("socket");
        freeaddrinfo(*result);
        exit(EXIT_FAILURE);
    }

	ret = connect(*fd, (*result)->ai_addr, (*result)->ai_addrlen);
	if (ret == -1) {
		perror("connect");
		close(*fd); 
		freeaddrinfo(*result);
		exit(EXIT_FAILURE);
	}
	
	 // Reception du flag de confirmation
    int received = 0;
    char buf_flag[1];
    while (received != sizeof(char)) {
        ret = read(*fd, buf_flag + received, sizeof(char) - received);
        if (ret == -1) {
            perror("read");
            close(*fd);
            freeaddrinfo(*result);
            exit(EXIT_FAILURE); // Erreur de lecture
        }
        received += ret;
    }

    if (strcmp(buf_flag, "1") == 0) {
        printf("Server has reached maximum user capacity.\n");
        close(*fd);
        freeaddrinfo(*result);
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server ... done!\n");
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// Les fonctions handleX(...) s'occupent du traitement des messages reçus du serveur////////////////


// Cette fonction affiche le message d'erreur correspondant à la réponse du serveur pour une démande de création de salon
void handleMulticastCreate(const char *payload, char *my_channel, const char *input_buffer, int* in_channel, int* new_channel) {
    
    if (strcmp(payload, "0") == 0) {
    	strcpy(my_channel, input_buffer + strlen("/create "));
		*in_channel = 1;
		*new_channel = 1;
    }
    
    else if (strcmp(payload, "1") == 0) {
        printf("[Server] : Channel name already taken. Please try again.\n");
    }
    
    else if (strcmp(payload, "2") == 0) {
        printf("[Server] : The server has reached its maximum channel capacity. Please try again later.\n");
    }
}

// Cette fonction affiche la liste des salons créés
void handleMulticastList(const char *payload, char *list_channel, const char *my_nickname, const char *my_channel) {
    if (payload[0] == '\0') {
        printf("[Server] : There are not any channels available.\n");
    } else {
        strcpy(list_channel, "[Server] : Existing channels are :");

        int idx_payload = 0, idx_name = 0;
        char name_channel[CHANNEL_NAME_LEN];

        while (payload[idx_payload] != '.') {
            strcat(list_channel, "\n    - ");

            memset(name_channel, 0, sizeof(name_channel));
            idx_name = 0;

            while (payload[idx_payload] != ',' && payload[idx_payload] != '.') {
                name_channel[idx_name] = payload[idx_payload];
                idx_name += 1;
                idx_payload += 1;
            }

            strcat(list_channel, name_channel);
            
			// Pour afficher un indicateur sur le salon auquel il appartient
            if (strcmp(name_channel, my_channel) == 0)
                strcat(list_channel, " (your channel)");
			
            // Pour passer à la position après le séparateur ", "
            if (payload[idx_payload] == ',')
                idx_payload += 2;
        }

        printf("%s\n", list_channel);
    }
}



// Cette fonction affiche le message d'erreur correspondant à la réponse du serveur pour une demande de rejoindre un salon, et stocke le nom du salon.
void handleMulticastJoin(struct message msg, char *input_buffer, const char *payload, const char *my_nickname, char *my_channel, int* in_channel, int* just_joined) {
    
    if (strcmp(msg.nick_sender, my_nickname) == 0) {
    
        if (strcmp(payload, "0") == 0) {
            strcpy(my_channel, input_buffer + strlen("/join "));
            *in_channel = 1;
            *just_joined = 1;
        }
        
        else if (strcmp(payload, "1") == 0) 
            printf("[Server] : Cannot join %s. You are already in this channel.\n", my_channel);
        
        else if (strcmp(payload, "2") == 0)
            printf("[Server] : %s is not an existing channel name.\nThe list of all existing channels can be acquired using the command \"/channel_list\"\n.", input_buffer + strlen("/join "));
        
        else if (strcmp(payload, "3") == 0)
            printf("[Server] : %s has reached its maximum members capacity. Please try again later.\n", input_buffer + strlen("/join "));
    }
    	else
			printf("\n%s[%s]> INFO> %s\n", my_nickname, my_channel, payload);
    	
}



// Cette fonction affiche le message d'erreur correspondant à la réponse du serveur pour une démande de quitter un salon, et réinitialise les variables associées au salon si besoin
void handleMulticastQuit(struct message msg, char* input_buffer, const char *payload, const char *my_nickname, char *my_channel, int* in_channel) {
   
    if (strcmp(payload, "0") == 0) {
        //printf("INFO> You have left %s\n", my_channel);
        *in_channel = 0;    // Réinitialisation des variables associées au salon
        my_channel[0] = '\0';
    }
    
    else if (strcmp(payload, "1") == 0) {
        printf("[Server] : %s is not an existing channel name.\nThe list of all existing channels can be acquired using the command \"/channel_list\"\n.", input_buffer + strlen("/quit "));
    }
    
    else if (strcmp(payload, "2") == 0) {
        printf("[Server] : You are not part of %s channel. You can only quit your own channel : %s\n", input_buffer + strlen("/quit "), my_channel);
    }
    
    else if (strlen(payload) > 1) {
        // Cas où le client est à l'origine de la demande de départ : On vérifie que le payload est bien une notification de destruction de salon
        char notification[strlen(my_nickname)+ strlen("You were the last user in this channel,  has been destroyed\n")];
		strcpy(notification, "You were the last user in this channel, ");
		strcat(notification, my_channel);
		strcat(notification, " has been destroyed");
		if (strcmp(payload, notification) == 0) {
			printf("%s> INFO> %s\n", my_nickname, payload);
			*in_channel = 0; // Réinitialisation des variables associées au salon
			my_channel[0] = '\0';
		}
		else // Cas où il s'agit d'un client qui est notifié du départ
        	printf("\n%s[%s]> INFO> %s\n", my_nickname, my_channel, payload);
    }
}

// Cette fonction permet d'extraire adresse (char *) et numéro de port (int).
// Elle est utilisée à la réception d'un FILE_ACCEPT (point de vue émetteur) pour pouvoir créer les sockets servant à la connexion P2P.
void extractAddressAndPort(const char *payload, char *addr, int *port){
	int colon_position = 0;
    for (int i = 0; i < strlen(payload); i++) {
        if (payload[i] == ':') {
            colon_position = i;
            break;
        }
    }

    // Extraction de l'adresse, correspondant aux "colon_position"-ièmes premiers char du payload
    strncpy(addr, payload, colon_position);
    addr[colon_position] = '\0';

    // Extraction du port, correspondant à ce vient après le ':' du payload
    *port = atoi(payload + colon_position + 1);
}



// Cette fonction permet de gérer l'établissement de la connexion directe entre pairs pour un transfert de fichier, du côté du récepteur, en créant une socket d'écoute au niveau de laquelle la demande de connexion de l'émetteur sera acceptée, et parsuite la socket d'échange créée. 
// Elle retourne 0 en cas de succès et -1 en cas d'erreur de création de socket, -2 en cas d'erreur de bind, -3 en cas d'erreur de configuration en mode écoute, -4 en cas d'erreur d'accept
int establish_direct_connection_receiver_side(const char* receiver_IP, int receiver_port, 
											  int* listening_sfd, int* receiver_connection_sfd) {
    
    // Création de la socket d'écoute
	*listening_sfd = socket(AF_INET, SOCK_STREAM, 0);
    // Contrôle d'erreur
	if (*listening_sfd == -1)
		return -1;
    
	struct sockaddr_in listening_addr;
	memset(&listening_addr, 0, sizeof(listening_addr));
	// Configuration de la socket d'écoute
	setsockopt(*listening_sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	// Remplissage des informations d'adressage
	listening_addr.sin_family = AF_INET;
	listening_addr.sin_port = htons(receiver_port);
	inet_aton(receiver_IP, &listening_addr.sin_addr);
	
	// Association d'adresse à la socket d'écoute
	int ret = bind(*listening_sfd, (struct sockaddr *)&listening_addr, sizeof(struct sockaddr_in));
	// Contrôle d'erreur
	if (ret == -1)
		return -2;
		
	// Configuration de la socket en mode écoute
	ret = listen(*listening_sfd, 10);
	// Contôle d'erreur
	if (ret == -1)
		return -3;
		
    // Acceptation de la connexion de l'émetteur et création de la socket d'échange "receiver_connection_fd"
    struct sockaddr_in sender_addr;
    memset(&sender_addr, 0, sizeof(sender_addr));
    socklen_t len = sizeof(sender_addr);
    // Établissement de la connexion avec l'émetteur
    while(1){
    	*receiver_connection_sfd = accept(*listening_sfd, (struct sockaddr *)&sender_addr, &len);
		// Contrôle d'erreur
		if (*receiver_connection_sfd == -1)
			return -4;
		else
			break;
	}
	return 0;			
}			
			
			

// Cette fonction permet de gérer l'établissement de la connexion directe entre pairs pour un transfert de fichier, du côté de l'émetteur, en créant une socket d'échange et émettant une demande de connexion au récepteur. 
// Elle retourne 0 en cas de succès et -1 en cas d'erreur de création de socket, -2 en cas d'erreur de connexion.
int establish_direct_connection_sender_side(const char* receiver_IP, int receiver_port, int* sender_connection_sfd) {
    
	// Création de la socket de connexion pour l'envoi
	*sender_connection_sfd = socket(AF_INET, SOCK_STREAM, 0);
    // Contôle d'erreur
    if (*sender_connection_sfd == -1){ 
    	return -1;
	}
	struct sockaddr_in receiver_addr;
	memset(&receiver_addr, 0, sizeof(receiver_addr));
	// Remplissage de la structure du recepteur
	receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);
	inet_aton(receiver_IP, &receiver_addr.sin_addr);

	// Connexion au récepteur
	while (1){
		int ret = connect(*sender_connection_sfd, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
		// Contôle d'erreur
		if (ret == -1)
        	return -2;
    	else
    		break;
	}
    return 0;
}



// Cette fonction permet de gérer l'envoi d'un fichier à un récepteur auquel l'émetteur s'est préalablement connecté.
// Elle renvoie 0 en cas de succès, -1 en cas d'erreur d'ouverture de fichier, et -2 en cas d'erreur d'écriture.
int send_file(int sender_connection_sfd, const char* file_path, char *my_nickname) {
    
	// Envoi du message FILE_SEND

	// Remplissage de la structure
	struct message response_msg;
	memset(&response_msg, 0, sizeof(struct message));

	strcpy(response_msg.nick_sender, my_nickname); // 
	response_msg.type = FILE_SEND;
	strcpy(response_msg.infos, file_path);
	response_msg.pld_len = 0;
   	
	int sent = 0;
	while (sent != sizeof(struct message)) { 
		// Envoi de la structure dans un premier temps
	   	int ret = write(sender_connection_sfd, (char *)&response_msg + sent, sizeof(struct message) - sent);
		if (ret == -1) {
			return -1; // Erreur d'écriture
		}

		sent += ret;
	}
    // Ouverture du ficher pour la lecture
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) 
		return -1;
   
    // Lecture et envoi du fichier en morceaux
    char buffer[BUFFER_SIZE];
    int bytes_read, bytes_sent;
    while ( ( bytes_read = read(file_fd, buffer, sizeof(buffer)) ) > 0 ) { //tant qu'il n'y a pas d'erreur
    	buffer[bytes_read] = '\0';
    	int total_sent = 0;
    	while(total_sent != bytes_read){
			bytes_sent = write(sender_connection_sfd, buffer + total_sent, bytes_read - total_sent);
			if(bytes_sent == -1){
				close(file_fd);			
				return -2;
			}
    	total_sent += bytes_sent;
    	}
    }
    
	// Fermeture du fichier
	close(file_fd);
	return 0;
}
    


// Cette fonction permet, après la création d'un répertoire sur le home directory destiné au stockage des fichiers reçus, la construction du chemin à partir du nom du fichier à sauvegarder vers ce répertoire. 
char* construct_save_path(const char* file_name, const char* save_directory) {
    
    // Vérification de l'existence du dossier de sauvegarde, sinon création de celui-ci
    if (access(save_directory, F_OK) == -1) {
        if (mkdir(save_directory, 0755) == -1) {
            perror("creating save directory");
            return NULL;
        }
	}
    // Construction du chemin avec le dossier de sauvegarde
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", save_directory, file_name);

 

    // Allocation de mémoire pour rendre accessible le chemin
    char* result = malloc(strlen(full_path) + 1);
    if (result == NULL) {
        perror("malloc");
        return NULL;
    }
    memset(result, 0, strlen(full_path) + 1);

    strcpy(result, full_path);
    return result;
}


   
// Cette fonction permet de gérer la réception d'un fichier une fois la connexion avec l'émetteur établie.
// Elle renvoi 0 en cas de succès et -1 en cas d'erreur d'ouverture du fichier et -2 en cas d'erreur d'écriture et -3 en cas d'erreur lors de la construction du chemin de sauvegarde 
int receive_file(int receiver_connection_sfd, const char* save_directory, char** save_path) {
    
    int received = 0;
	struct message msg;
	while (received != sizeof(struct message)) {
	// Réception de la structure dans un premier temps	
		int ret = read(receiver_connection_sfd, (char *)&msg + received, sizeof(struct message) - received);
		if (ret == -1)
			return -1; // Erreur de lecture
					
		received += ret;
   	}	
	*save_path = construct_save_path(basename(msg.infos), save_directory); // On extrait le nom de fichier du payload (qui est le path du fichier voulant être envoyé)
							
	if(save_path == NULL)
		return -3;
		
    // Ouverture du fichier de sauvegarde, en le créant s'il n'éxiste pas déjà
    int file_fd = open(*save_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1)
        return -1;

    // Lecture et écriture du fichier en morceaux
    char buffer[BUFFER_SIZE];
    int bytes_received, bytes_written;
    while ( ( bytes_received = read(receiver_connection_sfd, buffer, sizeof(buffer)) ) > 0 ) { //tant qu'il n'y a pas d'erreur
    	buffer[bytes_received] = '\0';
        int total_written = 0;
        while (total_written < bytes_received) {
			bytes_written = write(file_fd, buffer + total_written, bytes_received - total_written);
            if (bytes_written == -1) {
                close(file_fd);
                return -2;
            }
            total_written += bytes_written;
        }
    }
    
    // Fermeture du fichier
    close(file_fd);
    return 0;
}


int handle_file_request(int server_fd, struct message *msg, char *payload, char *my_nickname, char **sent_file_path_name, int *p2p_counter){
	// PDV de l'émetteur : soit le serveur a transmis la demande FILE_REQUEST au récepteur, soit le pseudo récepteur saisi n'existe pas
	if (strcmp(payload, "0") == 0){
	// Cas où le serveur a transmis la demande de transfert de fichier au récepteur
		printf("File transfer request forwarded to %s\n", msg->infos); // msg.infos contient le pseudo récepteur
	}
	
	else if (strcmp(payload, "1") == 0)
	// Cas où le pseudo du récepteur n'existe pas 
		printf("[Server] : User %s does not exist\n", msg->infos);
	
	else {
		
		// PDV du récepteur : demander à l'utilisateur de saisir Y ou N au clavier pour accepter ou non la demande de transfert de fichier.
		
		// Informations d'adressage utilisées pour les connexions P2P.  
		char my_IP[] = "127.0.0.1"; // adresse associée au localhost
		int my_port = 8081 + *p2p_counter;
		
		// Les descripteurs de fichiers des sockets du récepteur P2P
		int listening_sfd = -1;
		int receiver_connection_sfd = -4;
		
		printf("[Server] : %s\n", payload);
		
		// Boucler (redemander un input) jusqu'à obtenir un input valide (test de format), i.e Y ou N.
		char response[64]; // Taille délibérément grande pour gérer le cas où l'utilisateur ne respecte pas le format
		int valid_input = 0;
		while (!valid_input) {
			printf("%s> ", my_nickname);
			memset(response, 0, strlen(response));						
			if (fgets(response, sizeof(response), stdin) == NULL) 
				return -1;
			response[strlen(response)-1] = '\0'; // Retrait du '\n'
			
			// Mise en majuscule de response
			for (int i=0; i < strlen(response); i++){
				response[i] = toupper(response[i]);
			}
			
			if (strcmp(response, "Y") == 0 || strcmp(response, "N") == 0)
				valid_input = 1;
			else // Si aucun des deux caractères attendus n'est saisi
				printf("[Server] : Invalid input, please use \"Y\"/\"y\" or \"N\"/\"n\".\n");
		}
		
		// Si le récepteur répond Y en recevant ce msg, on crée les sockets nécessaires au transfert de fichier
		if (strcmp(response, "Y") == 0) {						

			// Envoi du FILE_ACCEPT à l'émetteur
			struct message response_msg;
			
			// Conversion du port en chaîne de caractères
			char my_port_str[16];
			sprintf(my_port_str, "%d", my_port);						
			
			// Remplissage du payload (format addr:port)
			char addressAndPort[strlen(my_IP) + 1 + strlen(my_port_str)]; 
			strcpy(addressAndPort, my_IP);
			strcat(addressAndPort, ":");
			strcat(addressAndPort, my_port_str);

			
			// Remplissage de la structure
			strcpy(response_msg.nick_sender, my_nickname);
			response_msg.type = FILE_ACCEPT;
			response_msg.pld_len = strlen(addressAndPort);
			strcpy(response_msg.infos, msg->nick_sender);
			
			// Envoi à l'émetteur
			int ret = send2steps(server_fd, response_msg, addressAndPort);
			if (ret == -1) 
				return -2;
			
			// Établissement de la connexion P2P
			ret = establish_direct_connection_receiver_side(my_IP, my_port, &listening_sfd, &receiver_connection_sfd);
			if (ret == -1){
				perror("socket");
				close(listening_sfd);						
				return -3; // On abandonne la connexion P2P en cas d'erreur
			}
			else if (ret == -2){
				perror("bind");
				close(listening_sfd);
				return -3;
			}
			else if (ret == -3){
				perror("listen");
				close(listening_sfd);
				return -3;
			}
			else if (ret == -4) {
				perror("accept");
				close(listening_sfd);
				close(receiver_connection_sfd);
				return -3;
			}
			
			(*p2p_counter) += 1; // Incrémentation pour identifier le prochain récepteur P2P
			
			// Gestion de la réception du fichier
			printf("Receiving the file from %s...\n", msg->nick_sender);
			char save_directory[] = "../inbox";
			char* save_path = NULL;
			
			ret = receive_file(receiver_connection_sfd, save_directory, &save_path);
			if (ret == -1)
				return -4;
			else if (ret == -2){
				free(save_path);
				close(listening_sfd);
				close(receiver_connection_sfd);
				return -2;
			}
			else if (ret == -3)
				return -5;
			
			 printf("%s saved in re216-2023-sol_mfarrej/inbox/%s\n", basename(save_path), basename(save_path));
			
			
			// Fermeture des sockets associés au transfert de fichier							
			close(listening_sfd);
			close(receiver_connection_sfd);
		
			// Envoi du FILE_ACK

			char payload_ack[NICK_LEN];							
			strcpy(payload_ack, msg->nick_sender); // Le payload contient le pseudo de l'émetteur
			
			// Remplissage de la structure
			memset(&response_msg, 0, sizeof(struct message));

			strcpy(response_msg.nick_sender, my_nickname);
			response_msg.type = FILE_ACK;
			response_msg.pld_len = strlen(msg->nick_sender); 
			strcpy(response_msg.infos, basename(save_path));
			
			ret = send2steps(server_fd, response_msg, payload_ack);
			if (ret == -1) 
				return -2;

			free(save_path);
		}
	
		// Sinon, si le récepteur répond N, on prepare un FILE_REJECT
		else if	(strcmp(response, "N") == 0){
			struct message reject_msg;
			// Remplissage de la structure
			memset(&reject_msg, 0, sizeof(struct message));

			strcpy(reject_msg.nick_sender, my_nickname);
			reject_msg.type = FILE_REJECT;
			reject_msg.pld_len = 0; 
			strcpy(reject_msg.infos, msg->nick_sender);
			
			// Envoi du FILE_REJECT
			int ret = send2steps(server_fd, reject_msg, NULL);
			if (ret == -1)
				return -2;
		}
		
		return 0; 
	}
	return 1;
}


int handle_file_accept(const char *payload, char *my_nickname, char **sent_file_path_name, const char *nick_sender){
// Du point de vue émetteur, lorsqu'on reçoit le FILE_ACCEPT, on extrait adresse IP et port d'écoute du récepteur, puis on crée les sockets servant à la connexion directe au récepteur.		
	char receiver_IP[INET_ADDRSTRLEN];
	int receiver_port = -1; // Les numéros de port vont de 0 à 66535.
	
	extractAddressAndPort(payload, receiver_IP, &receiver_port);
	printf("%s accepted the file transfer. Receiver Address : %s Receiver Port : %d\n", nick_sender, receiver_IP, receiver_port);
	

	// Établissement de la connexion P2P côté émetteur
	int sender_connection_sfd = -1;
	int ret = establish_direct_connection_sender_side(receiver_IP, receiver_port, &sender_connection_sfd);
	if (ret == -1){
		close(sender_connection_sfd);
		return -6;
	}
	else if (ret == -2){ 
		close(sender_connection_sfd);
		return -7;
	}	
	printf("Connecting to %s and sending the file...\n", nick_sender);
				
	ret = send_file(sender_connection_sfd, *sent_file_path_name, my_nickname);
	if (ret == -1){ 
		close(sender_connection_sfd);
		return -4;
	}
	else if (ret == -2) {
		close(sender_connection_sfd);
		return -2;
	}		
	close(sender_connection_sfd);
	
	return 0;
}



void process_server_message(int server_fd, struct message msg, char *input_buffer, char *payload, char *my_nickname, char *my_channel, int *in_channel, int *just_joined, int *new_channel, int *waiting_stage, char **sent_file_path_name, int *p2p_counter, int *nicknew_count) {
	
	int type = msg.type;
	
	switch (type) {
		case NICKNAME_NEW:
			if (strcmp(payload, "0") == 0) {
				(*nicknew_count) += 1;
				if (*nicknew_count >= 1){ // Pour différencier le premier choix de pseudo et les changements ultérieurs
					strcpy(my_nickname, input_buffer + strlen("/nick "));
					printf("[Server] : Nickname successfully changed to %s !\n", my_nickname);
				}
			}					
			else if (strcmp(payload, "1") == 0)
				printf("[Server] : Nickname already taken. Please try again.\n");					
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
					strcat(list_nickname, " (you)");
				
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
			if (strlen(payload) > 1)
				printf("%s\n", payload); 
			break;
		
		case BROADCAST_SEND:
			printf("%s\n", payload); 
			break;
			
		case ECHO_SEND:
			printf("%s\n", payload);
			break;


		case MULTICAST_CREATE:
			
			handleMulticastCreate(payload, my_channel, input_buffer, in_channel, new_channel);
			break;
	
		case MULTICAST_LIST:
			
			char list_channel[CHANNEL_NAME_LEN*MAX_CHANNELS + 14];					
			handleMulticastList(payload, list_channel, my_nickname, my_channel);
			break;

		case MULTICAST_JOIN:
			
			handleMulticastJoin(msg, input_buffer, payload, my_nickname, my_channel, in_channel, just_joined);
			break;
			
		case MULTICAST_SEND:
			printf("%s> : %s\n", msg.nick_sender, payload);
			break;
		
		case MULTICAST_QUIT:
			
			handleMulticastQuit(msg, input_buffer, payload, my_nickname, my_channel, in_channel);					
			break;					
	
		case FILE_REQUEST: 
		
			int ret = handle_file_request(server_fd, &msg, payload, my_nickname, sent_file_path_name, p2p_counter);
			if(ret == -1){
				perror("fgets");
				
			}
			else if (ret == -2){
				perror("write");
				
			}
			else if (ret == -3){
				perror("establish_direct_connection_receiver_side");

			}
			else if (ret == -4){
				perror("open");
			}
			else if (ret == -5){
				perror("construct_save_path");
			}
								
			break;
		 
		case FILE_ACCEPT:
			ret = handle_file_accept(payload, my_nickname, sent_file_path_name, msg.nick_sender);
			if (ret == -2)
				perror("write");
			else if (ret == -4)
				perror("open");
			else if (ret == -6)
				perror("sender socket connection");
			else if (ret == -7)
				perror("connect");
			break;
		
		case FILE_REJECT:
			printf("%s\n", payload);
			break;
			
		case FILE_ACK:
			printf("%s\n", payload);
			break;
	}
}







int main(int argc, char *argv[]) {
	
	if (argc != 3) {
    	fprintf(stderr, "Please provide 2 arguments : <server_name> <server_port>\n");
        exit(EXIT_FAILURE);
    }
	
	int fd = -1;
	struct addrinfo *result;

	initializeConnection(&fd, &result, argv[1], argv[2]);
	
	char *my_nickname = NULL; // stocke le pseudo de l'utilisateur
	int nicknew_count = 0; // compteur utilisé pour distinguer le choix initial du pseudo des changements de pseudos

	// Choix du pseudo avant le début de la boucle principale
	int ret = initialNickname(fd, &my_nickname);
	if (ret == -1) {
		perror("write");
		close(fd);
		free(my_nickname);
		exit(EXIT_FAILURE);
	}

	char *my_channel = NULL; // stocke le nom du salon de discussion
	my_channel = (char *)malloc(CHANNEL_NAME_LEN);
	if (my_channel == NULL) {
    	perror("malloc");
    	close(fd);
    	free(my_nickname);
    	exit(EXIT_FAILURE);
	}
	my_channel[0] = '\0';
	
	int in_channel = 0; // 1 si l'utilisateur est dans un salon, 0 sinon
	int new_channel = 0; // 1 si un canal vient d'être crée, 0 sinon : pour gérer l'affichage des prompt adéquats
	int just_joined = 0; // pour gérer l'affichage des prompts entre le client qui rejoins et les clients notifiés de son arrivée

	
	char *sent_file_path_name = NULL; // Chemin du fichier à envoyer, alloué dynamiquement dans prepare_message
	

	struct pollfd fds[2]; // Un descr. pour l'entrée, l'autre (fd) pour la socket 
	memset(fds, 0, 2*sizeof(struct pollfd));
	fds[0].fd = 0; // stdin
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = fd; // socket
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	
	int waiting_stage = 1; // permet de gérer l'affichage du prompt sur la ligne de commande
	while (1) {
		// Affichage du pseudo et salon de l'utilisateur avant chaque commande
		if (in_channel == 1 && waiting_stage){
			// Cas où le client est à l'origine de la demande de création d'un salon
			if (new_channel){
				printf("%s> ", my_nickname);
				fflush(stdout);
				printf("You have created channel %s\n", my_channel);
				printf("%s[%s]> You have joined %s\n", my_nickname, my_channel, my_channel);  	
				new_channel = 0;
			}
			
			// Cas où le client est à l'origine de la demande de rejoindre un salon
			if (just_joined){
				printf("%s[%s]> INFO> You have joined %s\n", my_nickname, my_channel, my_channel);
				just_joined = 0;	
			}
			
			printf("%s[%s]> ", my_nickname, my_channel);  	
			fflush(stdout);
		}
		else if (waiting_stage) {
			printf("%s> ", my_nickname);
			fflush(stdout);
		}
		
		
		int ret = poll(fds, 2, -1); // Call poll that awaits new events

		if (ret == -1) {
			perror("poll");
		   break;
		}
		
		// Buffer qui contient l'entrée utilisateur saisie sur le clavier (stdin)
		char input_buffer[256];
		
		
		// Le numéro de port sera incrémenté de 1 à chaque création de socket d'écoute pour le transfert de fichier en P2P, afin de distinguer les différents récepteurs entre eux. Cette variable permet l'incrémentation.
		int p2p_counter = 0;
				
		if (fds[0].revents & POLLIN) { // Activité sur stdin
		 	// Lire stdin et envoyer au serveur
		  	if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
 				perror("fgets");
 				break;
			}
		    input_buffer[strlen(input_buffer)-1]='\0'; // retrait du \n	   
		   	/************** Fonctionnalités supplémentaires : l'utilisateur peut connaître son pseudo et le salon auquel il appartient, via les commandes /me et /mychannel, respectivement *******/
			/*                                                                        */		   
		   	if (strcmp(input_buffer, "/me") == 0 || strcmp(input_buffer, "/me ") == 0) {
         		printf("[Server] : You are %s\n", my_nickname);
         		continue;
         	}
         	
         	if (strcmp(input_buffer, "/mychannel") == 0 || strcmp(input_buffer, "/mychannel ") == 0) {
         		if (strlen(my_channel) == 0)
         			printf("[Server] : You are not currently in a channel\n");
         		printf("[Server] : You are in %s channel\n", my_channel);
         		continue;
         	}
    			
         	/***********************************************************************************************/
		   	
		   	struct message msg;
		   	char payload[INFOS_LEN];
		   
		   	// Remplissage msg
		   	int res = prepare_message(&msg, input_buffer, my_nickname, payload, my_channel, in_channel, &sent_file_path_name);
			
			// Cas d'une demande de déconnexion au mauvais format
			if ((strncmp(input_buffer, "/quit", strlen("/quit")) == 0 && strlen(input_buffer) > strlen("/quit") && in_channel == 0) 
         	|| (strncmp(input_buffer, "/quit ", strlen("/quit ")) == 0 && strlen(input_buffer) > strlen("/quit ") && in_channel == 0)) {
         		printf("You are not currently in a channel. To quit the server, please use \"/quit\".\n");
         		continue;
         	}
			
			if (res == -2){
				perror("malloc");
				continue;
			}
			else if (res == -3){
				printf("[Server] : Sending files with yourself as recipient is not allowed. Please specify a different recipient.\n");
				free(sent_file_path_name);
				continue;
			}
			else if (res == -4){
				printf("[Server] : File %s not found. Please check the file name and try again.\n", sent_file_path_name);
				free(sent_file_path_name);
				continue;
			}
			
			if (res == 0) {
				send2steps(fds[1].fd, msg, payload);
				if(msg.type != BROADCAST_SEND && msg.type != MULTICAST_SEND)
					waiting_stage = 0;				
			}
         	
         	// Cas d'une demande de déconnexion.
		   	if ( (strcmp(payload, "/quit") == 0 && in_channel == 0) || (strcmp(payload, "/quit ") == 0 && in_channel == 0) ) {
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
			
			process_server_message(fds[1].fd, msg, input_buffer, payload, my_nickname, my_channel, &in_channel, &just_joined, &new_channel, &waiting_stage, &sent_file_path_name, &p2p_counter, &nicknew_count);
			
			free(payload);
			fds[1].revents = 0;
			waiting_stage = 1;
		} // if activité serveur
      
	} // while (1)
	
	free(my_nickname);
	free(my_channel);
	freeaddrinfo(result);
	close (fds[1].fd);
	printf("Have a nice day :)\n\n");
	return 0;	
} // main

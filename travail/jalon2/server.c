#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>
#include <time.h>

#include "common.h"
#include "msg_struct.h"


// ============================ Fonctions de gestion de la liste chaînée stockant les informations des clients connectés ============================

// Cette structure CLient permet de representer les information d'un client connecté
typedef struct Client {
char pseudo[NICK_LEN]; // Pseudonyme du client
int fd;           // Descripteur de fichier de la socket client
char address[INET_ADDRSTRLEN];  // Adresse IP du client (stockée sous forme de chaîne de caractère lisible)
int port;         // Numéro de port utilisé par le client
char connection_time[30]; // Date de connexion du client
struct Client* next;  // Pointeur vers le client suivant dans la liste
} Client;



// Cette fonction permet de créer une nouvelle structure CLient
Client* create_Client(char* client_pseudo, int client_fd, struct sockaddr_in client_addr, char* client_connection_time) {
	Client* new_Client = (Client*)malloc(sizeof(Client));
	// Contrôle d'erreur
	if (new_Client == NULL)
		return NULL;
		
	// Remplissage avec les informations du client
	strcpy(new_Client->pseudo, client_pseudo);
	new_Client->fd = client_fd;
	inet_ntop(AF_INET, &(client_addr.sin_addr), new_Client->address, INET_ADDRSTRLEN); // conversion de l'adresse IP en une chaîne lisible
	new_Client->port = ntohs(client_addr.sin_port);
	strcpy(new_Client->connection_time, client_connection_time);
	new_Client->next = NULL;
	    
	return new_Client;
}



// Cette fonction permet d'ajouter un Client à la chaîne
void add_Client(Client** head, int client_fd, struct sockaddr_in client_addr, char* client_connection_time) {

	Client* new_Client = create_Client("NotDefinedYet", client_fd, client_addr, client_connection_time);
	// Contrôle d'erreur
	if (new_Client == NULL) {
		perror("Failed to create a new client structure");
		return;
	}
	// Parcours de la liste pour trouver le dernier noeud
	Client* current = *head;
	if (current == NULL){	// si la liste est vide
		*head = new_Client;
	}
	else {
		// Recherche du dernier noeud
		 while (current->next != NULL) {
			current = current->next;
		}
		// Ajout du nouveau CLient
		current->next = new_Client;
	}
	// Le nouveau Client est le dernier élément de la liste
	new_Client->next = NULL;
}
	
	
	
// Cette fonction permet d'enlever un Client de la liste
void remove_Client(Client** head, int client_fd) {	
	Client* previous = NULL;
	Client* current = *head;
		
	// Parcours de la liste pour trouver le client à supprimer
	while ((current != NULL) && (current->fd != client_fd)) { 
		previous = current;
		current = current->next;
	}    
		   
	// Si le Client en question n'existe pas
	if (current == NULL) {
		return;
	}
	    
	// Suppression du Client en question
	if (previous == NULL) // Cas où le client est le premier noeud de la liste
    	*head = current->next;
    else
    	previous->next = current->next;
		            
	// Désallocation de mémoire
    free(current);
	return;
}



// Cette fonction permet de désallouer la liste chaînée
void free_Clients(Client** head) {
	Client* current = *head;
	while (current != NULL) {
		Client* temp = current;
		current = current->next;
		free(temp);
	}
	*head = NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////////


// ============================ Fonctions de gestion au niveau de la socket d'écoute / fonction d'envoi ============================

// Cette fonction permet de gérer la configuration de la socket d'écoute du serveur, le remplissage des informations d'adressage, ainsi que l'association de cette adresse à la socket d'écoute.
// Elle retourne 0 en cas de succès et -1 en cas d'erreur
int handle_bind(int listen_fd, struct sockaddr_in* listening_addr, int server_port) {
	// Configuration de la socket d'écoute
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	
	// Informations d'adressage
	listening_addr->sin_family = AF_INET;
	listening_addr->sin_port = htons(server_port);
	inet_aton("0.0.0.0", &listening_addr->sin_addr);
	
	// Association d'adresse à la socket
	int ret = bind(listen_fd, (struct sockaddr *)listening_addr, sizeof(struct sockaddr_in));
	// Contrôle d'erreur
	if (ret == -1)
		return -1;
	return 0;
}



// Cette fonction permet de gérer l'envoi d'un flag à un client pour le notifier d'un événement, la signification de ce flag étant définie en accord entre le serveur et le cient, et explicitée au cas par cas dans ce programme.
// Elle retourne 0 en cas de succès et -1 en cas d'erreur
int handle_flag_send(int client_socket, char* flag){
	int flag_size = sizeof(char);
	int sent = 0;  
	while (sent!= flag_size) {
		int ret = write(client_socket, flag+sent, flag_size-sent); 
		if (ret == -1)
			return -1;
		sent += ret;
	}
	return 0;
}



// Cette fonction permet de gérer l'ajout d'une nouvelle socket client au tableau des struct pollfd tout en notifiant le client de l'état de sa demande de connexion
// Elle retourne "0" en cas d'ajout réussi, "-2" en cas de surcharge du serveur et "-1" en cas d'erreur.
int add_client_to_poll(struct pollfd* fds, int new_fd){
	// Ajout de la socket client dans un case vide du tableau
	for (int i=1; i < MAX_CONN; i++){
		if( fds[i].fd == 0 ){
			fds[i].fd = new_fd;
			fds[i].events = POLLIN | POLLOUT; // Surveiller pour les événements de read et write
			fds[i].revents = 0;
			// Notification du client que sa demande de connexion a abouti : Envoi du flag "0"
			int ret = handle_flag_send(new_fd, "0");
			// Contrôle d'erreur
			if (ret == -1)
				return -1;
			return 0;
		}
	}
		// Surcharge du serveur : Notification du client que sa demande de connexion n'a pas abouti : Envoi du flag "1"
		int ret = handle_flag_send(new_fd, "1");
		// Contrôle d'erreur
		if (ret == -1)
			return -1;
			
		return -2;		 
}



// Cette fonction peret d'obtenir l'horodatage de connexion d'un nouveau client au format "AAAA/MM/JJ @ h:min"
void get_connection_time(char* formatted_time){
	// Obtention de la date actuelle
	time_t current_time_raw;
	time(&current_time_raw);
	// Conversion du temps actuel en structure struct tm pour personnaliser ensuite l'affichage via ses champs
	struct tm time_info;
	localtime_r(&current_time_raw, &time_info);
	// Conversion de l'horodatage en une chaîne de caractères lisible au format demandé
	strftime(formatted_time, 20, "%Y-%m-%d @ %H:%M", &time_info);
}


//////////////////////////////////////////////////////////////////////////////////////////////


// ============================ Fonctions d'envoi et de réception ============================

// Cette fonction permet de gérer un envoi à un client en deux temps : Un premier message contenant la struct message suivi préremplie d'un second contenant les octets utiles.
// Elle renseigne aussi sur le statut de l'envoi en retournant "0" en cas de succès et "-1" en cas d'erreur. 
int handle_send(int client_socket, struct message response_structure, char* payload) {
    
    // Premier envoi : la struct message
	int to_send = sizeof(response_structure);
	int sent = 0;
	while(sent != to_send){
		int ret = write(client_socket, (char*)&response_structure+sent, to_send-sent);
		// Contrôle d'erreur
		if(ret == -1)
			return -1;
		sent += ret;
	}
									
	//Deuxième envoi : Les octets utiles
	to_send = response_structure.pld_len;	
	sent = 0;
	while(sent != to_send){
		int ret = write(client_socket, payload+sent, to_send-sent);
		if(ret == -1)
			return -1;
		sent += ret;
	}    
    return 0;
} 



// Cette fonction permet de gérer la réception d'envoi par un client en deux temps : La réception de la struct message suivie des octets utiles.
// Elle renseigne aussi sur le statut de la lecture en retournant "0" en cas de succès, "-1" en cas d'erreur de lecture et "-2" en cas d'erreur lors de l'allocation dynamique de mémoire 
int handle_receive(int client_socket, struct message* received_structure, char** payload) {
	
    // Première lecture : la struct message			
	int to_read = sizeof(struct message);
	int received = 0;
	while(received != to_read){
		int ret = read(client_socket, (char*)received_structure+received, to_read-received);
		// Contrôle d'erreur
		if(ret == -1)
			return -1;
		received += ret;
	}
	
	// Il n'y a pas de  payload envoyé par le client pour le type NICKNAME_LIST
	if (received_structure->type == NICKNAME_LIST)
		return 0;
	
	//Deuxième lecture : les octets utiles
	int payload_size = received_structure->pld_len;
	// En se basant sur le taille du payload maintenant connue, on alloue dynamiquement de la mémoire pour le payload
   *payload = (char*)malloc(payload_size+1);
	if (*payload == NULL)
        return -2;
    memset(*payload, 0, payload_size);
  
    (*payload)[payload_size] = '\0';
	received = 0;
	while(received != payload_size){
		int ret = read(client_socket, *payload+received, payload_size-received);
		if(ret == -1){
			free(*payload);
			return -1;
		}
		received += ret;
	}
	return 0;
} 


//////////////////////////////////////////////////////////////////////////////////////////////


// ============================ Fonctions de gestion des demandes utilisateur selon le type du message ============================


// Cette fonction permet de vérifier si un peudo a déjà été choisi par un client. 
// Elle renvoie 1 si le peudo en question existe déjà et 0 s'il n'existe pas.
int pseudo_exists(Client** head, const char* client_pseudo) {
    Client* current = *head;
    
    while (current != NULL) { // Parcours de la liste chaînée
        // Cas où le pseudo est déjà attribué
        if (strcmp(current->pseudo, client_pseudo) == 0) {
            return 1;
        }
        current = current->next;
    }
    
	return 0;
}



// Cette fonction permet de vérifier dans le cas de la demande de définition ou de changement de pseudonyme par un client (avec la commande /nick , type NICKNAME_NEW) que le pseudonyme est valable, auquel cas il sera stocké dans la structure contenant ses informations. 
// Le contrôle d'erreur sur le pseudonyme renseigné est fait en utilisant un flag pour signaler si une erreur est survenue ainsi que sa nature.
// La fonction retoune 0 en cas de succès et -1 en cas d'erreur
int handle_nickname_request(Client** head, char* client_pseudo, int client_socket, struct message response_struct){
	// Cas où le pseudo existe déjà : envoi du flag "1" dans le payload au client
	if (pseudo_exists(head, client_pseudo) == 1) {
		response_struct.pld_len = strlen("1");		
		strcpy(response_struct.infos, "1");
		int ret = handle_send(client_socket, response_struct, "1");
			if (ret == -1)
				return -1;
		return 0;
    } else {
			// Cas où le pseudo est valable : envoi du flag "0" dans le payload au client
    		response_struct.pld_len = strlen("0");		
			strcpy(response_struct.infos, "0");
          	int ret = handle_send(client_socket, response_struct, "0");
          	if (ret == -1)
				return -1;
			// Enregistrement du pseudo après parcours de la liste chaînée
			Client* current = *head;
			int first_pseudo_flag = 0;
			while (current != NULL) {
        		if (current->fd == client_socket) {
        			if (strcmp(current->pseudo, "NotDefinedYet") == 0)
						first_pseudo_flag = 1;
            		strcpy(current->pseudo, client_pseudo);
            		
            		// Affichage de l'information de connexion une fois que le nouveau client a choisi son pseudo
            		if (first_pseudo_flag == 1)
            			printf("User %s connected\n", current->pseudo);
        		}
        		current = current->next;
    		}
    		
			return 0;
	}
}



// Cette fonction permet de gérer la demande d'obtention des informations sur un utilisateur en particulier par un client (avec la commande /whois, type NICKNAME_INFOS), en lui envoyant sa date de connexion, son adresse IP et le numéro de port utilisé dans une chaîne de caractères.
// Si le client dont les informations sont demandées n'existe pas, un message approprié est envoyé.
// La fonction retourne 0 en cas de succès et -1 en cas d'erreur
int handle_info_request(Client** head, const char* target_pseudo, int seeking_client_socket, struct message response_struct) {
	char response[INFOS_LEN];
	memset(response, 0, sizeof(response));
    // Parcours de la liste chaînée pour trouver le client cherché
    Client* current = *head;
    strcpy(response, "[Server] : ");
    while (current != NULL) {
    	// Cas où le client en question est trouvé
        if (strcmp(current->pseudo, target_pseudo) == 0) {
            // Récupération et formattage des information demandées
            strcat(response, current->pseudo);
            strcat(response, " connected since ");
            strcat(response, current->connection_time);
            strcat(response, " with IP address ");
            strcat(response, current->address);
            strcat(response, " and port number ");
            char port_str[16];
            sprintf(port_str, "%d", current->port); // Conversion du numéro de port en une chaîne de caractères
            strcat(response, port_str);
            strcat(response, "\n");
			
			// Envoi au client
			response_struct.pld_len = strlen(response);		
			strcpy(response_struct.infos, response);
			
			int ret = handle_send(seeking_client_socket, response_struct, response);
			if (ret == -1)
				return -1;
			
            return 0;
        }
        
        current = current->next;
    }
    // Cas où le client n'existe pas : Envoi d'une information pertinente
    
    strcpy(response, "[Server] : User ");
    strcat(response, target_pseudo);
    strcat(response, " does not exist\n");
    
	response_struct.pld_len = strlen(response);		
	strcpy(response_struct.infos, response);	
    
    // Envoi au client
	response_struct.pld_len = strlen(response);		
	strcpy(response_struct.infos, response);
	
	int ret = handle_send(seeking_client_socket, response_struct, response);
	if (ret == -1)
		return -1;
	return 0;
}



// Cette fonction permet de gérer la demande d'obtention de la liste des utilisateurs connectés par un client (avec la commande /who, type NICKNAME_LIST), en lui envoyant tous les pseudo dans une chaîne de caractères avec ", " pour délimiteur.
// Le choix d'une unique chaîne de caractère comme structure de données est basé sur la simplicité de l'envoi ainsi que de la séparation des différents pseudo côté client.
// La fonction retourne 0 en cas de succès et -1 en cas d'erreur
int handle_list_request(Client** head, int client_socket, struct message response_struct) {
        // Le buffer contenant la liste des pseudo
	char buffer_list[NICK_LEN*MAX_CONN];
    buffer_list[0] = '\0';

    // Parcours de la liste chaînée des Clients et ajout des pseudo au buffer
	Client* current = *head;
    while (current != NULL) {
	    strcat(buffer_list, current->pseudo);
        if (current->next != NULL){
	       	strcat(buffer_list, ", ");  // Ajout du délimiteur
       	}
       	current = current->next;
	}
	strcat(buffer_list, ".");
	response_struct.pld_len = strlen(buffer_list);
	// Envoi au client
	int ret = handle_send(client_socket, response_struct, buffer_list);
		
	return ret;
}



// Cette fonction permet de gérer la demande d'envoi d'un message en diffusion à tous les autres utilisateurs connectés (avec la commande /msgall, type BROADCAST_SEND).
// La fonction retourne 0 en cas de succès et -1 en cas d'erreur
int handle_broadcast_request(Client** head, int sender_socket, struct message response_struct, char* payload) {
   	
   	// Récupération du pseudo du client à l'origine du broadcast
   	char sender_pseudo[NICK_LEN];
   	strcpy(sender_pseudo, response_struct.nick_sender);
   	
   	// Cette chaîne sera à afficher telle quelle par les clients destinataires
   	// Sa taille est choisie de manière adaptée à la taille maximale des messages avec l'entête "[Nickname] : " pour identifier le client à l'origine de l'envoi
   	char payload_to_send[strlen(sender_pseudo)+5+INFOS_LEN]; 
   	strcpy(payload_to_send, "[");
   	strcat(payload_to_send, sender_pseudo);
   	strcat(payload_to_send, "] : ");
   	strcat(payload_to_send, payload);
   	
   	response_struct.pld_len = strlen(sender_pseudo)+5+strlen(payload);
   	
   	// Envoi du message en broadcast
    Client* current = *head;
    while (current != NULL) {
        // Parcours de la liste chaînée en sautant le client à l'origine de la demande d'envoi
        if (current->fd != sender_socket) {
			// Envoi au client
			int ret = handle_send(current->fd, response_struct, payload_to_send);
			// Indication de l'échec de l'envoi
			if (ret == -1)
				return -1;        
        }
        
        current = current->next;
    }
    
	return 0;
}



// Cette fonction permet de gérer la demande d'envoi d'un message privé d'un utilisateur à un autre, identifié par son pseudo (avec la commande /msg , type UNICAST_SEND), en procédant par la séparation entre le pseudo du destinataire et le message qui lui est destiné avant l'envoi.
// La fonction retourne 0 en cas de succès et -1 en cas d'erreur.  
int handle_PM_request(Client** head, char* received_info, int sender_socket, struct message response_struct) {
	
	char* info_ptr = received_info;

    // Extraction du pseudo du desinataire du payload envoyé par le client à l'origine de la demande d'envoi par la recherche du premier espace dans le payload
    char target_pseudo[NICK_LEN];
    int i = 0;
    while (*info_ptr != ' ') {
        target_pseudo[i] = *info_ptr;
        i+=1;
        info_ptr+=1;
    }
    target_pseudo[i] = '\0'; // Ajout du caractère de fin de chaîne

  	info_ptr += 1; // Déplacement du pointeur pour que la chaîne info_ptr ne contienne que le vrai message à envoyer
	
	// Parcours de la liste chaînée pour trouver le pseudo du destinataire
    Client* current = *head;
	while (current != NULL) {
        if (strcmp(current->pseudo, target_pseudo) == 0) {
        	
        	//Envoi au destinataire du message avec l'entête "[Nickname] : "  qui sera affiché tel quel par le destinataire
			char payload_to_send[strlen(response_struct.nick_sender)+5+INFOS_LEN]; 
  			strcpy(payload_to_send, "[");
   			strcat(payload_to_send, response_struct.nick_sender);
   			strcat(payload_to_send, "] : ");
   			strcat(payload_to_send, info_ptr);
   			
  			response_struct.pld_len = strlen(response_struct.nick_sender)+5+strlen(info_ptr);
			
			int ret = handle_send(current->fd, response_struct, payload_to_send);
			if (ret == -1)
				return -1;
			
			// Notification du client à l'origine de l'envoi que le destinataire existe : Envoi du flag "0"
          	response_struct.pld_len = strlen("0");		
			strcpy(response_struct.infos, "0");
          	ret = handle_send(sender_socket, response_struct, "0");
          	if (ret == -1)
				return -1;
			return 0;
        }
        current = current->next;
    }
    
   	// Cas où le client n'existe pas : Envoi d'un message pertinent au client à l'origine de l'envoi
   	 
   	char response[INFOS_LEN];
   	strcpy(response, "[Server] : User ");
   	strcat(response, target_pseudo);
   	strcat(response, " does not exist\n");
    
	response_struct.pld_len = strlen(response);		
	strcpy(response_struct.infos, response);
	
	int ret = handle_send(sender_socket, response_struct, response);
	if (ret == -1)
		return -1;

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {
	
	if (argc != 2){
		printf("Please use \"%s <Port number>\" to launch the server.\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	// Création de la socket d'écoute
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	// Contrôle d'erreur
	if (listen_fd == -1){
		perror("listening socket");
		exit(EXIT_FAILURE);
	}
	
	// Binding de la socket d'écoute
	struct sockaddr_in listening_addr;
	int ret = handle_bind(listen_fd, &listening_addr, atoi(argv[1]));
	//Contrôle d'erreur
	if (ret == -1){
		perror("bind");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}
	
	// Configuration de la socket en mode écoute
	ret = listen(listen_fd, 10);
	// Contôle d'erreur
	if (ret == -1){
		perror("listen");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}
	
	// Initialisation à 0 du tableau de struct pollfd
	struct pollfd fds[MAX_CONN];
	memset(fds, 0, MAX_CONN*sizeof(struct pollfd)); //mise à zéro des cases de l'array des struct pollfd 
	
	// Ajout de la socket d'écoute dans la première case
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	
	// Pointeur vers la tête de la liste des CLients (premier noeud)
	Client* head = NULL;
	
	printf("Server online - Waiting for a connection\n");
	
	while(1){ 
		// Appel à la fonction poll qui attend indéfiniment une activité au niveau d'une socket du tableau fds
		int nb_active_fds = poll(fds, MAX_CONN, -1);
		// Contrôle d'erreur - Arrêt du serveur en cas de disfonctionnement de poll
		if (nb_active_fds == -1){
			perror("poll");
			break;
		}
		
		
		// Vérification de l'activité au niveau de la socket d'écoute
		if ( (fds[0].revents & POLLIN) == POLLIN ){				
			// Acceptation de la demande de connexion et création d'une socket client associée
			struct sockaddr_in client_addr;
			socklen_t len = sizeof(client_addr);
			int new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &len);
			// Contrôle d'erreur - Passage à l'itération suivante en cas d'erreur pour garder le serveur réactif aux nouvelles demandes de connexion
			if (new_fd == -1){
				perror("accept");
				continue; 
			}
			// Ajout de la socket client dans un case vide du tableau et notification du client de l'état de sa demande de connexion
			int ret = add_client_to_poll(fds, new_fd);
			// Contrôle d'erreur
			if (ret == -1){
				perror("write");
				close(new_fd);
			} else if (ret == -2){
				perror("server overloaded");
				close(new_fd);
			}
			// Obtention de la date de connexion du client au format demandé
			char formatted_time[20];
			get_connection_time(formatted_time);
			// Stockage des informations du nouveau client dans la liste chaînée
			add_Client(&head, new_fd, client_addr, formatted_time);
			//On indique que la demande de connexion a été traitée
			fds[0].revents = 0;
		}

		
		
		// Vérification de l'activité au niveau des sockets clients
		for(int i = 1; i < MAX_CONN; i++){
			
			if (fds[i].fd  != 0){
				// Traitement en cas de données à lire
				if( (fds[i].revents & POLLIN) == POLLIN ) {
					
					// Première lecture : examen du type du message envoyé
					struct message received_structure;
					memset(&received_structure, 0, sizeof(received_structure));
					
					// Variable permettant de stocker le payload
					char* payload_data = NULL;
					
					int ret = handle_receive(fds[i].fd, &received_structure, &payload_data);
					// Contrôle d'erreur
					if(ret == -1){
						perror("read");
					} else if (ret == -2){
						perror("malloc");
					}
					
					enum msg_type request_type = 0;
					request_type = received_structure.type;
					
					// Gestion des demandes de l'utilisateur selon le type du message envoyé
					switch (request_type)
					{
						// Cas d'une demande de définition ou de changement de pseudonyme					
						case NICKNAME_NEW :
							ret = handle_nickname_request(&head, payload_data, fds[i].fd, received_structure);
							// Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
						// Cas d'une demande de la liste des utilisateurs connectés
						case NICKNAME_LIST :
							ret = handle_list_request(&head, fds[i].fd, received_structure);
							//Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
						// Cas d'une demande d'informations sur un utilisateur
						case NICKNAME_INFOS :
							ret = handle_info_request(&head, payload_data, fds[i].fd, received_structure);
							//Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
						// Cas d'une demande d'envoi d'un message en diffusion
						case BROADCAST_SEND :
							ret = handle_broadcast_request(&head, fds[i].fd, received_structure, payload_data);
							//Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
						// Cas d'une demande d'envoi d'un message privé
						case UNICAST_SEND :
							ret = handle_PM_request(&head, payload_data, fds[i].fd, received_structure);			
							//Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
						
						case ECHO_SEND :
							// Cas d'une demande de fermeture de connection
							if (strcmp(payload_data, "/quit") == 0){
								// Affichage de l'information de déconnexion
								Client* current = head;
    							while (current != NULL) {
        							if (current->fd == fds[i].fd) {
        	        				printf("User %s disconnected\n", current->pseudo);
        							}
									current = current->next;
								}
								close(fds[i].fd); 
								fds[i].fd = 0;
								remove_Client(&head, fds[i].fd);
								break;
							}
							
							// Cas d'un echo normal
							
							// Cette chaîne sera à afficher telle quelle par les clients destinataires
							char payload_to_send[received_structure.pld_len+strlen("[Server] : ")]; 
   							strcpy(payload_to_send, "[Server] : ");
   							strcat(payload_to_send, payload_data);
							
							received_structure.pld_len = received_structure.pld_len + strlen("[Server] : ");
							// Renvoi du message
							int ret = handle_send(fds[i].fd, received_structure, payload_to_send);
							//Contrôle d'erreur
							if (ret == -1)
								perror("write");
							break;
					}
					
					free(payload_data);				
					//On indique que l'activité au niveau de la socket a été traitée
					fds[i].revents = 0;
					}
			
					// Si la connection a été fermée sans demande explicite de la part de l'utilisateur, on ferme la socket client et on renseigne sa place dans le tableau comme vide. On enlève également le client de la liste chaînée.
					if( (fds[i].revents & POLLHUP) == POLLHUP ) {
						close(fds[i].fd); 
						fds[i].fd = 0;
					
						remove_Client(&head, fds[i].fd);
						
						// Affichage de l'information de déconnexion
						Client* current = head;
    					while (current != NULL) {
        					if (current->fd == fds[i].fd) {
        	        			printf("User %s disconnected\n", current->pseudo);
        					}
							current = current->next;
						}				
					}
					
				}
				
			}

		}

	// Désallocation de la liste chaînée
	free_Clients(&head);
	// Femeture de la socket du serveur
	close(listen_fd);
	
	printf("Server offline\n");
	
	exit(EXIT_SUCCESS);
}


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

// Structure pour representer les information d'un client connecté, alias "Client"
typedef struct Client {
	int fd;           // Descripteur de fichier de la socket client
	char address[INET_ADDRSTRLEN];  // Adresse IP du client (stockée sous forme de chaîne de caractère lisible)
	int port;         // Numéro de port du client
	struct Client* next;  // Pointeur vers le client suivant dans la liste
} Client;

// Pointeur vers la tête de la liste (premier noeud)
Client* head = NULL;

// Cette fonction permet de créer une nouvelle structure CLient
Client* create_Client(int client_fd, struct sockaddr_in client_addr) {
	Client* new_Client = (Client*)malloc(sizeof(Client));
	// Contrôle d'erreur
	if (new_Client == NULL) {
	   perror("malloc");
	   return NULL;
	}
	// Remplissage avec les informations du client
	new_Client->fd = client_fd;
	inet_ntop(AF_INET, &(client_addr.sin_addr), new_Client->address, INET_ADDRSTRLEN); // conversion de l'adresse IP en une chaîne lisible
	new_Client->port = ntohs(client_addr.sin_port);
   new_Client->next = NULL;
    
	return new_Client;
}

// Cette fonction permet d'ajouter un Client à la chaîne
void add_Client(Client** head, int client_fd, struct sockaddr_in client_addr) {
	Client* new_Client = create_Client(client_fd, client_addr);
	// Contrôle d'erreur
	if (new_Client == NULL) {
		perror("Failed to create a new client structure");
	   return;
	}
	
	// Parcours de la liste pour trouver le dernier noeud
	Client* current = *head;
	if (current == NULL)	// si la liste est vide
	    *head = new_Client;
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
void free_Clients(struct Client** head) {
	Client* current = *head;
	while (current != NULL) {
		Client* temp = current;
	   current = current->next;
	   free(temp);
	}
	
	*head = NULL;
}


int main(int argc, char *argv[]) {
	
	if (argc != 2) {
        fprintf(stderr, "Merci de rentrer un seul argument : <server_port>\n");
        exit(EXIT_FAILURE);
   }
   
	// Socket creation
	int listen_fd=socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd == -1) {
		perror("socket");
		close(listen_fd);		
		exit(EXIT_FAILURE);
	}
	
	setsockopt(listen_fd, SOL_SOCKET,SO_REUSEADDR, &(int){1}, sizeof(int));
	
	// Fill in server address
	struct sockaddr_in listening_addr;
	listening_addr.sin_family = AF_INET;
	listening_addr.sin_port = htons(atoi(argv[1])); //htons permet conversion du port (format machine ou host) au format réseau (network), en format short (s, 2 octets). Il y a en effet une différence dans l'ordre considéré par nous et le réseau lors de la lecture de données.
	inet_aton("0.0.0.0", &listening_addr.sin_addr); //indique au server d'écouter sur toutes les interfaces dispo

	// Bind to the address 
	int ret = bind(listen_fd, (struct sockaddr *)&listening_addr, sizeof(listening_addr));
	if (ret == -1) {
		perror("bind"); 
		close(listen_fd);
		exit(EXIT_FAILURE);
	}
	
	// Set up socket in listening mode
	ret = listen(listen_fd,10);
	if (ret == -1) {
		perror("listen");
		close(listen_fd); 
		exit(EXIT_FAILURE);
	}
	
	// Initiate struct pollfd array
	int max_conn = 256;
	struct pollfd fds[max_conn];
	memset(fds, 0, max_conn*sizeof(struct pollfd)); 
	
	
	// Insert listen_fd into array
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN; // évènement à monitorer : données disponible à la lecture 
	fds[0].revents = 0;
	
	
	while (1) {
		// Call poll that awaits new events
		int active_fds = poll(fds, max_conn, -1);
		if (active_fds == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
      }
		
		// Check activity on each struct pollfd
		for(int i = 0; i < max_conn; i++) {
			
			if (fds[i].fd == listen_fd && (fds[i].revents & POLLIN)) {
			// If activity on listen_fd, accept new connection, add new fd to array
				struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);
				int new_fd = accept(listen_fd,(struct sockaddr *)&client_addr, &len);
				if (new_fd == -1) {
					perror("accept"); 
					continue; 
				}
			
				else {
				// on parcourt fds pour y trouver un emplacement dispo pour new_fd 
					for (int j=0 ; j<max_conn ; j++) {
						if ( fds[j].fd == 0 ) {
							fds[j].fd = new_fd;
							fds[j].events = POLLIN; //surveille si data dispo en lecture
							fds[j].revents = 0;
							break; //on sort du for dès qu'on a trouvé une place
						}
						
						// Surcharge du serveur
						if ((i == max_conn) && (fds[i].fd != 0)) {
							printf("Unable to connect : too many connections\n");
							close(new_fd);
						}
					}
					
					// Stockage des informations du nouveau client
					add_Client(&head, fds[i].fd, client_addr);
			
					printf("New client connected\n");
					//On indique que la demande de connexion a été traitée
					fds[0].revents = 0;
					
				}

			}
						
			else if (fds[i].revents & POLLHUP) { 
			//cas où le client ferme la connexion
				close (fds[i].fd);
				fds[i].fd = 0;

				remove_Client(&head, fds[i].fd);
				printf("Client %i disconnected\n", i);
			}
			
			else if ( fds[i].revents & POLLIN ) {
			// If activity on other fd, read and print msg
				int msg_size = 0;
				int to_read = sizeof(msg_size);
				int received = 0;
				
				while (to_read > received) {

					int ret = read(fds[i].fd, (char *)&msg_size+received, to_read-received);				
					
					if (ret == -1) {
						perror("read");
						close(listen_fd);
						free_Clients(&head);
						exit(EXIT_FAILURE); 
					}
					
					received += ret;   
				}
				
				received = 0;
				char buf[msg_size+1]; // +1 prise en compte du char EOF
				buf[msg_size]='\0';
								
				while (to_read > received) {

					int ret = read(fds[i].fd, (char *)buf+received, msg_size-received);				
					//le cast en char* permet de se déplacer octet par octet
					if (ret == -1) {
						perror("read"); 
						close(listen_fd);
						free_Clients(&head);
						exit(EXIT_FAILURE); 
					}
					
					received += ret;   
				}
				
				if (strcmp(buf, "/quit\n") == 0) {
        		// Message "/quit" reçu, fermeture du serveur
				  close(listen_fd);
				  free_Clients(&head);
				  printf("Server terminated\n");
				  exit(EXIT_SUCCESS);
    			}
				
				printf("New Msg : %s\n", buf);
				
				// Renvoi du message au client
				int sent = 0;
            int to_send = msg_size;
            
    			while (to_send > sent) {

        			int ret = write(fds[i].fd, buf + sent, to_send - sent);

					if (ret == -1) {
		            perror("write");
		            free_Clients(&head);
		            close (listen_fd);
		            exit(EXIT_FAILURE);
		       	}

        			sent += ret;
    			}
    		
    			printf("Sent Msg: %s\n", buf);
			
			}	
		}
			
	}		
	// Désallocation de la liste chaînée
	free_Clients(&head);
	// Femeture de la socket du serveur
	close(listen_fd);
	
	printf("Server terminated\n");
			

	return 0;
}

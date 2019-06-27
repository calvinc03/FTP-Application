#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h> 
#include <limits.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include "server.h"
#include "netbuffer.h"
#include "dir.h"
#include "usage.h"

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.

bool loggedin = false;
bool type = false; // false represents ascii and true represents image
bool streamMode = false;
bool fileStruc = false;
bool passMode = false;
char currDir[1024];
char initialDir[1024];
int pasvPortNum, pasvSocket;

struct sockaddr_in address;
int addrlen = sizeof(address);

struct sockaddr_in svrAddr;
int svrAddrlen = sizeof(svrAddr);

struct sockaddr_in dataAddr;
int dataAddrlen = sizeof(dataAddr);

void handle_client(int fd);
int handle_command(int fd, char input[]);
void handle_retr(int fd, char * file);


int main(int argc, char *argv[]) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc

    int i;
    
    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor 
    // returned for the ftp server's data connection

    run_server(argv[1], handle_client);
    
    printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;
}

void handle_client(int fd) {
	net_buffer_t recvBuf = nb_create(fd, 1024);
	char buf[1025];
	
	send_string(fd, "220 Welcome to Assignment 3 :)\r\n");
	send_string(fd, "Log in with USER cs317 \r\n");
	
	int acc = nb_read_line(recvBuf, buf);
	getcwd(initialDir, sizeof(initialDir));
	while(acc > 0) {
		if (handle_command(fd, buf))
			break;
		nb_read_line(recvBuf, buf);
	}
	send_string(fd, "Goodbye.\r\n");
	nb_destroy(recvBuf);
}

int handle_command(int fd, char input[]) {
	// tokenize the input
	//how to identify the length of the input
	char * token = strtok(input, " \t\r\n");

    // Parse USER command
    if (!strcasecmp(token, "USER")) {
		token = strtok(NULL, " \t\r\n");
		if (strtok(NULL, " \t\r\n") != NULL) {
			send_string(fd, "501 Invalid number of arguments");
			return 0;
		}
        if (!loggedin) {
			// Compare to see if username matches "cs317"
			if (!strcasecmp(token, "cs317")) {
				loggedin = true;
				send_string(fd, "230 User logged in\n");
			}
			// username is not cs317
			else {
				send_string(fd, "530 This server only supports the username: cs317\n");
			}
		}
			// If already logged in
		else {
				send_string(fd, "530 Already logged in.\n");
		}
		return 0;
    }
	// Parse QUIT command
    if (!strcasecmp(token, "QUIT")) {
		if (strtok(NULL, " \t\r\n") != NULL) {
			send_string(fd, "501 Invalid number of arguments");
			return 0;
		}
		close(pasvSocket);
		send_string(fd, "221 Closing connection.\n");
		return 1;
	}
    if (!loggedin) {
      	send_string(fd, "530 User is not logged in\n");
	  	return 0;
    }
    else {
      	// Parse TYPE command
        if (!strcasecmp(token, "TYPE")) {
			token = strtok(NULL, " \t\r\n");
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (!strcasecmp(token, "I")) {
				if (!type) {
					type = true;
					send_string(fd, "200 Setting TYPE to Image\n"); 
				}
				else {
					send_string(fd, "200 TYPE is already Image.\n");
				}
			}
			else if (!strcasecmp(token, "A")) {	
				if (type) {
					type = false;
					send_string(fd, "200 Setting TYPE to ASCII.\n");
				}
				else {
					send_string(fd, "200 TYPE is already ASCII.\n"); 
				}		
			}
			else {
				send_string(fd, "504 This server does not support this TYPE\n");
			}
			return 0;
		}
    
    	// Parse MODE Command
    	else if (!strcasecmp(token, "MODE")) {
			token = strtok(NULL, " \t\r\n");
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (!strcasecmp(token, "S")) {
				if (!streamMode) {
					streamMode = true;
					send_string(fd, "200 Entering Stream mode.\n"); 
				}
				else {
					send_string(fd, "200 Already in Stream mode.\n");
          		}
			}
			else {
				send_string(fd, "504 This server does not support this MODE\n");
			}
			return 0;
		}

    	// Parse STRU Command
    	else if (!strcasecmp(token, "STRU")) {
			token = strtok(NULL, " \t\r\n");
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (!strcasecmp(token, "F")) {
				if (!fileStruc) {
					fileStruc = true;
					send_string(fd, "200 Data Structure set to File Structure.\n");
        		}
				else {
					send_string(fd, "200 Data Structure is already set to File Structure.\n");
			  	}
      		}
			else {
				send_string(fd, "504 This server does not support this STRU\n");
			}
			return 0;
		}

		// Parse CWD Command
		else if (!strcasecmp(token, "CWD")) {
			token = strtok(NULL, "");
			if (strstr(initialDir, token) != NULL) {
				send_string(fd, "500 cannot go above current directory");
				return 0;
			}
			if (strstr(token, ".") != NULL || strstr(token, "..") != NULL) {
				if (chdir(token) != 0) {
					send_string(fd, "501 No such directory");
				}
				else {
					send_string(fd, "250 Directory successfully changed");
					getcwd(currDir, sizeof(currDir));
				}
			} 
			else {
				send_string(fd, "500 Syntax error in directory \n");
			}
			return 0;
		}
		
		// Parse CDUP Commnad
		else if (!strcasecmp(token, "CDUP")) {
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			getcwd(currDir, sizeof(currDir));
			if (currDir == initialDir) {
				send_string(fd, "500 Cannot go above current directory");
				return 0;
			}
			if (chdir("..") != 0) {
				send_string(fd, "501 No such directory");
			}
			else {
				send_string(fd, "250 Directory successfully changed");
				getcwd(currDir, sizeof(currDir));
			}
			return 0;
		}

    	// Parse PASV Command
    	else if (!strcasecmp(token, "PASV")) {
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (!passMode) {

				getsockname(fd, (struct sockaddr*)&svrAddr, &svrAddrlen);

				while (true) {
					// Choose an arbitrary port number
					pasvPortNum = (rand() % 64512) + 1024;
					
					// Create new socket
					pasvSocket = socket(AF_INET, SOCK_STREAM, 0);
					if (pasvSocket == -1) {
						send_string(fd, "500 Error. Cannot enter Passive Mode.\n");
						return 0;
					}
				
					address.sin_family = AF_INET;
					address.sin_addr.s_addr = INADDR_ANY;
					address.sin_port = htons(pasvPortNum);

					if (bind(pasvSocket,(struct sockaddr *)&address, sizeof(address)) < 0) {
						break;
					}
				}

				listen(pasvSocket, 10);
				passMode = true;
				
				uint32_t addr = svrAddr.sin_addr.s_addr;
					
				send_string(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n", addr & 0xff, 
							(addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff, 
							pasvPortNum >> 8, pasvPortNum & 0xff);
			}
			
			else {
				send_string(fd, "227 Already in passive mode. Port number: %d\n", pasvPortNum);
			}
			return 0;
		}

		// Parse NLST Command
		else if (!strcasecmp(token, "NLST")) {
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (passMode) {

				listen(pasvSocket, 10);
				int newSocket = accept(pasvSocket, (struct sockaddr *)&dataAddr, (socklen_t*) &dataAddrlen);
				if (newSocket < 0) {
					send_string(fd, "500 Cannot open data connection\n");
				}
				send_string(fd, "150 Here comes the directory listing.\n");
						
				getcwd(currDir, sizeof(currDir));
				listFiles(newSocket, currDir);
				send_string(fd, "226 Closing data connection. Transfer successful.\n");
						
				close(newSocket);
				close(pasvSocket);
				passMode = false;
			}
			else {
				send_string(fd, "425 Not in Passive Mode\n");
			}
			return 0;
		}

		// Parse RETR Command
		else if (!strcasecmp(token, "RETR")) {
			token = strtok(NULL, " \t\r\n");
			if (strtok(NULL, " \t\r\n") != NULL) {
				send_string(fd, "501 Invalid number of arguments");
				return 0;
			}
			if (passMode) {
				
				listen(pasvSocket, 10);
				int newSocket = accept(pasvSocket, (struct sockaddr *)&dataAddr, (socklen_t*) &dataAddrlen);
				if (newSocket < 0) {
					send_string(fd, "500 Cannot open data connection\n");
				}

				send_string(fd, "150 File status okay; about to open data connection.\n");
				handle_retr(newSocket, token);
	
				close(newSocket);
				close(pasvSocket);
				passMode = false;		
			}
			else {
				send_string(fd, "425 Not in Passive Mode\n");
			}
			return 0;
		}

    	// Unsupported Commands
    	else {
			send_string(fd, "500 This server does not support this command.\n");
			return 0;
		}
  	}
}

// Helper to handle the retrieval of a file
void handle_retr(int sock, char * filename) {
	FILE * file = fopen(filename, "rb");
	char buf[1025];
	bool valid = true;
	if (file != NULL) {
		fseek(file, 0, SEEK_SET); 
		fread(buf, 1, 1024, file);
		if (send(sock, buf, 1024, 0) == -1) {
			valid = false;
		}
	} 
	if (file == NULL || !valid || fclose(file) != 0) {
		send_string(sock, "451 File Not Found\n");
		return;
	}
	send_string(sock, "226 Closing data connection. Transfer Successful\n");
	return;
}

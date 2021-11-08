#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <cctype>

using namespace std;

int getServerAdd (char* argv[], struct sockaddr_in* addr);
int sendHelp(int sock, char* buffer, unsigned long numB);
int recvHelp(int sock, char* buffer, unsigned long numB);
void sendName (int sock);
void sendGrade (int sock, double grade);
int recvWordLen(int sock);
void recvLeaderBoard (int sock);
void sendGuess (int sock, char guess);
void modifyWord (char* w, char* indexChar, int arrS, char guess);
void recvResult (int sock, char* w, char guess, int& restIndex);

const unsigned int NAME_SIZE = 1500;
const unsigned int WORD_SIZE = 1500;
const unsigned int GUESS_TIMES = 1500;

int main (int argc, char* argv[]) 
{
  	if (argc < 3 || argc > 3) {
    	cout << "Usage: " << argv[0] << " server_host server_port\n";
    	return 1;
  	}	

  	struct sockaddr_in serverAdd;
  	if (getServerAdd (argv, &serverAdd) < 0) {
    	cout << "ERROR: failed to look up DNS for " << argv[1] << ":" << argv[2] << endl;
    	return 1;
  	}
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
  	if (sock == -1) {
    	perror("socket");
    	return 1;
  	}
  	
  	if (connect(sock, (struct sockaddr*) &serverAdd, sizeof(serverAdd)) < 0) {
    	perror("connect");
    	return 1;
  	}
  
  	cout << "Successfully connected with the server...\n";
  	// send Name first
  	sendName (sock);
  	
  	// receive the length of the word
  	int wordL = recvWordLen(sock);
  	int indexToGuess = wordL;
  	if (wordL == -1) 
  		exit (1);
  	
  	char* word = new char [wordL + 1];
  	word[wordL] = '\0';
  	// start game
  	int turn = 0;
  	string guessInput;
  	char guess;
  	
  	while (indexToGuess > 0 && turn < GUESS_TIMES) {
  		turn++;
  		cout << "Turn " << turn << endl;
  		cout << "Word: " << word << endl;
  		if (turn == 1) 
  			memset (word, '-', wordL);
  		
		cout << "Enter your guess (1 character only): ";
  		getline (cin, guessInput);
  		while (guessInput.length() > 1) {
  			cout << "Please enter your guess again (1 character only): ";
  			getline (cin, guessInput);
		}
  		
  		guess = toupper(guessInput[0]);
  		
  		// send guess
  		sendGuess (sock, guess);
  		
  		// recv result of guess as an array
  		// if 1 then correct -> modifyWord. Else nothing
  		recvResult (sock, word, guess, indexToGuess);	
		
	}
	
	if (turn >= GUESS_TIMES && indexToGuess > 0) {
		cout << "\nYou reach the limit of how many guesses you can make!\n";
		cout << "You failed to guess the word\n";
		return 1;
	}
	// send grades to the server
	sendGrade (sock, (double)turn/wordL);
	
	cout << "Congratualtions! You guessed the word " << word << endl; // the word 
	cout << "It took " << turn << " turns to guess the word correctly.\n\n";
  
  	// show leaderboard
  	cout << "Leader Board:\n";
  	
  	cout << "Closing connection...\n";
	close (sock);
    delete word;
    return 0;

}

int recvWordLen(int sock) // without the \n
{
	int wordL;
	
	if (recvHelp(sock, (char*) &wordL, sizeof(wordL)) < 0) {
	    cout << "ERROR: failed to receive word's length\n";
	    return -1;
	}
	wordL = ntohl(wordL);
	
	if (wordL <= 0) {
      cout << "ERROR: received invalid word's length value " << wordL << " from the server\n";
      errno = EINVAL;
      return -1;
  	}
  	
  	return wordL;

}


void modifyWord (char* w, char* indexChar, int arrS, char guess)
{
	int w_i = 0;
	for (int i = 0; i < arrS; i++) {
		w_i = indexChar[i] - '0';
		w[w_i] = guess;
	}
		
}

void sendGuess (int sock, char guess) 
{
    char send_buffer [1] = {guess};
    int sendLen = sendHelp(sock, send_buffer, 1);
    if (sendLen < 0) {
      cout << "ERROR: failed to send guess\n";
      exit(1);
    }
}

void recvResult (int sock, char* w, char guess, int& restIndex)
{
	// will receive an array 
	// first index indicates whether the guess is correct (1 if correct, 0 is non)
	// if correct, the next consecutive indexes will be the indexes of where the correct guess should be
	// recv the length of array first
	int arrS = 0;
	
	if (recvHelp(sock, (char*) &arrS, sizeof(arrS)) < 0) {
	    cout << "ERROR: failed to receive the guess response array's length\n";
	    exit(1);
	}
	arrS = ntohl(arrS);
	
	if (arrS <= 0) {
      cout << "ERROR: received invalid array length of the guess response array " << arrS << " from the server\n";
      errno = EINVAL;
      exit(1);
  	}
  	
  	// recv the array
  	char* arr = new char [arrS];
  	if (recvHelp(sock, arr, arrS) < 0) {
      cout << "ERROR: failed to receive the guess response array\n";
      delete arr;
      exit(1);
  	}
  	
  	int i_guessRsp = 0;
  	if (arr[0] == '1') {
  		restIndex -= (arrS - 1);
  		char* guessResponse = new char [arrS - 1];
  		for (int i = 1; i < arrS; i++) {
			arr[i_guessRsp] = guessResponse[i];
			i_guessRsp++;
		}
  			
  	modifyWord (w, guessResponse, arrS - 1, guess);
  	delete guessResponse;
	}

  	delete arr;
}


void sendGrade (int sock, double grade)
{
	// convert grade to string
	string gr = to_string(grade);
	
	// send length first
	int len = htonl((int)gr.length()); // not including null-terminated
    int sendLen = sendHelp(sock, (char*) &len, sizeof(len));
    if (sendLen < 0) {
      cout << "ERROR: failed to send grade string length\n";
      exit(1);
    }
    // send grade
    char* gradeStr = new char [gr.length()];
    strncpy(gradeStr, gr.c_str(), gr.length());
    sendLen = sendHelp(sock, gradeStr, (int) gr.length()); 
    if (sendLen < 0) {
      cout << "ERROR: failed to send grade string\n";
      delete gradeStr;
      exit(1);
    }
	delete gradeStr;
}

void recvLeaderBoard (int sock)
{
	int boardLen = 0;
	if (recvHelp(sock, (char*) &boardLen, sizeof(boardLen)) < 0) {
	    cout << "ERROR: failed to receive the leader board's length\n";
	    exit(1);
	}
	boardLen = ntohl(boardLen);
	
	if (boardLen <= 0) {
      cout << "ERROR: received invalid length of leader board " << arrS << " from the server\n";
      errno = EINVAL;
      exit(1);
  	}
  	
  	// recv the array
  	char* boardName = new char [boardLen];
  	double* boardGrade = new char [boardLen];
  	int counter = 0;
  	while (counter != boardLen) {
  		counter++;
  		// recv name of the first, the second, third (or less)
  		int nameLen = 0;
		if (recvHelp(sock, (char*) &nameLen, sizeof(nameLen)) < 0) {
	    	cout << "ERROR: failed to receive the length of client's name who is in rank " << counter << "\n";
	    	exit(1);
		}
		nameLen = ntohl(nameLen);
		
		if (counter <= 0) {
	      cout << "ERROR: received invalid length of client's name who is in rank " << counter << " from the server\n";
	      errno = EINVAL;
	      exit(1);
	  	}
	  	
	  	char* name = new char [nameLen + 1];
	  	if (recvHelp(sock, name, nameLen) < 0) {
	      cout << "ERROR: failed to receive the guess response array\n";
	      delete name;
	      exit(1);
	  	}
	  	name[nameLen] = '\0';
	  	
	  	// receive grade 
	  	int gradeL = 0;
		if (recvHelp(sock, (char*) &gradeL, sizeof(gradeL)) < 0) {
		    cout << "ERROR: failed to receive grade string length of client in rank " << counter << "\n";
		    exit(1);
		}
		gradeL = ntohl(gradeL);
		
		if (gradeL <= 0) {
	      cout << "ERROR: received invalid grade string length value " <<  gradeL << " of client in rank " << counter << " from the server\n";
	      errno = EINVAL;
	      exit(1);
	  	}
	    
	    // recv the grade as string
	  	char* gr = new char [gradeL + 1];
	  	if (recvHelp(sock, gr, gradeL) < 0) {
	      cout << "ERROR: failed to receive the grade of client in rank " << counter << "\n";
	      delete gr;
	      exit(1);
	  	}
	  	gr[gradeL] = '\0'; 
	  	
	  	// convert string to double
	  	double grade = atof (gr);
	  	
		// print the record right here
		cout << setw(6) << counter << ". " << name << setw (4) << grade << endl;
		  	
  		delete gr;
  		delete name;
	}

}

int getServerAdd (char* argv[], struct sockaddr_in* addr)
{
  if (argv[1] == NULL || argv[2] == NULL || addr == NULL) {
    cout << "ERROR: argument is NULL\n";
    exit(1);
  }

  // check passed-in port number first
  char* endptr = NULL;

  unsigned long portN = strtoul(argv[2], &endptr, 10);
  if (endptr != NULL && *endptr != '\0') {
    cout << "ERROR: " << argv[2] << " is not a number\n";
    exit(1);
  }
  else if (portN == ULONG_MAX && errno == ERANGE) {
    cout << "ERROR: " << strerror(ERANGE)<< ": " <<  argv[1] << endl;
    exit(1);
  }
  else if (portN > USHRT_MAX || portN < 0) {
    cout << "ERROR: " << strerror(ERANGE) << ": " << portN << " is not a valid port number\n";
    exit(1);
  }

  // check the IP
  struct addrinfo* addrL = NULL;
  int internetAddr = getaddrinfo(argv[1], argv[2], NULL, &addrL);
  if (internetAddr != 0) {
    cout << "getaddrinfo: " << gai_strerror(internetAddr) << endl;
    exit(1);
  }

  struct addrinfo* addr_temp = addrL;
  while (addr_temp != NULL && addr_temp->ai_family != AF_INET) {
    addr_temp = addr_temp->ai_next;
  }

  if (addr_temp == NULL) {
    cout << "ERROR: Address not found\n";
    freeaddrinfo(addrL);
    exit(1);
  }

  assert(addr_temp->ai_addrlen == sizeof(struct sockaddr_in));
  memcpy(addr, addr_temp->ai_addr, sizeof(*addr));
  freeaddrinfo(addrL);
  
  return internetAddr;
}

int sendHelp(int sock, char* buffer, unsigned long numB) {
  int sendLen = 0;

  while (numB > 0) {
    sendLen = send(sock, buffer, numB, 0);
    if (sendLen < 0) {
      perror("send");
      return -1;
    }
    else if (sendLen == 0) {
      cout << "ERROR: client closed the connection unexpectedly\n";
      return -1;
    }

    assert(sendLen > 0);
    buffer += sendLen;
    numB -= sendLen;
    assert(numB >= 0);
  }
  return 0;
}

int recvHelp(int sock, char* buffer, unsigned long numB) {
  int recvLen;
  
  while (numB > 0) {
    recvLen = recv(sock, buffer, numB, 0);
    if (recvLen < 0) {
      perror("recv");
      return -1;
    }
    else if (recvLen == 0) {
      cout << "ERROR: client closed the connection unexpectedly\n";
      return -1;
    }

    assert(recvLen > 0);
    buffer += recvLen;
    numB -= recvLen;
    assert(numB >= 0);
  }
  return 0;
}

void sendName (int sock)
{
	char* name;
    unsigned long len = 0;
    cout << "Enter your name: ";
    
    ssize_t readLen = getline(&name, &len, stdin);
    if (readLen == -1) {
      perror("getline");
      exit(1);
    }
    if (readLen > NAME_SIZE) {
      cout << "ERROR: message exceeds maximum length\n";
      exit(1);  
	}
	
    if (name[readLen - 1] == '\n') 
      name[readLen  - 1] = '\0';  
  
  	// send the length first 
    int nameLen = htonl(readLen - 1);
    int sendLen = sendHelp(sock, (char*) &nameLen, sizeof(nameLen));
    if (sendLen < 0) {
      cout << "ERROR: failed to send string length\n";
      exit(1);
    }
    // send name
    sendLen = sendHelp(sock, name, readLen - 1);
    if (sendLen < 0) {
      cout << "ERROR: failed to send the string\n";
      exit(1);
    }
    
}


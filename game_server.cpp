#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <climits>
#include <string.h>
#include <unistd.h>
using namespace std;

int getPortNum (char* argv[]);
void getWord (string& w);
void recvClientName (int sock, string &cName);
int recvHelp(int sock, char* buffer, unsigned long numB);
char recvGuess (int sock);
void sendGuessResult (int sock, char* guessResp, int arrS);
double recvGrade (int sock);
void sendWordLen (int sock, int wordL);

const unsigned int NAME_SIZE = 1500;
const unsigned int STR_SIZE = 1500;
const unsigned int BOARD_SIZE = 2000;

int i_board = 0;
string nameBoard [BOARD_SIZE];
double scoreBoard [BOARD_SIZE];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *handleClient (void *arg)
{
  int sockNum = *((int *)arg);
  
  string word;
  string clientName;
  
  ///// mutex lock
  pthread_mutex_lock(&lock); 
  //choose a word
  getWord (word); 
    
  // get client's name
  recvClientName (sockNum, clientName); 
	
  nameBoard[i_board - 1] = clientName;
  
  pthread_mutex_unlock(&lock);
	
	cout << "Client's name: " << clientName << " with word " << word << endl;
	
	// send the word's length
	sendWordLen (sockNum, (int)word.length());
	
	int indexToGuess = word.length();
	char guess;
	int correctInd = 0;
	char* guessResponse;
	int i_guessResp = 1;
	double grade;
	
	while (indexToGuess > 0) {
		// recv guess
		guess = recvGuess (sockNum);
		
		// check guess exists in word?
		for (int i = 0; i < word.length(); i++) {
			if (word[i] == guess)
				correctInd++;	
		}
		
		if (correctIndex == 0) {
			guessResponse = new char[1];
			guessResponse[0] = '0';
		}
		else {
			guessResponse = new char[1 + correctInd];
			guessResponse[0] = '1';
			for (int i = 0; i < word.length(); i++) {
				if (word[i] == guess) {
					guessResponse[i_guessResp] = '0' + i;
					i_guessResp++;
				}	
			}
		}
		indexToGuess -= correctInd;
		// send result back
		sendGuessResult (sockNum, guessResponse, correctInd + 1);
		i_guessResp = 1;	
		delete guessReponse;
	}

	pthread_mutex_lock(&lock);
  	scoreBoard [i_board - 1] = recvGrade (sockNum);
  
  	pthread_mutex_unlock(&lock);
	//////// unlock

  	// send the leaderBoard
  	

  close(sockNum);
  pthread_exit(NULL);
  return (void *) 0;
}

int main (int argc, char* argv[])
{
  if (argc < 2 || argc > 2) {
    cout << "Usage: " << argv[0] << " port_number\n";
    return 1;
  }
  // get the port number
  unsigned short listenPortN = getPortNum(argv);
  // create listening socket
  int listenSock = socket (AF_INET, SOCK_STREAM, 0);
  if (listenSock == -1) {
    perror ("socket");
    exit(1);
  }
  // listening socket will listen for what portNumb?
  struct sockaddr_in listenSockAdd;
  memset(&listenSockAdd, 0, sizeof(listenSockAdd));
  listenSockAdd.sin_family = AF_INET;
  listenSockAdd.sin_port = htons(listenPortN);
  listenSockAdd.sin_addr.s_addr = htonl(INADDR_ANY);
  
  // bind socket and the address
  if (bind(listenSock, (struct sockaddr*) &listenSockAdd, sizeof(listenSockAdd)) < 0) {
    perror("bind");
    exit(1);
  }

  // start listening for connection
  if (listen(listenSock, 5) < 0) {
    perror("listen");
    exit(1);
  }
  // after this --> start listening for incoming connection
  cout << "Accepting connection...\n";
  
  while (1) {
    int clientSock = accept (listenSock, NULL, NULL);
    if (clientSock == -1) {
      perror("accept");
    }
    else {
      pthread_t p1;
      int rc = pthread_create(&p1, NULL, handleClient, &clientSock);
      if (rc < 0) {
        cout << "ERROR: Failed to create thread..." << endl;
        close (clientSock);
      }
      
      rc = pthread_detach (p1);
      if (rc != 0) {
        cout << "ERROR: Failed to detach thread" << endl;
      }
      
    }
 
  }
  
  close (listenSock);
  return 0;
}

void sendLeaderBoard (int sock)
{
	int boardLen = 3;
	// send length of leaderBoard
	if (i_board < 3)
		boardLen = i_board;
	
	int boardL = htonl(count); // not including null-terminated
    int sendLen = sendHelp(sock, (char*) &boardL, sizeof(boardL));
    if (sendLen < 0) {
      cout << "ERROR: failed to send the length of leader board\n";
      exit(1);
    }
    
    // sort leader board first
    sortLeaderBoard();
    
    // send the record
	int count = 0;
	string nameStr;
	int sendLen;
	while (count != boardLen) {
		count++;
		// send name first
		nameStr = nameBoard[count - 1];
		int nameL = htonl(nameStr.length()); // not including null-terminated
	    sendLen = sendHelp(sock, (char*) &nameL, sizeof(nameL));
	    if (sendLen < 0) {
	      cout << "ERROR: failed to send the length of the client in rank " << count << "\n";
	      exit(1);
	    }

		char* name = new char [nameStr.length()];
		strncp (name, nameBoard[count - 1].c_str(), nameStr.length()));
	    sendLen = sendHelp(sock, name, nameStr.length()); 
	    if (sendLen < 0) {
	      cout << "ERROR: failed to send name of client in rank " << count << "\n";
	      delete name;
	      exit(1);
	    }
		delete name;
		
		// send grade 
		// convert grade to string
		string gr = to_string(scoreBoard[count - 1]);
		
		int len = htonl((int)gr.length()); // not including null-terminated
	    int sendLen = sendHelp(sock, (char*) &len, sizeof(len));
	    if (sendLen < 0) {
	      cout << "ERROR: failed to send grade string length of client in rank " << count << "\n";
	      exit(1);
	    }
	
	    char* gradeStr = new char [gr.length()];
	    strncpy(gradeStr, gr.c_str(), gr.length());
	    sendLen = sendHelp(sock, gradeStr, (int) gr.length()); 
	    if (sendLen < 0) {
	      cout << "ERROR: failed to send grade string of client in rank " << count << "\n";
	      delete gradeStr;
	      exit(1);
	    }
		delete gradeStr;
	}			
}

void sortLeaderBoard () // use bubble sort
{
	double tempScore;
	string tempName;
    for (int i = 0; i < i_board - 1; i++) {
   		for (int j = 0; j < i_board - i - 1; j++)  {
        	if (scoreBoard[j] > scoreBoard[j + 1])  {
            	tempScore = scoreBoard[j];
            	scoreBoard[j] = scoreBoard[j + 1];
            	scoreBoard[j + 1] = tempScore;
            	tempName = nameBoard[j];
            	nameBoard[j] = nameBoard[j + 1];
            	nameBoard[j + 1] = tempName;
			}
		}
    }
		
}

void sendWordLen (int sock, int wordL)
{
	int len = htonl(wordL);
    int sendLen = sendHelp(sock, (char*) &len, sizeof(len));
    if (sendLen < 0) {
      	cout << "ERROR: failed to send the word length\n";
      	exit(1);
    }
}

double recvGrade (int sock)
{
	// receive gradeStrs's length first
	int gradeL = 0;

	if (recvHelp(sock, (char*) &gradeL, sizeof(gradeL)) < 0) {
	    cout << "ERROR: failed to receive grade string length\n";
	    exit(1);
	}
	gradeL = ntohl(gradeL);
	
	if (gradeL <= 0) {
      cout << "ERROR: received invalid grade string length value " << gradeL << " from the client\n";
      errno = EINVAL;
      exit(1);
  	}
    
    // recv the grade as string
  	char* gr = new char [gradeL + 1];
  	if (recvHelp(sock, gr, gradeL) < 0) {
      cout <<  "ERROR: failed to receive the grade\n";
      delete gr;
      exit(1);
  	}
  	gr[gradeL] = '\0'; 
  	
  	// convert string to double
  	double grade = atof (gr);
  	
  	delete gr;
  	return grade;
}


char recvGuess (int sock) 
{
	char guess[1];
  	if (recvHelp(sock, guess, 1) < 0) {
      cout << "ERROR: failed to receive the client's guess\n";
      exit(1);
  	}
	
	return guess[0];
}

void sendGuessResult (int sock, char* guessResp, int arrS)
{
	// send length first
	int len = htonl(arrS); // not including null-terminated
    int sendLen = sendHelp(sock, (char*) &len, sizeof(len));
    if (sendLen < 0) {
      cout << "ERROR: failed to send the length of the guess response array\n";
      exit(1);
    }
    // send response array
    sendLen = sendHelp(sock, guessResp, arrS); 
    if (sendLen < 0) {
      cout << "ERROR: failed to send guess response array\n";
      exit(1);
    }
}


int getPortNum (char* argv[])
{
  if (argv[1] == NULL) {
    cout << "ERROR: Argument is NULL\n";
    exit(1);
  }
  
  char* endptr = NULL;
  // the port passed in is string --> make it become digits type
  unsigned long portN = strtoul(argv[1], &endptr, 10);

  if (endptr != NULL && *endptr != '\0') {
    cout << "ERROR: " << argv[1] << " is not a number\n";
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

  return portN;
}

void getWord (string& w) 
{
	string line;
	ifstream wordF ("/home/fac/lillethd/cpsc3500/projects/networking/words.txt");
	if (!wordF.is_open()) { // check for file exist
      cout << "Unable to open file!\n";
      exit(1);
	}
	// leaderboard handle _the index
	i_board++;
	
    for (int i = 0; i < i_board; i++)
      getline (wordF,line);
    
	wordF.close();
	
	w = line;
}

void recvClientName (int sock, string &cName) 
{	
	// receive name's length first
	int nameL = 0;
	
	if (recvHelp(sock, (char*) &nameL, sizeof(nameL)) < 0) {
	    cout << "ERROR: failed to receive name's length\n";
	    exit(1);
	}
	nameL = ntohl(nameL);
	
	if (nameL <= 0) {
      cout << "ERROR: received invalid name's length value " << nameL << " from the client\n";
      errno = EINVAL;
      exit(1);
  	}
    
    // recv name
  	char* name = new char [(nameL + 1) * sizeof(char)];
  	if (recvHelp(sock, name, nameL) < 0) {
      cout <<  "ERROR: failed to receive the client's name\n";
      delete name;
      exit(1);
  	}
  	name[nameL] = '\0'; 
  	cName = name;
  	
  	delete name;
}

int recvHelp(int sock, char* buffer, unsigned long numB) {
  int recvLen = 0;
  
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
    buffer = buffer + recvLen;
    numB = numB - recvLen;
    assert(numB >= 0);
  }
  
  return 0;
}

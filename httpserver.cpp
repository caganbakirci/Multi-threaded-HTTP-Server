#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstddef>

#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <fcntl.h>
#include <err.h>

#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <string.h>
#include <cstdlib>

#include <pthread.h>

#include <unordered_map>

using namespace std;

void *threadHandler(void *id);
void logWriter(int val, char someArr[], int buffLen);
void argParser(int argc, char *argv[]);
void headerMessage(char head[], char thirdVal[], int32_t errNum, string errMsg);
bool validateResourceName(char dataToValid[], ssize_t length);
bool checkGetCommand(char name[]);
bool checkPutCommand(char name[]);
bool checkPatchCommand(char name[]);
int adjustThreadCount();
void hashMap(char name[], char alias[]);
bool hashRead(char alias[]);
void getHandler(int accept);
void putHandler(int accept);
void patchHandler(int accept);

//Arg Values
int numOfThreads = 4;
bool logCreate = false;
char myHostName[15] = "";
char portVal[5] = "80";
char fileName[20] = "";
char hashDocName[20] = "";
int logOpValue = 0;
ssize_t newOpenVal = 0;

bool flag = false;

//Others
char buff[16384];
ssize_t MAX_SIZE = 16384;
//Recieved Header Values
char first[5];
char second[28];
char third[8];
char header[4096];
int32_t errorCode = 200;
string errorMessage = "OK";

ssize_t acpVal = 0;
char hName[128];
ssize_t hashOpenVal = 0;

int currThreadCount = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;

int main(int argc, char *argv[])
{
	argParser(argc, argv); // arg values have been arranged

	hashOpenVal = open(hashDocName, O_RDWR | O_CREAT);

	pthread_t threads[numOfThreads]; // Create threads
	for (int i = 0; i < numOfThreads; i++)
	{
		pthread_create(&threads[i], NULL, threadHandler, &i);
	}

	if (logCreate)
	{
		logOpValue = open(fileName, O_CREAT | O_RDWR);
	}

	char *hostname = myHostName;
	char *port = portVal;
	struct addrinfo *addrs, hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(hostname, port, &hints, &addrs);
	int main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
	int enable = 1;
	setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);
	// N is the maximum number of "waiting" connections on the socket.
	// We suggest something like 16.
	listen(main_socket, 16);
	// Your code, starting with accept(), goes here

	while (1)
	{
		acpVal = accept(main_socket, addrs->ai_addr, &(addrs->ai_addrlen));
		pthread_mutex_lock(&mutex);

		if (acpVal > 0)
		{
			pthread_cond_signal(&cond);
			currThreadCount = adjustThreadCount();
		}
		pthread_mutex_unlock(&mutex);
	}
}

void *threadHandler(void *id)
{
	while (acpVal <= 0)
	{
		pthread_cond_wait(&cond, &mutex); //wait for the condition
	}

	recv(acpVal, buff, 4096, 0);

	//PARSE THE BUFFER NOW
	sscanf((char *)buff, "%s  %s  %s", first, second, third);
	ssize_t len = strlen(second); // Get the length of the file to make sure it's 27 or 28

	if (strlen(second) > 0)
	{
		if (second[0] == '/')
		{
			memmove(second, second + 1, strlen(second));
		}
	}

	//BUFFER IS PARSED
	bool valid = validateResourceName(second, len); //Check the resource name
	bool pCommand = checkPatchCommand(first);
	bool isFoundinHash = hashRead(second);

	if (!valid && strlen(hName) > 0)
	{
		strcpy(second, hName);
	}

	if (!valid && !pCommand && !isFoundinHash) 
	{
		errorCode = 400;
		errorMessage = "BAD REQUEST"; //Adjust the error message
		headerMessage(header, third, errorCode, errorMessage);
		send(acpVal, header, strlen(header), 0);
		if (logCreate)
		{
			logWriter(logOpValue, header, strlen(header));
		}
		close(acpVal);
	}
	else
	{
		//GET
		if (first[0] == 'G' && first[1] == 'E' && first[2] == 'T') // This is GET. HANDLE GET HERE.
		{
			getHandler(acpVal);
		}
		//PUT
		else if (first[0] == 'P' && first[1] == 'U' && first[2] == 'T') // This is PUT. HANDLE PUT HERE.
		{
			putHandler(acpVal);
		}
		//PATCH
		else if (first[0] == 'P' && first[1] == 'A' && first[2] == 'T' && first[3] == 'C' && first[4] == 'H') // This is PATCH. HANDLE PATCH HERE.
		{
			patchHandler(acpVal);
		}
		else //The command was neither GET nor PUT or PATCH
		{
			errorCode = 400;									   // Adjust the error code
			errorMessage = "Bad Request";						   //Adjust the error message
			headerMessage(header, third, errorCode, errorMessage); // Construct the header message
			send(acpVal, header, strlen(header), 0);			   // Send the header message
			if (logCreate)
			{
				logWriter(logOpValue, header, strlen(header));
			}
		}
		buff[0] = '\0';
		first[0] = '\0';
		second[0] = '\0';
		third[0] = '\0';
		header[0] = '\0';
		pthread_cond_wait(&cond, &mutex);
		close(acpVal); // Close
		pthread_exit(NULL);
	}
}

void logWriter(int val, char someArr[], int buffLen)
{
	if (val < 0)
	{
		cout << "Error: File could not be opened" << endl;
	}
	else // opened
	{
		int writeVal = write(logOpValue, someArr, buffLen);
		if (writeVal < 0)
		{
			cout << "Failed to write to the log file" << endl;
		}
	}
}

void argParser(int argc, char *argv[])
{
	bool found = false;
	for (int i = 1; i < argc - 1; i++)
	{
		if (!strcmp(argv[i], "-a"))
		{
			found = true;
			strcpy(hashDocName, argv[i + 1]);
		}
		else if (!strcmp(argv[i], "-N"))
		{
			numOfThreads = atoi(argv[i + 1]);
		}
		else if (!strcmp(argv[i], "-l"))
		{
			logCreate = true;
			strcpy(fileName, argv[i + 1]);
		}
		else
		{
			strcpy(myHostName, argv[i]);
			strcpy(portVal, argv[i + 1]);
		}
	}
	if (!found)
	{
		cout << "Error: -a must be presented" << endl;
		exit(-1);
	}
}

//function to construct the header message
void headerMessage(char head[], char thirdVal[], int32_t errNum, string errMsg)
{
	sprintf(head, "%s %d %s\r\n\r\n", thirdVal, errNum, errMsg.c_str());
}

//Function that validates the resource name
bool validateResourceName(char dataToValid[], ssize_t length)
{
	bool check = true;

	if (length != 27) //if (length > 28 || length < 27)
	{
		check = false;
	}
	else
	{
		ssize_t cnt = 0;
		while (cnt < length && check)
		{
			if (dataToValid[cnt] < 48 && dataToValid[cnt] != 45)
			{
				check = false;
			}
			else if (dataToValid[cnt] > 57 && dataToValid[cnt] < 65)
			{
				check = false;
			}
			else if (dataToValid[cnt] > 90 && dataToValid[cnt] != 95 && dataToValid[cnt] < 97)
			{
				check = false;
			}
			else if (dataToValid[cnt] > 122)
			{
				check = false;
			}
			cnt++;
		}
	}
	return check;
}

bool checkGetCommand(char name[])
{
	return (name[0] == 'G' && name[1] == 'E' && name[2] == 'T');
}

bool checkPutCommand(char name[])
{
	return (name[0] == 'P' && name[1] == 'U' && name[2] == 'T');
}

bool checkPatchCommand(char name[])
{
	return (name[0] == 'P' && name[1] == 'A' && name[2] == 'T' && name[3] == 'C' && name[4] == 'H');
}

int adjustThreadCount()
{
	int num = currThreadCount;
	int tCnt = numOfThreads;

	return (num + 1) % tCnt;
}

void getHandler(int accept)
{
	ssize_t openVal = open(second, O_RDONLY, O_CREAT); // Open the file
	if (openVal < 0 && !flag)						   //If file could not opened
	{
		errorCode = 404;				 // Adjust the error code
		errorMessage = "File not found"; //Adjust the error message
		headerMessage(header, third, errorCode, errorMessage);
		send(accept, header, strlen(header), 0);
		if (logCreate)
		{
			logWriter(logOpValue, header, strlen(header));
		}
	}
	else //We opened the file.
	{
		ssize_t readCount = read(openVal, buff, MAX_SIZE); //Read from the file
		if (readCount < 0)								   //Read failed
		{
			errorCode = 403;					  // Adjust the error code
			errorMessage = "File could not read"; //Adjust the error message
			headerMessage(header, third, errorCode, errorMessage);
			send(accept, header, strlen(header), 0);
			if (logCreate)
			{
				logWriter(logOpValue, header, strlen(header));
			}
		}
		else // Read was sucsessful
		{
			errorCode = 200;
			errorMessage = "OK";
			headerMessage(header, third, errorCode, errorMessage);
			send(accept, header, strlen(header), 0); // send the header
			if (logCreate)
			{
				logWriter(logOpValue, header, strlen(header));
			}
			errorMessage.clear();
			while (readCount > 0) // As long as there is data to be read
			{
				send(accept, buff, readCount, 0); //Send the data
				if (logCreate)
				{
					logWriter(logOpValue, buff, strlen(buff));
				}
				readCount = read(openVal, buff, MAX_SIZE); //Update read
			}
			close(openVal); // Close
		}
	}
	close(accept);
	close(openVal);
}

void putHandler(int accept)
{
	ssize_t openVal = open(second, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR); // Open the file

	char placeHolder[15];
	char strToInt[10];
	int32_t contentLength = 0;

	char *ptr = strstr(buff, "Content-Length:");
	if (ptr != nullptr)
	{
		//	cout << "*ptr ->" << ptr << endl;
		sscanf(ptr, "%s %s", placeHolder, strToInt); //Parse to get the Content Length
		sscanf(strToInt, "%d", &contentLength);		 //Conversion of content length from string to int
	}
	else
	{
		cout << "No content length specified" << endl;
	}

	if (openVal < 0) // Open failed
	{
		errorCode = 201;									   // Adjust the error code
		errorMessage = "File does not exist";				   //Adjust the error message
		headerMessage(header, third, errorCode, errorMessage); // Construct the header message
		send(accept, header, strlen(header), 0);			   // Send the header message
		if (logCreate)
		{
			logWriter(logOpValue, header, strlen(header));
		}
	}
	else //We opened the file.
	{
		ssize_t readCount = read(accept, buff, 16384); // Read the file
		if (readCount < 0) // Read failed
		{
			errorCode = 403;									   // Adjust the error code
			errorMessage = "File could not read";				   //Adjust the error message
			headerMessage(header, third, errorCode, errorMessage); // Construct the header message
			send(accept, header, strlen(header), 0);			   // Send the header message
			if (logCreate)
			{
				logWriter(logOpValue, header, strlen(header));
			}
		}
		else //Read was sucsessful
		{
			ssize_t writeCount = write(openVal, buff, readCount); //Write to the file
			if (writeCount >= 0)								  // Write was sucsessful
			{
				errorCode = 200;
				errorMessage = "OK";
				headerMessage(header, third, errorCode, errorMessage); // Construct the header message
				send(accept, header, strlen(header), 0);			   // Send the header message
				if (logCreate)
				{
					logWriter(logOpValue, header, strlen(header));
				}
				errorMessage.clear();

				while (writeCount > 0 && writeCount >= MAX_SIZE)
				{
					ssize_t recVal = recv(accept, buff, MAX_SIZE, 0); //Keep recieving the rest of the file
					writeCount = write(openVal, buff, recVal);		  // Write and update the writeCount
					if (logCreate)
					{
						logWriter(logOpValue, buff, recVal);
					}
				}
			}
			else // Write was not sucsessful
			{
				errorCode = 403;									   // Adjust the error code
				errorMessage = "File could not be written";			   //Adjust the error message
				headerMessage(header, third, errorCode, errorMessage); // Construct the header message
				send(accept, header, strlen(header), 0);			   // Send the header message
				if (logCreate)
				{
					logWriter(logOpValue, header, strlen(header));
				}
				//close(acpVal);
			}
		}
	}
	close(openVal);
	close(accept);
}

void patchHandler(int accept)
{
	char sign[5];
	char httpName[27];
	char alias[128];
	//ssize_t openVal = open(second, O_RDONLY); // Open the file
	open(second, O_RDONLY); // Open the file

	char placeHolder[15];
	char strToInt[10];
	int32_t contentLength = 0;
	char *ptr = strstr(buff, "Content-Length:");
	sscanf(ptr, "%s %s", placeHolder, strToInt); //Parse to get the Content Length
	sscanf(strToInt, "%d", &contentLength);		 //Conversion of content length from string to int
	recv(accept, buff, contentLength, 0);
	sscanf((char *)buff, "%s  %s  %s\r", sign, httpName, alias);
	ssize_t aliasLen = strlen(alias);
	alias[aliasLen - 1] = '\0';
	alias[aliasLen - 2] = '\0';
	alias[aliasLen - 3] = '\0';
	alias[aliasLen - 4] = '\0';

	ssize_t myHashVal = open(httpName, O_RDWR); // Open the file

	if (myHashVal < 0) // Open failed
	{
		errorCode = 404;									   // Adjust the error code
		errorMessage = "File does not exist";				   //Adjust the error message
		headerMessage(header, third, errorCode, errorMessage); // Construct the header message
		send(accept, header, strlen(header), 0);			   // Send the header message
		if (logCreate)
		{
			logWriter(logOpValue, header, strlen(header));
		}
	}
	else //We opened the file.
	{
		hashMap(alias, httpName);
	}
	close(accept);
}

void hashMap(char alias[], char name[])
{
	int32_t len = strlen(alias);
	int32_t keyVal = 0;
	for (int32_t i = 0; i < len; i++)
	{
		keyVal += alias[i] * i;
	}

	int32_t location = (keyVal % 8000) * 128;
	pwrite(hashOpenVal, name, strlen(name), location);
}

bool hashRead(char alias[])
{
	bool val = false;
	int32_t len = strlen(alias);
	int32_t keyVal = 0;
	for (int32_t i = 0; i < len; i++)
	{
		keyVal += alias[i] * i;
	}

	int32_t location = (keyVal % 8000) * 128;
	ssize_t pCount = pread(hashOpenVal, hName, 128, location);

	if (pCount >= 0)
	{
		val = true;
	}

	return val;
}
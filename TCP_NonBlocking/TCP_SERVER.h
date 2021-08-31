#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include<stdlib.h>
#include<stdio.h>
#include <string>
#include <fstream>
#include <sstream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
typedef enum Connect {
	keep_alive = 1,
	close = 2
} Connect;
typedef enum eMethod {
	GET = 1,
	POST = 2,
	HEAD = 3,
	PUT = 4,
	_DELETE = 5,
	TRACE = 6,
	OPTIONS = 7
} eMethod;
struct request_info
{
	char* resource_path;
	char*fullpath;
	int content_length;
	char*host;
	char*accepted_language;
	eMethod contentType;
	Connect connectType;
	char*body_message;
	char*acceptType;
	char*version;
	int linebody;
	string res;


};
struct response_info
{
	char*status_code;
	char*date;
	Connect connection;
	int age;
	int content_length;
	char *last_modified;
	char*version;
	char*accpettype;
};
typedef enum eStatus {
	EMPTY = 0,
	LISTEN = 1,
	RECEIVE = 2,
	IDLE = 3,
	SEND = 4,
} eStatus;


struct SocketState
{
	request_info*requesting;
	response_info*responsing;
	SOCKET id;				// Socket handle
	sockaddr_in addr;
	eStatus	recv;			// Receiving?
	eStatus	send;	// Sending?	// Sending sub-type
	eMethod sendSubType;
	char buffer[10000];
	int len;
	time_t lastUsed;
};
const int MAX_SOCKETS = 5;
const int TIME_PORT = 27016;
struct SocketState sockets[MAX_SOCKETS] = { 0 }; //global dew lazyness
int socketsCount = 0;
struct timeval timeOut;
void acceptConnection(int index);
bool addSocket(SOCKET id, sockaddr_in socketAddr, eStatus what);
void receiveMessage(int index);
void removeSocket(int index);
void intitiall(request_info*requ, int index);
void fillRequestHeaderMethod(request_info*requ, char*buff, int index);
void fillheadersandbody(request_info*requ, char*buff);
int countLines(char*buff);
int ConvertStringToInt(char str[]);
void sendMessage(int index);
string createGetAnswer(int index);
void fillHeaderToStruct(int index);
string prepare(int index);
int countdigits(int number);
void convertIntToString(char*num, int number, int countdigs);
string createPostAnswer(int index);
void checkStatusCode(int index);
void WriteToFile(int index, char*buffBody);
string createPutAnswer(int index);
string createHeadAnswer(int index);
string createOptionsAnswer(int index);
string prepareWithOptions(int index);
void preparewithopation(int index, char*buff);
void preparenotOption(int index, char*buff);
string CreateDeleteAnswer(int index);
int statusCode(int index);
void updatefullPath(request_info*requ, int index);
int checkstat(char*buff);
void createRequestInfo(request_info*requ, char*buff);
string createTraceAnswer(int index);

#include "TCP_SERVER.h"

void main()
{
	time_t currentTime;

	// Initialize Winsock (Windows Sockets).
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	// Create and bind a TCP socket.

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, serverService, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		//remove sockets that past timeout
		for (int i = 1; i < MAX_SOCKETS; i++)
		{
			currentTime = time(0);
			if ((sockets[i].recv != EMPTY || sockets[i].send != EMPTY) &&
				(sockets[i].lastUsed != 0) && (currentTime - sockets[i].lastUsed > 120))
			{
				cout << "Time Server: Client " << inet_ntoa(sockets[i].addr.sin_addr) << ":" << ntohs(sockets[i].addr.sin_port) << " has been disconected due to timeout." << endl;
				removeSocket(i);
			}
		}

		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeOut);
		/*select(int nfds, fd_set *read-fds, fd_set *write-fds, fd_set *except-fds, struct timeval *timeout)*/
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	//we will never get here because its a server...
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}
bool addSocket(SOCKET id, sockaddr_in socketAddr, eStatus what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			unsigned long flag = 1;
			if (ioctlsocket(id, FIONBIO, &flag) != 0)
			{
				cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
			}
			sockets[i].id = id;
			sockets[i].addr = socketAddr;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].lastUsed = time(0);
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	if (sockets[index].requesting != nullptr)
	{
		if (sockets[index].requesting->resource_path != nullptr)
		{
			delete[](sockets[index].requesting->resource_path);
		}
		if (sockets[index].requesting->accepted_language != nullptr)
		{
			delete[](sockets[index].requesting->accepted_language);
		}
		if (sockets[index].requesting->acceptType != nullptr)
		{
			delete[](sockets[index].requesting->acceptType);
		}
		if (sockets[index].requesting->body_message != nullptr)
		{
			delete[](sockets[index].requesting->body_message);
		}
		if(sockets[index].requesting->fullpath!=nullptr)
		{
			delete[](sockets[index].requesting->fullpath);
		}
	}
if(sockets[index].responsing!=nullptr)
{
	if(sockets[index].responsing->status_code!=nullptr)
	{
		delete[](sockets[index].responsing->status_code);
	}
	if(sockets[index].responsing->date!=nullptr)
	{
		delete[](sockets[index].responsing->date);
	}
	if(sockets[index].responsing->version!=nullptr)
	{
		delete[](sockets[index].responsing->version);
	}
	if(sockets[index].responsing->accpettype!=nullptr)
	{
		delete[](sockets[index].responsing->accpettype);
	}
}
	closesocket(sockets[index].id);
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSocket, from, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!" << endl;
		closesocket(msgSocket);
	}

	return;
}
void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of " << endl;
		cout << "\"" << &sockets[index].buffer[len] << "\"" << endl << "message." << endl;

		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			sockets[index].requesting = new request_info[1];
			intitiall(sockets[index].requesting, index);
			fillRequestHeaderMethod(sockets[index].requesting, &sockets[index].buffer[len], index);
			fillheadersandbody(sockets[index].requesting, &sockets[index].buffer[len]);
			createRequestInfo(sockets[index].requesting, &sockets[index].buffer[len]);
		}
	}
}
void intitiall(request_info*requ, int index)
{

	requ->resource_path = nullptr;
	requ->fullpath = nullptr;
	requ->content_length = 0;
	requ->host = nullptr;
	requ->accepted_language = nullptr;
	requ->contentType = GET;
	requ->body_message = nullptr;
	requ->version = nullptr;
	requ->linebody = 0;
}
void intiallresponse(response_info*res)
{
	res->version = nullptr;
	res->date = nullptr;
	res->last_modified = nullptr;
	res->status_code = nullptr;
	res->accpettype = nullptr;
}
void fillRequestHeaderMethod(request_info*requ, char*buff, int index)
{
	istringstream resp(buff);
	string header;
	char method[15];
	getline(resp, header);
	int indexes = header.find(' ');
	int i = 0;
	int k = 0;
	while (i<indexes)
	{
		method[i] = header[i];
		i++;
	}
	method[i] = '\0';
	i = 0;
	if (strncmp(method, "GET", 3) == 0)
	{
		requ->contentType = GET;
		sockets[index].send = SEND;

	}
	if (strncmp(method, "POST", 4) == 0)
	{
		requ->contentType = POST;
		sockets[index].send = SEND;
	}
	if (strncmp(method, "HEAD", 4) == 0)
	{
		requ->contentType = HEAD;
		sockets[index].send = SEND;
	}
	if (strncmp(method, "PUT", 3) == 0)
	{
		requ->contentType = PUT;
		sockets[index].send = SEND;
	}
	if (strncmp(method, "DELETE", 6) == 0)
	{
		requ->contentType = _DELETE;
		sockets[index].send = SEND;

	}
	if (strncmp(method, "OPTIONS", 7) == 0)
	{
		requ->contentType = OPTIONS;
		sockets[index].send = SEND;
	}
	if (strncmp(method, "TRACE", 5) == 0)
	{
		requ->contentType = TRACE;
		sockets[index].send = SEND;
	}
	char resorch[250];
	requ->resource_path = new char[200];
	int indexes2 = header.find(' ', indexes + 1);
	int indexesslesh = header.find('/');
	i = indexesslesh + 1;
	int p = 0;
	while (header[i]!=' '&&header[i]!='?')
	{
		resorch[p] = header[i];
		p++;
		i++;
	}
	resorch[p] = '\0';
	strcpy(requ->resource_path, resorch);
	requ->accepted_language = new char[30];
	requ->accepted_language[0] = '\0';
	int indexlange = header.find("lang");
	if(indexlange!=std::string::npos)
	{
		int equal = header.find("=");
		k = equal + 1;
		p = 0;
		while (k < indexes2)
		{
			requ->accepted_language[p] = header[k];
			p++;
			k++;
		}
		requ->accepted_language[p] = '\0';
	}
	updatefullPath(requ,index);
	requ->version = new char[15];
	int j = indexes2 + 1;
	p = 0;
	while (header[j] != '\0')
	{
		requ->version[p] = header[j];
		p++;
		j++;
	}
	requ->version[p] = '\0';
	p = 0;


}
void updatefullPath(request_info*requ,int index)
{
	requ->fullpath = new char[50];
	char*finish = new char[20];
	finish[0] = '\0';
	int i = 0;
	int p = 0;
	int t = 0;
	if (requ->accepted_language[0] != '\0')
	{
		while (requ->resource_path[i] != '.')
		{
			requ->fullpath[p] = requ->resource_path[i];
			p++;
			i++;
		}
		requ->fullpath[p] = '_';
		p++;
		int j = 0;
		while(requ->accepted_language[j]!='\0')
		{
			requ->fullpath[p] = requ->accepted_language[j];
			p++;
			j++;
		}
		while(requ->resource_path[i]!='\0')
		{
			requ->fullpath[p] = requ->resource_path[i];
			p++;
			i++;
		}
		requ->fullpath[p] = '\0';
	}
	else
	{
		strcpy(requ->fullpath, requ->resource_path);
	}

}
void fillheadersandbody(request_info*requ, char*buff)
{
	int foundType = 0;
	bool found = false;
	int count = countLines(buff);
	int indexLine = 0;
	istringstream resp(buff);
	string header;
	char str[100];
	char str2[100];
	int i = 0;
	int p = 0;
	int j;
	requ->acceptType = new char[100];
	requ->acceptType[0] = '\0';
	requ->body_message = new char[1000];
	requ->body_message[0] = '\0';
	getline(resp, header);
	indexLine = 1;
	while (indexLine <= count&&header != "\r")
	{
		i = 0;
		int indexes = header.find(':');
		while (i<indexes)
		{
			str[i] = header[i];
			i++;
		}
		str[i] = '\0';
		if (strncmp(str, "Connection", 10) == 0)
		{
			j = indexes + 2;
			p = 0;
			while (header[j] != '\0')
			{
				str2[p] = header[j];
				p++;
				j++;
			}
			str2[j] = '\0';
			j = 0;
			if (strncmp(str2, "keep-alive", 10) == 0)
			{
				requ->connectType = keep_alive;
			}
			if (strncmp(str2, "close", 5) == 0)
			{
				requ->connectType = close;
			}
			str2[0] = '\0';

		}
		if ((strcmp(str, "Accept") == 0)&&foundType==0)
		{
			j = indexes + 2;
			p = 0;
			while (header[j]!=','&&header[j]!='\0')
			{
				str2[p] = header[j];
				p++;
				j++;
			}
			str2[p] = '\0';
			strcpy(requ->acceptType, str2);
			str2[0] = '\0';
		}
		if (strcmp(str, "Content-Type") == 0)
		{
			j = indexes + 2;
			p = 0;
			while (header[j] != '\0')
			{
				requ->acceptType[p] = header[j];
				p++;
				j++;
			}
			requ->acceptType[p] = '\0';
			foundType = 1;
		}
		if (strcmp(str, "Content-Length") == 0)
		{
			char buffint[12];
			buffint[0] = '\0';
			j = indexes + 2;
			p = 0;
			while ((header[j] != '\r') && (header[j] != '\0'))
			{
				buffint[p] = header[j];
				p++;
				j++;
			}
			buffint[p] = '\0';
			requ->content_length = ConvertStringToInt(buffint);
			str2[0] = '\0';
			buffint[0] = '\0';

		}
		str[0] = '\0';
		indexLine++;
		getline(resp, header);

	}
	if (indexLine == count + 1)
	{
		requ->body_message[0] = '\0';
	}
	else
	{
		getline(resp, header);
		indexLine++;
		i = 0;
		p = 0;
		while (indexLine <= count)
		{
			if (header[i] != '\0')
			{
				requ->body_message[p] = header[i];
				p++;
				i++;
			}
			else
			{
				requ->body_message[p] = '\n';
				p++;
				getline(resp, header);
				indexLine++;
				i = 0;
				requ->linebody++;


			}
		}
		requ->body_message[p] = '\0';
	}

}
void createRequestInfo(request_info*requ, char*buff)
{
	requ->res = "";
	int count = countLines(buff);
	int indexline = 0;
	istringstream resp(buff);
	string header;
	getline(resp, header);
	indexline = 1;
	while(indexline<=count)
	{
		requ->res = requ->res + header;
		requ->res = requ->res + "\n";
		indexline++;
		getline(resp, header);
	}
	
}
int countLines(char*buff)
{
	istringstream resp(buff);
	string header;
	int count = 0;
	while (getline(resp, header))
	{
		count++;
	}
	return count;
}
int ConvertStringToInt(char str[])
{
	int i = 0;
	int count = strlen(str);
	int sum = 0;
	while (i<count)
	{
		sum = sum * 10 + str[i] - '0';
		i++;
	}
	return sum;
}
void sendMessage(int index)
{
	int bytesSent = 0;
	string sendbuffer="";
	int buffer_len = 0;
	SOCKET msgSocket = sockets[index].id;
	sockets[index].lastUsed = time(0);
	switch (sockets[index].requesting->contentType)
	{
	case GET:
		sendbuffer=createGetAnswer(index);
		break;
	case POST:
		sendbuffer=createPostAnswer(index);
		break;
	case PUT:
		sendbuffer = createPutAnswer(index);
		break;
	case HEAD: 
		sendbuffer = createHeadAnswer(index);
		break;
	case OPTIONS:
		sendbuffer= createOptionsAnswer(index);
		break;
	case _DELETE:
		sendbuffer = CreateDeleteAnswer(index);
		break;
	case TRACE:
		sendbuffer = createTraceAnswer(index);
		break;





	}
	if(sockets[index].requesting->contentType==POST)
	{
		cout << sendbuffer << endl;
		sockets[index].send = IDLE;
		removeSocket(index);
		return;
	}
	else
	{
		buffer_len = sendbuffer.size();
		bytesSent = send(msgSocket, sendbuffer.c_str(), buffer_len, 0);
		memset(sockets[index].buffer, 0, 10000);
		sockets[index].len = 0;
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "HTTP Server: Error at S_SEND(): " << WSAGetLastError() << endl;
			sockets[index].send = IDLE;
			return;
		}

		cout << "HTTP Server: Sent: " << bytesSent << "\\" << buffer_len << " bytes of " << endl;
		cout << "\"" << sendbuffer.c_str() << "\"" << endl << "message." << endl;
		sockets[index].send = IDLE;
		removeSocket(index);
		return;
	}
	
}
string createGetAnswer(int index)
{
	string send = "";
	fillHeaderToStruct(index);
	sockets[index].responsing->content_length = 0;
	char readBuff[500];
	string headersend = "";
	string bodyMessage ="";
	sockets[index].responsing->status_code = new char[30];
	ifstream file;
	string str;
	char*buffer = new char[30];
	buffer[0] = '\0';
	int status = checkstat(sockets[index].requesting->fullpath);
	if (status == 1)
		strcpy(buffer, sockets[index].requesting->fullpath);
	else
		strcpy(buffer, sockets[index].requesting->resource_path);
	// VALIDATE CONTENT TYPE
	bool isValidContetntType = validateContentType(sockets[index].requesting->acceptType);
	if (!isValidContetntType) {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
		headersend = prepare(index);
		send = headersend;
	}
	else {
		file.open(buffer);
		if (file.is_open() == false)
		{
			strcpy(sockets[index].responsing->status_code, "404 NOT FOUND");
			headersend = prepare(index);
			send = headersend;
		}
		else
		{
			strcpy(sockets[index].responsing->status_code, "200 OK");
			while (file.getline(readBuff, 512))
			{
				sockets[index].responsing->content_length = sockets[index].responsing->content_length + strlen(readBuff);
				bodyMessage = bodyMessage + readBuff;
				bodyMessage = bodyMessage + "\n";

			}
			headersend = prepare(index);
			headersend = headersend + "\n";
			send = headersend + bodyMessage;
			file.close();


		}
	}
	return send;

}

bool validateContentType(char* contentType) {
	return strncmp(contentType, "text/html", 9) == 0;
}

int checkstat(char*buff)
{
	ifstream file;
	int status = 0;
	file.open(buff);
	if(file.is_open()==false)
	{
		status = 0;
	}
	else
	{
		status = 1;
		file.close();
	}
	return status;

}

void fillHeaderToStruct(int index)
{
	sockets[index].responsing = new response_info[1];
	intiallresponse(sockets[index].responsing);
	sockets[index].responsing->version = new char[20];
	strcpy(sockets[index].responsing->version, sockets[index].requesting->version);
	sockets[index].responsing->connection = sockets[index].requesting->connectType;
	time_t CurrentTime;
	time(&CurrentTime);
	sockets[index].responsing->date = new char[100];
	strcpy(sockets[index].responsing->date, ctime(&CurrentTime));
	(sockets[index].responsing->date)[strlen(sockets[index].responsing->date) - 1] = '\0';
	sockets[index].responsing->accpettype = new char[50];
	strcpy(sockets[index].responsing->accpettype, sockets[index].requesting->acceptType);
}
string prepare(int index)
{
	int current = 0;
	string send = "";
	char num[10];
	send = send + "HTTP 1.1" + " " + sockets[index].responsing->status_code + "\n";
	if(sockets[index].responsing->connection==keep_alive)
	{
		send = send + "Connection" + " " + "keep-alive" + "\n";
	}
	else
	{
		send = send + "Connection" + " " + "close" + "\n";
	}
	send = send + "Date:" + " " + sockets[index].responsing->date + "\n";
	int digcount = countdigits(sockets[index].responsing->content_length);
	convertIntToString(num, sockets[index].responsing->content_length, digcount);
	send = send + "Content-Length: " + num + "\n";
	send = send + "Content-Type: " + sockets[index].responsing->accpettype;
	return send;
}

int countdigits(int number)
{
	int count = 0;
	while(number>0)
	{
		number = number / 10;
		count++;
	}
	return count;

}
void convertIntToString(char*num, int number,int countdigs)
{
	int i = countdigs - 1;
	while(number>0)
	{
		int dig = number % 10;
		if (dig == 9)
			num[i] = '9';
		if (dig == 8)
			num[i] = '8';
		if (dig == 7)
			num[i] = '7';
		if (dig == 6)
			num[i] = '6';
		if (dig == 5)
			num[i] = '5';
		if (dig == 4)
			num[i] = '4';
		if (dig == 3)
			num[i] = '3';
		if (dig == 2)
			num[i] = '2';
		if (dig == 1)
			num[i] = '1';
		if (dig == 0)
			num[i] = '0';
		i--;
		number = number / 10;
	}
	num[countdigs] = '\0';
}
string createPostAnswer(int index)
{
	string send = "";
	fillHeaderToStruct(index);
	sockets[index].responsing->content_length = 0;
	sockets[index].responsing->status_code = new char[30];
	bool isValidContentType = validateContentType(sockets[index].responsing->accpettype);
	if (isValidContentType) {
		checkStatusCode(index);
		WriteToFile(index, sockets[index].requesting->body_message);
	}
	else {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
	}
	
	send = prepare(index);
	return send;
}

void checkStatusCode(int index)
{
	ifstream file;
	int count = 0;
	char readBuff[500];
	file.open(sockets[index].requesting->resource_path);
	if(file.is_open()==false)
	{
		strcpy(sockets[index].responsing->status_code, "201 CREATED");
		
	}
	else
	{
		count = 0;
		while(file.getline(readBuff,512))
		{
			sockets[index].responsing->content_length = sockets[index].responsing->content_length + strlen(readBuff);
			count++;
		}
		if(count==0)
		{
			strcpy(sockets[index].responsing->status_code, "204 NO CONTENT");
		}
		else
		{
			strcpy(sockets[index].responsing->status_code, "200 0K");
		}
		file.close();
	}
	
}
void WriteToFile(int index,char*buffBody)
{
	int i = 0;
	istringstream resp(buffBody);
	string body;
	int count = 0;
	ofstream myfile;
	myfile.open(sockets[index].requesting->resource_path,ios::app);
	myfile << endl;
	while(count<sockets[index].requesting->linebody)
	{
		getline(resp, body);
		myfile << body << endl;
		count++;
	}
	count = 0;
	myfile.close();
}
string createPutAnswer(int index)
{
	string send = "";
	fillHeaderToStruct(index);
	sockets[index].responsing->content_length = 0;
	sockets[index].responsing->status_code = new char[30];
	bool isValidContentType = validateContentType(sockets[index].responsing->accpettype);
	if (isValidContentType) {
		checkStatusCode(index);
		WriteToFile(index, sockets[index].requesting->body_message);
	}
	else {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
	}
	
	send = prepare(index);
	return send;
}
string createHeadAnswer(int index)
{
	string send = "";
	ifstream file;
	fillHeaderToStruct(index);
	sockets[index].responsing->content_length = 0;
	char readBuff[500];
	sockets[index].responsing->status_code = new char[30];

	bool isValidContentType = validateContentType(sockets[index].responsing->accpettype);
	if (isValidContentType) {
		file.open(sockets[index].requesting->resource_path);
		if (file.is_open() == false)
		{
			strcpy(sockets[index].responsing->status_code, "404 NOT FOUND");
		}
		else
		{
			strcpy(sockets[index].responsing->status_code, "200 OK");
			while (file.getline(readBuff, 512))
			{
				sockets[index].responsing->content_length = sockets[index].responsing->content_length + strlen(readBuff);
			}
			file.close();
		}
	}
	else {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
	}

	send = prepare(index);
	return send;
}
string createOptionsAnswer(int index)
{
	string send = "";
	ifstream file;
	fillHeaderToStruct(index);
	sockets[index].responsing->content_length = 0;
	char readBuff[500];
	sockets[index].responsing->status_code = new char[30];

	bool isValidContentType = validateContentType(sockets[index].responsing->accpettype);
	if (isValidContentType) {
		file.open(sockets[index].requesting->resource_path);
		if (file.is_open() == false)
		{
			strcpy(sockets[index].responsing->status_code, "404 NOT FOUND");
			send = prepare(index);

		}
		else
		{
			strcpy(sockets[index].responsing->status_code, "200 OK");
			while (file.getline(readBuff, 512))
			{
				sockets[index].responsing->content_length = sockets[index].responsing->content_length + strlen(readBuff);
			}
			send = prepareWithOptions(index);
		}
	}
	else {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
		send = prepare(index);
	}
	
	return send;
}
string prepareWithOptions(int index)
{
	string send = "";
	char num[10];
	send = send + "HTTP 1.1" + " " + sockets[index].responsing->status_code + "\n";
	send = send + "Allow: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE" + "\n";
	if (sockets[index].responsing->connection == keep_alive)
	{
		send = send + "Connection" + " " + "keep-alive" + "\n";
	}
	else
	{
		send = send + "Connection" + " " + "close" + "\n";
	}
	send = send + "Date:" + " " + sockets[index].responsing->date + "\n";
	int digcount = countdigits(sockets[index].responsing->content_length);
	convertIntToString(num, sockets[index].responsing->content_length, digcount);
	send = send + "Content-Length: " + num + "\n";
	if (sockets[index].requesting->acceptType[0] != '\0')
		send = send + "Content-Type: " + sockets[index].responsing->accpettype;
	else
		send = send + "Content-Type: " + "text/html";
	return send;
}
string CreateDeleteAnswer(int index)
{
	string send = "";
	ifstream file;
	fillHeaderToStruct(index);
	sockets[index].responsing->status_code = new char[50];
	sockets[index].responsing->content_length = 0;
	bool isValidContentType = validateContentType(sockets[index].responsing->accpettype);
	if (isValidContentType) {
		int status = statusCode(index);

		if (status == 2 || (status == 3)) // File exists
		{
			if (remove(sockets[index].requesting->resource_path) != 0)
			{
				status = 1;
			}

		}
		if (status == 1) // Couldn't perform file deletion
		{
			strcpy(sockets[index].responsing->status_code, "500 Internal Server Error");
		}
		if (status == 2 || status == 3)
		{
			strcpy(sockets[index].responsing->status_code, "200 OK");
		}
	}
	else {
		strcpy(sockets[index].responsing->status_code, "415 Unsupported Media");
	}

	send = prepare(index);
	return send;

}
int statusCode(int index)
{
	int count = 0;
	sockets[index].responsing->content_length = 0;
	ifstream file;
	char readBuff[500];
	int status = 0;
	file.open(sockets[index].requesting->resource_path);
	if(file.is_open()==false)
	{ // File does not exist or is invalid
		status = 1;
	}
	else
	{
		while(file.getline(readBuff,512))
		{
			sockets[index].responsing->content_length = sockets[index].responsing->content_length + strlen(readBuff);
			count++;
		}
		if (count == 0) // File exists but is empty
			status = 2;
		else // File exists and has content
			status = 3;
		file.close();
	}
	return status;
}
string createTraceAnswer(int index)
{
	string send = "";
	fillHeaderToStruct(index);
	sockets[index].responsing->status_code = new char[50];
	sockets[index].responsing->content_length = 0;
	if(sockets[index].requesting->body_message[0]=='\0')
	{
		sockets[index].responsing->content_length = 0;
		strcpy(sockets[index].responsing->status_code, "200 OK There no body");
		send = prepare(index);
	}
	else
	{
		sockets[index].responsing->content_length = strlen(sockets[index].requesting->body_message);
		strcpy(sockets[index].responsing->status_code, "200 OK HAVE A BODY IN REQUEST");
		send = prepare(index);
	}
	send = send + "\n";
	send = send + sockets[index].requesting->res;
	return send;

}


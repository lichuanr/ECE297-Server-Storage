/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 *
 * This storage.c file is used by the client side, which includes
 * several different functions. These functions have the following
 * purposes, which include connect, authenticate, get, set and
 * disconnect
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "storage.h"
#include "utils.h"
#include "logger.h"
#include <errno.h>

#include <ctype.h> // For checking whether something's type is write

/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int Connected = 0;

/**
 * @brief Initializing the Authentication for indicating authenticate
 * status, which will be used in the functions below
 */
int Authenticated = 0;

/**
 * @brief Initializing a filestream
 */
FILE *file;



/**
 * @brief This function is used for removing spaces
 * 
 * When a string be passed inside this function (use 
 * pass by parameter, the spaces inside this string
 * will be get ridded.
 *
 * @param char_source The string which contain spaces and waiting to be purified
 */
void ModifyString(char* char_source)
{
  char* temp1 = char_source;
  char* temp2 = char_source;
  while(*temp2 != 0)
  {
    *temp1 = *temp2++;
    if(*temp1 != ' ')
    {
      temp1 = temp1 + 1;
    }
  }
  *temp1 = 0;
}

/**
 * @brief This function is for connecting to the server
 * from client side.
 *
 * @param hostname The hostname inputted by the user
 * @param port The port inputted by the user
 */
void* storage_connect(const char *hostname, const int port)
{
	//**********For error checking ***********//
	//**********Error checking No.1 **********//
	if ( (hostname == NULL) || (sizeof(port) != sizeof(int)) )
	{

		errno = ERR_INVALID_PARAM;
		return NULL; //When meet error, return -1
	}


	// Create a socket.
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return NULL;

	// Get info about the server.
	struct addrinfo serveraddr, *res;
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.ai_family = AF_UNSPEC;
	serveraddr.ai_socktype = SOCK_STREAM;
	char portstr[MAX_PORT_LEN];
	snprintf(portstr, sizeof portstr, "%d", port);

	int status = getaddrinfo(hostname, portstr, &serveraddr, &res);
	if (status != 0)
		return NULL;//probably error check here

	// Connect to the server.
	status = connect(sock, res->ai_addr, res->ai_addrlen);
	if (status != 0)
	{
		errno = ERR_CONNECTION_FAIL; //**********Error checking No.2 **********//
		return NULL;
	}
	else
	{
		Connected = 1;
	}


	return (void*) sock;
}


/**
 * @brief This function is for authentication based on teh username
 * and password inputted by the user.
 *
 * @param Username The username inputted by the user
 * @params Passwd The password inputted by the user
 * @conn This is a void pointer for connection purpose
 * @return Return 0 if successful, otherwise return -1
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{
	char a, check;
	//**********For error checking ***********//
	//**********Error checking No.1 **********//

	if (username == NULL || passwd == NULL)
	{
		errno = ERR_INVALID_PARAM;
		return -1;
	}
	if(conn == NULL)
	{
		errno = ERR_INVALID_PARAM;
		return -1;
	}
	
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
	snprintf(buf, sizeof buf, "AUTH %s %s\n", username, encrypted_passwd);

	//LOGGING function
	if(LOGGING == 0)
	{
	}
	else if(LOGGING == 1)
	{
		LOG((buf));
	}
	else if(LOGGING == 2)
	{
		logger(file, buf);
	}

	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
	{

		sscanf(buf, "%c %c", &a, &check);
		if(check == 'u')
		{
			printf("You just input a wrong password\n");
			errno = ERR_AUTHENTICATION_FAILED;
			return -1;
			//'u' is passed from the server side. When the passed character is u, we know the password is
			//not right, thus have the error message. If authenticate is right, authenticate is equal to 1
		}
		Authenticated = 1;
		return 0;
	}

	return -1;
}

/**
 * @brief This function is for users get the value from the storage server
 *
 * @param table The name of the table
 * @param key The name of the key
 * @param record Containing the current record
 * @param conn This is a void pointer for connection purpose
 * @return Return 0 if successful, otherwise return -1
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{

//	struct timeval start_time, end_time;
	//**********For error checking ***********//
	//**********Error checking No.1 **********//
	char check;
	int x = 0;
	if (table == NULL || key == NULL || record == NULL)
	{
		errno = ERR_INVALID_PARAM; //Checking invalid parameters
		return -1;
	}
	if(conn == NULL)
	{
		errno = ERR_INVALID_PARAM; //ERR_CONNECTION_FAIL? main.c line.517 says otherwise
		return -1;
	}
	
	while(key[x] != '\0')
	{
		if(!(isalpha(key[x]) || isdigit(key[x]) )){ //if it is not alpha nor digit
			printf("failed: %c\n", key[x]);
			printf("key: %s\n", key);
			errno = ERR_INVALID_PARAM; //Checking invalid parameters
			return -1;
		}
		x++;
	}x=0;
	while(table[x] != '\0')
	{

		if(!(isalpha(table[x]) || isdigit(table[x]) )){ //if it is not alpha nor digit
			printf("failed: %c\n", table[x]);
			printf("table: %s\n", table);
			errno = ERR_INVALID_PARAM; //Checking invalid parameters
			return -1;
		}
		x++;
	}

	if (Authenticated == 0)
	{
		errno = ERR_NOT_AUTHENTICATED;
		return -1;
	}

	char message[200];

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	strncpy(buf,"",sizeof(buf));
	memset(buf, 0, sizeof buf);
	snprintf(buf, sizeof buf, "GET %s %s\n", table, key);
	printf("buf before: %s \n", buf);
	//LOGGING function
	if(LOGGING == 0)
	{
	}
	else if(LOGGING == 1)
	{
		LOG((buf));
	}
	else if(LOGGING == 2)
	{
		sprintf(message, "%s------------Issue storage_set\n", buf);
		logger(file, message);
	}

	/*** For testing start time ***/
//	gettimeofday(&start_time, NULL);

    //**********Error checking No.4 **********//
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0 && Authenticated == 1)
	{
		/*** For testing end time ***/
//		gettimeofday(&end_time, NULL);
//
//		/*** For calculating the time interval ***/
//		long int time_interval;
//		time_interval = (end_time.tv_sec-start_time.tv_sec)*1000000L+(end_time.tv_usec-start_time.tv_usec);
//		printf("the execution time in microseconds: %ld\n", time_interval);

		//sscanf(buf,"%c", &check);
		printf("buf: %s \n", buf);


		//for memadata checking the value which is returning from the server side
		char* data_match;
		char metadata_receiver[20];
		char bufcopy[100];
		
		strncpy(bufcopy,buf,sizeof(bufcopy));

		data_match = strtok(bufcopy,":"); //get the rest of the line
		printf("the first part %s\n", data_match);
		data_match = strtok(NULL,":");
		printf("the second part %s\n", data_match);
		snprintf(metadata_receiver, sizeof(metadata_receiver), "%s", data_match);
		printf("the counter: %s \n", metadata_receiver);

		printf("buf: %s \n", buf);


		check = buf[0];
        if(check == 'N')
        {
        	errno = ERR_TABLE_NOT_FOUND;
        	return -1;
        }
        else if(check == 'Y')
        {
        	errno = ERR_KEY_NOT_FOUND;
        	return -1;
        }
        else if(check == 'P')
		{
			errno = ERR_INVALID_PARAM;
			return -1;
		}
        else if(check == 'E')
        {
        	errno = ERR_UNKNOWN;
        	return -1;
        }
        else
        {
        	record->metadata[0] = atoi(metadata_receiver);

        	printf("buf: %s \n", buf);
        	strncpy(record->value, buf, sizeof record->value);
        	printf("value: %s \n", record->value);
        }
		return 0;
	}

	return -1;
}

/**
 * @brief This function is for users set the value inside the storage server
 *
 * @param table The name of the table
 * @param key The name of the key
 * @param record The current record in struct storage_record type
 * @param conn This is a void pointer for connection purpose
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
	//**********For error checking ***********//
	//**********Error checking No.1 **********//
//	struct timeval start_time, end_time;
	char check;
	int x = 0;
	if (table == NULL || key == NULL)
	{
		errno = ERR_INVALID_PARAM;
		//printf("1 \n");
		return -1;
	}
	if(conn == NULL)
	{
		errno = ERR_INVALID_PARAM; //ERR_CONNECTION_FAIL? main.c line.517 says otherwise
		return -1;
	}

	while(key[x] != '\0')
	{

		//printf("key: %c\n", key[x]);
		if(!(isalpha(key[x]) || isdigit(key[x]) )){ //if it is not alpha nor digit
			printf("failed: %c\n", key[x]);
			printf("key: %s\n", key);
			errno = ERR_INVALID_PARAM; //Checking invalid parameters
			return -1;
		}
		x++;
	}
	x=0;
	while(table[x] != '\0')
	{

		if(!(isalpha(table[x]) || isdigit(table[x]) ))
		{ //if it is not alpha nor digit
			printf("failed: %c\n", table[x]);
			printf("table: %s\n", table);
			errno = ERR_INVALID_PARAM; //Checking invalid parameters
			return -1;
		}
		x++;
	}

	if (Authenticated == 0)
	{
		errno = ERR_NOT_AUTHENTICATED;
		return -1;
	}


	char message[200];
	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	strncpy(buf,"",sizeof(buf));
	memset(buf, 0, sizeof buf);
	if(record == NULL)
	{
		snprintf(buf, sizeof buf, "SET %s %s 0 %s\n", table, key, "NULL");
	}
	else if(strcmp(record->value,"")==0)
	{
		strncpy(record->value,"NULL",sizeof(record->value));
		//snprintf(buf, sizeof buf, "SET %s %s %d %s\n", table, key, record->metadata, record->value);
		snprintf(buf, sizeof buf, "SET %s %s %lu %s\n", table, key, record->metadata[0], record->value);
	}
	else
	{
		printf("rcd's value: %s \n", record->value);
		//snprintf(buf, sizeof buf, "SET %s %s %d %s\n", table, key, record->metadata, record->value);
		snprintf(buf, sizeof buf, "SET %s %s %lu %s\n", table, key, record->metadata[0], record->value);
	}

	printf("buf before: %s",buf);
	if(LOGGING == 0)
	{}
	else if(LOGGING == 1)
	{
		LOG((buf));
	}
	else if(LOGGING == 2)
	{
		logger(file, buf);
	}

	/*** For testing start time ***/
//	gettimeofday(&start_time, NULL);
	//printf("the send message 1");

	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0 && Authenticated == 1)
	{
	//printf("the message 2");
		/*** For testing end time ***/
//		gettimeofday(&end_time, NULL);

//		/*** For calculating the time interval ***/
//		long int time_interval;
//		time_interval = (end_time.tv_sec-start_time.tv_sec)*1000000L+(end_time.tv_usec-start_time.tv_usec);
//		printf("the execution time in microseconds: %ld\n", time_interval);

		//sscanf(buf, "%c", &check);
		check = buf[0];
		printf("set's buffer: %s\n", buf);
		if(check == 'N')
		{
			errno = ERR_TABLE_NOT_FOUND;
			return -1;
		}
		else if(check == 'P')
		{
			errno = ERR_INVALID_PARAM;
			return -1;
		}
		else if(check == 'T')
		{
			errno = ERR_TRANSACTION_ABORT;
			return -1;
		}
		else if(check == 'E')
		{
		    errno = ERR_UNKNOWN;
		    return -1;
		}
		return 0;
	}
	//Getting the current status (a character) from server side. If we cannot find that table, return the
	//error
	return -1;
}


/**
 * @brief This function is for disconnecting from the server
 *
 * @param conn This is a void pointer for connection purpose
 */
int storage_disconnect(void *conn)
{
	// For error checking
	// Error checking No.1
	if (conn == NULL )
	{
		errno = ERR_INVALID_PARAM;
		return -1;
	}

	char message[200];
	// Cleanup
	int sock = (int)conn;
	close(sock);
	Authenticated = 0;

	if(LOGGING == 0)
	{ }

	else if(LOGGING == 1)
	{
		LOG(("disconnected"));
	}

	else if(LOGGING == 2)
	{
			sprintf(message, "Disconnected -------------Disconnect from server\n");
			logger(file, message);
	}

	return 0;
}


/**
 * @brief This function is for querying databse for keys that fit
 *
 * With the help of this function, the program can get the required data
 * from the server, based on the predicates inputted by the user. 
 * 
 * @param table The table of records array
 * @param predicates The conditions given by the user
 * @param keys An array which makes strings as its elements, and used to match the predicates
 * @param max_keys The number of elements inside the array of string "keys"
 *
 * @return If the function be executed successfully, the number of the matching elements is returned. Otherwise, a value of -1 is returned.
 *
 */
int storage_query(const char *table, const char *predicates, char **keys, const int max_keys, void *conn)
{

	printf("stor maxkeys: %d \n", max_keys);
	//table is table name
	/*
	keys: An array of strings where keys whose records match the specified predicates will be stored.
	max_keys: The number of elements in the keys array.
	conn: A connection to the server.
	*/
	//"numer > 9 ", string[]

	char check;
	int x = 0;
	if (table == NULL || predicates == NULL || keys== NULL)
	{
		errno = ERR_INVALID_PARAM; //Checking invalid parameters
		return -1;
	}

	if(conn == NULL)
	{
		errno = ERR_INVALID_PARAM; //ERR_CONNECTION_FAIL? main.c line.517 says otherwise
		return -1;
	}

	if (Authenticated == 0)
	{
		errno = ERR_NOT_AUTHENTICATED;
		return -1;
	}
	char message[200];

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	//modify predicates
	char tempPred[MAX_STRTYPE_SIZE];
	//printf("pred before: %s", predicates);
	snprintf(tempPred, sizeof(tempPred), "%s", predicates);
	//printf("pred after: %s", predicates);
	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	//printf("place 1 \n");
	ModifyString(tempPred);
	//printf("place 2 \n");
	snprintf(buf, sizeof buf, "QUERY %s %s\n", table, tempPred);
	printf("buf before sending: %s \n", buf);

	//LOGGING function
	if(LOGGING == 0)
	{
	}
	else if(LOGGING == 1)
	{
		LOG((buf));
	}
	else if(LOGGING == 2)
	{
		sprintf(message, "%s------------Issue storage_set\n", buf);
		logger(file, message);
	}

	printf("stor maxkeys: %d \n", max_keys);
	//**********Error checking No.4 **********//
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0 && Authenticated == 1)
	{
		printf("buf: %s \n", buf);
		printf("stor maxkeys: %d \n", max_keys);
		check = buf[0];


		if(check == 'P')
		{
			errno = ERR_INVALID_PARAM;
			return -1;
		}
		else if(check == 'N')
		{
			errno = ERR_TABLE_NOT_FOUND;
			return -1;
		}
		else if(check == 'Y')
		{
			printf("no matching keys \n");
			char EmptyString[] = "";
			keys = EmptyString;
			return 0;
		}
		else if(check == 'E')
		{
			errno = ERR_UNKNOWN;
			return -1;
		}
		printf("stor maxkeys: %d \n", max_keys);
		int matchingkeys = 0;
		int a = 0;
		char *token;
		char tempArray[max_keys][MAX_STRTYPE_SIZE*max_keys*2];
		char temp[MAX_STRTYPE_SIZE*max_keys*2];
		char tempParts[MAX_STRTYPE_SIZE+1];		//The piece of temp[]
		strncpy(temp,buf,sizeof(temp));			//send copy to temp[]


		token=strtok(temp," ");
		ModifyString2(token);
		if(token!=NULL)//if not NULL, there is at least one value
		{
			printf("place 1\n");
			for(a=0;a<max_keys-1;a++)
			{
				printf("place 2\n");
				if(token != NULL)//if not NULL
				{
					matchingkeys++;
					snprintf(tempParts, sizeof(tempParts), "%s", token);
					printf("tempParts: %s \n",tempParts);
					printf("place 3\n");
					strncpy(tempArray[a],tempParts,sizeof(tempArray[a]));
					//strcpy(keys[a],tempParts);
					keys[a] = tempArray[a];
					printf("tempArray[a]: %s \n",tempArray[a]);
					printf("keys: %s \n",keys[a]);
					printf("place 4\n");
				}
				token=strtok(NULL," ");
				printf("place 6\n");
				printf("a: %d \n",a);
				/*
				snprintf(tempParts, sizeof(tempParts), "%s", token);
				if(strcmp(tempParts,"")) //if you didn't copy a null into tempParts
				{
					matchingkeys++;
				}
				 * token=strtok(NULL," ");
				printf("place 2\n");
				if(strcmp(tempParts,""))//if not NULL
				{
					printf("tempParts: %s \n",tempParts);
					matchingkeys++;
					printf("place 3\n");
					//printf("tempParts: %s \n", tempParts);
					strncpy(tempArray[a],tempParts,sizeof(tempArray[a]));
					printf("tempArray[a]: %s \n",tempArray[a]);
					printf("place 4\n");
				}
				printf("place 6\n");
				printf("a: %d \n",a);
				printf("mxkys: %d \n",max_keys);*/
			}//for: loop each max_key
		}//if:not NULL

		//keys = tempArray;
		/*
		printf("keys[0]: %s \n",*keys);
		keys++;
		printf("keys[1]: %s \n",*keys);
		keys++;
		printf("keys[2]: %s \n",*keys);
		keys++;
		printf("keys[3]: %s \n",*keys);
		keys++;
		printf("keys[4]: %s \n",*keys);
		keys++;
		printf("keys[5]: %s \n",*keys);*/

		//checking the number of keys here WILL TYPE CODE WHEN WE HAVE MULTIPLE PREDICATES
		printf("No errors: buffer received is : %s \n", buf);
		//printf("number of matching keys is: %d \n", matchingkeys);

		return matchingkeys;
	}
	return -1;
}

/**
 * @file
 * @brief This file is used to generate a client shell, which is used to connect to
 * the server part.
 * 
 * The client connects to the server, running at SERVERHOST:SERVERPORT
 * and performs a number of storage_* operations. If there are errors,
 * the client exits.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "storage.h"
#include <time.h>
#include <sys/time.h>
#include "utils.h"
#include "logger.h"

// This header file is used to control the logging in both client.c and server.c


/**
 * @brief The maximum city number in census part.
 */
#define MAX_CENSUS_CITY_NUM 692
//This macro is the maximum city number in census part. 
//This macro is used for data array


/**
 * @brief Start a client to interact with the storage server.
 *
 * If connect is successful, the client performs a storage_set/get() on
 * TABLE and KEY and outputs the results on stdout. Finally, it exists
 * after disconnecting from the server.
 *
 * @param argc The number of strings pointed to by argv
 * @param argv[] An array of pointers to characters
 *
 * @return Return 0 when we want to exit the main function
 */

void PrintValue(char* the_label)
{
    printf("value: %s\n", the_label);
}

int main(int argc, char *argv[])
{
	char message[200];
	time_t rawtime; // the input time 
	struct tm * timeinfo;
	char buffer [80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if(LOGGING == 2)
	{
		strftime(buffer, 80, "Client-%F-%H-%M-%S.log", timeinfo);
		file = fopen(buffer, "a+");
	}
	/*int max_keys = 5;
	int matchingkeys = 0;
	int a = 0;
	char *token;
	char buf[MAX_STRTYPE_SIZE] = "hello";
	char tempArray[max_keys][MAX_STRTYPE_SIZE*max_keys*2];
	char temp[MAX_STRTYPE_SIZE*max_keys*2];
	char *tempTEST[MAX_STRTYPE_SIZE];	//The piece of temp[]
	printf("tempTEST: %c \n",*tempTEST[0]);

*/



/*
	char* s = "asd";
	char** p = &s;
	char sentence[]="Rudolph is 12 years old";
	s = sentence;

	PrintValue(s);

	printf("The value of s is: %c\n", *s);
	printf("The direction of s is: %p\n", &s);

	printf("The value of s is: %p\n", s);
	printf("The direction of s is: %p\n", &s);
*/

/*	printf("The value of p is: %p\n", p);
	printf("The direction of p is: %p\n", &p);

	printf("The direction of s[0] is: %p\n", &s[0]);
	printf("The direction of s[1] is: %p\n", &s[1]);
	printf("The direction of s[2] is: %p\n", &s[2]);*/


//START
	/*
	char sentence []="Rudolph is 12 years old";
	char str[20];
	char end[20];
	int i;

	sscanf (sentence,"%s%[^\n]",str,end);
	printf ("sentence: %s \n",str);
	printf ("predi:%sd\n",end);
*/
//END

	char name;
	int status;
	struct storage_record r;
	void *conn;
	int selection1;
	char selection[50];

	int connected = 0, Authenticated = 0;

	printf("> ---------------------\n> 1) Connect \n> 2) Authenticate\n> 3) Get\n> 4) Set\n> 5) Disconnect\n> 6) Exit\n> ---------------------\n");


	do
	{
 		printf("Please enter your selection: ");
		///selection[0] = '\0';
		gets(selection);

  		if(selection[0] == '1' && selection[1] == '\0')
  		{
			char hostname[64];
			int port;
			printf("Please input the hostname:\n");
			scanf("%s", hostname);
			printf("Please input the port:\n");
			scanf("%d", &port);
			getchar();
			conn = storage_connect(hostname, port);
			if(conn == NULL)
				printf("conn is NULL \n");
			if(!conn)
			{
				printf("Cannot connect to server @ %s:%d. Error code: %d.\n",hostname, port, errno);
				return -1;
			}

			//check the network connected or not
			connected = 1;
			Authenticated = 0;

			//cl./server default.confear the string
		}

  		// Authenticate the client.
  		else if(selection[0] == '2' && selection[1] == '\0' && connected == 1)
  		{
	  		char serverusename[64];
	  		char serverpassword[64];
	  		printf("Please input SERVERUSERNAME:\n");
	  		scanf("%s", serverusename);
	  		printf("Please input the SERVERPASSPORT:\n");
	  		scanf("%s", serverpassword);
	  		getchar();
	  		status = storage_auth(serverusename, serverpassword, conn);

  			if(status != 0)
  			{
    				printf("storage_auth failed with username '%s' and password '%s'." \
           			"Error code: %d.\n", serverusename, serverpassword, errno);
    				storage_disconnect(conn);
    				return status;
  			}
  			printf("storage_auth: successful.\n");
  			Authenticated = 1;
  		}

  		// Issue storage_get
  		else if(selection[0] == '3' && selection[1] == '\0' && connected == 1)
  		{
			char table[64];
	  		char key[64];
	  		printf("Please input TABLE:\n");
	  		scanf("%s", table);
	  		printf("Please input the KEY:\n");
	  		scanf("%s", key);
	  		getchar();
  			status = storage_get(table, key, &r, conn);
  			if(status != 0) 
			{
    				printf("storage_get failed. Error code: %d.\n", errno);
    				storage_disconnect(conn);
    				//return status;
  			}
  			printf("metadata is: %lu \n", r.metadata[0]);
  			printf("storage_get: the value returned for key '%s' is '%s'.\n", key, r.value);
		}

		// Issue storage_set
		else if(selection[0] == '4' && selection[1] == '\0' && connected == 1)
		{
			//struct storage_record r;
			char table[64];
			char key[64];
			char value[64];
			int metadata;
			//int temp;
			printf("Please input TABLE:\n");
			scanf("%s", table);
			printf("Please input the KEY:\n");
			scanf("%s", key);

			/*printf("Please input the metadata:\n");
			scanf("%lu", &r.metadata[0]);

			*/
			/*scanf("%d", &metadata);
			printf("metadata is: %d", metadata);
			r.metadata = metadata;
			printf("metadata is: %d", (int)r.metadata);

			printf("%016" PRIxPTR "\n", (uintptr_t)ptr);
			*/
			getchar();
			printf("Please input the VALUE:\n");
			scanf("%[^\n]", value);
			getchar();


			printf("metadata is: %lu \n", r.metadata[0]);

			strncpy(r.value, value, sizeof r.value);//modified
			if(strcmp(r.value,"0")==0)
			{
				status = storage_set(table, key, NULL, conn);
			}
			else
			{
				status = storage_set(table, key, &r, conn);
			}
			if(status != 0) 
			{
				printf("storage_set failed. Error code: %d.\n", errno);
				storage_disconnect(conn);
				//return status;
			}
			printf("status %d \n",status);
		}

		// Disconnect from server
		else if(selection[0] == '5' && selection[1] == '\0' && connected == 1)
		{
			status = storage_disconnect(conn);
			if(status != 0) 
			{
				printf("storage_disconnect failed. Error code: %d.\n", errno);
				return status;
			}
			connected = 0;
			Authenticated = 0;
		}

		else if(selection[0] == '6' && selection[1] == '\0')// do nothing
		{}

		else if(selection[0] == '7' && selection[1] == '\0')
		{
		// For census
		// Here we build one string array and one int array
		// string array is for holding the key, while int array is for holding the value

			char city_key[MAX_CENSUS_CITY_NUM][1024];
			char city_population[MAX_CENSUS_CITY_NUM][1024];
			char *new_city_population;
			int index = 0;

	   		char *record, *line, *token;
	   		char buffer2[1024];

	   		FILE *fstream5 = fopen("census.csv","r");

	   		if(fstream5 == NULL)
	   		{
		  		printf("census.csv opening failed \n");
		  		return -1 ;
	   		}

	  	  	const char s[1] = ",";

	  		int temp = 0;
	  		int counter = 0;
	  		char strtemp[MAX_VALUE_LEN] = "census";

			while((line=fgets(buffer2,sizeof(buffer2),fstream5))!=NULL)
			{
				record = strtok(line, ",");

				if(record != NULL)
				{
					strcpy(city_key[index], record);
					record = strtok(NULL, ",");
					strcpy(city_population[index], record);
					//temp = atoi(record);
					//city_population[index] = temp;
				}
			index++;
			}

			//start timer for workload
			struct timeval start_time, end_time;
			gettimeofday(&start_time, NULL);

			for(; counter<MAX_CENSUS_CITY_NUM; counter++)
			{
				size_t ln = strlen(city_population[counter]) - 1;
				if (city_population[counter][ln] == '\n')
					city_population[counter][ln] = '\0';
				strncpy(r.value, city_population[counter], sizeof r.value);//modified
				//printf("value before : %s \n",r.value);
				status = storage_set(strtemp, city_key[counter], &r, conn);
				//printf("strtemp: %s \n",strtemp);
				printf("citykey: %s \n",city_key[counter]);
				printf("value after : %s \n",r.value);
				//printf("value after : %s \n",r.value);

				if(status != 0) 
				{
					printf("storage_set failed. Error code: %d.\n", errno);
					storage_disconnect(conn);
					return status;
				}
			}

			gettimeofday(&end_time, NULL);
			long int time_interval;
			time_interval = (end_time.tv_sec-start_time.tv_sec)*1000000L+(end_time.tv_usec-start_time.tv_usec);
			printf("The end-to-end execution time is : %ld microseconds \n", time_interval);

  		}
		else if(selection[0] == '8' && selection[1] == '\0')
		{
			char table[64];
			char predicates[MAX_STRTYPE_SIZE];
			char* resulting_keys[MAX_RECORDS_PER_TABLE];
			char CHARmax_keys[MAX_STRTYPE_SIZE];
			int max_keys;
			printf("Please input TABLE:\n");
			scanf("%s", table);
			printf("Please input MAX_KEYS:\n");
			scanf("%s", CHARmax_keys);

			if(atoi(CHARmax_keys) == 0)
			{
				printf("invalid. Please enter an integer \n");
			}else
				max_keys = atoi(CHARmax_keys);

			getchar();
			printf("Please input the predicate(s):\n");
			scanf("%[^\n]", predicates);
			getchar();
			//printf("Predicates: %s \n", predicates);

			//status = storage_query("inttbl", "col > 10", resulting_keys, MAX_RECORDS_PER_TABLE, conn);
			status = storage_query(table, predicates, resulting_keys, max_keys, conn);
			if(status < 0)
			{
					printf("storage_query failed. Error code: %d.\n", errno);
					storage_disconnect(conn);
					//return status;
			}
			printf("There are %d matching key(s) \n", status);
			//printf("storage_query: the value returned for key '%s' is '%s'.\n", key, r.value);
		}
  		else
		{
	  		printf("please give the right input\n");//for other invalid input
		}
	}

	while(selection[0] != '6');//exit

return 0;
}


/* ignore this: this is testing code Marco used for query (I might still need it)
	char predicates[40] = "value<1>0";
	char KeyName[40];
	char QueryValue[40];
	char comparison_sign[40];
	char temp = 'c';
	int n = 0;

	while(temp != '\0')
	{
		temp = predicates[n];
		if(temp == '<')
		{
			strncpy(comparison_sign,"<",sizeof(comparison_sign));
			break;
		}
		else if(temp == '>')
		{
			strncpy(comparison_sign,">",sizeof(comparison_sign));
			break;
		}
		else if(temp == '=')
		{
			strncpy(comparison_sign,"=",sizeof(comparison_sign));
			break;
		}
		n++;
	}//done checking for comparison_sign
	printf("sign is: %s \n", comparison_sign);
	char *token;
	token = strtok(predicates,comparison_sign);
	snprintf(KeyName, sizeof(KeyName), "%s", token);
	token = strtok(NULL,comparison_sign);
	snprintf(QueryValue, sizeof(QueryValue), "%s", token);
	printf("KeyName: %s, QueryValue: %s \n",KeyName,QueryValue);
 * */



/**
 * @file
 * @brief This file implements the storage server.
 *
 * The storage server should be named "server" and should take a single
 * command line argument that refers to the configuration file.
 * 
 * The storage server should be able to communicate with the client
 * library functions declared in storage.h and implemented in storage.c.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include "utils.h"
#include <time.h>
#include <ctype.h>

#include <stdbool.h>  // if using C99...  for C++ leave this out.



#define MAX_CONFIG_LINE_LEN 1024 ///< Max characters in each config file line.
#define MAX_USERNAME_LEN 64	///< Max characters of server username.
#define MAX_ENC_PASSWORD_LEN 64	///< Max characters of server's encrypted password.
#define MAX_HOST_LEN 64		///< Max characters of server hostname.
#define MAX_PORT_LEN 8		///< Max characters of server port.
#define MAX_PATH_LEN 256	///< Max characters of data directory path.

// Storage server constants.
#define MAX_TABLES 100		///< Max tables supported by the server.
#define MAX_RECORDS_PER_TABLE 1000 ///< Max records per table.
#define MAX_TABLE_LEN 20	///< Max characters of a table name.
#define MAX_KEY_LEN 20		///< Max characters of a key name.
#define MAX_CONNECTIONS 10	///< Max simultaneous client connections.

// Extended storage server constants.
#define MAX_COLUMNS_PER_TABLE 10 ///< Max columns per table.
#define MAX_COLNAME_LEN 20	///< Max characters of a column name.
#define MAX_STRTYPE_SIZE 40	///< Max SIZE of string types.
#define MAX_VALUE_LEN 800	///< Max characters of a value.

#define MAX_LISTENQUEUELEN 20	///< The maximum number of queued connections.
#define LOGGING 0
#define MAX_CENSUS_CITY_NUM 693 ///< The maximum city numbers
                                //in census part. This macro is used for data array

/**
 * @brief Check whether a string is actually a valid integer
 *
 * In this function, we handle negative numbers first, then
 * check whether the string is a valid integer or not. If is an integer,
 * return true, otherwise return false
 *
 * @param chararray The string to be tested
 * @return Return true if it is integer
 */
bool TestingCharArray(const char *chararray)
{
   // Handle negative numbers.
   //
   if (*chararray == '-')
   {
      chararray = chararray + 1;
   }
   // Handle empty string or just "-".
   //
   if (!*chararray)
   {
      return false;
   }
   // Check for non-digit chars in the rest of the string.
   //
   while (*chararray)
   {
      if (!isdigit(*chararray))
      {
         return false;
      }
      else
      {
         chararray = chararray + 1;
      }
   }

   return true;
}

void ModifyString3(char* char_source)
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
 * @brief This is the declaration of the struct string,
 * which is used for holding value data inside
 *
 */
typedef struct string{
	char value[MAX_VALUE_LEN];
	char type[20];
	char size[20];
	int counter;				// for each key cell
};


/**
 * @brief This initialization declares a struct,
 * which is used for holding table of data
 *
 */
struct string TableOfData[MAX_TABLES][MAX_RECORDS_PER_TABLE][11];//10+1

/**
 * @brief This initialization declares a struct,
 * which is used for holding query 
 *
 */
struct string QueryHolder[MAX_RECORDS_PER_TABLE][MAX_COLUMNS_PER_TABLE+1]; //holds the keys for comparison


/**
 * @brief This initialization declares a struct,
 * which is used for holding the result
 * of the query
 *
 */
struct string QueryResultHolder[MAX_RECORDS_PER_TABLE][MAX_COLUMNS_PER_TABLE+1]; //holds the result from comparison
//struct timeval total_processing_time{
//	.tv_sec = 0;
//	.tv_usec = 0;
//};

/**
 * @brief This initialization is for counting the total processing time
 */
int long total_processing_time = 0;

/**
 * @brief Process a command from the client.
 *
 * @param file1 The file stream used in this function.
 * @param sock The socket connected to the client.
 * @param cmd The command received from the client.
 * @return Returns 0 on success, -1 otherwise.
 */

struct thread_data{
   int  clientsock;
   struct config_params params;
   FILE *file1;
};



struct thread_data thread_data_array[MAX_CONNECTIONS];


void* handle_client(void *data)
{

	//printf("Hello World! It's me, thread #%ld!\n", tid);
	struct thread_data *my_data;
	my_data = (struct thread_data *)data;
	//copy values from data into my_data

	int clientsock = my_data->clientsock;

	//file1 my_data->file1
	//params

	// Get commands from client.
	int wait_for_commands = 1;
	do {
		// Read a line from the client.
		char cmd[MAX_CMD_LEN];
		int status = recvline(clientsock, cmd, MAX_CMD_LEN);
		printf("cmd after receiving: %s \n", cmd);

		if (status != 0)
		{
			// Either an error occurred or the client closed the connection.
			wait_for_commands = 0;
		}

		else
		{
			// Handle the command from the client.
			//printf("Table before handle: %s \n", TableOfData[0][0][0].value);
			printf("cmd before handle: %s \n", cmd);
			int status = handle_command(my_data->file1, clientsock, cmd, my_data->params);
			if (status != 0)
			{
			wait_for_commands = 0; // Oops.  An error occured.
			printf("failed cmd was %s \n", cmd);
			}
		}
	} while (wait_for_commands);

	pthread_exit(NULL);
	close(clientsock);
}

int handle_command(FILE* file1, int sock, char *cmd, struct config_params params)
{

	printf("beginning of handle command\n");
	//printf("%s \n", Table[0][0][0]);

	char message[200];

	if(LOGGING == 0)
	{}
	else if(LOGGING == 1)
	{
		LOG(("Processing command '%s'\n", cmd)); // replace LOG commands with logger() calls
	}
	else if(LOGGING == 2)
	{
		sprintf(message,"Processing command '%s'\n", cmd);
		logger(file1, message);
	}

	char read_temp;
	char command[20];
	char username[64];
	char password[64];
	char tablename[20];
	char key[50];
	char predicates[MAX_STRTYPE_SIZE];
	char value[800];
	int metadata = 0;
	//char* result = NULL;
	struct timeval start_time, end_time;
	//printf("cmd: %s \n",cmd);
	read_temp = cmd[0];

	if(read_temp == 'A')
	{
		sscanf(cmd, "%s %s %s",command,username,password);

		gettimeofday(&start_time, NULL);//start time

		if((strcmp(params.username, username) == 0) && (strcmp(params.password, password) == 0))
		{
			//////printf("Authenticated \n");
			cmd = "A s";
		}
		else
		{
			cmd = "A u";
		}

		gettimeofday(&end_time, NULL);//end time
	}
	else if(read_temp == 'G')
	{
		sscanf(cmd,"%s %s %s",command,tablename,key);
		gettimeofday(&start_time, NULL);//start time

		int a = 0,b = 0,c = 0,d = 0;
        int found_table = 0;
        int found_key = 0;
		cmd = NULL;

		//ALSO USED IN QUERY
		char space[MAX_STRTYPE_SIZE] = " ";
		char comma[MAX_STRTYPE_SIZE] = ",";
		char temp_cmd[800] = "";	//stores the temporary cmd value
		char temp = 'c';

		for(;c < MAX_TABLES; c++)
		{
			if(strcmp(TableOfData[c][0][0].value, tablename) == 0)
			{
				//////printf("found table \n");
				found_table = 1; //found the table for loop to find specified key
				
				b = 1;
				//loop to find the key in the table
				for(;b < MAX_RECORDS_PER_TABLE; b++)
				{

					if(strcmp(TableOfData[c][b][0].value, key) == 0)
					{
						//found key, loop to check if the column names are correct (?)
						//code here

						//populate temp_cmd with all the information
						a = 1; //don't need the tablename
						while(strcmp(TableOfData[c][0][a].value,"") && a<11) //while not NULL //change to 10+1
						{
							strcat(temp_cmd,TableOfData[c][0][a].value);//store the value name
							strcat(temp_cmd,space); //space
							printf("temp1: %s \n",temp_cmd);
							strcat(temp_cmd,TableOfData[c][b][a].value);//store the value
							strcat(temp_cmd,comma); //finished the "value 90,", now repeat for other values
							printf("temp2: %s \n",temp_cmd);
							a++;//increment a = move to the next column behind it
						}a = 1;

						//adding the counter at the end of string
						char temp_counter[50];
						snprintf(temp_counter, sizeof temp_counter, "%s :%d:", temp_cmd, TableOfData[c][b][0].counter);
						printf("final temp_counter: %s \n", temp_counter);

						/*
						printf("GET TableOfData[0][0][0]: %s \n", TableOfData[0][0][0].value);
						printf("GET TableOfData[0][0][1]: %s \n", TableOfData[0][0][1].value);
						printf("GET TableOfData[0][0][2]: %s \n", TableOfData[0][0][2].value);
						printf("GET TableOfData[0][1][0]: %s \n", TableOfData[0][1][0].value);
						printf("GET TableOfData[0][1][1]: %s \n", TableOfData[0][1][1].value);
						printf("GET TableOfData[0][2][0]: %s \n", TableOfData[0][2][0].value);
						printf("GET TableOfData[0][2][1]: %s \n", TableOfData[0][2][1].value);
*/
						printf("GET temp_cmd: %s \n", temp_cmd);
						cmd = temp_counter; //if key was found, make cmd point to temp_cmd
						//cmd = TableOfData[c][b][1].value;//found key, put information into cmd
						//strncpy(cmd, TableOfData[c][b][1].value, sizeof(cmd));//found key, put information 							//into cmd
						found_key = 1;
						break;
					}
				} // end of key search loop

				if(found_key == 1) //check if key was found
				{
					break;
				}
			}
		}//end of table search loop


		if(found_table == 0)//not found the table
		{
			cmd = NULL;
			cmd = "N";
		}

		//printf("test \n");

		if(found_table == 1 && found_key == 0)//found the table, not found the key
		{
			cmd = NULL;
			cmd = "Y";
		}
		gettimeofday(&end_time, NULL);//end time
	}

	else if(read_temp == 'S')
	{
		printf("metadata: %d \n",metadata);
		sscanf(cmd, "%s %s %s %lu %[^\n]",command,tablename,key,&metadata,value);
		//printf("metadata: %d \n",metadata);
		int c = 0, b = 0, a = 0;
		int InvalidParam = 0;
		int found_table = 0;
		int Transact_failed = 0;
		int key_set = 0;//flag for when we set the key
		char ColumnType = ' ';
		char ColumnSize[10];
		cmd = NULL;

		/*printf("table[0][0][0]: %s \n",TableOfData[0][0][0].value);
		printf("table[1][0][0]: %s \n",TableOfData[1][0][0].value);
		printf("table[2][0][0]: %s \n",TableOfData[2][0][0].value);
		*/
		char space[MAX_STRTYPE_SIZE] = " ";
		char comma[MAX_STRTYPE_SIZE] = ",";
		char temp_cmd[800] = " ";	//stores the temporary cmd value
		char ParsedInputValue[10][MAX_STRTYPE_SIZE];
		char ParsedValuenamesAndValues[10][2][MAX_STRTYPE_SIZE];//[X][0]: keyname, [X][1]:value
		char *token; //used for strtok
		char temp = 'c';

		//initialization
		for(c=0;c<10;c++)
		{
			strncpy(ParsedValuenamesAndValues[c][0],"",sizeof(ParsedValuenamesAndValues[c][0]));
			strncpy(ParsedValuenamesAndValues[c][1],"",sizeof(ParsedValuenamesAndValues[c][1]));
			strncpy(ParsedInputValue[c],"",sizeof(ParsedInputValue[c]));
		}c=0;

		//Error Checking - column names
		if(strcmp(value,"NULL")) //only parse if value is not "NULL"
		{
			//need to parse value several different column names first:
			//Error Checking - column names (breaking into sep. column names)
			token = strtok(value,","); //break according to commas
			while(token != NULL)
			{
				snprintf(ParsedInputValue[c], sizeof(ParsedInputValue[c]), "%s", token);
				//printf("ParsedInputValue[]: %s \n", ParsedInputValue[c]);
				token = strtok(NULL,","); //get the rest of the line
				c++;
			}c = 0; //just in case; Also, ParsedInputValue[] now has all the different values
					//[0]: col 22 [1]: value 33
			token = NULL;

			//need to parse ParsedInputValue[] into ParsedKeynamesAndValues[][]:
			//ASSUMPTION: Keyname is only one string and separated by space

			c=0;
			while(strcmp(ParsedInputValue[c],"") && c<10) //while not NULL and less than 10 times (possible bug: PIV uninitialized)
			{
				sscanf(ParsedInputValue[c],"%s %[^\n]",ParsedValuenamesAndValues[c][0],ParsedValuenamesAndValues[c][1]);
				ModifyString3(ParsedValuenamesAndValues[c][0]);
				ModifyString3(ParsedValuenamesAndValues[c][1]);
				printf("ValueName %s \n",ParsedValuenamesAndValues[c][0]);
				printf("Value %s \n",ParsedValuenamesAndValues[c][1]);
				//printf("into: %s and %s \n", ParsedValuenamesAndValues[c][0],ParsedValuenamesAndValues[c][1]);
				c++;
			}c=0;
			//ParsedKeynamesAndValues is now loaded
		}

		gettimeofday(&start_time, NULL);//start time
		cmd = NULL;

		a=0,b=0,c=0;

		for(;c < MAX_TABLES; c++)
		{
			printf("TableOfData: %d %s\n",c, TableOfData[c][0][0].value);
			printf("tablename %s \n",tablename);
			if(strcmp(TableOfData[c][0][0].value, tablename) == 0) //found table
			{
				found_table = 1;
				//check if column names match
				a = 0;
			//Error Checking - Found Table, Check Columns Immediately
				if(strcmp(value,"NULL"))
				{
					while(a<10) //while not NULL
					{
						printf("index: %d \n",a);
						//printf("ParsedVlNmes: %s dsnt match ToD: %s \n",ParsedValuenamesAndValues[a][0],TableOfData[c][0][a+1].value);
						//if the value name doesn't match up, failed
						if(strcmp(ParsedValuenamesAndValues[a][0],TableOfData[c][0][a+1].value)) //if it doesn't equal
						{
							//printf("index: %d \n",a);
							printf("ParsedVlNmes: %s dsnt match ToD: %s \n",ParsedValuenamesAndValues[a][0],TableOfData[c][0][a+1].value);
							InvalidParam = 1;
							//RETURN FAILED.
						}
						//CHECK TYPE
						if(TestingCharArray(ParsedValuenamesAndValues[a][1]) ) //if it's an int
						{
							ColumnType = TableOfData[c][0][a+1].type[0];
							if(ColumnType == 'i')
							{
							  char *pointer;
							  pointer = (char*)memchr(ParsedValuenamesAndValues[a][1], '.', strlen(ParsedValuenamesAndValues[a][1]));
							  if (pointer!=NULL)
								InvalidParam = 1;
							}
							else
							{
								printf("type failed. table: %s, but trying to enter %s\n",
								TableOfData[c][0][a+1].type, ParsedValuenamesAndValues[a][1]);
								InvalidParam = 1;
							}
						}
						else	//not an int, input data is char
						{
							b=0;
							while(ParsedValuenamesAndValues[a][1][b] != '\0') //check if its alphanumeric
							{
								if(isalpha(ParsedValuenamesAndValues[a][1][b]) || isspace(ParsedValuenamesAndValues[a][1][b])){ //if it is not alpha nor digit
								}
								else
								{
									printf("not alpha and not space \n");
									InvalidParam = 1;
								}
								b++;
							}b=0;

							if(strlen(ParsedValuenamesAndValues[a][1]) > atoi(TableOfData[c][0][a+1].size))//check size
							{
								printf("size failed. table: %s, but trying to enter %s\n",
									TableOfData[c][0][a+1].size, ParsedValuenamesAndValues[a][1]);
								InvalidParam = 1;
							}

						}

						char ColumnType = ' ';
						char ColumnSize[10];
						a++;//increment a = move to the next column behind it
					}a=0;
				}

				printf("passed: commence setting \n");

				b = 1;//want to skip first cell; find the key
				//checking if setting or deleting
				if(strcmp(value, "NULL") == 0) //deleting
				{
					printf("deleting \n");
					for(;b < MAX_RECORDS_PER_TABLE; b++)//b = 1 already
					{
						//check if deleting or setting
						if(strcmp(TableOfData[c][b][0].value, key) == 0) //found key @ [c][b][0]
						{
							a = 0;
							while(a < 10) //while not NULL
							{
								//set all entries to null
								strncpy(TableOfData[c][b][a].value,"",sizeof(TableOfData[c][b][a].value));//store the value
								//remove TableOfData's counter here
								a++;//increment a = move to the next column behind it
							}a = 0;
							cmd = "S tf kf";
							break;
						}//if:found key
					}//for loop: end of finding key
					b=1;
				}
				else							//not deleting, just setting value
				{
					//printf("setting %s and %s \n", ParsedValuenamesAndValues[0][0], ParsedValuenamesAndValues[0][1]);
					b = 1;
					//for loop until found
					for(;b < MAX_RECORDS_PER_TABLE; b++)//b = 1 already
					{
						if(strcmp(TableOfData[c][b][0].value, key) == 0) //found key; same operation
						{
							//set all values behind [c][b][0]
							key_set = 1;
							a = 0;
							//check if metadata and counter match here

							if(metadata == 0 || metadata == TableOfData[c][b][0].counter)
							{
								//SUCCESS
								//while there's a valuename inside ParsedValuenamesAndValues[][]
		/*OVER HERE*/			while(strcmp(ParsedValuenamesAndValues[a][0],"") && InvalidParam == 0 &&a < 11)
								{
									strncpy(TableOfData[c][b][a+1].value,ParsedValuenamesAndValues[a][1],sizeof(TableOfData[c][b][a+1].value));
									printf("SET TableOfData: %s \n", TableOfData[c][b][a+1].value);
									printf("SET ParsedValuenamesAndValues: %s \n", ParsedValuenamesAndValues[a][1]);
									a++;//increment a = move to the next column behind it
								}a=0;
								TableOfData[c][b][0].counter++;
								cmd = "S tf kf";
							}
							else
							{
								printf("metadata: %d doesnt match %d",metadata,TableOfData[c][b][0].counter);
								Transact_failed = 1;
							}
							break;

						}//if: if  found key
					}//for loop: finding key
					b=1;
					if(key_set == 0) //couldn't find key, for loop until first null entry
					{
						//printf("couldn't find key \n");
						b = 1;
						for(;b < MAX_RECORDS_PER_TABLE; b++)//b = 1 already
						{
							if(strcmp(TableOfData[c][b][0].value, "") == 0)	//check if key is null
							{
								//set all values behind [c][b][0]
								//check column name HERE (BEFORE)
								//set the key
								a = 0;
								strncpy(TableOfData[c][b][a].value,key,sizeof(TableOfData[c][b][a].value));
								printf("TableOfData: %s \n", TableOfData[c][b][a].value);
								printf("ParsedValuenamesAndValues: %s \n", ParsedValuenamesAndValues[a][0]);

								//while not NULL
	/*OVER HERE*/				while(strcmp(ParsedValuenamesAndValues[a][0],"") && InvalidParam == 0 && a<10)
								{
									//copy in value
									strncpy(TableOfData[c][b][a+1].value,ParsedValuenamesAndValues[a][1],sizeof(TableOfData[c][b][a+1].value));
									printf("SET TableOfData[0][1][0]: %s \n", TableOfData[0][1][0].value);
									printf("SET TableOfData[0][1][1]: %s \n", TableOfData[0][1][1].value);
									printf("SET TableOfData[0][1][2]: %s \n", TableOfData[0][1][2].value);
									printf("SET TableOfData[0][1][3]: %s \n", TableOfData[0][1][3].value);
									printf("SET TableOfData[0][1][4]: %s \n", TableOfData[0][1][4].value);

									/*printf("SET ParsedValue[0][0]: %s \n", ParsedValuenamesAndValues[0][0]);
									printf("SET ParsedValue[0][1]: %s \n", ParsedValuenamesAndValues[0][1]);
									printf("SET ParsedValue[1][0]: %s \n", ParsedValuenamesAndValues[1][0]);
									printf("SET ParsedValue[1][1]: %s \n", ParsedValuenamesAndValues[1][1]);
									printf("SET ParsedValue[2][0]: %s \n", ParsedValuenamesAndValues[2][0]);
									printf("SET ParsedValue[2][1]: %s \n", ParsedValuenamesAndValues[2][1]);
									printf("SET ParsedValue[2][0]: %s \n", ParsedValuenamesAndValues[3][0]);
									printf("SET ParsedValue[2][1]: %s \n", ParsedValuenamesAndValues[3][1]);*/
									a++;//increment a = move to the next column behind it
								}a=0;
								key_set = 1;
								cmd = "S tf kf";
								break;
							}//if: TableOfData[c][b][0] value is null
						}//for loop: end of finding key
					}//if:key_set was 0
					if(key_set == 0)
						printf("Too many entries, table full \n");
					//if(b<3)
					//	printf("key and value: %s and %s \n", ParsedValuenamesAndValues[0][0],ParsedValuenamesAndValues[0][1]);
					if(InvalidParam == 1 || Transact_failed == 1)
					{
						//already tried to set, but params didn't match up, so break
						//OR
						//transaction failed already
						break;
					}
				}//if: deleting or setting
				if(cmd != NULL || InvalidParam == 1)//if invalid parameters, break
				//if cmd already has a success flag, break out of the table find loop
				{
					break;
				}
			}//if: end of table found
		}//end of table find for loop

		if(Transact_failed == 1)
		{
			cmd = "T";
		}
		else if(InvalidParam == 1)
		{
			cmd = "P";
		}
		else if(found_table == 0)//not found the table
		{
			cmd = "N"; //cmd = "S tn"; //table not found, key not found
		}
		gettimeofday(&end_time, NULL);//end time
	}

	else if(read_temp == 'D')
	{
		gettimeofday(&start_time, NULL);//start time
		//disconnect doesn't need message
		cmd = "D";
		gettimeofday(&end_time, NULL);//end time
	}
	else if(read_temp == 'Q')
	{
		//break down
		sscanf(cmd, "%s %s %s",command,tablename,predicates);
		int InvalidParam = 0;
		int c = 0, b = 0, a = 0, p = 0, q = 0;
		int found_table = 0;
		int found_key = 0;//don't need this
		int index_of_Columnname = 0;
		int ValueNameCounter = 0;
		cmd = NULL;
		char space[MAX_STRTYPE_SIZE] = " ";
		char temp_cmd[800] = " ";	//stores the temporary cmd value

		//predicate variables
		char PredColumName[MAX_STRTYPE_SIZE];
		char comparison_sign[MAX_STRTYPE_SIZE];

		//struct string QueryHolder[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN]; //holds the keys for comparison
		//struct string QueryResultHolder[MAX_RECORDS_PER_TABLE][MAX_KEY_LEN]; //holds the result from comparison

		char QueryValue[MAX_STRTYPE_SIZE];
		char PredicatesArray[MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN];//only 10 possible columns
		int QueryValueINT;
		char *token; //used for strtok
		char temp = 'c';
		//int NumOfMatchingKeys = 0;

		//separate predicates into smaller predicates
		char predicatesCopy[MAX_STRTYPE_SIZE];
		strncpy(predicatesCopy,predicates,sizeof(predicatesCopy));
		token = strtok(predicatesCopy,","); //break according to commas

		c = 0;
		while(token != NULL)
		{
			snprintf(PredicatesArray[c], sizeof(PredicatesArray[c]), "%s", token);
			printf("PredicatesArray[]: %s \n", PredicatesArray[c]);
			token = strtok(NULL,","); //get the rest of the line
			c++;
		}c = 0; //just in case; Also, PredicatesArray[] now has all the different keynames
		token = NULL;

		//find the table first, then check the comparison sign
		for(;c < MAX_TABLES; c++)
		{
			if(strcmp(TableOfData[c][0][0].value, tablename) == 0)
			{
				printf("table_name TableOfData[%d][0][0].value %s\n", c, TableOfData[c][0][0].value);
				printf("tablename_for_input: %s\n", tablename);
				found_table = 1; //found the table for loop to find specified key

				//Copy table into QueryHolder
				b = 1; //avoid the tablename
				a = 0;
				for(;b<MAX_RECORDS_PER_TABLE;b++)
				{
					for(;a<MAX_COLUMNS_PER_TABLE;a++)
					{
						strncpy(QueryHolder[b][a].value, TableOfData[c][b][a].value, sizeof(QueryHolder[b][a].value));
					}a = 0;
				}b=0;//copied into QueryHolder (use a and b)


				p = 0;
				a = 1;
				q = 0;
				int d = 1;//used to track the index of Result
				while(strcmp(PredicatesArray[p],"")) //only parse the predicates if it is not NULL
				{
					printf("parsing %s \n",PredicatesArray[p]);
					while(temp != '\0')
					{
						temp = PredicatesArray[p][q];
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
						q++;
					}//done checking for comparison_sign
					q = 0; //just in case
					token = strtok(PredicatesArray[p],comparison_sign); //break predicate according to comparison sign
					snprintf(PredColumName, sizeof(PredColumName), "%s", token);
					token = strtok(NULL,comparison_sign);
					snprintf(QueryValue, sizeof(QueryValue), "%s", token);

					printf("QueryValue: %s",QueryValue);
					printf("place1\n");
					index_of_Columnname = 0;
					for(ValueNameCounter = 0;ValueNameCounter<MAX_COLUMNS_PER_TABLE;ValueNameCounter++)
					{
						if(strcmp(PredColumName,TableOfData[c][0][ValueNameCounter].value)==0)
							index_of_Columnname = ValueNameCounter;
					}
					printf("place2\n");

					if(index_of_Columnname == 0)
					{
						printf("column wasn't found %d \n", index_of_Columnname);
					}
					printf("place3\n");

					if(strcmp(comparison_sign,"<") == 0)
					{

						printf("place4\n");

						if(!TestingCharArray(QueryValue))
						{
							InvalidParam = 1;
							break;
						}
						//check if the QueryValue is an int

						QueryValueINT = atoi(QueryValue); //change requested value to int
						int e = 1;
						for(;e < MAX_RECORDS_PER_TABLE; e++)
						{
							if(strcmp(QueryHolder[e][index_of_Columnname].value,"") != 0 && atoi(QueryHolder[e][index_of_Columnname].value) < QueryValueINT)
							{
								printf("<: %s is less than %d\n", QueryHolder[e][0].value, QueryValueINT);
								int QueryCopyCounter = 0;
								for(;QueryCopyCounter<MAX_COLUMNS_PER_TABLE;QueryCopyCounter++)
								{
									strncpy(QueryResultHolder[d][QueryCopyCounter].value,QueryHolder[e][QueryCopyCounter].value,sizeof(QueryResultHolder[d][QueryCopyCounter].value));
								}
								/*strncpy(QueryResultHolder[d][0].value,QueryHolder[e][0].value,sizeof(QueryResultHolder[d][0].value));
								strncpy(QueryResultHolder[d][1].value,QueryHolder[e][1].value,sizeof(QueryResultHolder[d][1].value));*/
								//strcat(temp_cmd, TableOfData[c][b][0].value);	//if we find it, write the keyname to cmd
								//strcat(temp_cmd, space);
								d++;
								//found_key = 1;
							}
						}
					}
					else if(strcmp(comparison_sign,">") == 0)
					{
						printf("place5\n");
						if(!TestingCharArray(QueryValue))
						{
							InvalidParam = 1;
							break;
						}
						QueryValueINT = atoi(QueryValue); //change requested value to int
						int e = 1;
						for(;e < MAX_RECORDS_PER_TABLE; e++)
						{
							if(strcmp(QueryHolder[e][index_of_Columnname].value,"") != 0 && atoi(QueryHolder[e][index_of_Columnname].value) > QueryValueINT)
							{
								printf(">: %s is greater than %d\n", QueryHolder[e][0].value, QueryValueINT);
								int QueryCopyCounter = 0;
								for(;QueryCopyCounter<MAX_COLUMNS_PER_TABLE;QueryCopyCounter++)
								{
									strncpy(QueryResultHolder[d][QueryCopyCounter].value,QueryHolder[e][QueryCopyCounter].value,sizeof(QueryResultHolder[d][QueryCopyCounter].value));
								}
								/*strncpy(QueryResultHolder[d][0].value,QueryHolder[e][0].value,sizeof(QueryResultHolder[d][0].value));
								strncpy(QueryResultHolder[d][1].value,QueryHolder[e][1].value,sizeof(QueryResultHolder[d][1].value));*/
								//printf("Holder0: %s, and %s \n", QueryHolder[e][0].value, QueryHolder[e][1].value);
								//printf("ResultHolder: %s and %s \n", QueryResultHolder[e][0].value, QueryResultHolder[e][1].value);
								d++;
								//strcat(temp_cmd, TableOfData[c][b][0].value);	//if we find it, write the keyname to cmd
								//strcat(temp_cmd, space);
								//found_key = 1;
							}
						}
					}
					else if(strcmp(comparison_sign,"=") == 0)
					{
						printf("place6\n");

						//check if QueryValue is an int
						if(TestingCharArray(QueryValue))
						{
							printf("place7\n");

							QueryValueINT = atoi(QueryValue); //change requested value to int
							int e = 1;
							for(;e < MAX_RECORDS_PER_TABLE; e++)
							{
								if(strcmp(QueryHolder[e][index_of_Columnname].value,"") != 0 && atoi(QueryHolder[e][index_of_Columnname].value) == QueryValueINT)
								{
									printf("=: %s is equal to %d\n", QueryHolder[e][0].value, QueryValueINT);
									int QueryCopyCounter = 0;
									for(;QueryCopyCounter<MAX_COLUMNS_PER_TABLE;QueryCopyCounter++)
									{
										strncpy(QueryResultHolder[d][QueryCopyCounter].value,QueryHolder[e][QueryCopyCounter].value,sizeof(QueryResultHolder[d][QueryCopyCounter].value));
									}
									d++;
									//strcat(temp_cmd, TableOfData[c][b][0].value);	//if we find it, write the keyname to cmd
									//strcat(temp_cmd, space);
									//found_key = 1;
								}
							}

						}
						else	//not an int, use string comparisons
						{
							printf("place8\n");

							int e = 1;
							for(;e < MAX_RECORDS_PER_TABLE; e++)
							{
								if(e<5)
								{
									printf("index: %d \n", e);
									printf("QueryHolder: %s \n", QueryHolder[e][1].value);
									printf("QueryValue: %s \n", QueryValue);
								}

								if(strcmp(QueryHolder[e][index_of_Columnname].value, QueryValue) == 0)
								{
									int QueryCopyCounter = 0;
									for(;QueryCopyCounter<MAX_COLUMNS_PER_TABLE;QueryCopyCounter++)
									{
										strncpy(QueryResultHolder[d][QueryCopyCounter].value,QueryHolder[e][QueryCopyCounter].value,sizeof(QueryResultHolder[d][QueryCopyCounter].value));
									}
									/*strncpy(QueryResultHolder[d][0].value,QueryHolder[e][0].value,sizeof(QueryResultHolder[d][0].value));
									strncpy(QueryResultHolder[d][1].value,QueryHolder[e][1].value,sizeof(QueryResultHolder[d][1].value));
									printf("match, QueryResultHolder: %s \n", QueryResultHolder[d][0].value);
									printf("match, QueryResultHolder: %s \n", QueryResultHolder[d][1].value);*/
									d++;
									//strcat(temp_cmd, TableOfData[c][b][0].value);	//if we find it, write the keyname to cmd
									//strcat(temp_cmd, space);
									//found_key = 1;
								}
							}
						}
					}//end of = check
					else
					{
						printf("Bad comparison sign \n");
						printf("comparison sign is: %s \n", comparison_sign);
					}
					printf("place9\n");

				//done, check around QueryHolder
					//clear QueryHolder
					b = 1; //avoid the tablename
					a = 0;
					for(;b<MAX_RECORDS_PER_TABLE;b++)
					{
						a=0;
						for(;a<MAX_COLUMNS_PER_TABLE;a++)
						{
							strncpy(QueryHolder[b][a].value,"", sizeof(QueryHolder[b][a].value));
						}
					}//copied into QueryHolder (use a and b)

					//copy QueryResultHolder into QueryHolder
					b = 1; //avoid the tablename
					a = 0;
					for(;b<MAX_RECORDS_PER_TABLE;b++)
					{
						a=0;
						for(;a<MAX_COLUMNS_PER_TABLE;a++)
						{
							strncpy(QueryHolder[b][a].value,QueryResultHolder[b][a].value, sizeof(QueryHolder[b][a].value));
						}
						/*if(b < 4)
						{
							printf("Holder0: %s, and %s \n", QueryHolder[b][0].value, QueryHolder[b][1].value);
							printf("ResultHolder: %s and %s. Index: %d \n", QueryResultHolder[b][0].value, QueryResultHolder[b][1].value, b);
						}*/
					}//copied into QueryHolder (use a and b)

					//clear ResultHolder
					b = 1; //avoid the tablename
					a = 0; //INCLUDE ANOTHER LOOP TO COPY ALL VALUES
					for(;b<MAX_RECORDS_PER_TABLE;b++)
					{

						a=0;
						for(;a<MAX_COLUMNS_PER_TABLE;a++)
						{
							strncpy(QueryResultHolder[b][a].value,"", sizeof(QueryResultHolder[b][a].value));
						}

						/*if(b < 4)
						{
							printf("Holder0: %s, and %s \n", QueryHolder[b][0].value, QueryHolder[b][1].value);
							printf("ResultHolder: %s and %s. Index: %d \n", QueryResultHolder[b][0].value, QueryResultHolder[b][1].value, b);
						}*/
					}//copied into QueryResultHolder (use a and b)
					d = 1;//reset resultholder index
					p++; //move onto the next predicate
				}//end of while loop for predicates array





				if(InvalidParam == 1)
				{
					//already tried to set, but params didn't match up, so break
					break;
				}
				printf("place13\n");


				//error checking

				if(b < 4)
				{
					printf("Holder0: %s, and %s \n", QueryHolder[b][0].value, QueryHolder[b][1].value);
					printf("ResultHolder: %s and %s. Index: %d \n", QueryResultHolder[b][0].value, QueryResultHolder[b][1].value, b);
				}

				//copy result holder into cmd



				b = 1;
				printf("QueryHolder(outside): %s \n",QueryHolder[b][0].value);
				while(strcmp(QueryHolder[b][0].value,"")) //while not NULL
				{
					found_key = 1;
					//strncpy(QueryHolder[b][0].value,QueryResultHolder[b][0].value, sizeof(QueryHolder[b][0].value));
					strcat(temp_cmd,QueryHolder[b][0].value); //key name
					printf("QueryHolder[b][0].value: %s \n",QueryHolder[b][0].value);
					strcat(temp_cmd,space);
					b++;
				}cmd = temp_cmd;
				b = 0;
				while(strcmp(PredicatesArray[b],"")) //while not NULL
				{
					strncpy(PredicatesArray[b],"",sizeof(PredicatesArray[b]));
					b++;
				}
			}//end of found table if statement
		}//end of table search loop


		if(InvalidParam == 1)
		{
				cmd = "P";
		}

		//DONE QUERYING, FINAL CHECK IF FOUND TABLE TO ALTER CMD BEFORE RETURN
		else if(found_table == 0)//not found the table
		{
			cmd = NULL;
			cmd = "N";
		}

		//printf("test \n");

		else if(found_table == 1 && found_key == 0)//found the table, not found the key
		{
			cmd = NULL;
			cmd = "Y";
		}
		gettimeofday(&end_time, NULL);//end time
	}
	else
	{
		gettimeofday(&start_time, NULL);//start time
		//cmd = "E";
		//printf("werid errors");
		gettimeofday(&end_time, NULL);//end time
	}

	long int time_interval;
	time_interval = (end_time.tv_sec-start_time.tv_sec)*1000000L+(end_time.tv_usec-start_time.tv_usec);
	total_processing_time  += time_interval;
	//printf("The server execution time is : %ld microseconds \n", time_interval);

	printf("cmd is: %s \n", cmd);
	sendall(sock, cmd, strlen(cmd));
	sendall(sock, "\n", 1);
	cmd = NULL;
	return 0;
}


/**
 * @brief Start the storage server.
 *
 * This is the main entry point for the storage server.  It reads the
 * configuration file, starts listening on a port, and proccesses
 * commands from clients.
 *
 * @param argc The number of strings pointed to by argv
 * @param argv[] An array of pointers to characters
 * @return Return EXIT_SUCCESS if we can exit successfully
 */
int main(int argc, char *argv[])
{
	//thread setup

	pthread_t ThreadArray[MAX_CONNECTIONS];
	int threadcounter = 0;
	int threadstatus = 0;
	int MDataCounter = 0;

	char message[200];
	FILE *file1;

	// the input time
	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	if(LOGGING == 2)
	{
		strftime(buffer, 80, "Server-%F-%H-%M-%S.log", timeinfo);
		puts(buffer);
		file1 = fopen(buffer, "a+");
	}

	// Process command line arguments.
	// This program expects exactly one argument: the config file name.
	assert(argc > 0);
	if (argc != 2) {
		printf("Usage %s <config_file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *config_file = argv[1];

	// Read the config file.
	struct config_params params =
    	{
    	.server_host = "",
    	.server_port = -1,
    	.concurrency = -1,
		.username = "",
		.password = "",
	};

	int c = 0;
	for(;c<MAX_TABLES;c++)
	{
		//printf("initialize: %s", params.table[c].tablename);
		strncpy(params.table[c].tablename, "", sizeof(params.table[c].tablename));

		//clear the information of per_table
		int column_index_per_table = 0;
		for(;column_index_per_table < MAX_COLUMNS_PER_TABLE; column_index_per_table++)
		{
			int index_per_column = 0;
			for(; index_per_column < 2; index_per_column++)
			{
				strcpy(params.table[c].column_data[column_index_per_table][index_per_column].columnvalue, "");
			}
		}
	}

	//////printf("before read conf \n");
	int status = read_config(config_file, &params);
	if (status != 0) {
		printf("Error processing config file.\n");
		exit(EXIT_FAILURE);
	}

	//initialize this table with table names in param
	c = 0;
	int column_index_per_table = 0;
	int index_of_table_name = 1;

	for(c = 0;c < MAX_TABLES; c++)
	{
		if(!strcmp(params.table[c].tablename,"") ==0)
		{
			//tablename
			strncpy(TableOfData[c][0][0].value, params.table[c].tablename, sizeof(TableOfData[c][0][0].value));
			//printf("table name: %s \n", TableOfData[c][0][0].value);
			//table information
			column_index_per_table = 0;
			index_of_table_name = 1;
			for(;column_index_per_table < MAX_COLUMNS_PER_TABLE; column_index_per_table++, index_of_table_name++)
			{
				strcpy(TableOfData[c][0][index_of_table_name].value, params.table[c].column_data[column_index_per_table][0].columnvalue);
				strcpy(TableOfData[c][0][index_of_table_name].type,params.table[c].column_data[column_index_per_table][1].columnvalue);
				strcpy(TableOfData[c][0][index_of_table_name].size,params.table[c].column_data[column_index_per_table][2].columnvalue);


				/*printf("index: %d \n", index_of_table_name);
				printf("table_value: %s \n", TableOfData[c][0][index_of_table_name].value);
				printf("table_type: %s \n", TableOfData[c][0][index_of_table_name].type);
				printf("table_size: %s \n", TableOfData[c][0][index_of_table_name].size);
				printf("--------------------------\n");*/
			}
		}
	}

	//initializing counters
	for(c = 0; c< MAX_TABLES; c++)
	{
		for(MDataCounter = 1; MDataCounter < MAX_RECORDS_PER_TABLE; MDataCounter++)
		{
			TableOfData[c][MDataCounter][0].counter = 1;
		}
	}

	if(LOGGING == 0)
	{}
	else if(LOGGING == 1)
	{
		LOG(("Server on %s:%d\n", params.server_host, params.server_port));
	}
	else if(LOGGING == 2)
	{
		sprintf(message, "Server on %s:%d\n", params.server_host, params.server_port);
		logger(file1, message);
	}

	// Create a socket.
	int listensock = socket(PF_INET, SOCK_STREAM, 0);
	if (listensock < 0) {
		printf("Error creating socket.\n");
		exit(EXIT_FAILURE);
	}

	// Allow listening port to be reused if defunct.
	int yes = 1;
	status = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
	if (status != 0) {
		printf("Error configuring socket.\n");
		exit(EXIT_FAILURE);
	}

	// Bind it to the listening port.
	struct sockaddr_in listenaddr;
	memset(&listenaddr, 0, sizeof listenaddr);
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(params.server_port);
	inet_pton(AF_INET, params.server_host, &(listenaddr.sin_addr)); // bind to local IP address
	status = bind(listensock, (struct sockaddr*) &listenaddr, sizeof listenaddr);
	if (status != 0) {
		printf("Error binding socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen for connections.
	status = listen(listensock, MAX_LISTENQUEUELEN);
	if (status != 0) {
		printf("Error listening on socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen loop.
	int wait_for_connections = 1;
	while (wait_for_connections) {
		// Wait for a connection.
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof clientaddr;
		int clientsock = accept(listensock, (struct sockaddr*)&clientaddr, &clientaddrlen);

		if (clientsock < 0 || params.concurrency == 2 || params.concurrency == 3) {
			printf("Error accepting a connection.\n");
			exit(EXIT_FAILURE);
		}
		else if(params.concurrency == 1)
		{

			thread_data_array[threadcounter].clientsock = clientsock;
			thread_data_array[threadcounter].file1 = file1;
			thread_data_array[threadcounter].params = params;


			threadstatus = pthread_create(&ThreadArray[threadcounter], NULL, handle_client, (void *) &thread_data_array[threadcounter]);
			if (threadstatus){
				printf("ERROR; return code from pthread_create() is %d\n", threadstatus);
				exit(-1);
			}else
			{
				threadcounter++;
			}
		}



		if(LOGGING == 0)
		{}

		else if(LOGGING == 1)
		{
			LOG(("Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port));
		}

		else if(LOGGING == 2)
		{
			sprintf(message, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
			logger(file1, message);
		}

		if(params.concurrency == 0)
		{
			// Get commands from client.
			int wait_for_commands = 1;
			do {
				// Read a line from the client.
				char cmd[MAX_CMD_LEN];
				int status = recvline(clientsock, cmd, MAX_CMD_LEN);
				printf("cmd after receiving: %s \n", cmd);

				if (status != 0)
				{
					// Either an error occurred or the client closed the connection.
					wait_for_commands = 0;
				}
				else
				{
					// Handle the command from the client.
					//printf("Table before handle: %s \n", TableOfData[0][0][0].value);
					printf("cmd before handle: %s \n", cmd);
					int status = handle_command(file1, clientsock, cmd, params);
					if (status != 0)
					{
					wait_for_commands = 0; // Oops.  An error occured.
					printf("failed cmd was %s \n", cmd);
					}
				}
			} while (wait_for_commands);

		close(clientsock);
		}//CONCURRENCY == 0



		printf("The total server execution time is : %ld microseconds \n", total_processing_time);
		// Close the connection with the client.


		if(LOGGING == 0)
		{}
		else if(LOGGING == 1)
		{
			LOG(("Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port));
		}
		else if(LOGGING == 2)
		{
				sprintf(message, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
				logger(file1, message);
		}
	}

	// Stop listening for connections.

	close(listensock);

	fclose(file1);
	return EXIT_SUCCESS;
}


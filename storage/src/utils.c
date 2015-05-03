/**
 * @file
 * @brief This file implements various utility functions that are
 * can be used by the storage server and client library. 
 *
 * Most of the functions in this file have different utilities, while
 * can be used by server side or client side in order to connect with
 * each other, send data, read configuration file, process
 * configuration file and generate encrypted password for security
 * purpose
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include "utils.h"


/**
 * @brief This function is used for removing spaces
 * 
 * When a string be passed inside this function ( 
 * pass by parameter), the spaces inside this string
 * will be get removed.
 *
 * @param char_source The string which contain spaces about to be removed
 */
void ModifyString2(char* char_source)
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
 * @brief This function is used for testing the validity of the line
 * 
 * This function is used for testing whether a string has a right format
 *
 * @param char_array_tested The string to be tested
 */
int TestingStringCorrectness(char* char_array_tested)
{
	int x = 0;
	while(char_array_tested[x] != '\0')
	{
		if(!(isalpha(char_array_tested[x]) || isdigit(char_array_tested[x]) || char_array_tested[x] == ':' || char_array_tested[x] == ','
				|| char_array_tested[x] == '[' || char_array_tested[x] == ']')){ //if it is not alpha nor digit
			//printf("char_array_tested: %s\n", char_array_tested);
			return 1;
		}
		x++;
	}return 0;
}


/**
 * @brief This function is used for sending data through the socket
 * 
 *
 * @param sock The socket number
 * @param buf The buf for characters
 * @param len The length of the characters with type const size_t
 */
int sendall(const int sock, const char *buf, const size_t len)
{
	size_t tosend = len;
	while (tosend > 0) 
	{
		ssize_t bytes = send(sock, buf, tosend, 0);
		if (bytes <= 0) 
		{
			break; // send() was not successful, so stop.
		}
		tosend -= (size_t) bytes;
		buf += bytes;
	};

	return tosend == 0 ? 0 : -1;
}


/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise
 *
 * @param sock The socket which will be used
 * @param buf A buffer for storing string inside
 * @param buflen The length of the buffer
 */
int recvline(const int sock, char *buf, const size_t buflen)
{
	int status = 0; // Return status.
	size_t bufleft = buflen;

	while (bufleft > 1) {
		// Read one byte from scoket.
		ssize_t bytes = recv(sock, buf, 1, 0);
		if (bytes <= 0) {
			// recv() was not successful, so stop.
			status = -1;
			break;
		} else if (*buf == '\n') {
			// Found end of line, so stop.
			*buf = 0; // Replac e end of line with a null terminator.
			status = 0;
			break;
		} else {
			// Keep going.
			bufleft -= 1;
			buf += 1;
		}
	}
	*buf = 0; // add null terminator in case it's not already there.

	return status;
}


/**
 * @brief Parse and process a line in the config file.
 *
 * @param line Containing a line of strings
 * @param params Containing the data taken from configuration file
 */
int process_config_line(char *line, struct config_params *params)
{
	///char ColumnNameArray[MAX_COLNAME_LEN*MAX_COLUMNS_PER_TABLE]; ///For storing each column name inside
	///char ColumnTypeArray[MAX_COLNAME_LEN*MAX_COLUMNS_PER_TABLE]; ///For storing each column type inside

	if (line[0] == CONFIG_COMMENT_CHAR)
		return 0;

	// Extract config parameter name and value.
	char name[MAX_CONFIG_LINE_LEN];
	char value[MAX_CONFIG_LINE_LEN];
	char column[MAX_CONFIG_LINE_LEN]; //This line is for storing the column data

	char ColumnArray[MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN];//only 10 possible columns, this store each column (name + type)

	//Then we will parse this column here
	int items = sscanf(line, "%s %s %[^\n]", name, value, column);
	


		

	// Line wasn't as expected.
	if (items < 0)
	{
		return -1;
	}

	// Process this line.
	if (strcmp(name, "server_host") == 0)
	{
		//check if params->server_host is NULL

		if(strcmp(params->server_host, "") == 0)//check first_time get the comnfig file
		{
			strncpy(params->server_host, value, sizeof params->server_host);
		}
		else
		{
			//return error message
			printf("duplicated configuration parameters \n");
			return -1;
		}
	}

	else if(strcmp(name, "server_port") == 0)
	{

		if(params->server_port == -1)//check first_time get the config file
		{
			params->server_port = atoi(value);
		}
		else
		{
			//return error message
			printf("duplicated configuration parameters \n");
			return -1;
		}
	} 

	else if (strcmp(name, "username") == 0)
	{

		if(strcmp(params->username, "") == 0)//check first_time get the comnfig file
		{
			strncpy(params->username, value, sizeof params->username);
		}
		else
		{
			//return error messagechar *encrypted_passwd
			printf("duplicated configuration parameters \n");
			return -1;
		}

	} 

	else if (strcmp(name, "password") == 0)
	{
		if(strcmp(params->password,"") == 0)//check first_time get the comnfig file
		{
			strncpy(params->password, value, sizeof params->password);
		}
		else
		{
			//return error message
			printf("duplicated configuration parameters \n");
			return -1;
		}
	}

	else if(strcmp(name, "concurrency") == 0)
	{

		if(params->concurrency == -1)//check first_time get the config file
		{
			params->concurrency = atoi(value);
		}
		else
		{
			//return error message
			printf("duplicated configuration parameters \n");
			return -1;
		}
	}

        //new version that writes to an array with just table names
	else if(strcmp(name, "table") == 0)
	{
				

		char temp1 = 'c';
		int a = 0;

		ModifyString2(column);
		int i = 0;
		int failed = 0;

		temp1 = 'c';
		while(temp1 != '\0') // ",jkdjjf,,,jskjf,"
		{
			temp1 = column[i];
			//printf("char: %c \n", column[i]);
			if(temp1 == ',')
			{
				//check if the next one is alpha or digits
				if( (isalpha(column[i+1]) || isdigit(column[i+1])) && (isalpha(column[i-1]) || isdigit(column[i-1]) || column[i-1]==']') )
				{
					//good still safe
				}
				else
				{

					printf("failed_1 \n");
				    failed = 1;
					return -1;
				}
			}
			i++;
		}
		if(failed == 1)
		{
			printf("failed_2 \n");
			return -1;

		}

		if(TestingStringCorrectness(column))
		{
			printf("abnormal characters \n");
			return -1;
		}
		//printf("column %s \n", column);

		temp1 = 'c';
		a=0;

		int count1 = 0;//the " , "
		int count2 = 0;//the " : "

		while(temp1 != '\0')//"cll:cd," or "cll:cd,ch"
		{
			//printf("the place 1\n");
			temp1 = column[a];
			if(temp1 == ',')
			{
				//printf("the place 2\n");
		        count1++;
			}
			if(temp1 == ':')
			{
				//printf("the place3\n");
				count2++;

			}
			a++;
		}
		if(count2-1 != count1)
		{
			printf("does not matches");
			return -1;
		}

		//printf("the place 3\n");

		//printf("the place 4\n");
	//	printf("name: %s \n", name);
	//	printf("value: %s \n", value);
	//  printf("column: %s \n\n", column);

		//*****************This is the parsing column part**************************//
		char *token2; //A temporary string for parsing
		char *token3; //For parsing each sub string
		char *token4;
		char *token5;
		char column_name_recevier[MAX_COLNAME_LEN][MAX_COLNAME_LEN];//temp to store the data
		char column_type_recevier[MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN];//temp to store the data
		char column_size_recevier[MAX_COLUMNS_PER_TABLE][MAX_COLNAME_LEN];//temp to store the data
		char *checkType;
		a = 0;

		//initialize array
		while(a < MAX_COLUMNS_PER_TABLE)
		{
			strncpy(column_name_recevier[a],"",sizeof(column_name_recevier[a]));//temp to store the data
			strncpy(column_type_recevier[a],"",sizeof(column_type_recevier[a]));//temp to store the data
			strncpy(column_size_recevier[a],"",sizeof(column_size_recevier[a]));//temp to store the data
			a++;
		}

		printf("the place 5\n");
		token2 = strtok(column,",");	//For parsing the first comma
		///printf("column2: %s \n\n", token2);


		if( token2!= NULL && strcmp(token2,"")==0)//if the token is null
		{

			printf ("something is NULL \n");
			return -1;
		}
		else
		{
			//printf("the token2 is fine\n");
		}
		//printf("the place 6\n");
		int current_num = 0;

		while(token2 != NULL)
		{
			snprintf(ColumnArray[current_num], sizeof(ColumnArray[current_num]), "%s", token2);
			printf("CurrentColumnArray (To be parsed based on :): %s \n", ColumnArray[current_num]);
			token2 = strtok(NULL,","); //get the rest of the line
			printf("token2 is: %s \n",token2 );
			printf("columnarray: %s\n",ColumnArray[current_num]);


			//printf("the place 7\n");
			//ERROR CHECKING - if the token is null
			if(token2 != NULL && strcmp(token2,"")==0)//if the token is null
			{
				printf ("something is NULL \n");
				return -1;
			}
			else
			{
				//printf("the token2 is fine\n");
			}


			//ERROR CHECKING - if there is the correct # of :
			char temp = 'c';
			a = 0;
			int CharCheck = 0;
			while(temp != '\0')
			{
				temp=ColumnArray[current_num][a];
				//printf("columnarray %s\n",ColumnArray[current_num]);
				//printf("temp: %c\n",temp);
				//printf("char for array: %c\n",ColumnArray[current_num][a]);
				if(temp == ':')
				{
					//printf("the place 8\n");
					CharCheck++;
				}a++;
			}
//			printf("place 9\n");
//			printf("columnarray: %c \n",ColumnArray[current_num][a]);
//			printf("place 10\n");
			if(CharCheck != 1)
			{
				printf ("colon \n");
				return -1;
			}
			current_num++;
		}

		//Currently the data is stored inside the ColumnArray[]


		int column_num = 0;
		int index_for_store = 0;
		for ( ; column_num < current_num; column_num++, index_for_store++)
		{
			token3 = strtok(ColumnArray[column_num],":");
			snprintf(column_name_recevier[index_for_store], sizeof(column_name_recevier[index_for_store]), "%s", token3);
			//printf("The column name-----------: %s %d\n", column_name_recevier[index_for_store], index_for_store);

			if(strcmp(token3,"")==0)//if the token is null
			{
				printf("token_3");
				return -1;
			}
			token3 = strtok(NULL,":"); //get the rest of the line
			snprintf(column_type_recevier[index_for_store], sizeof(column_type_recevier[index_for_store]), "%s", token3);
			printf("The column type-----------: %s %d\n\n", column_type_recevier[index_for_store], index_for_store);

			if(strcmp(token3,"")==0)//if the token is null
			{
				printf("token_3");
				return -1;
			}
			char ErrorCheckString[MAX_STRTYPE_SIZE];
			snprintf(ErrorCheckString,sizeof(ErrorCheckString), "%s", token3);



		char checkType1[4] = "char";// the  correct type of configuration file
		char checkType2[3] = "int";
		char tempChar[4];
		char tempInt[3];
		int result1;
		int result2;
		tempChar[0] = token3[0];
		tempChar[1] = token3[1];
		tempChar[2] = token3[2];
		tempChar[3] = token3[3];

		tempInt[0] = token3[0];
		tempInt[1] = token3[1];
		tempInt[2] = token3[2];

		result1 =  strcmp (checkType1, tempChar);
		result2 = strcmp (checkType2, tempInt);

		if ( (result1 != 0) && (result2 != 0) )
		{
			return -1;
		}
		//end of testing


		char* checkType = (char*) memchr (token3, 'c', strlen(token3));
			if (checkType!=NULL) //It is a char
			{
				//printf ("Token3 is a char, now parse it: \n");
				token4 = strtok(token3, "[");
				token5 = strtok(NULL, "]");
				//printf("token4: %s \n", token4);//char
				//printf("token5: %s \n", token5);//size
				snprintf(column_size_recevier[index_for_store], sizeof(column_size_recevier[index_for_store]), "%s", token5);//store the size of char array
			}
			else
			{
				strncpy(column_size_recevier[index_for_store], "0",sizeof(column_size_recevier[index_for_store]));//0 space
			}
			token3 = NULL;
			token4 = NULL;
			token5 = NULL;
			checkType = NULL;
		}

		int c = 0;
		int column_index_per_table = 0;
		for(; c < MAX_TABLES; c++)//iterate through each cell of table to check
		{
			//printf("table: %s \n", name);
			//printf("value: %s \n", value);
			//printf("paramstable: %s \n", params->table[c].tablename);

			//if(value == params->table[c].tablename)
			if(strcmp(value, params->table[c].tablename) == 0)//check if value already exists
			{
				printf("duplicate tablenames \n");
				return -1;
			}

			if(strcmp(params->table[c].tablename,"")==0)
			{
				//copy the tablename
				strncpy(params->table[c].tablename, value, sizeof(params->table[c].tablename));
				printf("set to: %s \n", params->table[c].tablename);

				//copy the table information
				column_index_per_table = 0;
				for(;column_index_per_table < MAX_COLUMNS_PER_TABLE; column_index_per_table++)
				{
					/*if(column_index_per_table<5 && c < 5)
					{
						printf("name rec: %s\n", column_name_recevier[column_index_per_table]);
						printf("type rec: %s\n", column_type_recevier[column_index_per_table]);

						printf("pre name: %s\n", params->table[c].column_data[column_index_per_table][0].columnvalue);
						printf("pre type: %s\n", params->table[c].column_data[column_index_per_table][1].columnvalue);
					}*/

					strncpy(params->table[c].column_data[column_index_per_table][0].columnvalue, column_name_recevier[column_index_per_table],sizeof(params->table[c].column_data[column_index_per_table][0].columnvalue));//for the columnname
					strncpy(params->table[c].column_data[column_index_per_table][1].columnvalue, column_type_recevier[column_index_per_table],sizeof(params->table[c].column_data[column_index_per_table][1].columnvalue));//for the columntable
					strncpy(params->table[c].column_data[column_index_per_table][2].columnvalue, column_size_recevier[column_index_per_table],sizeof(params->table[c].column_data[column_index_per_table][0].columnvalue));//for the columnname
//					printf("name: %s\n", params->table[c].column_data[column_index_per_table][0].columnvalue);
//					printf("type: %s\n", params->table[c].column_data[column_index_per_table][1].columnvalue);
//					printf("size: %s\n", params->table[c].column_data[column_index_per_table][2].columnvalue);
				}

				//printf("___________________________________________________________\n");
				return 0;
				break;
             }
		}
	}

	else
	{
            printf("none of the above \n");
            // Ignore unknown config parameters.
	}

	return 0;
}


/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params)
{
	int error_occurred = 0;

	//printf("before open \n");

	// Open file for reading.
	FILE *file = fopen(config_file, "r");
	//printf("after open \n");

	if (file == NULL)
	{
		error_occurred = 1;
	}

	// Process the config file.
	while (!error_occurred && !feof(file)) 
	{
		// Read a line from the file.
		char line[MAX_CONFIG_LINE_LEN];
		char *l = fgets(line, sizeof line, file);

		// Process the line.
		if (l == line)
		{
			//printf("line is: %s \n", line);
			if(process_config_line(line, params) == -1)
			{
				error_occurred = 1;
			}
		}
		else if (!feof(file))
		{
			error_occurred = 1;
		}	
	}

	if((strcmp(params->server_host,"") == 0) || (params->server_port == -1) ||
			(strcmp(params->username,"") == 0) || (strcmp(params->password,"") == 0) )
	{
		error_occurred = 1;
	}

	return error_occurred ? -1 : 0;
}


/**
 * @brief Generates a log message.
 *
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message)
{
	//(*ptr[1])++;
	fprintf(file,"%s",message);
	fflush(file);
}


/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 *
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt)
{
	if(salt != NULL)
		return crypt(passwd, salt);
	else
		return crypt(passwd, DEFAULT_CRYPT_SALT);
}


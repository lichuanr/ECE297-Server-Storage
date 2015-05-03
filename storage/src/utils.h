/**
 * @file
 * @brief This file declares various utility functions that are
 * can be used by the storage server and client library.
 *
 * In this header file, we have some macros for limiting corrensponding
 * maximum numbers. Besides, there is also a structure declaration
 * called config_params in this file. The config_params structure is
 * for holding configuration data. There are also some function declarations,
 * which will be used in utils.c
 */

#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include "storage.h"
#include <stdbool.h>


/**
 * @brief Building a filestream named file
 */
extern FILE* file;	// Process this line.




/**
 * @brief Any lines in the config file that start with this character 
 * are treated as comments.
 */
static const char CONFIG_COMMENT_CHAR = '#';

/**
 * @brief The max length in bytes of a command from the client to the server.
 */
#define MAX_CMD_LEN (1024 * 8)

/**
 * @brief A macro to log some information.
 *
 * Use it like this:  LOG(("Hello %s", "world\n"))
 *
 * Don't forget the double parentheses, or you'll get weird errors!
 */
#define LOG(x)  {printf x; fflush(stdout);}

/**
 * @brief A macro to output debug information.
 * 
 * It is only enabled in debug builds.
 */
#ifdef NDEBUG
#define DBG(x)  {}
#else
#define DBG(x)  {printf x; fflush(stdout);}
#endif

/**
 * @brief A struct to store tablename.
 */

typedef struct string2{
	char columnvalue[MAX_COLNAME_LEN];//maximum length of the array
};

typedef struct string1{
	char tablename[MAX_VALUE_LEN];
	struct string2 column_data[MAX_COLUMNS_PER_TABLE][3]; //holds the keys for comparison
};

/**
 * @brief A struct to store config parameters.
 *
 * This struct is for holding server_host,
 * server_port, username, and several table names.
 * These data came fron the configuration file.
 */
struct config_params{
	/// The hostname of the server.
	char server_host[MAX_HOST_LEN];

	/// The listening port of the server.
	int server_port;

	/// The storage server's username
	char username[MAX_USERNAME_LEN];

	/// The storage server's encrypted password
	char password[MAX_ENC_PASSWORD_LEN];

	/// Multiple Table Names (MAX_TABLES = 100)

	int concurrency;
	//for the single client or muti-data


	struct string1 table[MAX_TABLES];
        
	/// The directory where tables are stored.
//	char data_directory[MAX_PATH_LEN];
};


/**
 * @brief Exit the program because a fatal error occured.
 *
 * @param msg The error message to print.
 * @param code The program exit return value.
 */
static inline void die(char *msg, int code)
{
	printf("%s\n", msg);
	exit(code);
}

/**
 * @brief Keep sending the contents of the buffer until complete.
 * @return Return 0 on success, -1 otherwise.
 *
 * The parameters mimic the send() function.
 */
int sendall(const int sock, const char *buf, const size_t len);

/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise.
 */
int recvline(const int sock, char *buf, const size_t buflen);

/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params);

/**
 * @brief Generates a log message.
 * 
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message);

/**
 * @brief Default two character salt used for password encryption.
 */
#define DEFAULT_CRYPT_SALT "xx"

/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 * 
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt);

#endif

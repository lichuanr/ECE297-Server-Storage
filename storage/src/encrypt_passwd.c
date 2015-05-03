/**
 * @file
 * @brief This program implements a password encryptor.
 *
 * This file can build a stand-alone program, which can
 * be used to generate encrypted password based on the
 * original password
 */

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

/**
 * @brief Print the usage to stdout.
 */
void print_usage()
{
	printf("Usage: encrypt_passwd PASSWORD [SALT]\n");
}

/**
 * @brief This is the main file of the encryption part,
 * which is used to generate the encrypted password
 *
 * @param argc The number of strings pointed to by argv
 * @param argv[] An array of pointers to characters
 * @return Return 0 if encrypted successfully, otherwise return -1
 */
int main(int argc, char *argv[])
{
	char *passwd;
	char *salt;

	if(argc == 2) {
		passwd = argv[1];
		salt = NULL;
	} else if(argc == 3) {
		passwd = argv[1];
		salt = argv[2];
	} else {
		print_usage();
		return -1;
	}

	char *encrypted_passwd = generate_encrypted_password(passwd, salt);
	if(encrypted_passwd == NULL) {
		printf("An error occured.\n");
		return -1;
	}
	printf("%s\n", encrypted_passwd);
	return 0;
}

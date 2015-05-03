#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <check.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include "storage.h"

#define TESTTIMEOUT	10		// How long to wait for each test to run.
#define SERVEREXEC	"./server"	// Server executable file.
#define SERVEROUT	"default.serverout"	// File where the server's output is stored.
#define SERVEROUT_MODE	0666		// Permissions of the server ouptut file.
#define ONETABLE_CONF			"conf-onetable.conf"	// Server configuration file with one table.
#define SIMPLETABLES_CONF		"conf-simpletables.conf"	// Server configuration file with simple tables.
#define COMPLEXTABLES_CONF		"conf-complextables.conf"	// Server configuration file with complex tables.
#define DUPLICATE_COLUMN_TYPES_CONF     "conf-duplicatetablecoltype.conf"        // Server configuration file with duplicate column types.
#define BADTABLE	"bad table"	// A bad table name.
#define BADKEY		"bad key"	// A bad key name.
#define KEY		"somekey"	// A key used in the test cases.
#define KEY1		"somekey1"	// A key used in the test cases.
#define KEY2		"somekey2"	// A key used in the test cases.
#define KEY3		"somekey3"	// A key used in the test cases.
#define KEY4		"somekey4"	// A key used in the test cases.
#define VALUESPC	"someval 4"	// A value used in the test cases.
#define INTCOL		"col"		// An integer column
#define INTVALUE	"22"		// An integer value
#define INTCOLVAL	"col 22"	// An integer column name and value

// These settings should correspond to what's in the config file.
#define SERVERHOST	"localhost"	// The hostname where the server is running.
#define SERVERPORT	4848		// The port where the server is running.
#define SERVERUSERNAME	"admin"		// The server username
#define SERVERPASSWORD	"dog4sale"	// The server password
//#define SERVERPUBLICKEY	"keys/public.pem"	// The server public key
//#define DATADIR		"./mydata/"	// The data directory.
#define TABLE		"inttbl"	// The table to use.
#define INTTABLE	"inttbl"	// The first simple table.
//#define FLOATTABLE	"floattbl"	// The second simple table.
#define STRTABLE	"strtbl"	// The third simple table.
#define THREECOLSTABLE	"threecols"	// The first complex table.
#define FOURCOLSTABLE	"fourcols"	// The second complex table.
#define SIXCOLSTABLE	"sixcols"	// The third complex table.
#define MISSINGTABLE	"missingtable"	// A non-existing table.
#define MISSINGKEY	"missingkey"	// A non-existing key.

#define FLOATTOLERANCE  0.0001		// How much a float value can be off by (due to type conversions).

/* Server port used by test */
int server_port;

/**
 * @brief Compare whether two floating point numbers are almost the same.
 * @return 0 if the numbers are almost the same, -1 otherwise.
 */
int floatcmp(float a, float b)
{
	if (fabs(a - b) < FLOATTOLERANCE)
		return 0;
	else
		return -1;
}

/**
 * @brief Remove trailing spaces from a string.
 * @param str The string to trim.
 * @return The modified string.
 */
char* trimtrailingspc(char *str)
{
	// Make sure string isn't null.
	if (str == NULL)
		return NULL;

	// Trim spaces from the end.
	int i = 0;
	for (i = strlen(str) - 1; i >= 0; i--) {
		if (str[i] == ' ')
			str[i] = '\0';
		else
			break; // stop if it's not a space.
	}
	return str;
}

/**
 * @brief Start the storage server.
 *
 * @param config_file The configuration file the server should use.
 * @param status Status info about the server (from waitpid).
 * @param serverout_file File where server output is stored.
 * @return Return server process id on success, or -1 otherwise.
 */
int start_server(char *config_file, int *status, const char *serverout_file)
{
	sleep(1);       // Give the OS enough time to kill previous process

	pid_t childpid = fork();
	if (childpid < 0) {
		// Failed to create child.
		return -1;
	} else if (childpid == 0) {
		// The child.

		// Redirect stdout and stderr to a file.
		const char *outfile = serverout_file == NULL ? SERVEROUT : serverout_file;
		//int outfd = creat(outfile, SERVEROUT_MODE);
		int outfd = open(outfile, O_CREAT|O_WRONLY, SERVEROUT_MODE);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		if (dup2(outfd, STDOUT_FILENO) < 0 || dup2(outfd, STDERR_FILENO) < 0) {
			perror("dup2 error");
			return -1;
		}

		// Start the server
		execl(SERVEREXEC, SERVEREXEC, config_file, NULL);

		// Should never get here.
		perror("Couldn't start server");
		exit(EXIT_FAILURE);
	} else {
		// The parent.

		// If the child terminates quickly, then there was probably a
		// problem running the server (e.g., config file not found).
		sleep(1);
		int pid = waitpid(childpid, status, WNOHANG);
		//printf("Parent returned %d with child status %d\n", pid, WEXITSTATUS(*status));
		if (pid == childpid)
			return -1; // Probably a problem starting the server.
		else
			return childpid; // Probably ok.
	}
}

/**
 * @brief Start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Start the server.
	int pid = start_server(config_file, NULL, serverout_file);
	fail_unless(pid > 0, "Server didn't run properly.");
	if (serverpid != NULL)
		*serverpid = pid;

	// Connect to the server.
	void *conn = storage_connect(SERVERHOST, server_port);
	fail_unless(conn != NULL, "Couldn't connect to server.");

	// Authenticate with the server.
	int status = storage_auth(SERVERUSERNAME,
				  SERVERPASSWORD,
				  //SERVERPUBLICKEY,
				      conn);
	fail_unless(status == 0, "Authentication failed.");

	return conn;
}

/**
 * @brief Delete the data directory, start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* clean_start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Delete the data directory.
//	system("rm -rf " DATADIR);

	return start_connect(config_file, serverout_file, serverpid);
}

/**
 * @brief Create an empty data directory, start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* init_start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Delete the data directory.
//	system("rm -rf " DATADIR);
	
	// Create the data directory.
//	mkdir(DATADIR, 0777);

	return start_connect(config_file, serverout_file, serverpid);
}

/**
 * @brief Kill the server with given pid.
 * @return 0 on success, -1 on error.
 */
int kill_server(int pid)
{
	int status = kill(pid, SIGKILL);
	fail_unless(status == 0, "Couldn't kill server.");
	return status;
}


/// Connection used by test fixture.
void *test_conn = NULL;


// Keys array used by test fixture.
char* test_keys[MAX_RECORDS_PER_TABLE];

/**
 * @brief Text fixture setup.  Start the server.
 */
void test_setup_simple()
{
	test_conn = init_start_connect(SIMPLETABLES_CONF, "simpleempty.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");
}

/**
 * @brief Text fixture setup.  Start the server and populate the tables.
 */
void test_setup_simple_populate()
{
	test_conn = init_start_connect(SIMPLETABLES_CONF, "simpledata.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");

	struct storage_record record;
	int status = 0;
	int i = 0;

	// Create an empty keys array.
	// No need to free this memory since Check will clean it up anyway.
	for (i = 0; i < MAX_RECORDS_PER_TABLE; i++) {
		test_keys[i] = (char*)malloc(MAX_KEY_LEN); 
		strncpy(test_keys[i], "", sizeof(test_keys[i]));
	}

	// Do a bunch of sets (don't bother checking for error).

	strncpy(record.value, "col -2", sizeof record.value);
	status = storage_set(INTTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col 2", sizeof record.value);
	status = storage_set(INTTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col 4", sizeof record.value);
	status = storage_set(INTTABLE, KEY3, &record, test_conn);

//	strncpy(record.value, "col -2.2", sizeof record.value);
//	status = storage_set(FLOATTABLE, KEY1, &record, test_conn);
//	strncpy(record.value, "col 2.2", sizeof record.value);
//	status = storage_set(FLOATTABLE, KEY2, &record, test_conn);
//	strncpy(record.value, "col 4.0", sizeof record.value);
//	status = storage_set(FLOATTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col abc", sizeof record.value);
	status = storage_set(STRTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col def", sizeof record.value);
	status = storage_set(STRTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col abc def", sizeof record.value);
	status = storage_set(STRTABLE, KEY3, &record, test_conn);
}

/**
 * @brief Text fixture setup.  Start the server with complex tables.
 */
void test_setup_complex()
{
	test_conn = init_start_connect(COMPLEXTABLES_CONF, "complexempty.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");
}

/**
 * @brief Text fixture setup.  Start the server with complex tables and populate the tables.
 */
void test_setup_complex_populate()
{
	test_conn = init_start_connect(COMPLEXTABLES_CONF, "complexdata.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");

	struct storage_record record;
	int status = 0;
	int i = 0;

	// Create an empty keys array.
	// No need to free this memory since Check will clean it up anyway.
	for (i = 0; i < MAX_RECORDS_PER_TABLE; i++) {
		test_keys[i] = (char*)malloc(MAX_KEY_LEN); 
		strncpy(test_keys[i], "", sizeof(test_keys[i]));
	}

	// Do a bunch of sets (don't bother checking for error).

	strncpy(record.value, "col1 -2,col2 -2,col3 abc", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 2,col2 2,col3 def", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 4,col2 4,col3 abc def", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col1 abc,col2 -2,col3 -2,col4 ABC", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 def,col2 2,col3 2,col4 DEF", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 abc def,col2 4,col3 4,col4 ABC DEF", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col1 abc,col2 ABC,col3 -2,col4 2,col5 -2,col6 2", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 abc,col2 ABC,col3 2,col4 -2,col5 2,col6 -2", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 def,col2 DEF,col3 4,col4 -4,col5 4,col6 -4", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY3, &record, test_conn);

}
/**
 * @brief Text fixture teardown.  Disconnect from the server.
 */
void test_teardown()
{
	// Disconnect from the server.
	storage_disconnect(test_conn);
	//fail_unless(status == 0, "Error disconnecting from the server.");
}



/**
 * This test makes sure that the storage.h file has not been modified.
 */
START_TEST (test_sanity_filemod)
{
	// Compare with the original version of storage.h.
	int status = system("md5sum --status -c md5sum.check &> /dev/null");
	fail_if(status == -1, "Error computing md5sum of storage.h.");
	int matches = WIFEXITED(status) && WEXITSTATUS(status) == 0;

	// Fail if it doesn't match original version.
	fail_if(!matches, "storage.h has been modified.");
}
END_TEST

/*
 * One server instance tests:
 * 	query from simple tables.
 */

/**********************Query test: invalid table/predicates/max-key/conn********************************/
START_TEST (test_queryinvalid_invalidtable)
{
	int foundkeys = storage_query(NULL, "col > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_invalidpredicates)
{
	int foundkeys = storage_query(INTTABLE, NULL, test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_invalidtestkeys)
{
	int foundkeys = storage_query(INTTABLE, "col > 10", NULL, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_invalidmaxkeys)
{
	int foundkeys = storage_query(INTTABLE, "col > 10", test_keys, NULL, test_conn);
	fail_unless(foundkeys == -1, "storage_query with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_invalidconn)
{
	int foundkeys = storage_query(INTTABLE, "col > 10", test_keys, MAX_RECORDS_PER_TABLE, NULL);
	fail_unless(foundkeys == -1, "storage_query with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST
/*******************************************************************************************************/

/******************Query test: bad table/max-key/column name/operator***********************************/
START_TEST (test_queryinvalid_badtable)
{
	int foundkeys = storage_query(BADTABLE, "col > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with bad table name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with bad table name not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badmaxkeys)
{
	int foundkeys = storage_query(INTTABLE,"col > 10", test_keys, -1, test_conn);
	fail_unless(foundkeys == -1, "storage_query with bad max key number should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with bad max key number not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badcolname)
{
	int foundkeys = storage_query(INTTABLE,"asdf > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with bad column name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with bad column name not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badoperator)
{
	int foundkeys = storage_query(STRTABLE,"col > asdf", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with bad operator should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with bad operator not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badkeyvalfloat)
{
	int foundkeys = storage_query(INTTABLE,"col > 4.56", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with float value should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with float value not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badkeyvalstr)
{
	int foundkeys = storage_query(INTTABLE,"col = asdf", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with string in int table should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with string in int table not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badkeyvalint)
{
	int foundkeys = storage_query(STRTABLE,"col > 4", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with int in string table should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with int in string table not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badkeyextrcommaint)
{
	int foundkeys = storage_query(INTTABLE,"col > 4,", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with extra comma should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with extra comma not setting errno properly.");
}
END_TEST

START_TEST (test_queryinvalid_badkeyextrcommastr)
{
	int foundkeys = storage_query(STRTABLE,"col = abc,", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with extra comma should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with extra comma not setting errno properly.");
}
END_TEST
/*******************************************************************************************************/

/*************************Query test: missing table/column name*****************************************/
START_TEST (test_querymissing_missingtable)
{
	int foundkeys = storage_query(MISSINGTABLE, "col > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with missing table name should fail.");
	fail_unless(errno == ERR_TABLE_NOT_FOUND, "storage_query with missing table name not setting errno properly.");
}
END_TEST

START_TEST (test_querymissing_missingcolname)
{
	int foundkeys = storage_query(INTTABLE, "asdf > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with missing column name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with missing column name not setting errno properly.");
}
END_TEST
/******************************************************************************************************/

/********************Query test: missing comma/predicate & too many columns****************************/
START_TEST (test_querymissing_missingcomma)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 > 10 col3 = asd , col2 < 4", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with missing comma should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with missing comma not setting errno properly.");
}
END_TEST

START_TEST (test_querymissing_missingpredicate)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 > 10 , , col3 = asdf ", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with missing predicate should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with missing predicate not setting errno properly.");
}
END_TEST

START_TEST (test_querymissing_toomanycols)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 > 10 , col2 < -5 , col3 = asdf , col4 > 5", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "storage_query with missing predicate should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_query with missing predicate not setting errno properly.");
}
END_TEST
/*******************************************************************************************************/

START_TEST (test_query_int0)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > 10", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 0, "Query didn't find the correct number of keys.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[0], "") == 0, "No extra keys should be modified.\n");
}
END_TEST


START_TEST (test_query_int1)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(INTTABLE, "col < -1", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

/************************Query test: query on simple tables with char*************************************/
START_TEST (test_query_char0)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(STRTABLE, "col = asdf", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 0, "Query didn't find the correct character of keys.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[0], "") == 0, "No extra keys should be modified.\n");
}
END_TEST


START_TEST (test_query_char1)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(STRTABLE, "col = abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

/*********************************************************************************************************/

/**************************Query test: query on complex tables********************************************/
START_TEST (test_querycomplex_none)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 > 4 , col2 > 4 , col3 = asdf", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 0, "Query didn't find the correct character of keys.");
	fail_unless(strcmp(test_keys[0], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

START_TEST (test_querycomplex_one)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 = -2 , col2 = -2 , col3 = abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");
	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

START_TEST (test_querycomplex_nospaceone)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1=-2,col2=-2,col3=abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");
	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

START_TEST (test_querycomplex_oneless)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 < 5 , col3 = abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");
	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

START_TEST (test_querycomplex_onemore)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 > -3 , col3 = abc def", test_keys, MAX_RECORDS_PER_TABLE, test_conn);	fail_unless(foundkeys == 1, "Query didn't find the coreect character of keys.");
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");
	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY3) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST

START_TEST (test_querycomplex_maxkeyless)
{
	int foundkeys = storage_query(THREECOLSTABLE, "col1 < 5", test_keys, 1, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct character of keys.");
	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.\n");
}
END_TEST
/*********************************************************************************************************/

/**
 * @brief This runs the marking tests for Assignment 3.
 */
int main(int argc, char *argv[])
{
	if(argc == 2)
		//server_port = 5827;
		server_port = atoi(argv[1]);
	else
		//server_port = 5827;
		server_port = SERVERPORT;
	printf("Using server port: %d.\n", server_port);
	Suite *s = suite_create("a3-partial");
	TCase *tc;

	// Sanity tests
	tc = tcase_create("sanity");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_test(tc, test_sanity_filemod);
	suite_add_tcase(s, tc);

	// Query tests on simple tables
	tc = tcase_create("querysimple");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_query_int0);
	tcase_add_test(tc, test_query_int1);
	tcase_add_test(tc, test_query_char0);
	tcase_add_test(tc, test_query_char1);
	suite_add_tcase(s, tc);

	// Query tests on complex tables
	tc = tcase_create("querycomplex");
	tcase_add_checked_fixture(tc, test_setup_complex_populate, test_teardown);
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_test(tc, test_querycomplex_none);
	tcase_add_test(tc, test_querycomplex_one);
	tcase_add_test(tc, test_querycomplex_nospaceone);
	tcase_add_test(tc, test_querycomplex_oneless);
	tcase_add_test(tc, test_querycomplex_onemore);
	suite_add_tcase(s, tc);

	// Query tests with invalid parameters on tables/predicates/keys/max_key/conn
	tc = tcase_create("queryinvalid");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_queryinvalid_invalidtable);
	tcase_add_test(tc, test_queryinvalid_invalidpredicates);
	tcase_add_test(tc, test_queryinvalid_invalidtestkeys);
	tcase_add_test(tc, test_queryinvalid_invalidmaxkeys);
	tcase_add_test(tc, test_queryinvalid_invalidconn);
	suite_add_tcase(s, tc);

	// Query test with bad table names/max key number
	tc = tcase_create("querybad");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_queryinvalid_badtable);
	tcase_add_test(tc, test_queryinvalid_badmaxkeys);
	tcase_add_test(tc, test_queryinvalid_badcolname);
	tcase_add_test(tc, test_queryinvalid_badoperator);
	tcase_add_test(tc, test_queryinvalid_badkeyvalfloat);
	tcase_add_test(tc, test_queryinvalid_badkeyvalstr);
	tcase_add_test(tc, test_queryinvalid_badkeyvalint);
	tcase_add_test(tc, test_queryinvalid_badkeyextrcommaint);
	tcase_add_test(tc, test_queryinvalid_badkeyextrcommastr);
	suite_add_tcase(s, tc);

	// Query test with missing table names/column name
	tc = tcase_create("querymissingsimple");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_querymissing_missingtable);
	tcase_add_test(tc, test_querymissing_missingcolname);
	suite_add_tcase(s, tc);

	// Query test with missing comma/predicate & too many columns
	tc = tcase_create("querymissingcomplex");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_complex_populate, test_teardown);
	tcase_add_test(tc, test_querymissing_missingcomma);
	tcase_add_test(tc, test_querymissing_missingpredicate);
	tcase_add_test(tc, test_querymissing_toomanycols);
	tcase_add_test(tc, test_querycomplex_maxkeyless);
	suite_add_tcase(s, tc);

	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "results.log");
	srunner_run_all(sr, CK_ENV);
	srunner_ntests_failed(sr);
	srunner_free(sr);

	return EXIT_SUCCESS;
}

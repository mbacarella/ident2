
#include "config.h"	/* need this for sys dependancy fun */

/*
 * User the ident service should run as to provide maximum system
 * safety (in case of exploits and such). User nobody should work
 * fine on Linux and FreeBSD. This is only supplied for admins that
 * may perhaps be -ultra- security conscious (insane?) enough to make 
 * a seperate user for ident services. It also allows for fun things
 * with groups.
 */
#define SAFE_ACCESS_USER	"nobody"

/*
 * Port to bind to.
 * I am not aware of any other ident server or ident query agent
 * in existance that uses a port other than 113.
 */
#define IDENT_PORT      113

/*
 * user should create this file in their home directory 
 * if they do not want ident2 to send out ident 
 * replies.
 */
#define	NOIDENT_FILE	".noident"

/*
 * ident server looks for this file in the user's
 * home directory and uses this to decide what to reply
 */
#define USERCONF_FILE   ".ident"

/*
 * number of default max connections (overriden via command-line)
 * that a server will handle at once.
 */
#define	DEF_MAX_CONNECTIONS	25

/*
 * default amount of time (overriden via command-line) to keep
 * any given connection open for (specified in seconds).
 */
#define	DEF_CLIENT_TIMEOUT	20

/*
 * if you happen to use this feature
 */
#define RAND_STRING_LENGTH	6


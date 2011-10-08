/* 
 * Ident-2 - an Identity server for UNIX
 * Copyright (C) 1998-2001 Michael Bacarella
 * Copyright (C) 2003 Netgraft Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Please view the file README for program information.
 *
 */

#include "ident2.h"

#ifdef HAS_GETOPT_LONG
#include <getopt.h>	/* getopt() is declared in unistd.h */
#endif

	/*
	 *	determine if we're running as a child of inetd
	 *	or if we were started via a command line (or from
	 *	a process not inetd)
	 */
static int
inetd_child (void)
{
	struct sockaddr_in sin;
	int sinsize = sizeof (struct sockaddr_in);
	
		/* if child of inetd, 0 would be a socket */
	if (getsockname (0, (struct sockaddr *)&sin, (int *)&sinsize) == -1) {
		if (errno == ENOTSOCK)
			return 0;	/* must be a filedescriptor */
		else {
			syslog (LOG_ERR, "getsockname: %s\n", strerror (errno));
			exit (-1);
		}
	}
	return 1;
}

#define print_header()	printf ("Ident2 version %s, Copyright (C) 1999-2004 Michael Bacarella\n", ID_VERSION)

static void
print_usage (void)
{
	print_header();
	puts ("Example command lines:");
puts ("		ident2 -r		sends random replies, always");
puts ("		ident2 -i -y .reply	allows user replies, which are read\n"
      "					from the file ~/.reply");
puts ("		ident2 -m 5 -o 10	ultra secure mode. no more than\n"
      "					5 connections at once, each connection\n"
      "					is killed after 10 seconds regardless.");
puts ("		ident2 -s		stop ident2 from trying to change it's\n"
      "					userid if it discovers that it's running\n"
      "					as a root process.");
}

static void
print_license (void)
{
	print_header();
	puts(
"ident2 is free software, and you are welcome to redistribute it\n"
"under certain conditions; ident2 comes with ABSOLUTELY NO WARRANTY;\n"
"for details, go to http://www.gnu.org/\n");
}

static void
print_help (void)
{
	print_header();
#ifdef HAS_GETOPT_LONG
	puts (  
"usage: ident2 [options]\n"
"options:\n"
"\n"
"these parameters apply to all incarnations of ident2\n"
"\n"
"	-h	--help			this command line information\n"
"	-u	--usage			shows example command lines\n"
"	-v	--version		show version information\n"
"	-l	--license		show licensing information\n"
"	-a	--force-inetd		force inetd mode\n"
"	-d	--force-daemon		force standalone daemon\n"
"	-i	--use-user-ident	allow user defined ident replies\n"
"	-y	--user-reply-file	file in user's homedir for replies\n"
"	-n	--allow-noident-file	don't reply if user has a ~/NOIDENT file\n"
"	-o	--client-timeout	clients timeout after this many secs\n"
"	-s	--dont-change-uid	don't try to change uid (to nobody)\n"
"	-r	--always-random		always send a random reply\n"
"\n"
"these parameters only apply to the daemon ident2\n"
"\n"
"	-m	--daemon-maxclients	accept no more than this many clients\n"
"	-p	--daemon-port		bind this port, instead of 'auth'\n"
);
#else
	puts(
"usage: ident2 [options]\n"
"options:\n"
"\n"
"these parameters apply to all incarnations of ident2\n"
"\n"
"	-h		this command line information\n"
"	-u		shows example command lines\n"
"	-v		show version information\n"
"	-l		show licensing information\n"
"	-a		force inetd mode\n"
"	-d		force standalone daemon\n"
"	-i		allow user defined ident replies\n"
"	-y		file in user's homedir for replies\n"
"	-n		don't reply if user has a ~/NOIDENT file\n"
"	-o		clients timeout after this many secs\n"
"	-s		don't try to change uid (ex: to nobody)\n"
"	-r		always send a random reply, even to bad requests\n"
"\n"
"these parameters only apply to the daemon ident2\n"
"\n"
"	-m		accept no more than this many clients\n"
"	-p		bind this port, instead of 'auth'\n"
);
#endif
}

        /*
         *      print version info
         */
static void
print_version (void)
{
        printf ("Ident2 version %s, maintainer %s\n\n",
                ID_VERSION, ID_MAINTAINER);
}


static void
command_line (int argc, char *argv[])
{
	int c;
			/* getopt_long support implemented
			 * by Alexander Reelsen <ar@rhwd.net> */
#ifdef HAS_GETOPT_LONG
	while (1) {
/* int this_option_optind = optind ? optind : 1; */	/*unused?*/
		int option_index = 0;
		static struct option long_options[] = {
			{"allow-noident-file", 0, 0, 'n'},
			{"help", 0, 0, 'h'},
			{"usage", 0, 0, 'u'},
			{"version", 0, 0, 'v'},
			{"license", 0, 0, 'l'},
			{"force-inetd", 0, 0, 'a'},
			{"force-daemon", 0, 0, 'd'},
			{"use-user-ident", 0, 0, 'i'},
			{"dont-change-uid", 0, 0, 's'},
			{"always-random", 0, 0, 'r'},
			{"user-reply-file", 1, 0, 'y'},
			{"client-timeout", 1, 0, 'o'},
			{"daemon-maxclients", 1, 0, 'm'},
			{"daemon-port", 1, 0, 'p'},
			{0, 0, 0, 0},
		};

		c = getopt_long (argc, argv, "nhuvrliady:o:ftTsm:p:",
			long_options, &option_index);
		if (c == -1) {
			break;
		}	
#else
        while ((c = getopt (argc, argv, "nhuvrliady:o:ftTsm:p:")) != -1) {
#endif
		switch (c) {
			case 'n':
				Allow_NOIDENT = TRUE;
				break;
			case 'h':
				print_help ();
				exit (0);
			case 'u':
				print_usage ();
				exit (0);	
			case 'v':
				print_version ();
				exit (0);
			case 'l':
				print_license ();
				exit (0);
			case 'a':
				Service_Type = INETD;
				break;
			case 'd':
				Service_Type = DAEMON;
				break;
			case 'i':
				Use_User_Ident = TRUE;
				break;
			case 's':
				Dont_Change_Uid = TRUE;
				break;
			case 'r':
				Reply_Always_Random = TRUE;
				break;
			case 'y':
				User_Ident_File = optarg;
				break;
			case 'o':
				if ((Client_Timeout = atol (optarg)) == 0) {
					fprintf (stderr, "ERROR: Ident2: bad "
						"value for arguement `o'\n");
					exit (1);
				}
				break;
			case 'm':
				if ((Max_Connections = atol (optarg)) == 0) {
					fprintf (stderr, "ERROR: Ident2: bad"
						" value for arguement `m'\n");
					exit (1);
				}
				break;
			case 'p':
				if ((Ident_Port = atoi (optarg)) == 0) { 
					fprintf (stderr, "ERROR: Ident2: bad"
						" value for arguement `p'\n");
					exit (1);
				}
				break;
			case '?':
			default:
#ifdef HAS_GETOPT_LONG
				fprintf (stderr, "ident2: bad command line:\n"
				"\t``ident2 --help | more'' for help\n"
				"\t``ident2 --usage'' for some examples\n\n");
#else
				fprintf (stderr, "ident2: bad command line:\n"
				"\t``ident2 -h | more'' for help\n"
				"\t``ident2 -u'' for some examples\n\n");
#endif
				exit (1);
		}
	}
}


int
main (int argc, char *argv[])
{	
	INIT_GLOBALS();
	command_line (argc, argv);
	 	
			/* try to determine the service type if nothing
			 * has been forced (by commandline) */
	if (Service_Type == NO_TYPE) {
		if (inetd_child ())
			Service_Type = INETD;
		else
			Service_Type = DAEMON;
	}

	if (Service_Type == INETD) {
		openlog ("in.ident2", LOG_PID, LOG_AUTH);
		child_service (STDIN_FILENO, STDOUT_FILENO);
		closelog ();
	}
	else
		daemon_service ();

	return 0;
}


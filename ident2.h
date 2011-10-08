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

#ifndef IDENT2_H_
#define IDENT2_H_

#define ID_VERSION	"1.05-FINAL"
#define ID_MAINTAINER	"mike@bacarella.com"

#define FALSE	0
#define TRUE	1

#define ID_BUF_SIZE	128

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>
  
#include <pwd.h>
#include <syslog.h>
 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "define.h"

/*
 * ==================================================================
 * GLOBALS
 * ==================================================================
 */

enum Service_Type {NO_TYPE, DAEMON, INETD};

/* ------------ variables ---------- */

size_t			Max_Connections;
size_t			Client_Timeout;

enum Service_Type	Service_Type;

char			Dont_Change_Uid;
char			Use_User_Ident;
char			Allow_NOIDENT;
char			*User_Ident_File;
char			Reply_Always_Random;

unsigned short		Ident_Port;

/* ----------- handy init macro ---------- */

#define INIT_GLOBALS()	{			\
	Dont_Change_Uid = FALSE;		\
	Reply_Always_Random = FALSE;		\
	Use_User_Ident = FALSE;			\
						\
	Ident_Port = IDENT_PORT;		\
	Service_Type = NO_TYPE;			\
						\
	User_Ident_File = USERCONF_FILE;	\
	Max_Connections = DEF_MAX_CONNECTIONS;	\
	Client_Timeout = DEF_CLIENT_TIMEOUT;	\
}
	
                        /* #### DAEMON.C #### */
void	daemon_service (void);


			/* #### MACHINE.C #### */
			/* ( M_<PLATFORM>.C ) */
/**
 **	MACHINE DEPENDANT FUNCTIONS
 **	ports need only implement the functionality
 **	exported from these three functions
 **/
 
int	m_get_uid (struct in_addr *, u_short, struct in_addr *, u_short);
int	m_reduce_rights (void);		/* ..to minimize damage.. */
int	m_register_pid (void);		/* for service management purposes */


		/* #### COMMON.C #### */

void	*xmalloc (size_t);	
void	child_service (int, int);
void	nexus (int, char *);

#endif /* WHOLE FILE */


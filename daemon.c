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
 * ---------------------
 *	This file provides the 'daemon' implementation of
 *	ident2.
 *
 *	Daemon waits for connections
 *	forks, handles connections, cleans up,
 *	dies, etc.
 *
 *	Module entry point is ``daemon_service''
 * ---------------------
 */

#include "ident2.h"


static int
_go_daemon (void)
{
	switch (fork()) {
	case 0:
		setsid();
		m_register_pid ();
		m_reduce_rights ();
		return 0;
	case -1:
		syslog (LOG_ERR, "d_core: error fork(): %s\n", strerror(errno));
		return -1;
	default:
		exit (0);
	}
}
	
	/*
	 *	remove dead children from the process vector
	 */
static void
_reap_proc (pid_t *pv, size_t Max_Connections)
{
	pid_t *p;
	int r, s;

	for (p = pv; p < pv+Max_Connections; p++) {
		if (*p == 0)
			continue;

		while ((r = waitpid (*p, &s, WNOHANG)) == -1)
			if (errno != EINTR) {
				syslog (LOG_NOTICE,
					"warning: waitpid() error: %s",
						strerror (errno));
				break;
			}
		if (r == 0)
			continue;
		if (r > 0)
			*p = 0;
	}
}


static void
declient (int s)
{
	struct sockaddr sin;
	int ss = sizeof (sin);
	close (accept (s, (struct sockaddr *)&sin, &ss));
}

                                                 	
static void _sig_ign (int s) { return; }


static int
_accept_connect (int sv, struct sockaddr_in *sin)
{
	size_t sl = sizeof (struct sockaddr_in);
	int cl;

	while ((cl = accept(sv, (struct sockaddr *)sin, &sl)) == -1) {
		if (errno == EINTR)
			continue;
		return -1;
	}
	return cl;
}
	
	/*
	 *	will wait for a connection on SV,
	 *	find a space in the proc table and fork() 
	 *	child for it. the child will accept() it and
	 *	process the waiting client.
	 *	we rely on the delivery of EINTR to interrupt
	 *	select(), which gives us a chance to test for
	 *	deceased children.
	 */
static void
d_core (int sv)
{
	struct sigaction sa;
 	struct sockaddr_in sin;
 	pid_t	*pv;
				/* become daemon */
	if (_go_daemon () == -1) {
		syslog (LOG_ERR, "d_core: error fork(): %s\n", strerror(errno));
		return;
	}
			
	pv = xmalloc (Max_Connections * sizeof (pid_t)); 
	memset (pv, 0, Max_Connections * sizeof(pid_t));
	 
	sa.sa_handler = _sig_ign;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	
	if (sigaction (SIGCHLD, &sa, NULL) == -1) {
		syslog (LOG_ERR, "error: registering SIGCHLD handler: %s",
			strerror (errno));
		return;
	}
	
	while (1) {
		pid_t *p;		
		int cl;
		fd_set rfd;
		
		FD_ZERO(&rfd);
		FD_SET(sv, &rfd);
		
		if (select (sv+1, &rfd, NULL, NULL, NULL) == -1) {
			if (errno != EINTR)
				break;
			_reap_proc (pv, Max_Connections);
			continue;
		}
		if (!FD_ISSET (sv, &rfd))
			continue;

		for (p = pv; p < pv+Max_Connections; p++)
			if (*p == 0)
				break;
			
		if (p == pv+Max_Connections) {
			syslog (LOG_INFO, "refusing %s: too many open "
				"connections", inet_ntoa (sin.sin_addr));
			declient (sv);
		}
	
		if ((cl = _accept_connect (sv, &sin)) == -1) {
			syslog (LOG_NOTICE, "warning: inconsistency error: "
				"select() and accept() disagree: %s",
					strerror (errno));
			continue;
		}	
		
		if ((*p = fork()) == -1) {
			syslog (LOG_ERR, "warning: couldn't fork(): %s",
				strerror (errno));
			*p = 0;
		} else if (*p == 0) {
			child_service (cl, cl);
			close (cl);
			exit (0);
		}
		close (cl); /* close now that it's mapped into the child */
	}
}


void daemon_service (void)
{
	int s;
	struct sockaddr_in sin;

	openlog ("ident2", LOG_PID, LOG_DAEMON);
		
	if ((s = socket (PF_INET, SOCK_STREAM, 0)) == -1) {
		syslog (LOG_ERR, "error: socket(): %s", strerror (errno));
		return;
	}
	
	sin.sin_family = AF_INET;
	sin.sin_port = htons (Ident_Port);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind (s, (struct sockaddr *)&sin, sizeof (sin)) == -1) {
		syslog (LOG_ERR, "error: binding to port %d: bind(): %s",
			Ident_Port, strerror (errno));
		fprintf (stderr, "error: binding to port %d: bind(): %s\n\n",
			Ident_Port, strerror (errno));
		return;
	}
	
	if (listen (s, 15) == -1) {
		syslog (LOG_ERR, "error: listening to port %d: listen(): %s",
				Ident_Port, strerror (errno));
		fprintf (stderr, "error: listening to port %d: listen(): %s\n",
				Ident_Port, strerror (errno));
		return;
	}

	fclose (stdin);
	fclose (stderr);
	fclose (stdout);

	syslog (LOG_NOTICE, "identity services started [%s]",
			ID_VERSION);
	d_core (s);

	syslog (LOG_ERR, "error: identity services terminated on "
		"internal error: See last (few) message(s)");

	closelog ();
	close (s);
	return;	
}

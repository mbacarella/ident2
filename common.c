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
 *	Common function definitions.
 *	Used without regulation by other modules
 */

#include "ident2.h"

void *
xmalloc (size_t nb)
{
	void *p = malloc (nb);
	
	if (p == NULL) {
		fprintf (stderr, "ident2: terminating on memory allocation error\n");
		syslog (LOG_ERR, "error: memory allocation error");
		exit (1);
	}
	return p;
}

	/*
	 *	a (skewed) fgets() that works on file descriptors
	 *	the '\r' charecter is ignored
	 *	returns the number of bytes written into the given
	 *	 buffer, including the terminating NUL
	 */
static int
_getl (int d, char *begin, u_short len)
{
	char *p, *end;

	end = &begin[len-1]; /* leave room for terminating NUL */
	for (p = begin; p < end; ++p) {
		if (read (d, p, 1) != 1)
			break;
		if (*p == '\n')
			break;
		if (*p == '\r')
			p--;	/* ignore \r */
	}
	*p++ = 0;
	return p-begin;
}

	/*
	 *	this function is called directly as an inetd
	 *	handler as well as a fork()ed into by the daemon
	 *	service of the server..
	 */
void 
child_service (int in, int out)
{
	char buf[ID_BUF_SIZE];

	signal (SIGALRM, exit);
	alarm (Client_Timeout);

	if (m_reduce_rights () == -1)
		syslog (LOG_ERR, "error: cannot reduce self's rights "
			"[m_reduce_rights()]");
	else
		while (_getl (in, buf, ID_BUF_SIZE) != 0)
			nexus (out, buf);

	close (in);
	if (out != in)
		close (out);
}


typedef struct cl_t {
	int			sd;
	uid_t			uid;
	unsigned short		lp, rp;
	struct sockaddr_in	laddr, raddr;
	struct passwd		*pw;
} cl_t;

	/*
	 *      send an ident formatted reply to client
	 */
static void
_clreply (cl_t *cl, char *rslt, char *info)
{
	char buf[ID_BUF_SIZE+1];

	buf[ID_BUF_SIZE] = 0;
	
	snprintf (buf, ID_BUF_SIZE-1, "%d , %d : %s : %s\r\n",
		cl->lp, cl->rp, rslt, info);
	write (cl->sd, buf, strlen (buf));
}


static int
_check_noident (cl_t *cl)
{
	char p[ID_BUF_SIZE+1];
	
	p[ID_BUF_SIZE] = 0;
	
	snprintf (p, ID_BUF_SIZE-1, "%s/%s", cl->pw->pw_dir, NOIDENT_FILE);
	
	if (access (p, F_OK) == 0) {
		_clreply (cl, "ERROR", "HIDDEN-USER");
		syslog (LOG_INFO, "no reply to %s (query %d, %d) due to %s",
				inet_ntoa (cl->raddr.sin_addr),
				cl->lp, cl->rp, p);
		return 0;
 	}
	return -1;
}

static int
_check_user_ident (cl_t *cl)
{
	FILE *fp;
	int retval = -1;
	char idfile[ID_BUF_SIZE+1];
	
	idfile[ID_BUF_SIZE] = 0;
	snprintf (idfile, ID_BUF_SIZE-1, "%s/%s",
		cl->pw->pw_dir, User_Ident_File);
	
	if (access (idfile, R_OK) == 0) {
		if ((fp = fopen (idfile, "r")) != NULL) {
			char *p, buf[4096];
			
			memset (buf, 0, 4096);	
			fgets (buf, 4095, fp);
			strtok (buf, "\r\n");
			
			if ((p = strstr (buf, "ident "))) {
				char rply[ID_BUF_SIZE+1];
				rply[ID_BUF_SIZE] = 0;
				
				snprintf (rply, ID_BUF_SIZE-1, "UNIX : %s", p+6);
				_clreply (cl, "USERID", rply);
				
				syslog (LOG_INFO, "sent reply `%s' to query %s "
					"(%d, %d), uid = %d", p,
					inet_ntoa (cl->raddr.sin_addr),
					cl->lp, cl->rp, cl->uid);
				retval = 0;
			}
			fclose (fp);		
		}
	}
	return retval;
}	

	/*
	 * handy for users who use an ip masqueraded setup that just
	 * want to friggin get on IRC and don't care what their ident
	 * is. Or if you want to annoy your IRC admins, because they
	 * totally deserve it.
	 */
static void
_send_random_reply (cl_t *cl)
{
	char randstr[RAND_STRING_LENGTH+1];
	char buf[ID_BUF_SIZE+1];
	size_t i;

	buf[ID_BUF_SIZE] = 0;
	
	srand (time(NULL));
	for (i = 0; i < RAND_STRING_LENGTH; i++) {
		randstr[i] = rand();
		while (randstr[i] > 'z')
			randstr[i] -= 26;
		while (randstr[i] < 'a')
			randstr[i] += 26;
	}
	randstr[RAND_STRING_LENGTH] = 0;
 	
 	snprintf (buf, ID_BUF_SIZE-1, "UNIX : %s", randstr);

	_clreply (cl, "USERID", buf);
	syslog (LOG_INFO, "sent random reply for query %s (%d, %d)",
		inet_ntoa (cl->raddr.sin_addr), cl->lp, cl->rp);
	
	return;
}
	/*
	 *	determines and collects client information
	 *	and returns it in a neat and conveniant package
	 */	
static cl_t *
_new_cl (int sd, char *line)
{
	int uid, ssiz = sizeof (struct sockaddr);
	cl_t *p, cl;
	char *s;
	
	cl.sd = sd;
	cl.lp = cl.rp = 0;
	
	if (!(s = strchr (line, ','))) {
		_clreply (&cl, "ERROR", "INVALID-PORT");
		return NULL;
	}		
	*s = 0;
	cl.lp = (u_short) atoi (line);
	cl.rp = (u_short) atoi (s+1);

	if (getsockname (sd, (struct sockaddr *)&cl.laddr, &ssiz) == -1) {
		syslog (LOG_WARNING, "warning: getsockname(): %s\n",
			strerror (errno));
                _clreply (&cl, "ERROR", "UNKNOWN-ERROR");
                return NULL;
	}
	if (getpeername (sd, (struct sockaddr *)&cl.raddr, &ssiz) == -1) {
		syslog (LOG_WARNING, "warning: getpeername(): %s\n",
			strerror (errno));
		_clreply (&cl, "ERROR", "UNKNOWN-ERROR");
		return NULL;
        }

	if (Reply_Always_Random == TRUE) {
		_send_random_reply (&cl);
		return NULL;
	}

	uid = m_get_uid (&cl.laddr.sin_addr, cl.lp, &cl.raddr.sin_addr, cl.rp);
	if (uid == -1) {
		syslog (LOG_WARNING, "warning: bad request: %d/%d, from %s",
			cl.lp, cl.rp, inet_ntoa (cl.raddr.sin_addr));
		_clreply (&cl, "ERROR", "NO-USER");
		return NULL;
	}
	cl.uid = uid;
		
	if ((cl.pw = getpwuid (cl.uid)) == NULL) {
		syslog (LOG_ERR, "warning: cannot map %d to a username: %s "
			"requested by query %s (%d, %d)", cl.uid,
				strerror (errno), inet_ntoa (cl.raddr.sin_addr),
				cl.lp, cl.rp);
		_clreply (&cl, "ERROR", "UNKNOWN-ERROR");
		return NULL;
	}
	p = xmalloc (sizeof (cl_t));
	*p = cl;
	
	return p;				
}

	/**
	 **	nexus
	 **
	 **	this is called by all service modes if
	 **	any reply is to be returned to the
	 **	client.
	 **/
void
nexus (int sd, char *line)
{
	cl_t *cl;
	char buf[ID_BUF_SIZE+1];

	buf[ID_BUF_SIZE] = 0;
	
	if ((cl = _new_cl (sd, line)) == NULL)
		return;

		/* if user configs are allowed, use the user's (if they
		 * have one, else just use the global config */
	if (Allow_NOIDENT == TRUE)
		if (!_check_noident (cl)) {
			free (cl);
			return;
		}
		
	if (Use_User_Ident == TRUE)
		if (!_check_user_ident (cl)) {
			free (cl);
			return;
		}
	
	snprintf (buf, ID_BUF_SIZE-1, "UNIX : %s", cl->pw->pw_name);
	
	_clreply (cl, "USERID", buf);
	syslog (LOG_INFO, "sent reply `%s' to query %s "
			"(%d, %d), uid = %d", cl->pw->pw_name,
			inet_ntoa (cl->raddr.sin_addr), cl->lp,
			cl->rp, cl->uid);

	free (cl);
	return;		
}


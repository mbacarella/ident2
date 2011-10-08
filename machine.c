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
 */

/*
 *	Support for Linux machine dependancies
 */

#include "ident2.h"

#define PID_FILE "/var/run/ident2.pid"

	/**
	 **	drop to the lowest permission level
	 **	possible. 'nobody' is ideal for linux
	 **/
int
m_reduce_rights (void)
{
	struct passwd *pw;

	if ((geteuid() && getuid())
	|| Dont_Change_Uid == TRUE)
		return 0;

	if ((pw = getpwnam ("nobody")) == NULL) {
		syslog (LOG_ERR, "error: getpwnam(nobody): %s",
				strerror (errno));
		return -1;
	} 
	if (setuid (pw->pw_uid) == -1) {
		syslog (LOG_ERR, "error: setuid(%d): %s",
			pw->pw_uid, strerror (errno));
		return -1;
	}
	return 0;
}


	/**
	 **	find what user belongs to the connection
	 **	described by LPORT, RPORT, RADDR, and LADDR.
	 **	return the uid.
	 **/	
int
m_get_uid (struct in_addr *laddr, u_short lp,
	struct in_addr *raddr, u_short rp)
{
	FILE	*fp;
	char	buf[150];

	if ((fp = fopen ("/proc/net/tcp", "r")) == NULL) {
		syslog (LOG_ERR, "error reading /proc/net/tcp: %s",
				strerror (errno));
		return -1;
	}

	fgets (buf, 149, fp);		/* eat header!)*$ */

	while (fgets (buf, 149, fp)) {
		unsigned long local_addr, remote_addr;
		unsigned long tx_queue, rx_queue, tm_when;
		int sl, uid, retrnsmt, st, tr, local_port, remote_port;
		
		if (sscanf (buf, "%d: %lX:%x %lX:%x %x %lX:%lX %x:%lX %x %d",

			&sl, &local_addr, &local_port, &remote_addr,
			&remote_port, &st, &tx_queue, &rx_queue,
			&tr, &tm_when, &retrnsmt, &uid) == 12) {

			if (lp == local_port && rp == remote_port
			&& remote_addr == raddr->s_addr) {
				if (laddr == NULL) {
					fclose (fp);
					return uid;
				}
				else if (laddr->s_addr
				== local_addr) {
					fclose (fp);
					return uid;
				}
			}
		}
	}
	fclose (fp);
	return -1;
}

	/*
	 *	records the pid for service management purposes.
	 *	example: under Red Hat,Debian,etc pid is written to
	 *	/var/run/identd.pid
	 *	PID support suggested (and previously implemented)
	 *	by Alexander Reelsen.
	 */
int
m_register_pid (void)
{
#ifdef HAS_VAR_RUN
	FILE	*fp;

	if ((fp = fopen (PID_FILE, "w")) == NULL) {
		syslog (LOG_WARNING, "couldn't record pid in %s: %s -- "
			"automatic shutdown with system not available",
			PID_FILE, strerror (errno));
		return -1;
	}
	fprintf (fp, "%u\n", getpid ()); 
	fclose (fp);
#endif	
	return 0;
}

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
 *	Support for FreeBSD machine dependancies
 */

/*
 *	NOTE: This supports only FreeBSD versions greater than 3.0!
 *		I do not deem versions less than 3.0 as worth supporting
 *		because it requires more code as well as decreased security
 *		for this ident server to function.
 */

#include "ident2.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/protosw.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_debug.h>

#include <arpa/inet.h>
#include <err.h>


#define PID_FILE "/var/run/ident2.pid"

	/**
	 **	drop to the lowest permission level
	 **	possible. 'nobody' is ideal for FreeBSD
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
	char *mibvar = "net.inet.tcp.pcblist";
	char *buf;
	struct tcpcb *tp;
	struct inpcb *inp;
	struct xinpgen *xig, *oxig;
	struct xsocket *so;
	size_t len;

	if (sysctlbyname (mibvar, 0, &len, 0, 0) < 0) {
		syslog (LOG_WARNING, "sysctl: %s: %s\n", mibvar,
				strerror (errno));
		return -1;
	}
	buf = xmalloc (len);
	
	if (sysctlbyname (mibvar, buf, &len, 0, 0) < 0) {
		syslog (LOG_WARNING, "sysctl: %s: %s\n", mibvar,
				strerror (errno));
		free (buf);
		return -1;
	}
	
	oxig = xig = (struct xinpgen *)buf;
	for (xig = (struct xinpgen *)((char *)xig + xig->xig_len);
			xig->xig_len > sizeof(struct xinpgen);
			xig = (struct xinpgen *)((char *)xig + xig->xig_len)) {
		tp = &((struct xtcpcb *)xig)->xt_tp;
		inp = &((struct xtcpcb *)xig)->xt_inp;
		so = &((struct xtcpcb *)xig)->xt_socket;
		if (so->xso_protocol != IPPROTO_TCP)    continue;
		if (inp->inp_gencnt > oxig->xig_gen)    continue;
		if (inet_lnaof(inp->inp_laddr) == INADDR_ANY)
			continue;

		if ((raddr->s_addr) == inp->inp_faddr.s_addr
		&&  (laddr->s_addr) == inp->inp_laddr.s_addr
		&& rp == ntohs (inp->inp_fport)
		&& lp == ntohs (inp->inp_lport)) {
            int uid = so->so_uid;
			free (buf);
			return uid;
		}	
	}
	free (buf);
	return -1;
}

	/*
	 *	FreeBSD seems to like recording this information
	 *	to /var/run. Good. nice and standard.
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

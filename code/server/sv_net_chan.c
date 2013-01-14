/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/*
=================
SV_Netchan_FreeQueue
=================
*/
void SV_Netchan_FreeQueue(client_t *client)
{
	netchan_buffer_t *netbuf, *next;
	
	for(netbuf = client->netchan_start_queue; netbuf; netbuf = next)
	{
		next = netbuf->next;
		Z_Free(netbuf);
	}
	
	client->netchan_start_queue = NULL;
	client->netchan_end_queue = &client->netchan_start_queue;
}

/*
=================
SV_Netchan_TransmitNextInQueue
=================
*/
void SV_Netchan_TransmitNextInQueue(client_t *client)
{
	netchan_buffer_t *netbuf;
		
	Com_DPrintf("#462 Netchan_TransmitNextFragment: popping a queued message for transmit\n");
	netbuf = client->netchan_start_queue;

	Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg.data);

	// pop from queue
	client->netchan_start_queue = netbuf->next;
	if(!client->netchan_start_queue)
	{
		Com_DPrintf("#462 Netchan_TransmitNextFragment: emptied queue\n");
		client->netchan_end_queue = &client->netchan_start_queue;
	}
	else
		Com_DPrintf("#462 Netchan_TransmitNextFragment: remaining queued message\n");

	Z_Free(netbuf);
}

/*
=================
SV_Netchan_TransmitNextFragment
Transmit the next fragment and the next queued packet
Return number of ms until next message can be sent based on throughput given by client rate,
-1 if no packet was sent.
=================
*/

int SV_Netchan_TransmitNextFragment(client_t *client)
{
	if(client->netchan.unsentFragments)
	{
		Netchan_TransmitNextFragment(&client->netchan);
		return SV_RateMsec(client);
	}
	else if(client->netchan_start_queue)
	{
		SV_Netchan_TransmitNextInQueue(client);
		return SV_RateMsec(client);
	}
	
	return -1;
}


/*
===============
SV_Netchan_Transmit
TTimo
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=462
if there are some unsent fragments (which may happen if the snapshots
and the gamestate are fragmenting, and collide on send for instance)
then buffer them and make sure they get sent in correct order
================
*/

void SV_Netchan_Transmit( client_t *client, msg_t *msg)
{
	MSG_WriteByte( msg, svc_EOF );

	if(client->netchan.unsentFragments || client->netchan_start_queue)
	{
		netchan_buffer_t *netbuf;
		Com_DPrintf("#462 SV_Netchan_Transmit: unsent fragments, stacked\n");
		netbuf = (netchan_buffer_t *) Z_Malloc(sizeof(netchan_buffer_t));
		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		MSG_Copy(&netbuf->msg, netbuf->msgBuffer, sizeof( netbuf->msgBuffer ), msg);
		netbuf->next = NULL;
		// insert it in the queue, the message will be encoded and sent later
		*client->netchan_end_queue = netbuf;
		client->netchan_end_queue = &(*client->netchan_end_queue)->next;
	}
	else
	{
		Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
	}
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process( client_t *client, msg_t *msg ) {
	int ret;
	ret = Netchan_Process( &client->netchan, msg );
	if (!ret)
		return qfalse;

	return qtrue;
}


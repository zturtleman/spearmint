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
// sv_client.c -- server code for dealing with clients

#include "server.h"

static void SV_CloseDownload( client_t *cl );

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

ioquake3: we added a possibility for clients to add a challenge
to their packets, to make it more difficult for malicious servers
to hi-jack client connections.
=================
*/
void SV_GetChallenge(netadr_t from)
{
	int		i;
	int		oldest;
	int		oldestTime;
	int		oldestClientTime;
	int		clientChallenge;
	challenge_t	*challenge;
	qboolean wasfound = qfalse;
	char *gameName;
	qboolean gameMismatch;

	// Don't allow players to connect if sv_public is -2
	if ( sv_public->integer <= -2 ) {
		return;
	}

	// ignore if we are in single player
	if ( Com_GameIsSinglePlayer() ) {
		return;
	}

	// Prevent using getchallenge as an amplifier
	if ( SVC_RateLimitAddress( from, 10, 1000 ) ) {
		Com_DPrintf( "SV_GetChallenge: rate limit from %s exceeded, dropping request\n",
			NET_AdrToString( from ) );
		return;
	}

	// Allow getchallenge to be DoSed relatively easily, but prevent
	// excess outbound bandwidth usage when being flooded inbound
	if ( SVC_RateLimit( &outboundLeakyBucket, 10, 100 ) ) {
		Com_DPrintf( "SV_GetChallenge: rate limit exceeded, dropping request\n" );
		return;
	}

	gameName = Cmd_Argv(2);
	gameMismatch = !*gameName || strcmp(gameName, com_gamename->string) != 0;

	// reject client if the gamename string sent by the client doesn't match ours
	if (gameMismatch)
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\nGame mismatch: This is a %s server\n",
			com_gamename->string);
		return;
	}

	oldest = 0;
	oldestClientTime = oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	challenge = &svs.challenges[0];
	clientChallenge = atoi(Cmd_Argv(1));

	for(i = 0 ; i < MAX_CHALLENGES ; i++, challenge++)
	{
		if(!challenge->connected && NET_CompareAdr(from, challenge->adr))
		{
			wasfound = qtrue;
			
			if(challenge->time < oldestClientTime)
				oldestClientTime = challenge->time;
		}
		
		if(wasfound && i >= MAX_CHALLENGES_MULTI)
		{
			i = MAX_CHALLENGES;
			break;
		}
		
		if(challenge->time < oldestTime)
		{
			oldestTime = challenge->time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// this is the first time this client has asked for a challenge
		challenge = &svs.challenges[oldest];
		challenge->clientChallenge = clientChallenge;
		challenge->adr = from;
		challenge->connected = qfalse;
	}

	// always generate a new challenge number, so the client cannot circumvent sv_maxping
	challenge->challenge = ( ((unsigned int)rand() << 16) ^ (unsigned int)rand() ) ^ svs.time;
	challenge->wasrefused = qfalse;
	challenge->time = svs.time;

	challenge->pingTime = svs.time;
	NET_OutOfBandPrint(NS_SERVER, challenge->adr, "challengeResponse %d %d %d",
			   challenge->challenge, clientChallenge, com_protocol->integer);
}

/*
==================
SV_IsBanned

Check whether a certain address is banned
==================
*/
static qboolean SV_IsBanned(netadr_t *from, qboolean isexception)
{
	int index;
	serverBan_t *curban;
	
	if(!isexception)
	{
		// If this is a query for a ban, first check whether the client is excepted
		if(SV_IsBanned(from, qtrue))
			return qfalse;
	}
	
	for(index = 0; index < serverBansCount; index++)
	{
		curban = &serverBans[index];
		
		if(curban->isexception == isexception)
		{
			if(NET_CompareBaseAdrMask(curban->ip, *from, curban->subnet))
				return qtrue;
		}
	}
	
	return qfalse;
}

/*
==================
SV_AddPlayer

Add a player, either at connect or mid-game.
==================
*/
void SV_AddPlayer( client_t *client, int localPlayerNum, const char *infoString ) {
	char		userinfo[MAX_INFO_STRING];
	int			i;
	player_t	*player, *newplayer;
	player_t	temp;
	int			playerNum;
	char		*ip;
	char		*password;
	int			startIndex;
	intptr_t	denied;

	if ( client->localPlayers[localPlayerNum] ) {
		return;
	}

	if ( strlen(infoString) <= 0 ) {
		// Ignore dummy userinfo string.
		return;
	}

	Q_strncpyz( userinfo, infoString, sizeof(userinfo) );

	// don't let "ip" overflow userinfo string
	if ( NET_IsLocalAddress ( client->netchan.remoteAddress ) )
		ip = "localhost";
	else
		ip = (char *)NET_AdrToString( client->netchan.remoteAddress );
	if( ( strlen( ip ) + strlen( userinfo ) + 4 ) >= MAX_INFO_STRING ) {
		NET_OutOfBandPrint( NS_SERVER, client->netchan.remoteAddress,
				"print \"Userinfo string length exceeded.  "
				"Try removing setu%s cvars from your config.\n\"", (localPlayerNum == 0 ? "" : va( "%d", localPlayerNum+1 ) ) );
		return;
	}
	Info_SetValueForKey( userinfo, "ip", ip );

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey( userinfo, "password" );
	if ( !strcmp( password, sv_privatePassword->string ) ) {
		startIndex = 0;
	} else {
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newplayer = NULL;
	for ( i = startIndex; i < sv_maxclients->integer ; i++ ) {
		player = &svs.players[i];
		if (!player->inUse) {
			newplayer = player;
			break;
		}
	}

	if (!newplayer) {
		NET_OutOfBandPrint( NS_SERVER, client->netchan.remoteAddress, "print\nCouldn't add local player %d, no free player slots!\n", localPlayerNum + 1 );
		return;
	}

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	Com_Memset (&temp, 0, sizeof(player_t));
	*newplayer = temp;
	playerNum = newplayer - svs.players;

	newplayer->inUse = qtrue;

	newplayer->client = client;
	client->localPlayers[localPlayerNum] = newplayer;

	// save the userinfo
	Q_strncpyz( newplayer->userinfo, userinfo, sizeof(newplayer->userinfo) );

	// setup entity before connecting
	SV_SetupPlayerEntity( newplayer );

	// get the game a chance to reject this connection or modify the userinfo
	denied = VM_Call( gvm, GAME_PLAYER_CONNECT, playerNum, qtrue, qfalse, client - svs.clients, localPlayerNum ); // firstTime = qtrue
	if ( denied ) {
		// we can't just use VM_ArgPtr, because that is only valid inside a VM_Call
		char *str = VM_ExplicitArgPtr( gvm, denied );

		NET_OutOfBandPrint( NS_SERVER, client->netchan.remoteAddress, "print\nRejected player: %s\n", str );
		Com_DPrintf ("Game rejected a player: %s.\n", str);

		// Free all allocated data on the client structure
		SV_FreePlayer( newplayer );

		// Nuke user info
		SV_SetUserinfo( playerNum, "" );

		// No longer in use
		newplayer->inUse = qfalse;

		// Update connection
		client->localPlayers[localPlayerNum] = NULL;
		return;
	}

	SV_UserinfoChanged( newplayer );

	if ( client->state == CS_ACTIVE ) {
		SV_PlayerEnterWorld( newplayer, NULL );
	}
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/

void SV_DirectConnect( netadr_t from ) {
	char		userinfo[MAX_INFO_STRING];
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	int			version;
	int			qport;
	int			challenge;
	char		*password;
	int			startIndex;
	int			count;
	int			playerCount;
	int			maxLocalPlayers;
#ifdef LEGACY_PROTOCOL
	qboolean	compat = qfalse;
#endif

	Com_DPrintf ("SVC_DirectConnect ()\n");
	
	// Check whether this client is banned.
	if(SV_IsBanned(&from, qfalse))
	{
		NET_OutOfBandPrint(NS_SERVER, from, "print\nYou are banned from this server.\n");
		return;
	}

	// Client sends a userinfo string for each local player they want.
	// However, empty userinfos can be sent to skip a local player.
	maxLocalPlayers = Com_Clamp( 1, MAX_SPLITVIEW, Cmd_Argc()-1 );

	// skip blank userinfos
	for ( i = 0; i < maxLocalPlayers; i++ ) {
		Q_strncpyz( userinfo, Cmd_Argv( 1 + i ), sizeof(userinfo) );
		if ( strlen( userinfo ) > 0 ) {
			break;
		}
	}

	version = atoi(Info_ValueForKey(userinfo, "protocol"));
	
#ifdef LEGACY_PROTOCOL
	if(version > 0 && com_legacyprotocol->integer == version)
		compat = qtrue;
	else
#endif
	{
		if(version != com_protocol->integer)
		{
			NET_OutOfBandPrint(NS_SERVER, from, "print\nServer uses protocol version %i "
					   "(yours is %i).\n", com_protocol->integer, version);
			Com_DPrintf("    rejected connect from version %i\n", version);
			return;
		}
	}

	challenge = atoi( Info_ValueForKey( userinfo, "challenge" ) );
	qport = atoi( Info_ValueForKey( userinfo, "qport" ) );

	// quick reject
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			&& ( cl->netchan.qport == qport 
			|| from.port == cl->netchan.remoteAddress.port ) ) {
			if (( svs.time - cl->lastConnectTime) 
				< (sv_reconnectlimit->integer * 1000)) {
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (from));
				return;
			}
			break;
		}
	}
	
	// see if the challenge is valid (LAN clients don't need to challenge)
	if (!NET_IsLocalAddress(from))
	{
		int ping;
		challenge_t *challengeptr;

		for (i=0; i<MAX_CHALLENGES; i++)
		{
			if (NET_CompareAdr(from, svs.challenges[i].adr))
			{
				if(challenge == svs.challenges[i].challenge)
					break;
			}
		}

		if (i == MAX_CHALLENGES)
		{
			NET_OutOfBandPrint( NS_SERVER, from, "print\nNo or bad challenge for your address.\n" );
			return;
		}
	
		challengeptr = &svs.challenges[i];
		
		if(challengeptr->wasrefused)
		{
			// Return silently, so that error messages written by the server keep being displayed.
			return;
		}

		ping = svs.time - challengeptr->pingTime;

		// never reject a LAN client based on ping
		if ( !Sys_IsLANAddress( from ) ) {
			if ( sv_minPing->value && ping < sv_minPing->value ) {
				NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is for high pings only\n" );
				Com_DPrintf ("Client %i rejected on a too low ping\n", i);
				challengeptr->wasrefused = qtrue;
				return;
			}
			if ( sv_maxPing->value && ping > sv_maxPing->value ) {
				NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is for low pings only\n" );
				Com_DPrintf ("Client %i rejected on a too high ping\n", i);
				challengeptr->wasrefused = qtrue;
				return;
			}
		}

		Com_Printf("Client %i connecting with %i challenge ping\n", i, ping);
		challengeptr->connected = qtrue;
	}

	newcl = &temp;
	Com_Memset (newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			&& ( cl->netchan.qport == qport 
			|| from.port == cl->netchan.remoteAddress.port ) ) {
			Com_Printf ("%s:reconnect\n", NET_AdrToString (from));
			newcl = cl;

			//
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey( userinfo, "password" );
	if ( *password && !strcmp( password, sv_privatePassword->string ) ) {
		startIndex = 0;
	} else {
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newcl = NULL;
	for ( i = startIndex, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if ( cl->state == CS_FREE ) {
			newcl = cl;
			break;
		}
	}

	if ( !newcl ) {
		if ( NET_IsLocalAddress( from ) ) {
			count = 0;
			for ( i = startIndex, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
				if (cl->netchan.remoteAddress.type == NA_BOT) {
					count++;
				}
			}
			// if they're all bots
			if (count >= sv_maxclients->integer - startIndex) {
				SV_DropClient(&svs.clients[sv_maxclients->integer - 1], "only bots on server");
				newcl = &svs.clients[sv_maxclients->integer - 1];
			}
			else {
				Com_Error( ERR_FATAL, "server is full on local connect" );
				return;
			}
		}
		else {
			NET_OutOfBandPrint( NS_SERVER, from, "print\nServer is full.\n" );
			Com_DPrintf ("Rejected a connection.\n");
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->reliableAcknowledge = 0;
	cl->reliableSequence = 0;

gotnewcl:	
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;

	// save the challenge
	newcl->challenge = challenge;

	// save the address
#ifdef LEGACY_PROTOCOL
	newcl->compat = compat;
	Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport, challenge, compat);
#else
	Netchan_Setup(NS_SERVER, &newcl->netchan, from, qport, challenge, qfalse);
#endif
	// init the netchan queue
	newcl->netchan_end_queue = &newcl->netchan_start_queue;

	// add the players
	for (i = 0; i < maxLocalPlayers; i++) {
		SV_AddPlayer( newcl, i, Cmd_Argv( 1 + i ) );
	}

	// check if all players were rejected
	if ( SV_ClientNumLocalPlayers( newcl ) < 1 ) {
		return;
	}

	// send the connect packet to the client
	NET_OutOfBandPrint(NS_SERVER, from, "connectResponse %d", challenge);

	Com_DPrintf( "Going from CS_FREE to CS_CONNECTED for %s\n", SV_ClientName( newcl ) );

	newcl->state = CS_CONNECTED;
	newcl->lastSnapshotTime = 0;
	newcl->lastPacketTime = svs.time;
	newcl->lastConnectTime = svs.time;
	newcl->needBaseline = qtrue;
	
	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->gamestateMessageNum = -1;

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	count = playerCount = 0;
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
			playerCount += SV_ClientNumLocalPlayers( &svs.clients[i] );
		}
	}
	if ( count == 1 || playerCount == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}

/*
=====================
SV_FreePlayer

Destructor for data allocated in a player structure
=====================
*/
void SV_FreePlayer( player_t *player )
{

}

/*
=====================
SV_FreeClient

Destructor for data allocated in a client structure
=====================
*/
void SV_FreeClient(client_t *client)
{
	int index;

	for (index = 0; index < ARRAY_LEN(client->frames); index++) {
		DA_Free( &client->frames[index].playerStates );
	}

#ifdef USE_VOIP
	for(index = client->queuedVoipIndex; index < client->queuedVoipPackets; index++)
	{
		index %= ARRAY_LEN(client->voipPacket);
		
		Z_Free(client->voipPacket[index]);
	}
	
	client->queuedVoipPackets = 0;
#endif

	SV_Netchan_FreeQueue(client);
	SV_CloseDownload(client);

	client->state = CS_FREE;
}

/*
=====================
SV_ClientNumLocalPlayers
=====================
*/
int SV_ClientNumLocalPlayers( client_t *client ) {
	int i, numPlayers = 0;

	for ( i = 0 ; i < MAX_SPLITVIEW ; i++ ) {
		if ( client->localPlayers[i] ) {
			numPlayers++;
		}
	}
	
	return numPlayers;
}

/*
==================
SV_ClientNumLocalPlayersInWorld
==================
*/
int SV_ClientNumLocalPlayersInWorld( client_t *client ) {
	int i, numPlayers = 0;

	for ( i = 0 ; i < MAX_SPLITVIEW ; i++ ) {
		if ( client->localPlayers[i] && client->localPlayers[i]->inWorld ) {
			numPlayers++;
		}
	}
	
	return numPlayers;
}

/*
==================
SV_ClientName
==================
*/
const char *SV_ClientName( client_t *client ) {
	static char name[MAX_NAME_LENGTH];
	int i;

	for ( i = 0; i < MAX_SPLITVIEW; i++ ) {
		if ( client->localPlayers[i] && client->localPlayers[i]->name[0] != '\0' )
			return client->localPlayers[i]->name;
	}

	Com_sprintf( name, sizeof (name), "client %d", (int)(client - svs.clients) );
	return name;
}

/*
==================
SV_LocalPlayerNum
==================
*/
int	SV_LocalPlayerNum( player_t *player ) {
	int i;

	for ( i = 0; i < MAX_SPLITVIEW; i++ ) {
		if ( player == player->client->localPlayers[i])
			return i;
	}

	Com_Error(ERR_FATAL, "player not found in client's localPlayers list" );
	return -1;
}

/*
=====================
SV_DropPlayer

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalMessage() will handle that
=====================
*/
void SV_DropPlayer( player_t *drop, const char *reason, qboolean force ) {
	int			i;
	challenge_t	*challenge;
	int			numLocalPlayers;
	int			playerNum = drop - svs.players;
	client_t	*client = drop->client;
	const qboolean isBot = client->netchan.remoteAddress.type == NA_BOT;

	if ( client->state == CS_ZOMBIE ) {
		return;		// already dropped
	}

	numLocalPlayers = SV_ClientNumLocalPlayers( client );

	if ( NET_IsLocalAddress( client->netchan.remoteAddress ) && numLocalPlayers == 1 ) {
		// allowing tthis would cause the server to keep running with disconnected local players that cannot rejoin
		return;
	}

	// call the game vm function for removing a player
	// it will remove the body, among other things
	// allow the game to reject splitscreen player disconnect
	if ( !VM_Call( gvm, GAME_PLAYER_DISCONNECT, playerNum, force || numLocalPlayers == 1 ) && !( force || numLocalPlayers == 1 ) ) {
		return;
	}

	if ( !isBot ) {
		// see if we already have a challenge for this ip
		challenge = &svs.challenges[0];

		for (i = 0 ; i < MAX_CHALLENGES ; i++, challenge++)
		{
			if(NET_CompareAdr(client->netchan.remoteAddress, challenge->adr))
			{
				Com_Memset(challenge, 0, sizeof(*challenge));
				break;
			}
		}
	}

	// Free all allocated data on the client structure
	SV_FreePlayer( drop );

	// tell everyone why they got dropped
	if ( reason ) {
		SV_SendServerCommand( NULL, -1, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason );
	}

	// Check if player is a extra local player
	if ( numLocalPlayers > 1 ) {
		for (i = 0; i < MAX_SPLITVIEW; i++) {
			if ( client->localPlayers[i] == drop ) {
				client->localPlayers[i] = NULL;
			}
		}
	} else {
		// add the disconnect command
		if ( reason ) {
			SV_SendServerCommand( client, -1, "disconnect \"%s\"", reason);
		} else {
			SV_SendServerCommand( client, -1, "disconnect");
		}
	}

	//
	drop->inUse = qfalse;

	if ( isBot ) {
		SV_BotFreeClient( playerNum );

		// bots shouldn't go zombie, as there's no real net connection.
		client->state = CS_FREE;
	} else if ( numLocalPlayers == 1 ) {
		Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
		client->state = CS_ZOMBIE;		// become free in a few seconds
	}

	// nuke user info
	SV_SetUserinfo( playerNum, "" );

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i=0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			break;
		}
	}
	if ( i == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}

/*
================
SV_DropClient
================
*/
void SV_DropClient( client_t *cl, const char *reason ) {
	int i;

	if ( cl->netchan.remoteAddress.type == NA_LOOPBACK ) {
		return;
	}

	for ( i = 0; i < MAX_SPLITVIEW; i++ ) {
		if ( !cl->localPlayers[i] ) {
			continue;
		}

		SV_DropPlayer( cl->localPlayers[i], reason, qtrue );
	}
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
static void SV_SendClientGameState( client_t *client ) {
	int			start;
	msg_t		msg;
	byte		msgBuffer[MAX_MSGLEN];
	int			i;

	Com_DPrintf ("SV_SendClientGameState() for %s\n", SV_ClientName(client));
	Com_DPrintf( "Going from CS_CONNECTED to CS_PRIMED for %s\n", SV_ClientName(client));
	client->state = CS_PRIMED;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, &msg );

	// send the gamestate
	MSG_WriteByte( &msg, svc_gamestate );
	MSG_WriteLong( &msg, client->reliableSequence );

	// write the configstrings
	for ( start = 0 ; start < MAX_CONFIGSTRINGS ; start++ ) {
		if (sv.configstrings[start].s[0]) {
			MSG_WriteByte( &msg, svc_configstring );
			MSG_WriteShort( &msg, start );
			MSG_WriteBigString( &msg, sv.configstrings[start].s );
		}
	}

	MSG_WriteByte( &msg, svc_EOF );

	for ( i = 0; i < MAX_SPLITVIEW; i++ ) {
		if (client->localPlayers[i]) {
			MSG_WriteLong( &msg, client->localPlayers[i] - svs.players );
		} else {
			MSG_WriteLong( &msg, -1);
		}
	}

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}


/*
==================
SV_SetupPlayerEntity

Call before GAME_PLAYER_CONNECT
==================
*/
void SV_SetupPlayerEntity( player_t *player ) {
	int		playerNum;
	sharedEntity_t *ent;

	player->inWorld = qfalse;

	// set up the entity for the client
	playerNum = player - svs.players;
	ent = SV_GentityNum( playerNum );
	ent->s.number = playerNum;
	player->gentity = ent;
}

/*
==================
SV_PlayerEnterWorld
==================
*/
void SV_PlayerEnterWorld( player_t *player, usercmd_t *cmd ) {
	client_t *client = player->client;

	if ( client->state == CS_PRIMED )
	{
		Com_DPrintf( "Going from CS_PRIMED to CS_ACTIVE for %s\n", player->name );
		client->state = CS_ACTIVE;

		// resend all configstrings using the cs commands since these are
		// no longer sent when the client is CS_PRIMED
		SV_UpdateConfigstrings( client );

		client->deltaMessage = -1;
		client->lastSnapshotTime = 0;	// generate a snapshot immediately
	}

	player->inWorld = qtrue;

	if(cmd)
		memcpy(&player->lastUsercmd, cmd, sizeof(player->lastUsercmd));
	else
		memset(&player->lastUsercmd, '\0', sizeof(player->lastUsercmd));

	// call the game begin function
	VM_Call( gvm, GAME_PLAYER_BEGIN, player - svs.players );
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload( client_t *cl ) {
	int i;

	// EOF
	if (cl->download) {
		FS_FCloseFile( cl->download );
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++) {
		if (cl->downloadBlocks[i]) {
			Z_Free(cl->downloadBlocks[i]);
			cl->downloadBlocks[i] = NULL;
		}
	}

}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
static void SV_StopDownload_f( client_t *cl ) {
	if (*cl->downloadName)
		Com_DPrintf( "clientDownload: %d : file \"%s\" aborted\n", (int) (cl - svs.clients), cl->downloadName );

	SV_CloseDownload( cl );
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
static void SV_DoneDownload_f( client_t *cl ) {
	if ( cl->state == CS_ACTIVE )
		return;

	Com_DPrintf( "clientDownload: %s Done\n", SV_ClientName( cl ) );
	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState(cl);
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
static void SV_NextDownload_f( client_t *cl )
{
	int block = atoi( Cmd_Argv(1) );

	if (block == cl->downloadClientBlock) {
		Com_DPrintf( "clientDownload: %d : client acknowledge of block %d\n", (int) (cl - svs.clients), block );

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0) {
			Com_Printf( "clientDownload: %d : file \"%s\" completed\n", (int) (cl - svs.clients), cl->downloadName );
			SV_CloseDownload( cl );
			return;
		}

		cl->downloadSendTime = svs.time;
		cl->downloadClientBlock++;
		return;
	}
	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//			because the cgame isn't loaded yet
	SV_DropClient( cl, "broken download" );
}

/*
==================
SV_BeginDownload_f
==================
*/
static void SV_BeginDownload_f( client_t *cl ) {

	// Kill any existing download
	SV_CloseDownload( cl );

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz( cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName) );
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data, return number of download blocks added
==================
*/
int SV_WriteDownloadToClient(client_t *cl, msg_t *msg)
{
	int curindex;
	int unreferenced = 1;
	char errorMessage[1024];
	char pakbuf[MAX_QPATH], *pakptr;
	int numRefPaks;

	if (!*cl->downloadName)
		return 0;	// Nothing being downloaded

	if(!cl->download)
	{
		pakType_t pakType = PAK_UNKNOWN;
	
 		// Chop off filename extension.
		Com_sprintf(pakbuf, sizeof(pakbuf), "%s", cl->downloadName);
		pakptr = strrchr(pakbuf, '.');
		
		if(pakptr)
		{
			*pakptr = '\0';

			// Check for pk3 filename extension
			if(!Q_stricmp(pakptr + 1, "pk3"))
			{
				const char *referencedPaks = FS_ReferencedPakNames();

				// Check whether the file appears in the list of referenced
				// paks to prevent downloading of arbitrary files.
				Cmd_TokenizeStringIgnoreQuotes(referencedPaks);
				numRefPaks = Cmd_Argc();

				for(curindex = 0; curindex < numRefPaks; curindex++)
				{
					if(!FS_FilenameCompare(Cmd_Argv(curindex), pakbuf))
					{
						unreferenced = 0;

						// now that we know the file is referenced,
						// check whether it's legal to download it
						// or if it is a default pak.
						pakType = FS_ReferencedPakType( Cmd_Argv(curindex), FS_ReferencedPakChecksum( curindex ), NULL );
						break;
					}
				}
			}
		}

		cl->download = 0;

		// We open the file here
		if ( !(sv_allowDownload->integer & DLF_ENABLE) ||
			(sv_allowDownload->integer & DLF_NO_UDP) ||
			pakType != PAK_FREE || unreferenced ||
			( cl->downloadSize = FS_SV_FOpenFileRead( cl->downloadName, &cl->download ) ) < 0 ) {
			// cannot auto-download file
			if(unreferenced)
			{
				Com_Printf("clientDownload: %d : \"%s\" is not referenced and cannot be downloaded.\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" is not referenced and cannot be downloaded.", cl->downloadName);
			}
			else if ( pakType == PAK_COMMERCIAL )
			{
				Com_Printf("clientDownload: %d : \"%s\" cannot download commercial pk3 files\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload commercial pk3 file \"%s\"", cl->downloadName);
			}
			else if ( pakType == PAK_NO_DOWNLOAD )
			{
				// Don't auto download default pk3s
				Com_Printf("clientDownload: %d : \"%s\" cannot download a pk3 file\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload pk3 file \"%s\"", cl->downloadName);
			}
			else if ( !(sv_allowDownload->integer & DLF_ENABLE) ||
				(sv_allowDownload->integer & DLF_NO_UDP) ) {

				Com_Printf("clientDownload: %d : \"%s\" download disabled\n", (int) (cl - svs.clients), cl->downloadName);
				if (sv_pure->integer) {
					Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
										"You will need to get this file elsewhere before you "
										"can connect to this pure server.\n", cl->downloadName);
				} else {
					Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                    "The server you are connecting to is not a pure server, "
                    "set autodownload to No in your settings and you might be "
                    "able to join the game anyway.\n", cl->downloadName);
				}
			} else {
        // NOTE TTimo this is NOT supposed to happen unless bug in our filesystem scheme?
        //   if the pk3 is referenced, it must have been found somewhere in the filesystem
				Com_Printf("clientDownload: %d : \"%s\" file not found on server\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName);
			}
			MSG_WriteByte( msg, svc_download );
			MSG_WriteShort( msg, 0 ); // client is expecting block zero
			MSG_WriteLong( msg, -1 ); // illegal file size
			MSG_WriteString( msg, errorMessage );

			*cl->downloadName = 0;
			
			if(cl->download)
				FS_FCloseFile(cl->download);
			
			return 1;
		}
 
		Com_Printf( "clientDownload: %d : beginning \"%s\"\n", (int) (cl - svs.clients), cl->downloadName );
		
		// Init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = qfalse;
	}

	// Perform any reads that we need to
	while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW &&
		cl->downloadSize != cl->downloadCount) {

		curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

		if (!cl->downloadBlocks[curindex])
			cl->downloadBlocks[curindex] = Z_Malloc(MAX_DOWNLOAD_BLKSIZE);

		cl->downloadBlockSize[curindex] = FS_Read( cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download );

		if (cl->downloadBlockSize[curindex] < 0) {
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[curindex];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if (cl->downloadCount == cl->downloadSize &&
		!cl->downloadEOF &&
		cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW) {

		cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = qtrue;  // We have added the EOF block
	}

	if (cl->downloadClientBlock == cl->downloadCurrentBlock)
		return 0; // Nothing to transmit

	// Write out the next section of the file, if we have already reached our window,
	// automatically start retransmitting
	if (cl->downloadXmitBlock == cl->downloadCurrentBlock)
	{
		// We have transmitted the complete window, should we start resending?
		if (svs.time - cl->downloadSendTime > 1000)
			cl->downloadXmitBlock = cl->downloadClientBlock;
		else
			return 0;
	}

	// Send current block
	curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

	MSG_WriteByte( msg, svc_download );
	MSG_WriteShort( msg, cl->downloadXmitBlock );

	// block zero is special, contains file size
	if ( cl->downloadXmitBlock == 0 )
		MSG_WriteLong( msg, cl->downloadSize );

	MSG_WriteShort( msg, cl->downloadBlockSize[curindex] );

	// Write the block
	if(cl->downloadBlockSize[curindex])
		MSG_WriteData(msg, cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex]);

	Com_DPrintf( "clientDownload: %d : writing block %d\n", (int) (cl - svs.clients), cl->downloadXmitBlock );

	// Move on to the next block
	// It will get sent with next snap shot.  The rate will keep us in line.
	cl->downloadXmitBlock++;
	cl->downloadSendTime = svs.time;

	return 1;
}

/*
==================
SV_SendQueuedMessages

Send one round of fragments, or queued messages to all clients that have data pending.
Return the shortest time interval for sending next packet to client
==================
*/

int SV_SendQueuedMessages(void)
{
	int i, retval = -1, nextFragT;
	client_t *cl;
	
	for(i=0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		
		if(cl->state)
		{
			nextFragT = SV_RateMsec(cl);

			if(!nextFragT)
				nextFragT = SV_Netchan_TransmitNextFragment(cl);

			if(nextFragT >= 0 && (retval == -1 || retval > nextFragT))
				retval = nextFragT;
		}
	}

	return retval;
}


/*
==================
SV_SendDownloadMessages

Send one round of download messages to all clients
==================
*/

int SV_SendDownloadMessages(void)
{
	int i, numDLs = 0, retval;
	client_t *cl;
	msg_t msg;
	byte msgBuffer[MAX_MSGLEN];
	
	for(i=0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		
		if(cl->state && *cl->downloadName)
		{
			MSG_Init(&msg, msgBuffer, sizeof(msgBuffer));
			MSG_WriteLong(&msg, cl->lastClientCommand);
			
			retval = SV_WriteDownloadToClient(cl, &msg);
				
			if(retval)
			{
				MSG_WriteByte(&msg, svc_EOF);
				SV_Netchan_Transmit(cl, &msg);
				numDLs += retval;
			}
		}
	}

	return numDLs;
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
static void SV_Disconnect_f( client_t *cl ) {
	SV_DropClient( cl, "disconnected" );
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( player_t *player ) {
	client_t *cl;
	char	*val;
	char	*ip;
	int		i;
	int	len;

	cl = player->client;

	// name for C code
	Q_strncpyz( player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name) );

	// rate command

	// if the client is on the same subnet as the server, assume they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress ) && sv_lanForceRate->integer == 1 ) {
		cl->rate = 99999;	// lans should not rate limit
	} else {
		val = Info_ValueForKey (player->userinfo, "rate");
		if (strlen(val)) {
			i = atoi(val);
			cl->rate = i;
			if (cl->rate < 1000) {
				cl->rate = 1000;
			} else if (cl->rate > 90000) {
				cl->rate = 90000;
			}
		} else {
			cl->rate = 3000;
		}
	}

	// snaps command
	val = Info_ValueForKey (player->userinfo, "snaps");
	
	if(strlen(val))
	{
		i = atoi(val);
		
		if(i < 1)
			i = 1;
		else if(i > sv_fps->integer)
			i = sv_fps->integer;

		i = 1000 / i;
	}
	else
		i = 1000 / sv_fps->integer;

	if(i != cl->snapshotMsec)
	{
		// Reset last sent snapshot so we avoid desync between server frame time and snapshot send time
		cl->lastSnapshotTime = 0;
		cl->snapshotMsec = i;		
	}

#ifdef USE_VOIP
	val = Info_ValueForKey(player->userinfo, "cl_voipProtocol");
	cl->hasVoip = !Q_stricmp( val, "opus" );
#endif

	// TTimo
	// maintain the IP information
	// the banning code relies on this being consistently present
	if( NET_IsLocalAddress(cl->netchan.remoteAddress) )
		ip = "localhost";
	else
		ip = (char*)NET_AdrToString( cl->netchan.remoteAddress );

	val = Info_ValueForKey( player->userinfo, "ip" );
	if( val[0] )
		len = strlen( ip ) - strlen( val ) + strlen( player->userinfo );
	else
		len = strlen( ip ) + 4 + strlen( player->userinfo );

	if( len >= MAX_INFO_STRING )
		SV_DropPlayer( player, "userinfo string length exceeded", qtrue );
	else
		Info_SetValueForKey( player->userinfo, "ip", ip );

}


/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *client, int localPlayerNum ) {
	player_t *player;

	player = client->localPlayers[localPlayerNum];

	if ( !player ) {
		return;
	}

	Q_strncpyz( player->userinfo, Cmd_Argv(1), sizeof(player->userinfo) );

	SV_UserinfoChanged( player );
	// call prog code to allow overrides
	VM_Call( gvm, GAME_PLAYER_USERINFO_CHANGED, player - svs.players );
}

/*
==================
SV_DropOut_f
==================
*/
void SV_DropOut_f( client_t *client, int localPlayerNum ) {
	player_t *player;

	// require disconnect command to leave server
	if ( SV_ClientNumLocalPlayers( client ) == 1 ) {
		return;
	}

	player = client->localPlayers[localPlayerNum];

	if (!player) {
		return;
	}

	SV_DropPlayer( player, "dropped out", qfalse );
}

/*
==================
SV_UpdateUserinfo1_f
==================
*/
static void SV_UpdateUserinfo1_f( client_t *client ) {
	SV_UpdateUserinfo_f( client, 0 );
}

/*
==================
SV_UpdateUserinfo2_f
==================
*/
static void SV_UpdateUserinfo2_f( client_t *client ) {
	SV_UpdateUserinfo_f( client, 1 );
}

/*
==================
SV_UpdateUserinfo3_f
==================
*/
static void SV_UpdateUserinfo3_f( client_t *client ) {
	SV_UpdateUserinfo_f( client, 2 );
}

/*
==================
SV_UpdateUserinfo4_f
==================
*/
static void SV_UpdateUserinfo4_f( client_t *client ) {
	SV_UpdateUserinfo_f( client, 3 );
}

/*
==================
SV_DropOut1_f
==================
*/
void SV_DropOut1_f( client_t *client ) {
	SV_DropOut_f( client, 0 );
}

/*
==================
SV_DropOut2_f
==================
*/
void SV_DropOut2_f( client_t *client ) {
	SV_DropOut_f( client, 1 );
}

/*
==================
SV_DropOut3_f
==================
*/
void SV_DropOut3_f( client_t *client ) {
	SV_DropOut_f( client, 2 );
}

/*
==================
SV_DropOut4_f
==================
*/
void SV_DropOut4_f( client_t *client ) {
	SV_DropOut_f( client, 3 );
}

/*
==================
SV_DropIn1_f
==================
*/
void SV_DropIn1_f( client_t *client ) {
	SV_AddPlayer( client, 0, Cmd_Argv(1) );
}

/*
==================
SV_DropIn2_f
==================
*/
void SV_DropIn2_f( client_t *client ) {
	SV_AddPlayer( client, 1, Cmd_Argv(1) );
}

/*
==================
SV_DropIn3_f
==================
*/
void SV_DropIn3_f( client_t *client ) {
	SV_AddPlayer( client, 2, Cmd_Argv(1) );
}

/*
==================
SV_DropIn4_f
==================
*/
void SV_DropIn4_f( client_t *client ) {
	SV_AddPlayer( client, 3, Cmd_Argv(1) );
}


#ifdef USE_VOIP
static
void SV_UpdateVoipIgnore(client_t *cl, const char *idstr, qboolean ignore)
{
	if ((*idstr >= '0') && (*idstr <= '9')) {
		const int id = atoi(idstr);
		if ((id >= 0) && (id < MAX_CLIENTS)) {
			cl->ignoreVoipFromPlayer[id] = ignore;
		}
	}
}

/*
==================
SV_Voip_f
==================
*/
static void SV_Voip_f( client_t *cl ) {
	const char *cmd = Cmd_Argv(1);
	if (strcmp(cmd, "ignore") == 0) {
		SV_UpdateVoipIgnore(cl, Cmd_Argv(2), qtrue);
	} else if (strcmp(cmd, "unignore") == 0) {
		SV_UpdateVoipIgnore(cl, Cmd_Argv(2), qfalse);
	} else if (strcmp(cmd, "muteall") == 0) {
		cl->muteAllVoip = qtrue;
	} else if (strcmp(cmd, "unmuteall") == 0) {
		cl->muteAllVoip = qfalse;
	}
}
#endif


typedef struct {
	char	*name;
	void	(*func)( client_t *cl );
} ucmd_t;

static ucmd_t ucmds[] = {
	{"userinfo1", SV_UpdateUserinfo1_f},
	{"userinfo2", SV_UpdateUserinfo2_f},
	{"userinfo3", SV_UpdateUserinfo3_f},
	{"userinfo4", SV_UpdateUserinfo4_f},
	{"dropout1", SV_DropOut1_f},
	{"dropout2", SV_DropOut2_f},
	{"dropout3", SV_DropOut3_f},
	{"dropout4", SV_DropOut4_f},
	{"dropin1", SV_DropIn1_f},
	{"dropin2", SV_DropIn2_f},
	{"dropin3", SV_DropIn3_f},
	{"dropin4", SV_DropIn4_f},
	{"disconnect", SV_Disconnect_f},
	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},
	{"stopdl", SV_StopDownload_f},
	{"donedl", SV_DoneDownload_f},

#ifdef USE_VOIP
	{"voip", SV_Voip_f},
#endif

	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK ) {
	ucmd_t	*u;
	qboolean bProcessed = qfalse;
	
	Cmd_TokenizeString( s );

	// see if it is a server level command
	for (u=ucmds ; u->name ; u++) {
		if (!strcmp (Cmd_Argv(0), u->name) ) {
			u->func( cl );
			bProcessed = qtrue;
			break;
		}
	}

	if (clientOK) {
		// pass unknown strings to the game
		if (!u->name && sv.state == SS_GAME && (cl->state == CS_ACTIVE || cl->state == CS_PRIMED)) {
			VM_Call( gvm, GAME_CLIENT_COMMAND, cl - svs.clients );
		}
	}
	else if (!bProcessed)
		Com_DPrintf( "client text ignored for %s: %s\n", SV_ClientName( cl ), Cmd_Argv(0) );
}

/*
===============
SV_ClientCommand
===============
*/
static qboolean SV_ClientCommand( client_t *cl, msg_t *msg ) {
	int		seq;
	const char	*s;
	qboolean clientOk = qtrue;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq ) {
		return qtrue;
	}

	Com_DPrintf( "clientCommand: %s : %i : %s\n", SV_ClientName( cl ), seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 ) {
		Com_Printf( "Client %s lost %i clientCommands\n", SV_ClientName( cl ), 
			seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since it's
	// normal to spam a lot of commands when downloading
	if ( !com_cl_running->integer && 
		cl->state >= CS_ACTIVE &&
		sv_floodProtect->integer && 
		svs.time < cl->nextReliableTime ) {
		// ignore any other text messages from this client but let them keep playing
		// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
		clientOk = qfalse;
	} 

	// don't allow another command for one second
	cl->nextReliableTime = svs.time + 1000;

	SV_ExecuteClientCommand( cl, s, clientOk );

	cl->lastClientCommand = seq;
	Com_sprintf(cl->lastClientCommandString, sizeof(cl->lastClientCommandString), "%s", s);

	return qtrue;		// continue procesing
}


//==================================================================================


/*
==================
SV_PlayerThink

Also called by bot code
==================
*/
void SV_PlayerThink (player_t *player, usercmd_t *cmd) {
	player->lastUsercmd = *cmd;

	if ( !player->inUse ) {
		return;		// may have been kicked during the last usercmd
	}

	VM_Call( gvm, GAME_PLAYER_THINK, player - svs.players );
}

/*
==================
SV_UserMove

The message usually contains all the movement commands 
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove( client_t *cl, msg_t *msg, qboolean delta ) {
	int			i, key;
	int			cmdCount;
	int			lc, localPlayerBits;
	usercmd_t	nullcmd;
	usercmd_t	cmds[MAX_PACKET_USERCMDS];
	usercmd_t	*cmd, *oldcmd;
	player_t	*player;

	if ( delta ) {
		cl->deltaMessage = cl->messageAcknowledge;
	} else {
		cl->deltaMessage = -1;
	}

	localPlayerBits = MSG_ReadByte( msg );
	cmdCount = MSG_ReadByte( msg );

	if ( localPlayerBits == 0 ) {
		return;
	}

	if ( cmdCount < 1 ) {
		Com_Printf( "cmdCount < 1\n" );
		return;
	}

	if ( cmdCount > MAX_PACKET_USERCMDS ) {
		Com_Printf( "cmdCount > MAX_PACKET_USERCMDS\n" );
		return;
	}

	// use the message acknowledge in the key
	key = cl->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= MSG_HashKey(cl->reliableCommands[ cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ], 32);

	for (lc = 0; lc < MAX_SPLITVIEW; ++lc) {
		if (!(localPlayerBits & (1<<lc))) {
			continue;
		}

		Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
		oldcmd = &nullcmd;
		for ( i = 0 ; i < cmdCount ; i++ ) {
			cmd = &cmds[i];
			MSG_ReadDeltaUsercmdKey( msg, key, oldcmd, cmd );
			oldcmd = cmd;
		}

		player = cl->localPlayers[lc];

		if (!player) {
			continue;
		}

		// save time for ping calculation
		cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

		// if this is the first usercmd we have received
		// this gamestate, put the player into the world
		if ( !player->inWorld ) {
			SV_PlayerEnterWorld( player, &cmds[0] );
			// the moves can be processed normaly
		}

		assert( cl->state == CS_ACTIVE );

		// usually, the first couple commands will be duplicates
		// of ones we have previously received, but the servertimes
		// in the commands will cause them to be immediately discarded
		for ( i =  0 ; i < cmdCount ; i++ ) {
			// if this is a cmd from before a map_restart ignore it
			if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
				continue;
			}
			// extremely lagged or cmd from before a map_restart
			//if ( cmds[i].serverTime > svs.time + 3000 ) {
			//	continue;
			//}
			// don't execute if this is an old cmd which is already executed
			// these old cmds are included when cl_packetdup > 0
			if ( cmds[i].serverTime <= player->lastUsercmd.serverTime ) {
				continue;
			}
			SV_PlayerThink( player, &cmds[ i ] );
		}
	}
}


#ifdef USE_VOIP
/*
==================
SV_ShouldIgnoreVoipSender

Blocking of voip packets based on source client
==================
*/

static qboolean SV_ShouldIgnoreVoipSender(const client_t *cl, int localPlayerNum)
{
	if (!sv_voip->integer)
		return qtrue;  // VoIP disabled on this server.
	else if (!cl->hasVoip)  // client doesn't have VoIP support?!
		return qtrue;
	else if (localPlayerNum < 0 || localPlayerNum >= MAX_SPLITVIEW || !cl->localPlayers[localPlayerNum])
		return qtrue; // local player doesn't exist

	// !!! FIXME: implement player blacklist.

	return qfalse;  // don't ignore.
}

/*
==================
SV_ClientIgnoreVoipTalker

Blocking of voip packets based on target client's opinion of source player
==================
*/

static qboolean SV_ClientIgnoreVoipTalker(const client_t *client, int playerNum)
{
	if ( client->ignoreVoipFromPlayer[ playerNum ] )
		return qtrue;

	return qfalse;  // don't ignore.
}

static
void SV_UserVoip(client_t *cl, msg_t *msg, qboolean ignoreData)
{
	int sender, localPlayerNum, generation, sequence, frames, packetsize;
	uint8_t recips[(MAX_CLIENTS + 7) / 8];
	int flags;
	byte encoded[sizeof(cl->voipPacket[0]->data)];
	client_t *client = NULL;
	int playerNum;
	voipServerPacket_t *packet = NULL;
	int i, j;

	sender = cl - svs.clients;
	localPlayerNum = MSG_ReadByte(msg);
	generation = MSG_ReadByte(msg);
	sequence = MSG_ReadLong(msg);
	frames = MSG_ReadByte(msg);
	MSG_ReadData(msg, recips, sizeof(recips));
	flags = MSG_ReadByte(msg);
	packetsize = MSG_ReadShort(msg);

	if (msg->readcount > msg->cursize)
		return;   // short/invalid packet, bail.

	if (packetsize > sizeof (encoded)) {  // overlarge packet?
		int bytesleft = packetsize;
		while (bytesleft) {
			int br = bytesleft;
			if (br > sizeof (encoded))
				br = sizeof (encoded);
			MSG_ReadData(msg, encoded, br);
			bytesleft -= br;
		}
		return;   // overlarge packet, bail.
	}

	MSG_ReadData(msg, encoded, packetsize);

	if (ignoreData || SV_ShouldIgnoreVoipSender(cl, localPlayerNum))
		return;   // Blacklisted, disabled, etc.

	// !!! FIXME: see if we read past end of msg...

	// !!! FIXME: reject if not opus data.
	// !!! FIXME: decide if this is bogus data?

	playerNum = cl->localPlayers[localPlayerNum] - svs.players;

	// decide who needs this VoIP packet sent to them...
	for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
		if (client->state != CS_ACTIVE)
			continue;  // not in the game yet, don't send to this guy.
		else if (i == sender)
			continue;  // don't send voice packet back to original author.
		else if (!client->hasVoip)
			continue;  // no VoIP support, or unsupported protocol
		else if (client->muteAllVoip)
			continue;  // client is ignoring everyone.
		else if (SV_ClientIgnoreVoipTalker(client, playerNum))
			continue;  // client is ignoring this talker.
		else if (*cl->downloadName)   // !!! FIXME: possible to DoS?
			continue;  // no VoIP allowed if downloading, to save bandwidth.

		for (j = 0; j < MAX_SPLITVIEW; j++) {
			if (!client->localPlayers[j])
				continue;
			if(Com_IsVoipTarget(recips, sizeof(recips), client->localPlayers[j] - svs.players))
				break;
		}

		if ( j != MAX_SPLITVIEW )
			flags |= VOIP_DIRECT;
		else
			flags &= ~VOIP_DIRECT;

		if (!(flags & (VOIP_SPATIAL | VOIP_DIRECT)))
			continue;  // not addressed to this player.

		// Transmit this packet to the client.
		if (client->queuedVoipPackets >= ARRAY_LEN(client->voipPacket)) {
			Com_Printf("Too many VoIP packets queued for client #%d\n", i);
			continue;  // no room for another packet right now.
		}

		// ZTM: FIXME: include which local players this is VOIP_DIRECT for
		packet = Z_Malloc(sizeof(*packet));
		packet->sender = playerNum;
		packet->frames = frames;
		packet->len = packetsize;
		packet->generation = generation;
		packet->sequence = sequence;
		packet->flags = flags;
		memcpy(packet->data, encoded, packetsize);

		client->voipPacket[(client->queuedVoipIndex + client->queuedVoipPackets) % ARRAY_LEN(client->voipPacket)] = packet;
		client->queuedVoipPackets++;
	}
}
#endif



/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg ) {
	int			c;
	int			serverId;

	MSG_Bitstream(msg);

	serverId = MSG_ReadLong( msg );
	cl->messageAcknowledge = MSG_ReadLong( msg );

	if (cl->messageAcknowledge < 0) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong( msg );

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if ((cl->reliableSequence - cl->reliableAcknowledge >= MAX_RELIABLE_COMMANDS) || (cl->reliableSequence - cl->reliableAcknowledge < 0)) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
#ifndef NDEBUG
		SV_DropClient( cl, "DEBUG: illegible client message" );
#endif
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	// 
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if ( serverId != sv.serverId && !*cl->downloadName && !strstr(cl->lastClientCommandString, "nextdl") ) {
		if ( serverId >= sv.restartedServerId && serverId < sv.serverId ) { // TTimo - use a comparison here to catch multiple map_restart
			// they just haven't caught the map_restart yet
			Com_DPrintf("%s : ignoring pre map_restart / outdated client message\n", SV_ClientName( cl ) );
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->state != CS_ACTIVE && cl->messageAcknowledge > cl->gamestateMessageNum ) {
			Com_DPrintf( "%s : dropped gamestate, resending\n", SV_ClientName( cl ) );
			SV_SendClientGameState( cl );
		}
		return;
	}

	// this client has acknowledged the new gamestate so it's
	// safe to start sending it the real time again
	if( cl->oldServerTime && serverId == sv.serverId ){
		Com_DPrintf( "%s acknowledged gamestate\n", SV_ClientName( cl ) );
		cl->oldServerTime = 0;
	}

	// read optional clientCommand strings
	do {
		c = MSG_ReadByte( msg );

		if ( c == clc_EOF ) {
			break;
		}

		if ( c == clc_clientCommand ) {
			if ( !SV_ClientCommand( cl, msg ) ) {
				return;	// we couldn't execute it because of the flood protection
			}

			if (cl->state == CS_ZOMBIE) {
				return;	// disconnect command
			}
#ifdef USE_VOIP
		} else if ( c == clc_voipSpeex ) {
			// skip legacy speex voip data
			SV_UserVoip( cl, msg, qtrue );
		} else if ( c == clc_voipOpus ) {
			SV_UserVoip( cl, msg, qfalse );
#endif
		} else {
			// unknown command
			break;
		}
	} while ( 1 );

	// read the usercmd_t
	if ( c == clc_move ) {
		SV_UserMove( cl, msg, qtrue );
	} else if ( c == clc_moveNoDelta ) {
		SV_UserMove( cl, msg, qfalse );
	} else if ( c != clc_EOF ) {
		Com_Printf( "WARNING: bad command byte for client %i\n", (int) (cl - svs.clients) );
	}
//	if ( msg->readcount != msg->cursize ) {
//		Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//	}
}

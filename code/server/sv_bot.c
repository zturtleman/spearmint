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
// sv_bot.c

#include "server.h"

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

static bot_debugpoly_t *debugpolygons;
cvar_t *bot_maxdebugpolys;

cvar_t *bot_enable;


/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(void) {
	int			i;
	client_t	*cl;
	player_t	*player;

	// find a client slot
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		if ( cl->state == CS_FREE ) {
			break;
		}
	}

	if ( i == sv_maxclients->integer ) {
		return -1;
	}

	// find a player slot
	for ( i = 0, player = svs.players; i < sv_maxclients->integer; i++, player++ ) {
		if ( !player->inUse ) {
			break;
		}
	}

	if ( i == sv_maxclients->integer ) {
		return -1;
	}

	player->inUse = qtrue;
	player->client = cl;

	cl->state = CS_ACTIVE;
	cl->localPlayers[0] = player;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	SV_SetupPlayerEntity( player );

	return ( (int)( cl - svs.clients ) & 0xFFFF ) | ( (int)( player - svs.players ) << 16 );
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient( int playerNum ) {
	client_t	*client;
	player_t	*player;

	if ( playerNum < 0 || playerNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_BotFreeClient: bad playerNum: %i", playerNum );
	}

	player = &svs.players[playerNum];

	client = player->client;
	client->localPlayers[0] = NULL;
	SV_FreeClient( client );

	player->client = NULL;
	player->name[0] = 0;
	if ( player->gentity ) {
		player->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
SV_BotDrawDebugPolygons

Set r_debugSurface to 2 to enable
==================
*/
void SV_BotDrawDebugPolygons( void (*drawPoly)(int color, int numPoints, float *points) ) {
	bot_debugpoly_t *poly;
	int i;

	if (!debugpolygons)
		return;

	//draw all debug polys
	for (i = 0; i < bot_maxdebugpolys->integer; i++) {
		poly = &debugpolygons[i];
		if (!poly->inuse) continue;
		drawPoly(poly->color, poly->numPoints, (float *) poly->points);
		//Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints);
	}
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
int BotImport_DebugPolygonCreate(int color, int numPoints, vec3_t *points) {
	bot_debugpoly_t *poly;
	int i;

	if (!debugpolygons)
		return 0;

	for (i = 1; i < bot_maxdebugpolys->integer; i++) {
		if (!debugpolygons[i].inuse)
			break;
	}
	if (i >= bot_maxdebugpolys->integer)
		return 0;
	poly = &debugpolygons[i];
	poly->inuse = qtrue;
	poly->color = color;
	if (!points || numPoints <= 0) {
		poly->numPoints = 0;
	} else {
		poly->numPoints = numPoints;
		Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
	}
	//
	return i;
}

/*
==================
BotImport_DebugPolygonShow
==================
*/
void BotImport_DebugPolygonShow(int id, int color, int numPoints, vec3_t *points) {
	bot_debugpoly_t *poly;

	if (!debugpolygons)
		return;
	if (id < 0 || id >= bot_maxdebugpolys->integer)
		return;

	poly = &debugpolygons[id];
	poly->inuse = qtrue;
	poly->color = color;
	if (!points || numPoints <= 0) {
		poly->numPoints = 0;
	} else {
		poly->numPoints = numPoints;
		Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
	}
}

/*
==================
BotImport_DebugPolygonDelete
==================
*/
void BotImport_DebugPolygonDelete(int id)
{
	if (!debugpolygons)
		return;
	if (id < 0 || id >= bot_maxdebugpolys->integer)
		return;

	debugpolygons[id].inuse = qfalse;
}

/*
==================
SV_ClientForPlayerNum

ClientNum is for players array and we want client_t
==================
*/
client_t *SV_ClientForPlayerNum( int playerNum ) {
	if ( playerNum < 0 || playerNum >= sv_maxclients->integer )
		return NULL;

	return svs.players[playerNum].client;
}

/*
==================
SV_ForceClientCommand

Sends a client command for the specified playerNum
==================
*/
void SV_ForceClientCommand( int playerNum, const char *command ) {
	client_t *client = SV_ClientForPlayerNum( playerNum );

	if ( !client )
		return;

	SV_ExecuteClientCommand( client, command, qtrue );
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame( int time ) {
	if (!bot_enable->integer) return;
	//NOTE: maybe the game is already shutdown
	if (!gvm) return;
	VM_Call( gvm, BOTAI_START_FRAME, time );
}

/*
==================
SV_BotInitCvars

Called at start up and map change
==================
*/
void SV_BotInitCvars(void) {
	bot_enable = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	bot_maxdebugpolys = Cvar_Get( "bot_maxdebugpolys", "2", CVAR_LATCH );
}

/*
==================
SV_BotInitBotLib

Called at map change
==================
*/
void SV_BotInitBotLib(void) {
	if ( !debugpolygons || bot_maxdebugpolys->modified ) {
		if (debugpolygons) Z_Free(debugpolygons);
		debugpolygons = Z_Malloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys->integer);
		bot_maxdebugpolys->modified = qfalse;
	}
	Com_Memset( debugpolygons, 0, sizeof(bot_debugpoly_t) * bot_maxdebugpolys->integer );
}


//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetServerCommand
==================
*/
int SV_BotGetServerCommand( int playerNum, char *buf, int size )
{
	client_t	*cl;
	int			index;

	cl = SV_ClientForPlayerNum( playerNum );

	if ( !cl )
		return qfalse;

	cl->lastPacketTime = svs.time;

	if ( cl->reliableAcknowledge == cl->reliableSequence ) {
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 );

	if ( !cl->reliableCommands[index][0] ) {
		return qfalse;
	}

	Q_strncpyz( buf, cl->reliableCommands[index], size );
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS( int playerNum, int entityNum ) {
	client_t			*cl;
	clientSnapshot_t	*frame;
	int					i;

	cl = SV_ClientForPlayerNum( playerNum );

	if ( !cl )
		return qfalse;

	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	for ( i = 0; i < frame->num_entities; i++ )	{
		if ( SV_SnapshotEntity(frame->first_entity + i)->number == entityNum ) {
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity( int playerNum, int sequence ) {
	client_t *cl;
	clientSnapshot_t	*frame;

	cl = SV_ClientForPlayerNum( playerNum );

	if ( !cl )
		return -1;

	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if (sequence < 0 || sequence >= frame->num_entities) {
		return -1;
	}
	return SV_SnapshotEntity(frame->first_entity + sequence)->number;
}


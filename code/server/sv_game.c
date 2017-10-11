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
// sv_game.c -- interface to the game dll

#include "server.h"

#include "../botlib/l_script.h"
#include "../botlib/l_precomp.h"

define_t	*game_globaldefines;

void		SV_GameCommand( void );
void		SV_GameCompleteArgument( char *args, int argNum );

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int	SV_NumForGentity( sharedEntity_t *ent ) {
	int		num;

	num = ( (byte *)ent - (byte *)sv.gentities ) / sv.gentitySize;

	return num;
}

sharedEntity_t *SV_GentityNum( int num ) {
	sharedEntity_t *ent;

	ent = (sharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num));

	return ent;
}

sharedEntityState_t *SV_GameEntityStateNum( int num ) {
	sharedEntity_t *ent;

	ent = SV_GentityNum( num );

	return &ent->s;
}

sharedPlayerState_t *SV_GamePlayerNum( int num ) {
	sharedPlayerState_t	*ps;

	ps = (sharedPlayerState_t *)((byte *)sv.gamePlayers + sv.gamePlayerSize*(num));

	return ps;
}

svEntity_t	*SV_SvEntityForGentity( sharedEntity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int		num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, int localPlayerNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, -1, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, localPlayerNum, "%s", text );	
	}
}


/*
===============
SV_GameDropPlayer

Disconnects the player with a message
===============
*/
void SV_GameDropPlayer( int playerNum, const char *reason ) {
	if ( playerNum < 0 || playerNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropPlayer( svs.players + playerNum, reason, qtrue );
}


/*
=================
SV_GetBrushBounds

gets mins and maxs for inline bmodels
=================
*/
void SV_GetBrushBounds( int modelindex, vec3_t mins, vec3_t maxs ) {
	clipHandle_t	h;

	if (!mins || !maxs) {
		Com_Error( ERR_DROP, "SV_GetBrushBounds: NULL" );
	}

	h = CM_InlineModel( modelindex );
	CM_ModelBounds( h, mins, maxs );
}



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS (const vec3_t p1, const vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;
	if (!CM_AreasConnected (area1, area2))
		return qfalse;		// a door blocks sight
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2)
{
	int		leafnum;
	int		cluster;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);

	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;

	return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t	*svEnt;

	svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}


/*
==================
SV_EntityContact
==================
*/
qboolean	SV_EntityContact( const vec3_t mins, const vec3_t maxs, const sharedEntity_t *gEnt, traceType_t type ) {
	const float	*origin, *angles;
	clipHandle_t	ch;
	trace_t			trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace ( &trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, type );

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
					   sharedPlayerState_t *players, int sizeofGamePlayer ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gamePlayers = players;
	sv.gamePlayerSize = sizeofGamePlayer;
}

/*
===============
SV_SetNetFields

===============
*/
void SV_SetNetFields( int entityStateSize, int entityNetworkSize, vmNetField_t *entityStateFields, int numEntityStateFields,
					   int playerStateSize, int playerNetworkSize, vmNetField_t *playerStateFields, int numPlayerStateFields ) {
	sv.gameEntityStateSize = entityStateSize;
	sv.gamePlayerStateSize = playerStateSize;

	MSG_SetNetFields( entityStateFields, numEntityStateFields, entityStateSize, entityNetworkSize,
					  playerStateFields, numPlayerStateFields, playerStateSize, playerNetworkSize );
}

/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int playerNum, usercmd_t *cmd ) {
	if ( playerNum < 0 || playerNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad playerNum:%i", playerNum );
	}
	*cmd = svs.players[playerNum].lastUsercmd;
}

//==============================================

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
intptr_t SV_GameSystemCalls( intptr_t *args ) {
	switch( args[0] ) {
	case G_PRINT:
		Com_Printf( "%s", (const char*)VMA(1) );
		return 0;
	case G_ERROR:
		Com_Error( ERR_DROP, "%s", (const char*)VMA(1) );
		return 0;
	case G_MILLISECONDS:
		return Sys_Milliseconds();
	case G_REAL_TIME:
		return Com_RealTime( VMA(1) );
	case G_SNAPVECTOR:
		Q_SnapVector(VMA(1));
		return 0;

	case G_ADDCOMMAND:
		Cmd_AddCommandSafe( VMA(1), SV_GameCommand, SV_GameCompleteArgument );
		return 0;
	case G_REMOVECOMMAND:
		Cmd_RemoveCommandSafe( VMA(1), SV_GameCommand );
		return 0;
	case G_CMD_EXECUTETEXT:
		Cbuf_ExecuteTextSafe( args[1], VMA(2) );
		return 0;

	case G_CVAR_REGISTER:
		Cvar_Register( VMA(1), VMA(2), VMA(3), args[4] ); 
		return 0;
	case G_CVAR_UPDATE:
		Cvar_Update( VMA(1) );
		return 0;
	case G_CVAR_SET:
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), "Game" );
		return 0;
	case G_CVAR_SET_VALUE:
		Cvar_VM_SetValue( VMA(1), VMF(2), "Game" );
		return 0;
	case G_CVAR_RESET:
		Cvar_Reset( VMA(1) );
		return 0;
	case G_CVAR_VARIABLE_VALUE:
		return FloatAsInt( Cvar_VariableValue( VMA(1) ) );
	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue( (const char *)VMA(1) );
	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer( VMA(1), VMA(2), args[3] );
		return 0;
	case G_CVAR_LATCHED_VARIABLE_STRING_BUFFER:
		Cvar_LatchedVariableStringBuffer( VMA( 1 ), VMA( 2 ), args[3] );
		return 0;
	case G_CVAR_INFO_STRING_BUFFER:
		Cvar_InfoStringBuffer( args[1], VMA(2), args[3] );
		return 0;
	case G_CVAR_CHECK_RANGE:
		Cvar_CheckRangeSafe( VMA(1), VMF(2), VMF(3), args[4] );
		return 0;

	case G_ARGC:
		return Cmd_Argc();
	case G_ARGV:
		Cmd_ArgvBuffer( args[1], VMA(2), args[3] );
		return 0;
	case G_ARGS:
		Cmd_ArgsBuffer( VMA(1), args[2] );
		return 0;
	case G_LITERAL_ARGS:
		Cmd_LiteralArgsBuffer( VMA(1), args[2] );
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode( VMA(1), VMA(2), args[3] );
	case G_FS_READ:
		return FS_Read( VMA(1), args[2], args[3] );
	case G_FS_WRITE:
		return FS_Write( VMA(1), args[2], args[3] );
	case G_FS_SEEK:
		return FS_Seek( args[1], args[2], args[3] );
	case G_FS_TELL:
		return FS_FTell( args[1] );
	case G_FS_FCLOSE_FILE:
		FS_FCloseFile( args[1] );
		return 0;
	case G_FS_GETFILELIST:
		return FS_GetFileListBuffer( VMA(1), VMA(2), VMA(3), args[4] );
	case G_FS_DELETE:
		return FS_Delete( VMA(1) );
	case G_FS_RENAME:
		return FS_Rename( VMA(1), VMA(2) );

	case G_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine( &game_globaldefines, VMA(1) );
	case G_PC_REMOVE_GLOBAL_DEFINE:
		PC_RemoveGlobalDefine( &game_globaldefines, VMA(1) );
		return 0;
	case G_PC_REMOVE_ALL_GLOBAL_DEFINES:
		PC_RemoveAllGlobalDefines( &game_globaldefines );
		return 0;
	case G_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle( VMA(1), VMA(2), game_globaldefines );
	case G_PC_FREE_SOURCE:
		return PC_FreeSourceHandle( args[1] );
	case G_PC_ADD_DEFINE:
		return PC_AddDefineHandle( args[1], VMA(2) );
	case G_PC_READ_TOKEN:
		return PC_ReadTokenHandle( args[1], VMA(2) );
	case G_PC_UNREAD_TOKEN:
		PC_UnreadLastTokenHandle( args[1] );
		return 0;
	case G_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case G_HEAP_MALLOC:
		return VM_HeapMalloc( args[1] );
	case G_HEAP_AVAILABLE:
		return VM_HeapAvailable();
	case G_HEAP_FREE:
		VM_HeapFree( VMA(1) );
		return 0;

	case G_FIELD_COMPLETEFILENAME:
		Field_CompleteFilename( VMA(1), VMA(2), args[3], args[4] );
		return 0;
	case G_FIELD_COMPLETECOMMAND:
		Field_CompleteCommand( VMA(1), args[2], args[3] );
		return 0;
	case G_FIELD_COMPLETELIST:
		Field_CompleteList( VMA(1) );
		return 0;

		//====================================

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData( VMA(1), args[2], args[3], VMA(4), args[5] );
		return 0;
	case G_SET_NET_FIELDS:
		SV_SetNetFields( args[1], args[2], VMA(3), args[4], args[5], args[6], VMA(7), args[8] );
		return 0;
	case G_DROP_PLAYER:
		SV_GameDropPlayer( args[1], VMA(2) );
		return 0;
	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand( args[1], args[2], VMA(3) );
		return 0;
	case G_LINKENTITY:
		SV_LinkEntity( VMA(1) );
		return 0;
	case G_UNLINKENTITY:
		SV_UnlinkEntity( VMA(1) );
		return 0;
	case G_ENTITIES_IN_BOX:
		return SV_AreaEntities( VMA(1), VMA(2), VMA(3), args[4] );
	case G_ENTITY_CONTACT:
		return SV_EntityContact( VMA(1), VMA(2), VMA(3), TT_AABB );
	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact( VMA(1), VMA(2), VMA(3), TT_CAPSULE );
	case G_TRACE:
		SV_Trace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_AABB );
		return 0;
	case G_TRACECAPSULE:
		SV_Trace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_CAPSULE );
		return 0;
	case G_CLIPTOENTITIES:
		SV_ClipToEntities( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_AABB );
		return 0;
	case G_CLIPTOENTITIESCAPSULE:
		SV_ClipToEntities( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_CAPSULE );
		return 0;
	case G_POINT_CONTENTS:
		return SV_PointContents( VMA(1), args[2] );
	case G_GET_BRUSH_BOUNDS:
		SV_GetBrushBounds( args[1], VMA(2), VMA(3) );
		return 0;
	case G_IN_PVS:
		return SV_inPVS( VMA(1), VMA(2) );
	case G_IN_PVS_IGNORE_PORTALS:
		return SV_inPVSIgnorePortals( VMA(1), VMA(2) );

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring( args[1], VMA(2) );
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring( args[1], VMA(2), args[3] );
		return 0;
	case G_SET_CONFIGSTRING_RESTRICTIONS:
		SV_SetConfigstringRestrictions( args[1], VMA(2) );
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo( args[1], VMA(2) );
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo( args[1], VMA(2), args[3] );
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo( VMA(1), args[2] );
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState( VMA(1), args[2] );
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected( args[1], args[2] );

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient( args[1] );
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd( args[1], VMA(2) );
		return 0;
	case G_GET_ENTITY_TOKEN:
		return CM_GetEntityToken( VMA(1), VMA(2), args[3] );

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate( args[1], args[2], VMA(3) );
	case G_DEBUG_POLYGON_SHOW:
		BotImport_DebugPolygonShow( args[1], args[2], args[3], VMA(4) );
		return 0;
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete( args[1] );
		return 0;

	case G_CLIENT_COMMAND:
		SV_ForceClientCommand( args[1], VMA(2) );
		return 0;

	case G_R_REGISTERMODEL:
		return re.RegisterModel( VMA(1) );
	case G_R_LERPTAG:
		return re.LerpTag( VMA(1), args[2], 0, args[3], 0, args[4], VMF(5), VMA(6), NULL, NULL, 0, 0, 0, 0, 0 );
	case G_R_LERPTAG_FRAMEMODEL:
		return re.LerpTag( VMA(1), args[2], args[3], args[4], args[5], args[6], VMF(7), VMA(8), VMA(9), NULL, 0, 0, 0, 0, 0 );
	case G_R_LERPTAG_TORSO:
		return re.LerpTag( VMA(1), args[2], args[3], args[4], args[5], args[6], VMF(7), VMA(8), VMA(9), VMA(10), args[11], args[12], args[13], args[14], VMF(15) );
	case G_R_MODELBOUNDS:
		return re.ModelBounds( args[1], VMA(2), VMA(3), args[4], args[5], VMF(6) );

	case G_BOT_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity( args[1], args[2] );
	case G_BOT_GET_SERVER_COMMAND:
		return SV_BotGetServerCommand( args[1], VMA(2), args[3] );
	case G_BOT_USER_COMMAND:
		{
			int playerNum = args[1];

			if ( playerNum >= 0 && playerNum < sv_maxclients->integer ) {
				SV_PlayerThink( &svs.players[playerNum], VMA(2) );
			}
		}
		return 0;

	default:
		Com_Error( ERR_DROP, "Bad game system trap: %ld", (long int) args[0] );
	}
	return 0;
}

/*
===============
SV_GameInternalShutdown

Call SV_ShutdownGameProgs or SV_RestartGameProgs instead of this directly.
===============
*/
void SV_GameInternalShutdown( qboolean restart ) {
	VM_Call( gvm, GAME_SHUTDOWN, restart );

	Cmd_RemoveCommandsByFunc( SV_GameCommand );
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void ) {
	if ( !gvm ) {
		return;
	}
	SV_GameInternalShutdown( qfalse );
	VM_Free( gvm );
	gvm = NULL;

	//remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines( &game_globaldefines );

	// print any files still open
	PC_CheckOpenSourceHandles();
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM( qboolean restart ) {
	char			apiName[64];
	int				major, minor;
	int				i;

	VM_GetVersion( gvm, GAME_GETAPINAME, GAME_GETAPIVERSION, apiName, sizeof(apiName), &major, &minor );
	Com_DPrintf("Loading Game VM with API %s %d.%d\n", apiName, major, minor);

	// sanity check
	if ( !strcmp( apiName, GAME_API_NAME ) && major == GAME_API_MAJOR_VERSION
		&& ( ( major > 0 && minor <= GAME_API_MINOR_VERSION )
		  || ( major == 0 && minor == GAME_API_MINOR_VERSION ) ) ) {
		// Supported API
	} else {
		// Free gvm now, so GAME_SHUTDOWN doesn't get called later.
		VM_Free( gvm );
		gvm = NULL;

		Com_Error(ERR_DROP, "Game VM uses unsupported API %s %d.%d, %s %d.%d",
				  apiName, major, minor, GAME_API_NAME, GAME_API_MAJOR_VERSION, GAME_API_MINOR_VERSION );
	}

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		svs.players[i].gentity = NULL;
	}
	
	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call (gvm, GAME_INIT, sv.time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs( void ) {
	if ( !gvm ) {
		return;
	}
	SV_GameInternalShutdown( qtrue );

	// do a restart instead of a free
	gvm = VM_Restart(gvm, qtrue);
	if ( !gvm ) {
		Com_Error( ERR_FATAL, "VM_Restart on game failed" );
	}

	SV_InitGameVM( qtrue );
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs( void ) {
	// load the dll or bytecode
	gvm = VM_Create( VM_PREFIX "game", SV_GameSystemCalls, Cvar_VariableValue( "vm_game" ),
			TAG_GAME, Cvar_VariableValue( "vm_gameHeapMegs" ) * 1024 * 1024 );

	if ( !gvm ) {
		Com_Error( ERR_FATAL, "VM_Create on game failed" );
	}

	SV_InitGameVM( qfalse );
}


/*
====================
SV_GameCommand

Pass current console command to game VM
====================
*/
void SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return;
	}

	VM_Call( gvm, GAME_CONSOLE_COMMAND );
}

/*
====================
CL_GameCompleteArgument
====================
*/
void SV_GameCompleteArgument( char *args, int argNum ) {
	if ( sv.state != SS_GAME ) {
		return;
	}

	VM_Call( gvm, GAME_CONSOLE_COMPLETEARGUMENT, argNum );
}

/*
===============
SV_GameVidRestart

Called every time client restarts renderer while running server
===============
*/
void SV_GameVidRestart( void ) {
	if ( !gvm ) {
		return;
	}
	VM_Call( gvm, GAME_VID_RESTART );
}


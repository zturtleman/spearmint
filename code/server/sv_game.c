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

#include "../botlib/botlib.h"

botlib_export_t	*botlib_export;

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

sharedPlayerState_t *SV_GameClientNum( int num ) {
	sharedPlayerState_t	*ps;

	ps = (sharedPlayerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num));

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
	SV_DropPlayer( svs.players + playerNum, reason );	
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t	h;
	vec3_t			mins, maxs;

	if (!name) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if (name[0] != '*') {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}


	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy (mins, ent->s.mins);
	VectorCopy (maxs, ent->s.maxs);
	ent->s.bmodel = qtrue;

	ent->s.contents = -1;		// we don't know exactly what is in the brushes

	SV_LinkEntity( ent );		// FIXME: remove
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
					   sharedPlayerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}

/*
===============
SV_SetNetFields

===============
*/
void SV_SetNetFields( int entityStateSize, vmNetField_t *entityStateFields, int numEntityStateFields,
					   int playerStateSize, vmNetField_t *playerStateFields, int numPlayerStateFields ) {
	sv.gameEntityStateSize = entityStateSize;
	sv.gamePlayerStateSize = playerStateSize;

	MSG_SetNetFields( entityStateFields, numEntityStateFields, entityStateSize,
					  playerStateFields, numPlayerStateFields, playerStateSize );
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
		Cmd_AddCommandSafe( VMA(1), SV_GameCommand );
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
		Cvar_VM_Set( (const char *)VMA(1), (const char *)VMA(2), qtrue );
		return 0;
	case G_CVAR_SET_VALUE:
		Cvar_VM_SetValue( VMA(1), VMF(2), qtrue );
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
		return FS_Read2( VMA(1), args[2], args[3] );
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
		return FS_GetFileList( VMA(1), VMA(2), VMA(3), args[4] );
	case G_FS_DELETE:
		return FS_Delete( VMA(1) );
	case G_FS_RENAME:
		return FS_Rename( VMA(1), VMA(2) );

	case G_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( VMA(1) );
	case G_PC_REMOVE_ALL_GLOBAL_DEFINES:
		botlib_export->PC_RemoveAllGlobalDefines();
		return 0;
	case G_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( VMA(1), VMA(2) );
	case G_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case G_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], VMA(2) );
	case G_PC_UNREAD_TOKEN:
		botlib_export->PC_UnreadLastTokenHandle( args[1] );
		return 0;
	case G_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case G_ALLOC:
		return VM_Alloc( args[1], VMA(2) );

		//====================================

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData( VMA(1), args[2], args[3], VMA(4), args[5] );
		return 0;
	case G_SET_NET_FIELDS:
		SV_SetNetFields( args[1], VMA(2), args[3], args[4], VMA(5), args[6] );
		return 0;
	case G_DROP_CLIENT:
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
	case G_POINT_CONTENTS:
		return SV_PointContents( VMA(1), args[2] );
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel( VMA(1), VMA(2) );
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
		{
			const char	*s;

			s = COM_Parse( &sv.entityParsePoint );
			Q_strncpyz( VMA(1), s, args[2] );
			if ( !sv.entityParsePoint && !s[0] ) {
				return qfalse;
			} else {
				return qtrue;
			}
		}

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate( args[1], args[2], VMA(3) );
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete( args[1] );
		return 0;

	case G_CLIENT_COMMAND:
		SV_ForcePlayerCommand( args[1], VMA(2) );
		return 0;

	case G_R_REGISTERMODEL:
		return re.RegisterModel( VMA(1) );
	case G_R_LERPTAG:
		return re.LerpTag( VMA(1), args[2], args[3], args[4], VMF(5), VMA(6) );
	case G_R_MODELBOUNDS:
		return re.ModelBounds( args[1], VMA(2), VMA(3), args[4], args[5], VMF(6) );

		//====================================

	case BOTLIB_SETUP:
		return SV_BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return botlib_export->BotLibVarSet( VMA(1), VMA(2) );
	case BOTLIB_LIBVAR_GET:
		return botlib_export->BotLibVarGet( VMA(1), VMA(2), args[3] );

	case BOTLIB_START_FRAME:
		return botlib_export->BotLibStartFrame( VMF(1) );
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap( VMA(1) );
	case BOTLIB_UPDATENTITY:
		return botlib_export->BotLibUpdateEntity( args[1], VMA(2) );
	case BOTLIB_TEST:
		return botlib_export->Test( args[1], VMA(2), VMA(3), VMA(4) );

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity( args[1], args[2] );
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage( args[1], VMA(2), args[3] );
	case BOTLIB_USER_COMMAND:
		SV_PlayerThink( &svs.players[args[1]], VMA(2) );
		return 0;

	case BOTLIB_AAS_BBOX_AREAS:
		return botlib_export->aas.AAS_BBoxAreas( VMA(1), VMA(2), VMA(3), args[4] );
	case BOTLIB_AAS_AREA_INFO:
		return botlib_export->aas.AAS_AreaInfo( args[1], VMA(2) );
	case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:
		return botlib_export->aas.AAS_AlternativeRouteGoals( VMA(1), args[2], VMA(3), args[4], args[5], VMA(6), args[7], args[8] );

	case BOTLIB_AAS_LOADED:
		return botlib_export->aas.AAS_Loaded();
	case BOTLIB_AAS_INITIALIZED:
		return botlib_export->aas.AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		botlib_export->aas.AAS_PresenceTypeBoundingBox( args[1], VMA(2), VMA(3) );
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt( botlib_export->aas.AAS_Time() );

	case BOTLIB_AAS_POINT_AREA_NUM:
		return botlib_export->aas.AAS_PointAreaNum( VMA(1) );
	case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:
		return botlib_export->aas.AAS_PointReachabilityAreaIndex( VMA(1) );
	case BOTLIB_AAS_TRACE_CLIENT_BBOX:
		botlib_export->aas.AAS_TraceClientBBox( VMA(1), VMA(2), VMA(3), args[4], args[5] );
		return 0;
	case BOTLIB_AAS_TRACE_AREAS:
		return botlib_export->aas.AAS_TraceAreas( VMA(1), VMA(2), VMA(3), VMA(4), args[5] );

	case BOTLIB_AAS_POINT_CONTENTS:
		return botlib_export->aas.AAS_PointContents( VMA(1) );
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return botlib_export->aas.AAS_NextBSPEntity( args[1] );
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_ValueForBSPEpairKey( args[1], VMA(2), VMA(3), args[4] );
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_VectorForBSPEpairKey( args[1], VMA(2), VMA(3) );
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_FloatForBSPEpairKey( args[1], VMA(2), VMA(3) );
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return botlib_export->aas.AAS_IntForBSPEpairKey( args[1], VMA(2), VMA(3) );

	case BOTLIB_AAS_AREA_REACHABILITY:
		return botlib_export->aas.AAS_AreaReachability( args[1] );
	case BOTLIB_AAS_BEST_REACHABLE_AREA:
		return botlib_export->aas.AAS_BestReachableArea( VMA(1), VMA(2), VMA(3), VMA(4) );
	case BOTLIB_AAS_BEST_REACHABLE_FROM_JUMP_PAD_AREA:
		return botlib_export->aas.AAS_BestReachableFromJumpPadArea( VMA(1), VMA(2), VMA(3) );
	case BOTLIB_AAS_NEXT_MODEL_REACHABILITY:
		return botlib_export->aas.AAS_NextModelReachability( args[1], args[2] );
	case BOTLIB_AAS_AREA_GROUND_FACE_AREA:
		return FloatAsInt( botlib_export->aas.AAS_AreaGroundFaceArea( args[1] ) );
	case BOTLIB_AAS_AREA_CROUCH:
		return botlib_export->aas.AAS_AreaCrouch( args[1] );
	case BOTLIB_AAS_AREA_SWIM:
		return botlib_export->aas.AAS_AreaSwim( args[1] );
	case BOTLIB_AAS_AREA_LIQUID:
		return botlib_export->aas.AAS_AreaLiquid( args[1] );
	case BOTLIB_AAS_AREA_LAVA:
		return botlib_export->aas.AAS_AreaLava( args[1] );
	case BOTLIB_AAS_AREA_SLIME:
		return botlib_export->aas.AAS_AreaSlime( args[1] );
	case BOTLIB_AAS_AREA_GROUNDED:
		return botlib_export->aas.AAS_AreaGrounded( args[1] );
	case BOTLIB_AAS_AREA_LADDER:
		return botlib_export->aas.AAS_AreaLadder( args[1] );
	case BOTLIB_AAS_AREA_JUMP_PAD:
		return botlib_export->aas.AAS_AreaJumpPad( args[1] );
	case BOTLIB_AAS_AREA_DO_NOT_ENTER:
		return botlib_export->aas.AAS_AreaDoNotEnter( args[1] );

	case BOTLIB_AAS_TRAVEL_FLAG_FOR_TYPE:
		return botlib_export->aas.AAS_TravelFlagForType( args[1] );
	case BOTLIB_AAS_AREA_CONTENTS_TRAVEL_FLAGS:
		return botlib_export->aas.AAS_AreaContentsTravelFlags( args[1] );
	case BOTLIB_AAS_NEXT_AREA_REACHABILITY:
		return botlib_export->aas.AAS_NextAreaReachability( args[1], args[2] );
	case BOTLIB_AAS_REACHABILITY_FROM_NUM:
		botlib_export->aas.AAS_ReachabilityFromNum( args[1], VMA(2) );
		return 0;
	case BOTLIB_AAS_RANDOM_GOAL_AREA:
		return botlib_export->aas.AAS_RandomGoalArea( args[1], args[2], VMA(3), VMA(4) );
	case BOTLIB_AAS_ENABLE_ROUTING_AREA:
		return botlib_export->aas.AAS_EnableRoutingArea( args[1], args[2] );
	case BOTLIB_AAS_AREA_TRAVEL_TIME:
		return botlib_export->aas.AAS_AreaTravelTime( args[1], VMA(2), VMA(3) );
	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return botlib_export->aas.AAS_AreaTravelTimeToGoalArea( args[1], VMA(2), args[3], args[4] );
	case BOTLIB_AAS_PREDICT_ROUTE:
		return botlib_export->aas.AAS_PredictRoute( VMA(1), args[2], VMA(3), args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11] );

	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return botlib_export->aas.AAS_PredictClientMovement( VMA(1), args[2], VMA(3), args[4], args[5],
			VMA(6), VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13] );
	case BOTLIB_AAS_ON_GROUND:
		return botlib_export->aas.AAS_OnGround( VMA(1), args[2], args[3] );
	case BOTLIB_AAS_SWIMMING:
		return botlib_export->aas.AAS_Swimming( VMA(1) );
	case BOTLIB_AAS_JUMP_REACH_RUN_START:
		botlib_export->aas.AAS_JumpReachRunStart( VMA(1), VMA(2) );
		return 0;
	case BOTLIB_AAS_AGAINST_LADDER:
		return botlib_export->aas.AAS_AgainstLadder( VMA(1) );
	case BOTLIB_AAS_HORIZONTAL_VELOCITY_FOR_JUMP:
		return botlib_export->aas.AAS_HorizontalVelocityForJump( VMF(1), VMA(2), VMA(3), VMA(4) );
	case BOTLIB_AAS_DROP_TO_FLOOR:
		return botlib_export->aas.AAS_DropToFloor( VMA(1), VMA(2), VMA(3) );

	case BOTLIB_AI_LOAD_CHARACTER:
		return botlib_export->ai.BotLoadCharacter( VMA(1), VMF(2) );
	case BOTLIB_AI_FREE_CHARACTER:
		botlib_export->ai.BotFreeCharacter( args[1] );
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_Float( args[1], args[2] ) );
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt( botlib_export->ai.Characteristic_BFloat( args[1], args[2], VMF(3), VMF(4) ) );
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return botlib_export->ai.Characteristic_Integer( args[1], args[2] );
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return botlib_export->ai.Characteristic_BInteger( args[1], args[2], args[3], args[4] );
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		botlib_export->ai.Characteristic_String( args[1], args[2], VMA(3), args[4] );
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return botlib_export->ai.BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		botlib_export->ai.BotFreeChatState( args[1] );
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		botlib_export->ai.BotQueueConsoleMessage( args[1], args[2], VMA(3) );
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		botlib_export->ai.BotRemoveConsoleMessage( args[1], args[2] );
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNextConsoleMessage( args[1], VMA(2) );
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return botlib_export->ai.BotNumConsoleMessages( args[1] );
	case BOTLIB_AI_INITIAL_CHAT:
		botlib_export->ai.BotInitialChat( args[1], VMA(2), args[3], VMA(4), VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11) );
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return botlib_export->ai.BotNumInitialChats( args[1], VMA(2) );
	case BOTLIB_AI_REPLY_CHAT:
		return botlib_export->ai.BotReplyChat( args[1], VMA(2), args[3], args[4], VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11), VMA(12) );
	case BOTLIB_AI_CHAT_LENGTH:
		return botlib_export->ai.BotChatLength( args[1] );
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat( args[1], args[2], args[3] );
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		botlib_export->ai.BotGetChatMessage( args[1], VMA(2), args[3] );
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return botlib_export->ai.StringContains( VMA(1), VMA(2), args[3] );
	case BOTLIB_AI_FIND_MATCH:
		return botlib_export->ai.BotFindMatch( VMA(1), VMA(2), args[3] );
	case BOTLIB_AI_MATCH_VARIABLE:
		botlib_export->ai.BotMatchVariable( VMA(1), args[2], VMA(3), args[4] );
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		botlib_export->ai.UnifyWhiteSpaces( VMA(1) );
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		botlib_export->ai.BotReplaceSynonyms( VMA(1), args[2] );
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return botlib_export->ai.BotLoadChatFile( args[1], VMA(2), VMA(3) );
	case BOTLIB_AI_SET_CHAT_GENDER:
		botlib_export->ai.BotSetChatGender( args[1], args[2] );
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		botlib_export->ai.BotSetChatName( args[1], VMA(2), args[3] );
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return botlib_export->ai.GeneticParentsAndChildSelection(args[1], VMA(2), VMA(3), VMA(4), VMA(5));


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

	Z_FreeTags( TAG_GAME );

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
	cvar_t	*var;
	//FIXME these are temp while I make bots run in vm
	extern int	bot_enable;

	var = Cvar_Get( "bot_enable", "1", CVAR_LATCH );
	if ( var ) {
		bot_enable = var->integer;
	}
	else {
		bot_enable = 0;
	}

	// load the dll or bytecode
	gvm = VM_Create( "mint-game", SV_GameSystemCalls, Cvar_VariableValue( "vm_game" ) );
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


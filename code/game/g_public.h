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
//

// g_public.h -- game module information visible to server

#define GAME_API_NAME			"SPEARMINT_GAME"

// major 0 means each minor is an API break.
// major > 0 means each major is an API break and each minor extends API.
// ZTM: FIXME: There is no way for the VM to know what the engine support API is
//             so there is no way to add more system calls.
#define	GAME_API_MAJOR_VERSION	1
#define	GAME_API_MINOR_VERSION	0


// entity->svFlags
// the server does not know how to interpret most of the values
// in entityStates (level eType), so the game must explicitly flag
// special server behaviors
#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects

#define SVF_PLAYERMASK			0x00000002 // send to limited list of players

#define SVF_BOT					0x00000008	// set if the entity is a bot
#define	SVF_BROADCAST			0x00000020	// send to all connected players (r.cullDistance will still be checked)
#define	SVF_PORTAL				0x00000040	// merge a second pvs at origin2 into snapshots
#define	SVF_USE_CURRENT_ORIGIN	0x00000080	// entity->r.currentOrigin instead of entity->s.origin
											// for link position (missiles and movers)

#define SVF_VISDUMMY            0x00000400  // this ent is a "visibility dummy" and needs it's master to be sent to players that can see it even if they can't see the master ent
#define SVF_VISDUMMY_MULTIPLE   0x00000800  // so that one vis dummy can add to snapshot multiple speakers



//===============================================================


typedef struct {
	qboolean	linked;				// qfalse if not in any good cluster
	int			linkcount;

	int			svFlags;			// SVF_NOCLIENT, SVF_BROADCAST, etc

	// if SVF_PLAYERMASK is set, use bitmask for players to send to
	clientList_t	sendPlayers;

	vec3_t		absmin, absmax;		// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t		currentOrigin;
	vec3_t		currentAngles;

	// when a trace call is made and passEntityNum != ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->r.ownerNum == passEntityNum	(don't interact with your own missiles)
	// entity[ent->r.ownerNum].r.ownerNum == passEntityNum	(don't interact with other missiles from owner)
	int			ownerNum;

	// if set, entity is only sent to client if distance between entity and client <= cullDistance (even if SVF_BROADCAST is set)
	int			cullDistance;

	// if set, portal entities are only sent to client if distance between portal and player <= portalCullDistance
	int			portalCullDistance;

	// if SVF_VISDUMMY, number of master, else if not 0, it's the number of a target_vis_dummy_multiple.
	int			visDummyNum;
} entityShared_t;



// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
typedef struct {
	entityShared_t	r;				// shared by both the server system and game
	sharedEntityState_t	s;				// communicated by server to clients
} sharedEntity_t;



//===============================================================

//
// system traps provided by the main engine
//
// also see qvmTraps_t in qcommon.h for QVM-specific system calls
//
typedef enum {
	//============== general Quake services ==================

	G_PRINT = 0,	// ( const char *string );
	// print message on the local console

	G_ERROR,		// ( const char *string );
	// abort the game

	G_MILLISECONDS,	// ( void );
	// get current time for profiling reasons
	// this should NOT be used for any game related tasks,
	// because it is not journaled

	G_REAL_TIME,	// ( qtime_t *qtime );

	G_SNAPVECTOR,	// ( float *v );

	// ClientCommand and ServerCommand parameter access
	G_ARGC,			// ( void );
	G_ARGV,			// ( int n, char *buffer, int bufferLength );
	G_ARGS,			// ( char *buffer, int bufferLength );
	G_LITERAL_ARGS,	// ( char *buffer, int bufferLength );

	G_ADDCOMMAND,	// ( const char *cmdName );
	G_REMOVECOMMAND,// ( const char *cmdName );

	G_CMD_EXECUTETEXT,	// ( int exec_when, const char *text );
	// add commands to the console as if they were typed in
	// for map changing, etc

	// console variable interaction
	G_CVAR_REGISTER,	// ( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
	G_CVAR_UPDATE,		// ( vmCvar_t *vmCvar );
	G_CVAR_SET,			// ( const char *var_name, const char *value );
	G_CVAR_SET_VALUE,	// ( const char *var_name, float value );
	G_CVAR_RESET,		// ( const char *var_name );
	G_CVAR_VARIABLE_VALUE,					// ( const char *var_name );
	G_CVAR_VARIABLE_INTEGER_VALUE,			// ( const char *var_name );
	G_CVAR_VARIABLE_STRING_BUFFER,			// ( const char *var_name, char *buffer, int bufsize );
	G_CVAR_LATCHED_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );
	G_CVAR_DEFAULT_VARIABLE_STRING_BUFFER,	// ( const char *var_name, char *buffer, int bufsize );
	G_CVAR_INFO_STRING_BUFFER,				// ( int bit, char *buffer, int bufsize );
	G_CVAR_CHECK_RANGE,						// (  const char *var_name, float min, float max, qboolean integral );

	G_FS_FOPEN_FILE,	// ( const char *qpath, fileHandle_t *file, fsMode_t mode );
	G_FS_READ,			// ( void *buffer, int len, fileHandle_t f );
	G_FS_WRITE,			// ( const void *buffer, int len, fileHandle_t f );
	G_FS_SEEK,
	G_FS_TELL,			// ( fileHandle_t f );
	G_FS_FCLOSE_FILE,	// ( fileHandle_t f );
	G_FS_GETFILELIST,
	G_FS_DELETE,		// ( const void *path );
	G_FS_RENAME,		// ( const void *from, const void *to );

	G_PC_ADD_GLOBAL_DEFINE,				// ( const char *define );
	G_PC_REMOVE_GLOBAL_DEFINE,			// ( const char *define );
	G_PC_REMOVE_ALL_GLOBAL_DEFINES,		// ( void );
	G_PC_LOAD_SOURCE,					// ( const char *filename, const char *basepath );
	G_PC_FREE_SOURCE,					// ( int handle );
	G_PC_ADD_DEFINE,					// ( int handle, const char *define );
	G_PC_READ_TOKEN,					// ( int handle, pc_token_t *pc_token );
	G_PC_UNREAD_TOKEN,					// ( int handle );
	G_PC_SOURCE_FILE_AND_LINE,			// ( int handle, char *filename, int *line );

	G_HEAP_MALLOC,		// ( int size );
	G_HEAP_AVAILABLE,	// ( void );
	G_HEAP_FREE,		// ( void *data );

	G_FIELD_COMPLETEFILENAME, // ( const char *dir, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk );
	G_FIELD_COMPLETECOMMAND, // ( const char *cmd, qboolean doCommands, qboolean doCvars );
	G_FIELD_COMPLETELIST, // ( const char *list );

	//=========== server specific functionality =============

	G_LOCATE_GAME_DATA = 100,		// ( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
	//							playerState_t *players, int sizeofGamePlayer );
	// the game needs to let the server system know where and how big the gentities
	// are, so it can look at them directly without going through an interface

	G_DROP_PLAYER,		// ( int playerNum, const char *reason );
	// kick a player off the server with a message

	G_SEND_SERVER_COMMAND,	// ( int clientNum, int localPlayerNum, const char *text );
	// reliably sends a command string to be interpreted by the given
	// client.  If clientNum is -1, it will be sent to all clients

	G_SET_CONFIGSTRING,	// ( int num, const char *string );
	// config strings hold all the index strings, and various other information
	// that is reliably communicated to all clients
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	// All confgstrings are cleared at each level start.

	G_GET_CONFIGSTRING,	// ( int num, char *buffer, int bufferSize );

	G_SET_CONFIGSTRING_RESTRICTIONS, // ( int num, const clientList_t *clientList );

	G_GET_USERINFO,		// ( int num, char *buffer, int bufferSize );
	// userinfo strings are maintained by the server system, so they
	// are persistant across level loads, while all other game visible
	// data is completely reset

	G_SET_USERINFO,		// ( int num, const char *buffer );

	G_GET_SERVERINFO,	// ( char *buffer, int bufferSize );
	// the serverinfo info string has all the cvars visible to server browsers

	G_GET_BRUSH_BOUNDS,	// ( int modelindex, vec3_t mins, vec3_t maxs );
	// gets mins and maxs for inline brush model

	G_TRACE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	// collision detection against all linked entities

	G_POINT_CONTENTS,	// ( const vec3_t point, int passEntityNum );
	// point contents against all linked entities

	G_IN_PVS,			// ( const vec3_t p1, const vec3_t p2 );

	G_IN_PVS_IGNORE_PORTALS,	// ( const vec3_t p1, const vec3_t p2 );

	G_ADJUST_AREA_PORTAL_STATE,	// ( gentity_t *ent, qboolean open );

	G_AREAS_CONNECTED,	// ( int area1, int area2 );

	G_LINKENTITY,		// ( gentity_t *ent );
	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.

	G_UNLINKENTITY,		// ( gentity_t *ent );		
	// call before removing an interactive entity

	G_ENTITIES_IN_BOX,	// ( const vec3_t mins, const vec3_t maxs, gentity_t **list, int maxcount );
	// EntitiesInBox will return brush models based on their bounding box,
	// so exact determination must still be done with EntityContact

	G_ENTITY_CONTACT,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
	// perform an exact check against inline brush models of non-square shape

	// access for bots to get and free a server client
	G_BOT_ALLOCATE_CLIENT,			// ( void );
	G_BOT_FREE_CLIENT,				// ( int playerNum );
	G_BOT_GET_SNAPSHOT_ENTITY,		// ( int playerNum, int ent );
	G_BOT_GET_SERVER_COMMAND,		// ( int playerNum, char *command, int size );
	G_BOT_USER_COMMAND,				// ( int playerNum, usercmd_t *ucmd );
	G_CLIENT_COMMAND,				// ( int playerNum, const char *command );

	G_GET_USERCMD,	// ( int playerNum, usercmd_t *cmd )

	G_GET_ENTITY_TOKEN,	// qboolean ( int *parseOffset, char *buffer, int bufferSize )
	// Retrieves the next string token from the entity spawn text, returning
	// false when all tokens have been parsed.

	G_DEBUG_POLYGON_CREATE, // ( int color, int numPoints, vec3_t *points );
	G_DEBUG_POLYGON_SHOW, // ( int id, int color, int numPoints, vec3_t *points );
	G_DEBUG_POLYGON_DELETE, // ( int id );

	G_TRACECAPSULE,	// ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	G_ENTITY_CONTACTCAPSULE,	// ( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );

	G_SET_NET_FIELDS,

	G_R_REGISTERMODEL, // ( const char *name );
	G_R_LERPTAG, // ( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagName );
	G_R_LERPTAG_FRAMEMODEL, // ( orientation_t *tag, qhandle_t handle, qhandle_t frameModel, int startFrame, qhandle_t endFrameModel, int endFrame, float frac, const char *tagName, int *tagIndex );
	G_R_LERPTAG_TORSO, // ( orientation_t *tag, qhandle_t handle, qhandle_t frameModel, int startFrame, qhandle_t endFrameModel, int endFrame, float frac, const char *tagName, int *tagIndex,
	                   //   const vec3_t *torsoAxis, qhandle_t torsoFrameModel, int torsoFrame, qhandle_t oldTorsoFrameModel, int oldTorsoFrame, float torsoFrac );
	G_R_MODELBOUNDS, // ( qhandle_t handle, vec3_t mins, vec3_t maxs, int startFrame, int endFrame, float frac );

	G_CLIPTOENTITIES, // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
	G_CLIPTOENTITIESCAPSULE, // ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );

} gameImport_t;


//
// functions exported by the game subsystem
//
typedef enum {
	GAME_GETAPINAME = 100,
	GAME_GETAPIVERSION,

	GAME_INIT = 200,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.

	GAME_SHUTDOWN,	// (void);

	GAME_PLAYER_CONNECT,	// ( int playerNum, qboolean firstTime, qboolean isBot, int clientNum, int localPlayerNum );
	// return NULL if the player is allowed to connect, otherwise return
	// a text string with the reason for denial

	GAME_PLAYER_BEGIN,				// ( int playerNum );

	GAME_PLAYER_USERINFO_CHANGED,	// ( int playerNum );

	GAME_PLAYER_DISCONNECT,			// ( int playerNum, qboolean force );

	GAME_CLIENT_COMMAND,			// ( int clientNum );

	GAME_PLAYER_THINK,				// ( int playerNum );

	GAME_RUN_FRAME,					// ( int levelTime );

	GAME_CONSOLE_COMMAND,			// ( void );
	// G_ConsoleCommand will be called when a command has been issued
	// that is not recognized as a builtin function.
	// The game can issue trap_argc() / trap_argv() commands to get the command
	// and parameters.  Return qfalse if the game doesn't recognize it as a command.

	BOTAI_START_FRAME,				// ( int time );

	GAME_SNAPSHOT_CALLBACK,         // ( int entityNum, int playerNum );
	// return qfalse if you don't want it to be added

	GAME_VID_RESTART,				// ( void );
	// caused by vid_restart on localhost server.
	// model handles are no longer valid, must re-register all models.

	GAME_MAP_RESTART,				// ( int levelTime, int restartTime );
	// G_MapRestart will be called when a map_restart command has been issued;
	//    caused by user, VM code, or server after restart time is hit.
	// return restart time (levelTime + delay), -1 to do a full map reload,
	//    or INT_MAX to prevent map restart.

	GAME_CONSOLE_COMPLETEARGUMENT,		// ( int completeArgument );

} gameExport_t;


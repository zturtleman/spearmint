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

#define CG_API_MAJOR_VERSION	0xdead
#define CG_API_MINOR_VERSION	0xbeef

#define	CMD_BACKUP			64	
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				numPSs;
	playerState_t	pss[MAX_SPLITVIEW];		// complete information about the current players at this time
	int				lcIndex[MAX_SPLITVIEW];		// Local Client Indexes

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
} snapshot_t;

enum {
  CGAME_EVENT_NONE,
  CGAME_EVENT_TEAMMENU,
  CGAME_EVENT_SCOREBOARD,
  CGAME_EVENT_EDITHUD
};


/*
==================================================================

functions imported from the main executable

==================================================================
*/

typedef enum {
	//============== general Quake services ==================

	// See sharedTraps_t in qcommon.h for TRAP_MEMSET=0, etc

	CG_PRINT = 20,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_REAL_TIME,
	CG_SNAPVECTOR,

	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_LITERAL_ARGS,

	CG_ADDCOMMAND,
	CG_REMOVECOMMAND,
	CG_CMD_EXECUTETEXT,

	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_SET_VALUE,
	CG_CVAR_RESET,
	CG_CVAR_VARIABLE_VALUE,
	CG_CVAR_VARIABLE_INTEGER_VALUE,
	CG_CVAR_VARIABLE_STRING_BUFFER,
	CG_CVAR_LATCHED_VARIABLE_STRING_BUFFER,
	CG_CVAR_INFO_STRING_BUFFER,

	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_SEEK,
	CG_FS_FCLOSEFILE,
	CG_FS_GETFILELIST,
	CG_FS_DELETE,
	CG_FS_RENAME,

	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_REMOVE_ALL_GLOBAL_DEFINES,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_UNREAD_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,

	//=========== client game specific functionality =============

	CG_GETCLIPBOARDDATA = 100,
	CG_GETGLCONFIG,
	CG_MEMORY_REMAINING,
	CG_UPDATESCREEN,
	CG_GET_VOIP_TIME,
	CG_GET_VOIP_POWER,
	CG_GET_VOIP_GAIN,
	CG_GET_VOIP_MUTE_CLIENT,
	CG_GET_VOIP_MUTE_ALL,

	// these are not available in ui
	CG_GETGAMESTATE = 150,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_SENDCLIENTCOMMAND,


	// these are not available in ui
	CG_CM_LOADMAP = 200,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_MARKFRAGMENTS,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_TEMPBOXMODEL,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_TEMPCAPSULEMODEL,
	CG_CM_CAPSULETRACE,
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_CM_BISPHERETRACE,
	CG_CM_TRANSFORMEDBISPHERETRACE,


	CG_R_REGISTERMODEL = 300,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REGISTERFONT,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_LERPTAG,
	CG_R_MODELBOUNDS,
	CG_R_REMAP_SHADER,
	CG_R_SETCLIPREGION,
	CG_R_DRAWROTATEDPIC,
	CG_R_DRAWSTRETCHPIC_GRADIENT,
	CG_R_DRAW2DPOLYS,
	CG_R_ADDPOLYSTOSCENE,
	CG_R_ADDPOLYBUFFERTOSCENE,

	// these are not available in ui
	CG_R_LOADWORLDMAP = 350,
	CG_GET_ENTITY_TOKEN,
	CG_R_LIGHTFORPOINT,
	CG_R_INPVS,
	CG_R_ADDADDITIVELIGHTTOSCENE,
	CG_R_GET_GLOBAL_FOG,
	CG_R_GET_WATER_FOG,


	CG_S_REGISTERSOUND = 400,
	CG_S_SOUNDDURATION,
	CG_S_STARTLOCALSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_S_STOPBACKGROUNDTRACK,

	// these are not available in ui
	CG_S_STARTSOUND = 450,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_ADDREALLOOPINGSOUND,
	CG_S_STOPLOOPINGSOUND,


	CG_KEY_KEYNUMTOSTRINGBUF = 500,
	CG_KEY_GETBINDINGBUF,
	CG_KEY_SETBINDING,
	CG_KEY_ISDOWN,
	CG_KEY_GETOVERSTRIKEMODE,
	CG_KEY_SETOVERSTRIKEMODE,
	CG_KEY_CLEARSTATES,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,


 	CG_CIN_PLAYCINEMATIC = 600,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,

/*
	CG_LOADCAMERA = 700,
	CG_STARTCAMERA,
	CG_GETCAMERAINFO
*/
} cgameImport_t;


/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_GETAPIVERSION,	// system reserved

	CG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int clientNum0, int clientNum1, int clientNum2, int clientNum3 )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( int localClientNum );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( int localClientNum );

	CG_VOIP_STRING,
//  char *(*CG_VoIPString)( void );
//  returns a string of comma-delimited clientnums based on args

	CG_KEY_EVENT, 
//	void	(*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int localClientNum, int dx, int dy );

	CG_EVENT_HANDLING,
//	void (*CG_EventHandling)(int type);

	CG_CONSOLE_TEXT,
//	void (*CG_ConsoleText)( void );
//	pass text that has been printed to the console to cgame
//	use Cmd_Argc() / Cmd_Argv() to read it

	CG_WANTSBINDKEYS
//	qboolean CG_WantsBindKeys( void );

} cgameExport_t;

//----------------------------------------------

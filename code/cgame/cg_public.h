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
#ifndef __CG_PUBLIC_H__
#define __CG_PUBLIC_H__

#define CG_API_NAME				"SPEARMINT_CGAME"

// major 0 means each minor is an API break.
// major > 0 means each major is an API break and each minor extends API.
// ZTM: FIXME: There is no way for the VM to know what the engine support API is
//             so there is no way to add more system calls.
#define CG_API_MAJOR_VERSION	1
#define CG_API_MINOR_VERSION	0


#define	CMD_BACKUP			64	
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_SPLITVIEW][MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				playerNums[MAX_SPLITVIEW];

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current

	int				numEntities;
} vmSnapshot_t;

#ifdef CGAME
#define	MAX_ENTITIES_IN_SNAPSHOT	256 * MAX_SPLITVIEW

typedef struct {
	//
	// Must exactly match vmSnapshot_t
	//
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_SPLITVIEW][MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				playerNums[MAX_SPLITVIEW];

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current

	int				numEntities;

	//
	// CGame specific data
	//
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT]; // all of the entities that need to be presented
											// at the time of this snapshot

	playerState_t	pss[MAX_SPLITVIEW];		// complete information about the current players at this time

} snapshot_t;
#endif

typedef enum {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_TEAM,
	UIMENU_POSTGAME
} uiMenuCommand_t;

typedef struct {
	connstate_t		connState;
	int				connectPacketCount;
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

// Used by LAN_CompareServers
#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS		2
#define SORT_GAMETYPE		3
#define SORT_PING			4
#define SORT_HUMANS			5
#define SORT_BOTS			6
#define SORT_MAXCLIENTS		7
#define SORT_GAMEDIR		8

// server browser sources
#define AS_LOCAL			0
#define AS_FAVORITES		1
#define AS_GLOBAL			2
#define AS_NUM_SOURCES		3

typedef enum {
	DS_NONE,

	DS_PLAYBACK,
	DS_RECORDING,

	DS_NUM_DEMO_STATES
} demoState_t;

// bit flags for trap_Mouse_GetState and trap_Mouse_SetState
#define MOUSE_CLIENT			0x0001		// update mouse x and y for usercmd_t creation, don't allow mouse to leave window
#define MOUSE_CGAME				0x0002		// call CG_MOUSE_EVENT when mouse moves
#define MOUSE_SYSTEMCURSOR		0x0004		// show system cursor, ignored if MOUSE_CLIENT is set or fullscreen

enum {
	JOYEVENT_NONE,
	JOYEVENT_AXIS,
	JOYEVENT_BUTTON,
	JOYEVENT_HAT,

	JOYEVENT_MAX
};

typedef struct {
	int type; // JOYEVENT_*

	union {
		int button;
		struct {
			int num;
			int sign; // 1 or -1
		} axis;
		struct {
			int num;
			int mask;
		} hat;
	} value;
} joyevent_t;

/*
==================================================================

functions imported from the main executable

also see qvmTraps_t in qcommon.h for QVM-specific system calls

==================================================================
*/

typedef enum {
	//============== general Quake services ==================

	CG_PRINT = 0,
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
	CG_CVAR_DEFAULT_VARIABLE_STRING_BUFFER,
	CG_CVAR_INFO_STRING_BUFFER,
	CG_CVAR_CHECK_RANGE,

	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_SEEK,
	CG_FS_TELL,
	CG_FS_FCLOSEFILE,
	CG_FS_GETFILELIST,
	CG_FS_DELETE,
	CG_FS_RENAME,

	CG_PC_ADD_GLOBAL_DEFINE,			// ( const char *define );
	CG_PC_REMOVE_GLOBAL_DEFINE,			// ( const char *define );
	CG_PC_REMOVE_ALL_GLOBAL_DEFINES,	// ( void );
	CG_PC_LOAD_SOURCE,					// ( const char *filename, const char *basepath );
	CG_PC_FREE_SOURCE,					// ( int handle );
	CG_PC_ADD_DEFINE,					// ( int handle, const char *define );
	CG_PC_READ_TOKEN,					// ( int handle, pc_token_t *pc_token );
	CG_PC_UNREAD_TOKEN,					// ( int handle );
	CG_PC_SOURCE_FILE_AND_LINE,			// ( int handle, char *filename, int *line );

	CG_HEAP_MALLOC,		// ( int size );
	CG_HEAP_AVAILABLE,	// ( void );
	CG_HEAP_FREE,		// ( void *data );

	CG_FIELD_COMPLETEFILENAME, // ( const char *dir, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk );
	CG_FIELD_COMPLETECOMMAND, // ( const char *cmd, qboolean doCommands, qboolean doCvars );
	CG_FIELD_COMPLETELIST, // ( const char *list );

	//=========== client game specific functionality =============

	CG_GETCLIPBOARDDATA = 100,
	CG_GETGLCONFIG,
	CG_MEMORY_REMAINING,
	CG_UPDATESCREEN,
	CG_GET_VOIP_TIME,
	CG_GET_VOIP_POWER,
	CG_GET_VOIP_GAIN,
	CG_GET_VOIP_MUTE_PLAYER,
	CG_GET_VOIP_MUTE_ALL,
	CG_CMD_AUTOCOMPLETE,
	CG_SV_SHUTDOWN,

	// note: these were not originally available in ui
	CG_GETGAMESTATE = 150,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SENDCLIENTCOMMAND,
	CG_SET_NET_FIELDS,
	CG_GETDEMOSTATE,
	CG_GETDEMOPOS,
	CG_GETDEMONAME,
	CG_GETDEMOLENGTH,
	CG_SETMAPTITLE,
	CG_SETVIEWANGLES,
	CG_GETVIEWANGLES,
	CG_GETDEMOFILEINFO,

	// note: these were not originally available in cgame
	CG_GETCLIENTSTATE = 190,
	CG_GETCONFIGSTRING,

	// note: these were not originally available in ui
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
	CG_R_REGISTERSHADEREX,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REGISTERFONT,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_ADDADDITIVELIGHTTOSCENE,
	CG_R_ADDVERTEXLIGHTTOSCENE,
	CG_R_ADDJUNIORLIGHTTOSCENE,
	CG_R_ADDDIRECTEDLIGHTTOSCENE,
	CG_R_ADDCORONATOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_LERPTAG,
	CG_R_LERPTAG_FRAMEMODEL,
	CG_R_LERPTAG_TORSO,
	CG_R_MODELBOUNDS,
	CG_R_REMAP_SHADER,
	CG_R_SETCLIPREGION,
	CG_R_DRAWROTATEDPIC,
	CG_R_DRAWSTRETCHPIC_GRADIENT,
	CG_R_DRAW2DPOLYS,
	CG_R_ADDPOLYSTOSCENE,
	CG_R_ADDPOLYBUFFERTOSCENE,
	CG_R_ALLOCSKINSURFACE,
	CG_R_ADDSKINTOFRAME,
	CG_R_ADDPOLYREFENTITYTOSCENE,

	// note: these were not originally available in ui
	CG_R_LOADWORLDMAP = 350,
	CG_GET_ENTITY_TOKEN,
	CG_R_LIGHTFORPOINT,
	CG_R_INPVS,
	CG_R_GET_GLOBAL_FOG,
	CG_R_GET_VIEW_FOG,
	CG_R_SET_SURFACE_SHADER,
	CG_R_GET_SURFACE_SHADER,
	CG_R_GET_SHADER_FROM_MODEL,
	CG_R_GET_SHADER_NAME,


	CG_S_REGISTERSOUND = 400,
	CG_S_SOUNDDURATION,
	CG_S_STARTLOCALSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_S_STOPBACKGROUNDTRACK,
	CG_S_STARTSTREAMINGSOUND,
	CG_S_STOPSTREAMINGSOUND,
	CG_S_QUEUESTREAMINGSOUND,
	CG_S_GETSTREAMPLAYCOUNT,
	CG_S_SETSTREAMVOLUME,
	CG_S_STOPALLSOUNDS,

	// note: these were not originally available in ui
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
	CG_KEY_GETCAPSLOCKMODE,
	CG_KEY_GETNUMLOCKMODE,
	CG_KEY_GETOVERSTRIKEMODE,
	CG_KEY_SETOVERSTRIKEMODE,
	CG_KEY_CLEARSTATES,
	CG_KEY_GETKEY,
	CG_KEY_SETREPEAT,

	CG_MOUSE_GETSTATE,
	CG_MOUSE_SETSTATE,

	CG_SET_KEY_FOR_JOY_EVENT, // ( int localPlayerNum, const joyevent_t *joyevent, int keynum );
	CG_GET_KEY_FOR_JOY_EVENT, // ( int localPlayerNum, const joyevent_t *joyevent );
	CG_GET_JOY_EVENT_FOR_KEY, // ( int localPlayerNum, int keynum, int startIndex, joyevent_t *joyevent );
	CG_JOY_EVENT_TO_STRING, // ( const joyevent_t *joyevent, char *buf, int size );


	CG_LAN_GETPINGQUEUECOUNT = 600,
	CG_LAN_CLEARPING,
	CG_LAN_GETPING,
	CG_LAN_GETPINGINFO,

	CG_LAN_GETSERVERCOUNT,
	CG_LAN_GETSERVERADDRESSSTRING,
	CG_LAN_GETSERVERINFO,
	CG_LAN_MARKSERVERVISIBLE,
	CG_LAN_UPDATEVISIBLEPINGS,
	CG_LAN_RESETPINGS,
	CG_LAN_LOADCACHEDSERVERS,
	CG_LAN_SAVECACHEDSERVERS,
	CG_LAN_ADDSERVER,
	CG_LAN_REMOVESERVER,

	CG_LAN_SERVERSTATUS,
	CG_LAN_GETSERVERPING,
	CG_LAN_SERVERISVISIBLE,
	CG_LAN_COMPARESERVERS,

	CG_LAN_SERVERISINFAVORITELIST,


	CG_CIN_PLAYCINEMATIC = 700,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,

/*
	CG_LOADCAMERA = 800,
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
	CG_GETAPINAME = 100,
	CG_GETAPIVERSION,

	CG_INIT = 200,
//	void	UI_Init( connstate_t state, int maxSplitView, int playVideo );
	// playVideo = 1 means first game to run and no start up arguments
	// playVideo = 2 means switched to a new game/mod and not connecting to a server

	CG_INGAME_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int maxSplitView, int playerNum0, int playerNum1, int playerNum2, int playerNum3 )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// opportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( connstate_t state, int realTime );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_REFRESH,
//	void (*CG_Refresh)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback, connstate_t state, int realTime );
	// Draws UI and if connected to a server, generates and draws a game scene
	// and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( int localPlayerNum );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( int localPlayerNum );

	CG_VOIP_STRING,
//  char *(*CG_VoIPString)( void );
	// pass voip target token unknown by client to cgame to convert into playerNums
	// use Cmd_Argc() / Cmd_Argv() to read the target token, return a
	// string of comma-delimited playerNums based on target token or
	// NULL if unknown token.

	CG_KEY_EVENT, 
//	void	(*CG_KeyEvent)( int key, qboolean down, unsigned time, connstate_t state );

	CG_CHAR_EVENT,
//	void	(*CG_CharEvent)( int character, connstate_t state );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int localPlayerNum, int dx, int dy );

	CG_JOYSTICK_AXIS_EVENT,
//	void	(*CG_JoystickAxisEvent)( int localPlayerNum, int axis, int value, unsigned time, connstate_t state, int negKey, int posKey );

	CG_JOYSTICK_BUTTON_EVENT,
//	void	(*CG_JoystickButtonEvent)( int localPlayerNum, int button, qboolean down, unsigned time, connstate_t state, int key );

	CG_JOYSTICK_HAT_EVENT,
//	void	(*CG_JoystickHatEvent)( int localPlayerNum, int hat, int state, unsigned time, connstate_t state, int upKey, int rightKey, int downKey, int leftKey );

	CG_MOUSE_POSITION,
//  int		(*CG_MousePosition)( int localPlayerNum );

	CG_SET_MOUSE_POSITION,
//  void	(*CG_SetMousePosition)( int localPlayerNum, int x, int y );

	CG_SET_ACTIVE_MENU,
//	void (*CG_SetActiveMenu)( uiMenuCommand_t menu );

	CG_CONSOLE_TEXT,
//	void (*CG_ConsoleText)( int realTime, qboolean restoredText );
//	pass text that has been printed to the console to cgame
//	use Cmd_Argc() / Cmd_Argv() to read it
//	if restoredText is qtrue, text is being added from before cgame was loaded.

	CG_CONSOLE_CLOSE,
//	void Con_Close( void );
//	force console to close, used before loading screens

	CG_CREATE_USER_CMD,
//	usercmd_t *CG_CreateUserCmd( int localPlayerNum, int frameTime, int frameMsec, float mx, float my, qboolean anykeydown );

	CG_UPDATE_GLCONFIG,
//	void	CG_UpdateGLConfig( void );

	CG_CONSOLE_COMPLETEARGUMENT,
//	qboolean (*CG_ConsoleCompleteArgument)( connstate_t state, int realTime, int completeArgument );

} cgameExport_t;

#endif

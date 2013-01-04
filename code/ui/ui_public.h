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
#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__

#define UI_API_MAJOR_VERSION	0xdead
#define UI_API_MINOR_VERSION	0xbeef

typedef struct {
	connstate_t		connState;
	int				connectPacketCount;
	int				clientNums[MAX_SPLITVIEW];
	int				psClientNums[MAX_SPLITVIEW]; // clientNum from local client's playerState_t, which could be a followed client.
	char			servername[MAX_STRING_CHARS];
	char			updateInfoString[MAX_STRING_CHARS];
	char			messageString[MAX_STRING_CHARS];
} uiClientState_t;

typedef enum {
	//============== general Quake services ==================

	// See sharedTraps_t in qcommon.h for TRAP_MEMSET=0, etc

	UI_PRINT = 20,
	UI_ERROR,
	UI_MILLISECONDS,
	UI_REAL_TIME,
	UI_SNAPVECTOR,

	UI_ARGC,
	UI_ARGV,
	UI_ARGS,
	UI_LITERAL_ARGS,

	UI_ADDCOMMAND,
	UI_REMOVECOMMAND,
	UI_CMD_EXECUTETEXT,

	UI_CVAR_REGISTER,
	UI_CVAR_UPDATE,
	UI_CVAR_SET,
	UI_CVAR_SET_VALUE,
	UI_CVAR_RESET,
	UI_CVAR_VARIABLE_VALUE,
	UI_CVAR_VARIABLE_INTEGER_VALUE,
	UI_CVAR_VARIABLE_STRING_BUFFER,
	UI_CVAR_LATCHED_VARIABLE_STRING_BUFFER,
	UI_CVAR_INFO_STRING_BUFFER,

	UI_FS_FOPENFILE,
	UI_FS_READ,
	UI_FS_WRITE,
	UI_FS_SEEK,
	UI_FS_FCLOSEFILE,
	UI_FS_GETFILELIST,
	UI_FS_DELETE,
	UI_FS_RENAME,

	UI_PC_ADD_GLOBAL_DEFINE,
	UI_PC_REMOVE_ALL_GLOBAL_DEFINES,
	UI_PC_LOAD_SOURCE,
	UI_PC_FREE_SOURCE,
	UI_PC_READ_TOKEN,
	UI_PC_UNREAD_TOKEN,
	UI_PC_SOURCE_FILE_AND_LINE,

	//=========== client ui specific functionality =============

	UI_GETCLIPBOARDDATA = 100,
	UI_GETGLCONFIG,
	UI_MEMORY_REMAINING,
	UI_UPDATESCREEN,
	UI_GET_VOIP_TIME,
	UI_GET_VOIP_POWER,
	UI_GET_VOIP_GAIN,
	UI_GET_VOIP_MUTE_CLIENT,
	UI_GET_VOIP_MUTE_ALL,

	// not available in cgame
	UI_GETCLIENTSTATE = 150,
	UI_GETCONFIGSTRING,


	// NOTE: LAN functions are not available in cgame.
	UI_LAN_GETPINGQUEUECOUNT = 200,
	UI_LAN_CLEARPING,
	UI_LAN_GETPING,
	UI_LAN_GETPINGINFO,

	UI_LAN_GETSERVERCOUNT,
	UI_LAN_GETSERVERADDRESSSTRING,
	UI_LAN_GETSERVERINFO,
	UI_LAN_MARKSERVERVISIBLE,
	UI_LAN_UPDATEVISIBLEPINGS,
	UI_LAN_RESETPINGS,
	UI_LAN_LOADCACHEDSERVERS,
	UI_LAN_SAVECACHEDSERVERS,
	UI_LAN_ADDSERVER,
	UI_LAN_REMOVESERVER,

	UI_LAN_SERVERSTATUS,
	UI_LAN_GETSERVERPING,
	UI_LAN_SERVERISVISIBLE,
	UI_LAN_COMPARESERVERS,


	UI_R_REGISTERMODEL = 300,
	UI_R_REGISTERSKIN,
	UI_R_REGISTERSHADER,
	UI_R_REGISTERSHADERNOMIP,
	UI_R_REGISTERFONT,
	UI_R_CLEARSCENE,
	UI_R_ADDREFENTITYTOSCENE,
	UI_R_ADDPOLYTOSCENE,
	UI_R_ADDLIGHTTOSCENE,
	UI_R_RENDERSCENE,
	UI_R_SETCOLOR,
	UI_R_DRAWSTRETCHPIC,
	UI_R_LERPTAG,
	UI_R_MODELBOUNDS,
	UI_R_REMAP_SHADER,
	UI_R_SETCLIPREGION,
	UI_R_DRAWROTATEDPIC,
	UI_R_DRAWSTRETCHPIC_GRADIENT,
	UI_R_DRAW2DPOLYS,
	UI_R_ADDPOLYSTOSCENE,
	UI_R_ADDPOLYBUFFERTOSCENE,


	UI_S_REGISTERSOUND = 400,
	UI_S_SOUNDDURATION,
	UI_S_STARTLOCALSOUND,
	UI_S_STARTBACKGROUNDTRACK,
	UI_S_STOPBACKGROUNDTRACK,


	UI_KEY_KEYNUMTOSTRINGBUF = 500,
	UI_KEY_GETBINDINGBUF,
	UI_KEY_SETBINDING,
	UI_KEY_ISDOWN,
	UI_KEY_GETOVERSTRIKEMODE,
	UI_KEY_SETOVERSTRIKEMODE,
	UI_KEY_CLEARSTATES,
	UI_KEY_GETCATCHER,
	UI_KEY_SETCATCHER,
	UI_KEY_GETKEY,


	UI_CIN_PLAYCINEMATIC = 600,
	UI_CIN_STOPCINEMATIC,
	UI_CIN_RUNCINEMATIC,
	UI_CIN_DRAWCINEMATIC,
	UI_CIN_SETEXTENTS

} uiImport_t;

typedef enum {
	UIMENU_NONE,
	UIMENU_MAIN,
	UIMENU_INGAME,
	UIMENU_TEAM,
	UIMENU_POSTGAME
} uiMenuCommand_t;

#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS		2
#define SORT_GAME			3
#define SORT_PING			4

typedef enum {
	UI_GETAPIVERSION = 0,	// system reserved

	UI_INIT,
//	void	UI_Init( qboolean inGameLoad, int maxSplitView );

	UI_SHUTDOWN,
//	void	UI_Shutdown( void );

	UI_KEY_EVENT,
//	void	UI_KeyEvent( int key );

	UI_MOUSE_EVENT,
//	void	UI_MouseEvent( int localClientNum, int dx, int dy );

	UI_MOUSE_POSITION,
//  int		UI_MousePosition( int localClientNum );

	UI_SET_MOUSE_POSITION,
//  void	UI_SetMousePosition( int localClientNum, int x, int y );

	UI_REFRESH,
//	void	UI_Refresh( int time );

	UI_IS_FULLSCREEN,
//	qboolean UI_IsFullscreen( void );

	UI_SET_ACTIVE_MENU,
//	void	UI_SetActiveMenu( uiMenuCommand_t menu );

	UI_CONSOLE_COMMAND,
//	qboolean UI_ConsoleCommand( int realTime );

	UI_DRAW_CONNECT_SCREEN,
//	void	UI_DrawConnectScreen( qboolean overlay );
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings

	UI_WANTSBINDKEYS,
//	qboolean UI_WantsBindKeys( void );

} uiExport_t;

#endif

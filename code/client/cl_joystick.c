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
#include "client.h"

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
	};
} joyevent_t;

typedef struct {
	joyevent_t	event;
	int			keynum;
} joyeventkey_t;

#define MAX_JOY_REMAPS	32 // 5 (LS) + 5 (RS) + 4 (DPAD) + 18 button/hat/trigger events
joyeventkey_t	joyEventRemap[CL_MAX_SPLITVIEW][MAX_JOY_REMAPS];

char joystickRemapIdent[CL_MAX_SPLITVIEW][MAX_QPATH];

// joystick remaps are not cross-platform
#ifdef _WIN32
// use generic "windows" instead of being separate for mingw/msvc and 32/64 bit
#define JOY_PLATFORM "windows"
#else
#define JOY_PLATFORM OS_STRING
#endif


/*
===================
CL_StringToJoyEvent

Returns qtrue if string is a valid event.
===================
*/
qboolean CL_StringToJoyEvent( char *str, joyevent_t *event ) {
	Com_Memset( event, 0, sizeof ( *event ) );

	if ( !str || !str[0] ) {
		return qfalse;
	}

	// button0
	if ( !Q_stricmpn( str, "button", 6 ) ) {
		event->type = JOYEVENT_BUTTON;
		event->button = atoi( &str[6] );
		return qtrue;
	}
	// b0
	else if ( str[0] == 'b' ) {
		event->type = JOYEVENT_BUTTON;
		event->button = atoi( &str[1] );
		return qtrue;
	}
	// hat0.8
	else if ( !Q_stricmpn( str, "hat", 3 ) ) {
		event->type = JOYEVENT_BUTTON;
		event->hat.num = str[3] - '0';
		event->hat.mask = str[5] - '0';
		return qtrue;
	}
	// h0.8
	else if ( str[0] == 'h' && strlen( str ) == 4 ) {
		event->type = JOYEVENT_HAT;
		event->hat.num = str[1] - '0';
		event->hat.mask = str[3] - '0';
		return qtrue;
	}
	else if ( str[0] == '+' || str[0] == '-' ) {
		// +axis0
		if ( !Q_stricmpn( str+1, "axis", 4 ) ) {
			event->type = JOYEVENT_AXIS;
			event->axis.num = atoi( &str[5] );
			event->axis.sign = ( str[0] == '+' ) ? 1 : -1;
			return qtrue;
		}
		// +a0
		else if ( str[1] == 'a' ) {
			event->type = JOYEVENT_AXIS;
			event->axis.num = atoi( &str[2] );
			event->axis.sign = ( str[0] == '+' ) ? 1 : -1;
			return qtrue;
		}
	}

	return qfalse;
}

/*
===================
CL_JoyEventToString

Returns a string for the given joystick event.
===================
*/
char *CL_JoyEventToString( const joyevent_t *event ) {
	static	char	str[32];

	switch ( event->type ) {
		case JOYEVENT_AXIS:
			Com_sprintf( str, sizeof( str ), "%saxis%d",
						( event->axis.sign == -1 ) ? "-" : "+",
						event->axis.num );
			break;
		case JOYEVENT_BUTTON:
			Com_sprintf( str, sizeof( str ), "button%d",
						event->button );
			break;
		case JOYEVENT_HAT:
			Com_sprintf( str, sizeof( str ), "hat%d.%d",
						event->hat.num, event->hat.mask );
			break;
		default:
			Q_strncpyz( str, "<UNKNOWN-EVENT>", sizeof ( str ) );
	}

	return str;
}

/*
===================
CL_JoyEventsMatch

Returns qtrue if events are the same
===================
*/
qboolean CL_JoyEventsMatch( const joyevent_t *e1, const joyevent_t *e2 ) {
	if ( e1->type != e2->type )
		return qfalse;

	switch ( e1->type ) {
		case JOYEVENT_AXIS:
			return ( e1->axis.num == e2->axis.num && e1->axis.sign == e2->axis.sign );

		case JOYEVENT_HAT:
			return ( e1->hat.num == e2->hat.num && e1->hat.mask == e2->hat.mask );

		case JOYEVENT_BUTTON:
			return ( e1->button == e2->button );

		default:
			return qfalse;
	}
}

/*
===================
CL_SetKeyForJoyEvent

If keynum is -1 removes remap for joyevent. Returns qtrue if there was an event.
Else; Returns qfalse if cannot add joystick event remap due to all slots being full.
===================
*/
qboolean CL_SetKeyForJoyEvent( int localPlayerNum, const joyevent_t *joyevent, int keynum ) {
	int i;
	int freeSlot = -1;

	for ( i = 0; i < MAX_JOY_REMAPS; i++ ) {
		if ( joyEventRemap[localPlayerNum][i].event.type == JOYEVENT_NONE ) {
			if ( freeSlot == -1 ) {
				freeSlot = i;
			}
			continue;
		}

		if ( !CL_JoyEventsMatch( &joyEventRemap[localPlayerNum][i].event, joyevent ) )
			continue;

		if ( keynum == -1 ) {
			joyEventRemap[localPlayerNum][i].event.type = JOYEVENT_NONE;
			return qtrue;
		}

		joyEventRemap[localPlayerNum][i].keynum = keynum;
		return qtrue;
	}

	// didn't find existing remap to replace, create new
	if ( freeSlot != -1 && keynum != -1 ) {
		joyEventRemap[localPlayerNum][freeSlot].event = *joyevent;
		joyEventRemap[localPlayerNum][freeSlot].keynum = keynum;
		return qtrue;
	}

	return qfalse;
}

/*
===================
CL_GetKeyForJoyEvent

Returns keynum.
===================
*/
int CL_GetKeyForJoyEvent( int localPlayerNum, const joyevent_t *joyevent ) {
	int keynum = -1;
	int i;

	for ( i = 0; i < MAX_JOY_REMAPS; i++ ) {
		if ( !CL_JoyEventsMatch( &joyEventRemap[localPlayerNum][i].event, joyevent ) )
			continue;

		keynum = joyEventRemap[localPlayerNum][i].keynum;
		break;
	}

	return keynum;
}

/*
===================
Cmd_JoyUnmap_f
===================
*/
void Cmd_JoyUnmap_f (void)
{
	joyevent_t	joyevent;
	int			localPlayerNum;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("%s <event> : remove a joystick event remap\n", Cmd_Argv(0) );
		return;
	}

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	if (!CL_StringToJoyEvent(Cmd_Argv(1), &joyevent))
	{
		Com_Printf ("\"%s\" isn't a valid joystick event\n", Cmd_Argv(1));
		return;
	}

	if ( !CL_SetKeyForJoyEvent( localPlayerNum, &joyevent, -1 ) ) {
		Com_Printf ("\"%s\" is not remapped\n", CL_JoyEventToString( &joyevent ) );
	}
}

/*
===================
Cmd_JoyUnmapAll_f
===================
*/
void Cmd_JoyUnmapAll_f (void)
{
	int			localPlayerNum;

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	Com_Memset( &joyEventRemap[localPlayerNum], 0, sizeof ( joyEventRemap[0] ) );
}

/*
===================
Cmd_JoyRemap_f
===================
*/
void Cmd_JoyRemap_f (void)
{
	int			c, key;
	joyevent_t	joyevent;
	int			localPlayerNum;

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("%s <event> [key] : attach a joystick event to a virtual key\n", Cmd_Argv(0));
		return;
	}

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	if (!CL_StringToJoyEvent(Cmd_Argv(1), &joyevent))
	{
		Com_Printf ("\"%s\" isn't a valid joystick event\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		key = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );
		if ( key != -1 )
			Com_Printf ("\"%s\" = \"%s\"\n", CL_JoyEventToString( &joyevent ), Key_KeynumToString( key ) );
		else
			Com_Printf ("\"%s\" is not remapped\n", CL_JoyEventToString( &joyevent ) );
		return;
	}

	key = Key_StringToKeynum (Cmd_Argv(2));
	if (key==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(2));
		return;
	}

	if ( !CL_SetKeyForJoyEvent( localPlayerNum, &joyevent, key ) ) {
		Com_Printf ("Max joystick remaps reached (%d), cannot add remap for %s.\n", MAX_JOY_REMAPS, CL_JoyEventToString( &joyevent ) );
	}
}

/*
============
Cmd_JoyRemapList_f
============
*/
void Cmd_JoyRemapList_f( void ) {
	int		i;
	int		localPlayerNum;

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	for ( i = 0 ; i < MAX_JOY_REMAPS ; i++ ) {
		if ( joyEventRemap[localPlayerNum][i].event.type == JOYEVENT_NONE ) {
			continue;
		}

		Com_Printf ("%s %s\n", CL_JoyEventToString( &joyEventRemap[localPlayerNum][i].event ),
								Key_KeynumToString( joyEventRemap[localPlayerNum][i].keynum ) );
	}
}

#if 0
/*
====================
Cmd_CompleteJoyRemap
====================
*/
static void Cmd_CompleteJoyUnmap( char *args, int argNum )
{
	if( argNum == 2 )
	{
		// Skip "joyunmap "
		char *p = Com_SkipTokens( args, 1, " " );

		// ZTM: TODO: add completion for (existing) joystick event?
	}
}
#endif

/*
====================
Cmd_CompleteJoyRemap
====================
*/
static void Cmd_CompleteJoyRemap( char *args, int argNum )
{
	char *p;

	if( argNum == 2 )
	{
		// Skip "joyremap "
		//p = Com_SkipTokens( args, 1, " " );

		// ZTM: TODO: add completion for (any) joystick event?
	}
	else if( argNum == 3 )
	{
		// Skip "joyremap <event> "
		p = Com_SkipTokens( args, 2, " " );

		if( p > args )
			Field_CompleteKeyname( );
	}
}

/*
===================
CL_InitJoyRemapCommands
===================
*/
void CL_InitJoyRemapCommands( void ) {
	int i;

	Com_Memset( &joyEventRemap, 0, sizeof ( joyEventRemap ) );
	Com_Memset( &joystickRemapIdent, 0, sizeof ( joystickRemapIdent ) );

	// register our functions
	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		Cmd_AddCommand( Com_LocalPlayerCvarName( i, "joyremap" ),Cmd_JoyRemap_f );
		Cmd_SetCommandCompletionFunc( Com_LocalPlayerCvarName( i, "joyremap" ), Cmd_CompleteJoyRemap );

		Cmd_AddCommand( Com_LocalPlayerCvarName( i, "joyunmap" ), Cmd_JoyUnmap_f );
		//Cmd_SetCommandCompletionFunc( Com_LocalPlayerCvarName( i, "joyunmap" ), Cmd_CompleteJoyUnmap );

		Cmd_AddCommand( Com_LocalPlayerCvarName( i, "joyunmapall" ), Cmd_JoyUnmapAll_f );

		Cmd_AddCommand( Com_LocalPlayerCvarName( i, "joyremaplist" ), Cmd_JoyRemapList_f);
	}
}


/*
=================
CL_SaveJoystickRemap

Write out "<event> <key>" lines
=================
*/
void CL_SaveJoystickRemap( int localPlayerNum ) {
	fileHandle_t	f;
	char			filename[MAX_QPATH];
	int				i;

	if ( !*joystickRemapIdent[localPlayerNum] ) {
		return;
	}

	Com_sprintf( filename, sizeof ( filename ), "joy-%s-p%d-%s.txt", JOY_PLATFORM, localPlayerNum+1, joystickRemapIdent[localPlayerNum] );

	f = FS_SV_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf ("Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf (f, "// Generated by " PRODUCT_NAME ", do not modify\n");

	for ( i = 0 ; i < MAX_JOY_REMAPS ; i++ ) {
		if ( joyEventRemap[localPlayerNum][i].event.type == JOYEVENT_NONE ) {
			continue;
		}

		FS_Printf (f, "%s %s\n", CL_JoyEventToString( &joyEventRemap[localPlayerNum][i].event ),
								Key_KeynumToString( joyEventRemap[localPlayerNum][i].keynum ) );
	}

	FS_FCloseFile( f );
}

/*
=================
CL_LoadJoystickRemap

joystickIdent could be a name or hash
=================
*/
qboolean CL_LoadJoystickRemap( int localPlayerNum, const char *joystickIdent ) {
	fileHandle_t	f;
	char		filename[MAX_QPATH];
	char		*buffer, *text, *token;
	int			len;
	joyevent_t	joyevent;
	int			key;

	if ( !strcmp(joystickRemapIdent[localPlayerNum], joystickIdent ) ) {
		return qtrue;
	}
	Q_strncpyz( joystickRemapIdent[localPlayerNum], joystickIdent, sizeof ( joystickRemapIdent[0] ) );

	// clear existing remap
	Com_Memset( &joyEventRemap[localPlayerNum], 0, sizeof ( joyEventRemap[0] ) );

	Com_sprintf( filename, sizeof ( filename ), "joy-%s-p%d-%s.txt", JOY_PLATFORM, localPlayerNum+1, joystickRemapIdent[localPlayerNum] );
	len = FS_SV_FOpenFileRead( filename, &f );
	if ( !f ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: Couldn't load %s\n", filename);
		return qfalse;
	}

	buffer = Hunk_AllocateTempMemory(len+1);

	FS_Read (buffer, len, f);

	// guarantee that it will have a trailing 0 for string operations
	buffer[len] = 0;
	FS_FCloseFile( f );

	text = buffer;

	while ( 1 ) {
		token = COM_Parse( &text );
		if ( !*token ) {
			break;
		}

		if ( !CL_StringToJoyEvent( token, &joyevent) )
		{
			SkipRestOfLine( &text );
			Com_Printf ("\"%s\" isn't a valid joystick event in %s\n", token, filename );
			continue;
		}

		token = COM_ParseExt( &text, qfalse );
		if ( !*token ) {
			Com_Printf("WARNING: Missing key for joy event in %s\n", filename );
			continue;
		}

		key = Key_StringToKeynum( token );
		if ( key == -1 )
		{
			Com_Printf( "\"%s\" isn't a valid key in %s\n", token, filename );
			continue;
		}

		if ( !CL_SetKeyForJoyEvent( localPlayerNum, &joyevent, key ) ) {
			Com_Printf ("Max joystick remaps reached (%d), cannot add remap for %s.\n", MAX_JOY_REMAPS, CL_JoyEventToString( &joyevent ) );
			break;
		}
	}

	Hunk_FreeTempMemory( buffer );

	return qtrue;
}


/*
=================
CL_JoystickAxisEvent
=================
*/
void CL_JoystickAxisEvent( int localPlayerNum, int axis, int value, unsigned time ) {
	joyevent_t joyevent;
	int negKey, posKey;

	if ( !cgvm ) {
		return;
	}

	joyevent.type = JOYEVENT_AXIS;
	joyevent.axis.num = axis;

	joyevent.axis.sign = -1;
	negKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	joyevent.axis.sign = 1;
	posKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	VM_Call( cgvm, CG_JOYSTICK_AXIS_EVENT, localPlayerNum, axis, value, time, clc.state, negKey, posKey );
}

/*
=================
CL_JoystickButtonEvent
=================
*/
void CL_JoystickButtonEvent( int localPlayerNum, int button, qboolean down, unsigned time ) {
	joyevent_t joyevent;
	int key;

	if ( !cgvm ) {
		return;
	}

	joyevent.type = JOYEVENT_BUTTON;
	joyevent.button = button;
	key = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	VM_Call( cgvm, CG_JOYSTICK_BUTTON_EVENT, localPlayerNum, button, down, time, clc.state, key );
}

/*
=================
CL_JoystickHatEvent
=================
*/
void CL_JoystickHatEvent( int localPlayerNum, int hat, int state, unsigned time ) {
	joyevent_t joyevent;
	int upKey, rightKey, downKey, leftKey;

	if ( !cgvm ) {
		return;
	}

	joyevent.type = JOYEVENT_HAT;
	joyevent.hat.num = hat;

	joyevent.hat.mask = HAT_UP;
	upKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	joyevent.hat.mask = HAT_RIGHT;
	rightKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	joyevent.hat.mask = HAT_DOWN;
	downKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	joyevent.hat.mask = HAT_LEFT;
	leftKey = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );

	VM_Call( cgvm, CG_JOYSTICK_HAT_EVENT, localPlayerNum, hat, state, time, clc.state, upKey, rightKey, downKey, leftKey );
}


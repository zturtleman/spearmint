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

typedef struct {
	joyevent_t	event;
	int			keynum;
} joyeventkey_t;

#define MAX_JOY_REMAPS	32 // 5 (LS) + 5 (RS) + 4 (DPAD) + 18 button/hat/trigger events
typedef struct {
	char			ident[MAX_QPATH];
	char			name[MAX_QPATH];
	int				references;
	joyeventkey_t	remap[MAX_JOY_REMAPS];
	qboolean		modified;	// qtrue if remap has been modified and needs to be saved
} joyDevice_t;

joyDevice_t joyDevice[CL_MAX_SPLITVIEW];
int playerJoyRemapIndex[CL_MAX_SPLITVIEW];

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
		event->value.button = atoi( &str[6] );
		return qtrue;
	}
	// b0
	else if ( str[0] == 'b' ) {
		event->type = JOYEVENT_BUTTON;
		event->value.button = atoi( &str[1] );
		return qtrue;
	}
	// hat0.8
	else if ( !Q_stricmpn( str, "hat", 3 ) ) {
		event->type = JOYEVENT_BUTTON;
		event->value.hat.num = str[3] - '0';
		event->value.hat.mask = str[5] - '0';
		return qtrue;
	}
	// h0.8
	else if ( str[0] == 'h' && strlen( str ) == 4 ) {
		event->type = JOYEVENT_HAT;
		event->value.hat.num = str[1] - '0';
		event->value.hat.mask = str[3] - '0';
		return qtrue;
	}
	else if ( str[0] == '+' || str[0] == '-' ) {
		// +axis0
		if ( !Q_stricmpn( str+1, "axis", 4 ) ) {
			event->type = JOYEVENT_AXIS;
			event->value.axis.num = atoi( &str[5] );
			event->value.axis.sign = ( str[0] == '+' ) ? 1 : -1;
			return qtrue;
		}
		// +a0
		else if ( str[1] == 'a' ) {
			event->type = JOYEVENT_AXIS;
			event->value.axis.num = atoi( &str[2] );
			event->value.axis.sign = ( str[0] == '+' ) ? 1 : -1;
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
						( event->value.axis.sign == -1 ) ? "-" : "+",
						event->value.axis.num );
			break;
		case JOYEVENT_BUTTON:
			Com_sprintf( str, sizeof( str ), "button%d",
						event->value.button );
			break;
		case JOYEVENT_HAT:
			Com_sprintf( str, sizeof( str ), "hat%d.%d",
						event->value.hat.num, event->value.hat.mask );
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
			return ( e1->value.axis.num == e2->value.axis.num && e1->value.axis.sign == e2->value.axis.sign );

		case JOYEVENT_HAT:
			return ( e1->value.hat.num == e2->value.hat.num && e1->value.hat.mask == e2->value.hat.mask );

		case JOYEVENT_BUTTON:
			return ( e1->value.button == e2->value.button );

		default:
			return qfalse;
	}
}

/*
===================
CL_SetKeyForJoyEvent

If keynum is -1 removes remap for joyevent. Returns qtrue if there was an event.
Else; Returns qtrue if updated or added event/key.
===================
*/
qboolean CL_SetKeyForJoyEvent( int localPlayerNum, const joyevent_t *joyevent, int keynum ) {
	int i;
	int freeSlot = -1;
	joyDevice_t *device;

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 )
		return qfalse;

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];

	// always remap to main joystick enum range
	if ( keynum >= K_FIRST_2JOY && keynum <= K_LAST_2JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_2JOY;
	}
	else if ( keynum >= K_FIRST_3JOY && keynum <= K_LAST_3JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_3JOY;
	}
	else if ( keynum >= K_FIRST_4JOY && keynum <= K_LAST_4JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_4JOY;
	}

	for ( i = 0; i < MAX_JOY_REMAPS; i++ ) {
		if ( device->remap[i].event.type == JOYEVENT_NONE ) {
			if ( freeSlot == -1 ) {
				freeSlot = i;
			}
			continue;
		}

		if ( !CL_JoyEventsMatch( &device->remap[i].event, joyevent ) )
			continue;

		if ( keynum == -1 ) {
			device->modified = qtrue;
			device->remap[i].event.type = JOYEVENT_NONE;
			return qtrue;
		}

		if ( device->remap[i].keynum != keynum ) {
			device->modified = qtrue;
			device->remap[i].keynum = keynum;
		}
		return qtrue;
	}

	// didn't find existing remap to replace, create new
	if ( freeSlot != -1 && keynum != -1 ) {
		device->modified = qtrue;
		device->remap[freeSlot].event = *joyevent;
		device->remap[freeSlot].keynum = keynum;
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
	joyDevice_t *device;

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 )
		return -1;

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];

	for ( i = 0; i < MAX_JOY_REMAPS; i++ ) {
		if ( !CL_JoyEventsMatch( &device->remap[i].event, joyevent ) )
			continue;

		keynum = device->remap[i].keynum;
		break;
	}

	// use correct joystick keys for player
	if ( keynum >= K_FIRST_JOY && keynum <= K_LAST_JOY ) {
		if ( localPlayerNum == 1 ) {
			keynum = K_FIRST_2JOY + keynum - K_FIRST_JOY;
		} else if ( localPlayerNum == 2 ) {
			keynum = K_FIRST_3JOY + keynum - K_FIRST_JOY;
		} else if (  localPlayerNum == 3 ) {
			keynum = K_FIRST_4JOY + keynum - K_FIRST_JOY;
		}
	}

	return keynum;
}

/*
===================
CL_GetJoyEventForKey

Sets joyevent and returns index (for using with startIndex to find multiple joyevents).
===================
*/
int CL_GetJoyEventForKey( int localPlayerNum, int keynum, int startIndex, joyevent_t *joyevent ) {
	int i;
	joyDevice_t *device;

	Com_Memset( joyevent, 0, sizeof ( *joyevent ) );

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 )
		return -1;

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];

	// always look in main joystick enum range
	if ( keynum >= K_FIRST_2JOY && keynum <= K_LAST_2JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_2JOY;
	}
	else if ( keynum >= K_FIRST_3JOY && keynum <= K_LAST_3JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_3JOY;
	}
	else if ( keynum >= K_FIRST_4JOY && keynum <= K_LAST_4JOY ) {
		keynum = K_FIRST_JOY + keynum - K_FIRST_4JOY;
	}

	for ( i = startIndex; i < MAX_JOY_REMAPS; i++ ) {
		if ( device->remap[i].event.type == JOYEVENT_NONE )
			continue;

		if ( keynum == device->remap[i].keynum ) {
			*joyevent = device->remap[i].event;
			return i;
		}
	}

	return -1;
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

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		Com_Printf( "Joystick for player %d not initialized\n", localPlayerNum+1 );
		return;
	}

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
	joyDevice_t *device;

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		Com_Printf( "Joystick for player %d not initialized\n", localPlayerNum+1 );
		return;
	}

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];

	Com_Memset( device->remap, 0, sizeof ( device->remap[0] ) );
}

/*
===================
Cmd_JoyRemap_f
===================
*/
void Cmd_JoyRemap_f (void)
{
	int			c, keynums[KEYNUMS_PER_STRING];
	joyevent_t	joyevent;
	int			localPlayerNum;

	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("%s <event> [key] : attach a joystick event to a virtual key\n", Cmd_Argv(0));
		return;
	}

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		Com_Printf( "Joystick for player %d not initialized\n", localPlayerNum+1 );
		return;
	}

	if (!CL_StringToJoyEvent(Cmd_Argv(1), &joyevent))
	{
		Com_Printf ("\"%s\" isn't a valid joystick event\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		int key = CL_GetKeyForJoyEvent( localPlayerNum, &joyevent );
		if ( key != -1 )
			Com_Printf ("\"%s\" = \"%s\"\n", CL_JoyEventToString( &joyevent ), Key_KeynumToString( key ) );
		else
			Com_Printf ("\"%s\" is not remapped\n", CL_JoyEventToString( &joyevent ) );
		return;
	}

	if ( !Key_StringToKeynum( Cmd_Argv(2), keynums ) )
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(2));
		return;
	}

	// Binding to ALT maps to both LEFTALT and RIGHTALT but joystick only supports one key.
	if ( keynums[1] != -1 )
	{
		Com_Printf( "WARNING: %s only maps to %s for joystick event\n", Cmd_Argv(2), Key_KeynumToString( keynums[0] ) );
	}

	if ( !CL_SetKeyForJoyEvent( localPlayerNum, &joyevent, keynums[0] ) ) {
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
	joyDevice_t *device;

	localPlayerNum = Com_LocalPlayerForCvarName( Cmd_Argv(0) );

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		Com_Printf( "Joystick for player %d not initialized\n", localPlayerNum+1 );
		return;
	}

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];

	if ( strcmp( device->name, device->ident ) ) {
		Com_Printf("Device: %s (%s)\n", device->name, device->ident );
	} else {
		Com_Printf("Device: %s\n", device->name );
	}

	for ( i = 0 ; i < MAX_JOY_REMAPS ; i++ ) {
		if ( device->remap[i].event.type == JOYEVENT_NONE ) {
			continue;
		}

		Com_Printf ("%s %s\n", CL_JoyEventToString( &device->remap[i].event ),
								Key_KeynumToString( device->remap[i].keynum ) );
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

	Com_Memset( &joyDevice, 0, sizeof ( joyDevice ) );

	// register our functions
	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		playerJoyRemapIndex[i] = -1;

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
CL_SetAutoJoyRemap

The joy remap wasn't user created so don't archive it, unless user changes it
=================
*/
void CL_SetAutoJoyRemap( int localPlayerNum ) {
	joyDevice_t *device;

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		return;
	}

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];
	device->modified = qfalse;
}

/*
=================
CL_CloseJoystickRemap

Write out "<event> <key>" lines
=================
*/
void CL_CloseJoystickRemap( int localPlayerNum ) {
	fileHandle_t	f;
	char			filename[MAX_QPATH];
	int				i;
	joyDevice_t *device;

	if ( playerJoyRemapIndex[ localPlayerNum ] == -1 ) {
		return;
	}

	device = &joyDevice[ playerJoyRemapIndex[ localPlayerNum ] ];
	device->references--;

	playerJoyRemapIndex[ localPlayerNum ] = -1;

	if ( !device->modified ) {
		return;
	}
	device->modified = qfalse;

	Com_sprintf( filename, sizeof ( filename ), "joy-%s-%s.txt", JOY_PLATFORM, device->ident );

	f = FS_SV_FOpenFileWrite( filename );
	if ( !f ) {
		Com_Printf ("Couldn't write %s.\n", filename );
		return;
	}

	FS_Printf (f, "// Joystick remap created using " PRODUCT_NAME " on " JOY_PLATFORM " for %s\n", device->name);

	for ( i = 0 ; i < MAX_JOY_REMAPS ; i++ ) {
		if ( device->remap[i].event.type == JOYEVENT_NONE ) {
			continue;
		}

		FS_Printf (f, "%s %s\n", CL_JoyEventToString( &device->remap[i].event ),
								Key_KeynumToString( device->remap[i].keynum ) );
	}

	FS_FCloseFile( f );
}

/*
=================
CL_OpenJoystickRemap

joystickIdent could be a name or hash
=================
*/
qboolean CL_OpenJoystickRemap( int localPlayerNum, const char *joystickName, const char *joystickIdent ) {
	fileHandle_t	f;
	char		filename[MAX_QPATH];
	char		*buffer, *text, *token;
	int			len, i;
	joyevent_t	joyevent;
	int			keynums[KEYNUMS_PER_STRING];

	if ( !joystickName ) {
		return qfalse;
	}

	if ( !joystickIdent ) {
		joystickIdent = joystickName;
	}

	// check if already loaded
	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		if ( !strcmp(joyDevice[i].ident, joystickIdent ) ) {
			break;
		}
	}

	if ( i != CL_MAX_SPLITVIEW ) {
		playerJoyRemapIndex[localPlayerNum] = i;
		joyDevice[i].references++;
		return qtrue;
	}

	// find free slot
	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		if ( !joyDevice[i].references ) {
			break;
		}
	}

	if ( i == CL_MAX_SPLITVIEW ) {
		Com_Printf("BUG: Tried to open joystick but no free slot\n");
		playerJoyRemapIndex[localPlayerNum] = -1;
		return qfalse;
	}

	playerJoyRemapIndex[localPlayerNum] = i;

	// initialize remap
	Com_Memset( &joyDevice[i], 0, sizeof ( joyDevice[0] ) );
	Q_strncpyz( joyDevice[i].ident, joystickIdent, sizeof ( joyDevice[i].ident ) );
	Q_strncpyz( joyDevice[i].name, joystickName, sizeof ( joyDevice[i].ident ) );
	joyDevice[i].references = 1;

	Com_sprintf( filename, sizeof ( filename ), "joy-%s-%s.txt", JOY_PLATFORM, joyDevice[i].ident );
	len = FS_SV_FOpenFileRead( filename, &f );
	if ( !f ) {
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

		if ( !Key_StringToKeynum( token, keynums ) )
		{
			Com_Printf( "\"%s\" isn't a valid key in %s\n", token, filename );
			continue;
		}

		// Binding to ALT maps to both LEFTALT and RIGHTALT but joystick only supports one key.
		if ( keynums[1] != -1 )
		{
			Com_Printf( "WARNING: %s only maps to %s for joystick event\n", token, Key_KeynumToString( keynums[0] ) );
		}

		if ( !CL_SetKeyForJoyEvent( localPlayerNum, &joyevent, keynums[0] ) ) {
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
	if ( !cgvm ) {
		return;
	}

	VM_Call( cgvm, CG_JOYSTICK_AXIS_EVENT, localPlayerNum, axis, value, time, clc.state );
}

/*
=================
CL_JoystickButtonEvent
=================
*/
void CL_JoystickButtonEvent( int localPlayerNum, int button, qboolean down, unsigned time ) {
	if ( !cgvm ) {
		return;
	}

	VM_Call( cgvm, CG_JOYSTICK_BUTTON_EVENT, localPlayerNum, button, down, time, clc.state );
}

/*
=================
CL_JoystickHatEvent
=================
*/
void CL_JoystickHatEvent( int localPlayerNum, int hat, int state, unsigned time ) {
	if ( !cgvm ) {
		return;
	}

	VM_Call( cgvm, CG_JOYSTICK_HAT_EVENT, localPlayerNum, hat, state, time, clc.state );
}


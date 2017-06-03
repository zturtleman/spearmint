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

qboolean	key_overstrikeMode;

int				anykeydown;
qkey_t		keys[MAX_KEYS];


typedef struct {
	char	*name;
	int		keynum;
} keyname_t;


// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
keyname_t keynames[] =
{
	{"TAB", K_TAB},
	{"ENTER", K_ENTER},
	{"ESCAPE", K_ESCAPE},
	{"SPACE", K_SPACE},
	{"BACKSPACE", K_BACKSPACE},
	{"UPARROW", K_UPARROW},
	{"DOWNARROW", K_DOWNARROW},
	{"LEFTARROW", K_LEFTARROW},
	{"RIGHTARROW", K_RIGHTARROW},

	{"ALT", K_ALT},
	{"CTRL", K_CTRL},
	{"SHIFT", K_SHIFT},

	{"COMMAND", K_COMMAND},

	{"CAPSLOCK", K_CAPSLOCK},

	
	{"F1", K_F1},
	{"F2", K_F2},
	{"F3", K_F3},
	{"F4", K_F4},
	{"F5", K_F5},
	{"F6", K_F6},
	{"F7", K_F7},
	{"F8", K_F8},
	{"F9", K_F9},
	{"F10", K_F10},
	{"F11", K_F11},
	{"F12", K_F12},
	{"F13", K_F13},
	{"F14", K_F14},
	{"F15", K_F15},

	{"INS", K_INS},
	{"DEL", K_DEL},
	{"PGDN", K_PGDN},
	{"PGUP", K_PGUP},
	{"HOME", K_HOME},
	{"END", K_END},

	{"MOUSE1", K_MOUSE1},
	{"MOUSE2", K_MOUSE2},
	{"MOUSE3", K_MOUSE3},
	{"MOUSE4", K_MOUSE4},
	{"MOUSE5", K_MOUSE5},

	{"MWHEELUP",	K_MWHEELUP },
	{"MWHEELDOWN",	K_MWHEELDOWN },

	// player 1
	{"JOY_A", K_JOY_A},
	{"JOY_B", K_JOY_B},
	{"JOY_X", K_JOY_X},
	{"JOY_Y", K_JOY_Y},
	{"JOY_BACK", K_JOY_BACK},
	{"JOY_GUIDE", K_JOY_GUIDE},
	{"JOY_START", K_JOY_START},

	{"JOY_DPAD_UP", K_JOY_DPAD_UP},
	{"JOY_DPAD_RIGHT", K_JOY_DPAD_RIGHT},
	{"JOY_DPAD_DOWN", K_JOY_DPAD_DOWN},
	{"JOY_DPAD_LEFT", K_JOY_DPAD_LEFT},

	{"JOY_LEFTSHOULDER", K_JOY_LEFTSHOULDER},
	{"JOY_RIGHTSHOULDER", K_JOY_RIGHTSHOULDER},

	{"JOY_LEFTTRIGGER", K_JOY_LEFTTRIGGER},
	{"JOY_RIGHTTRIGGER", K_JOY_RIGHTTRIGGER},

	{"JOY_LEFTSTICK", K_JOY_LEFTSTICK},
	{"JOY_RIGHTSTICK", K_JOY_RIGHTSTICK},

	{"JOY_LEFTSTICK_UP", K_JOY_LEFTSTICK_UP},
	{"JOY_LEFTSTICK_RIGHT", K_JOY_LEFTSTICK_RIGHT},
	{"JOY_LEFTSTICK_DOWN", K_JOY_LEFTSTICK_DOWN},
	{"JOY_LEFTSTICK_LEFT", K_JOY_LEFTSTICK_LEFT},

	{"JOY_RIGHTSTICK_UP", K_JOY_RIGHTSTICK_UP},
	{"JOY_RIGHTSTICK_RIGHT", K_JOY_RIGHTSTICK_RIGHT},
	{"JOY_RIGHTSTICK_DOWN", K_JOY_RIGHTSTICK_DOWN},
	{"JOY_RIGHTSTICK_LEFT", K_JOY_RIGHTSTICK_LEFT},

	// player 2
	{"2JOY_A", K_2JOY_A},
	{"2JOY_B", K_2JOY_B},
	{"2JOY_X", K_2JOY_X},
	{"2JOY_Y", K_2JOY_Y},
	{"2JOY_BACK", K_2JOY_BACK},
	{"2JOY_GUIDE", K_2JOY_GUIDE},
	{"2JOY_START", K_2JOY_START},

	{"2JOY_DPAD_UP", K_2JOY_DPAD_UP},
	{"2JOY_DPAD_RIGHT", K_2JOY_DPAD_RIGHT},
	{"2JOY_DPAD_DOWN", K_2JOY_DPAD_DOWN},
	{"2JOY_DPAD_LEFT", K_2JOY_DPAD_LEFT},

	{"2JOY_LEFTSHOULDER", K_2JOY_LEFTSHOULDER},
	{"2JOY_RIGHTSHOULDER", K_2JOY_RIGHTSHOULDER},

	{"2JOY_LEFTTRIGGER", K_2JOY_LEFTTRIGGER},
	{"2JOY_RIGHTTRIGGER", K_2JOY_RIGHTTRIGGER},

	{"2JOY_LEFTSTICK", K_2JOY_LEFTSTICK},
	{"2JOY_RIGHTSTICK", K_2JOY_RIGHTSTICK},

	{"2JOY_LEFTSTICK_UP", K_2JOY_LEFTSTICK_UP},
	{"2JOY_LEFTSTICK_RIGHT", K_2JOY_LEFTSTICK_RIGHT},
	{"2JOY_LEFTSTICK_DOWN", K_2JOY_LEFTSTICK_DOWN},
	{"2JOY_LEFTSTICK_LEFT", K_2JOY_LEFTSTICK_LEFT},

	{"2JOY_RIGHTSTICK_UP", K_2JOY_RIGHTSTICK_UP},
	{"2JOY_RIGHTSTICK_RIGHT", K_2JOY_RIGHTSTICK_RIGHT},
	{"2JOY_RIGHTSTICK_DOWN", K_2JOY_RIGHTSTICK_DOWN},
	{"2JOY_RIGHTSTICK_LEFT", K_2JOY_RIGHTSTICK_LEFT},

	// player 3
	{"3JOY_A", K_3JOY_A},
	{"3JOY_B", K_3JOY_B},
	{"3JOY_X", K_3JOY_X},
	{"3JOY_Y", K_3JOY_Y},
	{"3JOY_BACK", K_3JOY_BACK},
	{"3JOY_GUIDE", K_3JOY_GUIDE},
	{"3JOY_START", K_3JOY_START},

	{"3JOY_DPAD_UP", K_3JOY_DPAD_UP},
	{"3JOY_DPAD_RIGHT", K_3JOY_DPAD_RIGHT},
	{"3JOY_DPAD_DOWN", K_3JOY_DPAD_DOWN},
	{"3JOY_DPAD_LEFT", K_3JOY_DPAD_LEFT},

	{"3JOY_LEFTSHOULDER", K_3JOY_LEFTSHOULDER},
	{"3JOY_RIGHTSHOULDER", K_3JOY_RIGHTSHOULDER},

	{"3JOY_LEFTTRIGGER", K_3JOY_LEFTTRIGGER},
	{"3JOY_RIGHTTRIGGER", K_3JOY_RIGHTTRIGGER},

	{"3JOY_LEFTSTICK", K_3JOY_LEFTSTICK},
	{"3JOY_RIGHTSTICK", K_3JOY_RIGHTSTICK},

	{"3JOY_LEFTSTICK_UP", K_3JOY_LEFTSTICK_UP},
	{"3JOY_LEFTSTICK_RIGHT", K_3JOY_LEFTSTICK_RIGHT},
	{"3JOY_LEFTSTICK_DOWN", K_3JOY_LEFTSTICK_DOWN},
	{"3JOY_LEFTSTICK_LEFT", K_3JOY_LEFTSTICK_LEFT},

	{"3JOY_RIGHTSTICK_UP", K_3JOY_RIGHTSTICK_UP},
	{"3JOY_RIGHTSTICK_RIGHT", K_3JOY_RIGHTSTICK_RIGHT},
	{"3JOY_RIGHTSTICK_DOWN", K_3JOY_RIGHTSTICK_DOWN},
	{"3JOY_RIGHTSTICK_LEFT", K_3JOY_RIGHTSTICK_LEFT},

	// player 4
	{"4JOY_A", K_4JOY_A},
	{"4JOY_B", K_4JOY_B},
	{"4JOY_X", K_4JOY_X},
	{"4JOY_Y", K_4JOY_Y},
	{"4JOY_BACK", K_4JOY_BACK},
	{"4JOY_GUIDE", K_4JOY_GUIDE},
	{"4JOY_START", K_4JOY_START},

	{"4JOY_DPAD_UP", K_4JOY_DPAD_UP},
	{"4JOY_DPAD_RIGHT", K_4JOY_DPAD_RIGHT},
	{"4JOY_DPAD_DOWN", K_4JOY_DPAD_DOWN},
	{"4JOY_DPAD_LEFT", K_4JOY_DPAD_LEFT},

	{"4JOY_LEFTSHOULDER", K_4JOY_LEFTSHOULDER},
	{"4JOY_RIGHTSHOULDER", K_4JOY_RIGHTSHOULDER},

	{"4JOY_LEFTTRIGGER", K_4JOY_LEFTTRIGGER},
	{"4JOY_RIGHTTRIGGER", K_4JOY_RIGHTTRIGGER},

	{"4JOY_LEFTSTICK", K_4JOY_LEFTSTICK},
	{"4JOY_RIGHTSTICK", K_4JOY_RIGHTSTICK},

	{"4JOY_LEFTSTICK_UP", K_4JOY_LEFTSTICK_UP},
	{"4JOY_LEFTSTICK_RIGHT", K_4JOY_LEFTSTICK_RIGHT},
	{"4JOY_LEFTSTICK_DOWN", K_4JOY_LEFTSTICK_DOWN},
	{"4JOY_LEFTSTICK_LEFT", K_4JOY_LEFTSTICK_LEFT},

	{"4JOY_RIGHTSTICK_UP", K_4JOY_RIGHTSTICK_UP},
	{"4JOY_RIGHTSTICK_RIGHT", K_4JOY_RIGHTSTICK_RIGHT},
	{"4JOY_RIGHTSTICK_DOWN", K_4JOY_RIGHTSTICK_DOWN},
	{"4JOY_RIGHTSTICK_LEFT", K_4JOY_RIGHTSTICK_LEFT},

	{"AUX1", K_AUX1},
	{"AUX2", K_AUX2},
	{"AUX3", K_AUX3},
	{"AUX4", K_AUX4},
	{"AUX5", K_AUX5},
	{"AUX6", K_AUX6},
	{"AUX7", K_AUX7},
	{"AUX8", K_AUX8},
	{"AUX9", K_AUX9},
	{"AUX10", K_AUX10},
	{"AUX11", K_AUX11},
	{"AUX12", K_AUX12},
	{"AUX13", K_AUX13},
	{"AUX14", K_AUX14},
	{"AUX15", K_AUX15},
	{"AUX16", K_AUX16},

	{"KP_HOME",			K_KP_HOME },
	{"KP_UPARROW",		K_KP_UPARROW },
	{"KP_PGUP",			K_KP_PGUP },
	{"KP_LEFTARROW",	K_KP_LEFTARROW },
	{"KP_5",			K_KP_5 },
	{"KP_RIGHTARROW",	K_KP_RIGHTARROW },
	{"KP_END",			K_KP_END },
	{"KP_DOWNARROW",	K_KP_DOWNARROW },
	{"KP_PGDN",			K_KP_PGDN },
	{"KP_ENTER",		K_KP_ENTER },
	{"KP_INS",			K_KP_INS },
	{"KP_DEL",			K_KP_DEL },
	{"KP_SLASH",		K_KP_SLASH },
	{"KP_MINUS",		K_KP_MINUS },
	{"KP_PLUS",			K_KP_PLUS },
	{"KP_NUMLOCK",		K_KP_NUMLOCK },
	{"KP_STAR",			K_KP_STAR },
	{"KP_EQUALS",		K_KP_EQUALS },

	{"PAUSE", K_PAUSE},
	
	{"SEMICOLON", ';'},	// because a raw semicolon seperates commands

	{"WORLD_0", K_WORLD_0},
	{"WORLD_1", K_WORLD_1},
	{"WORLD_2", K_WORLD_2},
	{"WORLD_3", K_WORLD_3},
	{"WORLD_4", K_WORLD_4},
	{"WORLD_5", K_WORLD_5},
	{"WORLD_6", K_WORLD_6},
	{"WORLD_7", K_WORLD_7},
	{"WORLD_8", K_WORLD_8},
	{"WORLD_9", K_WORLD_9},
	{"WORLD_10", K_WORLD_10},
	{"WORLD_11", K_WORLD_11},
	{"WORLD_12", K_WORLD_12},
	{"WORLD_13", K_WORLD_13},
	{"WORLD_14", K_WORLD_14},
	{"WORLD_15", K_WORLD_15},
	{"WORLD_16", K_WORLD_16},
	{"WORLD_17", K_WORLD_17},
	{"WORLD_18", K_WORLD_18},
	{"WORLD_19", K_WORLD_19},
	{"WORLD_20", K_WORLD_20},
	{"WORLD_21", K_WORLD_21},
	{"WORLD_22", K_WORLD_22},
	{"WORLD_23", K_WORLD_23},
	{"WORLD_24", K_WORLD_24},
	{"WORLD_25", K_WORLD_25},
	{"WORLD_26", K_WORLD_26},
	{"WORLD_27", K_WORLD_27},
	{"WORLD_28", K_WORLD_28},
	{"WORLD_29", K_WORLD_29},
	{"WORLD_30", K_WORLD_30},
	{"WORLD_31", K_WORLD_31},
	{"WORLD_32", K_WORLD_32},
	{"WORLD_33", K_WORLD_33},
	{"WORLD_34", K_WORLD_34},
	{"WORLD_35", K_WORLD_35},
	{"WORLD_36", K_WORLD_36},
	{"WORLD_37", K_WORLD_37},
	{"WORLD_38", K_WORLD_38},
	{"WORLD_39", K_WORLD_39},
	{"WORLD_40", K_WORLD_40},
	{"WORLD_41", K_WORLD_41},
	{"WORLD_42", K_WORLD_42},
	{"WORLD_43", K_WORLD_43},
	{"WORLD_44", K_WORLD_44},
	{"WORLD_45", K_WORLD_45},
	{"WORLD_46", K_WORLD_46},
	{"WORLD_47", K_WORLD_47},
	{"WORLD_48", K_WORLD_48},
	{"WORLD_49", K_WORLD_49},
	{"WORLD_50", K_WORLD_50},
	{"WORLD_51", K_WORLD_51},
	{"WORLD_52", K_WORLD_52},
	{"WORLD_53", K_WORLD_53},
	{"WORLD_54", K_WORLD_54},
	{"WORLD_55", K_WORLD_55},
	{"WORLD_56", K_WORLD_56},
	{"WORLD_57", K_WORLD_57},
	{"WORLD_58", K_WORLD_58},
	{"WORLD_59", K_WORLD_59},
	{"WORLD_60", K_WORLD_60},
	{"WORLD_61", K_WORLD_61},
	{"WORLD_62", K_WORLD_62},
	{"WORLD_63", K_WORLD_63},
	{"WORLD_64", K_WORLD_64},
	{"WORLD_65", K_WORLD_65},
	{"WORLD_66", K_WORLD_66},
	{"WORLD_67", K_WORLD_67},
	{"WORLD_68", K_WORLD_68},
	{"WORLD_69", K_WORLD_69},
	{"WORLD_70", K_WORLD_70},
	{"WORLD_71", K_WORLD_71},
	{"WORLD_72", K_WORLD_72},
	{"WORLD_73", K_WORLD_73},
	{"WORLD_74", K_WORLD_74},
	{"WORLD_75", K_WORLD_75},
	{"WORLD_76", K_WORLD_76},
	{"WORLD_77", K_WORLD_77},
	{"WORLD_78", K_WORLD_78},
	{"WORLD_79", K_WORLD_79},
	{"WORLD_80", K_WORLD_80},
	{"WORLD_81", K_WORLD_81},
	{"WORLD_82", K_WORLD_82},
	{"WORLD_83", K_WORLD_83},
	{"WORLD_84", K_WORLD_84},
	{"WORLD_85", K_WORLD_85},
	{"WORLD_86", K_WORLD_86},
	{"WORLD_87", K_WORLD_87},
	{"WORLD_88", K_WORLD_88},
	{"WORLD_89", K_WORLD_89},
	{"WORLD_90", K_WORLD_90},
	{"WORLD_91", K_WORLD_91},
	{"WORLD_92", K_WORLD_92},
	{"WORLD_93", K_WORLD_93},
	{"WORLD_94", K_WORLD_94},
	{"WORLD_95", K_WORLD_95},

	{"WINDOWS", K_SUPER},
	{"COMPOSE", K_COMPOSE},
	{"MODE", K_MODE},
	{"HELP", K_HELP},
	{"PRINT", K_PRINT},
	{"SYSREQ", K_SYSREQ},
	{"SCROLLOCK", K_SCROLLOCK },
	{"BREAK", K_BREAK},
	{"MENU", K_MENU},
	{"POWER", K_POWER},
	{"EURO", K_EURO},
	{"UNDO", K_UNDO},

	{NULL,0}
};

//============================================================================


qboolean Key_GetOverstrikeMode( void ) {
	return key_overstrikeMode;
}


void Key_SetOverstrikeMode( qboolean state ) {
	key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum ) {
	if ( keynum < 0 || keynum >= MAX_KEYS ) {
		return qfalse;
	}

	return keys[keynum].down;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( char *str ) {
	keyname_t	*kn;
	
	if ( !str || !str[0] ) {
		return -1;
	}
	if ( !str[1] ) {
		return tolower( str[0] );
	}

	// check for hex code
	if ( strlen( str ) == 4 ) {
		int n = Com_HexStrToInt( str );

		if ( n >= 0 ) {
			return n;
		}
	}

	// scan for a text match
	for ( kn=keynames ; kn->name ; kn++ ) {
		if ( !Q_stricmp( str,kn->name ) )
			return kn->keynum;
	}

	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
char *Key_KeynumToString( int keynum ) {
	keyname_t	*kn;	
	static	char	tinystr[5];
	int			i, j;

	if ( keynum == -1 ) {
		return "<KEY NOT FOUND>";
	}

	if ( keynum < 0 || keynum >= MAX_KEYS ) {
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if ( keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';' ) {
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	// check for a key string
	for ( kn=keynames ; kn->name ; kn++ ) {
		if (keynum == kn->keynum) {
			return kn->name;
		}
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( int keynum, const char *binding ) {
	if ( keynum < 0 || keynum >= MAX_KEYS ) {
		return;
	}

	// free old bindings
	if ( keys[ keynum ].binding ) {
		Z_Free( keys[ keynum ].binding );
	}
		
	// allocate memory for new binding
	keys[keynum].binding = CopyString( binding );

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}


/*
===================
Key_GetBinding
===================
*/
char *Key_GetBinding( int keynum ) {
	if ( keynum < 0 || keynum >= MAX_KEYS ) {
		return "";
	}

	return keys[ keynum ].binding;
}

/* 
===================
Key_GetKey
===================
*/

int Key_GetKey(const char *binding, int startKey) {
  int i;

  if (binding) {
  	for (i=startKey ; i < MAX_KEYS ; i++) {
      if (keys[i].binding && Q_stricmp(binding, keys[i].binding) == 0) {
        return i;
      }
    }
  }
  return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
	int		b;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("unbind <key> : remove commands from a key\n");
		return;
	}
	
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	Key_SetBinding (b, "");
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f (void)
{
	int		i;
	
	for (i=0 ; i < MAX_KEYS; i++)
		if (keys[i].binding)
			Key_SetBinding (i, "");
}


/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
	int			i, c, b;
	char		cmd[1024];
	
	c = Cmd_Argc();

	if (c < 2)
	{
		Com_Printf ("bind <key> [command] : attach a command to a key\n");
		return;
	}
	b = Key_StringToKeynum (Cmd_Argv(1));
	if (b==-1)
	{
		Com_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		if (keys[b].binding && keys[b].binding[0])
			Com_Printf ("\"%s\" = \"%s\"\n", Key_KeynumToString(b), keys[b].binding );
		else
			Com_Printf ("\"%s\" is not bound\n", Key_KeynumToString(b) );
		return;
	}
	
// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c-1))
			strcat (cmd, " ");
	}

	Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f ) {
	int		i;

	FS_Printf (f, "unbindall\n" );

	for (i=0 ; i<MAX_KEYS ; i++) {
		if (keys[i].binding && keys[i].binding[0] ) {
			FS_Printf (f, "bind %s \"%s\"\n", Key_KeynumToString(i), keys[i].binding);

		}

	}
}


/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void ) {
	int		i;

	for ( i = 0 ; i < MAX_KEYS ; i++ ) {
		if ( keys[i].binding && keys[i].binding[0] ) {
			Com_Printf( "%s \"%s\"\n", Key_KeynumToString(i), keys[i].binding );
		}
	}
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( void(*callback)(const char *s) ) {
	int		i;

	for( i = 0; keynames[ i ].name != NULL; i++ )
		callback( keynames[ i ].name );
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( char *args, int argNum )
{
	if( argNum == 2 )
	{
		// Skip "unbind "
		char *p = Com_SkipTokens( args, 1, " " );

		if( p > args )
			Field_CompleteKeyname( );
	}
}

/*
====================
Key_CompleteBind
====================
*/
static void Key_CompleteBind( char *args, int argNum )
{
	char *p;

	if( argNum == 2 )
	{
		// Skip "bind "
		p = Com_SkipTokens( args, 1, " " );

		if( p > args )
			Field_CompleteKeyname( );
	}
	else if( argNum >= 3 )
	{
		// Skip "bind <key> "
		p = Com_SkipTokens( args, 2, " " );

		if( p > args )
			Field_CompleteCommand( p, qtrue, qtrue );
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands( void ) {
	// register our functions
	Cmd_AddCommand ("bind",Key_Bind_f);
	Cmd_SetCommandCompletionFunc( "bind", Key_CompleteBind );
	Cmd_AddCommand ("unbind",Key_Unbind_f);
	Cmd_SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
	Cmd_AddCommand ("unbindall",Key_Unbindall_f);
	Cmd_AddCommand ("bindlist",Key_Bindlist_f);
}

/*
===================
CL_KeyDownEvent

Called by CL_KeyEvent to handle a keypress
===================
*/
void CL_KeyDownEvent( int key, unsigned time )
{
	keys[key].down = qtrue;
	keys[key].repeats++;
	if( keys[key].repeats == 1 )
		anykeydown++;

	if( keys[K_ALT].down && key == K_ENTER )
	{
		// don't repeat fullscreen toggle when keys are held down
		if ( keys[K_ENTER].repeats > 1 ) {
			return;
		}

		Cvar_SetValue( "r_fullscreen",
			!Cvar_VariableIntegerValue( "r_fullscreen" ) );
		return;
	}

	if ( cgvm ) {
		VM_Call( cgvm, CG_KEY_EVENT, key, qtrue, time, clc.state );
	}
}

/*
===================
CL_KeyUpEvent

Called by CL_KeyEvent to handle a keyrelease
===================
*/
void CL_KeyUpEvent( int key, unsigned time )
{
	keys[key].repeats = 0;
	keys[key].down = qfalse;
	anykeydown--;

	if (anykeydown < 0) {
		anykeydown = 0;
	}

	if ( cgvm ) {
		VM_Call( cgvm, CG_KEY_EVENT, key, qfalse, time, clc.state );
	}
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent (int key, qboolean down, unsigned time) {
	if( down )
		CL_KeyDownEvent( key, time );
	else
		CL_KeyUpEvent( key, time );
}

/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int character ) {
	// delete is not a printable character and is
	// otherwise handled by Field_KeyDownEvent
	if ( character == 127 ) {
		return;
	}

	if ( cgvm )
	{
		VM_Call( cgvm, CG_CHAR_EVENT, character, clc.state );
	}
}


/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
	int		i;

	anykeydown = 0;

	for ( i=0 ; i < MAX_KEYS ; i++ ) {
		if ( keys[i].down ) {
			CL_KeyEvent( i, qfalse, 0 );

		}
		keys[i].down = 0;
		keys[i].repeats = 0;
	}
}

/*
====================
Key_KeynumToStringBuf
====================
*/
void Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	Q_strncpyz( buf, Key_KeynumToString( keynum ), buflen );
}

/*
====================
Key_GetBindingBuf
====================
*/
void Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	char	*value;

	value = Key_GetBinding( keynum );
	if ( value ) {
		Q_strncpyz( buf, value, buflen );
	}
	else {
		*buf = 0;
	}
}

static qboolean keyRepeat = qfalse;

/*
====================
Key_GetRepeat
====================
*/
qboolean Key_GetRepeat( void ) {
	return keyRepeat;
}

/*
====================
Key_SetRepeat
====================
*/
void Key_SetRepeat( qboolean repeat ) {
	keyRepeat = repeat;
}


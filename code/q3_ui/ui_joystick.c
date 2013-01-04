/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010-2011 by Zack Middleton

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
/*
=======================================================================

JOYSTICK SELECT MENU

=======================================================================
*/

#include "ui_local.h"

#define MAX_JOYSTICKS 16

#define JOY_MENU_VERTICAL_SPACING		16

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_JOYSTICK		10
#define ID_BACK			ID_JOYSTICK + MAX_JOYSTICKS


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		joysticks[MAX_JOYSTICKS];

	menubitmap_s	back;

	char			joystickNames[ MAX_JOYSTICKS ][ 61 ];
	int				numJoysticks;

	int				originalJoystick;
	qboolean		changed;

	int				localClient;
} joystickMenu_t;

static joystickMenu_t	joystickMenu;


/*
===============
UI_JoystickMenu_Event
===============
*/
static void UI_JoystickMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	if (((menucommon_s*)ptr)->id >= ID_JOYSTICK && ((menucommon_s*)ptr)->id < ID_JOYSTICK + MAX_JOYSTICKS) {
		int joystick = ((menucommon_s*)ptr)->id - ID_JOYSTICK;
		int i;

		if (joystick == 0) {
			// Disable joystick
			trap_Cvar_SetValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystick"), 0);
			trap_Cvar_SetValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystickNo"), 0);
		} else {
			// Enable joystick
			trap_Cvar_SetValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystick"), 1);
			trap_Cvar_SetValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystickNo"), joystick-1);
		}

		// Disable last selected joystick.
		for (i = 0; i < MAX_JOYSTICKS; i++) {
			joystickMenu.joysticks[i].generic.flags |= QMF_PULSEIFFOCUS;
			joystickMenu.joysticks[i].generic.flags &= ~(QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		}

		// Select joystick.
		((menucommon_s*)ptr)->flags &= ~QMF_PULSEIFFOCUS;
		((menucommon_s*)ptr)->flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);

		joystickMenu.changed = (joystick != joystickMenu.originalJoystick);
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {

	case ID_BACK:
		if (joystickMenu.changed) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
		}

		UI_PopMenu();
		break;
	}
}

/*
============
UI_TokenizeDelimitedString
============
*/
static void UI_TokenizeDelimitedString( const char *text_in, char *cmd_tokenized, int *cmd_argc, char **cmd_argv, char delimiter)
{
	const char	*text;
	char	*textOut;

	// clear previous args
	*cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	text = text_in;
	textOut = cmd_tokenized;

	while ( 1 ) {
		if ( *cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		// skip whitespace
		while ( *text && *text <= ' ' ) {
			text++;
		}
		if ( !*text ) {
			return;			// all tokens parsed
		}

		// regular token
		cmd_argv[*cmd_argc] = textOut;
		(*cmd_argc)++;

		// skip until newline or null.
		while ( *text && *text != delimiter) {
			*textOut++ = *text++;
		}

		*textOut++ = 0;
	}
}

/*
===============
UI_Joystick_GetNames
===============
*/
static void UI_Joystick_GetNames( void ) {
	char	joybuf[ MAX_STRING_CHARS ];
	int		i;
	char	*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
	char	cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted

	// Get newline delimited string of joystick names.
	Q_strncpyz(joybuf, UI_Cvar_VariableString("in_availableJoysticks"), sizeof(joybuf));

	// Option to disable joystick
	Q_strncpyz(joystickMenu.joystickNames[0], "None", sizeof (joystickMenu.joystickNames[0]));

	if (*joybuf) {
		// Seperate joystick names
		UI_TokenizeDelimitedString( joybuf, cmd_tokenized, &joystickMenu.numJoysticks, cmd_argv, '\n' );

		// Joysticks plus disable option
		joystickMenu.numJoysticks += 1;

		// Get names of joysticks
		for (i = 1; i < joystickMenu.numJoysticks; i++) {
			Q_strncpyz(joystickMenu.joystickNames[i], cmd_argv[i-1], sizeof (joystickMenu.joystickNames[i]));
		}
	} else {
		joystickMenu.numJoysticks = i = 1;
	}

	// Set default text for empty slots.
	for ( ; i < MAX_JOYSTICKS; i++) {
		Q_strncpyz(joystickMenu.joystickNames[i], "Unavailable", sizeof (joystickMenu.joystickNames[i]));
	}
}

/*
===============
UI_Joystick_MenuInit
===============
*/
static void UI_Joystick_MenuInit( int localClient ) {
	int				y;
	int				i;
	int				joystick;

	UI_Joystick_Cache();

	memset( &joystickMenu, 0, sizeof(joystickMenu) );

	UI_Joystick_GetNames();

	joystickMenu.localClient = localClient;

	joystickMenu.menu.wrapAround = qtrue;
	joystickMenu.menu.fullscreen = qtrue;

	joystickMenu.banner.generic.type			= MTYPE_BTEXT;
	joystickMenu.banner.generic.x				= 320;
	joystickMenu.banner.generic.y				= 16;
	joystickMenu.banner.string					= "SELECT JOYSTICK";
	joystickMenu.banner.color					= color_white;
	joystickMenu.banner.style					= UI_CENTER;

	joystickMenu.framel.generic.type			= MTYPE_BITMAP;
	joystickMenu.framel.generic.name			= ART_FRAMEL;
	joystickMenu.framel.generic.flags			= QMF_INACTIVE;
	joystickMenu.framel.generic.x				= 0;
	joystickMenu.framel.generic.y				= 78;
	joystickMenu.framel.width  					= 256;
	joystickMenu.framel.height  				= 329;

	joystickMenu.framer.generic.type			= MTYPE_BITMAP;
	joystickMenu.framer.generic.name			= ART_FRAMER;
	joystickMenu.framer.generic.flags			= QMF_INACTIVE;
	joystickMenu.framer.generic.x				= 376;
	joystickMenu.framer.generic.y				= 76;
	joystickMenu.framer.width  					= 256;
	joystickMenu.framer.height  				= 334;

	y = (SCREEN_HEIGHT - MAX_JOYSTICKS * JOY_MENU_VERTICAL_SPACING) / 2;

	for (i = 0; i < MAX_JOYSTICKS; i++) {
		joystickMenu.joysticks[i].generic.type		= MTYPE_PTEXT;
		joystickMenu.joysticks[i].generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		joystickMenu.joysticks[i].generic.x			= 320;
		joystickMenu.joysticks[i].generic.y			= y;
		joystickMenu.joysticks[i].generic.id		= ID_JOYSTICK+i;
		joystickMenu.joysticks[i].generic.callback	= UI_JoystickMenu_Event;
		joystickMenu.joysticks[i].string			= joystickMenu.joystickNames[i];
		joystickMenu.joysticks[i].color				= color_red;
		joystickMenu.joysticks[i].style				= UI_CENTER|UI_SMALLFONT;

		if (i >= joystickMenu.numJoysticks) {
			joystickMenu.joysticks[i].generic.flags |= QMF_GRAYED;
		}

		y += JOY_MENU_VERTICAL_SPACING;
	}

	joystickMenu.back.generic.type				= MTYPE_BITMAP;
	joystickMenu.back.generic.name				= ART_BACK0;
	joystickMenu.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	joystickMenu.back.generic.id				= ID_BACK;
	joystickMenu.back.generic.callback			= UI_JoystickMenu_Event;
	joystickMenu.back.generic.x					= 0;
	joystickMenu.back.generic.y					= 480-64;
	joystickMenu.back.width						= 128;
	joystickMenu.back.height					= 64;
	joystickMenu.back.focuspic					= ART_BACK1;

	Menu_AddItem( &joystickMenu.menu, &joystickMenu.banner );
	Menu_AddItem( &joystickMenu.menu, &joystickMenu.framel );
	Menu_AddItem( &joystickMenu.menu, &joystickMenu.framer );

	for (i = 0; i < MAX_JOYSTICKS; i++) {
		Menu_AddItem( &joystickMenu.menu, &joystickMenu.joysticks[i] );
	}

	Menu_AddItem( &joystickMenu.menu, &joystickMenu.back );

	// Store original joystick
	if (trap_Cvar_VariableValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystick")) == 0) {
		joystick = 0;
	} else {
		joystick = 1 + (int)trap_Cvar_VariableValue(Com_LocalClientCvarName(joystickMenu.localClient, "in_joystickNo"));
	}

	if (joystick < 0 || joystick >= joystickMenu.numJoysticks) {
		joystick = 0;
	}

	joystickMenu.originalJoystick = joystick;

	// Select joystick
	joystickMenu.joysticks[joystick].generic.flags &= ~QMF_PULSEIFFOCUS;
	joystickMenu.joysticks[joystick].generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
}

/*
=================
UI_Joystick_Cache
=================
*/
void UI_Joystick_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}

/*
===============
UI_JoystickMenu
===============
*/
void UI_JoystickMenu( int localClient ) {
	UI_Joystick_MenuInit(localClient);
	UI_PushMenu( &joystickMenu.menu );
}


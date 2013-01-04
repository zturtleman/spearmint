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

INGAME LOCAL CLIENT SELECT MENU

This is a general select local client menu. Used for accessing menus for a specific local client.
It runs a function, passing the selected client to the function.
Automaticly disables players not in the game, so the function can expect that the client is ingame.
If there is only one local client simply runs the function.

=======================================================================
*/


#include "ui_local.h"

//#define INGAME_FRAME					"menu/art/addbotframe"
#define INGAME_FRAME					"menu/art/cut_frame"
#define INGAME_MENU_VERTICAL_SPACING	28

#define ID_BACK					10
#define ID_CUSTOMIZEPLAYER		11 // + MAX_SPLITVIEW



typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		player[MAX_SPLITVIEW];

	menutext_s		back;

	char			bannerString[32];
	char			playerString[MAX_SPLITVIEW][12];
	void 			(*playerfunc)(int);
} setupplayersmenu_t;

static setupplayersmenu_t	s_setupplayers;


/*
=================
InSelectPlayer_Event
=================
*/
void InSelectPlayer_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	if (((menucommon_s*)ptr)->id >= ID_CUSTOMIZEPLAYER && ((menucommon_s*)ptr)->id < ID_CUSTOMIZEPLAYER+MAX_SPLITVIEW) {
		s_setupplayers.playerfunc( ((menucommon_s*)ptr)->id - ID_CUSTOMIZEPLAYER );
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
InSelectPlayerMenu_Draw
=================
*/
static void InSelectPlayerMenu_Draw( void ) {
	UI_DrawBannerString( 320, 16, s_setupplayers.bannerString, UI_CENTER, color_white );

	// standard menu drawing
	Menu_Draw( &s_setupplayers.menu );
}


/*
=================
InSelectPlayer_MenuInit
=================
*/
void InSelectPlayer_MenuInit( uiClientState_t *cs, const char *banner, qboolean disableMissingPlayers ) {
	int		y;
	int		i;

	memset( &s_setupplayers, 0, sizeof(setupplayersmenu_t) );

	InSelectPlayer_Cache();

	Q_strncpyz(s_setupplayers.bannerString, banner, sizeof (s_setupplayers.bannerString));
	s_setupplayers.menu.draw = InSelectPlayerMenu_Draw;
	s_setupplayers.menu.wrapAround = qtrue;
	s_setupplayers.menu.fullscreen = qfalse;

	s_setupplayers.frame.generic.type		= MTYPE_BITMAP;
	s_setupplayers.frame.generic.flags		= QMF_INACTIVE;
	s_setupplayers.frame.generic.name		= INGAME_FRAME;
	s_setupplayers.frame.generic.x			= 320-233;//142;
	s_setupplayers.frame.generic.y			= 240-166;//118;
	s_setupplayers.frame.width				= 466;//359;
	s_setupplayers.frame.height				= 332;//256;

	y = (SCREEN_HEIGHT - (1+UI_MaxSplitView())*INGAME_MENU_VERTICAL_SPACING) / 2;

	for (i = 0; i < UI_MaxSplitView(); i++) {
		Com_sprintf(s_setupplayers.playerString[i], sizeof (s_setupplayers.playerString[i]), "Player %d", i+1);

		s_setupplayers.player[i].generic.type		= MTYPE_PTEXT;
		s_setupplayers.player[i].generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_setupplayers.player[i].generic.x			= 320;
		s_setupplayers.player[i].generic.y			= y;
		s_setupplayers.player[i].generic.id			= ID_CUSTOMIZEPLAYER + i;
		s_setupplayers.player[i].generic.callback	= InSelectPlayer_Event;
		s_setupplayers.player[i].string				= s_setupplayers.playerString[i];
		if (!disableMissingPlayers) {
			// Have players in game be red and not ingame be white.
			if (cs->clientNums[i] == -1) {
				s_setupplayers.player[i].color		= color_white;
			} else {
				s_setupplayers.player[i].color		= color_red;
			}
		} else {
			s_setupplayers.player[i].color			= color_red;
		}
		s_setupplayers.player[i].style				= UI_CENTER|UI_SMALLFONT;

		if (disableMissingPlayers && cs->clientNums[i] == -1) {
			s_setupplayers.player[i].generic.flags |= QMF_GRAYED;
		}

		y += INGAME_MENU_VERTICAL_SPACING;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_setupplayers.back.generic.type		= MTYPE_PTEXT;
	s_setupplayers.back.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_setupplayers.back.generic.x			= 320;
	s_setupplayers.back.generic.y			= y;
	s_setupplayers.back.generic.id			= ID_BACK;
	s_setupplayers.back.generic.callback	= InSelectPlayer_Event;
	s_setupplayers.back.string				= "Back";
	s_setupplayers.back.color				= color_red;
	s_setupplayers.back.style				= UI_CENTER|UI_SMALLFONT;

	Menu_AddItem( &s_setupplayers.menu, &s_setupplayers.frame );

	for (i = 0; i < UI_MaxSplitView(); i++) {
		Menu_AddItem( &s_setupplayers.menu, &s_setupplayers.player[i] );
	}

	Menu_AddItem( &s_setupplayers.menu, &s_setupplayers.back );
}


/*
=================
InSelectPlayer_Cache
=================
*/
void InSelectPlayer_Cache( void ) {
	trap_R_RegisterShaderNoMip( INGAME_FRAME );
}

/*
=================
InSelectPlayerMenu
=================
*/
void InSelectPlayerMenu( void (*playerfunc)(int), const char *banner, qboolean disableMissingPlayers ) {
	uiClientState_t	cs;

	trap_GetClientState( &cs );

	// If there is only one local client skip this menu.
	if (UI_NumLocalClients(&cs) <= 1 && disableMissingPlayers) {
		playerfunc(0);
		return;
	}

	InSelectPlayer_MenuInit( &cs, banner, disableMissingPlayers );
	s_setupplayers.playerfunc = playerfunc;
	UI_PushMenu( &s_setupplayers.menu );
}


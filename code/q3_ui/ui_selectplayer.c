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

LOCAL CLIENT SELECT MENU

This is a general select local client menu. Used for accessing menus for a specific local client.
It runs a function, passing the selected client to the function.

=======================================================================
*/

#include "ui_local.h"

#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_BACK					10
#define ID_CUSTOMIZECONTROLS	11 // + MAX_SPLITVIEW


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		player[MAX_SPLITVIEW];

	menubitmap_s	back;

	char			bannerString[32];
	char			playerString[MAX_SPLITVIEW][12];
	void 			(*playerfunc)(int);
} selectPlayerMenu_t;

static selectPlayerMenu_t	selectPlayerMenu;


/*
===============
UI_SelectPlayerMenu_Event
===============
*/
static void UI_SelectPlayerMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	if (((menucommon_s*)ptr)->id >= ID_CUSTOMIZECONTROLS && ((menucommon_s*)ptr)->id < ID_CUSTOMIZECONTROLS+MAX_SPLITVIEW) {
		selectPlayerMenu.playerfunc( ((menucommon_s*)ptr)->id - ID_CUSTOMIZECONTROLS );
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SelectPlayer_MenuInit
===============
*/
static void UI_SelectPlayer_MenuInit( const char *banner ) {
	int		i, y;

	UI_SelectPlayer_Cache();

	memset( &selectPlayerMenu, 0, sizeof(selectPlayerMenu) );
	selectPlayerMenu.menu.wrapAround = qtrue;
	selectPlayerMenu.menu.fullscreen = qtrue;

	Q_strncpyz(selectPlayerMenu.bannerString, banner, sizeof (selectPlayerMenu.bannerString));

	selectPlayerMenu.banner.generic.type			= MTYPE_BTEXT;
	selectPlayerMenu.banner.generic.x				= 320;
	selectPlayerMenu.banner.generic.y				= 16;
	selectPlayerMenu.banner.string					= selectPlayerMenu.bannerString;
	selectPlayerMenu.banner.color					= color_white;
	selectPlayerMenu.banner.style					= UI_CENTER;

	selectPlayerMenu.framel.generic.type			= MTYPE_BITMAP;
	selectPlayerMenu.framel.generic.name			= ART_FRAMEL;
	selectPlayerMenu.framel.generic.flags			= QMF_INACTIVE;
	selectPlayerMenu.framel.generic.x				= 0;
	selectPlayerMenu.framel.generic.y				= 78;
	selectPlayerMenu.framel.width  					= 256;
	selectPlayerMenu.framel.height  				= 329;

	selectPlayerMenu.framer.generic.type			= MTYPE_BITMAP;
	selectPlayerMenu.framer.generic.name			= ART_FRAMER;
	selectPlayerMenu.framer.generic.flags			= QMF_INACTIVE;
	selectPlayerMenu.framer.generic.x				= 376;
	selectPlayerMenu.framer.generic.y				= 76;
	selectPlayerMenu.framer.width  					= 256;
	selectPlayerMenu.framer.height  				= 334;

	y = (SCREEN_HEIGHT - UI_MaxSplitView()*SETUP_MENU_VERTICAL_SPACING) / 2;

	for (i = 0; i < UI_MaxSplitView(); i++) {
		Com_sprintf(selectPlayerMenu.playerString[i], sizeof (selectPlayerMenu.playerString[i]), "Player %d", i+1);

		selectPlayerMenu.player[i].generic.type			= MTYPE_PTEXT;
		selectPlayerMenu.player[i].generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		selectPlayerMenu.player[i].generic.x			= 320;
		selectPlayerMenu.player[i].generic.y			= y;
		selectPlayerMenu.player[i].generic.id			= ID_CUSTOMIZECONTROLS + i;
		selectPlayerMenu.player[i].generic.callback		= UI_SelectPlayerMenu_Event;
		selectPlayerMenu.player[i].string				= selectPlayerMenu.playerString[i];
		selectPlayerMenu.player[i].color				= color_red;
		selectPlayerMenu.player[i].style				= UI_CENTER;

		y += SETUP_MENU_VERTICAL_SPACING;
	}

	selectPlayerMenu.back.generic.type				= MTYPE_BITMAP;
	selectPlayerMenu.back.generic.name				= ART_BACK0;
	selectPlayerMenu.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	selectPlayerMenu.back.generic.id				= ID_BACK;
	selectPlayerMenu.back.generic.callback			= UI_SelectPlayerMenu_Event;
	selectPlayerMenu.back.generic.x					= 0;
	selectPlayerMenu.back.generic.y					= 480-64;
	selectPlayerMenu.back.width						= 128;
	selectPlayerMenu.back.height					= 64;
	selectPlayerMenu.back.focuspic					= ART_BACK1;

	Menu_AddItem( &selectPlayerMenu.menu, &selectPlayerMenu.banner );
	Menu_AddItem( &selectPlayerMenu.menu, &selectPlayerMenu.framel );
	Menu_AddItem( &selectPlayerMenu.menu, &selectPlayerMenu.framer );

	for (i = 0; i < UI_MaxSplitView(); i++) {
		Menu_AddItem( &selectPlayerMenu.menu, &selectPlayerMenu.player[i] );
	}

	Menu_AddItem( &selectPlayerMenu.menu, &selectPlayerMenu.back );
}

/*
=================
UI_SelectPlayer_Cache
=================
*/
void UI_SelectPlayer_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}

/*
===============
UI_SelectPlayerMenu
===============
*/
void UI_SelectPlayerMenu( void (*playerfunc)(int), const char *banner )
{
	if (UI_MaxSplitView() == 1) {
		playerfunc(0);
		return;
	}

	UI_SelectPlayer_MenuInit(banner);
	selectPlayerMenu.playerfunc = playerfunc;
	UI_PushMenu( &selectPlayerMenu.menu );
}

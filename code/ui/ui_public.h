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

#include "../cgame/cg_public.h"

// UI functions used by cgame

void	UI_Init( qboolean inGameLoad, int maxSplitView );
void	UI_Shutdown( void );

void	UI_KeyEvent( int key, qboolean down );
void	UI_MouseEvent( int localClientNum, int dx, int dy );
int		UI_MousePosition( int localClientNum );
void	UI_SetMousePosition( int localClientNum, int x, int y );

qboolean UI_IsFullscreen( void );

void	UI_Refresh( int time );
void	UI_SetActiveMenu( uiMenuCommand_t menu );
qboolean UI_ConsoleCommand( int realTime );

void	UI_DrawConnectScreen( qboolean overlay );
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings

qboolean UI_WantsBindKeys( void );

// used by cg_info.c
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );

#endif

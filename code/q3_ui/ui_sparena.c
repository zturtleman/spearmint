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
#include "ui_local.h"

void UI_SPArena_Start( const char *arenaInfo ) {
	char	*map;
	int		level;
	int		n;
	char	*txt;

	n = (int)trap_Cvar_VariableValue( "sv_maxclients" );
	if ( n < 8 ) {
		trap_Cvar_SetValue( "sv_maxclients", 8 );
	}

	level = atoi( Info_ValueForKey( arenaInfo, "num" ) );
	txt = Info_ValueForKey( arenaInfo, "special" );
	if( txt[0] ) {
		if( Q_stricmp( txt, "training" ) == 0 ) {
			level = -4;
		}
		else if( Q_stricmp( txt, "final" ) == 0 ) {
			level = UI_GetNumSPTiers() * ARENAS_PER_TIER;
		}
	}
	trap_Cvar_SetValue( "ui_spSelection", level );

	trap_Cvar_SetValue( "ui_singlePlayerActive", 1 );
	trap_Cvar_SetValue( "g_gametype", GT_SINGLE_PLAYER );
	trap_Cvar_SetValue( "g_doWarmup", 0 );

	map = Info_ValueForKey( arenaInfo, "map" );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "map %s\n", map ) );
}

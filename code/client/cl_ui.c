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

#include "../botlib/botlib.h"

extern	botlib_export_t	*botlib_export;

vm_t *uivm;

// cl_cgame.c
intptr_t CL_CgameSystemCalls( intptr_t *args );

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_UI );
	cls.uiStarted = qfalse;
	if ( !uivm ) {
		return;
	}
	VM_Call( uivm, UI_SHUTDOWN );
	VM_Free( uivm );
	uivm = NULL;
}

/*
====================
CL_InitUI
====================
*/
void CL_InitUI( void ) {
	unsigned int		version;
	unsigned int		major;
	unsigned int		minor;

	// load the dll or bytecode
	uivm = VM_Create( "ui", CL_CgameSystemCalls, Cvar_VariableValue( "vm_ui" ) );
	if ( !uivm ) {
		Com_Error( ERR_FATAL, "VM_Create on UI failed" );
	}

	// sanity check
	version = VM_SafeCall( uivm, UI_GETAPIVERSION );
	major = (version >> 16) & 0xFFFF;
	minor = version & 0xFFFF;
	Com_DPrintf("Loading UI with version %x.%x\n", major, minor);
	if (major != UI_API_MAJOR_VERSION || minor > UI_API_MINOR_VERSION) {
		// Free uivm now, so UI_SHUTDOWN doesn't get called later.
		VM_Free( uivm );
		uivm = NULL;

		Com_Error( ERR_DROP, "User Interface is version %x.%x, expected %x.%x", major, minor, UI_API_MAJOR_VERSION, UI_API_MINOR_VERSION );
		cls.uiStarted = qfalse;
	}
	else {
		// init for this gamestate
		VM_Call( uivm, UI_INIT, (clc.state >= CA_CONNECTING && clc.state < CA_ACTIVE), CL_MAX_SPLITVIEW );
	}
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand( void ) {
	if ( !uivm ) {
		return qfalse;
	}

	return VM_Call( uivm, UI_CONSOLE_COMMAND, cls.realtime );
}

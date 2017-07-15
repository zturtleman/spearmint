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

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

cvar_t *cl_shownet;

void CL_Shutdown(char *finalmsg, qboolean disconnect, qboolean quit)
{
}

void CL_Init( void ) {
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
}

void CL_MouseEvent( int localPlayerNum, int dx, int dy, int time ) {
}

void Key_WriteBindings( fileHandle_t f ) {
}

void CL_Frame ( int msec ) {
}

void CL_PacketEvent( netadr_t from, msg_t *msg ) {
}

void CL_CharEvent( int character ) {
}

void CL_Disconnect( qboolean showMainMenu ) {
}

void CL_MapLoading( void ) {
}

void CL_KeyEvent (int key, qboolean down, unsigned time) {
}

void CL_ConsolePrint( char *txt ) {
}

void CL_JoystickAxisEvent( int localPlayerNum, int axis, int value, unsigned time ) {

}

void CL_JoystickButtonEvent( int localPlayerNum, int button, qboolean down, unsigned time ) {

}

void CL_JoystickHatEvent( int localPlayerNum, int hat, int state, unsigned time ) {

}

void Key_Dummy_f( void ) {
}

void CL_InitKeyCommands( void ) {
	// stop server from printing unknown command bind when executing default.cfg
	Cmd_AddCommand ("bind",Key_Dummy_f);
	Cmd_AddCommand ("unbindall",Key_Dummy_f);
}

void CL_InitJoyRemapCommands( void ) {

}

void CL_FlushMemory(void)
{
}

void CL_ShutdownAll(qboolean shutdownRef)
{
}

void CL_StartHunkUsers( qboolean rendererOnly ) {
}

qboolean CL_ConnectedToRemoteServer( void ) {
	return qfalse;
}

void CL_MissingDefaultCfg( const char *gamedir ) {

}


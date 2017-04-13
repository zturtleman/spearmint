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
// console.c

#include "client.h"

/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	fileHandle_t	f;
	unsigned int	size;
	char	buffer[1024];
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	if (!COM_CompareExtension(filename, ".txt"))
	{
		Com_Printf("Con_Dump_f: Only the \".txt\" extension is supported by this command!\n");
		return;
	}

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", filename );

	CON_LogSaveReadPos();

	// ZTM: TODO: convert "\n" to "\r\n" if _WIN32 is defined.
	while ( ( size = CON_LogRead( buffer, sizeof ( buffer ) ) ) > 0 )
	{
		FS_Write( buffer, size, f );
	}

	CON_LogRestoreReadPos();

	FS_FCloseFile( f );
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	Cmd_TokenizeString( NULL );
	CL_GameConsoleText( qfalse );
}

/*
==================
Cmd_CompleteTxtName
==================
*/
void Cmd_CompleteTxtName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "", "txt", qfalse, qtrue );
	}
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	Cmd_AddCommand ("condump", Con_Dump_f);
	Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );
}

/*
================
Con_Shutdown
================
*/
void Con_Shutdown(void)
{
	Cmd_RemoveCommand("condump");
}

/*
================
CL_ConsolePrint
================
*/
void CL_ConsolePrint( char *txt ) {

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	if ( cgvm && cls.printToCgame ) {
		Cmd_SaveCmdContext( );

		// feed the text to cgame
		Cmd_TokenizeString( txt );
		CL_GameConsoleText( qfalse );

		Cmd_RestoreCmdContext( );
	}
}

/*
================
Con_Close
================
*/
void Con_Close( void ) {
	if ( !cgvm ) {
		return;
	}

	VM_Call( cgvm, CG_CONSOLE_CLOSE );
}


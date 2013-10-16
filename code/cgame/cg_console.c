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
#include "cg_local.h"

#define		CON_MAXLINES	512
#define		CON_LINELENGTH	128
typedef struct {
	qboolean	initialized;

	char		lines[CON_MAXLINES][CON_LINELENGTH];

	int		current;		// line where next message will be printed
	int		display;		// bottom of console displays this line
	int		x;			// offset in current line for next print

	int		topline; // 0 <= topline < CON_MAXLINES

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	char		version[80];

	int		sideMargin;
	int		screenFakeWidth; // width in fake 640x480, it can be more than 640
} console_t;

console_t	con;

#define	DEFAULT_CONSOLE_WIDTH	78

#define		COMMAND_HISTORY		32

mfield_t	historyEditLines[COMMAND_HISTORY];

int			nextHistoryLine;		// the last line in the history buffer, not masked
int			historyLine;	// the line being displayed from history buffer
							// will be <= nextHistoryLine

mfield_t		g_consoleField;
int			g_console_field_width = DEFAULT_CONSOLE_WIDTH;

void CG_LoadConsoleHistory( void );
void CG_SaveConsoleHistory( void );

/*
================
Con_ClearConsole_f
================
*/
void Con_ClearConsole_f (void) {
	int i;

	for ( i = 0; i < CON_MAXLINES; i++ ) {
		con.lines[i][0] = '\0';
	}

	con.current = con.display = con.topline = con.x = 0;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	uiClientState_t	cls;

	trap_GetClientState( &cls );

	// Can't toggle the console when it's the only thing available
	if ( cls.connState == CA_DISCONNECTED && trap_Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		return;
	}

	if ( con_autoclear.integer ) {
		MField_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	trap_Key_SetCatcher( trap_Key_GetCatcher( ) ^ KEYCATCH_CONSOLE );
}

/*
================
Con_LineFeed
================
*/
void Con_LineFeed( qboolean keepColor ) {
	char color = 0;
	int i;

	if ( keepColor ) {
		// find last color char on line
		for ( i = con.x-1; i >= 0; i-- ) {
			if ( Q_IsColorString( &con.lines[con.current % CON_MAXLINES][i] ) ) {
				color = con.lines[con.current % CON_MAXLINES][i+1];
				break;
			}
		}
	}

	if ( con.display == con.current ) {
		con.display++;
	}
	con.current++;

	// add last color at start of new line. ignore white, it's the default.
	if ( keepColor && color != 0 && color != COLOR_WHITE ) {
		con.lines[con.current % CON_MAXLINES][0] = Q_COLOR_ESCAPE;
		con.lines[con.current % CON_MAXLINES][1] = color;
		con.x = 2;
	} else {
		con.x = 0;
	}

	con.lines[con.current % CON_MAXLINES][con.x] = '\0';

	if ( con.current < CON_MAXLINES ) {
		con.topline = con.current;
	}
}

/*
================
CG_ConsolePrint
================
*/
void CG_ConsolePrint( const char *p ) {
	int lineDrawLen, wordDrawLen, charDrawLen;
	int wordLen;
	int i;

	while ( *p != '\0' ) {
		if ( *p == '\r' ) {
			con.x = 0;
			p++;
			continue;
		} else if ( *p == '\n' ) {
			Con_LineFeed( qfalse );
			p++;
			continue;
		}

		lineDrawLen = CG_DrawStrlen( con.lines[con.current % CON_MAXLINES] ) * SMALLCHAR_WIDTH;
		wordDrawLen = charDrawLen = 0;

		for ( i = 0; i < CON_LINELENGTH; i++ ) {
			if ( Q_IsColorString( &p[i] ) ) {
				continue;
			}

			if ( p[i] == '\0' || p[i] == '\r' || p[i] == '\n' || p[i] == ' ' ) {
				break;
			}

			// break string at slashes, but don't leave them trailing at end of line (stick to beginning of next word)
			if ( i > 0 && ( p[i] == '/' || p[i] == '\\' ) ) {
				break;
			}

			charDrawLen = CG_DrawStrlenEx( &p[i], 1 ) * SMALLCHAR_WIDTH;

			// make sure the word will fit on screen, even if it needs a whole line to do so.
			if ( wordDrawLen + charDrawLen > con.screenFakeWidth - ( con.sideMargin * 2 ) ) {
				break;
			}

			wordDrawLen += charDrawLen;
		}

		// if *p is a word separator or word is too long, just copy one char at a time
		if ( i == 0 || i == CON_LINELENGTH ) {
			wordLen = 1;
		} else {
			wordLen = i;
		}

		if ( con.x + wordLen > CON_LINELENGTH ) {
			Con_LineFeed( qtrue );
		} else  if ( lineDrawLen + wordDrawLen > con.screenFakeWidth - ( con.sideMargin * 2 ) ) {
			Con_LineFeed( qtrue );
		}

		Q_strncpyz( &con.lines[con.current % CON_MAXLINES][con.x], p, wordLen+1 );
		con.x += wordLen;
		p += wordLen;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput ( connstate_t state, int lines ) {
	int		y;

	if ( state != CA_DISCONNECTED && !(trap_Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = lines - ( SMALLCHAR_HEIGHT * 2 );

	CG_DrawSmallStringColor( con.sideMargin, y, "]", g_color_table[ColorIndex(COLOR_WHITE)] );

	MField_Draw( &g_consoleField, 2 * SMALLCHAR_WIDTH, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, g_color_table[ColorIndex(COLOR_WHITE)] );
}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( connstate_t state, float frac ) {
	int				i;
	int				x, y;
	int				rows;
	char			*text;
	int				row;
	int				lines;
	vec4_t			color;

	lines = SCREEN_HEIGHT * frac;
	if (lines <= 0)
		return;

	if (lines > SCREEN_HEIGHT )
		lines = SCREEN_HEIGHT;

	CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );

	// draw the background
	y = frac * SCREEN_HEIGHT;
	if ( y < 1 ) {
		y = 0;
	}
	else {
		CG_DrawPic( 0, 0, SCREEN_WIDTH, y, cgs.media.consoleShader );
	}

	color[0] = 1;
	color[1] = 0;
	color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, y, SCREEN_WIDTH, 2, color );

	CG_SetScreenPlacement( PLACE_RIGHT, PLACE_TOP );

	// draw the version number
	CG_DrawSmallStringColor( SCREEN_WIDTH - CG_DrawStrlen( con.version ) * SMALLCHAR_WIDTH,
			lines - SMALLCHAR_HEIGHT, con.version, color );

	CG_SetScreenPlacement( PLACE_LEFT, PLACE_TOP );

	// draw the text
	rows = (lines-SMALLCHAR_HEIGHT)/SMALLCHAR_HEIGHT;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{
		int linewidth = con.screenFakeWidth / SMALLCHAR_WIDTH;
		// draw arrows to show the buffer is backscrolled
		trap_R_SetColor( color );
		for (x=0 ; x<linewidth ; x+=4)
			CG_DrawChar( (x+1)*SMALLCHAR_WIDTH, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
		trap_R_SetColor( NULL );
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= CON_MAXLINES) {
			// past scrollback wrap point
			continue;
		}

		text = con.lines[row % CON_MAXLINES];
		CG_DrawSmallString( con.sideMargin, y, text, 1.0f );
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ( state, lines );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( connstate_t state ) {
	// if disconnected, render console full screen
	if ( state == CA_DISCONNECTED ) {
		if ( !( trap_Key_GetCatcher( ) & KEYCATCH_UI) ) {
			Con_DrawSolidConsole( state, 1.0f );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( state, con.displayFrac );
	}
}

//================================================================

/*
==================
CG_RunConsole

Scroll it up or down and draw it
==================
*/
void CG_RunConsole( connstate_t state ) {
	// decide on the destination height of the console
	if ( trap_Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5;		// half screen
	else
		con.finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed.value*cg.realFrameTime*0.001;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed.value*cg.realFrameTime*0.001;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

	Con_DrawConsole( state );
}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.topline ) {
		con.display = con.current - con.topline + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.topline;
	if ( con.current - con.display >= con.topline ) {
		con.display = con.current - con.topline + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void CG_CloseConsole( void ) {
	MField_Clear( &g_consoleField );
	trap_Key_SetCatcher( trap_Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
====================
Console_Key

Handles history and console scrollback
====================
*/
void Console_Key ( int key, qboolean down ) {

	if ( !down ) {
		return;
	}

	if ( key & K_CHAR_FLAG ) {
		key &= ~K_CHAR_FLAG;
		MField_CharEvent( &g_consoleField, key );
		return;
	}

	// ctrl-L clears screen
	if ( key == 'l' && trap_Key_IsDown( K_CTRL ) ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "clear\n" );
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		uiClientState_t cls;

		trap_GetClientState( &cls );

		// if not in the game explicitly prepend a slash if needed
		if ( cls.connState != CA_ACTIVE && con_autochat.integer &&
				g_consoleField.buffer[0] &&
				g_consoleField.buffer[0] != '\\' &&
				g_consoleField.buffer[0] != '/' ) {
			char	temp[MAX_EDIT_LINE-1];

			Q_strncpyz( temp, g_consoleField.buffer, sizeof( temp ) );
			Com_sprintf( g_consoleField.buffer, sizeof( g_consoleField.buffer ), "\\%s", temp );
			g_consoleField.cursor++;
		}

		Com_Printf ( "]%s\n", g_consoleField.buffer );

		// leading slash is an explicit command
		if ( g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/' ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, g_consoleField.buffer+1 );	// valid command
			trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
		} else {
			// other text will be chat messages
			if ( !g_consoleField.buffer[0] ) {
				return;	// empty lines just scroll the console without adding to history
			} else {
				if ( con_autochat.integer ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, "cmd say " );
				}
				trap_Cmd_ExecuteText( EXEC_APPEND, g_consoleField.buffer );
				trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
			}
		}

		// copy line to history buffer
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;

		MField_Clear( &g_consoleField );

		g_consoleField.widthInChars = g_console_field_width;

		CG_SaveConsoleHistory( );

		if ( cls.connState == CA_DISCONNECTED ) {
			trap_UpdateScreen();	// force an update, because the command
		}							// may take some time
		return;
	}

	// command completion

	if (key == K_TAB) {
		char newbuf[MAX_EDIT_LINE];

		trap_Cmd_AutoComplete( g_consoleField.buffer, newbuf, sizeof ( newbuf ) );

		if ( strcmp( newbuf, g_consoleField.buffer ) != 0 ) {
			Q_strncpyz( g_consoleField.buffer, newbuf, sizeof ( g_consoleField.buffer ) );
			g_consoleField.cursor = strlen( g_consoleField.buffer );
		}
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	if ( ( key == K_MWHEELUP && trap_Key_IsDown( K_SHIFT ) ) || ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
		 ( ( tolower(key) == 'p' ) && trap_Key_IsDown( K_CTRL ) ) ) {
		if ( nextHistoryLine - historyLine < COMMAND_HISTORY 
			&& historyLine > 0 ) {
			historyLine--;
		}
		g_consoleField = historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	if ( ( key == K_MWHEELDOWN && trap_Key_IsDown( K_SHIFT ) ) || ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
		 ( ( tolower(key) == 'n' ) && trap_Key_IsDown( K_CTRL ) ) ) {
		historyLine++;
		if (historyLine >= nextHistoryLine) {
			historyLine = nextHistoryLine;
			MField_Clear( &g_consoleField );
			g_consoleField.widthInChars = g_console_field_width;
			return;
		}
		g_consoleField = historyEditLines[ historyLine % COMMAND_HISTORY ];
		return;
	}

	// console scrolling
	if ( key == K_PGUP || key == K_KP_PGUP ) {
		Con_PageUp();
		return;
	}

	if ( key == K_PGDN || key == K_KP_PGDN ) {
		Con_PageDown();
		return;
	}

	if ( key == K_MWHEELUP) {	//----(SA)	added some mousewheel functionality to the console
		Con_PageUp();
		if ( trap_Key_IsDown( K_CTRL ) ) {	// hold <ctrl> to accelerate scrolling
			Con_PageUp();
			Con_PageUp();
		}
		return;
	}

	if ( key == K_MWHEELDOWN) {	//----(SA)	added some mousewheel functionality to the console
		Con_PageDown();
		if ( trap_Key_IsDown( K_CTRL ) ) {	// hold <ctrl> to accelerate scrolling
			Con_PageDown();
			Con_PageDown();
		}
		return;
	}

	// ctrl-home = top of console
	if ( ( key == K_HOME || key == K_KP_HOME ) && trap_Key_IsDown( K_CTRL ) ) {
		Con_Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( ( key == K_END || key == K_KP_END ) && trap_Key_IsDown( K_CTRL ) ) {
		Con_Bottom();
		return;
	}

	// pass to the normal editline routine
	MField_KeyDownEvent( &g_consoleField, key );
}


// This must not exceed MAX_CMD_LINE
#define			MAX_CONSOLE_SAVE_BUFFER	1024
#define			CONSOLE_HISTORY_FILE    "consolehistory.dat"
static char	consoleSaveBuffer[ MAX_CONSOLE_SAVE_BUFFER ];
static int	consoleSaveBufferSize = 0;

/*
================
CG_LoadConsoleHistory

Load the console history from cl_consoleHistory
================
*/
void CG_LoadConsoleHistory( void )
{
	char					*token, *text_p;
	int						i, numChars, numLines = 0;
	fileHandle_t	f;

	consoleSaveBufferSize = trap_FS_FOpenFile( CONSOLE_HISTORY_FILE, &f, FS_READ );
	if( !f )
	{
		Com_Printf( "Couldn't read %s.\n", CONSOLE_HISTORY_FILE );
		return;
	}

	if( consoleSaveBufferSize <= MAX_CONSOLE_SAVE_BUFFER &&
			trap_FS_Read( consoleSaveBuffer, consoleSaveBufferSize, f ) == consoleSaveBufferSize )
	{
		text_p = consoleSaveBuffer;

		for( i = COMMAND_HISTORY - 1; i >= 0; i-- )
		{
			if( !*( token = COM_Parse( &text_p ) ) )
				break;

			historyEditLines[ i ].cursor = atoi( token );

			if( !*( token = COM_Parse( &text_p ) ) )
				break;

			historyEditLines[ i ].scroll = atoi( token );

			if( !*( token = COM_Parse( &text_p ) ) )
				break;

			numChars = atoi( token );
			text_p++;
			if( numChars > ( strlen( consoleSaveBuffer ) -	( text_p - consoleSaveBuffer ) ) )
			{
				Com_DPrintf( S_COLOR_YELLOW "WARNING: probable corrupt history\n" );
				break;
			}
			Com_Memcpy( historyEditLines[ i ].buffer,
					text_p, numChars );
			historyEditLines[ i ].buffer[ numChars ] = '\0';
			text_p += numChars;

			numLines++;
		}

		memmove( &historyEditLines[ 0 ], &historyEditLines[ i + 1 ],
				numLines * sizeof( mfield_t ) );
		for( i = numLines; i < COMMAND_HISTORY; i++ )
			MField_Clear( &historyEditLines[ i ] );

		historyLine = nextHistoryLine = numLines;
	}
	else
		Com_Printf( "Couldn't read %s.\n", CONSOLE_HISTORY_FILE );

	trap_FS_FCloseFile( f );
}

/*
================
CG_SaveConsoleHistory

Save the console history into the cvar cl_consoleHistory
so that it persists across invocations of q3
================
*/
void CG_SaveConsoleHistory( void )
{
	int						i;
	int						lineLength, saveBufferLength, additionalLength;
	fileHandle_t	f;

	consoleSaveBuffer[ 0 ] = '\0';

	i = ( nextHistoryLine - 1 ) % COMMAND_HISTORY;
	do
	{
		if( historyEditLines[ i ].buffer[ 0 ] )
		{
			lineLength = strlen( historyEditLines[ i ].buffer );
			saveBufferLength = strlen( consoleSaveBuffer );

			//ICK
			additionalLength = lineLength + strlen( "999 999 999  " );

			if( saveBufferLength + additionalLength < MAX_CONSOLE_SAVE_BUFFER )
			{
				Q_strcat( consoleSaveBuffer, MAX_CONSOLE_SAVE_BUFFER,
						va( "%d %d %d %s ",
						historyEditLines[ i ].cursor,
						historyEditLines[ i ].scroll,
						lineLength,
						historyEditLines[ i ].buffer ) );
			}
			else
				break;
		}
		i = ( i - 1 + COMMAND_HISTORY ) % COMMAND_HISTORY;
	}
	while( i != ( nextHistoryLine - 1 ) % COMMAND_HISTORY );

	consoleSaveBufferSize = strlen( consoleSaveBuffer );

	trap_FS_FOpenFile( CONSOLE_HISTORY_FILE, &f, FS_WRITE );
	if( !f )
	{
		Com_Printf( "Couldn't write %s.\n", CONSOLE_HISTORY_FILE );
		return;
	}

	if( trap_FS_Write( consoleSaveBuffer, consoleSaveBufferSize, f ) < consoleSaveBufferSize )
		Com_Printf( "Couldn't write %s.\n", CONSOLE_HISTORY_FILE );

	trap_FS_FCloseFile( f );
}


/*
================
CG_ConsoleInit
================
*/
void CG_ConsoleInit( void ) {
	int i;

	trap_Cvar_VariableStringBuffer( "version", con.version, sizeof ( con.version ) );

	con.sideMargin = SMALLCHAR_WIDTH;
	// fit across whole screen inside of a 640x480 box
	con.screenFakeWidth = cgs.glconfig.vidWidth / cgs.screenXScale;

	//g_console_field_width = con.screenFakeWidth / SMALLCHAR_WIDTH - 2;

	MField_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		MField_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}

	CG_LoadConsoleHistory();
}


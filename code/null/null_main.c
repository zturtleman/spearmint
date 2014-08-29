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
// null_main.c -- null system driver to aid porting efforts

#include <errno.h>
#include <stdio.h>
#include "../qcommon/qcommon.h"

int			sys_curtime;


//===================================================================


void Sys_Error (char *error, ...) {
	va_list		argptr;

	printf ("Sys_Error: ");	
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Quit (void) {
	exit (0);
}

char *Sys_GetClipboardData( void ) {
	return NULL;
}

int		Sys_Milliseconds (void) {
	return 0;
}

FILE	*Sys_FOpen(const char *ospath, const char *mode) {
	return fopen( ospath, mode );
}

void	Sys_Mkdir (char *path) {
}

void	Sys_Init (void) {
}


void main (int argc, char **argv) {
	Com_Init (argc, argv);

	while (1) {
		Com_Frame( );
	}
}



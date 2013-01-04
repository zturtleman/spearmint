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

/*****************************************************************************
 * name:		be_interface.h
 *
 * desc:		botlib interface
 *
 * $Archive: /source/code/botlib/be_interface.h $
 *
 *****************************************************************************/

//#define DEBUG			//debug code
#define RANDOMIZE		//randomize bot behaviour

//FIXME: get rid of this global structure
typedef struct botlib_globals_s
{
	int botlibsetup;						//true when the bot library has been setup
	int maxentities;						//maximum number of entities
	int maxclients;							//maximum number of clients
	float time;								//the global time
#ifdef DEBUG
	qboolean debug;							//true if debug is on
	int goalareanum;
	vec3_t goalorigin;
	int runai;
#endif
} botlib_globals_t;


extern botlib_globals_t botlibglobals;
extern botlib_import_t botimport;
extern int botDeveloper;					//true if developer is on

//
int Sys_MilliSeconds(void);


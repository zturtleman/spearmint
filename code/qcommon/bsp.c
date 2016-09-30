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

#include "q_shared.h"
#include "qcommon.h"
#include "bsp.h"

#ifdef BSPC
#include "../bspc/l_qfiles.h"
#endif

bspFormat_t *bspFormats[] = {
	&quake3BspFormat,
	&wolfBspFormat,
	&darksBspFormat,
	&fakkBspFormat,
	&aliceBspFormat,
	&sof2BspFormat,
	&ef2BspFormat,
	&mohaaBspFormat,
	&q3Test106BspFormat,
};

const int numBspFormats = ARRAY_LEN( bspFormats );

#define MAX_BSP_FILES 1
bspFile_t *bsp_loadedFiles[MAX_BSP_FILES] = {0};


bspFile_t *BSP_Load( const char *name ) {
	union {
		int				*i;
		void			*v;
	} buf;
	int				i;
	int				length;
	bspFile_t		*bspFile = NULL;
	int				freeSlot = -1;

#ifndef BSPC
	if ( !name || !name[0] ) {
		Com_Error( ERR_DROP, "BSP_Load: NULL name" );
	}
#endif

	// check if already loaded
	for ( i = 0; i < MAX_BSP_FILES; i++ ) {
		if ( !bsp_loadedFiles[i] ) {
			if ( freeSlot == -1 ) {
				freeSlot = i;
			}
			continue;
		}

		if ( !Q_stricmp( bsp_loadedFiles[i]->name, name ) ) {
			bsp_loadedFiles[i]->references++;
			return bsp_loadedFiles[i];
		}
	}

	if ( freeSlot == -1 ) {
		Com_Error( ERR_DROP, "No free slot to load BSP '%s'", name );
	}

	//
	// load the file
	//
#ifndef BSPC
	length = FS_ReadFile( name, &buf.v );
#else
	length = LoadQuakeFile((quakefile_t *) name, &buf.v);
#endif

	if ( !buf.i ) {
		// File not found.
		return NULL;
	}

	//
	// check formats
	//
	for ( i = 0; i < numBspFormats; i++ ) {
		bspFile = bspFormats[i]->loadFunction( bspFormats[i], name, buf.v, length );
		if ( bspFile ) {
			break;
		}
	}

	if ( i == numBspFormats ) {
		int ident = LittleLong( buf.i[0] );
		int version = LittleLong( buf.i[1] );

		Com_Error( ERR_DROP, "Unsupported BSP %s: ident %c%c%c%c, version %d",
				name, ident & 0xff, ( ident >> 8 ) & 0xff, ( ident >> 16 ) & 0xff,
				( ident >> 24 ) & 0xff, version );
	}

	if ( bspFile ) {
		Q_strncpyz( bspFile->name, name, sizeof ( bspFile->name ) );
		bspFile->references++;
		bsp_loadedFiles[freeSlot] = bspFile;
	}

	FS_FreeFile (buf.v);

	return bspFile;
}

static void BSP_FreeInternal( bspFile_t *bsp ) {
	free( bsp->entityString );
	free( bsp->shaders );
	free( bsp->planes );
	free( bsp->nodes );
	free( bsp->leafs );
	free( bsp->leafSurfaces );
	free( bsp->leafBrushes );
	free( bsp->submodels );
	free( bsp->brushes );
	free( bsp->brushSides );
	free( bsp->drawVerts );
	free( bsp->drawIndexes );
	free( bsp->fogs );
	free( bsp->surfaces );
	free( bsp->lightmapData );
	free( bsp->lightGridData );
	free( bsp->visibility );
	free( bsp );
}

void BSP_Free( bspFile_t *bspFile ) {
	int i;

	if ( !bspFile )
		return;

	bspFile->references--;
	if ( bspFile->references > 0 )
		return;

	for ( i = 0; i < MAX_BSP_FILES; i++ ) {
		if ( bspFile == bsp_loadedFiles[i] ) {
			bsp_loadedFiles[i] = NULL;
			break;
		}
	}

	BSP_FreeInternal( bspFile );
}

void BSP_Shutdown( void ) {
	int i;

	for ( i = 0; i < MAX_BSP_FILES; i++ ) {
		if ( !bsp_loadedFiles[i] ) {
			continue;
		}

		BSP_FreeInternal( bsp_loadedFiles[i] );
		bsp_loadedFiles[i] = NULL;
	}
}

/*
    Common BSP Loading Utility functions
 */

/*
   SwapBlock()
   if all values are 32 bits, this can be used to swap everything
 */
void BSP_SwapBlock( int *dest, const int *src, int size ) {
	int i;

	/* dummy check */
	if ( dest == NULL || src == NULL ) {
		return;
	}

	/* swap */
	size >>= 2;
	for ( i = 0; i < size; i++ )
		dest[ i ] = LittleLong( src[ i ] );
}


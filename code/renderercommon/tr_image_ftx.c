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

#include "tr_common.h"

typedef struct {
	int width, height, hasTranparency;
} FtxHeader;

void R_LoadFTX( const char *name, int *numTexLevels, textureLevel_t **pic )
{
	unsigned	numPixels;
	byte	*buf_p;
	union {
		byte *b;
		void *v;
	} buffer;
	FtxHeader	ftx_header;
	byte		*ftx_rgba;
	int length;

	*pic = NULL;
	*numTexLevels = 0;

	//
	// load the file
	//
	length = ri.FS_ReadFile ( ( char * ) name, &buffer.v);
	if ( !buffer.b || length < 0 ) {
		return;
	}

	if ( length < sizeof (FtxHeader) )
	{
		ri.Error( ERR_DROP, "LoadFTX: header too short (%s)", name );
	}

	buf_p = buffer.b;

	ftx_header = *(FtxHeader*)buf_p;
	ftx_header.width = LittleLong(ftx_header.width);
	ftx_header.height = LittleLong(ftx_header.height);
	ftx_header.hasTranparency = LittleLong(ftx_header.hasTranparency);

	buf_p += sizeof (FtxHeader);

	numPixels = ftx_header.width * ftx_header.height * 4;

	if ( ftx_header.width <= 0 || ftx_header.height <= 0 || numPixels > 0x7FFFFFFF )
	{
		ri.Error (ERR_DROP, "LoadFTX: %s has an invalid image size", name);
	}

	*pic = ri.Malloc( sizeof(textureLevel_t) + numPixels );
	(*pic)->format = GL_RGBA8;
	(*pic)->width = ftx_header.width;
	(*pic)->height = ftx_header.height;
	(*pic)->size = numPixels;
	(*pic)->data = ftx_rgba = (byte *)(*pic + 1);
	*numTexLevels = 1;

	Com_Memcpy( ftx_rgba, buf_p, numPixels );

	ri.FS_FreeFile (buffer.v);
}

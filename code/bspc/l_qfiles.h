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

#include "../qcommon/unzip.h"

#define QFILETYPE_UNKNOWN			0x8000
#define QFILETYPE_PK3				0x0001
#define QFILETYPE_BSP				0x0002
#define QFILETYPE_MAP				0x0004
#define QFILETYPE_AAS				0x0008

#define QFILEEXT_UNKNOWN			""
#define QFILEEXT_PK3				".PK3"
#define QFILEEXT_BSP				".BSP"
#define QFILEEXT_MAP				".MAP"
#define QFILEEXT_AAS				".AAS"

//maximum path length
#ifndef _MAX_PATH
	#define _MAX_PATH				1024
#endif

typedef struct quakefile_s
{
	char pakfile[_MAX_PATH];
	char filename[_MAX_PATH];
	char origname[_MAX_PATH];
	int zipfile;
	int type;
	unsigned long length;
	unsigned long pos;
	struct quakefile_s *next;
} quakefile_t;

//returns the file type for the given extension
int QuakeFileExtensionType(char *extension);
//return the Quake file type for the given file
int QuakeFileType(char *filename);
//returns true if the filename complies to the filter
int FileFilter(char *filter, char *filename, int casesensitive);
//find Quake files using the given filter
quakefile_t *FindQuakeFiles(char *filter);
//load the given Quake file, returns the length of the file
int LoadQuakeFile(quakefile_t *qf, void **bufferptr);
//read part of a Quake file into the buffer
int ReadQuakeFile(quakefile_t *qf, void *buffer, int length);

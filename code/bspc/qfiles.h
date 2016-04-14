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

#include "../qcommon/surfaceflags.h"

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

#define	MAX_MAP_ENTITIES	0x1000 // ZTM: NOTE: Using value from WolfET, it's only 0x800 in Quake3/RTCW-MP.
#define	MAX_MAP_BRUSHSIDES	0x100000 // ZTM: NOTE: Using value from WolfET, it's only 0x20000 in Quake3/RTCW-MP.

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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

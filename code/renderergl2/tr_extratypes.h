/*
===========================================================================
Copyright (C) 2009-2011 Andrei Drexler, Richard Allen, James Canete

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

#ifndef __TR_EXTRATYPES_H__
#define __TR_EXTRATYPES_H__

// tr_extratypes.h, for mods that want to extend tr_types.h without losing compatibility with original VMs

// extra renderfx flags start at 0x0400
#define RF_SUNFLARE			0x0400

// extra refdef flags start at 0x0008
#define RDF_NOFOG		0x0008		// don't apply fog
#define RDF_EXTRA		0x0010		// Makro - refdefex_t to follow after refdef_t
#define RDF_SUNLIGHT    0x0020      // SmileTheory - render sunlight and shadows

typedef struct {
	float			blurFactor;
	float           sunDir[3];
	float           sunCol[3];
	float           sunAmbCol[3];
} refdefex_t;

#endif

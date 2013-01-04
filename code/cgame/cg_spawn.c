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

/*
 * name:		cg_spawn.c
 *
 * desc:		Client sided only map entities
*/

#include "cg_local.h"

qboolean CG_SpawnString( const char *key, const char *defaultString, char **out ) {
	int i;

	if ( !cg.spawning ) {
		*out = (char *)defaultString;
		CG_Error( "CG_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < cg.numSpawnVars ; i++ ) {
		if ( !strcmp( key, cg.spawnVars[i][0] ) ) {
			*out = cg.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean CG_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char        *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean CG_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char        *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean CG_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char        *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}

qboolean CG_SpawnVector2D( const char *key, const char *defaultString, float *out ) {
	char        *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f", &out[0], &out[1] );
	return present;
}

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char    *vtos( const vec3_t v ) {
	static int index;
	static char str[8][32];
	char    *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2] );

	return s;
}

#if 0
void SP_misc_gamemodel( void ) {
	char* model;
	vec_t angle;
	vec3_t angles;

	vec_t scale;
	vec3_t vScale;

	vec3_t org;

	cg_gamemodel_t* gamemodel;

	int i;

	if ( CG_SpawnString( "targetname", "", &model ) || CG_SpawnString( "scriptname", "", &model ) || CG_SpawnString( "spawnflags", "", &model ) ) {
		// Gordon: this model may not be static, so let the server handle it
		return;
	}

	if ( cg.numMiscGameModels >= MAX_STATIC_GAMEMODELS ) {
		CG_Error( "^1MAX_STATIC_GAMEMODELS(%i) hit", MAX_STATIC_GAMEMODELS );
	}

	CG_SpawnString( "model", "", &model );

	CG_SpawnVector( "origin", "0 0 0", org );

	if ( !CG_SpawnVector( "angles", "0 0 0", angles ) ) {
		if ( CG_SpawnFloat( "angle", "0", &angle ) ) {
			angles[YAW] = angle;
		}
	}

	if ( !CG_SpawnVector( "modelscale_vec", "1 1 1", vScale ) ) {
		if ( CG_SpawnFloat( "modelscale", "1", &scale ) ) {
			VectorSet( vScale, scale, scale, scale );
		}
	}

	gamemodel = &cgs.miscGameModels[cg.numMiscGameModels++];
	gamemodel->model = trap_R_RegisterModel( model );
	AnglesToAxis( angles, gamemodel->axes );
	for ( i = 0; i < 3; i++ ) {
		VectorScale( gamemodel->axes[i], vScale[i], gamemodel->axes[i] );
	}
	VectorCopy( org, gamemodel->org );

	if ( gamemodel->model ) {
		vec3_t mins, maxs;

		trap_R_ModelBounds( gamemodel->model, mins, maxs );

		for ( i = 0; i < 3; i++ ) {
			mins[i] *= vScale[i];
			maxs[i] *= vScale[i];
		}

		gamemodel->radius = RadiusFromBounds( mins, maxs );
	} else {
		gamemodel->radius = 0;
	}
}
#endif

typedef struct {
	char    *name;
	void ( *spawn )( void );
} spawn_t;

spawn_t spawns[] = {
	{0, 0},
#if 0
	{"misc_gamemodel",               SP_misc_gamemodel},
#endif
};

#define NUMSPAWNS   ( sizeof( spawns ) / sizeof( spawn_t ) )

/*
===================
CG_ParseEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
cg.spawnVars[], then call the class specfic spawn function
===================
*/
void CG_ParseEntityFromSpawnVars( void ) {
	int i;
	char    *classname;

#if 0
	// check for "notteam" / "notfree" flags
	CG_SpawnInt( "notteam", "0", &i );
	if ( i ) {
		return;
	}
#endif

	if ( CG_SpawnString( "classname", "", &classname ) ) {
		for ( i = 0; i < NUMSPAWNS; i++ ) {
			if ( !Q_stricmp( spawns[i].name, classname ) ) {
				spawns[i].spawn();
				break;
			}
		}
	}

}

/*
====================
CG_AddSpawnVarToken
====================
*/
char *CG_AddSpawnVarToken( const char *string ) {
	int l;
	char    *dest;

	l = strlen( string );
	if ( cg.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		CG_Error( "CG_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = cg.spawnVarChars + cg.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	cg.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
CG_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into cg.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean CG_ParseSpawnVars( void ) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	cg.numSpawnVars = 0;
	cg.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		CG_Error( "CG_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			CG_Error( "CG_ParseSpawnVars: closing brace without data" );
		}
		if ( cg.numSpawnVars == MAX_SPAWN_VARS ) {
			CG_Error( "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		cg.spawnVars[ cg.numSpawnVars ][0] = CG_AddSpawnVarToken( keyname );
		cg.spawnVars[ cg.numSpawnVars ][1] = CG_AddSpawnVarToken( com_token );
		cg.numSpawnVars++;
	}

	return qtrue;
}

void SP_worldspawn( void ) {
	char    *s;

	CG_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		CG_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	if ( CG_SpawnVector2D( "mapcoordsmins", "-128 128", cg.mapcoordsMins ) &&  // top left
		 CG_SpawnVector2D( "mapcoordsmaxs", "128 -128", cg.mapcoordsMaxs ) ) { // bottom right
		cg.mapcoordsValid = qtrue;
	} else {
		cg.mapcoordsValid = qfalse;
	}

#if 0
	CG_ParseSpawns();

	cg.mapcoordsScale[0] = 1 / ( cg.mapcoordsMaxs[0] - cg.mapcoordsMins[0] );
	cg.mapcoordsScale[1] = 1 / ( cg.mapcoordsMaxs[1] - cg.mapcoordsMins[1] );

	BG_InitLocations( cg.mapcoordsMins, cg.mapcoordsMaxs );
#endif

	CG_SpawnString( "atmosphere", "", &s );
	CG_EffectParse( s );
}

/*
==============
CG_ParseEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void CG_ParseEntitiesFromString( void ) {
	// allow calls to CG_Spawn*()
	cg.spawning = qtrue;
	cg.numSpawnVars = 0;
#if 0
	cg.numMiscGameModels = 0;
#endif

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !CG_ParseSpawnVars() ) {
		CG_Error( "ParseEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while ( CG_ParseSpawnVars() ) {
		CG_ParseEntityFromSpawnVars();
	}

	cg.spawning = qfalse;           // any future calls to CG_Spawn*() will be errors
}

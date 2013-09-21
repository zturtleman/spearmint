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
// g_misc.c

#include "g_local.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear
Non-displayed light.
"light" overrides the default 300 intensity.
Linear checbox gives linear falloff instead of inverse square
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
*/
void SP_light( gentity_t *self ) {
	G_FreeEntity( self );
}

/*QUAKED lightJunior (0 0.7 0.3) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point
Non-displayed light that only affects dynamic game models, but does not contribute to lightmaps
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights)
*/
void SP_lightJunior( gentity_t *self ) {
	G_FreeEntity( self );
}



/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t	*tent;
	qboolean noAngles;

	noAngles = (angles[0] > 999999.0);
	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity (player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;
	if (!noAngles) {
	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	player->client->ps.pm_time = 160;		// hold time
	player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	// set angles
	SetClientViewAngle(player, angles);
	}
	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	// kill anything at the destination
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		G_KillBox (player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap_LinkEntity (player);
	}
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
}


//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
*/
void SP_misc_model( gentity_t *ent ) {

#if 0
	ent->s.modelindex = G_ModelIndex( ent->model );
	VectorSet (ent->mins, -16, -16, -16);
	VectorSet (ent->maxs, 16, 16, 16);
	trap_LinkEntity (ent);

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
#else
	G_FreeEntity( ent );
#endif
}

/*QUAKED misc_gamemodel (1 0 0) (-16 -16 -16) (16 16 16)
md3 placed in the game at runtime (rather than in the bsp)
"model"			arbitrary .md3 file to display
"modelscale"	scale multiplier (defaults to 1x, and scales uniformly)
"modelscale_vec"	scale multiplier (defaults to 1 1 1, scales each axis as requested)

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored
*/
// ZTM: FIXME: Stub. Currently just loaded by cgame
void SP_misc_gamemodel( gentity_t *ent ) {
	G_FreeEntity( ent );
}


void locateMaster( gentity_t *ent ) {
	ent->target_ent = G_PickTarget( ent->target );
	if ( ent->target_ent ) {
		ent->r.visDummyNum = ent->target_ent->s.number;
	} else {
		G_Printf( "Couldn't find target(%s) for misc_vis_dummy at %s\n", ent->target, vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
	}
}

/*QUAKED misc_vis_dummy (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
For safety, you should have each dummy only point at one entity (however, it's okay to have many dummies pointing at one entity)
*/
void SP_misc_vis_dummy( gentity_t *ent ) {

	if ( !ent->target ) {
		G_Printf( "No target specified for misc_vis_dummy at %s\n", vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY;
	G_SetOrigin( ent, ent->s.origin );
	trap_LinkEntity( ent );

	ent->think = locateMaster;
	ent->nextthink = level.time + 1000;

}

/*QUAKED misc_vis_dummy_multiple (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
This entity was created to have multiple speakers targeting it
*/
void SP_misc_vis_dummy_multiple( gentity_t *ent ) {
	if ( !ent->targetname ) {
		G_Printf( "misc_vis_dummy_multiple needs a targetname at %s\n", vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY_MULTIPLE;
	G_SetOrigin( ent, ent->s.origin );
	trap_LinkEntity( ent );

}

//===========================================================

void locateCamera( gentity_t *ent ) {
	vec3_t		dir;
	gentity_t	*target;
	gentity_t	*owner;

	owner = G_PickTarget( ent->target );
	if ( !owner ) {
		G_Printf( "Couldn't find target for misc_partal_surface\n" );
		G_FreeEntity( ent );
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 ) {
		ent->s.frame = 25;
	} else if ( owner->spawnflags & 2 ) {
		ent->s.frame = 75;
	}

	// swing camera ?
	if ( owner->spawnflags & 4 ) {
		// set to 0 for no rotation at all
		ent->s.powerups = 0;
	}
	else {
		ent->s.powerups = 1;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickTarget( owner->target );
	if ( target ) {
		VectorSubtract( target->s.origin, owner->s.origin, dir );
		VectorNormalize( dir );
	} else {
		G_SetMovedir( owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface(gentity_t *ent) {
	VectorClear( ent->s.mins );
	VectorClear( ent->s.maxs );
	trap_LinkEntity (ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->target ) {
		VectorCopy( ent->s.origin, ent->s.origin2 );
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate noswing
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera(gentity_t *ent) {
	float	roll;

	VectorClear( ent->s.mins );
	VectorClear( ent->s.maxs );
	trap_LinkEntity (ent);

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll/360.0 * 256;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void Use_Shooter( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	vec3_t		dir;
	float		deg;
	vec3_t		up, right;

	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->r.currentOrigin, ent->s.origin, dir );
		VectorNormalize( dir );
	} else {
		VectorCopy( ent->movedir, dir );
	}

	// randomize a bit
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, up, dir );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, right, dir );

	VectorNormalize( dir );

	switch ( ent->s.weapon ) {
	case WP_GRENADE_LAUNCHER:
		fire_grenade( ent, ent->s.origin, dir );
		break;
	case WP_ROCKET_LAUNCHER:
		fire_rocket( ent, ent->s.origin, dir );
		break;
	case WP_PLASMAGUN:
		fire_plasma( ent, ent->s.origin, dir );
		break;
	}

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}


static void InitShooter_Finish( gentity_t *ent ) {
	ent->enemy = G_PickTarget( ent->target );
	ent->think = 0;
	ent->nextthink = 0;
}

void InitShooter( gentity_t *ent, int weapon ) {
	ent->use = Use_Shooter;
	ent->s.weapon = weapon;

	RegisterItem( BG_FindItemForWeapon( weapon ) );

	G_SetMovedir( ent->s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}
	ent->random = sin( M_PI * ent->random / 180 );
	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity( ent );
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket( gentity_t *ent ) {
	InitShooter( ent, WP_ROCKET_LAUNCHER );
}

/*QUAKED shooter_plasma (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_plasma( gentity_t *ent ) {
	InitShooter( ent, WP_PLASMAGUN);
}

/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade( gentity_t *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER);
}

/*QUAKED corona (0 1 0) (-4 -4 -4) (4 4 4) START_OFF
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
"scale" will designate a multiplier to the default size.  (so 2.0 is 2xdefault size, 0.5 is half)
*/

/*
==============
use_corona
	so level designers can toggle them on/off
==============
*/
void use_corona( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else {
		trap_LinkEntity( ent );
	}
}


/*
==============
SP_corona
==============
*/
void SP_corona( gentity_t *ent ) {
	float scale;

	ent->s.eType = ET_CORONA;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1; // set white

	}
	ent->dl_color[0] = ent->dl_color[0] * 255;
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	ent->s.dl_intensity = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 );

	G_SpawnFloat( "scale", "1", &scale );
	ent->s.density = (int)( scale * 255 );

	ent->use = use_corona;

	if ( !( ent->spawnflags & 1 ) ) {
		trap_LinkEntity( ent );
	}
}


char* predef_lightstyles[] = {
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"ma",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomono",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	"mmnommomhkmmomnonmmonqnmmo",
	"kmamaamakmmmaakmamakmakmmmma",
	"kmmmakakmmaaamammamkmamakmmmma",
	"mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm",
	"mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm",
	"zzzzzzzzaaaaaaaa",
	"zzzzzzzzaaaaaaaaaaaaaaaa",
	"aaaaaaaazzzzzzzzaaaaaaaa",
	"aaaaaaaaaaaaaaaazzzzzzzz"
};


/*
==============
dlight_finish_spawning
	All the dlights should call this on the same frame, thereby
	being synched, starting	their sequences all at the same time.
==============
*/
void dlight_finish_spawning( gentity_t *ent ) {
	G_FindConfigstringIndex( va( "%i %s %i %i %i", ent->s.number, ent->dl_stylestring, ent->health, ent->soundLoop, ent->dl_atten ), CS_DLIGHTS, MAX_DLIGHT_CONFIGSTRINGS, qtrue );
}

static int dlightstarttime = 0;


/*QUAKED dlight (0 1 0) (-12 -12 -12) (12 12 12) FORCEACTIVE STARTOFF ONETIME
"style": value is an int from 1-19 that contains a pre-defined 'flicker' string.
"stylestring": set your own 'flicker' string.  (ex. "klmnmlk"). NOTE: this should be all lowercase
Stylestring characters run at 10 cps in the game. (meaning the alphabet, at 24 characters, would take 2.4 seconds to cycle)
"offset": change the initial index in a style string.  So val of 3 in the above example would start this light at 'N'.  (used to get dlights using the same style out of sync).
"atten": offset from the alpha values of the stylestring.  stylestring of "ddeeffzz" with an atten of -1 would result in "ccddeeyy"
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
FORCEACTIVE	- toggle makes sure this light stays alive in a map even if the user has r_dynamiclight set to 0.
STARTOFF	- means the dlight doesn't spawn in until ent is triggered
ONETIME		- when the dlight is triggered, it will play through it's cycle once, then shut down until triggered again
"shader" name of shader to apply
"sound" sound to loop every cycle (this actually just plays the sound at the beginning of each cycle)

styles:
1 - "mmnmmommommnonmmonqnmmo"
2 - "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba"
3 - "mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
4 - "ma"
5 - "jklmnopqrstuvwxyzyxwvutsrqponmlkj"
6 - "nmonqnmomnmomomono"
7 - "mmmaaaabcdefgmmmmaaaammmaamm"
8 - "aaaaaaaazzzzzzzz"
9 - "mmamammmmammamamaaamammma"
10 - "abcdefghijklmnopqrrqponmlkjihgfedcba"
11 - "mmnommomhkmmomnonmmonqnmmo"
12 - "kmamaamakmmmaakmamakmakmmmma"
13 - "kmmmakakmmaaamammamkmamakmmmma"
14 - "mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm"
15 - "mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm"
16 - "zzzzzzzzaaaaaaaa"
17 - "zzzzzzzzaaaaaaaaaaaaaaaa"
18 - "aaaaaaaazzzzzzzzaaaaaaaa"
19 - "aaaaaaaaaaaaaaaazzzzzzzz"
*/


/*
==============
shutoff_dlight
	the dlight knew when it was triggered to unlink after going through it's cycle once
==============
*/
void shutoff_dlight( gentity_t *ent ) {
	if ( !( ent->r.linked ) ) {
		return;
	}

	trap_UnlinkEntity( ent );
	ent->think = 0;
	ent->nextthink = 0;
}


/*
==============
use_dlight
==============
*/
void use_dlight( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else {
		trap_LinkEntity( ent );

		if ( ent->spawnflags & 4 ) {   // ONETIME
			ent->think = shutoff_dlight;
			ent->nextthink = level.time + (  strlen( ent->dl_stylestring )  * 100 ) - 100;
		}
	}
}

/*
==============
SP_dlight
	ent->dl_stylestring contains the lightstyle string
	ent->health tracks current index into style string
	ent->count tracks length of style string
==============
*/
void SP_dlight( gentity_t *ent ) {
	char *snd, *shader;
	int i;
	int offset, style, atten;

	G_SpawnInt( "offset", "0", &offset );              // starting index into the stylestring
	G_SpawnInt( "style", "0", &style );                // predefined stylestring
	G_SpawnString( "sound", "", &snd );                //
	G_SpawnInt( "atten", "0", &atten );                //
	G_SpawnString( "shader", "", &shader );            // name of shader to use for this dlight image

	if ( G_SpawnString( "sound", "0", &snd ) ) {
		ent->soundLoop = G_SoundIndex( snd );
	}

	if ( ent->dl_stylestring && strlen( ent->dl_stylestring ) ) {    // if they're specified in a string, use em
	} else if ( style )       {
		style = MAX( 1, style );                                  // clamp to predefined range
		style = MIN( 19, style );
		ent->dl_stylestring = predef_lightstyles[style - 1];    // these are input as 1-20
	} else {
		ent->dl_stylestring = "mmmaaa";                          // default to a strobe to call attention to this not being set
	}

	ent->count = strlen( ent->dl_stylestring );

	ent->dl_atten = atten;

	// make the initial offset a valid index into the stylestring
	offset = offset % ( ent->count );

	ent->health = offset;                   // set the offset into the string

	ent->think = dlight_finish_spawning;
	if ( !dlightstarttime ) {                      // sync up all the dlights
		dlightstarttime = level.time + 100;
	}
	ent->nextthink  = dlightstarttime;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned, make it white
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1;
	}

	ent->dl_color[0] = ent->dl_color[0] * 255;  // range 0-255 now so the client doesn't have to on every update
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	i = (int)( ent->dl_stylestring[offset] ) - (int)'a';
	i = i * ( 1000.0f / 24.0f );

	ent->s.constantLight = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 ) | ( i / 4 << 24 );

	ent->use = use_dlight;

	if ( !( ent->spawnflags & 2 ) ) {
		trap_LinkEntity( ent );
	}
}

#ifdef MISSIONPACK
static void PortalDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod) {
	G_FreeEntity( self );
	//FIXME do something more interesting
}


void DropPortalDestination( gentity_t *player ) {
	gentity_t	*ent;
	vec3_t		snapped;

	// create the portal destination
	ent = G_Spawn();
	ent->s.modelindex = G_ModelIndex( "models/powerups/teleporter/tele_exit.md3" );

	VectorCopy( player->s.pos.trBase, snapped );
	SnapVector( snapped );
	G_SetOrigin( ent, snapped );
	VectorCopy( player->s.mins, ent->s.mins );
	VectorCopy( player->s.maxs, ent->s.maxs );

	ent->classname = "hi_portal destination";
	ent->s.pos.trType = TR_STATIONARY;

	ent->s.contents = CONTENTS_CORPSE;
	ent->takedamage = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	VectorCopy( player->s.apos.trBase, ent->s.angles );

	ent->think = G_FreeEntity;
	ent->nextthink = level.time + 2 * 60 * 1000;

	trap_LinkEntity( ent );

	player->client->portalID = ++level.portalSequence;
	ent->count = player->client->portalID;

	// give the item back so they can drop the source now
	player->client->ps.stats[STAT_HOLDABLE_ITEM] = BG_FindItem( "Portal" ) - bg_itemlist;
}


static void PortalTouch( gentity_t *self, gentity_t *other, trace_t *trace) {
	gentity_t	*destination;

	// see if we will even let other try to use it
	if( other->health <= 0 ) {
		return;
	}
	if( !other->client ) {
		return;
	}
//	if( other->client->ps.persistant[PERS_TEAM] != self->spawnflags ) {
//		return;
//	}

	if ( other->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_NEUTRALFLAG ), 0 );
		other->client->ps.powerups[PW_NEUTRALFLAG] = 0;
	}
	else if ( other->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_REDFLAG ), 0 );
		other->client->ps.powerups[PW_REDFLAG] = 0;
	}
	else if ( other->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
		Drop_Item( other, BG_FindItemForPowerup( PW_BLUEFLAG ), 0 );
		other->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	// find the destination
	destination = NULL;
	while( (destination = G_Find(destination, FOFS(classname), "hi_portal destination")) != NULL ) {
		if( destination->count == self->count ) {
			break;
		}
	}

	// if there is not one, die!
	if( !destination ) {
		if( self->pos1[0] || self->pos1[1] || self->pos1[2] ) {
			TeleportPlayer( other, self->pos1, self->s.angles );
		}
		G_Damage( other, other, other, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
		return;
	}

	TeleportPlayer( other, destination->s.pos.trBase, destination->s.angles );
}


static void PortalEnable( gentity_t *self ) {
	self->touch = PortalTouch;
	self->think = G_FreeEntity;
	self->nextthink = level.time + 2 * 60 * 1000;
}


void DropPortalSource( gentity_t *player ) {
	gentity_t	*ent;
	gentity_t	*destination;
	vec3_t		snapped;

	// create the portal source
	ent = G_Spawn();
	ent->s.modelindex = G_ModelIndex( "models/powerups/teleporter/tele_enter.md3" );

	VectorCopy( player->s.pos.trBase, snapped );
	SnapVector( snapped );
	G_SetOrigin( ent, snapped );
	VectorCopy( player->s.mins, ent->s.mins );
	VectorCopy( player->s.maxs, ent->s.maxs );

	ent->classname = "hi_portal source";
	ent->s.pos.trType = TR_STATIONARY;

	ent->s.contents = CONTENTS_CORPSE | CONTENTS_TRIGGER;
	ent->takedamage = qtrue;
	ent->health = 200;
	ent->die = PortalDie;

	trap_LinkEntity( ent );

	ent->count = player->client->portalID;
	player->client->portalID = 0;

//	ent->spawnflags = player->client->ps.persistant[PERS_TEAM];

	ent->nextthink = level.time + 1000;
	ent->think = PortalEnable;

	// find the destination
	destination = NULL;
	while( (destination = G_Find(destination, FOFS(classname), "hi_portal destination")) != NULL ) {
		if( destination->count == ent->count ) {
			VectorCopy( destination->s.pos.trBase, ent->pos1 );
			break;
		}
	}

}
#endif

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
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT+MAX_SPLITVIEW];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap;
	entityState_t	*ent;
	playerState_t	*ps;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		ent = &snap->entities[i];
		cent = &cg_entities[ent->number];

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( ent->bmodel || ent->contents ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}

	// Add local clients to solid entity list
	for ( i = 0 ; i < snap->numPSs ; i++ ) {
		ps = &snap->pss[i];
		cent = &cg_entities[ps->clientNum];
		if ( ps->linked && ps->contents ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins,
		const vec3_t maxs, const vec3_t end, int skipNumber,
		int mask, trace_t *tr, traceType_t collisionType )
{
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		// if it doesn't have any brushes of a type we
		// are looking for, ignore it
		if ( !(mask & ent->contents) ) {
			continue;
		}

		if ( ent->bmodel ) {
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else if ( ent->capsule ) {
			cmodel = trap_CM_TempCapsuleModel( ent->mins, ent->maxs, ent->contents );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		} else {
			cmodel = trap_CM_TempBoxModel( ent->mins, ent->maxs, ent->contents );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		if( collisionType == TT_CAPSULE )
		{
			trap_CM_TransformedCapsuleTrace ( &trace, start, end,
					mins, maxs, cmodel,  mask, origin, angles );
		}
		else if( collisionType == TT_AABB )
		{
			trap_CM_TransformedBoxTrace ( &trace, start, end,
					mins, maxs, cmodel,  mask, origin, angles );
		}
		else if( collisionType == TT_BISPHERE )
		{
			trap_CM_TransformedBiSphereTrace( &trace, start, end,
					mins[ 0 ], maxs[ 0 ], cmodel, mask, origin );
		}

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			if( tr->lateralFraction < trace.lateralFraction )
			{
				float oldLateralFraction = tr->lateralFraction;
				*tr = trace;
				tr->lateralFraction = oldLateralFraction;
			} else {
				*tr = trace;
			}
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
			tr->entityNum = ent->number;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, &t, TT_AABB );

	*result = t;
}

/*
================
CG_TraceCapsule
================
*/
void	CG_TraceCapsule( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					int skipNumber, int mask )
{
	trace_t t;

	trap_CM_CapsuleTrace( &t, start, end, mins, maxs, 0, mask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, &t, TT_CAPSULE );

	*result = t;
}

/*
================
CG_BiSphereTrace
================
*/
void CG_BiSphereTrace( trace_t *result, const vec3_t start, const vec3_t end,
    const float startRadius, const float endRadius, int skipNumber, int mask )
{
	trace_t t;
	vec3_t  mins, maxs;

	mins[ 0 ] = startRadius;
	maxs[ 0 ] = endRadius;

	trap_CM_BiSphereTrace( &t, start, end, startRadius, endRadius, 0, mask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, &t, TT_BISPHERE );

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if ( !ent->bmodel ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, cent->lerpOrigin, cent->lerpAngles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.cur_lc->predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;
	playerState_t	*prevPS, *nextPS;

	out = &cg.cur_lc->predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = *cg.cur_ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd, cg.cur_localClientNum );

		PM_UpdateViewAngles( out, &cmd );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	if (prev->lcIndex[cg.cur_localClientNum] == -1 ||
		next->lcIndex[cg.cur_localClientNum] == -1) {
		return;
	}

	prevPS = &prev->pss[prev->lcIndex[cg.cur_localClientNum]];
	nextPS = &next->pss[next->lcIndex[cg.cur_localClientNum]];

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = nextPS->bobCycle;
	if ( i < prevPS->bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prevPS->bobCycle + f * ( i - prevPS->bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prevPS->origin[i] + f * (nextPS->origin[i] - prevPS->origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle( 
				prevPS->viewangles[i], nextPS->viewangles[i], f );
		}
		out->velocity[i] = prevPS->velocity[i] + 
			f * (nextPS->velocity[i] - prevPS->velocity[i] );
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;

	if ( !cg_predictItems.integer ) {
		return;
	}
	if ( !BG_PlayerTouchesItem( &cg.cur_lc->predictedPlayerState, &cent->currentState, cg.time ) ) {
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( cgs.gametype, &cent->currentState, &cg.cur_lc->predictedPlayerState ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// Special case for flags.  
	// We don't predict touching our own flag
#ifdef MISSIONPACK
	if( cgs.gametype == GT_1FCTF ) {
		if( item->giTag != PW_NEUTRALFLAG ) {
			return;
		}
	}
#endif
	if( cgs.gametype == GT_CTF ) {
		if (cg.cur_lc->predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
			item->giTag == PW_REDFLAG)
			return;
		if (cg.cur_lc->predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
			item->giTag == PW_BLUEFLAG)
			return;
	}

	// grab it
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex , &cg.cur_lc->predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if it's a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		cg.cur_lc->predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if ( !cg.cur_lc->predictedPlayerState.ammo[ item->giTag ] ) {
			cg.cur_lc->predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.cur_lc->predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.cur_lc->predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.cur_lc->predictedPlayerState.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( !ent->bmodel ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		if ( cg.cur_lc->predictedPlayerState.capsule ) {
			trap_CM_CapsuleTrace( &trace, cg.cur_lc->predictedPlayerState.origin, cg.cur_lc->predictedPlayerState.origin,
					cg.cur_lc->predictedPlayerState.mins, cg.cur_lc->predictedPlayerState.maxs, cmodel, -1 );
		} else {
			trap_CM_BoxTrace( &trace, cg.cur_lc->predictedPlayerState.origin, cg.cur_lc->predictedPlayerState.origin,
					cg.cur_lc->predictedPlayerState.mins, cg.cur_lc->predictedPlayerState.maxs, cmodel, -1 );
		}

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.cur_lc->hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.cur_lc->predictedPlayerState, ent );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.cur_lc->predictedPlayerState.jumppad_frame != cg.cur_lc->predictedPlayerState.pmove_framecount ) {
		cg.cur_lc->predictedPlayerState.jumppad_frame = 0;
		cg.cur_lc->predictedPlayerState.jumppad_ent = 0;
	}
}



/*
=================
CG_PredictPlayerState

Generates cg.cur_lc->predictedPlayerState for the current cg.time
cg.cur_lc->predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	int			cmdNum, current;
	playerState_t	oldPlayerState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;

	cg.cur_lc->hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.cur_lc->validPPS ) {
		cg.cur_lc->validPPS = qtrue;
		cg.cur_lc->predictedPlayerState = *cg.cur_ps;
	}


	// demo playback just copies the moves
	if ( cg.demoPlayback || (cg.cur_ps->pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_InterpolatePlayerState( qtrue );
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.cur_lc->predictedPlayerState;
	if (cg.cur_lc->predictedPlayerState.capsule) {
		cg_pmove.trace = CG_TraceCapsule;
	} else {
		cg_pmove.trace = CG_Trace;
	}
	cg_pmove.pointcontents = CG_PointContents;
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.cur_lc->predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd, cg.cur_localClientNum );
	if ( oldestCmd.serverTime > cg.cur_ps->commandTime 
		&& oldestCmd.serverTime < cg.time ) {	// special check for map_restart
		if ( cg_showmiss.integer ) {
			CG_Printf ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd, cg.cur_localClientNum );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to 
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport
		&& cg.nextSnap->lcIndex[cg.cur_localClientNum] != -1) {
		cg.cur_lc->predictedPlayerState = cg.nextSnap->pss[cg.nextSnap->lcIndex[cg.cur_localClientNum]];
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.cur_lc->predictedPlayerState = *cg.cur_ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	// run cmds
	moved = qfalse;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd, cg.cur_localClientNum );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.cur_lc->predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.cur_lc->predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.cur_lc->predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t adjusted, new_angles;
				CG_AdjustPositionForMover( cg.cur_lc->predictedPlayerState.origin, 
				cg.cur_lc->predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted, cg.cur_lc->predictedPlayerState.viewangles, new_angles);

				if ( cg_showmiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						CG_Printf("prediction error\n");
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.cur_lc->predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.cur_lc->predictedError, f, cg.cur_lc->predictedError );
					} else {
						VectorClear( cg.cur_lc->predictedError );
					}
					VectorAdd( delta, cg.cur_lc->predictedError, cg.cur_lc->predictedError );
					cg.cur_lc->predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

		Pmove (&cg_pmove);

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.cur_lc->predictedPlayerState);
	}

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showmiss.integer ) {
			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.cur_lc->predictedPlayerState.origin, 
		cg.cur_lc->predictedPlayerState.groundEntityNum, 
		cg.physicsTime, cg.time, cg.cur_lc->predictedPlayerState.origin, cg.cur_lc->predictedPlayerState.viewangles,cg.cur_lc->predictedPlayerState.viewangles);

	if ( cg_showmiss.integer ) {
		if (cg.cur_lc->predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS) {
			CG_Printf("WARNING: dropped event\n");
		}
	}

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.cur_lc->predictedPlayerState, &oldPlayerState );

	if ( cg_showmiss.integer ) {
		if (cg.cur_lc->eventSequence > cg.cur_lc->predictedPlayerState.eventSequence) {
			CG_Printf("WARNING: double event\n");
			cg.cur_lc->eventSequence = cg.cur_lc->predictedPlayerState.eventSequence;
		}
	}
}



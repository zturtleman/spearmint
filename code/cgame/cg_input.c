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
// usercmd_t creation

#include "cg_local.h"

unsigned in_frameMsec;
int in_frameTime;

vmCvar_t	cg_freelook;

vmCvar_t	m_pitch;
vmCvar_t	m_yaw;
vmCvar_t	m_forward;
vmCvar_t	m_side;

vmCvar_t	cg_yawspeed[MAX_SPLITVIEW];
vmCvar_t	cg_pitchspeed[MAX_SPLITVIEW];

vmCvar_t	cg_anglespeedkey[MAX_SPLITVIEW];

vmCvar_t	cg_run[MAX_SPLITVIEW];

vmCvar_t	j_pitch[MAX_SPLITVIEW];
vmCvar_t	j_yaw[MAX_SPLITVIEW];
vmCvar_t	j_forward[MAX_SPLITVIEW];
vmCvar_t	j_side[MAX_SPLITVIEW];
vmCvar_t	j_up[MAX_SPLITVIEW];
vmCvar_t	j_pitch_axis[MAX_SPLITVIEW];
vmCvar_t	j_yaw_axis[MAX_SPLITVIEW];
vmCvar_t	j_forward_axis[MAX_SPLITVIEW];
vmCvar_t	j_side_axis[MAX_SPLITVIEW];
vmCvar_t	j_up_axis[MAX_SPLITVIEW];

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

typedef struct {
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame if both a down and up happened
	qboolean	active;			// current state
	qboolean	wasPressed;		// set when down, not cleared when up
} kbutton_t;

typedef struct
{
	kbutton_t	in_left, in_right, in_forward, in_back;
	kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
	kbutton_t	in_strafe, in_speed;
	kbutton_t	in_up, in_down;

	kbutton_t	in_buttons[15];

	qboolean	in_mlooking;
} clientInput_t;

clientInput_t cis[MAX_SPLITVIEW];

void IN_MLookDown( int localPlayerNum ) {
	cis[localPlayerNum].in_mlooking = qtrue;
}

void IN_MLookUp( int localPlayerNum ) {
	cis[localPlayerNum].in_mlooking = qfalse;
	if ( !cg_freelook.integer ) {
		IN_CenterView ( localPlayerNum );
	}
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	char	c[20];
	
	trap_Argv( 1, c, sizeof (c) );
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}
	
	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	trap_Argv( 2, c, sizeof (c) );
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	c[20];
	unsigned	uptime;
	
	trap_Argv( 1, c, sizeof (c) );
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	trap_Argv( 2, c, sizeof (c) );
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += in_frameMsec / 2;
	}

	b->active = qfalse;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( kbutton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = in_frameTime;
		} else {
			msec += in_frameTime - key->downtime;
		}
		key->downtime = in_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / in_frameMsec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}

	return val;
}



void IN_UpDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_up);}
void IN_UpUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_up);}
void IN_DownDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_down);}
void IN_DownUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_down);}
void IN_LeftDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_left);}
void IN_LeftUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_left);}
void IN_RightDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_right);}
void IN_RightUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_right);}
void IN_ForwardDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_forward);}
void IN_ForwardUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_forward);}
void IN_BackDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_back);}
void IN_BackUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_back);}
void IN_LookupDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_lookup);}
void IN_LookupUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_lookup);}
void IN_LookdownDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_lookdown);}
void IN_LookdownUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_lookdown);}
void IN_MoveleftDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_moveleft);}
void IN_MoveleftUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_moveleft);}
void IN_MoverightDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_moveright);}
void IN_MoverightUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_moveright);}

void IN_SpeedDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_speed);}
void IN_SpeedUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_speed);}
void IN_StrafeDown( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_strafe);}
void IN_StrafeUp( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_strafe);}

void IN_Button0Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[0]);}
void IN_Button0Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[0]);}
void IN_Button1Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[1]);}
void IN_Button1Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[1]);}
void IN_Button2Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[2]);}
void IN_Button2Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[2]);}
void IN_Button3Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[3]);}
void IN_Button3Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[3]);}
void IN_Button4Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[4]);}
void IN_Button4Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[4]);}
void IN_Button5Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[5]);}
void IN_Button5Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[5]);}
void IN_Button6Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[6]);}
void IN_Button6Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[6]);}
void IN_Button7Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[7]);}
void IN_Button7Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[7]);}
void IN_Button8Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[8]);}
void IN_Button8Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[8]);}
void IN_Button9Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[9]);}
void IN_Button9Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[9]);}
void IN_Button10Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[10]);}
void IN_Button10Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[10]);}
void IN_Button11Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[11]);}
void IN_Button11Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[11]);}
void IN_Button12Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[12]);}
void IN_Button12Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[12]);}
void IN_Button13Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[13]);}
void IN_Button13Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[13]);}
void IN_Button14Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[14]);}
void IN_Button14Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[14]);}

void IN_CenterView( int localPlayerNum ) {
	playerState_t *ps;

	if ( localPlayerNum < 0 || localPlayerNum >= MAX_SPLITVIEW || !cg.snap || cg.snap->lcIndex[localPlayerNum] == -1 ) {
		return;
	}

	ps = &cg.snap->pss[ cg.snap->lcIndex[ localPlayerNum ] ];

	cg.localClients[localPlayerNum].viewangles[PITCH] = -SHORT2ANGLE(ps->delta_angles[PITCH]);
}

//==========================================================================

/*
================
CG_AdjustAngles

Moves the local angle positions
================
*/
void CG_AdjustAngles( cglc_t *lc, clientInput_t *ci ) {
	float	speed;
	int		lcNum = lc - cg.localClients;
	
	if ( ci->in_speed.active ) {
		speed = 0.001 * cg.frametime * cg_anglespeedkey[lcNum].value;
	} else {
		speed = 0.001 * cg.frametime;
	}

	if ( !ci->in_strafe.active ) {
		lc->viewangles[YAW] -= speed*cg_yawspeed[lcNum].value*CL_KeyState (&ci->in_right);
		lc->viewangles[YAW] += speed*cg_yawspeed[lcNum].value*CL_KeyState (&ci->in_left);
	}

	lc->viewangles[PITCH] -= speed*cg_pitchspeed[lcNum].value * CL_KeyState (&ci->in_lookup);
	lc->viewangles[PITCH] += speed*cg_pitchspeed[lcNum].value * CL_KeyState (&ci->in_lookdown);
}

/*
================
CG_KeyMove

Sets the usercmd_t based on key states
================
*/
void CG_KeyMove( clientInput_t *ci, usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( ci->in_speed.active ^ cg_run[ci-cis].integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( ci->in_strafe.active ) {
		side += movespeed * CL_KeyState (&ci->in_right);
		side -= movespeed * CL_KeyState (&ci->in_left);
	}

	side += movespeed * CL_KeyState (&ci->in_moveright);
	side -= movespeed * CL_KeyState (&ci->in_moveleft);


	up += movespeed * CL_KeyState (&ci->in_up);
	up -= movespeed * CL_KeyState (&ci->in_down);

	forward += movespeed * CL_KeyState (&ci->in_forward);
	forward -= movespeed * CL_KeyState (&ci->in_back);

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
}


/*
=================
CG_JoystickMove
=================
*/
void CG_JoystickMove( cglc_t *lc, clientInput_t *ci, usercmd_t *cmd ) {
	float	anglespeed;
	int		lcNum = lc - cg.localClients;

	if ( !(ci->in_speed.active ^ cg_run[lcNum].integer) ) {
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( ci->in_speed.active ) {
		anglespeed = 0.001 * cg.frametime * cg_anglespeedkey[lcNum].value;
	} else {
		anglespeed = 0.001 * cg.frametime;
	}

	if ( !ci->in_strafe.active ) {
		lc->viewangles[YAW] += anglespeed * j_yaw[lcNum].value * lc->joystickAxis[j_yaw_axis[lcNum].integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_side[lcNum].value * lc->joystickAxis[j_side_axis[lcNum].integer]) );
	} else {
		lc->viewangles[YAW] += anglespeed * j_side[lcNum].value * lc->joystickAxis[j_side_axis[lcNum].integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_yaw[lcNum].value * lc->joystickAxis[j_yaw_axis[lcNum].integer]) );
	}

	if ( ci->in_mlooking ) {
		lc->viewangles[PITCH] += anglespeed * j_forward[lcNum].value * lc->joystickAxis[j_forward_axis[lcNum].integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_pitch[lcNum].value * lc->joystickAxis[j_pitch_axis[lcNum].integer]) );
	} else {
		lc->viewangles[PITCH] += anglespeed * j_pitch[lcNum].value * lc->joystickAxis[j_pitch_axis[lcNum].integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_forward[lcNum].value * lc->joystickAxis[j_forward_axis[lcNum].integer]) );
	}

	cmd->upmove = ClampChar( cmd->upmove + (int) (j_up[lcNum].value * lc->joystickAxis[j_up_axis[lcNum].integer]) );
}

/*
=================
CG_MouseMove
=================
*/
void CG_MouseMove(cglc_t *lc, clientInput_t *ci, usercmd_t *cmd, float mx, float my)
{
	// ingame FOV
	mx *= lc->zoomSensitivity;
	my *= lc->zoomSensitivity;

	// add mouse X/Y movement to cmd
	if(ci->in_strafe.active)
	{
		cmd->rightmove = ClampChar(cmd->rightmove + m_side.value * mx);
	}
	else
		lc->viewangles[YAW] -= m_yaw.value * mx;

	if ((ci->in_mlooking || cg_freelook.integer) && !ci->in_strafe.active)
		lc->viewangles[PITCH] += m_pitch.value * my;
	else
		cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward.value * my);
}

/*
==============
CG_CmdButtons
==============
*/
void CG_CmdButtons( clientInput_t *ci, usercmd_t *cmd ) {
	int		i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//	
	for (i = 0 ; i < 15 ; i++) {
		if ( ci->in_buttons[i].active || ci->in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		ci->in_buttons[i].wasPressed = qfalse;
	}
}

/*
==============
CG_FinishMove
==============
*/
void CG_FinishMove( cglc_t *lc, usercmd_t *cmd ) {
	int i;

	cmd->stateValue = BG_ComposeUserCmdValue( lc->weaponSelect );

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(lc->viewangles[i]);
	}
}

/*
=================
CG_CreateUserCmd
=================
*/
usercmd_t *CG_CreateUserCmd( int localClientNum, int frameTime, unsigned frameMsec, float mx, float my ) {
	static usercmd_t cmd;
	vec3_t		oldAngles;
	cglc_t		*lc;
	clientInput_t *ci;

	in_frameTime = frameTime;
	in_frameMsec = frameMsec;

	lc = &cg.localClients[localClientNum];
	ci = &cis[localClientNum];

	VectorCopy( lc->viewangles, oldAngles );

	// keyboard angle adjustment
	CG_AdjustAngles(lc, ci);

	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CG_CmdButtons( ci, &cmd );

	// get basic movement from keyboard
	CG_KeyMove( ci, &cmd );

	// get basic movement from mouse
	CG_MouseMove( lc, ci, &cmd, mx, my );

	// get basic movement from joystick
	CG_JoystickMove( lc, ci, &cmd );

	// check to make sure the angles haven't wrapped
	if ( lc->viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		lc->viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - lc->viewangles[PITCH] > 90 ) {
		lc->viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// store out the final values
	CG_FinishMove( lc, &cmd );

	return &cmd;
}

/*
=================
CG_RegisterInputCvars
=================
*/
void CG_RegisterInputCvars( void ) {
	int i;

	trap_Cvar_Register( &cg_freelook, "cl_freelook", "1", CVAR_ARCHIVE );

	trap_Cvar_Register( &m_pitch, "m_pitch", "0.022", CVAR_ARCHIVE );
	trap_Cvar_Register( &m_yaw, "m_yaw", "0.022", CVAR_ARCHIVE );
	trap_Cvar_Register( &m_forward, "m_forward", "0.25", CVAR_ARCHIVE );
	trap_Cvar_Register( &m_side, "m_side", "0.25", CVAR_ARCHIVE );

	for (i = 0; i < CG_MaxSplitView(); i++) {
		trap_Cvar_Register( &cg_yawspeed[i], Com_LocalClientCvarName(i, "cl_yawspeed"), "140", CVAR_ARCHIVE );
		trap_Cvar_Register( &cg_pitchspeed[i], Com_LocalClientCvarName(i, "cl_pitchspeed"), "140", CVAR_ARCHIVE );
		trap_Cvar_Register( &cg_anglespeedkey[i], Com_LocalClientCvarName(i, "cl_anglespeedkey"), "1.5", 0 );
		trap_Cvar_Register( &cg_run[i], Com_LocalClientCvarName(i, "cl_run"), "1", CVAR_ARCHIVE );

		trap_Cvar_Register( &j_pitch[i],	Com_LocalClientCvarName(i, "j_pitch"),        "0.022",	CVAR_ARCHIVE );
		trap_Cvar_Register( &j_yaw[i],		Com_LocalClientCvarName(i, "j_yaw"),          "-0.022",	CVAR_ARCHIVE );
		trap_Cvar_Register( &j_forward[i],	Com_LocalClientCvarName(i, "j_forward"),      "-0.25",	CVAR_ARCHIVE );
		trap_Cvar_Register( &j_side[i],		Com_LocalClientCvarName(i, "j_side"),         "0.25",	CVAR_ARCHIVE );
		trap_Cvar_Register( &j_up[i],		Com_LocalClientCvarName(i, "j_up"),           "1",		CVAR_ARCHIVE );

		trap_Cvar_Register( &j_pitch_axis[i],	Com_LocalClientCvarName(i, "j_pitch_axis"),   "3", CVAR_ARCHIVE );
		trap_Cvar_Register( &j_yaw_axis[i],		Com_LocalClientCvarName(i, "j_yaw_axis"),     "4", CVAR_ARCHIVE );
		trap_Cvar_Register( &j_forward_axis[i],	Com_LocalClientCvarName(i, "j_forward_axis"), "1", CVAR_ARCHIVE );
		trap_Cvar_Register( &j_side_axis[i],	Com_LocalClientCvarName(i, "j_side_axis"),    "0", CVAR_ARCHIVE );
		trap_Cvar_Register( &j_up_axis[i],		Com_LocalClientCvarName(i, "j_up_axis"),      "2", CVAR_ARCHIVE );

		trap_Cvar_CheckRange( Com_LocalClientCvarName(i, "j_pitch_axis"), 0, MAX_JOYSTICK_AXIS-1, qtrue );
		trap_Cvar_CheckRange( Com_LocalClientCvarName(i, "j_yaw_axis"), 0, MAX_JOYSTICK_AXIS-1, qtrue );
		trap_Cvar_CheckRange( Com_LocalClientCvarName(i, "j_forward_axis"), 0, MAX_JOYSTICK_AXIS-1, qtrue );
		trap_Cvar_CheckRange( Com_LocalClientCvarName(i, "j_side_axis"), 0, MAX_JOYSTICK_AXIS-1, qtrue );
		trap_Cvar_CheckRange( Com_LocalClientCvarName(i, "j_up_axis"), 0, MAX_JOYSTICK_AXIS-1, qtrue );
	}
}

/*
=================
CG_UpdateInputCvars
=================
*/
void CG_UpdateInputCvars( void ) {
	int i;

	trap_Cvar_Update( &cg_freelook );

	trap_Cvar_Update( &m_pitch );
	trap_Cvar_Update( &m_yaw );
	trap_Cvar_Update( &m_forward );
	trap_Cvar_Update( &m_side );

	for (i = 0; i < CG_MaxSplitView(); i++) {
		trap_Cvar_Update( &cg_yawspeed[i] );
		trap_Cvar_Update( &cg_pitchspeed[i] );
		trap_Cvar_Update( &cg_anglespeedkey[i] );
		trap_Cvar_Update( &cg_run[i] );

		trap_Cvar_Update( &j_pitch[i] );
		trap_Cvar_Update( &j_yaw[i] );
		trap_Cvar_Update( &j_forward[i] );
		trap_Cvar_Update( &j_side[i] );
		trap_Cvar_Update( &j_up[i] );

		trap_Cvar_Update( &j_pitch_axis[i] );
		trap_Cvar_Update( &j_yaw_axis[i] );
		trap_Cvar_Update( &j_forward_axis[i] );
		trap_Cvar_Update( &j_side_axis[i] );
		trap_Cvar_Update( &j_up_axis[i] );
	}
}


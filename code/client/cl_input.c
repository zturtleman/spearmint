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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

unsigned	frame_msec;
int			old_com_frameTime;

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


typedef struct
{
	kbutton_t	in_left, in_right, in_forward, in_back;
	kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
	kbutton_t	in_strafe, in_speed;
	kbutton_t	in_up, in_down;

	kbutton_t	in_buttons[16];

	qboolean	in_mlooking;
} clientInput_t;

clientInput_t cis[CL_MAX_SPLITVIEW];

#ifdef USE_VOIP
kbutton_t	in_voiprecord;
#endif

void IN_MLookDown( int localPlayerNum ) {
	cis[localPlayerNum].in_mlooking = qtrue;
}

void IN_MLookUp( int localPlayerNum ) {
	cis[localPlayerNum].in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView ( localPlayerNum );
	}
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
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
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
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
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
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
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
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

#ifdef USE_VOIP
void IN_VoipRecordDown(void)
{
	IN_KeyDown(&in_voiprecord);
	Cvar_Set("cl_voipSend", "1");
}

void IN_VoipRecordUp(void)
{
	IN_KeyUp(&in_voiprecord);
	Cvar_Set("cl_voipSend", "0");
}
#endif

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
void IN_Button15Down( int localPlayerNum ) {IN_KeyDown(&cis[localPlayerNum].in_buttons[15]);}
void IN_Button15Up( int localPlayerNum ) {IN_KeyUp(&cis[localPlayerNum].in_buttons[15]);}

void IN_CenterView( int localClientNum ) {
	sharedPlayerState_t *ps;

	if (localClientNum < 0 || localClientNum >= CL_MAX_SPLITVIEW || !cl.snap.valid || cl.snap.lcIndex[localClientNum] == -1) {
		return;
	}

	ps = DA_ElementPointer(cl.snap.playerStates, cl.snap.lcIndex[localClientNum]);
	if ( !ps ) {
		return;
	}

	cl.localClients[localClientNum].viewangles[PITCH] = -SHORT2ANGLE(ps->delta_angles[PITCH]);
}


//==========================================================================

cvar_t	*cl_yawspeed[CL_MAX_SPLITVIEW];
cvar_t	*cl_pitchspeed[CL_MAX_SPLITVIEW];

cvar_t	*cl_anglespeedkey[CL_MAX_SPLITVIEW];

cvar_t	*cl_run[CL_MAX_SPLITVIEW];

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( calc_t *lc, clientInput_t *ci ) {
	float	speed;
	int		lcNum = lc - cl.localClients;
	
	if ( ci->in_speed.active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey[lcNum]->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !ci->in_strafe.active ) {
		lc->viewangles[YAW] -= speed*cl_yawspeed[lcNum]->value*CL_KeyState (&ci->in_right);
		lc->viewangles[YAW] += speed*cl_yawspeed[lcNum]->value*CL_KeyState (&ci->in_left);
	}

	lc->viewangles[PITCH] -= speed*cl_pitchspeed[lcNum]->value * CL_KeyState (&ci->in_lookup);
	lc->viewangles[PITCH] += speed*cl_pitchspeed[lcNum]->value * CL_KeyState (&ci->in_lookdown);
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( clientInput_t *ci, usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( ci->in_speed.active ^ cl_run[ci-cis]->integer ) {
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
CL_MouseEvent
=================
*/
void CL_MouseEvent( int localClientNum, int dx, int dy, int time ) {
	calc_t *lc;

	if ( localClientNum < 0 || localClientNum >= CL_MAX_SPLITVIEW) {
		return;
	}

	if ( Key_GetCatcher( ) & KEYCATCH_UI ) {
		VM_Call(uivm, UI_MOUSE_EVENT, localClientNum, dx, dy);
	} else if (Key_GetCatcher( ) & KEYCATCH_CGAME) {
		VM_Call(cgvm, CG_MOUSE_EVENT, localClientNum, dx, dy);
	} else {
		lc = &cl.localClients[localClientNum];
		lc->mouseDx[lc->mouseIndex] += dx;
		lc->mouseDy[lc->mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( int localClientNum, int axis, int value, int time ) {
	if ( localClientNum < 0 || localClientNum >= CL_MAX_SPLITVIEW) {
		return;
	}
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.localClients[localClientNum].joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( calc_t *lc, clientInput_t *ci, usercmd_t *cmd ) {
	float	anglespeed;
	int		lcNum = lc - cl.localClients;

	if ( !(ci->in_speed.active ^ cl_run[lcNum]->integer) ) {
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( ci->in_speed.active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey[lcNum]->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

	if ( !ci->in_strafe.active ) {
		lc->viewangles[YAW] += anglespeed * j_yaw[lcNum]->value * lc->joystickAxis[j_yaw_axis[lcNum]->integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_side[lcNum]->value * lc->joystickAxis[j_side_axis[lcNum]->integer]) );
	} else {
		lc->viewangles[YAW] += anglespeed * j_side[lcNum]->value * lc->joystickAxis[j_side_axis[lcNum]->integer];
		cmd->rightmove = ClampChar( cmd->rightmove + (int) (j_yaw[lcNum]->value * lc->joystickAxis[j_yaw_axis[lcNum]->integer]) );
	}

	if ( ci->in_mlooking ) {
		lc->viewangles[PITCH] += anglespeed * j_forward[lcNum]->value * lc->joystickAxis[j_forward_axis[lcNum]->integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_pitch[lcNum]->value * lc->joystickAxis[j_pitch_axis[lcNum]->integer]) );
	} else {
		lc->viewangles[PITCH] += anglespeed * j_pitch[lcNum]->value * lc->joystickAxis[j_pitch_axis[lcNum]->integer];
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int) (j_forward[lcNum]->value * lc->joystickAxis[j_forward_axis[lcNum]->integer]) );
	}

	cmd->upmove = ClampChar( cmd->upmove + (int) (j_up[lcNum]->value * lc->joystickAxis[j_up_axis[lcNum]->integer]) );
}

/*
=================
CL_MouseMove
=================
*/

void CL_MouseMove(calc_t *lc, clientInput_t *ci, usercmd_t *cmd)
{
	float mx, my;

	// allow mouse smoothing
	if (m_filter->integer)
	{
		mx = (lc->mouseDx[0] + lc->mouseDx[1]) * 0.5f;
		my = (lc->mouseDy[0] + lc->mouseDy[1]) * 0.5f;
	}
	else
	{
		mx = lc->mouseDx[lc->mouseIndex];
		my = lc->mouseDy[lc->mouseIndex];
	}

	lc->mouseIndex ^= 1;
	lc->mouseDx[lc->mouseIndex] = 0;
	lc->mouseDy[lc->mouseIndex] = 0;

	if (mx == 0.0f && my == 0.0f)
		return;
	
	if (cl_mouseAccel->value != 0.0f)
	{
		if(cl_mouseAccelStyle->integer == 0)
		{
			float accelSensitivity;
			float rate;
			
			rate = sqrt(mx * mx + my * my) / (float) frame_msec;

			accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
			mx *= accelSensitivity;
			my *= accelSensitivity;
			
			if(cl_showMouseRate->integer)
				Com_Printf("rate: %f, accelSensitivity: %f\n", rate, accelSensitivity);
		}
		else
		{
			float rate[2];
			float power[2];

			// sensitivity remains pretty much unchanged at low speeds
			// cl_mouseAccel is a power value to how the acceleration is shaped
			// cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
			// NOTE: decouple the config cvars for independent acceleration setup along X and Y?

			rate[0] = fabs(mx) / (float) frame_msec;
			rate[1] = fabs(my) / (float) frame_msec;
			power[0] = powf(rate[0] / cl_mouseAccelOffset->value, cl_mouseAccel->value);
			power[1] = powf(rate[1] / cl_mouseAccelOffset->value, cl_mouseAccel->value);

			mx = cl_sensitivity->value * (mx + ((mx < 0) ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
			my = cl_sensitivity->value * (my + ((my < 0) ? -power[1] : power[1]) * cl_mouseAccelOffset->value);

			if(cl_showMouseRate->integer)
				Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1]);
		}
	}
	else
	{
		mx *= cl_sensitivity->value;
		my *= cl_sensitivity->value;
	}

	// ingame FOV
	mx *= lc->cgameSensitivity;
	my *= lc->cgameSensitivity;

	// add mouse X/Y movement to cmd
	if(ci->in_strafe.active)
	{
		cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
	}
	else
		lc->viewangles[YAW] -= m_yaw->value * mx;

	if ((ci->in_mlooking || cl_freelook->integer) && !ci->in_strafe.active)
		lc->viewangles[PITCH] += m_pitch->value * my;
	else
		cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward->value * my);
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( clientInput_t *ci, usercmd_t *cmd ) {
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

	if ( Key_GetCatcher( ) ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( anykeydown && Key_GetCatcher( ) == 0 ) {
		cmd->buttons |= BUTTON_ANY;
	}
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( calc_t *lc, usercmd_t *cmd ) {
	int		i;

	// copy the state that the cgame is currently sending
	cmd->stateValue = lc->cgameUserCmdValue;

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(lc->viewangles[i]);
	}
}


/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( int localClientNum ) {
	usercmd_t	cmd;
	vec3_t		oldAngles;
	calc_t		*lc;
	clientInput_t *ci;

	lc = &cl.localClients[localClientNum];
	ci = &cis[localClientNum];

	VectorCopy( lc->viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles(lc, ci);
	
	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( ci, &cmd );

	// get basic movement from keyboard
	CL_KeyMove( ci, &cmd );

	// get basic movement from mouse
	CL_MouseMove( lc, ci, &cmd );

	// get basic movement from joystick
	CL_JoystickMove( lc, ci, &cmd );

	// check to make sure the angles haven't wrapped
	if ( lc->viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		lc->viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - lc->viewangles[PITCH] > 90 ) {
		lc->viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// store out the final values
	CL_FinishMove( lc, &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs(lc->viewangles[YAW] - oldAngles[YAW]) );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs(lc->viewangles[PITCH] - oldAngles[PITCH]) );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	int			i;
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( clc.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & CMD_MASK;

	for (i = 0; i < CL_MAX_SPLITVIEW; i++) {
		if (cl.snap.valid && cl.snap.lcIndex[i] == -1) {
			continue;
		}
		cl.cmdss[i][cmdNum] = CL_CreateCmd(i);
	}
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || clc.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( clc.state != CA_ACTIVE && 
		clc.state != CA_PRIMED && 
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( cl_lanForcePackets->integer && Sys_IsLANAddress( clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 125 ) {
		Cvar_Set( "cl_maxpackets", "125" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	local client bits
1	command count
<local clients * count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;
	int			lc, localClientBits;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || clc.state == CA_CINEMATIC ) {
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}

#ifdef USE_VOIP
	if (clc.voipOutgoingDataSize > 0)
	{
		// ZTM: TODO: Allow each local player to use voip, or at least allow using voip when player 1 isn't present.
		const int voipLocalClientNum = 0;

		if(clc.clientNums[voipLocalClientNum] != -1 && ((clc.voipFlags & VOIP_SPATIAL) || Com_IsVoipTarget(clc.voipTargets, sizeof(clc.voipTargets), -1)))
		{
			MSG_WriteByte (&buf, clc_voip);
			MSG_WriteByte (&buf, voipLocalClientNum);
			MSG_WriteByte (&buf, clc.voipOutgoingGeneration);
			MSG_WriteLong (&buf, clc.voipOutgoingSequence);
			MSG_WriteByte (&buf, clc.voipOutgoingDataFrames);
			MSG_WriteData (&buf, clc.voipTargets, sizeof(clc.voipTargets));
			MSG_WriteByte(&buf, clc.voipFlags);
			MSG_WriteShort (&buf, clc.voipOutgoingDataSize);
			MSG_WriteData (&buf, clc.voipOutgoingData, clc.voipOutgoingDataSize);

			// If we're recording a demo, we have to fake a server packet with
			//  this VoIP data so it gets to disk; the server doesn't send it
			//  back to us, and we might as well eliminate concerns about dropped
			//  and misordered packets here.
			if(clc.demorecording && !clc.demowaiting)
			{
				const int voipSize = clc.voipOutgoingDataSize;
				msg_t fakemsg;
				byte fakedata[MAX_MSGLEN];
				MSG_Init (&fakemsg, fakedata, sizeof (fakedata));
				MSG_Bitstream (&fakemsg);
				MSG_WriteLong (&fakemsg, clc.reliableAcknowledge);
				MSG_WriteByte (&fakemsg, svc_voip);
				MSG_WriteShort (&fakemsg, clc.clientNums[voipLocalClientNum]);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingGeneration);
				MSG_WriteLong (&fakemsg, clc.voipOutgoingSequence);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingDataFrames);
				MSG_WriteShort (&fakemsg, clc.voipOutgoingDataSize );
				MSG_WriteBits (&fakemsg, clc.voipFlags, VOIP_FLAGCNT);
				MSG_WriteData (&fakemsg, clc.voipOutgoingData, voipSize);
				MSG_WriteByte (&fakemsg, svc_EOF);
				CL_WriteDemoMessage (&fakemsg, 0);
			}

			clc.voipOutgoingSequence += clc.voipOutgoingDataFrames;
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
		else
		{
			// We have data, but no targets. Silently discard all data
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
	}
#endif

	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// set bits
		localClientBits = 0;
		for (lc = 0; lc < MAX_SPLITVIEW; lc++) {
			if ( cl.snap.valid && cl.snap.lcIndex[lc] == -1 ) {
				continue;
			}
			localClientBits |= (1<<lc);
		}

		// write the local client bits
		MSG_WriteByte( &buf, localClientBits );

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the message acknowledge in the key
		key = clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= MSG_HashKey(clc.serverCommands[ clc.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		for (lc = 0; lc < MAX_SPLITVIEW; lc++) {
			if (!(localClientBits & (1<<lc))) {
				continue;
			}

			Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
			oldcmd = &nullcmd;

			// write all the commands, including the predicted command
			for ( i = 0 ; i < count ; i++ ) {
				j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
				cmd = &cl.cmdss[lc][j];
				MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
				oldcmd = cmd;
			}
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);	
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( clc.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

/*
============
CL_AddPlayerCommand
============
*/
void CL_AddPlayerCommand( const char *cmd_name, icommand_t function ) {
	int i;

	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		Cmd_AddIntCommand( Com_LocalClientCvarName(i, cmd_name), function, i );
	}
}

/*
============
CL_RemovePlayerCommand
============
*/
void CL_RemovePlayerCommand( const char *cmd_name ) {
	int i;

	for ( i = 0; i < CL_MAX_SPLITVIEW; i++ ) {
		Cmd_RemoveCommand( Com_LocalClientCvarName( i, cmd_name ) );
	}
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	CL_AddPlayerCommand ("centerview",IN_CenterView);

	CL_AddPlayerCommand ("+moveup",IN_UpDown);
	CL_AddPlayerCommand ("-moveup",IN_UpUp);
	CL_AddPlayerCommand ("+movedown",IN_DownDown);
	CL_AddPlayerCommand ("-movedown",IN_DownUp);
	CL_AddPlayerCommand ("+left",IN_LeftDown);
	CL_AddPlayerCommand ("-left",IN_LeftUp);
	CL_AddPlayerCommand ("+right",IN_RightDown);
	CL_AddPlayerCommand ("-right",IN_RightUp);
	CL_AddPlayerCommand ("+forward",IN_ForwardDown);
	CL_AddPlayerCommand ("-forward",IN_ForwardUp);
	CL_AddPlayerCommand ("+back",IN_BackDown);
	CL_AddPlayerCommand ("-back",IN_BackUp);
	CL_AddPlayerCommand ("+lookup", IN_LookupDown);
	CL_AddPlayerCommand ("-lookup", IN_LookupUp);
	CL_AddPlayerCommand ("+lookdown", IN_LookdownDown);
	CL_AddPlayerCommand ("-lookdown", IN_LookdownUp);
	CL_AddPlayerCommand ("+strafe", IN_StrafeDown);
	CL_AddPlayerCommand ("-strafe", IN_StrafeUp);
	CL_AddPlayerCommand ("+moveleft", IN_MoveleftDown);
	CL_AddPlayerCommand ("-moveleft", IN_MoveleftUp);
	CL_AddPlayerCommand ("+moveright", IN_MoverightDown);
	CL_AddPlayerCommand ("-moveright", IN_MoverightUp);
	CL_AddPlayerCommand ("+speed", IN_SpeedDown);
	CL_AddPlayerCommand ("-speed", IN_SpeedUp);
	CL_AddPlayerCommand ("+button0", IN_Button0Down);
	CL_AddPlayerCommand ("-button0", IN_Button0Up);
	CL_AddPlayerCommand ("+button1", IN_Button1Down);
	CL_AddPlayerCommand ("-button1", IN_Button1Up);
	CL_AddPlayerCommand ("+button2", IN_Button2Down);
	CL_AddPlayerCommand ("-button2", IN_Button2Up);
	CL_AddPlayerCommand ("+button3", IN_Button3Down);
	CL_AddPlayerCommand ("-button3", IN_Button3Up);
	CL_AddPlayerCommand ("+button4", IN_Button4Down);
	CL_AddPlayerCommand ("-button4", IN_Button4Up);
	CL_AddPlayerCommand ("+button5", IN_Button5Down);
	CL_AddPlayerCommand ("-button5", IN_Button5Up);
	CL_AddPlayerCommand ("+button6", IN_Button6Down);
	CL_AddPlayerCommand ("-button6", IN_Button6Up);
	CL_AddPlayerCommand ("+button7", IN_Button7Down);
	CL_AddPlayerCommand ("-button7", IN_Button7Up);
	CL_AddPlayerCommand ("+button8", IN_Button8Down);
	CL_AddPlayerCommand ("-button8", IN_Button8Up);
	CL_AddPlayerCommand ("+button9", IN_Button9Down);
	CL_AddPlayerCommand ("-button9", IN_Button9Up);
	CL_AddPlayerCommand ("+button10", IN_Button10Down);
	CL_AddPlayerCommand ("-button10", IN_Button10Up);
	CL_AddPlayerCommand ("+button11", IN_Button11Down);
	CL_AddPlayerCommand ("-button11", IN_Button11Up);
	CL_AddPlayerCommand ("+button12", IN_Button12Down);
	CL_AddPlayerCommand ("-button12", IN_Button12Up);
	CL_AddPlayerCommand ("+button13", IN_Button13Down);
	CL_AddPlayerCommand ("-button13", IN_Button13Up);
	CL_AddPlayerCommand ("+button14", IN_Button14Down);
	CL_AddPlayerCommand ("-button14", IN_Button14Up);
	CL_AddPlayerCommand ("+mlook", IN_MLookDown);
	CL_AddPlayerCommand ("-mlook", IN_MLookUp);

#ifdef USE_VOIP
	Cmd_AddCommand ("+voiprecord", IN_VoipRecordDown);
	Cmd_AddCommand ("-voiprecord", IN_VoipRecordUp);
#endif

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
}

/*
============
CL_ShutdownInput
============
*/
void CL_ShutdownInput(void)
{
	CL_RemovePlayerCommand("centerview");

	CL_RemovePlayerCommand("+moveup");
	CL_RemovePlayerCommand("-moveup");
	CL_RemovePlayerCommand("+movedown");
	CL_RemovePlayerCommand("-movedown");
	CL_RemovePlayerCommand("+left");
	CL_RemovePlayerCommand("-left");
	CL_RemovePlayerCommand("+right");
	CL_RemovePlayerCommand("-right");
	CL_RemovePlayerCommand("+forward");
	CL_RemovePlayerCommand("-forward");
	CL_RemovePlayerCommand("+back");
	CL_RemovePlayerCommand("-back");
	CL_RemovePlayerCommand("+lookup");
	CL_RemovePlayerCommand("-lookup");
	CL_RemovePlayerCommand("+lookdown");
	CL_RemovePlayerCommand("-lookdown");
	CL_RemovePlayerCommand("+strafe");
	CL_RemovePlayerCommand("-strafe");
	CL_RemovePlayerCommand("+moveleft");
	CL_RemovePlayerCommand("-moveleft");
	CL_RemovePlayerCommand("+moveright");
	CL_RemovePlayerCommand("-moveright");
	CL_RemovePlayerCommand("+speed");
	CL_RemovePlayerCommand("-speed");
	CL_RemovePlayerCommand("+button0");
	CL_RemovePlayerCommand("-button0");
	CL_RemovePlayerCommand("+button1");
	CL_RemovePlayerCommand("-button1");
	CL_RemovePlayerCommand("+button2");
	CL_RemovePlayerCommand("-button2");
	CL_RemovePlayerCommand("+button3");
	CL_RemovePlayerCommand("-button3");
	CL_RemovePlayerCommand("+button4");
	CL_RemovePlayerCommand("-button4");
	CL_RemovePlayerCommand("+button5");
	CL_RemovePlayerCommand("-button5");
	CL_RemovePlayerCommand("+button6");
	CL_RemovePlayerCommand("-button6");
	CL_RemovePlayerCommand("+button7");
	CL_RemovePlayerCommand("-button7");
	CL_RemovePlayerCommand("+button8");
	CL_RemovePlayerCommand("-button8");
	CL_RemovePlayerCommand("+button9");
	CL_RemovePlayerCommand("-button9");
	CL_RemovePlayerCommand("+button10");
	CL_RemovePlayerCommand("-button10");
	CL_RemovePlayerCommand("+button11");
	CL_RemovePlayerCommand("-button11");
	CL_RemovePlayerCommand("+button12");
	CL_RemovePlayerCommand("-button12");
	CL_RemovePlayerCommand("+button13");
	CL_RemovePlayerCommand("-button13");
	CL_RemovePlayerCommand("+button14");
	CL_RemovePlayerCommand("-button14");
	CL_RemovePlayerCommand("+mlook");
	CL_RemovePlayerCommand("-mlook");

#ifdef USE_VOIP
	Cmd_RemoveCommand("+voiprecord");
	Cmd_RemoveCommand("-voiprecord");
#endif
}

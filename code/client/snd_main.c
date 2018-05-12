/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

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

#include "client.h"
#include "snd_codec.h"
#include "snd_local.h"
#include "snd_public.h"

cvar_t *s_volume;
cvar_t *s_muted;
cvar_t *s_musicVolume;
cvar_t *s_doppler;
cvar_t *s_backend;
cvar_t *s_muteWhenMinimized;
cvar_t *s_muteWhenUnfocused;

static soundInterface_t si;

listener_t listeners[MAX_LISTENERS];

/*
=================
S_ListenersInit
=================
*/
void S_ListenersInit(void) {
	int i;

	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		listeners[i].valid = qfalse;
		listeners[i].updated = qfalse;

		listeners[i].number = -1;
		VectorClear(listeners[i].origin);
		AxisClear(listeners[i].axis);
		listeners[i].inwater = 0;
		listeners[i].firstPerson = qfalse;
	}
}

/*
=================
S_ListenersEndFrame
=================
*/
void S_ListenersEndFrame(void) {
	int i;

	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		// Didn't receive update for this listener this frame, so free it.
		if (!listeners[i].updated)
			listeners[i].valid = qfalse;

		listeners[i].updated = qfalse;
	}
}

/*
=================
S_HearingThroughEntity
=================
*/
qboolean S_HearingThroughEntity( int entityNum )
{
	int i;

	// Note: Listeners may be one frame out of date.
	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid && listeners[i].number == entityNum)
		{
			return listeners[i].firstPerson;
		}
	}

	return qfalse;
}

/*
====================
S_EntityIsListener
====================
*/
qboolean S_EntityIsListener(int entityNum)
{
	int i;

	// NOTE: Listeners may be one frame out of date.
	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid && listeners[i].number == entityNum)
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
====================
S_ClosestListener
====================
*/
int S_ClosestListener(const vec3_t origin) {
	float dist, closestDist = INT_MAX;
	int closestListener = -1;
	int i;

	// NOTE: Listeners may be one frame out of date.
	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid)
		{
			dist = Distance(origin, listeners[i].origin);
			if (dist < closestDist) {
				closestDist = dist;
				closestListener = i;
			}
		}
	}

	return closestListener;
}

/*
====================
S_ListenersClosestDistance
====================
*/
float S_ListenersClosestDistance(const vec3_t origin) {
	float dist, closestDist = INT_MAX;
	int i;

	// NOTE: Listeners may be one frame out of date.
	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid)
		{
			dist = Distance(origin, listeners[i].origin);
			if (dist < closestDist) {
				closestDist = dist;
			}
		}
	}
	
	return closestDist;
}

/*
====================
S_ListenersClosestDistanceSquared
====================
*/
float S_ListenersClosestDistanceSquared(const vec3_t origin) {
	float dist, closestDist = INT_MAX;
	int i;

	// NOTE: Listeners may be one frame out of date.
	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid)
		{
			dist = DistanceSquared(origin, listeners[i].origin);
			if (dist < closestDist) {
				closestDist = dist;
			}
		}
	}
	
	return closestDist;
}

/*
====================
S_ListenerNumForEntity
====================
*/
int S_ListenerNumForEntity(int entityNum, qboolean create)
{
	int i;
	int freeListener = -1;

	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].valid)
		{
			if (listeners[i].number == entityNum)
				return i;
		}
		else if (create && freeListener == -1)
			freeListener = i;
	}

	if (create && freeListener == -1)
	{
		// Find listener that might be freed next frame, otherwise we
		// could fail to get a slot when listener changes entityNums.
		for (i = MAX_LISTENERS-1; i >= 0; --i)
		{
			if (listeners[i].valid && !listeners[i].updated)
			{
				freeListener = i;
				break;
			}
		}
	}

	return freeListener;
}

/*
=================
S_UpdateListener
=================
*/
void S_UpdateListener(int entityNum, const vec3_t origin, const vec3_t axis[3], int inwater, qboolean firstPerson)
{
	int listener;

	// Get listener for entityNum.
	listener = S_ListenerNumForEntity(entityNum, qtrue);

	if (listener < 0 || listener >= MAX_LISTENERS)
		return;

	listeners[listener].valid = qtrue;
	listeners[listener].updated = qtrue;

	// Update listener info.
	listeners[listener].number = entityNum;
	VectorCopy(origin, listeners[listener].origin);
	VectorCopy(axis[0], listeners[listener].axis[0]);
	VectorCopy(axis[1], listeners[listener].axis[1]);
	VectorCopy(axis[2], listeners[listener].axis[2]);
	listeners[listener].inwater = inwater;
	listeners[listener].firstPerson = firstPerson;
}

/*
=================
S_NumUpdatedListeners

Returns the number of listeners updated this frame.

Listeners are valid for one frame after they're added, so they can carry over
between frames. Therefore this function has very limited usefulness.
=================
*/
int S_NumUpdatedListeners( void ) {
	int i;
	int numListeners = 0;

	for (i = 0; i < MAX_LISTENERS; ++i)
	{
		if (listeners[i].updated)
		{
			numListeners++;
		}
	}

	return numListeners;
}

/*
=================
S_ValidateInterface
=================
*/
static qboolean S_ValidSoundInterface( soundInterface_t *si )
{
	if( !si->Shutdown ) return qfalse;
	if( !si->StartSound ) return qfalse;
	if( !si->StartLocalSound ) return qfalse;
	if( !si->StartBackgroundTrack ) return qfalse;
	if( !si->StopBackgroundTrack ) return qfalse;
	if( !si->StartStreamingSound ) return qfalse;
	if( !si->StopStreamingSound ) return qfalse;
	if( !si->QueueStreamingSound ) return qfalse;
	if( !si->GetStreamPlayCount ) return qfalse;
	if( !si->SetStreamVolume ) return qfalse;
	if( !si->RawSamples ) return qfalse;
	if( !si->StopAllSounds ) return qfalse;
	if( !si->ClearLoopingSounds ) return qfalse;
	if( !si->AddLoopingSound ) return qfalse;
	if( !si->AddRealLoopingSound ) return qfalse;
	if( !si->StopLoopingSound ) return qfalse;
	if( !si->Respatialize ) return qfalse;
	if( !si->UpdateEntityPosition ) return qfalse;
	if( !si->Update ) return qfalse;
	if( !si->DisableSounds ) return qfalse;
	if( !si->BeginRegistration ) return qfalse;
	if( !si->RegisterSound ) return qfalse;
	if( !si->SoundDuration ) return qfalse;
	if( !si->ClearSoundBuffer ) return qfalse;
	if( !si->SoundInfo ) return qfalse;
	if( !si->SoundList ) return qfalse;

#ifdef USE_VOIP
	if( !si->StartCapture ) return qfalse;
	if( !si->AvailableCaptureSamples ) return qfalse;
	if( !si->Capture ) return qfalse;
	if( !si->StopCapture ) return qfalse;
	if( !si->MasterGain ) return qfalse;
#endif

	return qtrue;
}

/*
=================
S_StartSound
=================
*/
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if( si.StartSound ) {
		si.StartSound( origin, entnum, entchannel, sfx );
	}
}

/*
=================
S_StartLocalSound
=================
*/
void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	if( si.StartLocalSound ) {
		si.StartLocalSound( sfx, channelNum );
	}
}

/*
=================
S_StartBackgroundTrack
=================
*/
void S_StartBackgroundTrack( const char *intro, const char *loop, float volume, float loopVolume )
{
	if( si.StartBackgroundTrack ) {
		si.StartBackgroundTrack( intro, loop, volume, loopVolume );
	}
}

/*
=================
S_StopBackgroundTrack
=================
*/
void S_StopBackgroundTrack( void )
{
	if( si.StopBackgroundTrack ) {
		si.StopBackgroundTrack( );
	}
}

/*
=================
S_StartStreamingSound
=================
*/
void S_StartStreamingSound( int stream, int entityNum, const char *filename, float volume )
{
	if(si.StartStreamingSound)
		si.StartStreamingSound(stream, entityNum, filename, volume);
}

/*
=================
S_StopStreamingSound
=================
*/
void S_StopStreamingSound( int stream )
{
	if(si.StopStreamingSound)
		si.StopStreamingSound(stream);
}

/*
=================
S_QueueStreamingSound
=================
*/
void S_QueueStreamingSound( int stream, const char *filename, float volume )
{
	if(si.QueueStreamingSound)
		si.QueueStreamingSound(stream, filename, volume);
}

/*
=================
S_GetStreamPlayCount
=================
*/
int  S_GetStreamPlayCount( int stream )
{
	if(si.GetStreamPlayCount)
		return si.GetStreamPlayCount(stream);
	return 0;
}

/*
=================
S_SetStreamVolume
=================
*/
void S_SetStreamVolume( int stream, float volume )
{
	if(si.SetStreamVolume)
		si.SetStreamVolume(stream, volume);
}

/*
=================
S_RawSamples
=================
*/
void S_RawSamples (int stream, int samples, int rate, int width, int channels,
		   const byte *data, float volume, int entityNum)
{
	if(si.RawSamples)
		si.RawSamples(stream, samples, rate, width, channels, data, volume, entityNum);
}

/*
=================
S_StopAllSounds
=================
*/
void S_StopAllSounds( void )
{
	if( si.StopAllSounds ) {
		si.StopAllSounds( );
	}
}

/*
=================
S_ClearLoopingSounds
=================
*/
void S_ClearLoopingSounds( qboolean killall )
{
	if( si.ClearLoopingSounds ) {
		si.ClearLoopingSounds( killall );
	}
}

/*
=================
S_AddLoopingSound
=================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	if( si.AddLoopingSound ) {
		si.AddLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_AddRealLoopingSound
=================
*/
void S_AddRealLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	if( si.AddRealLoopingSound ) {
		si.AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_StopLoopingSound
=================
*/
void S_StopLoopingSound( int entityNum )
{
	if( si.StopLoopingSound ) {
		si.StopLoopingSound( entityNum );
	}
}

/*
=================
S_Respatialize
=================
*/
void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater, qboolean firstPerson )
{
	if( si.Respatialize ) {
		si.Respatialize( entityNum, origin, axis, inwater, firstPerson );
	}
}

/*
=================
S_UpdateEntityPosition
=================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if( si.UpdateEntityPosition ) {
		si.UpdateEntityPosition( entityNum, origin );
	}
}

/*
=================
S_Update
=================
*/
void S_Update( void )
{
	if(s_muted->integer)
	{
		if(!(s_muteWhenMinimized->integer && com_minimized->integer) &&
		   !(s_muteWhenUnfocused->integer && com_unfocused->integer))
		{
			s_muted->integer = qfalse;
			s_muted->modified = qtrue;
		}
	}
	else
	{
		if((s_muteWhenMinimized->integer && com_minimized->integer) ||
		   (s_muteWhenUnfocused->integer && com_unfocused->integer))
		{
			s_muted->integer = qtrue;
			s_muted->modified = qtrue;
		}
	}
	
	if( si.Update ) {
		si.Update( );
	}

	S_ListenersEndFrame();
}

/*
=================
S_DisableSounds
=================
*/
void S_DisableSounds( void )
{
	if( si.DisableSounds ) {
		si.DisableSounds( );
	}
}

/*
=================
S_BeginRegistration
=================
*/
void S_BeginRegistration( void )
{
	if( si.BeginRegistration ) {
		si.BeginRegistration( );
	}
}

/*
=================
S_RegisterSound
=================
*/
sfxHandle_t	S_RegisterSound( const char *sample, qboolean compressed )
{
	if( si.RegisterSound ) {
		return si.RegisterSound( sample, compressed );
	} else {
		return 0;
	}
}

/*
=================
S_SoundDuration
=================
*/
int S_SoundDuration( sfxHandle_t handle )
{
	if( si.SoundDuration )
		return si.SoundDuration( handle );
	else
		return 0;
}

/*
=================
S_ClearSoundBuffer
=================
*/
void S_ClearSoundBuffer( void )
{
	if( si.ClearSoundBuffer ) {
		si.ClearSoundBuffer( );
	}
}

/*
=================
S_SoundInfo
=================
*/
void S_SoundInfo( void )
{
	if( si.SoundInfo ) {
		si.SoundInfo( );
	}
}

/*
=================
S_SoundList
=================
*/
void S_SoundList( void )
{
	if( si.SoundList ) {
		si.SoundList( );
	}
}


#ifdef USE_VOIP
/*
=================
S_StartCapture
=================
*/
void S_StartCapture( void )
{
	if( si.StartCapture ) {
		si.StartCapture( );
	}
}

/*
=================
S_AvailableCaptureSamples
=================
*/
int S_AvailableCaptureSamples( void )
{
	if( si.AvailableCaptureSamples ) {
		return si.AvailableCaptureSamples( );
	}
	return 0;
}

/*
=================
S_Capture
=================
*/
void S_Capture( int samples, byte *data )
{
	if( si.Capture ) {
		si.Capture( samples, data );
	}
}

/*
=================
S_StopCapture
=================
*/
void S_StopCapture( void )
{
	if( si.StopCapture ) {
		si.StopCapture( );
	}
}

/*
=================
S_MasterGain
=================
*/
void S_MasterGain( float gain )
{
	if( si.MasterGain ) {
		si.MasterGain( gain );
	}
}
#endif

//=============================================================================

/*
=================
S_Init
=================
*/
void S_Init( void )
{
	cvar_t		*cv;
	qboolean	started = qfalse;

	Com_Printf( "------ Initializing Sound ------\n" );

	s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
	s_muted = Cvar_Get("s_muted", "0", CVAR_ROM);
	s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_backend = Cvar_Get( "s_backend", "", CVAR_ROM );
	s_muteWhenMinimized = Cvar_Get( "s_muteWhenMinimized", "0", CVAR_ARCHIVE );
	s_muteWhenUnfocused = Cvar_Get( "s_muteWhenUnfocused", "0", CVAR_ARCHIVE );

	cv = Cvar_Get( "s_initsound", "1", 0 );
	if( !cv->integer ) {
		Com_Printf( "Sound disabled.\n" );
	} else {

		S_CodecInit( );
		S_ListenersInit( );

		Cmd_AddCommand( "s_list", S_SoundList );
		Cmd_AddCommand( "s_stop", S_StopAllSounds );
		Cmd_AddCommand( "s_info", S_SoundInfo );

		cv = Cvar_Get( "s_useOpenAL", "0", CVAR_ARCHIVE | CVAR_LATCH );
		if( cv->integer ) {
			//OpenAL
			started = S_AL_Init( &si );
			Cvar_Set( "s_backend", "OpenAL" );
		}

		if( !started ) {
			started = S_Base_Init( &si );
			Cvar_Set( "s_backend", "base" );
		}

		if( started ) {
			if( !S_ValidSoundInterface( &si ) ) {
				Com_Error( ERR_FATAL, "Sound interface invalid" );
			}

			S_SoundInfo( );
			Com_Printf( "Sound initialization successful.\n" );
		} else {
			Com_Printf( "Sound initialization failed.\n" );
		}
	}

	Com_Printf( "--------------------------------\n");
}

/*
=================
S_Shutdown
=================
*/
void S_Shutdown( void )
{
	if( si.Shutdown ) {
		si.Shutdown( );
	}

	Com_Memset( &si, 0, sizeof( soundInterface_t ) );

	Cmd_RemoveCommand( "s_list" );
	Cmd_RemoveCommand( "s_stop" );
	Cmd_RemoveCommand( "s_info" );

	S_CodecShutdown( );
}


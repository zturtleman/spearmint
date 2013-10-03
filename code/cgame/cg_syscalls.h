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

#ifndef __CG_SYSCALLS_H__
#define __CG_SYSCALLS_H__

//
// system traps
// These functions are how cgame communicates with the client system
//

// Additional shared traps in ../game/bg_misc.h

void		trap_GetClipboardData( char *buf, int bufsize );
void		trap_GetGlconfig( glconfig_t *glconfig );
// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );
int			trap_MemoryRemaining( void );
int			trap_GetVoipTime( int clientNum );
float		trap_GetVoipPower( int clientNum );
float		trap_GetVoipGain( int clientNum );
qboolean	trap_GetVoipMute( int clientNum );
qboolean	trap_GetVoipMuteAll( void );


// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot, void *playerStates, void *entities );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );	

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd, int localClientNum );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

//
void		trap_SetNetFields( int entityStateSize, vmNetField_t *entityStateFields, int numEntityStateFields,
						int playerStateSize, vmNetField_t *playerStateFields, int numPlayerStateFields );

int			trap_GetDemoState( void );
int			trap_GetDemoPos( void );
void		trap_GetDemoName( char *buffer, int size );
int			trap_GetDemoLength( void );

void		trap_GetClientState( uiClientState_t *state );
int			trap_GetConfigString( int index, char* buff, int buffsize );
void		trap_SetMapTitle( const char *name );
void		trap_SetViewAngles( int localPlayerNum, const vec3_t angles );
void		trap_GetViewAngles( int localPlayerNum, const vec3_t angles );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int contents );
clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs, int contents );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void		trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void          trap_CM_BiSphereTrace( trace_t *results, const vec3_t start,
                const vec3_t end, float startRad, float endRad,
                clipHandle_t model, int mask );
void          trap_CM_TransformedBiSphereTrace( trace_t *results, const vec3_t start,
                const vec3_t end, float startRad, float endRad,
                clipHandle_t model, int mask,
                const vec3_t origin );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points, 
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

void		trap_R_LoadWorldMap( const char *mapname );
qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void        trap_R_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer );
void		trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b );
void		trap_R_AddAdditiveLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b );
void		trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_SetClipRegion( const float *region );
void		trap_R_DrawStretchPic( float x, float y, float w, float h, 
				float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void		trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader,
				const float *gradientColor, int gradientType );
void		trap_R_Add2dPolys( polyVert_t* verts, int numverts, qhandle_t hShader );
int			trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs, int startFrame, int endFrame, float frac );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean	trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
void		trap_R_GetGlobalFog( fogType_t *type, vec3_t color, float *depthForOpaque, float *density );
void		trap_R_GetViewFog( const vec3_t origin, fogType_t *type, vec3_t color, float *depthForOpaque, float *density, qboolean inwater );

void		trap_R_SetSurfaceShader( int surfaceNum, const char *name );
qhandle_t	trap_R_GetSurfaceShader( int surfaceNum, int withlightmap );
qhandle_t	trap_R_GetShaderFromModel( qhandle_t hModel, int surfnum, int withlightmap );
void		trap_R_GetShaderName( qhandle_t hShader, char *buffer, int bufferSize );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );

void		trap_S_StopLoopingSound(int entnum);
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater, qboolean firstPerson );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
int			trap_S_SoundDuration( sfxHandle_t handle );
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop, float volume, float loopVolume );	// empty name stops music
void		trap_S_StopBackgroundTrack( void );

void			trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void			trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void			trap_Key_SetBinding( int keynum, const char *binding );
qboolean		trap_Key_IsDown( int keynum );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
void			trap_Key_SetCatcher( int catcher );
int				trap_Key_GetKey( const char *binding, int startKey );

int				trap_Mouse_GetState( int localClientNum );
void			trap_Mouse_SetState( int localClientNum, int state );

int				trap_LAN_GetPingQueueCount( void );
void			trap_LAN_ClearPing( int n );
void			trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void			trap_LAN_GetPingInfo( int n, char *buf, int buflen );
int				trap_LAN_GetServerCount( int source );
void			trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void			trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int				trap_LAN_GetServerPing( int source, int n );
int				trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
void			trap_LAN_SaveCachedServers( void );
void			trap_LAN_LoadCachedServers( void );
void			trap_LAN_ResetPings(int n);
void			trap_LAN_MarkServerVisible( int source, int n, qboolean visible );
int				trap_LAN_ServerIsVisible( int source, int n);
qboolean		trap_LAN_UpdateVisiblePings( int source );
int				trap_LAN_AddServer(int source, const char *name, const char *addr);
void			trap_LAN_RemoveServer(int source, const char *addr);
int				trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
qboolean		trap_LAN_ServerIsInFavoriteList( int source, int n  );

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int			trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status	trap_CIN_StopCinematic(int handle);

// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status	trap_CIN_RunCinematic (int handle);

// draws the current frame
void		trap_CIN_DrawCinematic (int handle);

// allows you to resize the animation dynamically
void		trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

/*
qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);
*/

#endif


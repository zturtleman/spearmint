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
#include "ui_local.h"

// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif

static intptr_t (QDECL *syscall)( intptr_t arg, ... ) = (intptr_t (QDECL *)( intptr_t, ...))-1;

Q_EXPORT void dllEntry( intptr_t (QDECL *syscallptr)( intptr_t arg,... ) ) {
	syscall = syscallptr;
}

int PASSFLOAT( float x ) {
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void trap_Print( const char *string ) {
	syscall( UI_PRINT, string );
}

void trap_Error(const char *string)
{
	syscall(UI_ERROR, string);
	// shut up GCC warning about returning functions, because we know better
	exit(1);
}

int trap_Milliseconds( void ) {
	return syscall( UI_MILLISECONDS ); 
}

void trap_SnapVector( float *v ) {
	syscall( UI_SNAPVECTOR, v );
}

void trap_AddCommand( const char *cmdName ) {
	syscall( UI_ADDCOMMAND, cmdName );
}

void trap_RemoveCommand( const char *cmdName ) {
	syscall( UI_REMOVECOMMAND, cmdName );
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( UI_CVAR_UPDATE, cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( UI_CVAR_SET, var_name, value );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	syscall( UI_CVAR_SET_VALUE, var_name, PASSFLOAT( value ) );
}

void trap_Cvar_Reset( const char *name ) {
	syscall( UI_CVAR_RESET, name );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	floatint_t fi;
	fi.i = syscall( UI_CVAR_VARIABLE_VALUE, var_name );
	return fi.f;
}

int trap_Cvar_VariableIntegerValue( const char *var_name ) {
	return syscall( UI_CVAR_VARIABLE_INTEGER_VALUE, var_name );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( UI_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize );
}

void trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( UI_CVAR_LATCHED_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	syscall( UI_CVAR_INFO_STRING_BUFFER, bit, buffer, bufsize );
}

int trap_Argc( void ) {
	return syscall( UI_ARGC );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( UI_ARGV, n, buffer, bufferLength );
}

void trap_Args( char *buffer, int bufferLength ) {
	syscall( UI_ARGS, buffer, bufferLength );
}

void trap_LiteralArgs( char *buffer, int bufferLength ) {
	syscall( UI_LITERAL_ARGS, buffer, bufferLength );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	syscall( UI_CMD_EXECUTETEXT, exec_when, text );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( UI_FS_FOPENFILE, qpath, f, mode );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_READ, buffer, len, f );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_WRITE, buffer, len, f );
}

int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return syscall( UI_FS_SEEK, f, offset, origin );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( UI_FS_FCLOSEFILE, f );
}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

int trap_FS_Delete( const char *path ) {
	return syscall( UI_FS_DELETE, path );
}

int trap_FS_Rename( const char *from, const char *to ) {
	return syscall( UI_FS_RENAME, from, to );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return syscall( UI_R_REGISTERMODEL, name );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return syscall( UI_R_REGISTERSKIN, name );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	return syscall( UI_R_REGISTERSHADER, name );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return syscall( UI_R_REGISTERSHADERNOMIP, name );
}

void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) {
	syscall( UI_R_REGISTERFONT, fontName, pointSize, font );
}

void trap_R_ClearScene( void ) {
	syscall( UI_R_CLEARSCENE );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( UI_R_ADDREFENTITYTOSCENE, re );
}

void trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num ) {
	syscall( UI_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, num );
}

void trap_R_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer ) {
	syscall( UI_R_ADDPOLYBUFFERTOSCENE, pPolyBuffer );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	syscall( UI_R_RENDERSCENE, fd );
}

void trap_R_SetColor( const float *rgba ) {
	syscall( UI_R_SETCOLOR, rgba );
}

void  trap_R_SetClipRegion( const float *region ) {
	syscall( UI_R_SETCLIPREGION, region );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader );
}

void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle ) {
	syscall( UI_R_DRAWROTATEDPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, PASSFLOAT( angle ) );
}

void trap_R_DrawStretchPicGradient(  float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader,
										const float *gradientColor, int gradientType ) {
	syscall( UI_R_DRAWSTRETCHPIC_GRADIENT, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, gradientColor, gradientType  );
}

void trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader ) {
	syscall( UI_R_DRAW2DPOLYS, verts, numverts, hShader );
}

void	trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}

void trap_UpdateScreen( void ) {
	syscall( UI_UPDATESCREEN );
}

int trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	return syscall( UI_R_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName );
}

void	trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	syscall( UI_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}

sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return syscall( UI_S_REGISTERSOUND, sample, compressed );
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( UI_S_STARTLOCALSOUND, sfx, channelNum );
}

int trap_S_SoundDuration( sfxHandle_t handle ) {
	return syscall( UI_S_SOUNDDURATION, handle );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_GETBINDINGBUF, keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	syscall( UI_KEY_SETBINDING, keynum, binding );
}

qboolean trap_Key_IsDown( int keynum ) {
	return syscall( UI_KEY_ISDOWN, keynum );
}

qboolean trap_Key_GetOverstrikeMode( void ) {
	return syscall( UI_KEY_GETOVERSTRIKEMODE );
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}

void trap_Key_ClearStates( void ) {
	syscall( UI_KEY_CLEARSTATES );
}

int trap_Key_GetCatcher( void ) {
	return syscall( UI_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( UI_KEY_SETCATCHER, catcher );
}

int trap_Key_GetKey( const char *binding, int startKey ) {
	return syscall( UI_KEY_GETKEY, binding, startKey );
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	syscall( UI_GETCLIPBOARDDATA, buf, bufsize );
}

void trap_GetGlconfig( glconfig_t *glconfig ) {
	syscall( UI_GETGLCONFIG, glconfig );
}

int trap_GetVoipTime( int clientNum ) {
	return syscall( UI_GET_VOIP_TIME, clientNum );
}

float trap_GetVoipPower( int clientNum ) {
	floatint_t fi;
	fi.i = syscall( UI_GET_VOIP_POWER, clientNum );
	return fi.f;
}

float trap_GetVoipGain( int clientNum ) {
	floatint_t fi;
	fi.i = syscall( UI_GET_VOIP_GAIN, clientNum );
	return fi.f;
}

qboolean trap_GetVoipMute( int clientNum ) {
	return syscall( UI_GET_VOIP_MUTE_CLIENT, clientNum );
}

qboolean trap_GetVoipMuteAll( void ) {
	return syscall(	UI_GET_VOIP_MUTE_ALL );
}

void trap_GetClientState( uiClientState_t *state ) {
	syscall( UI_GETCLIENTSTATE, state );
}

int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return syscall( UI_GETCONFIGSTRING, index, buff, buffsize );
}

int trap_LAN_GetPingQueueCount( void ) {
	return syscall( UI_LAN_GETPINGQUEUECOUNT );
}

void trap_LAN_ClearPing( int n ) {
	syscall( UI_LAN_CLEARPING, n );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime ) {
	syscall( UI_LAN_GETPING, n, buf, buflen, pingtime );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETPINGINFO, n, buf, buflen );
}

int	trap_LAN_GetServerCount( int source ) {
	return syscall( UI_LAN_GETSERVERCOUNT, source );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen ) {
	syscall( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}

int trap_LAN_GetServerPing( int source, int n ) {
	return syscall( UI_LAN_GETSERVERPING, source, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen ) {
	return syscall( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

void trap_LAN_SaveCachedServers( void ) {
	syscall( UI_LAN_SAVECACHEDSERVERS );
}

void trap_LAN_LoadCachedServers( void ) {
	syscall( UI_LAN_LOADCACHEDSERVERS );
}

void trap_LAN_ResetPings(int n) {
	syscall( UI_LAN_RESETPINGS, n );
}

void trap_LAN_MarkServerVisible( int source, int n, qboolean visible ) {
	syscall( UI_LAN_MARKSERVERVISIBLE, source, n, visible );
}

int trap_LAN_ServerIsVisible( int source, int n) {
	return syscall( UI_LAN_SERVERISVISIBLE, source, n );
}

qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}

int trap_LAN_AddServer(int source, const char *name, const char *addr) {
	return syscall( UI_LAN_ADDSERVER, source, name, addr );
}

void trap_LAN_RemoveServer(int source, const char *addr) {
	syscall( UI_LAN_REMOVESERVER, source, addr );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 ) {
	return syscall( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

int trap_MemoryRemaining( void ) {
	return syscall( UI_MEMORY_REMAINING );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( UI_PC_ADD_GLOBAL_DEFINE, define );
}

void trap_PC_RemoveAllGlobalDefines( void ) {
	syscall( UI_PC_REMOVE_ALL_GLOBAL_DEFINES );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( UI_PC_LOAD_SOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( UI_PC_FREE_SOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( UI_PC_READ_TOKEN, handle, pc_token );
}

void trap_PC_UnreadToken( int handle ) {
	syscall( UI_PC_UNREAD_TOKEN, handle );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

void trap_S_StopBackgroundTrack( void ) {
	syscall( UI_S_STOPBACKGROUNDTRACK );
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop) {
	syscall( UI_S_STARTBACKGROUNDTRACK, intro, loop );
}

int trap_RealTime(qtime_t *qtime) {
	return syscall( UI_REAL_TIME, qtime );
}

int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits) {
  return syscall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}

e_status trap_CIN_StopCinematic(int handle) {
  return syscall(UI_CIN_STOPCINEMATIC, handle);
}

e_status trap_CIN_RunCinematic (int handle) {
  return syscall(UI_CIN_RUNCINEMATIC, handle);
}

void trap_CIN_DrawCinematic (int handle) {
  syscall(UI_CIN_DRAWCINEMATIC, handle);
}

void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  syscall(UI_CIN_SETEXTENTS, handle, x, y, w, h);
}


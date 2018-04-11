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
#include "q_shared.h"
#include "qcommon.h"

static huffman_t		msgHuff;

static qboolean			msgInit = qfalse;

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

void MSG_initHuffman( void );

void MSG_Init( msg_t *buf, byte *data, int length ) {
	if (!msgInit) {
		MSG_initHuffman();
	}
	Com_Memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void MSG_InitOOB( msg_t *buf, byte *data, int length ) {
	if (!msgInit) {
		MSG_initHuffman();
	}
	Com_Memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
	buf->oob = qtrue;
}

void MSG_Clear( msg_t *buf ) {
	buf->cursize = 0;
	buf->overflowed = qfalse;
	buf->bit = 0;					//<- in bits
}


void MSG_Bitstream( msg_t *buf ) {
	buf->oob = qfalse;
}

void MSG_BeginReading( msg_t *msg ) {
	msg->readcount = 0;
	msg->bit = 0;
	msg->oob = qfalse;
}

void MSG_BeginReadingOOB( msg_t *msg ) {
	msg->readcount = 0;
	msg->bit = 0;
	msg->oob = qtrue;
}

void MSG_Copy(msg_t *buf, byte *data, int length, msg_t *src)
{
	if (length<src->cursize) {
		Com_Error( ERR_DROP, "MSG_Copy: can't copy into a smaller msg_t buffer");
	}
	Com_Memcpy(buf, src, sizeof(msg_t));
	buf->data = data;
	Com_Memcpy(buf->data, src->data, src->cursize);
}

/*
=============================================================================

bit functions
  
=============================================================================
*/

// negative bit values include signs
void MSG_WriteBits( msg_t *msg, int value, int bits ) {
	int	i;

	if ( msg->overflowed ) {
		return;
	}

	if ( bits == 0 || bits < -31 || bits > 32 ) {
		Com_Error( ERR_DROP, "MSG_WriteBits: bad bits %i", bits );
	}

	if ( bits < 0 ) {
		bits = -bits;
	}

	if ( msg->oob ) {
		if ( msg->cursize + ( bits >> 3 ) > msg->maxsize ) {
			msg->overflowed = qtrue;
			return;
		}

		if ( bits == 8 ) {
			msg->data[msg->cursize] = value;
			msg->cursize += 1;
			msg->bit += 8;
		} else if ( bits == 16 ) {
			short temp = value;

			CopyLittleShort( &msg->data[msg->cursize], &temp );
			msg->cursize += 2;
			msg->bit += 16;
		} else if ( bits==32 ) {
			CopyLittleLong( &msg->data[msg->cursize], &value );
			msg->cursize += 4;
			msg->bit += 32;
		} else {
			Com_Error( ERR_DROP, "can't write %d bits", bits );
		}
	} else {
		value &= (0xffffffff >> (32 - bits));
		if ( bits&7 ) {
			int nbits;
			nbits = bits&7;
			if ( msg->bit + nbits > msg->maxsize << 3 ) {
				msg->overflowed = qtrue;
				return;
			}
			for( i = 0; i < nbits; i++ ) {
				Huff_putBit( (value & 1), msg->data, &msg->bit );
				value = (value >> 1);
			}
			bits = bits - nbits;
		}
		if ( bits ) {
			for( i = 0; i < bits; i += 8 ) {
				Huff_offsetTransmit( &msgHuff.compressor, (value & 0xff), msg->data, &msg->bit, msg->maxsize << 3 );
				value = (value >> 8);

				if ( msg->bit > msg->maxsize << 3 ) {
					msg->overflowed = qtrue;
					return;
				}
			}
		}
		msg->cursize = (msg->bit >> 3) + 1;
	}
}

int MSG_ReadBits( msg_t *msg, int bits ) {
	int			value;
	int			get;
	qboolean	sgn;
	int			i, nbits;
//	FILE*	fp;

	if ( msg->readcount > msg->cursize ) {
		return 0;
	}

	value = 0;

	if ( bits < 0 ) {
		bits = -bits;
		sgn = qtrue;
	} else {
		sgn = qfalse;
	}

	if (msg->oob) {
		if (msg->readcount + (bits>>3) > msg->cursize) {
			msg->readcount = msg->cursize + 1;
			return 0;
		}

		if(bits==8)
		{
			value = msg->data[msg->readcount];
			msg->readcount += 1;
			msg->bit += 8;
		}
		else if(bits==16)
		{
			short temp;
			
			CopyLittleShort(&temp, &msg->data[msg->readcount]);
			value = temp;
			msg->readcount += 2;
			msg->bit += 16;
		}
		else if(bits==32)
		{
			CopyLittleLong(&value, &msg->data[msg->readcount]);
			msg->readcount += 4;
			msg->bit += 32;
		}
		else
			Com_Error(ERR_DROP, "can't read %d bits", bits);
	} else {
		nbits = 0;
		if (bits&7) {
			nbits = bits&7;
			if (msg->bit + nbits > msg->cursize << 3) {
				msg->readcount = msg->cursize + 1;
				return 0;
			}
			for(i=0;i<nbits;i++) {
				value |= (Huff_getBit(msg->data, &msg->bit)<<i);
			}
			bits = bits - nbits;
		}
		if (bits) {
//			fp = fopen("c:\\netchan.bin", "a");
			for(i=0;i<bits;i+=8) {
				Huff_offsetReceive (msgHuff.decompressor.tree, &get, msg->data, &msg->bit, msg->cursize<<3);
//				fwrite(&get, 1, 1, fp);
				value = (unsigned int)value | ((unsigned int)get<<(i+nbits));

				if (msg->bit > msg->cursize<<3) {
					msg->readcount = msg->cursize + 1;
					return 0;
				}
			}
//			fclose(fp);
		}
		msg->readcount = (msg->bit>>3)+1;
	}
	if ( sgn && bits > 0 && bits < 32 ) {
		if ( value & ( 1 << ( bits - 1 ) ) ) {
			value |= -1 ^ ( ( 1 << bits ) - 1 );
		}
	}

	return value;
}



//================================================================================

//
// writing functions
//

void MSG_WriteChar( msg_t *sb, int c ) {
#ifdef PARANOID
	if (c < -128 || c > 127)
		Com_Error (ERR_FATAL, "MSG_WriteChar: range error");
#endif

	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteByte( msg_t *sb, int c ) {
#ifdef PARANOID
	if (c < 0 || c > 255)
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
#endif

	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteData( msg_t *buf, const void *data, int length ) {
	int i;
	for(i=0;i<length;i++) {
		MSG_WriteByte(buf, ((byte *)data)[i]);
	}
}

void MSG_WriteShort( msg_t *sb, int c ) {
#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Com_Error (ERR_FATAL, "MSG_WriteShort: range error");
#endif

	MSG_WriteBits( sb, c, 16 );
}

void MSG_WriteLong( msg_t *sb, int c ) {
	MSG_WriteBits( sb, c, 32 );
}

void MSG_WriteFloat( msg_t *sb, float f ) {
	floatint_t dat;
	dat.f = f;
	MSG_WriteBits( sb, dat.i, 32 );
}

void MSG_WriteString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData (sb, "", 1);
	} else {
		int		l;
		char	string[MAX_STRING_CHARS];

		l = strlen( s );
		if ( l >= MAX_STRING_CHARS ) {
			Com_Printf( "MSG_WriteString: MAX_STRING_CHARS" );
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData (sb, string, l+1);
	}
}

void MSG_WriteBigString( msg_t *sb, const char *s ) {
	if ( !s ) {
		MSG_WriteData (sb, "", 1);
	} else {
		int		l;
		char	string[BIG_INFO_STRING];

		l = strlen( s );
		if ( l >= BIG_INFO_STRING ) {
			Com_Printf( "MSG_WriteString: BIG_INFO_STRING" );
			MSG_WriteData (sb, "", 1);
			return;
		}
		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData (sb, string, l+1);
	}
}

void MSG_WriteAngle( msg_t *sb, float f ) {
	MSG_WriteByte (sb, (int)(f*256/360) & 255);
}

void MSG_WriteAngle16( msg_t *sb, float f ) {
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}


//============================================================

//
// reading functions
//

// returns -1 if no more characters are available
int MSG_ReadChar (msg_t *msg ) {
	int	c;
	
	c = (signed char)MSG_ReadBits( msg, 8 );
	if ( msg->readcount > msg->cursize ) {
		c = -1;
	}	
	
	return c;
}

int MSG_ReadByte( msg_t *msg ) {
	int	c;
	
	c = (unsigned char)MSG_ReadBits( msg, 8 );
	if ( msg->readcount > msg->cursize ) {
		c = -1;
	}	
	return c;
}

int MSG_LookaheadByte( msg_t *msg ) {
	const int bloc = Huff_getBloc();
	const int readcount = msg->readcount;
	const int bit = msg->bit;
	int c = MSG_ReadByte(msg);
	Huff_setBloc(bloc);
	msg->readcount = readcount;
	msg->bit = bit;
	return c;
}

int MSG_ReadShort( msg_t *msg ) {
	int	c;
	
	c = (short)MSG_ReadBits( msg, 16 );
	if ( msg->readcount > msg->cursize ) {
		c = -1;
	}	

	return c;
}

int MSG_ReadLong( msg_t *msg ) {
	int	c;
	
	c = MSG_ReadBits( msg, 32 );
	if ( msg->readcount > msg->cursize ) {
		c = -1;
	}	
	
	return c;
}

float MSG_ReadFloat( msg_t *msg ) {
	floatint_t dat;
	
	dat.i = MSG_ReadBits( msg, 32 );
	if ( msg->readcount > msg->cursize ) {
		dat.f = -1;
	}	
	
	return dat.f;	
}

char *MSG_ReadString( msg_t *msg ) {
	static char	string[MAX_STRING_CHARS];
	int		l,c;
	
	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if ( c == -1 || c == 0 ) {
			break;
		}
		// break only after reading all expected data from bitstream
		if ( l >= sizeof(string)-1 ) {
			break;
		}
		string[l++] = c;
	} while (1);
	
	string[l] = '\0';
	
	return string;
}

char *MSG_ReadBigString( msg_t *msg ) {
	static char	string[BIG_INFO_STRING];
	int		l,c;
	
	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if ( c == -1 || c == 0 ) {
			break;
		}
		// break only after reading all expected data from bitstream
		if ( l >= sizeof(string)-1 ) {
			break;
		}
		string[l++] = c;
	} while (1);
	
	string[l] = '\0';
	
	return string;
}

char *MSG_ReadStringLine( msg_t *msg ) {
	static char	string[MAX_STRING_CHARS];
	int		l,c;

	l = 0;
	do {
		c = MSG_ReadByte(msg);		// use ReadByte so -1 is out of bounds
		if (c == -1 || c == 0 || c == '\n') {
			break;
		}
		// break only after reading all expected data from bitstream
		if ( l >= sizeof(string)-1 ) {
			break;
		}
		string[l++] = c;
	} while (1);
	
	string[l] = '\0';
	
	return string;
}

float MSG_ReadAngle16( msg_t *msg ) {
	return SHORT2ANGLE(MSG_ReadShort(msg));
}

void MSG_ReadData( msg_t *msg, void *data, int len ) {
	int		i;

	for (i=0 ; i<len ; i++) {
		((byte *)data)[i] = MSG_ReadByte (msg);
	}
}

// a string hasher which gives the same hash value even if the
// string is later modified via the legacy MSG read/write code
int MSG_HashKey(const char *string, int maxlen) {
	int hash, i;

	hash = 0;
	for (i = 0; i < maxlen && string[i] != '\0'; i++) {
		if (string[i] == '%')
			hash += '.' * (119 + i);
		else
			hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

extern cvar_t *cl_shownet;

#define	LOG(x) if( cl_shownet && cl_shownet->integer == 4 ) { Com_Printf("%s ", x ); };

/*
=============================================================================

delta functions with keys
  
=============================================================================
*/

int kbitmask[32] = {
	0x00000001, 0x00000003, 0x00000007, 0x0000000F,
	0x0000001F,	0x0000003F,	0x0000007F,	0x000000FF,
	0x000001FF,	0x000003FF,	0x000007FF,	0x00000FFF,
	0x00001FFF,	0x00003FFF,	0x00007FFF,	0x0000FFFF,
	0x0001FFFF,	0x0003FFFF,	0x0007FFFF,	0x000FFFFF,
	0x001FFFFf,	0x003FFFFF,	0x007FFFFF,	0x00FFFFFF,
	0x01FFFFFF,	0x03FFFFFF,	0x07FFFFFF,	0x0FFFFFFF,
	0x1FFFFFFF,	0x3FFFFFFF,	0x7FFFFFFF,	0xFFFFFFFF,
};

void MSG_WriteDeltaKey( msg_t *msg, int key, int oldV, int newV, int bits ) {
	if ( oldV == newV ) {
		MSG_WriteBits( msg, 0, 1 );
		return;
	}
	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, newV ^ key, bits );
}

int	MSG_ReadDeltaKey( msg_t *msg, int key, int oldV, int bits ) {
	if ( MSG_ReadBits( msg, 1 ) ) {
		return MSG_ReadBits( msg, bits ) ^ (key & kbitmask[ bits - 1 ]);
	}
	return oldV;
}

void MSG_WriteDeltaKeyFloat( msg_t *msg, int key, float oldV, float newV ) {
	floatint_t fi;
	if ( oldV == newV ) {
		MSG_WriteBits( msg, 0, 1 );
		return;
	}
	fi.f = newV;
	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, fi.i ^ key, 32 );
}

float MSG_ReadDeltaKeyFloat( msg_t *msg, int key, float oldV ) {
	if ( MSG_ReadBits( msg, 1 ) ) {
		floatint_t fi;

		fi.i = MSG_ReadBits( msg, 32 ) ^ key;
		return fi.f;
	}
	return oldV;
}


/*
============================================================================

usercmd_t communication

============================================================================
*/

/*
=====================
MSG_WriteDeltaUsercmdKey
=====================
*/
void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to ) {
	if ( to->serverTime - from->serverTime < 256 ) {
		MSG_WriteBits( msg, 1, 1 );
		MSG_WriteBits( msg, to->serverTime - from->serverTime, 8 );
	} else {
		MSG_WriteBits( msg, 0, 1 );
		MSG_WriteBits( msg, to->serverTime, 32 );
	}
	if (from->angles[0] == to->angles[0] &&
		from->angles[1] == to->angles[1] &&
		from->angles[2] == to->angles[2] &&
		from->forwardmove == to->forwardmove &&
		from->rightmove == to->rightmove &&
		from->upmove == to->upmove &&
		from->buttons == to->buttons &&
		from->stateValue == to->stateValue) {
			MSG_WriteBits( msg, 0, 1 );				// no change
			return;
	}
	key ^= to->serverTime;
	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteDeltaKey( msg, key, from->angles[0], to->angles[0], 16 );
	MSG_WriteDeltaKey( msg, key, from->angles[1], to->angles[1], 16 );
	MSG_WriteDeltaKey( msg, key, from->angles[2], to->angles[2], 16 );
	MSG_WriteDeltaKey( msg, key, from->forwardmove, to->forwardmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->rightmove, to->rightmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->upmove, to->upmove, 8 );
	MSG_WriteDeltaKey( msg, key, from->buttons, to->buttons, 16 );
	MSG_WriteDeltaKey( msg, key, from->stateValue, to->stateValue, 32 );
}


/*
=====================
MSG_ReadDeltaUsercmdKey
=====================
*/
void MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to ) {
	if ( MSG_ReadBits( msg, 1 ) ) {
		to->serverTime = from->serverTime + MSG_ReadBits( msg, 8 );
	} else {
		to->serverTime = MSG_ReadBits( msg, 32 );
	}
	if ( MSG_ReadBits( msg, 1 ) ) {
		key ^= to->serverTime;
		to->angles[0] = MSG_ReadDeltaKey( msg, key, from->angles[0], 16);
		to->angles[1] = MSG_ReadDeltaKey( msg, key, from->angles[1], 16);
		to->angles[2] = MSG_ReadDeltaKey( msg, key, from->angles[2], 16);
		to->forwardmove = MSG_ReadDeltaKey( msg, key, from->forwardmove, 8);
		if( to->forwardmove == -128 )
			to->forwardmove = -127;
		to->rightmove = MSG_ReadDeltaKey( msg, key, from->rightmove, 8);
		if( to->rightmove == -128 )
			to->rightmove = -127;
		to->upmove = MSG_ReadDeltaKey( msg, key, from->upmove, 8);
		if( to->upmove == -128 )
			to->upmove = -127;
		to->buttons = MSG_ReadDeltaKey( msg, key, from->buttons, 16);
		to->stateValue = MSG_ReadDeltaKey( msg, key, from->stateValue, 32);
	} else {
		to->angles[0] = from->angles[0];
		to->angles[1] = from->angles[1];
		to->angles[2] = from->angles[2];
		to->forwardmove = from->forwardmove;
		to->rightmove = from->rightmove;
		to->upmove = from->upmove;
		to->buttons = from->buttons;
		to->stateValue = from->stateValue;
	}
}

/*
=============================================================================

netField_t communication
  
=============================================================================
*/

// These can't be blindly increased.
#define MAX_NETF_ARRAY_BITS 32
#define MAX_NETF_ELEMENTS (32 * MAX_NETF_ARRAY_BITS)

typedef struct {
	int		offset;
	int		numElements; // 1 to 1024 (MAX_NETF_ELEMENTS)
	int		numElementArrays;
	int		bits;		// 0 = float
	int		pcount;
} netField_t;

typedef struct {
	char		*objectName;
	int			objectSize;

	int			numFields;
	netField_t	*fields;
} netFields_t;

static netFields_t msg_playerStateFields = { "playerState_t", 0, 0, NULL };
static netFields_t msg_entityStateFields = { "entityState_t", 0, 0, NULL };

/*
=================
MSG_ReportChangeVectors

Prints out a table from the current statistics for copying to code
=================
*/
static void MSG_ReportChangeVectors( netFields_t *stateFields ) {
	netField_t *field;
	int i;

 	Com_Printf( "%s (number of fields: %i, object size: %i)\n", stateFields->objectName, stateFields->numFields, stateFields->objectSize );

	for ( i = 0, field = stateFields->fields; i < msg_entityStateFields.numFields; i++, field++ ) {
		if ( field->pcount ) {
			Com_Printf( "field %i used %i times\n", i, field->pcount );
		}
	}
}

/*
=================
MSG_ReportChangeVectors_f

Prints out a table from the current statistics for copying to code
=================
*/
void MSG_ReportChangeVectors_f( void ) {
	MSG_ReportChangeVectors( &msg_playerStateFields );
	MSG_ReportChangeVectors( &msg_entityStateFields );
}

/*
==================
MSG_FreeNetFields
==================
*/
static void MSG_FreeNetFields( netFields_t *stateFields ) {
	if ( stateFields->fields ) {
		Z_Free( stateFields->fields );
		stateFields->fields = NULL;
	}
}

/*
==================
MSG_InitNetFields

Validate net fields and setup each field's numElementArrays.

Returns pointer to error message or NULL if no error.
==================
*/
static const char *MSG_InitNetFields( netFields_t *stateFields, const vmNetField_t *vmFields, int numFields, int objectSize, int expectedNetworkSize ) {
	netField_t			*field;
	const vmNetField_t	*vmField;
	int					fieldLength;
	int					networkSize;
	int					i, n;

	if ( !vmFields || numFields < 1 ) {
		return "no fields";
	}

	MSG_FreeNetFields( stateFields );

	stateFields->objectSize = objectSize;
	stateFields->numFields = numFields;
	stateFields->fields = Z_Malloc( sizeof (netField_t) * numFields );

	networkSize = 0;

	for ( i = 0, field = stateFields->fields, vmField = vmFields ; i < numFields ; i++, field++, vmField++ ) {
		field->offset = vmField->offset;
		field->numElements = vmField->numElements;
		field->bits = vmField->bits;

		if ( field->offset < 0 ) {
			return "field->offset < 0";
		}

		if ( field->bits < -31 ) {
			return "field->bits < -31";
		}

		if ( field->bits > 32 ) {
			return "field->bits > 32";
		}

		if ( field->numElements < 1 ) {
			return "field->numElements < 1";
		}

		if ( field->numElements > MAX_NETF_ELEMENTS ) {
			return va("field->numElements > %d", MAX_NETF_ELEMENTS);
		}

		fieldLength = 0;
		for (n=0 ; n<field->numElements ; n++) {
			fieldLength += 4;
		}

		if ( field->offset + fieldLength > objectSize ) {
			return "a field goes past end of state";
		}

		field->numElementArrays = 0;
		for ( n = field->numElements; n > 0; n -= MAX_NETF_ARRAY_BITS ) {
			field->numElementArrays++;
		}

		networkSize += fieldLength;
	}

	if ( networkSize > objectSize ) {
		return "fields send more data than size of state";
	}

	// For entityState_t:
	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this fails, someone added a field to the entityState_t
	// struct without updating the message fields
	if ( expectedNetworkSize > 0 ) {
		if ( networkSize < expectedNetworkSize ) {
			return "networks less data than expected";
		} else if ( networkSize > expectedNetworkSize ) {
			return "networks more data than expected";
		}
	}

	return NULL;
}

/*
==================
MSG_SetNetFields
==================
*/
void MSG_SetNetFields( vmNetField_t *vmEntityFields, int numEntityFields, int entityStateSize, int entityNetworkSize,
					   vmNetField_t *vmPlayerFields, int numPlayerFields, int playerStateSize, int playerNetworkSize ) {
	const char *error;

	error = MSG_InitNetFields( &msg_entityStateFields, vmEntityFields, numEntityFields, entityStateSize, entityNetworkSize );

	if ( error ) {
		Com_Error( ERR_DROP, "entityState_t: %s", error );
	}

	error = MSG_InitNetFields( &msg_playerStateFields, vmPlayerFields, numPlayerFields, playerStateSize, playerNetworkSize );

	if ( error ) {
		Com_Error( ERR_DROP, "playerState_t: %s", error );
	}
}

/*
==================
MSG_ShutdownNetFields
==================
*/
void MSG_ShutdownNetFields( void ) {
	MSG_FreeNetFields( &msg_entityStateFields );
	MSG_FreeNetFields( &msg_playerStateFields );
}

/*
==================
MSG_LastChangedField

Returns index of last changed field + 1, or 0 if no fields are different.
==================
*/
static int MSG_LastChangedField( void *from, void *to, netFields_t *stateFields ) {
	int			i, lc, n;
	int			*fromF, *toF;
	netField_t	*field;

	lc = 0;
	// build the change vector as bytes so it is endien independent
	for ( i = stateFields->numFields-1, field = &stateFields->fields[i] ; i >= 0 ; i--, field-- ) {
		toF = (int *)( (byte *)to + field->offset );

		if ( from ) {
			fromF = (int *)( (byte *)from + field->offset );
			for (n=0 ; n<field->numElements ; n++) {
				if (toF[n] != fromF[n]) {
					break;
				}
			}
		} else {
			for (n=0 ; n<field->numElements ; n++) {
				if (toF[n] != 0) {
					break;
				}
			}
		}

		if ( n != field->numElements ) {
			lc = i+1;
			break;
		}
	}

	return lc;
}

// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define	FLOAT_INT_BITS	13
#define	FLOAT_INT_BIAS	(1<<(FLOAT_INT_BITS-1))

/*
==================
MSG_WriteDeltaNetFields
==================
*/
static void MSG_WriteDeltaNetFields( msg_t *msg, void *from, void *to,
						   netFields_t *stateFields, int numSendFields ) {
	int			i, n;
	netField_t	*field;
	int			trunc;
	float		fullFloat;
	int			*fromF, *toF;
	int			elementsLeft;
	int			arraysChanged;
	int			bitsArray[MAX_NETF_ELEMENTS / MAX_NETF_ARRAY_BITS];

	for ( i = 0, field = stateFields->fields ; i < numSendFields ; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		arraysChanged = 0;
		Com_Memset( bitsArray, 0, sizeof (bitsArray) );

		for (n=0 ; n<field->numElements ; n++) {
			if ( ( from && toF[n] != fromF[n] ) || ( !from && toF[n] != 0 ) ) {
				arraysChanged |= 1 << ( n / MAX_NETF_ARRAY_BITS );
				bitsArray[ n / MAX_NETF_ARRAY_BITS ] |= 1 << ( n & ( MAX_NETF_ARRAY_BITS - 1 ) );
			}
		}

		if ( arraysChanged == 0 ) {
			MSG_WriteBits( msg, 0, field->numElementArrays );	// no change
			continue;
		}

		MSG_WriteBits( msg, arraysChanged, field->numElementArrays );	// changed

		if ( field->numElements > 1 ) {
			elementsLeft = field->numElements;
			// write bits for changed arrays
			for ( n = 0; n < field->numElementArrays; n++, elementsLeft -= MAX_NETF_ARRAY_BITS ) {
				if ( arraysChanged & ( 1 << n ) ) {
					MSG_WriteBits( msg, bitsArray[ n ], MIN( elementsLeft, MAX_NETF_ARRAY_BITS ) );
				}
			}
		} else {
			bitsArray[ 0 ] = 1;
		}

		for ( n = 0; n < field->numElements; n++, toF++ ) {
			if ( !( bitsArray[ n / MAX_NETF_ARRAY_BITS ] & ( 1 << ( n & ( MAX_NETF_ARRAY_BITS - 1 ) ) ) ) ) {
				continue;
			}

			if ( field->bits == 0 ) {
				// float
				fullFloat = *(float *)toF;
				trunc = (int)fullFloat;

				if (fullFloat == 0.0f) {
						MSG_WriteBits( msg, 0, 1 );
				} else {
					MSG_WriteBits( msg, 1, 1 );
					if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && 
						trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) ) {
						// send as small integer
						MSG_WriteBits( msg, 0, 1 );
						MSG_WriteBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
					} else {
						// send as full floating point value
						MSG_WriteBits( msg, 1, 1 );
						MSG_WriteBits( msg, *toF, 32 );
					}
				}
			} else {
				if (*toF == 0) {
					MSG_WriteBits( msg, 0, 1 );
				} else {
					MSG_WriteBits( msg, 1, 1 );
					// integer
					MSG_WriteBits( msg, *toF, field->bits );
				}
			}
		}
	}
}

/*
==================
MSG_ReadDeltaNetFields
==================
*/
static void MSG_ReadDeltaNetFields( msg_t *msg, void *from, void *to,
						 netFields_t *stateFields, int numReadFields,
						 int startBit, int print ) {
	int			i, n;
	netField_t	*field;
	int			trunc;
	int			*fromF, *toF;
	int			elementsLeft;
	int			arraysChanged;
	int			bitsArray[MAX_NETF_ELEMENTS / MAX_NETF_ARRAY_BITS];
	int			endBit;

	for ( i = 0, field = stateFields->fields ; i < numReadFields ; i++, field++ ) {
		toF = (int *)( (byte *)to + field->offset );

		arraysChanged = MSG_ReadBits( msg, field->numElementArrays );

		if ( arraysChanged == 0 ) {
			// no change
			if (from) {
				fromF = (int *)( (byte *)from + field->offset );
				for ( n = 0; n < field->numElements; n++, toF++, fromF++ ) {
					*toF = *fromF;
				}
			} else {
				for ( n = 0; n < field->numElements; n++, toF++ ) {
					*toF = 0;
				}
			}

			continue;
		}

		if ( field->numElements > 1 ) {
			elementsLeft = field->numElements;
			// read bits for changed arrays
			for ( n = 0; n < field->numElementArrays; n++, elementsLeft -= MAX_NETF_ARRAY_BITS ) {
				if ( arraysChanged & ( 1 << n ) ) {
					bitsArray[ n ] = MSG_ReadBits( msg, MIN( elementsLeft, MAX_NETF_ARRAY_BITS ) );
				} else {
					bitsArray[ n ] = 0;
				}
			}

			if ( print ) {
				Com_Printf( "array %i ", i );
			}
		} else {
			bitsArray[ 0 ] = 1;

			if ( print ) {
				Com_Printf( "field %i:", i );
			}
		}

		for ( n = 0; n < field->numElements; n++, toF++ ) {
			if ( !( bitsArray[ n / MAX_NETF_ARRAY_BITS ] & ( 1 << ( n & ( MAX_NETF_ARRAY_BITS - 1 ) ) ) ) ) {
				// no change
				if (from) {
					fromF = (int *)( (byte *)from + field->offset );

					*toF = fromF[n];
				} else {
					*toF = 0;
				}
				continue;
			}

			if ( print && field->numElements > 1 ) {
				Com_Printf( "[%i]:", n );
			}

			if ( field->bits == 0 ) {
				// float
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
					*(float *)toF = 0.0f;
					if ( print ) {
						Com_Printf( "%i ", 0 );
					}
				} else {
					if ( MSG_ReadBits( msg, 1 ) == 0 ) {
						// integral float
						trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
						// bias to allow equal parts positive and negative
						trunc -= FLOAT_INT_BIAS;
						*(float *)toF = trunc; 
						if ( print ) {
							Com_Printf( "%i ", trunc );
						}
					} else {
						// full floating point value
						*toF = MSG_ReadBits( msg, 32 );
						if ( print ) {
							Com_Printf( "%f ", *(float *)toF );
						}
					}
				}
			} else {
				if ( MSG_ReadBits( msg, 1 ) == 0 ) {
					*toF = 0;
					if ( print ) {
						Com_Printf( "%i ", 0 );
					}
				} else {
					// integer
					*toF = MSG_ReadBits( msg, field->bits );
					if ( print ) {
						Com_Printf( "%i ", *toF );
					}
				}
			}
			field->pcount++;
		}
	}
	// copy unchanged fields
	for ( i = numReadFields, field = &stateFields->fields[i] ; i < stateFields->numFields ; i++, field++ ) {
		toF = (int *)( (byte *)to + field->offset );
		// no change
		if (from) {
			fromF = (int *)( (byte *)from + field->offset );
			for ( n = 0; n < field->numElements; n++, toF++, fromF++ ) {
				*toF = *fromF;
			}
		} else {
			for ( n = 0; n < field->numElements; n++, toF++ ) {
				*toF = 0;
			}
		}
	}

	if ( print ) {
		if ( msg->bit == 0 ) {
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		} else {
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}
		Com_Printf( " (%i bits)\n", endBit - startBit );
	}
}

/*
=============================================================================

entityState_t communication
  
=============================================================================
*/

/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is NULL, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
void MSG_WriteDeltaEntity( msg_t *msg, sharedEntityState_t *from, sharedEntityState_t *to,
						   qboolean force ) {
	int			lc;

	if ( !msg_entityStateFields.fields ) {
		Com_Error( ERR_DROP, "entityState_t missing netFields" );
	}

	// a NULL to is a delta remove message
	if ( to == NULL ) {
		if ( from == NULL ) {
			return;
		}
		MSG_WriteBits( msg, from->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 1, 1 );
		return;
	}

	if ( to->number < 0 || to->number >= MAX_GENTITIES ) {
		Com_Error (ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number );
	}

	lc = MSG_LastChangedField( from, to, &msg_entityStateFields );

	if ( lc == 0 ) {
		// nothing at all changed
		if ( !force ) {
			return;		// nothing at all
		}
		// write two bits for no change
		MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 0, 1 );		// not removed
		MSG_WriteBits( msg, 0, 1 );		// no delta
		return;
	}

	MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
	MSG_WriteBits( msg, 0, 1 );			// not removed
	MSG_WriteBits( msg, 1, 1 );			// we have a delta

	MSG_WriteByte( msg, lc );	// # of changes

	MSG_WriteDeltaNetFields( msg, from, to, &msg_entityStateFields, lc );
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
==================
*/
void MSG_ReadDeltaEntity( msg_t *msg, sharedEntityState_t *from, sharedEntityState_t *to, 
						 int number) {
	int			lc;
	int			print;
	int			startBit;

	if ( !msg_entityStateFields.fields ) {
		Com_Error( ERR_DROP, "entityState_t missing netFields" );
	}

	if ( number < 0 || number >= MAX_GENTITIES) {
		Com_Error( ERR_DROP, "Bad delta entity number: %i", number );
	}

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// check for a remove
	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		Com_Memset( to, 0, sizeof( *to ) );	
		to->number = MAX_GENTITIES - 1;
		if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) ) {
			Com_Printf( "%3i: #%-3i remove\n", msg->readcount, number );
		}
		return;
	}

	// check for no delta
	if ( MSG_ReadBits( msg, 1 ) == 0 ) {
		if ( from )
			Com_Memcpy(to, from, msg_entityStateFields.objectSize );
		else
			Com_Memset(to, 0, msg_entityStateFields.objectSize );

		to->number = number;
		return;
	}

	lc = MSG_ReadByte(msg);

	if ( lc > msg_entityStateFields.numFields || lc < 0 ) {
		Com_Error( ERR_DROP, "invalid entityState field count" );
	}

	// shownet 2/3 will interleave with other printed info, -1 will
	// just print the delta records`
	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) ) {
		print = 1;
		Com_Printf( "%3i: #%-3i ", msg->readcount, to->number );
	} else {
		print = 0;
	}

	to->number = number;

	MSG_ReadDeltaNetFields( msg, from, to, &msg_entityStateFields, lc, startBit, print );
}


/*
============================================================================

plyer_state_t communication

============================================================================
*/

/*
=============
MSG_WriteDeltaPlayerstate

=============
*/
void MSG_WriteDeltaPlayerstate( msg_t *msg, sharedPlayerState_t *from, sharedPlayerState_t *to ) {
	int				lc;

	if ( !msg_playerStateFields.fields ) {
		Com_Error( ERR_DROP, "playerState_t missing netFields" );
	}

	lc = MSG_LastChangedField( from, to, &msg_playerStateFields );

	MSG_WriteByte( msg, lc );	// # of changes

	MSG_WriteDeltaNetFields( msg, from, to, &msg_playerStateFields, lc );
}


/*
===================
MSG_ReadDeltaPlayerstate
===================
*/
void MSG_ReadDeltaPlayerstate( msg_t *msg, sharedPlayerState_t *from, sharedPlayerState_t *to, int number ) {
	int			lc;
	int			startBit;
	int			print;

	if ( !msg_playerStateFields.fields ) {
		Com_Error( ERR_DROP, "playerState_t missing netFields" );
	}

	if ( msg->bit == 0 ) {
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	} else {
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) ) {
		print = 1;
		Com_Printf( "%3i: playerstate #%-i3", msg->readcount, number );
	} else {
		print = 0;
	}

	lc = MSG_ReadByte(msg);

	if ( lc > msg_playerStateFields.numFields || lc < 0 ) {
		Com_Error( ERR_DROP, "invalid playerState field count" );
	}

	MSG_ReadDeltaNetFields( msg, from, to, &msg_playerStateFields, lc, startBit, print );
}

int msg_hData[256] = {
250315,			// 0
41193,			// 1
6292,			// 2
7106,			// 3
3730,			// 4
3750,			// 5
6110,			// 6
23283,			// 7
33317,			// 8
6950,			// 9
7838,			// 10
9714,			// 11
9257,			// 12
17259,			// 13
3949,			// 14
1778,			// 15
8288,			// 16
1604,			// 17
1590,			// 18
1663,			// 19
1100,			// 20
1213,			// 21
1238,			// 22
1134,			// 23
1749,			// 24
1059,			// 25
1246,			// 26
1149,			// 27
1273,			// 28
4486,			// 29
2805,			// 30
3472,			// 31
21819,			// 32
1159,			// 33
1670,			// 34
1066,			// 35
1043,			// 36
1012,			// 37
1053,			// 38
1070,			// 39
1726,			// 40
888,			// 41
1180,			// 42
850,			// 43
960,			// 44
780,			// 45
1752,			// 46
3296,			// 47
10630,			// 48
4514,			// 49
5881,			// 50
2685,			// 51
4650,			// 52
3837,			// 53
2093,			// 54
1867,			// 55
2584,			// 56
1949,			// 57
1972,			// 58
940,			// 59
1134,			// 60
1788,			// 61
1670,			// 62
1206,			// 63
5719,			// 64
6128,			// 65
7222,			// 66
6654,			// 67
3710,			// 68
3795,			// 69
1492,			// 70
1524,			// 71
2215,			// 72
1140,			// 73
1355,			// 74
971,			// 75
2180,			// 76
1248,			// 77
1328,			// 78
1195,			// 79
1770,			// 80
1078,			// 81
1264,			// 82
1266,			// 83
1168,			// 84
965,			// 85
1155,			// 86
1186,			// 87
1347,			// 88
1228,			// 89
1529,			// 90
1600,			// 91
2617,			// 92
2048,			// 93
2546,			// 94
3275,			// 95
2410,			// 96
3585,			// 97
2504,			// 98
2800,			// 99
2675,			// 100
6146,			// 101
3663,			// 102
2840,			// 103
14253,			// 104
3164,			// 105
2221,			// 106
1687,			// 107
3208,			// 108
2739,			// 109
3512,			// 110
4796,			// 111
4091,			// 112
3515,			// 113
5288,			// 114
4016,			// 115
7937,			// 116
6031,			// 117
5360,			// 118
3924,			// 119
4892,			// 120
3743,			// 121
4566,			// 122
4807,			// 123
5852,			// 124
6400,			// 125
6225,			// 126
8291,			// 127
23243,			// 128
7838,			// 129
7073,			// 130
8935,			// 131
5437,			// 132
4483,			// 133
3641,			// 134
5256,			// 135
5312,			// 136
5328,			// 137
5370,			// 138
3492,			// 139
2458,			// 140
1694,			// 141
1821,			// 142
2121,			// 143
1916,			// 144
1149,			// 145
1516,			// 146
1367,			// 147
1236,			// 148
1029,			// 149
1258,			// 150
1104,			// 151
1245,			// 152
1006,			// 153
1149,			// 154
1025,			// 155
1241,			// 156
952,			// 157
1287,			// 158
997,			// 159
1713,			// 160
1009,			// 161
1187,			// 162
879,			// 163
1099,			// 164
929,			// 165
1078,			// 166
951,			// 167
1656,			// 168
930,			// 169
1153,			// 170
1030,			// 171
1262,			// 172
1062,			// 173
1214,			// 174
1060,			// 175
1621,			// 176
930,			// 177
1106,			// 178
912,			// 179
1034,			// 180
892,			// 181
1158,			// 182
990,			// 183
1175,			// 184
850,			// 185
1121,			// 186
903,			// 187
1087,			// 188
920,			// 189
1144,			// 190
1056,			// 191
3462,			// 192
2240,			// 193
4397,			// 194
12136,			// 195
7758,			// 196
1345,			// 197
1307,			// 198
3278,			// 199
1950,			// 200
886,			// 201
1023,			// 202
1112,			// 203
1077,			// 204
1042,			// 205
1061,			// 206
1071,			// 207
1484,			// 208
1001,			// 209
1096,			// 210
915,			// 211
1052,			// 212
995,			// 213
1070,			// 214
876,			// 215
1111,			// 216
851,			// 217
1059,			// 218
805,			// 219
1112,			// 220
923,			// 221
1103,			// 222
817,			// 223
1899,			// 224
1872,			// 225
976,			// 226
841,			// 227
1127,			// 228
956,			// 229
1159,			// 230
950,			// 231
7791,			// 232
954,			// 233
1289,			// 234
933,			// 235
1127,			// 236
3207,			// 237
1020,			// 238
927,			// 239
1355,			// 240
768,			// 241
1040,			// 242
745,			// 243
952,			// 244
805,			// 245
1073,			// 246
740,			// 247
1013,			// 248
805,			// 249
1008,			// 250
796,			// 251
996,			// 252
1057,			// 253
11457,			// 254
13504,			// 255
};

void MSG_initHuffman( void ) {
	int i,j;

	msgInit = qtrue;
	Huff_Init(&msgHuff);
	for(i=0;i<256;i++) {
		for (j=0;j<msg_hData[i];j++) {
			Huff_addRef(&msgHuff.compressor,	(byte)i);			// Do update
			Huff_addRef(&msgHuff.decompressor,	(byte)i);			// Do update
		}
	}
}

/*
void MSG_NUinitHuffman() {
	byte	*data;
	int		size, i, ch;
	int		array[256];

	msgInit = qtrue;

	Huff_Init(&msgHuff);
	// load it in
	size = FS_ReadFile( "netchan/netchan.bin", (void **)&data );

	for(i=0;i<256;i++) {
		array[i] = 0;
	}
	for(i=0;i<size;i++) {
		ch = data[i];
		Huff_addRef(&msgHuff.compressor,	ch);			// Do update
		Huff_addRef(&msgHuff.decompressor,	ch);			// Do update
		array[ch]++;
	}
	Com_Printf("msg_hData {\n");
	for(i=0;i<256;i++) {
		if (array[i] == 0) {
			Huff_addRef(&msgHuff.compressor,	i);			// Do update
			Huff_addRef(&msgHuff.decompressor,	i);			// Do update
		}
		Com_Printf("%d,			// %d\n", array[i], i);
	}
	Com_Printf("};\n");
	FS_FreeFile( data );
	Cbuf_AddText( "condump dump.txt\n" );
}
*/

//===========================================================================

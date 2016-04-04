/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Tremfusion.

Tremfusion is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremfusion is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremfusion; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "tr_common.h"
#define	LL(x) x=LittleLong(x)
typedef byte color4ub_t[4];

#define FOURCC(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define DDS_MAGIC   FOURCC('D','D','S',' ')

// DDS_PIXELFORMAT.dwFlags
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_ALPHA       0x00000002
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040
#define DDPF_YUV         0x00000200
#define DDPF_LUMINANCE   0x00020000

typedef struct {
  unsigned int dwSize;
  unsigned int dwFlags;
  unsigned int dwFourCC;
  unsigned int dwRGBBitCount;
  unsigned int dwRBitMask;
  unsigned int dwGBitMask;
  unsigned int dwBBitMask;
  unsigned int dwABitMask;
} DDS_PIXELFORMAT;

// DDS_HEADER.dwHeaderFlags
#define DDSD_CAPS        0x00000001
#define DDSD_HEIGHT      0x00000002
#define DDSD_WIDTH       0x00000004
#define DDSD_PITCH       0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE  0x00080000
#define DDSD_DEPTH       0x00800000

// DDS_HEADER.dwSurfaceFlags
#define DDSCAPS_COMPLEX 0x00000008
#define DDSCAPS_TEXTURE 0x00001000
#define DDSCAPS_MIPMAP  0x00400000

// DDS_HEADER.dwCubemapFlags
#define DDSCAPS2_CUBEMAP           0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_VOLUME            0x00200000

typedef struct {
  unsigned int    dwSize;
  unsigned int    dwHeaderFlags;
  unsigned int    dwHeight;
  unsigned int    dwWidth;
  unsigned int    dwPitchOrLinearSize;
  unsigned int    dwDepth;
  unsigned int    dwMipMapCount;
  unsigned int    dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  unsigned int    dwSurfaceFlags;
  unsigned int    dwCubemapFlags;
  unsigned int    dwReserved2[3];
} DDS_HEADER;

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN              =  0,

    D3DFMT_R8G8B8               = 20,
    D3DFMT_A8R8G8B8             = 21,
    D3DFMT_X8R8G8B8             = 22,
    D3DFMT_R5G6B5               = 23,
    D3DFMT_X1R5G5B5             = 24,
    D3DFMT_A1R5G5B5             = 25,
    D3DFMT_A4R4G4B4             = 26,
    D3DFMT_R3G3B2               = 27,
    D3DFMT_A8                   = 28,
    D3DFMT_A8R3G3B2             = 29,
    D3DFMT_X4R4G4B4             = 30,
    D3DFMT_A2B10G10R10          = 31,
    D3DFMT_A8B8G8R8             = 32,
    D3DFMT_X8B8G8R8             = 33,
    D3DFMT_G16R16               = 34,
    D3DFMT_A2R10G10B10          = 35,
    D3DFMT_A16B16G16R16         = 36,

    D3DFMT_A8P8                 = 40,
    D3DFMT_P8                   = 41,

    D3DFMT_L8                   = 50,
    D3DFMT_A8L8                 = 51,
    D3DFMT_A4L4                 = 52,

    D3DFMT_V8U8                 = 60,
    D3DFMT_L6V5U5               = 61,
    D3DFMT_X8L8V8U8             = 62,
    D3DFMT_Q8W8V8U8             = 63,
    D3DFMT_V16U16               = 64,
    D3DFMT_A2W10V10U10          = 67,

    D3DFMT_UYVY                 = FOURCC('U','Y','V','Y'),
    D3DFMT_R8G8_B8G8            = FOURCC('R','G','B','G'),
    D3DFMT_YUY2                 = FOURCC('Y','U','Y','2'),
    D3DFMT_G8R8_G8B8            = FOURCC('G','R','G','B'),

    D3DFMT_DXT1                 = FOURCC('D','X','T','1'),
    D3DFMT_DXT2                 = FOURCC('D','X','T','2'),
    D3DFMT_DXT3                 = FOURCC('D','X','T','3'),
    D3DFMT_DXT4                 = FOURCC('D','X','T','4'),
    D3DFMT_DXT5                 = FOURCC('D','X','T','5'),
    D3DFMT_ATI1                 = FOURCC('A','T','I','1'),
    D3DFMT_ATI2                 = FOURCC('A','T','I','2'),
    D3DFMT_DX10                 = FOURCC('D','X','1','0'),

    D3DFMT_D16_LOCKABLE         = 70,
    D3DFMT_D32                  = 71,
    D3DFMT_D15S1                = 73,
    D3DFMT_D24S8                = 75,
    D3DFMT_D24X8                = 77,
    D3DFMT_D24X4S4              = 79,
    D3DFMT_D16                  = 80,

    D3DFMT_D32F_LOCKABLE        = 82,
    D3DFMT_D24FS8               = 83,

    D3DFMT_D32_LOCKABLE         = 84,
    D3DFMT_S8_LOCKABLE          = 85,

    D3DFMT_L16                  = 81,

    D3DFMT_VERTEXDATA           =100,
    D3DFMT_INDEX16              =101,
    D3DFMT_INDEX32              =102,

    D3DFMT_Q16W16V16U16         =110,

    D3DFMT_MULTI2_ARGB8         = FOURCC('M','E','T','1'),

    D3DFMT_R16F                 = 111,
    D3DFMT_G16R16F              = 112,
    D3DFMT_A16B16G16R16F        = 113,

    D3DFMT_R32F                 = 114,
    D3DFMT_G32R32F              = 115,
    D3DFMT_A32B32G32R32F        = 116,

    D3DFMT_CxV8U8               = 117,

    D3DFMT_A1                   = 118,
    D3DFMT_A2B10G10R10_XR_BIAS  = 119,
    D3DFMT_BINARYBUFFER         = 199,

    D3DFMT_FORCE_DWORD          =0x7fffffff
} D3DFORMAT;

typedef enum DXGI_FORMAT {
  DXGI_FORMAT_UNKNOWN                      = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS        = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT           = 2,
  DXGI_FORMAT_R32G32B32A32_UINT            = 3,
  DXGI_FORMAT_R32G32B32A32_SINT            = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS           = 5,
  DXGI_FORMAT_R32G32B32_FLOAT              = 6,
  DXGI_FORMAT_R32G32B32_UINT               = 7,
  DXGI_FORMAT_R32G32B32_SINT               = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS        = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT           = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM           = 11,
  DXGI_FORMAT_R16G16B16A16_UINT            = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM           = 13,
  DXGI_FORMAT_R16G16B16A16_SINT            = 14,
  DXGI_FORMAT_R32G32_TYPELESS              = 15,
  DXGI_FORMAT_R32G32_FLOAT                 = 16,
  DXGI_FORMAT_R32G32_UINT                  = 17,
  DXGI_FORMAT_R32G32_SINT                  = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS            = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT         = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS     = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT      = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS         = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM            = 24,
  DXGI_FORMAT_R10G10B10A2_UINT             = 25,
  DXGI_FORMAT_R11G11B10_FLOAT              = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS            = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM               = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB          = 29,
  DXGI_FORMAT_R8G8B8A8_UINT                = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM               = 31,
  DXGI_FORMAT_R8G8B8A8_SINT                = 32,
  DXGI_FORMAT_R16G16_TYPELESS              = 33,
  DXGI_FORMAT_R16G16_FLOAT                 = 34,
  DXGI_FORMAT_R16G16_UNORM                 = 35,
  DXGI_FORMAT_R16G16_UINT                  = 36,
  DXGI_FORMAT_R16G16_SNORM                 = 37,
  DXGI_FORMAT_R16G16_SINT                  = 38,
  DXGI_FORMAT_R32_TYPELESS                 = 39,
  DXGI_FORMAT_D32_FLOAT                    = 40,
  DXGI_FORMAT_R32_FLOAT                    = 41,
  DXGI_FORMAT_R32_UINT                     = 42,
  DXGI_FORMAT_R32_SINT                     = 43,
  DXGI_FORMAT_R24G8_TYPELESS               = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT            = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS        = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT         = 47,
  DXGI_FORMAT_R8G8_TYPELESS                = 48,
  DXGI_FORMAT_R8G8_UNORM                   = 49,
  DXGI_FORMAT_R8G8_UINT                    = 50,
  DXGI_FORMAT_R8G8_SNORM                   = 51,
  DXGI_FORMAT_R8G8_SINT                    = 52,
  DXGI_FORMAT_R16_TYPELESS                 = 53,
  DXGI_FORMAT_R16_FLOAT                    = 54,
  DXGI_FORMAT_D16_UNORM                    = 55,
  DXGI_FORMAT_R16_UNORM                    = 56,
  DXGI_FORMAT_R16_UINT                     = 57,
  DXGI_FORMAT_R16_SNORM                    = 58,
  DXGI_FORMAT_R16_SINT                     = 59,
  DXGI_FORMAT_R8_TYPELESS                  = 60,
  DXGI_FORMAT_R8_UNORM                     = 61,
  DXGI_FORMAT_R8_UINT                      = 62,
  DXGI_FORMAT_R8_SNORM                     = 63,
  DXGI_FORMAT_R8_SINT                      = 64,
  DXGI_FORMAT_A8_UNORM                     = 65,
  DXGI_FORMAT_R1_UNORM                     = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP           = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM              = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM              = 69,
  DXGI_FORMAT_BC1_TYPELESS                 = 70,
  DXGI_FORMAT_BC1_UNORM                    = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB               = 72,
  DXGI_FORMAT_BC2_TYPELESS                 = 73,
  DXGI_FORMAT_BC2_UNORM                    = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB               = 75,
  DXGI_FORMAT_BC3_TYPELESS                 = 76,
  DXGI_FORMAT_BC3_UNORM                    = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB               = 78,
  DXGI_FORMAT_BC4_TYPELESS                 = 79,
  DXGI_FORMAT_BC4_UNORM                    = 80,
  DXGI_FORMAT_BC4_SNORM                    = 81,
  DXGI_FORMAT_BC5_TYPELESS                 = 82,
  DXGI_FORMAT_BC5_UNORM                    = 83,
  DXGI_FORMAT_BC5_SNORM                    = 84,
  DXGI_FORMAT_B5G6R5_UNORM                 = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM               = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM               = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM               = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM   = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS            = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB          = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS            = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB          = 93,
  DXGI_FORMAT_BC6H_TYPELESS                = 94,
  DXGI_FORMAT_BC6H_UF16                    = 95,
  DXGI_FORMAT_BC6H_SF16                    = 96,
  DXGI_FORMAT_BC7_TYPELESS                 = 97,
  DXGI_FORMAT_BC7_UNORM                    = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB               = 99,
  DXGI_FORMAT_FORCE_UINT                   = 0xffffffffUL 
} DXGI_FORMAT;

typedef enum {
  D3D10_RESOURCE_DIMENSION_UNKNOWN     = 0,
  D3D10_RESOURCE_DIMENSION_BUFFER      = 1,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D   = 2,
  D3D10_RESOURCE_DIMENSION_TEXTURE2D   = 3,
  D3D10_RESOURCE_DIMENSION_TEXTURE3D   = 4 
} D3D10_RESOURCE_DIMENSION;

// DDS_HEADER_DXT10.miscFlag;
#define DDS_RESOURCE_MISC_TEXTURECUBE 0x00000004

typedef struct {
  DXGI_FORMAT              dxgiFormat;
  D3D10_RESOURCE_DIMENSION resourceDimension;
  unsigned int             miscFlag;
  unsigned int             arraySize;
  unsigned int             reserved;
} DDS_HEADER_DXT10;

//
// handler for block-compressed formats (DXT1 and friends)
//
static void SetupBlocks( textureLevel_t **pic, int width, int height,
			 GLuint format, int blockW, int blockH, int blockSize,
			 int mipmaps, byte *data ) {
	textureLevel_t *lvl;
	int i;
	int size = 0;
	int w = width;
	int h = height;

	// compute required image size
	for( i = 0; i < mipmaps; i++ ) {
		int blocksHoriz = (w + blockW - 1) / blockW;
		int blocksVert  = (h + blockH - 1) / blockH;

		size += blocksHoriz * blocksVert * blockSize;

		if( w > 1 ) w >>= 1;
		if( h > 1 ) h >>= 1;
	}

	*pic = lvl = ri.Malloc( mipmaps * sizeof( textureLevel_t ) + size );

	// copy compressed data unchanged
	Com_Memcpy( (byte *)&lvl[mipmaps], data, size );
	data = (byte *)&lvl[mipmaps];

	// setup pointers to each miplevel
	w = width;
	h = height;
	for( i = 0; i < mipmaps; i++ ) {
		int blocksHoriz = (w + blockW - 1) / blockW;
		int blocksVert  = (h + blockH - 1) / blockH;

		size = blocksHoriz * blocksVert * blockSize;

		lvl[i].format = format;
		lvl[i].width = w;
		lvl[i].height = h;
		lvl[i].size = size;
		lvl[i].data = data;

		data += size;

		if( w > 1 ) w >>= 1;
		if( h > 1 ) h >>= 1;
	}
}

//
// handler for linear formats, always unpack into RGBA8
//
static void SetupLinear( textureLevel_t **pic, int width, int height, int depth,
			 int bitCount, int RMask, int GMask, int BMask,
			 int AMask, int mipmaps, byte *base ) {
	textureLevel_t *lvl;
	int i, j, k;
	int size = 0;
	int w = width;
	int h = height;
	color4ub_t *out, *in;
	int RShift, GShift, BShift, AShift;
	int RMult, GMult, BMult, AMult;

	// compute required image size
	for( i = 0; i < mipmaps; i++ ) {
		size += w * h * sizeof(color4ub_t);

		if( w > 1 ) w >>= 1;
		if( h > 1 ) h >>= 1;
	}

	*pic = lvl = ri.Malloc( mipmaps * sizeof( textureLevel_t ) + size * depth );
	out = (color4ub_t *)&lvl[mipmaps];

	if( RMask ) {
		for( RShift = 0; !(RMask & (1 << RShift)); RShift++ );
		RMult = 0xffff / (RMask >> RShift);
	} else {
		RShift = 0;
	}
	if( GMask ) {
		for( GShift = 0; !(GMask & (1 << GShift)); GShift++ );
		GMult = 0xffff / (GMask >> GShift);
	} else {
		GShift = 0;
	}
	if( BMask ) {
		for( BShift = 0; !(BMask & (1 << BShift)); BShift++ );
		BMult = 0xffff / (BMask >> BShift);
	} else {
		BShift = 0;
	}
	if( AMask ) {
		for( AShift = 0; !(AMask & (1 << AShift)); AShift++ );
		AMult = 0xffff / (AMask >> AShift);
	} else {
		AShift = 0;
	}

	w = width;
	h = height;
	for( i = 0; i < mipmaps; i++ ) {
		size = w * h;

		lvl[i].format = GL_RGBA8;
		lvl[i].width = w;
		lvl[i].height = h;
		lvl[i].size = size * sizeof(color4ub_t) * depth;
		lvl[i].data = (byte *)out;

		out += lvl[i].size;

		if( w > 1 ) w >>= 1;
		if( h > 1 ) h >>= 1;
	}

	// ZTM: Tested and works with RGBA8 cubemap without mipmaps
	for( k = 0; k < depth; k++ ) {
		for( i = 0; i < mipmaps; i++ ) {
			uint64_t bits = 0;
			int bitsStored = 0;

			size = lvl[i].width * lvl[i].height;

			out = (color4ub_t *)((byte *)lvl[i].data + k * size * sizeof(color4ub_t));
			in = (color4ub_t *)((byte *)base + k * size * bitCount / 8);

			for( j = 0; j < size; j++ ) {
				while( bitsStored < bitCount ) {
					bits |= ((uint64_t)(*(unsigned int *)in) << bitsStored);
					bitsStored += 32;
					in ++;
				}

				// ZTM: I disabled the bit shift right 8 because the RGBA8 opengl2 cubemap was invisible.
				//     Not sure how this code works exactly, but it was only reading one byte for the whole
				//     image before so who knows what's broke about this code path.
				if( RMask )
					(*out)[0] = (((bits & RMask) >> RShift) * RMult);// >> 8;
				else
					(*out)[0] = 0;
				if( GMask )
					(*out)[1] = (((bits & GMask) >> GShift) * GMult);// >> 8;
				else
					(*out)[1] = 0;
				if( BMask )
					(*out)[2] = (((bits & BMask) >> BShift) * BMult);// >> 8;
				else
					(*out)[2] = 0;
				if( AMask )
					(*out)[3] = (((bits & AMask) >> AShift) * AMult);// >> 8;
				else
					(*out)[3] = 255;

				out ++;

				bits >>= bitCount;
				bitsStored -= bitCount;
			}
		}
	}
}

void R_LoadDDS( const char *name, int *numTexLevels, textureLevel_t **pic )
{
	union {
		byte *b;
		void *v;
	} buffer;
	DDS_HEADER *hdr;
	int length;
	GLuint glFormat;
	int    mipmaps;
	byte  *base;
	int    depth;

	*pic = NULL;
	*numTexLevels = 0;

	//
	// load the file
	//
	length = ri.FS_ReadFile ( ( char * ) name, &buffer.v );
	if ( !buffer.b || length < 0 ) {
		return;
	}

	if( length < 4 || Q_strncmp((char *)buffer.v, "DDS ", 4) ) {
		ri.Error( ERR_DROP, "LoadDDS: Missing DDS signature (%s)", name );
	}

	if( length < 4 + sizeof(DDS_HEADER) ) {
		ri.Error( ERR_DROP, "LoadDDS: DDS header missing (%s)", name );
	}
	hdr = (DDS_HEADER *)(buffer.b + 4);
	LL(hdr->dwSize);
	LL(hdr->dwHeaderFlags);
	LL(hdr->dwHeight);
	LL(hdr->dwWidth);
	LL(hdr->dwMipMapCount);
	LL(hdr->ddspf.dwSize);
	LL(hdr->ddspf.dwFlags);
	LL(hdr->ddspf.dwFourCC);
	LL(hdr->ddspf.dwRGBBitCount);
	LL(hdr->ddspf.dwRBitMask);
	LL(hdr->ddspf.dwGBitMask);
	LL(hdr->ddspf.dwBBitMask);
	LL(hdr->ddspf.dwABitMask);
	LL(hdr->dwSurfaceFlags);
	LL(hdr->dwCubemapFlags);

	if( hdr->dwSize != sizeof( DDS_HEADER ) ||
	    hdr->ddspf.dwSize != sizeof( DDS_PIXELFORMAT ) ) {
		ri.Error( ERR_DROP, "LoadDDS: DDS header missing (%s)", name );
	}

	if ( hdr->dwCubemapFlags & DDSCAPS2_CUBEMAP )
	{
		depth = 6;
	}
	else if ( ( hdr->dwCubemapFlags & DDSCAPS2_VOLUME ) && ( hdr->dwHeaderFlags & DDSD_DEPTH ) )
	{
		// The file can probably(?) be loaded, but will not be uploaded into OpenGL as a 3D texture
		ri.Printf( PRINT_WARNING, "LoadDDS: 3D images are not supported (%s)\n", name );
		depth = hdr->dwDepth;
	}
	else
	{
		depth = 1;
	}

	// analyze the header
	glFormat = 0;
	mipmaps = hdr->dwMipMapCount > 0 ? hdr->dwMipMapCount : 1;
	base = (byte *)(hdr + 1);
	
	if( (hdr->ddspf.dwFlags & DDPF_FOURCC) ) {
		if( hdr->ddspf.dwFourCC == D3DFMT_DX10 ) {
			// has special DX10 header
			DDS_HEADER_DXT10 *hdr10 = (DDS_HEADER_DXT10 *)(hdr + 1);
			base = (byte *)(hdr10 + 1);

			switch( hdr10->dxgiFormat ) {
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x000000ff;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x00ff0000;
				hdr->ddspf.dwABitMask = 0xff000000;
				break;
			case DXGI_FORMAT_A8_UNORM:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 8;
				hdr->ddspf.dwRBitMask = 0;
				hdr->ddspf.dwGBitMask = 0;
				hdr->ddspf.dwBBitMask = 0;
				hdr->ddspf.dwABitMask = 0xff;
				break;
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
			case DXGI_FORMAT_B5G6R5_UNORM:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0xf800;
				hdr->ddspf.dwGBitMask = 0x07e0;
				hdr->ddspf.dwBBitMask = 0x001f;
				hdr->ddspf.dwABitMask = 0;
				break;
			case DXGI_FORMAT_B5G5R5A1_UNORM:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x7c00;
				hdr->ddspf.dwGBitMask = 0x03e0;
				hdr->ddspf.dwBBitMask = 0x001f;
				hdr->ddspf.dwABitMask = 0x8000;
				break;
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x00ff0000;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x000000ff;
				hdr->ddspf.dwABitMask = 0xff000000;
				break;
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x00ff0000;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x000000ff;
				hdr->ddspf.dwABitMask = 0;
				break;
			default:
				ri.Error( ERR_DROP, "LoadDDS: Unsupported DXGI format (%s)", name );
			}
		} else {
			// check if it is one of the DXTn formats
			switch( hdr->ddspf.dwFourCC ) {
			case D3DFMT_DXT1:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;
			case D3DFMT_DXT2:
			case D3DFMT_DXT3:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;
			case D3DFMT_DXT4:
			case D3DFMT_DXT5:
				glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;
// should we support compressed luminance-alpha formats too ?
#ifdef GL_COMPRESSED_LUMINANCE_LATC1_EXT
			case D3DFMT_ATI1:
				glFormat = GL_COMPRESSED_LUMINANCE_LATC1_EXT;
				break;
			case D3DFMT_ATI2:
				glFormat = GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
				break;
#endif
			case D3DFMT_R8G8B8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 24;
				hdr->ddspf.dwRBitMask = 0xff0000;
				hdr->ddspf.dwGBitMask = 0x00ff00;
				hdr->ddspf.dwBBitMask = 0x0000ff;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_A8R8G8B8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x00ff0000;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x000000ff;
				hdr->ddspf.dwABitMask = 0xff000000;
				break;
			case D3DFMT_X8R8G8B8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x00ff0000;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x000000ff;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_R5G6B5:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0xf800;
				hdr->ddspf.dwGBitMask = 0x07e0;
				hdr->ddspf.dwBBitMask = 0x001f;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_X1R5G5B5:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x7c00;
				hdr->ddspf.dwGBitMask = 0x03e0;
				hdr->ddspf.dwBBitMask = 0x001f;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_A1R5G5B5:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x7c00;
				hdr->ddspf.dwGBitMask = 0x03e0;
				hdr->ddspf.dwBBitMask = 0x001f;
				hdr->ddspf.dwABitMask = 0x8000;
				break;
			case D3DFMT_A4R4G4B4:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x0f00;
				hdr->ddspf.dwGBitMask = 0x00f0;
				hdr->ddspf.dwBBitMask = 0x000f;
				hdr->ddspf.dwABitMask = 0xf000;
				break;
			case D3DFMT_R3G3B2:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 8;
				hdr->ddspf.dwRBitMask = 0xe0;
				hdr->ddspf.dwGBitMask = 0x1c;
				hdr->ddspf.dwBBitMask = 0x03;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_A8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 8;
				hdr->ddspf.dwRBitMask = 0;
				hdr->ddspf.dwGBitMask = 0;
				hdr->ddspf.dwBBitMask = 0;
				hdr->ddspf.dwABitMask = 0xff;
				break;
			case D3DFMT_A8R3G3B2:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x00e0;
				hdr->ddspf.dwGBitMask = 0x001c;
				hdr->ddspf.dwBBitMask = 0x0003;
				hdr->ddspf.dwABitMask = 0xff00;
				break;
			case D3DFMT_X4R4G4B4:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x0f00;
				hdr->ddspf.dwGBitMask = 0x00f0;
				hdr->ddspf.dwBBitMask = 0x000f;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_A8B8G8R8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x000000ff;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x00ff0000;
				hdr->ddspf.dwABitMask = 0xff000000;
				break;
			case D3DFMT_X8B8G8R8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 32;
				hdr->ddspf.dwRBitMask = 0x000000ff;
				hdr->ddspf.dwGBitMask = 0x0000ff00;
				hdr->ddspf.dwBBitMask = 0x00ff0000;
				hdr->ddspf.dwABitMask = 0;
				break;

			case D3DFMT_L8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 8;
				hdr->ddspf.dwRBitMask = 0xff;
				hdr->ddspf.dwGBitMask = 0xff;
				hdr->ddspf.dwBBitMask = 0xff;
				hdr->ddspf.dwABitMask = 0;
				break;
			case D3DFMT_A8L8:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 16;
				hdr->ddspf.dwRBitMask = 0x00ff;
				hdr->ddspf.dwGBitMask = 0x00ff;
				hdr->ddspf.dwBBitMask = 0x00ff;
				hdr->ddspf.dwABitMask = 0xff00;
				break;
			case D3DFMT_A4L4:
				glFormat = GL_RGBA8;
				hdr->ddspf.dwRGBBitCount = 8;
				hdr->ddspf.dwRBitMask = 0x0f;
				hdr->ddspf.dwGBitMask = 0x0f;
				hdr->ddspf.dwBBitMask = 0x0f;
				hdr->ddspf.dwABitMask = 0xf0;
				break;
			default:
				ri.Error( ERR_DROP, "LoadDDS: Unsupported texture format (%s)", name );
				break;
			}
		}
	} else {
		// linear data
		glFormat = GL_RGBA8;
	}

	if ( !qglCompressedTexImage2DARB && (
			glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
			glFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
			glFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ) ) {
		ri.Printf( PRINT_WARNING, "LoadDDS: DXTn decompression is not supported by GPU driver (%s)\n", name );
	} else {
		switch( glFormat ) {
		case GL_RGBA8:
			SetupLinear( pic, hdr->dwWidth, hdr->dwHeight, depth,
					 hdr->ddspf.dwRGBBitCount,
					 hdr->ddspf.dwRBitMask, hdr->ddspf.dwGBitMask,
					 hdr->ddspf.dwBBitMask, hdr->ddspf.dwABitMask,
					 mipmaps, base );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			SetupBlocks( pic, hdr->dwWidth, hdr->dwHeight, glFormat,
					 4, 4, 8, mipmaps, base );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			SetupBlocks( pic, hdr->dwWidth, hdr->dwHeight, glFormat,
					 4, 4, 16, mipmaps, base );
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			SetupBlocks( pic, hdr->dwWidth, hdr->dwHeight, glFormat,
					 4, 4, 16, mipmaps, base );
			break;
		default:
			ri.Error( ERR_DROP, "LoadDDS: GL format %x not supported (%s)", glFormat, name );
			break;
		}

		*numTexLevels = mipmaps;
	}

#if 0
	// debug code for exporting cubemap to TGA
	extern void RE_SaveTGA_EXT(char * filename, int image_width, int image_height, int bytesPerPixel, byte *image_buffer, int padding);
	int i;
	for ( i = 0; i < depth; i++ ) {
		char filename[128];
		Com_sprintf( filename, sizeof(filename), "%s_%d.tga", name, i );

		RE_SaveTGA_EXT(filename, (*pic)[0].width, (*pic)[0].height, 4, (*pic)[0].data + (*pic)[0].width * (*pic)[0].height * 4 * i, 0);
	}
#endif

	ri.FS_FreeFile (buffer.v);
}

// ZTM: TODO: Fix saving DDS for big endianess
void R_SaveDDS(const char *filename, byte *pic, int width, int height, int depth)
{
	byte *data;
	DDS_HEADER *ddsHeader;
	int picSize, size;

	if (!depth) {
		depth = 1;
	}

	picSize = width * height * depth * 4;
	size = 4 + sizeof(*ddsHeader) + picSize;
	data = ri.Malloc(size);

	data[0] = 'D';
	data[1] = 'D';
	data[2] = 'S';
	data[3] = ' ';

	ddsHeader = (DDS_HEADER *)(data + 4);
	memset(ddsHeader, 0, sizeof(*ddsHeader));

	ddsHeader->dwSize = 0x7c;
	ddsHeader->dwHeaderFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsHeader->dwHeight = height;
	ddsHeader->dwWidth = width;
	ddsHeader->ddspf.dwSize = 0x00000020;
	ddsHeader->dwSurfaceFlags = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE;

	if (depth == 6) {
		ddsHeader->dwCubemapFlags = DDSCAPS2_CUBEMAP |
									DDSCAPS2_CUBEMAP_POSITIVEX |
									DDSCAPS2_CUBEMAP_NEGATIVEX |
									DDSCAPS2_CUBEMAP_POSITIVEY |
									DDSCAPS2_CUBEMAP_NEGATIVEY |
									DDSCAPS2_CUBEMAP_POSITIVEZ |
									DDSCAPS2_CUBEMAP_NEGATIVEZ;
	}

	ddsHeader->ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	ddsHeader->ddspf.dwRGBBitCount = 32;
	ddsHeader->ddspf.dwRBitMask = 0x000000ff;
	ddsHeader->ddspf.dwGBitMask = 0x0000ff00;
	ddsHeader->ddspf.dwBBitMask = 0x00ff0000;
	ddsHeader->ddspf.dwABitMask = 0xff000000;

	Com_Memcpy(data + 4 + sizeof(*ddsHeader), pic, picSize);

	ri.FS_WriteFile(filename, data, size);

	ri.Free(data);
}

/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "nrrd.h"

char
nrrdTypeConv[NRRD_TYPE_MAX+1][AIR_STRLEN_SMALL] = {
  "%*d",  /* what else? sscanf: skip, printf: use "minimum precision" */
  "%d",
  "%u",
  "%hd",
  "%hu",
  "%d",
  "%u",
  AIR_LLONG_FMT,
  AIR_ULLONG_FMT,
  "%f",
  "%lf",
  "%*d"  /* what else? */
};

/*
** the setting of NRRD_TYPE_BIGGEST has to be in accordance with this
*/
int 
nrrdTypeSize[NRRD_TYPE_MAX+1] = {
  0,  /* unknown */
  1,  /* char */
  1,  /* unsigned char */
  2,  /* short */
  2,  /* unsigned short */
  4,  /* int */
  4,  /* unsigned int */
  8,  /* long long */
  8,  /* unsigned long long */
  4,  /* float */
  8,  /* double */
  0   /* effectively unknown; user has to set explicitly */
};

int 
nrrdTypeFixed[NRRD_TYPE_MAX+1] = {
  0,  /* unknown */
  1,  /* char */
  1,  /* unsigned char */
  1,  /* short */
  1,  /* unsigned short */
  1,  /* int */
  1,  /* unsigned int */
  1,  /* long long */
  1,  /* unsigned long long */
  0,  /* float */
  0,  /* double */
  1   /* for some reason we pretend that blocks are fixed point */
};

int 
nrrdTypeUnsigned[NRRD_TYPE_MAX+1] = {
  0,  /* unknown */
  0,  /* char */
  1,  /* unsigned char */
  0,  /* short */
  1,  /* unsigned short */
  0,  /* int */
  1,  /* unsigned int */
  0,  /* long long */
  1,  /* unsigned long long */
  0,  /* float */
  0,  /* double */
  0   /* for some reason we pretend that blocks are signed */
};

/*
******** nrrdTypeMin[]
******** nrrdTypeMax[]
**
** only intended for small (<= 32 bits) fixed-point type,
** so that we know how to "unquantize" fixed-point values.
** A 64-bit double can correctly store the 32-bit integral
** mins and maxs, but gets the last few places wrong in the
** 64-bit mins and max.
*/
double
nrrdTypeMin[NRRD_TYPE_MAX+1] = {
  0,               /* unknown */
  SCHAR_MIN,       /* char */
  0,               /* unsigned char */
  SHRT_MIN,        /* short */
  0,               /* unsigned short */
  INT_MIN,         /* int */
  0,               /* unsigned int */
  NRRD_LLONG_MIN,  /* long long */
  0,               /* unsigned long long */
  0,               /* float */
  0,               /* double */
  0                /* punt */
},
nrrdTypeMax[NRRD_TYPE_MAX+1] = {
  0,               /* unknown */
  SCHAR_MAX,       /* char */
  UCHAR_MAX,       /* unsigned char */
  SHRT_MAX,        /* short */
  USHRT_MAX,       /* unsigned short */
  INT_MAX,         /* int */
  UINT_MAX,        /* unsigned int */
  NRRD_LLONG_MAX,  /* long long */
  NRRD_ULLONG_MAX, /* unsigned long long */
  0,               /* float */
  0,               /* double */
  0                /* punt */
};

/*
******** nrrdTypeNumberValues[]
**
** only meaningful for fixed point values, and only correct for
** 32-bit values; tells the number of different integral values that
** can be represented by the type
*/
double
nrrdTypeNumberValues[NRRD_TYPE_MAX+1] = {
  0,                         /* unknown */
  UCHAR_MAX+1,               /* char */
  UCHAR_MAX+1,               /* unsigned char */
  USHRT_MAX+1,               /* short */
  USHRT_MAX+1,               /* unsigned short */
  (double)UINT_MAX+1,        /* int */
  (double)UINT_MAX+1,        /* unsigned int */
  (double)NRRD_ULLONG_MAX+1, /* long long */
  (double)NRRD_ULLONG_MAX+1, /* unsigned long long */
  0,                         /* float */
  0,                         /* double */
  0                          /* punt */
};

/*
** _nrrdFieldValidInPNM[]
**
** these fields are valid embedded in PNM comments
** This does NOT include the fields who's values are constrained
** the PNM format/magic itself.
*/
int
_nrrdFieldValidInPNM[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  1, /* nrrdField_comment */
  1, /* nrrdField_content */
  0, /* nrrdField_number */
  0, /* nrrdField_type */
  0, /* nrrdField_block_size */
  0, /* nrrdField_dimension */
  0, /* nrrdField_sizes */
  1, /* nrrdField_spacings */
  1, /* nrrdField_axis_mins */
  1, /* nrrdField_axis_maxs */
  1, /* nrrdField_centers */
  1, /* nrrdField_labels */
  1, /* nrrdField_min */
  1, /* nrrdField_max */
  1, /* nrrdField_old_min */
  1, /* nrrdField_old_max */
  0, /* nrrdField_data_file */
  0, /* nrrdField_endian */
  0, /* nrrdField_encoding */
  0, /* nrrdField_line_skip */
  0  /* nrrdField_byte_skip */
};

/*
** _nrrdFieldOnePerAxis
** 
** whether or not you need one value per axis, like labels and spacings
*/
int
_nrrdFieldOnePerAxis[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  0, /* nrrdField_comment */
  0, /* nrrdField_content */
  0, /* nrrdField_number */
  0, /* nrrdField_type */
  0, /* nrrdField_block_size */
  0, /* nrrdField_dimension */
  1, /* nrrdField_sizes */
  1, /* nrrdField_spacings */
  1, /* nrrdField_axis_mins */
  1, /* nrrdField_axis_maxs */
  1, /* nrrdField_centers */
  1, /* nrrdField_labels */
  0, /* nrrdField_min */
  0, /* nrrdField_max */
  0, /* nrrdField_old_min */
  0, /* nrrdField_old_max */
  0, /* nrrdField_data_file */
  0, /* nrrdField_endian */
  0, /* nrrdField_encoding */
  0, /* nrrdField_line_skip */
  0  /* nrrdField_byte_skip */
};

/*
** _nrrdFieldValidInTable[]
** 
** these fields are valid embedded in table comments
** This does NOT include the fields who's values are constrained
** the table format itself.
*/
int
_nrrdFieldValidInTable[NRRD_FIELD_MAX+1] = {
  0, /* nrrdField_unknown */
  1, /* nrrdField_comment */
  1, /* nrrdField_content */
  0, /* nrrdField_number */
  0, /* nrrdField_type: decided AGAINST table holding general type 
	(but I forget why ...) */
  0, /* nrrdField_block_size */
  1, /* nrrdField_dimension: but can only be 1 or 2 */
  0, /* nrrdField_sizes */
  1, /* nrrdField_spacings */
  1, /* nrrdField_axis_mins */
  1, /* nrrdField_axis_maxs */
  1, /* nrrdField_centers */
  1, /* nrrdField_labels */
  1, /* nrrdField_min */
  1, /* nrrdField_max */
  1, /* nrrdField_old_min */
  1, /* nrrdField_old_max */
  0, /* nrrdField_data_file */
  0, /* nrrdField_endian */
  0, /* nrrdField_encoding */
  0, /* nrrdField_line_skip */
  0  /* nrrdField_byte_skip */
};

/*
** _nrrdFieldRequired[]
**
** regardless of whether its a nrrd, PNM, or table, these things
** need to be conveyed, either explicity or implicitly
*/
int
_nrrdFieldRequired[NRRD_FIELD_MAX+1] = {
  0, /* "Ernesto \"Che\" Guevara" */
  0, /* "#" */
  0, /* "content" */
  0, /* "number" */
  1, /* "type" */
  0, /* "block size" */
  1, /* "dimension" */
  1, /* "sizes" */
  0, /* "spacings" */
  0, /* "axis mins" */
  0, /* "axis maxs" */
  0, /* "centers" */
  0, /* "labels" */
  0, /* "min" */
  0, /* "max" */
  0, /* "old min" */
  0, /* "old max" */
  0, /* "data file" */
  0, /* "endian" */
  1, /* "encoding" */
  0, /* "line skip" */
  0  /* "byte skip" */
};

/*
******** nrrdEncodingEndianMatters[]
** 
** tells if given encoding exposes endianness of architecture
*/
int 
nrrdEncodingEndianMatters[NRRD_ENCODING_MAX+1] = {
  0,   /* unknown */
  1,   /* raw */
  0,   /* ascii */
#ifdef TEEM_ZLIB
  1,   /* gzip */
#endif
};

/*
******** nrrdEncodingCompresion[]
** 
** tells if given encoding is compression, which changes the
** semantics of "byte skip"
*/
int 
nrrdEncodingCompression[NRRD_ENCODING_MAX+1] = {
  0,   /* unknown */
  0,   /* raw */
  0,   /* ascii */
#ifdef TEEM_ZLIB
  1,   /* gzip */
#endif
};

/*
** _nrrdFormatUsesDIO[]
**
** whether or not try using direct I/O for a given format
*/
int
_nrrdFormatUsesDIO[NRRD_FORMAT_MAX+1] = {
  0,   /* nrrdFormatUnknown */
  1,   /* nrrdFormatNRRD */
  0,   /* nrrdFormatPNM */
  0    /* nrrdFormatTable */
};

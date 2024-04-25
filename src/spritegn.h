//
// spritegn.h: header file for sprite generation program
//

// **********************************************************
// * This file must be identical in the spritegen directory *
// * and in the Quake directory, because it's used to       *
// * pass data from one to the other via .spr files.        *
// **********************************************************

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#ifdef INCLUDELIBS

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdlib.h"
#include "dictlib.h"
#include "lbmlib.h"
#include "mathlib.h"
#include "scriplib.h"
#include "trilib.h"

#endif

#define SPRITE_VERSION 1

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum { ST_SYNC = 0, ST_RAND } synctype_t;
#endif

// TODO: shorten these?
typedef struct {
  int32_t ident;
  int32_t version;
  int32_t type;
  float boundingradius;
  int32_t width;
  int32_t height;
  int32_t numframes;
  float beamlength;
  synctype_t synctype;
} dsprite_t;

#define SPR_VP_PARALLEL_UPRIGHT 0
#define SPR_FACING_UPRIGHT 1
#define SPR_VP_PARALLEL 2
#define SPR_ORIENTED 3
#define SPR_VP_PARALLEL_ORIENTED 4

typedef struct {
  int32_t origin[2];
  int32_t width;
  int32_t height;
} dspriteframe_t;

typedef struct {
  int32_t numframes;
} dspritegroup_t;

typedef struct {
  float interval;
} dspriteinterval_t;

typedef enum { SPR_SINGLE = 0, SPR_GROUP } spriteframetype_t;

typedef struct {
  spriteframetype_t type;
} dspriteframetype_t;

#define IDSPRITEHEADER (('P' << 24) + ('S' << 16) + ('D' << 8) + 'I')
// little-endian "IDSP"

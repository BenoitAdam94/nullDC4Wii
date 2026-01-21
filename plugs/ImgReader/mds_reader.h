#pragma once
#include "ImgReader.h"
#include <stdio.h>
#include <string.h>

typedef struct _track {
	int track;
	int mode;
	int flags;

	int pmin;
	int psec;
	int pfrm;

	int sectorsize;
	int sector;
	int sectors;
	int pregap;
	s64 offset;
} strack;

typedef struct _session
{
	int session;
	int pregap;
	int sectors;
	int datablocks;
	int leadinblocks;
	int last_track;

	int something1;
	int something2;

	int datablocks_offset;
	int extrablocks_offset;

	strack tracks[10];
	int ntracks;
} session;

extern session sessions[5];
extern int nsessions;

bool parse_mds(wchar *mds_filename,bool verbose);
bool parse_nrg(wchar *nrg_filename,bool verbose);

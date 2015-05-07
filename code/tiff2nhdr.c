/*
** tiff2nhdr: TIFF to detached nrrd header converter
** Copyright (C) 2015  University of Chicago
** Author: Gordon Kindlmann
**
** First pass at TIFF to nrrd converter, which permits (if all goes well)
** nrrdLoad to extract data from a TIFF file.  Compile with:

gcc -O2 -o tiff2nhdr tiff2nhdr.c -I $TEEM/include -L $TEEM/lib  -lteem -ltiff

** where $TEEM is path into teem-install, as compiled via instructions
** at http://teem.sourceforge.net/build.html
** and libtiff http://www.libtiff.org has been installed so that gcc can
** find it.
**
** Run the program with:

./tiff2nhdr -i image.tif -o image.nhdr

**
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiffio.h"
#include "teem/meet.h"

const char t2nInfo[] =
  ("self-contained utility for trying to wrap a detached NRRD header around "
   "a TIFF file.  Not yet good for 3D images (time-series). ");

int
main(int argc, const char* argv[]) {
  const char *me;
  airArray *mop;
  hestParm *hparm;
  hestOpt *hopt = NULL;

  char *iname, *oname;

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  airMopAdd(mop, hparm, AIR_CAST(airMopper, hestParmFree), airMopAlways);
  hparm->elideSingleOtherType = AIR_TRUE;
  hestOptAdd(&hopt, "i", "tiff", airTypeString, 1, 1, &iname, NULL,
             "input tiff image filename");
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &oname, NULL,
             "output nhdr filename");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, t2nInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestOptFree), airMopAlways);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestParseFree), airMopAlways);

  /* with geotiff files, you'll get:
     TIFFReadDirectory: Warning, Unknown field with tag 33550 (0x830e) encountered.
     TIFFReadDirectory: Warning, Unknown field with tag 33922 (0x8482) encountered.
     TIFFReadDirectory: Warning, Unknown field with tag 34735 (0x87af) encountered.
     TIFFReadDirectory: Warning, Unknown field with tag 34736 (0x87b0) encountered.
     TIFFReadDirectory: Warning, Unknown field with tag 34737 (0x87b1) encountered.
     and the following line silences all warnigns */
  TIFFSetWarningHandler(NULL);

  TIFF* tif = TIFFOpen(iname, "r");
  if (!tif) {
    TIFFError(NULL, "");
    fprintf(stderr, "%s: can't open \"%s\" as TIFF\n", me, iname);
    airMopError(mop); return 1;
  }
  /* GLK can't figure out how to get this information from
     the TIFF*tif; so we'll get it right from the file */
  unsigned short magic;
  FILE *fil = fopen(iname, "r");
  if (!fil) {
    fprintf(stderr, "%s: can't open file \"%s\"\n", me, iname);
    airMopError(mop); return 1;
  }
  fread(&magic, sizeof(short), 1, fil);
  fclose(fil);

  uint16 compression;
  TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
  if (compression != COMPRESSION_NONE) {
    fprintf(stderr, "%s: sorry non-trivial compression can't be handled\n", me);
    airMopError(mop); return 1;
  }

  uint32 width, length, depth, rowsPerStrip;
  uint16 sampleFormat, bitsPerSample;
  if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) ||
      !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length) ||
      !TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip) ||
      !TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample) ||
      !TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat)) {
    fprintf(stderr, "%s: couldn't get basic fields\n", me);
    airMopError(mop); return 1;
  }
  if (!TIFFGetField(tif, TIFFTAG_IMAGEDEPTH, &depth)) {
    /* couldn't get depth, so its probably 0, or 1 */
    depth = 0;
  } else {
    fprintf(stderr, "%s: sorry can't handle depth %u\n", me, depth);
    airMopError(mop); return 1;
  }

  /* http://www.awaresystems.be/imaging/tiff/tifftags/sampleformat.html */
  if (!( SAMPLEFORMAT_UINT == sampleFormat ||
         SAMPLEFORMAT_INT == sampleFormat ||
         SAMPLEFORMAT_IEEEFP == sampleFormat )) {
    fprintf(stderr, "%s: sampleFormat %hu unexpected\n", me, sampleFormat);
    airMopError(mop); return 1;
  }
  int ntype;
  if (8 == bitsPerSample) {
    if (SAMPLEFORMAT_UINT == sampleFormat) {
      ntype = nrrdTypeUChar;
    } else if (SAMPLEFORMAT_INT == sampleFormat) {
      ntype = nrrdTypeChar;
    } else {
      fprintf(stderr, "%s: 8-bit SAMPLEFORMAT_IEEEFP confusing\n", me);
      airMopError(mop); return 1;
    }
  } else if (16 == bitsPerSample) {
    if (SAMPLEFORMAT_UINT == sampleFormat) {
      ntype = nrrdTypeUShort;
    } else if (SAMPLEFORMAT_INT == sampleFormat) {
      ntype = nrrdTypeShort;
    } else {
      fprintf(stderr, "%s: 16-bit SAMPLEFORMAT_IEEEFP confusing\n", me);
      airMopError(mop); return 1;
    }
  } else if (32 == bitsPerSample) {
    if (SAMPLEFORMAT_UINT == sampleFormat) {
      ntype = nrrdTypeUInt;
    } else if (SAMPLEFORMAT_INT == sampleFormat) {
      ntype = nrrdTypeInt;
    } else {
      ntype = nrrdTypeFloat;
    }
  } else if (64 == bitsPerSample) {
    if (SAMPLEFORMAT_UINT == sampleFormat) {
      ntype = nrrdTypeULLong;
    } else if (SAMPLEFORMAT_INT == sampleFormat) {
      ntype = nrrdTypeLLong;
    } else {
      ntype = nrrdTypeDouble;
    }
  } else {
    fprintf(stderr, "%s: %hu-bit pixels unexpected\n", me, bitsPerSample);
    airMopError(mop); return 1;
  }

  long *offset=NULL;
  TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, &offset);
  if (!offset) {
    fprintf(stderr, "%s: didn't get allocate offset array\n", me);
    airMopError(mop); return 1;
  }
  uint32 stripNum = TIFFNumberOfStrips(tif);
  uint32 stripSize = TIFFStripSize(tif);
  if (!( stripNum && stripSize )) {
    fprintf(stderr, "%s: strip number (%u) or size (%u) seem wrong\n",
            me, stripNum, stripSize);
    airMopError(mop); return 1;
  }
  uint32 ii;
  for (ii=0; ii<stripNum-1; ii++) {
    if (offset[ii+1] - offset[ii] != stripSize) {
      fprintf(stderr, "%s: strip[%u] start not contiguous with strip[%u] end "
              "(%lu != %u)\n", me, ii+1, ii,
              offset[ii+1] - offset[ii], stripSize);
      airMopError(mop); return 1;
    }
  }

  if (!( fil = airFopen(oname, stdout, "w") )) {
    fprintf(stderr, "%s: couldn't fopen(\"%s\",\"w\"): %s\n", me,
            oname, strerror(errno));
    airMopError(mop); return 1;
  }
  airMopAdd(mop, fil, (airMopper)airFclose, airMopAlways);

  fprintf(fil, "NRRD0004\n");
  fprintf(fil, "dimension: 2\n");
  fprintf(fil, "sizes: %u %u\n", width, length);
  fprintf(fil, "type: %s\n", airEnumStr(nrrdType, ntype));
  fprintf(fil, "encoding: raw\n");
  fprintf(fil, "endian: %s\n", (TIFF_BIGENDIAN == magic
                                ? "big" : "little"));
  fprintf(fil, "byte skip: %lu\n", offset[0]);
  fprintf(fil, "data file: %s\n", iname);

  TIFFClose(tif);
  airMopOkay(mop);
  return 0;
}


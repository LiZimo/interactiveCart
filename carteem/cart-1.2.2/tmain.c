/* Example program to calculate a grid of points for a Gastner-Newman
 * cartogram using the cartogram.c code
 *
 * Written by Mark Newman
 *
 * See http://www.umich.edu/~mejn/ for further details.
 */


#include <stdio.h>
#include <stdlib.h>

#include <teem/meet.h>
#include "tcart.h"


/* The parameter OFFSET specifies a small amount to be added the density in
 * every grid square, as a Fraction of the mean density on the whole
 * lattice.  This prevents negative densities from being generated by
 * numerical errors in the FFTs which can cause problems for the
 * integrator.  If the program is giving weird behavior, particularly at
 * the beginning of a calculation, try increasing this quantity by a factor
 * of 10.
 */

#define OFFSET 0.005


int minElem (int * array, int size) {
  int min_elem = array[0];
  int j;
  for (j = 1; j < size; j++) {
    if (array[j] < min_elem) {min_elem = array[j];}
  }
  return min_elem;
}

int addOffset(double *rho, int xsize, int ysize) {
  int ii;
  double mean;
  double sum=0.0;

  for (ii=0; ii<xsize*ysize; ii++) {
    sum += rho[ii];
  }
  mean = sum/(xsize*ysize);
  for (ii=0; ii<xsize*ysize; ii++) {
    rho[ii] += OFFSET*mean;
  }

  return 0;
}


/* Function to make the grid of points */

void creategrid(double *gridxy, int xsize, int ysize) {
  int ix,iy;
  int i=0;

  for (iy=0; iy<=ysize; iy++) {
    for (ix=0; ix<=xsize; ix++) {
      gridxy[0 + 2*i] = ix;
      gridxy[1 + 2*i] = iy;
      i++;
    }
  }
}

void
findPadSize(unsigned int *sizePad, const unsigned int *sizeOrig, int fff, int noop) {
  static const char me[]="findPadSize";
  //char tmp[5];

  sizePad[0] = sizeOrig[0];
  sizePad[1] = sizeOrig[1];
  if (noop) {
    sizePad[0] = (int) sizeOrig[0];
    sizePad[1] = (int) sizeOrig[1];
  } else if (!fff) {
    sizePad[0] = 1.5*sizeOrig[0];
    sizePad[1] = 1.5*sizeOrig[1];
  } else {
    int i;
    for (i = 0; i<2; i++) {

      int original = (int) (1.5 * ((float) sizeOrig[i]));
      int threePower = 1;
      int remainders[(int) log2(sizePad[0])];
      int rmndr_ind = 0;

      //printf("original, threePower: %i %i\n", original, threePower);
      //scanf("%s", tmp);
      while (threePower < original) {
        remainders[rmndr_ind] = (int) original / threePower;
        threePower *= 3;

        //int test = 800 % 900;
        //printf("remainder, threePower: %i %i \n", remainders[rmndr_ind], threePower);
        //scanf("%s", tmp);

        rmndr_ind++;

      }

      threePower /= 3;
      int potential[rmndr_ind];

      int j;
      for (j = 0; j < rmndr_ind; j ++) {
        int rmndr = remainders[j];
        int twoPower = 1;

        while ( twoPower < rmndr + 1 ) {
          twoPower *= 2;
        }

        potential[j] = (int) twoPower * (int) powf(3, j);
        //printf("potential size: %i %i \n", potential[j], j);
        //scanf("%s", tmp);

      }

      sizePad[i] = minElem(potential, rmndr_ind);
    }
  }
  printf("%s: %d %d\n", me, sizePad[0], sizePad[1]);
  //scanf("%s",tmp);
}

int
applySubst(double *avg, Nrrd *nout, const Nrrd *nsub, const Nrrd *nmap) {
  static const char me[]="applySubst";
  /* these types are checked in main() below */
  const float *sub = (const float*)(nsub->data);
  const unsigned short *map = (const unsigned short *)(nmap->data);
  double *out = (double*)(nout->data);
  unsigned int sx = (unsigned int)nmap->axis[0].size;
  unsigned int sy = (unsigned int)nmap->axis[1].size;
  unsigned int ss = (unsigned int)nsub->axis[1].size;
  unsigned int ii, jj, nn = sx*sy;
  unsigned int posnum;
  double possum;

  posnum = 0;
  possum = 0.0;
  for (ii=0; ii<nn; ii++) {
    unsigned short mval = map[ii];
    double oval=AIR_NAN;
    /* yea, a slow O(n) search, for every pixel;
       this should be optimized if it becomes a bottleneck */
    for (jj=0; jj<ss; jj++) {
      if (mval == (unsigned short)(sub[0 + 2*jj])) {
        oval = sub[1 + 2*jj];
        break;
      }
    }
    if (ss==jj) {
      fprintf(stderr, "%s: map[%u] value %u not found in subst table\n",
              me, ii, mval);
      return 1;
    }
    out[ii] = oval;
    if (oval >= 0) {
      posnum++;
      possum += oval;
    } else {
      if (-1 != oval) {
        fprintf(stderr, "%s: (sub[%u]) map[%u]=%u --> %g < 0 but != -1\n",
                me, jj, ii, mval, oval);
        return 1;
      }
    }
  }
  if (!posnum) {
    fprintf(stderr, "%s: produced no positive values\n", me);
    return 1;
  }
  *avg = possum/posnum;
  return 0;
}

static const char *cartInfo =
  ("heavily Teem-fied and augmented \"cart\", "
   "based on http://www-personal.umich.edu/~mejn/cart/.");

int
main(int argc, const char *argv[]) {
  double *gridxy;  // Array for grid points
  cartContext *ctx;

  const char *me = argv[0];
  airArray *mop = airMopNew();
  hestParm *hparm = hestParmNew();
  hestOpt *hopt = NULL;
  Nrrd *nmap, *nsub, *nrho;
  char *err, *rhoName, *outName;
  unsigned int repeats;
  double time0, time1, temm[4];
  char *wispath;
  FILE *fwise;
  int noop, fftFriendly;

  ctx = cartContextNew();
  airMopAdd(mop, ctx, (airMopper)cartContextNix, airMopAlways);
  airMopAdd(mop, hparm, AIR_CAST(airMopper, hestParmFree), airMopAlways);
  hestOptAdd(&hopt, "i", "map", airTypeOther, 1, 1, &nmap, NULL,
             "output of gdal_rasterize", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "te", "x0 y0 x1 y1", airTypeDouble, 4, 4,
             temm, NULL, "the -te args given to gdal_rasterize");
  hestOptAdd(&hopt, "ff", NULL, airTypeInt, 0, 0, &fftFriendly, NULL,
             "by giving this option, the input image size is optimized "
             "to be fft-friendly (powers of 2 and 3)");
  hestOptAdd(&hopt, "nop", NULL, airTypeInt, 0, 0, &noop, NULL,
             "disable all clever padding");
  hestOptAdd(&hopt, "s", "subst", airTypeOther, 1, 1, &nsub, NULL,
             "substitution table, to apply to \"-i\" map to generate "
             "the initial density map", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "h0", "init h", airTypeDouble, 1, 1,
             &(ctx->initH), "0.001",
             "Initial size of a time-step");
  hestOptAdd(&hopt, "mr", "maxratio", airTypeDouble, 1, 1,
             &(ctx->maxRatio), "4.0",
             "Max ratio to increase step size by");
  hestOptAdd(&hopt, "err", "targErr", airTypeDouble, 1, 1,
             &(ctx->targetError), "0.01",
             "Desired accuracy per step in pixels");
  hestOptAdd(&hopt, "or", "fname", airTypeString, 1, 1, &rhoName, "",
             "if a filename is given here, the computed density map "
             "is saved to this file, to allow inspecting the input "
             "to the diffusion cartogram algorithm");
  hestOptAdd(&hopt, "r", "repeats", airTypeUInt, 1, 1, &repeats, "1",
             "number of times to re-run the computation, just so "
             "that it takes longer and provides an easier target "
             "for profiling tools. Output is not saved until "
             "final iteration.");
  hestOptAdd(&hopt, "v", "verbosity", airTypeInt, 1, 1, &(ctx->verbosity), "0",
             "level of printf verbosity");
  hestOptAdd(&hopt, "snap", "rho vel disp", airTypeBool, 3, 3, ctx->savesnaps,
             "false false false",
             "save snapshots of quantities used for computation: the "
             "scalar density rho, the velocity field, and the displacement");
  hestOptAdd(&hopt, "pr", "rigor", airTypeEnum, 1, 1, &(ctx->rigor), "est",
             "rigor with which fftw plan is constructed. Options are:\n "
             "\b\bo \"e\", \"est\", \"estimate\": only an estimate\n "
             "\b\bo \"m\", \"meas\", \"measure\": standard amount of "
             "measurements of system properties\n "
             "\b\bo \"p\", \"pat\", \"patient\": slower, more measurements\n "
             "\b\bo \"x\", \"ex\", \"exhaustive\": slowest, most measurements",
             NULL, nrrdFFTWPlanRigor);
  hestOptAdd(&hopt, "w", "filename", airTypeString, 1, 1, &wispath, "",
             "A filename here is used to read in fftw \"wisdom\" (if the file "
             "exists already), and is used to save out updated wisdom "
             "after the transform.  By default (not using this option), "
             "no wisdom is read or saved. Note: no wisdom is gained "
             "(that is, learned by fftw) with planning rigor \"estimate\".");
  hestOptAdd(&hopt, "o", "fname", airTypeString, 1, 1, &outName, NULL,
             "output filename");
  hestOptAdd(&hopt, "nn", "nearestneighbor", airTypeInt, 1, 1, &(ctx->nn), "0", "for using nearest neighbor instead of bilinear interpolation");
  hestOptAdd(&hopt, "rk2", "rung-kutta 2nd order", airTypeInt, 1, 1, &(ctx->rk2), "0", "for using 2nd order Rung-Kutta instead of 4th");
  hestParseOrDie(hopt, argc-1, argv+1, hparm,
                 me, cartInfo, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestOptFree), airMopAlways);
  airMopAdd(mop, hopt, AIR_CAST(airMopper, hestParseFree), airMopAlways);

  if (!( 2 == nmap->dim && nrrdTypeUShort == nmap->type )) {
    fprintf(stderr, "%s: want map as 2-D %s array (not %u-D %s)\n", me,
            airEnumStr(nrrdType, nrrdTypeUShort),
            nmap->dim, airEnumStr(nrrdType, nmap->type));
    airMopError(mop);
    return 1;
  }

  if (!( 2 == nsub->dim && 2 == nsub->axis[0].size
         && nrrdTypeFloat == nsub->type )) {
    fprintf(stderr, "%s: want substitution list as 2-D 2-by-N %s array "
            "(not %u-D %u-by-X %s array)\n", me,
            airEnumStr(nrrdType, nrrdTypeFloat), nsub->dim,
            (unsigned int)(nsub->axis[0].size),
            airEnumStr(nrrdType, nsub->type));
    airMopError(mop);
    return 1;
  }

  if (airStrlen(wispath)) {
    fwise = fopen(wispath, "r");
    if (fwise) {
      if (!fftw_import_wisdom_from_file(fwise)) {
        fprintf(stderr, "%s: (couldn't import wisdom from \"%s\"; "
                "will try to save later)\n", me, wispath);
        return 1;
      }
      fclose(fwise);
    } else {
      fprintf(stderr, "%s: (\"%s\" couldn't be opened, will try to save "
              "wisdom afterwards)\n", me, wispath);
    }
  }

  {
    /* (new block to limit scope of "sub") */
    float *sub = (float*)(nsub->data);
    if (!( 0 == sub[0] && -1 == sub[1] )) {
      fprintf(stderr, "%s: substitution list needs to start with \"0 -1\" "
              "(not \"%g %g\")\n", me, sub[0], sub[1]);
      airMopError(mop);
      return 1;
    }
  }
  /* temm=west south east north
     .     0    1    2     3 */
  nmap->axis[0].center = nmap->axis[1].center = nrrdCenterCell;
  nrrdSpaceSet(nmap, nrrdSpaceRightUp);
  nmap->spaceOrigin[0] = temm[0];
  nmap->spaceOrigin[1] = temm[3];
  nmap->axis[0].spaceDirection[0] = (temm[2]-temm[0])/(nmap->axis[0].size-1);
  nmap->axis[0].spaceDirection[1] = 0;
  nmap->axis[1].spaceDirection[0] = 0;
  nmap->axis[1].spaceDirection[1] = (temm[1]-temm[3])/(nmap->axis[1].size-1);
  // nrrdSave("map.nrrd", nmap, NULL);

  unsigned int sizeOrig[2], sizePad[2];
  sizeOrig[0] = (unsigned int)nmap->axis[0].size;
  sizeOrig[1] = (unsigned int)nmap->axis[1].size;

  //printf("%u %u \n", sizeOrig[0], sizeOrig[1]);
  findPadSize(sizePad, sizeOrig, fftFriendly, noop);
  ptrdiff_t padmin[3], padmax[3];
  padmin[0] = -(ptrdiff_t)((sizePad[0] - sizeOrig[0])/2);
  padmin[1] = -(ptrdiff_t)((sizePad[1] - sizeOrig[1])/2);
  padmax[0] = padmin[0] + sizePad[0];
  padmax[1] = padmin[1] + sizePad[1];

  Nrrd *nprerho = nrrdNew();
  airMopAdd(mop, nprerho, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdConvert(nprerho, nmap, nrrdTypeDouble)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problem initializing rho: %s", me, err);
    airMopError(mop);
    return 1;
  }
  double avgRho;
  if (applySubst(&avgRho, nprerho, nsub, nmap)) {
    fprintf(stderr, "%s: problem applying substitution\n", me);
    airMopError(mop);
    return 1;
  }
  nrho = nrrdNew();
  airMopAdd(mop, nrho, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdPad_nva(nrho, nprerho, padmin, padmax,
                  nrrdBoundaryBleed, 0)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problem padding rho: %s", me, err);
    airMopError(mop);
    return 1;
  }
  { /* set background to average density */
    double *rho = (double *)nrho->data;
    unsigned int ii, nn=(unsigned int)nrrdElementNumber(nrho);
    for (ii=0; ii<nn; ii++) {
      if (-1 == rho[ii]) {
        rho[ii] = avgRho;
      }
    }
  }
  if (airStrlen(rhoName)) {
    if (nrrdSave(rhoName, nrho, NULL)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: problem saving density field: %s", me, err);
      airMopError(mop);
      return 1;
    }
  }

  double *rho = (double*)nrho->data;
  int xsize = (int)nrho->axis[0].size;
  int ysize = (int)nrho->axis[1].size;

  /* Allocate space for the cartogram code to use */
  time0 = airTime();
  cart_makews(ctx,xsize,ysize);
  time1 = airTime();
  printf("%s: %g secs for cart_makews\n", me, time1-time0);

  addOffset(rho,xsize,ysize);
  cart_forward(ctx,rho,xsize,ysize);

  /* allocated grid of control points */
  Nrrd *ngrid = nrrdNew();
  airMopAdd(mop, ngrid, (airMopper)nrrdNuke, airMopAlways);
  Nrrd *ntmp = nrrdNew();
  airMopAdd(mop, ntmp, (airMopper)nrrdNuke, airMopAlways);
  padmin[0] = 0; padmax[0] = 1;
  padmin[1] = 0; padmax[1] = xsize;
  padmin[2] = 0; padmax[2] = ysize;

  if (nrrdConvert(ntmp, nrho, nrrdTypeDouble)
      || nrrdAxesInsert(ntmp, ntmp, 0)
      || nrrdPad_nva(ngrid, ntmp, padmin, padmax, nrrdBoundaryBleed, 0)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problem allocating grid: %s", me, err);
    airMopError(mop);
    return 1;
  }
  /* adjust ngrid to be node-centered grid around
     the cell-centered samples of nrho */
  gridxy = (double*)(ngrid->data);
  ngrid->axis[0].center = nrrdCenterUnknown;
  ngrid->axis[1].center = nrrdCenterNode;
  ngrid->axis[2].center = nrrdCenterNode;
  double vx = nrho->axis[0].spaceDirection[0];
  double vy = nrho->axis[0].spaceDirection[1];
  ngrid->spaceOrigin[0] = nrho->spaceOrigin[0] - 0.5*vx;
  ngrid->spaceOrigin[1] = nrho->spaceOrigin[1] - 0.5*vy;
  vx = nrho->axis[1].spaceDirection[0];
  vy = nrho->axis[1].spaceDirection[1];
  ngrid->spaceOrigin[0] -= 0.5*vx;
  ngrid->spaceOrigin[1] -= 0.5*vy;
  ctx->ngrid = ngrid;

  /* Make the cartogram */
  unsigned int repIdx;
  for (repIdx=0; repIdx<repeats; repIdx++) {
    if (repeats > 1) {
      printf("%s: %u/%u begins ... \n", me, repIdx, repeats);
    }
    creategrid(gridxy,xsize,ysize);
    time0 = airTime();
    cart_makecart(ctx,gridxy,(xsize+1)*(ysize+1),xsize,ysize,0.0,ctx->nn,ctx->rk2);
    time1 = airTime();
    if (repeats > 1) {
      printf("%s:              ... %g secs for %u/%u\n", me, time1-time0, repIdx, repeats);
    } else {
      printf("%s:              ... %g secs\n", me, time1-time0);
    }
  }

  if (airStrlen(wispath)) {
    if (!(fwise = fopen(wispath, "w"))) {
      fprintf(stderr, "%s: couldn't open %s for writing: %s\n",
              me, wispath, strerror(errno));
      airMopError(mop);
      return 1;
    }
    fftw_export_wisdom_to_file(fwise);
    fclose(fwise);
  }

  /* ZIMO: convert vectors in ngrid from index-space to world-space */

  if (nrrdSave(outName, ngrid, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: problem saving displacement field: %s", me, err);
    airMopError(mop);
    return 1;
  }

  /* Free up the allocated space */
  cart_freews(ctx);

  /* cleanup */
  airMopOkay(mop);
  return 0;
}

/* Header file for cartogram.c
 *
 * Written by Mark Newman
 *
 * See http://www.umich.edu/~mejn/ for further details.
 */

#ifndef _CART_H
#define _CART_H

/* Inclusions */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>
// #include <teem/meet.h>

/* Constants */

#define EXPECTEDTIME 1.0e8   // Guess as to the time it will take, used to
                             // estimate completion

#define PI 3.1415926535897932384626

typedef struct {
  int savesnaps;        // save snapshots
  int verbosity;
  int rigor;            /* of fftw plan construction, with values from the
                           nrrdFFTWPlanRigor* enum */
  double initH,         /* replaces "#define INITH 0.001"
                           Initial size of a time-step */
    maxRatio,           /* replaces "#define MAXRATIO 4.0"
                           Max ratio to increase step size by */
    targetError;        /* replaces "#define TARGETERROR 0.01"
                           Desired accuracy per step in pixels */
  double *rhot[5];      // Pop density at time t (five snaps needed)
  double *fftrho;       // FT of initial density
  double *fftexpt;      // FT of density at time t

  /* GLK removed previous the "2D array" structure of vxt and vyt, which
     halved the number of memory loads in their use, and also interleaved
     vxt and vyt arrays, considering locality */
  double *vxyt;        // (x,y) velocity at time t

  double *preexp;         // Array needed for the Gaussian convolution

  fftw_plan rhotplan[5]; // Plan for rho(t) back-transform at time t

  int nn;
} cartContext;

/* HEY cartContextNew() should take xsize, ysize, and
   do the work of cart_makews */
extern cartContext *cartContextNew();
/* HEY this should do the work of cart_freews */
extern cartContext *cartContextNix(cartContext *ctx);

/* HEY the xsize,ysize arguments to these functions are dumb;
   that should be inside the cartContext */
extern void cart_makews(cartContext *ctx, int xsize, int ysize);
extern void cart_freews(cartContext *ctx);

extern void cart_forward(cartContext *ctx, double *rho, int xsize, int ysize);

extern void cart_makecart(cartContext *ctx,
                          /* GLK interleaved pointx and pointy arrays */
                          double *pointxy, int npoints,
                          int xsize, int ysize, double blur, int nn);

#endif

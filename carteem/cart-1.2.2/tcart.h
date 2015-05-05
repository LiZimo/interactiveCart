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

#define INITH 0.001          // Initial size of a time-step
#define TARGETERROR 0.01     // Desired accuracy per step in pixels
#define MAXRATIO 4.0         // Max ratio to increase step size by
#define EXPECTEDTIME 1.0e8   // Guess as to the time it will take, used to
                             // estimate completion

#define PI 3.1415926535897932384626

typedef struct {
  double *rhot[5];       // Pop density at time t (five snaps needed)
  double *fftrho;        // FT of initial density
  double *fftexpt;       // FT of density at time t

  double *vxyt[5];        // (x,y) velocity at time t

  double *expky;         // Array needed for the Gaussian convolution

  fftw_plan rhotplan[5]; // Plan for rho(t) back-transform at time t
} cartContext;

extern cartContext *cartContextNew();
extern cartContext *cartContextNix(cartContext *ctx);

extern void cart_makews(cartContext *ctx, int xsize, int ysize);
extern void cart_freews(cartContext *ctx, int xsize, int ysize);

extern void cart_transform(cartContext *ctx, double **userrho, int xsize, int ysize);
extern void cart_forward(cartContext *ctx, double *rho, int xsize, int ysize);

extern double** cart_dmalloc(int xsize, int ysize);
extern void cart_dfree(double **userrho);
extern void cart_makecart(cartContext *ctx,
                          double *pointxy, int npoints,
                          int xsize, int ysize, double blur);

#endif

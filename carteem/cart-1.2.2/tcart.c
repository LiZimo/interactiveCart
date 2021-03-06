/* Routines to transform a given set of points to a Gastner-Newman cartogram
 *
 * Written by Mark Newman
 *
 * See http://www.umich.edu/~mejn/ for further details.
 */

/* Globals */

#include "tcart.h"
#include "teem/meet.h"

static int frigor(int rigor) {
  /* static const char me[]="frigor"; */
  int ret;
  switch (rigor) {
  case nrrdFFTWPlanRigorExhaustive:
    ret = FFTW_EXHAUSTIVE;
    /* fprintf(stderr, "!%s: exhaustive %d\n", me, ret); */
    break;
  case nrrdFFTWPlanRigorMeasure:
    ret = FFTW_MEASURE;
    /* fprintf(stderr, "!%s: measure %d\n", me, ret); */
    break;
  case nrrdFFTWPlanRigorPatient:
    ret = FFTW_PATIENT;
    /* fprintf(stderr, "!%s: patient %d\n", me, ret); */
    break;
  case nrrdFFTWPlanRigorEstimate:
  default:
    ret = FFTW_ESTIMATE;
    /* fprintf(stderr, "!%s: estimate %d\n", me, ret); */
    break;
  }
  return ret;
}

cartContext *
cartContextNew() {
  cartContext *ctx;
  unsigned int si;
  ctx = (cartContext *)(calloc(1, sizeof(cartContext)));
  if (ctx) {
    ctx->initH = 0.001;
    ctx->targetError = 0.01;
    ctx->savesnaps[0] = AIR_FALSE;
    ctx->savesnaps[1] = AIR_FALSE;
    ctx->savesnaps[2] = AIR_FALSE;
    for (si=0; si<4; si++) {
      ctx->rhot[si] = NULL;
      //ctx->vxyt[si] = NULL;
      ctx->vxyt = NULL;
      ctx->rhotplan[si] = NULL;
    }
    ctx->fftrho = NULL;
    ctx->fftexpt = NULL;
    ctx->preexp = NULL;
    ctx->ngrid = NULL;
  }
  return ctx;
}

cartContext *
cartContextNix(cartContext *ctx) {

  if (ctx) {
    free(ctx);
  }
  return NULL;
}

/* Function to allocate space for the global arrays */

void cart_makews(cartContext *ctx, int xsize, int ysize)
{
  int s,i;

  /* Space for the FFT arrays is allocated single blocks, rather than using
   * a true two-dimensional array, because libfftw demands that it be so */

  for (s=0; s<5; s++) ctx->rhot[s] = fftw_malloc(xsize*ysize*sizeof(double));
  ctx->fftrho = fftw_malloc(xsize*ysize*sizeof(double));
  ctx->fftexpt = fftw_malloc(xsize*ysize*sizeof(double));

  //for (s=0; s<5; s++) {
  //  ctx->vxyt[s] = (double*)malloc(2*(xsize+1)*(ysize+1)*sizeof(double));
  //}

  ctx->vxyt = (double*)malloc(5*2*(xsize+1)*(ysize+1)*sizeof(double));

  /* VIDX() macro for indexing into ctx->vxyt
     C: vector component: 0 or 1
     X: x coord: 0 to xsize
     Y: y coord: 0 to ysize
     S: timestep: 0 through 4
     NOTE: these macros assume xsp=1+xsize and/or ysp=1+ysize.
     Different VIDX below are annotated with axes ordered fast-to-slow
  */
#define VIDX(C,X,Y,S) ((S) + 5*((C) + 2*((X) + xsp*(Y)))) // S C X Y
  //#define VIDX(C,X,Y,S) ((C) + 2*((S) + 5*((X) + xsp*(Y)))) // C S X Y
  //#define VIDX(C,X,Y,S) ((C) + 2*((X) + xsp*((Y) + ysp*(S)))) // C X Y S
  //#define VIDX(C,X,Y,S) ((C) + 2*((X) + xsp*((S) + 5*(Y)))) // C X S Y
  //#define VIDX(C,X,Y,S) ((S) + 5*((C) + 2*((Y) + ysp*(X)))) // S C Y X  (definitely slower)

  ctx->preexp = malloc(xsize*sizeof(double));

  /* Make plans for the back transforms */
  for (i=0; i<5; i++) {
    ctx->rhotplan[i] = fftw_plan_r2r_2d(ysize,xsize,ctx->fftexpt,ctx->rhot[i],
                                        FFTW_REDFT01, FFTW_REDFT01,
                                        frigor(ctx->rigor));
  }
}


/* Function to free up space for the global arrays and destroy the FFT
 * plans */

void cart_freews(cartContext *ctx)
{
  int s,i;

  for (s=0; s<5; s++) fftw_free(ctx->rhot[s]);
  fftw_free(ctx->fftrho);
  fftw_free(ctx->fftexpt);

  //for (s=0; s<5; s++) {
  //  free(ctx->vxyt[s]);
  //}
  free(ctx->vxyt);

  free(ctx->preexp);

  for (i=0; i<5; i++) fftw_destroy_plan(ctx->rhotplan[i]);
}


/* Function to calculate the discrete cosine transform of the input data.
 * assumes its input is an fftw_malloced array in column-major form with
 * size xsize*ysize
 * NOTE: This is done only once, so efficiency is not a concern */
void cart_forward(cartContext *ctx, double *rho, int xsize, int ysize)
{
  fftw_plan plan;

  plan = fftw_plan_r2r_2d(ysize,xsize,rho,ctx->fftrho,
			  FFTW_REDFT10,FFTW_REDFT10,frigor(ctx->rigor));
  fftw_execute(plan);
  fftw_destroy_plan(plan);
}


/* Function to calculate the population density at arbitrary time by back-
 * transforming and put the result in a particular rhot[] snapshot array.
 * Calculates unnormalized densities, since FFTW gives unnormalized back-
 * transforms, but this doesn't matter because the cartogram method is
 * insensitive to variation in the density by a multiplicative constant */

void cart_density(cartContext *ctx, double t, int s, int xsize, int ysize)
{
  static const char me[]="cart_density";
  int ix,iy;
  double kx,ky;
  double expky;
  static unsigned int snapi = 0;

  /* Calculate the preexp array, to save time in the next part */

  for (ix=0; ix<xsize; ix++) {
    kx = PI*ix/xsize;
    ctx->preexp[ix] = exp(-kx*kx*t);
  }

  /* Multiply the FT of the density by the appropriate factors */

  for (iy=0; iy<ysize; iy++) {
    ky = PI*iy/ysize;
    expky = exp(-ky*ky*t);
    for (ix=0; ix<xsize; ix++) {
      ctx->fftexpt[ix + xsize*iy] = expky*ctx->preexp[ix]*ctx->fftrho[ix + xsize*iy];
    }
  }

  /* Perform the back-transform */
  fftw_execute(ctx->rhotplan[s]);
  if (ctx->savesnaps[0]) {
    char fname[128], key[128], val[128];
    sprintf(fname, "snaprho-%04u.nrrd", snapi);
    Nrrd *nsnap = nrrdNew();
    sprintf(key, "difftime");
    sprintf(val, "%g", t);
    if (nrrdWrap_va(nsnap, ctx->rhot[s], nrrdTypeDouble, 2,
                    (size_t)xsize, (size_t)ysize)
        || nrrdKeyValueAdd(nsnap, key, val)
        || nrrdSave(fname, nsnap, NULL)) {
      char *err = biffGetDone(NRRD);
      fprintf(stderr, "%s: couldn't wrap and save: %s\n", me, err);
      free(err);
    }
    if (ctx->verbosity > 1) {
      fprintf(stderr, "%s: saved %s\n", me, fname);
    }
    nrrdNix(nsnap);
    snapi++;
  }
}


/* Function to calculate the velocity at all integer grid points for a
 * specified snapshot */

void cart_vgrid(cartContext *ctx, int s, int xsize, int ysize)
{
  static const char me[]="cart_vgrid";
  int ix,iy;
  double r00,r10;
  double r01,r11;
  double mid;
  int xsp=xsize+1;
  int ysp=ysize+1;
  static unsigned int snapi = 0;

  /* Do the corners */
 //ctx->vxyt[s][0 + 2*(0     + xsp*0)]     = ctx->vxyt[s][1 + 2*(0     + xsp*0)] = 0.0;
 //ctx->vxyt[s][0 + 2*(xsize + xsp*0)]     = ctx->vxyt[s][1 + 2*(xsize + xsp*0)] = 0.0;
 //ctx->vxyt[s][0 + 2*(0     + xsp*ysize)] = ctx->vxyt[s][1 + 2*(0     + xsp*ysize)] = 0.0;
 //ctx->vxyt[s][0 + 2*(xsize + xsp*ysize)] = ctx->vxyt[s][1 + 2*(xsize + xsp*ysize)] = 0.0;

  ctx->vxyt[VIDX(0,0,0,s)]     = ctx->vxyt[VIDX(1,0,0,s)]     = 0.0;
  ctx->vxyt[VIDX(0,xsize,0,s)] = ctx->vxyt[VIDX(1,xsize,0,s)] = 0.0;
  ctx->vxyt[VIDX(0,0,ysize,s)] = ctx->vxyt[VIDX(1,0,ysize,s)] = 0.0;
  ctx->vxyt[VIDX(0,xsize,ysize,s)] = ctx->vxyt[VIDX(1,xsize,ysize,s)] = 0.0;

  /* Do the top border */

  r11 = ctx->rhot[s][0 + xsize*0];

  for (ix=1; ix<xsize; ix++) {
    r01 = r11;
    r11 = ctx->rhot[s][ix + xsize*0];
    ctx->vxyt[VIDX(0,ix,0,s)] = -2*(r11-r01)/(r11+r01);
    ctx->vxyt[VIDX(1,ix,0,s)] = 0.0;
  }

  /* Do the bottom border */

  r10 = ctx->rhot[s][0 + xsize*(ysize-1)];
  for (ix=1; ix<xsize; ix++) {
    r00 = r10;
    r10 = ctx->rhot[s][ix + xsize*(ysize-1)];
    ctx->vxyt[VIDX(0,ix,ysize,s)] = -2*(r10-r00)/(r10+r00);
    ctx->vxyt[VIDX(1,ix,ysize,s)] = 0.0;
  }

  /* Left edge */

  r11 = ctx->rhot[s][0 + xsize*0];
  for (iy=1; iy<ysize; iy++) {
    r10 = r11;
    r11 = ctx->rhot[s][0 + xsize*iy];
    ctx->vxyt[VIDX(0,0,iy,s)] = 0.0;
    ctx->vxyt[VIDX(1,0,iy,s)] = -2*(r11-r10)/(r11+r10);
  }

  /* Right edge */

  r01 = ctx->rhot[s][xsize-1];
  for (iy=1; iy<ysize; iy++) {
    r00 = r01;
    r01 = ctx->rhot[s][xsize-1 + xsize*iy];
    ctx->vxyt[VIDX(0,xsize,iy,s)] = 0.0;
    ctx->vxyt[VIDX(0,xsize,iy,s)] = -2*(r01-r00)/(r01+r00);
  }

  /* Now do all the points in the middle */
  for (iy=1; iy<ysize; iy++) {
    r10 = ctx->rhot[s][0 + xsize*(iy-1)];
    r11 = ctx->rhot[s][0 + xsize*iy];
    for (ix=1; ix<xsize; ix++) {
      r00 = r10;
      r01 = r11;
      r10 = ctx->rhot[s][ix + xsize*(iy-1)];
      r11 = ctx->rhot[s][ix + xsize*iy];
      mid = r10 + r00 + r11 + r01;
      ctx->vxyt[VIDX(0,ix,iy,s)] = -2*(r10-r00+r11-r01)/mid;
      ctx->vxyt[VIDX(1,ix,iy,s)] = -2*(r01-r00+r11-r10)/mid;
    }
  }

  if (ctx->savesnaps[1]) {
    char fname[128];
    sprintf(fname, "snapvel-%04u.nrrd", snapi);
    Nrrd *nsnap = nrrdNew();
    if (nrrdWrap_va(nsnap, ctx->vxyt, nrrdTypeDouble, 4,
                    /* HEY: the exact ordering here depends on VIDX */
                    (size_t)5, (size_t)2, (size_t)(xsize+1), (size_t)(ysize+1))
        || nrrdSave(fname, nsnap, NULL)) {
      char *err = biffGetDone(NRRD);
      fprintf(stderr, "%s: couldn't wrap and save: %s\n", me, err);
      free(err);
    }
    if (ctx->verbosity > 1) {
      fprintf(stderr, "%s: saved %s\n", me, fname);
    }
    nrrdNix(nsnap);
    snapi++;
  }
}



/* Function to calculate the velocity at an arbitrary point from the grid
 * velocities for a specified snapshot by interpolating between grid
 * points.  If the requested point is outside the boundaries, we
 * extrapolate (ensures smooth flow back in if we get outside by mistake,
 * although we should never actually do this because function cart_twosteps()
 * contains code to prevent it) */

void cart_velocity(const cartContext *ctx,
                   double rx, double ry, int s, int xsize, int ysize,
		   double *vxp, double *vyp, int nn)
{
  //printf("changed! \n");
  int ix,iy;
  double dx,dy;
  double dx1m,dy1m;
  double w11,w21,w12,w22;
  int xsp=xsize+1;
  int ysp=ysize+1;

  /* Deal with the boundary conditions */
  if (nn == 1) {
  ix = (int) (rx + 0.5) ;}
  if (nn == 0) {ix = (int) rx;}
  if (ix<0) ix = 0;
  else if (ix>=xsize) ix = xsize - 1;

  if (nn == 1) {
  iy = (int) (ry + 0.5);}
  if (nn == 0) {iy = (int) ry;}
  if (iy<0) iy = 0;
  else if (iy>=ysize) iy = ysize - 1;

  /* Calculate the weights for the bilinear interpolation */

  dx = rx - ix;
  dy = ry - iy;

  dx1m = 1.0 - dx;
  dy1m = 1.0 - dy;

  w11 = dx1m*dy1m;
  w21 = dx*dy1m;
  w12 = dx1m*dy;
  w22 = dx*dy;

  /* Perform the interpolation for x and y components of velocity */
  //const double *vxy = ctx->vxyt[s] + 0 + 2*(ix   + xsp*iy);
  //*vxp = w11*vxy[0] + w21*vxy[0 + 2] + w12*vxy[0 + 2*xsp] + w22*vxy[0 + 2*(1 + xsp)];
  //*vyp = w11*vxy[1] + w21*vxy[1 + 2] + w12*vxy[1 + 2*xsp] + w22*vxy[1 + 2*(1 + xsp)];
  /*
  const double *vxy = ctx->vxyt + (0 + 2*(ix   + xsp*iy))*5 + s;
  *vxp = w11*vxy[0*5] + w21*vxy[(0 + 2)*5] + w12*vxy[(0 + 2*xsp)*5] + w22*vxy[(0 + 2*(1 + xsp))*5];
  *vyp = w11*vxy[1*5] + w21*vxy[(1 + 2)*5] + w12*vxy[(1 + 2*xsp)*5] + w22*vxy[(1 + 2*(1 + xsp))*5];
  */

  const double *vxyt = ctx->vxyt;

  if (nn == 0) {
  *vxp = w11*vxyt[VIDX(0,ix,iy,s)] + w21*vxyt[VIDX(0,ix+1,iy,s)] + w12*vxyt[VIDX(0,ix,iy+1,s)] + w22*vxyt[VIDX(0,ix+1,iy+1,s)];
  *vyp = w11*vxyt[VIDX(1,ix,iy,s)] + w21*vxyt[VIDX(1,ix+1,iy,s)] + w12*vxyt[VIDX(1,ix,iy+1,s)] + w22*vxyt[VIDX(1,ix+1,iy+1,s)];
  }
  
  if (nn == 1) {
    //printf("using nearest neighbor \n");
  *vxp = vxyt[VIDX(0, ix, iy, s)];
  *vyp = vxyt[VIDX(1, ix, iy, s)];
  }
}


/* Function to integrate 2h time into the future two different ways using
 * four-order Runge-Kutta and compare the differences for the purposes of
 * the adaptive step size.  Parameters are:
 *   *pointxy = array of (x,y)-coords of points
 *   npoints = number of points
 *   t = current time, i.e., start time of these two steps
 *   h = delta t
 *   s = snapshot index of the initial time
 *   xsize, ysize = size of grid
 *   *errorp = the maximum integration error found for any polygon vertex for
 *             the complete two-step process
 *   *drp = maximum distance moved by any point
 *   *spp = the snapshot index for the final function evaluation
 */

void cart_twosteps(cartContext *ctx,
                   double *pointxy, int npoints,
		   double t, double h, int s, int xsize, int ysize,
		   double *errorp, double *drp, int *spp, int NN, int rk2)
{

 if (rk2 == 1) {//printf("got here in cart_twosteps \n"); 
 cart_twosteps_rk2(ctx,
                    pointxy, npoints,
       t, h, s, xsize, ysize,
        errorp, drp, spp, NN);}

 else {
 int s0,s1,s2,s3,s4;
 int p;
 double rx1,ry1;
 double rx2,ry2;
 double rx3,ry3;
 double v1x,v1y;
 double v2x,v2y;
 double v3x,v3y;
 double v4x,v4y;
 double k1x,k1y;
 double k2x,k2y;
 double k3x,k3y;
 double k4x,k4y;
 double dx1,dy1;
 double dx2,dy2;
 double dx12,dy12;
 double dxtotal,dytotal;
 double ex,ey;
 double esq,esqmax;
 double drsq,drsqmax;

 s0 = s;
 s1 = (s+1)%5;
 s2 = (s+2)%5;
 s3 = (s+3)%5;
 s4 = (s+4)%5;

 /* Calculate the density field for the four new time slices */
 /* GLK interleaved density and vgrid calls, considering locality;
    but it wasn't the bottleneck */
 cart_density(ctx,t+0.5*h,s1,xsize,ysize);
 cart_vgrid(ctx,s1,xsize,ysize);
 cart_density(ctx,t+1.0*h,s2,xsize,ysize);
 cart_vgrid(ctx,s2,xsize,ysize);
 cart_density(ctx,t+1.5*h,s3,xsize,ysize);
 cart_vgrid(ctx,s3,xsize,ysize);
 cart_density(ctx,t+2.0*h,s4,xsize,ysize);
 cart_vgrid(ctx,s4,xsize,ysize);

 /* Do all three RK steps for each point in turn */

 esqmax = drsqmax = 0.0;

 for (p=0; p<npoints; p++) {

   rx1 = pointxy[0 + 2*p];
   ry1 = pointxy[1 + 2*p];

   /* Do the big combined (2h) RK step */

   cart_velocity(ctx,rx1,ry1,s0,xsize,ysize,&v1x,&v1y, NN);
   k1x = 2*h*v1x;
   k1y = 2*h*v1y;
   cart_velocity(ctx,rx1+0.5*k1x,ry1+0.5*k1y,s2,xsize,ysize,&v2x,&v2y, NN);
   k2x = 2*h*v2x;
   k2y = 2*h*v2y;
   cart_velocity(ctx,rx1+0.5*k2x,ry1+0.5*k2y,s2,xsize,ysize,&v3x,&v3y,NN);
   k3x = 2*h*v3x;
   k3y = 2*h*v3y;
   cart_velocity(ctx,rx1+k3x,ry1+k3y,s4,xsize,ysize,&v4x,&v4y,NN);
   k4x = 2*h*v4x;
   k4y = 2*h*v4y;

   dx12 = (k1x+k4x+2.0*(k2x+k3x))/6.0;
   dy12 = (k1y+k4y+2.0*(k2y+k3y))/6.0;

   /* Do the first small RK step.  No initial call to cart_velocity() is done
    * because it would be the same as the one above, so there's no need
    * to do it again */

   k1x = h*v1x;
   k1y = h*v1y;
   cart_velocity(ctx,rx1+0.5*k1x,ry1+0.5*k1y,s1,xsize,ysize,&v2x,&v2y, NN);
   k2x = h*v2x;
   k2y = h*v2y;
   cart_velocity(ctx,rx1+0.5*k2x,ry1+0.5*k2y,s1,xsize,ysize,&v3x,&v3y,NN);
   k3x = h*v3x;
   k3y = h*v3y;
   cart_velocity(ctx,rx1+k3x,ry1+k3y,s2,xsize,ysize,&v4x,&v4y,NN);
   k4x = h*v4x;
   k4y = h*v4y;

   dx1 = (k1x+k4x+2.0*(k2x+k3x))/6.0;
   dy1 = (k1y+k4y+2.0*(k2y+k3y))/6.0;

   /* Do the second small RK step */

   rx2 = rx1 + dx1;
   ry2 = ry1 + dy1;

   cart_velocity(ctx,rx2,ry2,s2,xsize,ysize,&v1x,&v1y,NN);
   k1x = h*v1x;
   k1y = h*v1y;
   cart_velocity(ctx,rx2+0.5*k1x,ry2+0.5*k1y,s3,xsize,ysize,&v2x,&v2y,NN);
   k2x = h*v2x;
   k2y = h*v2y;
   cart_velocity(ctx,rx2+0.5*k2x,ry2+0.5*k2y,s3,xsize,ysize,&v3x,&v3y,NN);
   k3x = h*v3x;
   k3y = h*v3y;
   cart_velocity(ctx,rx2+k3x,ry2+k3y,s4,xsize,ysize,&v4x,&v4y,NN);
   k4x = h*v4x;
   k4y = h*v4y;

   dx2 = (k1x+k4x+2.0*(k2x+k3x))/6.0;
   dy2 = (k1y+k4y+2.0*(k2y+k3y))/6.0;

   /* Calculate the (squared) error */

   ex = (dx1+dx2-dx12)/15;
   ey = (dy1+dy2-dy12)/15;
   esq = ex*ex + ey*ey;
   if (esq>esqmax) esqmax = esq;

   /* Update the position of the vertex using the more accurate (two small
    * steps) result, and deal with the boundary conditions.  This code
    * does 5th-order "local extrapolation" (which just means taking
    * the estimate of the 5th-order term above and adding it to our
    * 4th-order result get a result accurate to the next highest order) */

   dxtotal = dx1 + dx2 + ex;   // Last term is local extrapolation
   dytotal = dy1 + dy2 + ey;   // Last term is local extrapolation
   drsq = dxtotal*dxtotal + dytotal*dytotal;
   if (drsq>drsqmax) drsqmax = drsq;

   rx3 = rx1 + dxtotal;
   ry3 = ry1 + dytotal;

   if (rx3<0) rx3 = 0;
   else if (rx3>xsize) rx3 = xsize;
   if (ry3<0) ry3 = 0;
   else if (ry3>ysize) ry3 = ysize;

   pointxy[0 + 2*p] = rx3;
   pointxy[1 + 2*p] = ry3;

 }

 *errorp = sqrt(esqmax);
 *drp =  sqrt(drsqmax);
 *spp = s4;
}
}

void cart_twosteps_rk2(cartContext *ctx,
                   double *pointxy, int npoints,
       double t, double h, int s, int xsize, int ysize,
       double *errorp, double *drp, int *spp, int NN)
{

  //printf("got here in rk2 \n");
  int s0,s1,s2;
  int p;
  double rx1,ry1;
  double rx2,ry2;
  double rx3,ry3;
  double v1x,v1y;
  double v2x,v2y;
  double k1x,k1y;
  double k2x,k2y;
  double dx1,dy1;
  double dx2,dy2;
  double dx12,dy12;
  double dxtotal,dytotal;
  double ex,ey;
  double esq,esqmax;
  double drsq,drsqmax;

  s0 = s;
  s1 = (s+1)%3;
  s2 = (s+2)%3;

  /* Calculate the density field for the two new time slices */

  cart_density(ctx,t+h,s1,xsize,ysize);
  cart_density(ctx,t+2*h,s2,xsize,ysize);

  /* Calculate the resulting velocity grids */

  cart_vgrid(ctx,s1,xsize,ysize);
  cart_vgrid(ctx,s2,xsize,ysize);

  /* Do all three RK steps for each point in turn */

  esqmax = drsqmax = 0.0;

  for (p=0; p<npoints; p++) {

    rx1 = pointxy[0 + 2*p];
    ry1 = pointxy[1 + 2*p];

    /* Do the big combined (2h) RK step */

    cart_velocity(ctx,rx1,ry1,s0,xsize,ysize,&v1x,&v1y,NN);
    k1x = 2*h*v1x;
    k1y = 2*h*v1y;
    cart_velocity(ctx,rx1+k1x,ry1+k1y,s2,xsize,ysize,&v2x,&v2y,NN);
    k2x = 2*h*v2x;
    k2y = 2*h*v2y;

    dx12 = 0.5*(k1x+k2x);
    dy12 = 0.5*(k1y+k2y);

    /* Do the first small RK step.  No initial call to cart_velocity() is done
     * because it would be the same as the one above, so there's no need
     * to do it again */

    k1x = h*v1x;
    k1y = h*v1y;
    cart_velocity(ctx,rx1+k1x,ry1+k1y,s1,xsize,ysize,&v2x,&v2y,NN);
    k2x = h*v2x;
    k2y = h*v2y;

    dx1 = 0.5*(k1x+k2x);
    dy1 = 0.5*(k1y+k2y);

    /* Do the second small RK step */

    rx2 = rx1 + dx1;
    ry2 = ry1 + dy1;

    cart_velocity(ctx,rx2,ry2,s1,xsize,ysize,&v1x,&v1y,NN);
    k1x = h*v1x;
    k1y = h*v1y;
    cart_velocity(ctx,rx2+k1x,ry2+k1y,s2,xsize,ysize,&v2x,&v2y,NN);
    k2x = h*v2x;
    k2y = h*v2y;

    dx2 = 0.5*(k1x+k2x);
    dy2 = 0.5*(k1y+k2y);

    /* Calculate the (squared) error */

    ex = (dx1+dx2-dx12)/3;
    ey = (dy1+dy2-dy12)/3;
    esq = ex*ex + ey*ey;
    if (esq>esqmax) esqmax = esq;

    /* Update the position of the vertex using the more accurate (two small
     * steps) result, and deal with the boundary conditions.  This code
     * does 3rd-order "local extrapolation" (which just means taking
     * the estimate of the 3rd-order term above and adding it to our
     * 2nd-order result get a result accurate to the next highest order) */

    dxtotal = dx1 + dx2 + ex;   // Last term is local extrapolation
    dytotal = dy1 + dy2 + ey;   // Last term is local extrapolation
    drsq = dxtotal*dxtotal + dytotal*dytotal;
    if (drsq>drsqmax) drsqmax = drsq;

    rx3 = rx1 + dxtotal;
    ry3 = ry1 + dytotal;

    if (rx3<0) rx3 = 0;
    else if (rx3>xsize) rx3 = xsize;
    if (ry3<0) ry3 = 0;
    else if (ry3>ysize) ry3 = ysize;

    pointxy[0 + 2*p] = rx3;
    pointxy[1 + 2*p] = ry3;

  }

  *errorp = sqrt(esqmax);
  *drp =  sqrt(drsqmax);
  *spp = s2;
}

/* Function to estimate the percentage completion */

int cart_complete(cartContext *ctx, double t)
{
  int res;

  res = 100*log(t/(ctx->initH))/log(EXPECTEDTIME/(ctx->initH));
  if (res>100) res = 100;

  return res;
}


/* Function to do the transformation of the given set of points
 * to the cartogram */

void cart_makecart(cartContext *ctx, double *pointxy, int npoints,
		   int xsize, int ysize, double blur, int NN, int rk2)
{
  static const char me[]="cart_makecart";
  int i;
  int s,sp;
  int step;
  int done, loopi;
  double t,h;
  double error,dr;
  double desiredratio;
  static unsigned int snapi=0;

  /* Calculate the initial density and velocity for snapshot zero */

  cart_density(ctx,0.0,0,xsize,ysize);
  cart_vgrid(ctx,0,xsize,ysize);
  s = 0;

  /* Now integrate the points in the polygons */

  step = 0;
  t = 0.5*blur*blur;
  h = ctx->initH;
  loopi = 0;
  do {

    if (ctx->savesnaps[2]) {
      char fname[128];
      sprintf(fname, "snapdsp-%04u.nrrd", snapi);
      if (nrrdSave(fname, ctx->ngrid, NULL)) {
        char *err = biffGetDone(NRRD);
        fprintf(stderr, "%s: couldn't save displacement: %s\n", me, err);
        free(err);
      }
      if (ctx->verbosity > 1) {
        fprintf(stderr, "%s: saved %s\n", me, fname);
      }
      snapi++;
    }

    /* Do a combined (triple) integration step */

    cart_twosteps(ctx,pointxy,npoints,t,h,s,xsize,ysize,&error,&dr,&sp, NN, rk2);

    /* Increase the time by 2h and rotate snapshots */

    t += 2.0*h;
    step += 2;
    s = sp;

    /* Adjust the time-step.  Factor of 2 arises because the target for
     * the two-step process is twice the target for an individual step */

    desiredratio = pow(2*(ctx->targetError)/error,0.2);
    if (ctx->verbosity) {
      printf("%s(%d): h=%g, error=%g (vs %g) -> desiredratio=%g ",
             me, loopi, h, error, ctx->targetError, desiredratio);
    }
    if (desiredratio>(ctx->maxRatio)) {
      h *= ctx->maxRatio;
      if (ctx->verbosity) {
        printf("> %g -> h=%g\n", ctx->maxRatio, h);
      }
    } else {
      h *= desiredratio;
      if (ctx->verbosity) {
        printf("<= %g -> h=%g\n", ctx->maxRatio, h);
      }
    }

    done = cart_complete(ctx, t);
#ifdef PERCENT
    fprintf(stdout,"%i\n",done);
#endif
#ifndef NOPROGRESS
    fprintf(stderr,"  %3i%%  |",done);
    for (i=0; i<done/2; i++) fprintf(stderr,"=");
    for (i=done/2; i<50; i++) fprintf(stderr," ");
    fprintf(stderr,"|\r");
#endif
   loopi++;
    /* If no point moved then we are finished */

  } while (dr>0.0);

#ifdef PERCENT
  fprintf(stdout,"\n");
#endif
#ifndef NOPROGRESS
  fprintf(stderr,"  100%%  |==================================================|\n");
#endif
}

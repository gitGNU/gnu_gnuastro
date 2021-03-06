/*********************************************************************
Crop - Crop a given size from one or multiple images.
Crop is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>

#include <gnuastro/wcs.h>

#include "main.h"

#include "onecrop.h"








/*******************************************************************/
/****************        Check for ui.c        *********************/
/*******************************************************************/
/* This function is called from ui.c. Its job is to check the WCS
   values of this  */
void
wcs_check_prepare(struct cropparams *p, struct inputimgs *img)
{
  double twidth, *pixscale;
  struct wcsprm *wcs=img->wcs;
  int i, status[4]={0,0,0,0}, ncoord=4, nelem=2;
  double imgcrd[8], phi[4], theta[4], pixcrd[8];


  /* Check if the image is aligned with the WCS coordinates. Note that
     because of small floating point errors, some programs might still keep
     very small values in the off-diagonal matrix elements. */
  if( fabs(wcs->pc[1]/wcs->pc[3])>1e-6 || fabs(wcs->pc[2]/wcs->pc[3])>1e-6 )
    error(EXIT_FAILURE, 0, "%s: HDU %s: is not aligned to the "
          "celestial coordinates. The first FITS axis should be "
          "along the Right Ascension and the second FITS axis "
          "should be along the declination.\n\n"
          "Gnuastro's Warp program can align it with the following "
          "command:\n\n"
          "    $ astwarp %s --hdu=%s --align\n",
          img->name, p->cp.hdu, img->name, p->cp.hdu);
  if(wcs->pc[0]>0)
    error(EXIT_FAILURE, 0, "%s: HDU %s: An increase in the first "
          "FITS axis pixel coordinates should be a decrease in the "
          "RA. You have to flip the image along the second axis "
          "before running Crop", img->name, p->cp.hdu);
  if(wcs->pc[3]<0)
    error(EXIT_FAILURE, 0, "%s: HDU %s: An increase in the second "
          "FITS axis pixel coordinates should translate to an "
          "increase in the declination. You have to flip the "
          "image along the first axis before running Crop",
          img->name, p->cp.hdu);


  /* Check the image resolution (if the pixels are actually a square), then
     compare the resolution with the other input images. Due to floating
     point errors, some very small differences might exist in the pixel
     scale, so break out with an error only if the pixel scales are more
     different than 1e-6. */
  pixscale=gal_wcs_pixel_scale_deg(wcs);
  if( fabs(pixscale[0]-pixscale[1])/pixscale[0] > 1e-6 )
    error(EXIT_FAILURE, 0, "%s: HDU %s: The pixel scale along "
          "the two image axises is not the same. The first axis "
          "is %.15g deg/pixel, while the second is %.15g",
          img->name, p->cp.hdu, pixscale[1], pixscale[0]);
  if(p->res==0.0f)
    {
      /* Set the resolution of the image. */
      p->res=pixscale[0];
      free(pixscale);

      /* Set the widths such that iwidth and wwidth are exactly the same
         (within their different units ofcourse). Also make sure that the
         image size is an odd number (so the central pixel is in the
         center). */
      p->wwidth/=3600;                 /* Convert the width to degrees. */
      twidth=p->wwidth/p->res;
      if(twidth<3)
        error(EXIT_FAILURE, 0, "--wwidth = %f (arcseconds) translates "
              "to %.0f pixels in scale of input image(s). This is probably "
              "not what you want", p->wwidth*3600, twidth);
      p->iwidth[0] = (twidth-(long)twidth)>0.5 ? twidth+1 : twidth;
      if(p->iwidth[0]%2==0)
        {
          p->iwidth[0]+=1;
          p->wwidth+=p->res;
        }
      p->iwidth[1]=p->iwidth[0];
    }
  else
    {
      if(p->res!=pixscale[0])
        error(EXIT_FAILURE, 0, "%s: HDU %s: The resolution of "
              "this image is %f arcseconds/pixel while the "
              "previously checked input image(s) had a resolution "
              "of %f", img->name, p->cp.hdu, 3600*wcs->pc[3],
              3600*p->res);
      free(pixscale);
    }


  /* Get the coordinates of the first pixel in the image. Note that `dsize'
     is in C axises, while pixcrd is in FITS axises. */
  pixcrd[0]=1;               pixcrd[1]=1;
  pixcrd[2]=img->dsize[1];   pixcrd[3]=1;
  pixcrd[4]=1;               pixcrd[5]=img->dsize[0];
  pixcrd[6]=img->dsize[1];   pixcrd[7]=img->dsize[0];
  wcsp2s(wcs, ncoord, nelem, pixcrd, imgcrd, phi, theta,
         img->corners, status);

  /* Check if there was no error in the conversion. */
  for(i=0;i<4;++i)
    if(status[i])
      error(EXIT_FAILURE, 0, "wcsp2s ERROR %d in row %d of pixcrd: %s",
            i, status[i], wcs_errmsg[status[i]]);


  /* Fill in the size of the image in celestial degrees from the first
     pixel in the image. Note that `dsize' is in C axises, while pixcrd is
     in FITS axises. */
  img->sized[0]=img->dsize[1]*p->res/cos(img->corners[1]*M_PI/180);
  img->sized[1]=img->dsize[0]*p->res;


  /* In case the image crosses the equator, we will calculate these
     values here so later on, we don't have to calculate them on every
     check. See the explanation above radecoverlap.*/
  if( img->corners[1]*(img->corners[1]+img->sized[1]) < 0 )
    {
      /* re in the explanations. */
      img->equatorcorr[0]=img->corners[0]
        -0.5*img->sized[0]*(1-cos(img->corners[1]*M_PI/180));

      /* sre in the explanations. */
      img->equatorcorr[1]=img->sized[0]*cos(img->corners[1]*M_PI/180);
    }


  /* Just to check:
  printf("\n\n%s:\n(%.10f, %.10f)\n(%.10f, %.10f)"
         "\n(%.10f, %.10f)\n(%.10f, %.10f)\n\n", img->name,
         img->corners[0], img->corners[1],
         img->corners[2], img->corners[3],
         img->corners[4], img->corners[5],
         img->corners[6], img->corners[7]);
  exit(0);
  */
}




















/*******************************************************************/
/************        Check if WCS is in image         **************/
/*******************************************************************/
/* Set the four sides around the point of interest in RA and Dec.

   NOTE: In this format we are working on here (where the image is
   aligned with the celestial coordinates), the declination is
   measured on a great circle, while the right ascension is not. So we
   have to consider the
*/
void
setcsides(struct onecropparams *crp)
{
  size_t i;
  double h, hr, r, d, dr;        /* The second r is for radians. */
  struct cropparams *p=crp->p;
  double minra=FLT_MAX, mindec=FLT_MAX;
  double maxra=-FLT_MAX, maxdec=-FLT_MAX;

  /* Set the four corners of the WCS region. */
  if(p->polygon)
    {
      /* Find their minimum and maximum values. */
      for(i=0;i<p->nvertices;++i)
        {
          if(p->wpolygon[i*2]>maxra) maxra=p->wpolygon[i*2];
          if(p->wpolygon[i*2]<minra) minra=p->wpolygon[i*2];
          if(p->wpolygon[i*2+1]>maxdec) maxdec=p->wpolygon[i*2+1];
          if(p->wpolygon[i*2+1]<mindec) mindec=p->wpolygon[i*2+1];
        }

      /* Set the corners: */
      crp->corners[0] = maxra;  crp->corners[1] = mindec; /* Bottom Left  */
      crp->corners[2] = minra;  crp->corners[3] = mindec; /* Bottom Right */
      crp->corners[4] = maxra;  crp->corners[5] = maxdec; /* Top Left     */
      crp->corners[6] = minra;  crp->corners[7] = maxdec; /* Top Right    */
    }
  else
    {
      if(p->catname)
        {
          r=crp->world[0]=p->c1[crp->out_ind];
          d=crp->world[1]=p->c2[crp->out_ind];
        }
      else
        {
          r=crp->world[0]=p->ra;
          d=crp->world[1]=p->dec;
        }

      h=p->wwidth/2;
      dr=d*M_PI/180;
      hr=h*M_PI/180;

      /* Set the four corners of this crop. */
      crp->corners[0] = r+h/cos(dr-hr);  crp->corners[1] = d-h; /* Bt Lf */
      crp->corners[2] = r-h/cos(dr-hr);  crp->corners[3] = d-h; /* Bt Rt */
      crp->corners[4] = r+h/cos(dr+hr);  crp->corners[5] = d+h; /* Tp Lf */
      crp->corners[6] = r-h/cos(dr+hr);  crp->corners[7] = d+h; /* Tp Rt */
    }

  /* Set the bottom width and height of the crop in degrees. Note that
     the width changes as the height changes, so here we want the
     height and the lowest declination. Note that on the bottom edge,
     corners[0] is the maximum RA and corners[2] is the minimum RA.
     For all the region, corners[5] is one of the maximum declinations
     and corners[3] is one of the the minimum declinations.*/
  crp->sized[0]=( (crp->corners[0]-crp->corners[2])
                  / cos(crp->corners[1]*M_PI/180) );
  crp->sized[1]=crp->corners[5]-crp->corners[3];

  /* In case the crop crosses the equator, then we need these two
     corrections. See the complete explanations below. */
  if(crp->corners[1]*(crp->corners[1]+crp->sized[1]) < 0 )
    {
      /* re in the explanations. */
      crp->equatorcorr[0]=crp->corners[0]
        -0.5*crp->sized[0]*(1-cos(crp->corners[1]*M_PI/180));

      /* sre in the explanations. */
      crp->equatorcorr[1]=crp->sized[0]*cos(crp->corners[1]*M_PI/180);
    }

  /* Just to check:
  printf("\n\nCorner 1: (%.10f, %.10f)\n"
         "Corner 2: (%.10f, %.10f)\nCorner 3: (%.10f, %.10f)\n"
         "Corner 4: (%.10f, %.10f)\n\n",
         crp->corners[0], crp->corners[1],
         crp->corners[2], crp->corners[3],
         crp->corners[4], crp->corners[5],
         crp->corners[6], crp->corners[7]);
  */
}





/* We have the polygon vertices in WCS coordinates and need to change them
   to one input image's pixel coordinates. */
void
fillcrpipolygon(struct onecropparams *crp)
{
  size_t i;
  double *x, *y, *ra, *dec;
  struct cropparams *p=crp->p;

  /* Allocate the necessary arrays. */
  x=gal_data_calloc_array(GAL_TYPE_FLOAT64, p->nvertices);
  y=gal_data_calloc_array(GAL_TYPE_FLOAT64, p->nvertices);
  ra=gal_data_calloc_array(GAL_TYPE_FLOAT64, p->nvertices);
  dec=gal_data_calloc_array(GAL_TYPE_FLOAT64, p->nvertices);
  crp->ipolygon=gal_data_malloc_array(GAL_TYPE_FLOAT64, 2*p->nvertices);

  /* Fill in the RA and Dec columns. */
  for(i=0;i<p->nvertices;++i)
    {
      ra[i]=p->wpolygon[i*2];
      dec[i]=p->wpolygon[i*2+1];
    }

  /* Convert them to image coordinates. */
  gal_wcs_world_to_img(p->imgs[crp->in_ind].wcs, ra, dec, &x, &y,
                       p->nvertices);

  /* Put them in the image polygon vertice array. */
  for(i=0;i<p->nvertices;++i)
    {
      crp->ipolygon[i*2]   = x[i];
      crp->ipolygon[i*2+1] = y[i];
    }

  /* Clean up. */
  free(x);
  free(y);
  free(ra);
  free(dec);
}





/* BASICS:
   =======

   An image is a rectangle, but the sky is on a globe. When the images
   are aligned to the celstial coordinates, (as we have required here
   in wcscheckprepare) the first FITS axis shows change in RA, while
   the second axis shows change in Dec. The declination always changes
   along a great circle, so there is no problem. But unless
   declination is constrained to zero, RA changes on small circles.

   See the rectangle below, assume it is an image. To check if a given
   point is within the same declination as this rectangle is very
   simple, since d3==d4 and d1==d2. Note that r1>r2 and r3>r4 (because
   right ascension increases to the east).

       (r3,d3)    ------------------ (r4,d4)
                  |                |
                  |                |
                  |                |
                  |                |
       (r1,d1)    ------------------ (r2,d2)

   But for RA, the same number of pixels on each declination,
   corresponds to different ranges in right ascention. As the
   declination gets higher in the northern hemisphere (where the
   declination rises towards the top of the image) r1-r2 becomes
   smaller than r3-r4. So, in terms of coverage in RA and Dec, this
   box should rather be shown like this trapezoid (exaggerated):

                 --------------------
                 |                  |
                  |                |         (Northern hemisphere)
                   |              |
                    |            |
                    --------------

   On the southern hemisphere it should be shown like this:

                   ----------------
                   |              |
                  |                |         (Southern hemisphere)
                 |                  |
                |                    |
                ----------------------

   The functional form of the change is the inverse cosine, so:

           (r3-r4)=(r1-r2)/cos(d3-d1)            (North)

           (r1-r2)=(r3-r4)/cos(d1-d3)            (South)

   QUESTION:
   ========
   Is a given point at the RA and Dec of (rp,dp) inside our
   rectangular image?



   IMAGE IS FULLY WITHIN ONE HEMISPHERE
   ------------------------------------

   Our reference point for the image is the first pixel in the image,
   which by convention is the (r1,d1) point in the rectangle above. We
   also have the angular size of the rectangular image as 'sr', 'sd'
   (for "size in RA" and "size in dec"). NOTE: This has to be We also
   assume r1+sr and d1+sd are the distances to the last pixels in our
   rectangular image.

   As explained above, to check the declination range, everything is
   very easy:
                                        dp>=d1     &&      dp<=d1+sd

   For RA, things become a little more complicated (recall that
   r1>r3). `n` is defined as half of the extra space between the top
   and bottom lines of the two trapezoids.

   (North) n=0.5*sr*(1/cos(dp-d1)-1) ==> rp<=r1+n   &&   rp>=r1-sr-n

   (South) n=0.5*sr*(1-cos(dp-d1))   ==> rp<=r1-n   &&   rp>=r1-sr+n



   IMAGE CROSSES THE EQUATOR
   -------------------------

   When d1*(d1+sd)<0, the image crosses the equator (d1 is negative
   and d1+sd is positive). In this case, we define `re` and `sre` as
   an equivalent of r1 and sr but on the equator:

       re=r1-0.5*sr*(1-cos(d1))   &&   sre=sr*cos(d1)

   then  for all  the  points  with negative  declination  we use  the
   (South) equations of above as before and for those points that have
   a positive declination,  we use the North formula  but replacing r1
   with re, d1 with 0 and sr with sre.
*/
/*
   p[2]: RA and Dec of a point (rp and dp above).
   i[2]: RA and Dec of first point in image. (r1 and d1 above).
   s[2]: Size of the image in degrees (sr and sd above).
   c[2]: Corrections if equator is passed, (se and sre above).
*/
int
radecinimg(double *p, double *i, double *s, double *c)
{
  double n;

  /* First check the declination. If it is not in range, you can
     safely return 0.*/
  if(p[1]>=i[1]  && p[1]<=i[1]+s[1])
    {
      if(p[1]<=0)            /* Point is in southern hemisphere, it     */
        {                    /* doesn't matter if image passes equator! */
          n=0.5f*s[0]*( 1 - cos((p[1]-i[1])*M_PI/180) );
          if(p[0]<=i[0]-n   &&   p[0]>=i[0]-s[0]+n)
            return 1;
        }
      else                      /* Point is in the northern hemisphere. */
        {
          if( i[1] * (s[1]+i[1]) > 0 ) /* Image does not cross equator. */
            {
              n=0.5f*s[0]*( 1/cos((p[1]-i[1])*M_PI/180) - 1);
              if(p[0]<=i[0]+n   &&   p[0]>=i[0]-s[0]-n)
                return 1;
            }
          else                             /* Image crosses the equator.*/
            {
              n=0.5f*c[1]*( 1/cos((p[1]     )*M_PI/180) - 1);
              if(p[0]<=c[0]+n   &&   p[0]>=c[0]-c[1]-n)
                return 1;
            }
        }
    }
  return 0;
}






/* Is there an overlap between this crop box and the survey image?
   This function will return 0 if there isn't and 1 if there is.

   We don't want to necessarily assume that the crop box is smaller
   than the survey images. If we made that assumption, we only had to
   check if the corners of the crop are in the image. When we allow
   the input survey images to be smaller than the crop box, it might
   happen that none of the corners of the crop are in the image but
   there is an overlap (the survey image is completely within the crop
   box). So we have to check both.  */
int
radecoverlap(struct onecropparams *crp)
{
  double *d, *fd;
  double *i, *s, *c;                /* for clear viewing. */
  struct cropparams *p=crp->p;

  /* First check if the four sides of the crop box are in the image.*/
  fd=(d=crp->corners)+8;
  s=p->imgs[crp->in_ind].sized;
  i=p->imgs[crp->in_ind].corners;
  c=p->imgs[crp->in_ind].equatorcorr;
  do
    {
      if( radecinimg(d, i, s, c) ) return 1;
      d+=2;
    }
  while(d<fd);

  /* None of the crop box corners where within the image. Now check if
     the image corners are within the crop.*/
  s=crp->sized;
  i=crp->corners;
  c=crp->equatorcorr;
  fd=(d=p->imgs[crp->in_ind].corners)+8;
  do
    {
      if( radecinimg(d, i, s, c) ) return 1;
      d+=2;
    }
  while(d<fd);

  return 0;
}

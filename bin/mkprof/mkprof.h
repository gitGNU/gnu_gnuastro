/*********************************************************************
MakeProfiles - Create mock astronomical profiles.
MakeProfiles is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef MOCKGALS_H
#define MOCKGALS_H

#include <gnuastro/threads.h>

#include "main.h"

struct mkonthread
{
  /* General parameters: */
  double                r;   /* Elliptical radius at this point.      */
  double                x;   /* x value of this point.                */
  double               xl;   /* lower  x boundary                     */
  double               xh;   /* higher x boundary.                    */
  double                y;   /* y value when integrating over x.      */
  double               yl;   /* lower  y boundary.                    */
  double               yh;   /* higher y boundary.                    */
  double                c;   /* Cosine of the position angle.         */
  double                s;   /* Sine of the position angle.           */
  double                q;   /* axis ratio of the position angle.     */
  double               xc;   /* Center in C of created(oversampled).  */
  double               yc;   /* Center in C of created(oversampled).  */
  double (*profile)(struct mkonthread *); /* Function to use.         */
  double           truncr;   /* Truncation radius in pixels.          */
  double         intruncr;   /* Inner truncation radius in pixels.    */
  long           width[2];   /* Enclosing box in FITS axes, not C.    */
  float          peakflux;   /* Flux at profile peak.                 */
  float        brightness;   /* The brightness of the profile.        */
  int                func;   /* Radial function of the profile.       */
  long            *onaxes;   /* Sides of the unover-sampled image.    */
  long        fpixel_i[2];   /* fpixel_i before running overlap.      */
  int          correction;   /* ==1: correct the pixels afterwards.   */

  /* Random number generator: */
  gsl_rng            *rng;   /* Copy of main random number generator. */

  /* Profile specific parameters: */
  double        sersic_re;   /* r/re in Sersic profile.               */
  double     sersic_inv_n;   /* Sersic index of Sersic profile.       */
  double        sersic_nb;   /* Negative of b(n) constant.            */

  double   moffat_alphasq;   /* r divided by alpha in Moffat.         */
  double        moffat_nb;   /* Negative beta in the Moffat.          */

  double       gaussian_c;   /* Constant value in Gaussian.           */

  double       fixedvalue;   /* Value of a point source.              */

  /* General parameters */
  struct mkprofparams  *p;   /* Pointer to the main.h structure.      */
  size_t          *indexs;   /* Indexs to build on this thread.       */
  pthread_barrier_t    *b;   /* Pthread barrier pointer.              */
  struct builtqueue  *ibq;   /* Internally built queue.               */
};





void
mkprof(struct mkprofparams *p);

#endif

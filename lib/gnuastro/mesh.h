/*********************************************************************
meshgrid -- Create a mesh grid ready for multithreaded analysis.
This is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef __GAL_MESH_H__
#define __GAL_MESH_H__



#include <wcslib/wcs.h>

#include <gnuastro/threads.h>





/* Operations to do on each mesh. If input parameters are needed (for
   example the quantile), they are given as other argument(s) to the
   fillcharray function. */
#define GAL_MESH_MAX_NUM_CHARRAY  2 /* Maximum number of charrays.                */
#define GAL_MESH_INTERP_ALL       1 /* Interpolate over the whole image as one.   */
#define GAL_MESH_INTERP_CHANNEL   2 /* Interpolate over each channel individually.*/


/* The minimum number of acceptable nearest pixels. */
#define GAL_MESH_MIN_ACCEPTABLE_NEAREST 3



struct gal_mesh_thread_params
{
  /* For convolve: */
  float           *conv; /* The convolved array.                        */
  size_t         *chbrd; /* Bordering x and y values all channels.      */

  /* For all: */
  struct gal_mesh_params *mp; /* Pointer to gal_mesh_params structure.  */
  size_t             id; /* The thread ID starting from zero.           */
};




/*
   garray:
   -------

   Or grid-array. It is used for operations on the mesh grid, where
   one value is to be assigned for each mesh. It has one element for
   each mesh in the image.  Each channel has its own part of this
   larger array. The respective parts have gs0*gs1 elements. There are
   `nch' parts (or channels). in total.

   In short, the meshs in each channel have to be contiguous to
   facilitate the neighbor analysis in interpolation and other channel
   specific jobs.g

   The operations on the meshs might need more than one output, for
   example the mean and the standard deviation. So we have two garrays
   and two nearest arrays. So the garrays have to be used such that
   they are both either valid on one mesh or not.
*/
struct gal_mesh_params
{
  /* Image: */
  void              *img; /* Input image array.                          */
  size_t              s0; /* Height of input image.                      */
  size_t              s1; /* Width of input image.                       */

  /* Threads: */
  size_t      numthreads; /* Number of CPU threads.                      */
  size_t         *indexs; /* 2D array of mesh indexs for each thread.    */
  size_t        thrdcols; /* The number of columns in indexs array.      */
  pthread_barrier_t    b; /* pthreads barrier for running threads.       */

  /* Channel(s): */
  size_t             nch; /* Total number of channels.                   */
  size_t            nch1; /* Number of channels along first FITS axis.   */
  size_t            nch2; /* Number of channels along first FITS axis.   */
  size_t             gs0; /* Number of meshes on axis 0 in each channel. */
  size_t             gs1; /* Number of meshes on axis 1 in each channel. */

  /* Meshs: */
  float     lastmeshfrac; /* Last mesh fraction of size, to add new.     */
  size_t        meshsize; /* Size of each mesh.                          */
  size_t          nmeshc; /* Number of meshes in each channel.           */
  size_t          nmeshi; /* Number of meshes in all image.              */
  size_t          *start; /* Starting pixel for each mesh.               */
  size_t          *types; /* The type of each mesh.                      */
  size_t        *chindex; /* The index of each mesh in its channel.      */
  size_t       *imgindex; /* The index of each mesh in the image.        */
  size_t           maxs0; /* Maximum number of rows in all types.        */
  size_t           maxs1; /* Maximum number of columns in all types.     */

  /* garrays: */
  int           ngarrays; /* Number of garrays in this run.              */
  float         *garray1; /* Either equal to cgarray1 or fgarray1.       */
  float         *garray2; /* Either equal to cgarray2 or fgarray2.       */
  float        *cgarray1; /* In cgarray1 or cgarray2, the meshs in each  */
  float        *cgarray2; /*    channel are contiguous.                  */
  float        *fgarray1; /* In fgarray1 or fgarray2, we have contiguous */
  float        *fgarray2; /*    meshs in the full image. Ignore channels.*/

  /* Operate on each mesh: */
  void           *params; /* Pointer to parameters structure of caller.  */
  void        *oneforall; /* One array that can contain all the meshs.   */

  /* Interpolation: */
  float       mirrordist; /* For finding the mode. Distance after mirror.*/
  float         minmodeq; /* Minimum acceptable quantile for the mode.   */
  unsigned char     *byt; /* To keep track of pixels already checked.    */
  size_t      numnearest; /* Number of the nearest pixels for interp.    */
  float        *nearest1; /* Array keeping nearest pixels for garray1.   */
  float        *nearest2; /* Array keeping nearest pixels for garray2.   */
  int    interponlyblank; /* Only interpolate over blank pixels.         */
  float      *outgarray1; /* The interpolated garray1.                   */
  float      *outgarray2; /* The interpolated garray2.                   */
  int  fullinterpolation; /* ==1: Ignore channels in interpolation.      */
  char         *errstart; /* First sentence for error message.           */

  /* Smoothing: */
  size_t     smoothwidth; /* Width of smoothing kernel.                  */
  int         fullsmooth; /* ==1: Ignore channels in smoothing.          */

  /* Convolution: */
  float          *kernel; /* Convolution kernel.                         */
  size_t             ks0; /* Size of kernel along first C axis.          */
  size_t             ks1; /* Size of kernel along second C axis.         */
  int    fullconvolution; /* ==1: Convove over all channels.             */

  /* Mesh types and information: */
  int     meshbasedcheck; /* ==1: use one pixel for each mesh in checks. */
  size_t          ts0[4]; /* Size (along first FITS axis) of mesh types. */
  size_t          ts1[4]; /* Size (along second FITS axis) of mesh types.*/
};

size_t
gal_mesh_ch_based_id_from_gid(struct gal_mesh_params *mp, size_t gid);

size_t
gal_mesh_gid_from_ch_based_id(struct gal_mesh_params *mp, size_t chbasedid);

size_t
gal_mesh_img_xy_to_mesh_id(struct gal_mesh_params *mp, size_t x, size_t y);

void
gal_check_mesh_id(struct gal_mesh_params *mp, long **out);

void
gal_mesh_check_garray(struct gal_mesh_params *mp, float **out1, float **out2);

void
gal_mesh_value_file(struct gal_mesh_params *mp, char *filename, char *extname1,
                    char *extname2, struct wcsprm *wcs, char *spack_string);

void
gal_mesh_full_garray(struct gal_mesh_params *mp, int reverse);

void
gal_mesh_make_mesh(struct gal_mesh_params *mp);

void
gal_mesh_free_mesh(struct gal_mesh_params *mp);

void
gal_mesh_operate_on_mesh(struct gal_mesh_params *mp, void *(*meshfunc)(void *),
                         size_t oneforallsize, int makegarray2, int initialize);

void
gal_mesh_interpolate(struct gal_mesh_params *mp, char *errstart);

void
gal_mesh_smooth(struct gal_mesh_params *mp);

void
gal_mesh_spatial_convolve_on_mesh(struct gal_mesh_params *mp, float **conv);

void
gal_mesh_change_to_full_convolution(struct gal_mesh_params *mp, float *conv);

#endif
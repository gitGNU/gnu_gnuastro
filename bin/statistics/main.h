/*********************************************************************
Statistics - Statistical analysis on input dataset.
Statistics is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef MAIN_H
#define MAIN_H

/* Include necessary headers */
#include <gnuastro/data.h>

#include <options.h>

/* Progarm names.  */
#define PROGRAM_NAME   "Statistics"    /* Program full name.       */
#define PROGRAM_EXEC   "aststatistics" /* Program executable name. */
#define PROGRAM_STRING PROGRAM_NAME" (" PACKAGE_NAME ") " PACKAGE_VERSION





/* Input formats. */
enum statistics_input_format
  {
    INPUT_FORMAT_INVALID,

    INPUT_FORMAT_TABLE,
    INPUT_FORMAT_IMAGE,
  };





/* Main program parameters structure */
struct statisticsparams
{
  /* From command-line */
  struct gal_options_common_params cp; /* Common parameters.             */
  struct gal_linkedlist_ill  *singlevalue; /* Single value calculations. */
  struct gal_linkedlist_dll  *tp_args; /* Arguments for printing.        */
  char          *inputname;  /* Input filename.                          */
  char             *column;  /* Column name or number if input is table. */
  char             *refcol;  /* Reference column name or number.         */
  float       greaterequal;  /* Only use values >= this value.           */
  float           lessthan;  /* Only use values <  this value.           */
  float           quantmin;  /* Quantile min or range: from Q to 1-Q.    */
  float           quantmax;  /* Quantile maximum.                        */
  uint8_t           ontile;  /* Do single value calculations on tiles.   */
  uint8_t      interpolate;  /* Use interpolation to fill blank tiles.   */

  uint8_t        asciihist;  /* Print an ASCII histogram.                */
  uint8_t         asciicfp;  /* Print an ASCII cumulative frequency plot.*/
  uint8_t        histogram;  /* Save histogram in output.                */
  uint8_t       cumulative;  /* Save cumulative distibution in output.   */
  float      sigclipmultip;  /* Multiple of sigma in sigma clipping.     */
  float       sigclipparam;  /* Tolerance to stop or number of clips.    */
  double            mirror;  /* Mirror value for hist and CFP.           */

  size_t           numbins;  /* Number of bins in histogram or CFP.      */
  size_t      numasciibins;  /* Number of bins in ASCII plots.           */
  size_t       asciiheight;  /* Height of ASCII histogram or CFP plots.  */
  uint8_t        normalize;  /* set the sum of all bins to 1.            */
  float        onebinstart;  /* Shift bins to start at this value.       */
  uint8_t        maxbinone;  /* Set the maximum bin to 1.                */
  float         mirrordist;  /* Maximum distance after mirror for mode.  */

  /* Internal */
  uint8_t      inputformat;  /* Format of input dataset.                 */
  int          numoutfiles;  /* Number of output files made in this run. */
  uint8_t        needssort;  /* If sorting is needed.                    */
  gal_data_t        *input;  /* Input data structure.                    */
  gal_data_t       *sorted;  /* Sorted input data structure.             */
  gal_data_t    *reference;  /* Reference data structure.                */
  int               isfits;  /* Input is a FITS file.                    */
  int             hdu_type;  /* Type of HDU (image or table).            */
  time_t           rawtime;  /* Starting time of the program.            */
};

#endif

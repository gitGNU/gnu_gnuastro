/*********************************************************************
NoiseChisel - Detect and segment signal in a noisy dataset.
NoiseChisel is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef UI_H
#define UI_H





/* Available letters for short options:

   a b f j l n u w x z
   A H J W X Y
*/
enum option_keys_enum
{
  /* With short-option version. */
  UI_KEY_LARGETILESIZE      = 'L',
  UI_KEY_KERNEL             = 'k',
  UI_KEY_SKYSUBTRACTED      = 'E',
  UI_KEY_MINSKYFRAC         = 'B',
  UI_KEY_MIRRORDIST         = 'r',
  UI_KEY_MODMEDQDIFF        = 'Q',
  UI_KEY_QTHRESH            = 't',
  UI_KEY_ERODE              = 'e',
  UI_KEY_OPENING            = 'p',
  UI_KEY_SIGMACLIP          = 's',
  UI_KEY_DTHRESH            = 'R',
  UI_KEY_DETSNMINAREA       = 'i',
  UI_KEY_DETQUANT           = 'c',
  UI_KEY_DILATE             = 'd',
  UI_KEY_SEGSNMINAREA       = 'm',
  UI_KEY_SEGQUANT           = 'g',
  UI_KEY_KEEPMAXNEARRIVER   = 'v',
  UI_KEY_GTHRESH            = 'G',
  UI_KEY_MINRIVERLENGTH     = 'y',
  UI_KEY_OBJBORDERSN        = 'O',
  UI_KEY_CONTINUEAFTERCHECK = 'C',


  /* Only with long version (start with a value 1000, the rest will be set
     automatically). */
  UI_KEY_KHDU               = 1000,
  UI_KEY_MINNUMFALSE,
  UI_KEY_ONLYDETECTION,
  UI_KEY_GROWNCLUMPS,
  UI_KEY_SMOOTHWIDTH,
  UI_KEY_CHECKQTHRESH,
  UI_KEY_ERODENGB,
  UI_KEY_NOERODEQUANT,
  UI_KEY_OPENINGNGB,
  UI_KEY_CHECKDETSKY,
  UI_KEY_CHECKDETSN,
  UI_KEY_CLEANDILATED,
  UI_KEY_CHECKDETECTION,
  UI_KEY_CHECKSKY,
  UI_KEY_CHECKCLUMPSN,
  UI_KEY_CHECKSEGMENTATION,
};





void
ui_read_check_inputs_setup(int argc, char *argv[],
                           struct noisechiselparams *p);

void
ui_abort_after_check(struct noisechiselparams *p, char *filename,
                     char *file2name, char *description);

void
ui_free_report(struct noisechiselparams *p, struct timeval *t1);

#endif

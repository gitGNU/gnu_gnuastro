/*********************************************************************
Convolve - Convolve input data with a given kernel.
Convolve is part of GNU Astronomy Utilities (gnuastro) package.

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
along with gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#ifndef ARGS_H
#define ARGS_H

#include "ui.h"
#include "commonargs.h"
#include "fixedstringmacros.h"


/* Definition parameters for the argp: */
const char *argp_program_version=SPACK_STRING"\n"COPYRIGHT
  "\n\nWritten by Mohammad Akhlaghi";
const char *argp_program_bug_address=PACKAGE_BUGREPORT;
static char args_doc[] = "InputFile1 [InputFile2] ... [InputFile4]";





const char doc[] =
  /* Before the list of options: */
  TOPHELPINFO
  SPACK_NAME" will convolve an input image with a given spatial kernel "
  "(image) in the spatial domain (edge-corrected) or frequency domain.\n"
  MOREHELPINFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;





/* Free letters for options:

   a b c d e g i j l m n p r t u w x y z
   A B E F G I J L M O Q R T U W X Y Z

   Free numbers: >=502
*/
static struct argp_option options[] =
  {
    {
      0, 0, 0, 0,
      "Operating modes:",
      -1
    },





    {
      0, 0, 0, 0,
      "Input:",
      1
    },
    {
      "kernel",
      'k',
      "STR",
      0,
      "Name of kernel for convolution.",
      1
    },
    {
      "khdu",
      'H',
      "STR",
      0,
      "HDU of kernel file.",
      1
    },
    {
      "nokernelflip",
      500,
      0,
      0,
      "Do not flip the kernel image.",
      1
    },
    {
      "nokernelnorm",
      501,
      0,
      0,
      "Do not normalize the kernel image.",
      1
    },




    {
      0, 0, 0, 0,
      "Output:",
      2
    },
    {
      "noedgecorrection",
      'C',
      0,
      0,
      "Do not correct the edges in the spatial domain.",
      2
    },
    {
      "viewfreqsteps",
      'v',
      0,
      0,
      "View the steps in the frequency domain.",
      2
    },





    {
      "spatial",
      's',
      0,
      0,
      "Spatial domain convolution.",
      -1
    },
    {
      "frequency",
      'f',
      0,
      0,
      "Frequency domain convolution.",
      -1
    },

    {0}
  };




















/* Parse a single option: */
static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{

  /* Save the arguments structure: */
  struct convolveparams *p = state->input;

  /* Set the pointer to the common parameters for all programs
     here: */
  state->child_inputs[0]=&p->cp;

  /* In case the user incorrectly uses the equal sign (for example
     with a short format or with space in the long format, then `arg`
     start with (if the short version was called) or be (if the long
     version was called with a space) the equal sign. So, here we
     check if the first character of arg is the equal sign, then the
     user is warned and the program is stopped: */
  if(arg && arg[0]=='=')
    argp_error(state, "Incorrect use of the equal sign (`=`). For short "
	       "options, `=` should not be used and for long options, "
	       "there should be no space between the option, equal sign "
	       "and value.");

  switch(key)
    {

    /* Inputs: */
    case 'k':
      p->up.kernelname=arg;
      p->up.kernelnameset=1;
      break;
    case 'H':
      p->up.khdu=arg;
      p->up.khduset=1;
      break;
    case 500:
      p->kernelflip=0;
      break;
    case 501:
      p->kernelnorm=0;
      break;


    /* Output: */
    case 'C':
      p->edgecorrection=0;
      break;


   /* Operating mode: */
    case 's':
      if(p->up.spatialset)
	argp_error(state, "Only one of spatial or frequency domain "
                   "convolution modes may be chosen.");
      p->spatial=1;
      p->frequency=0;
      p->up.spatialset=p->up.frequencyset=1;
      break;
    case 'f':
      if(p->up.spatialset)
	argp_error(state, "Only one of spatial or frequency domain "
                   "convolution modes may be chosen.");
      p->spatial=0;
      p->frequency=1;
      p->up.spatialset=p->up.frequencyset=1;
      break;
    case 'v':
      p->viewfreqsteps=1;
      break;




    /* Read the non-option arguments: */
    case ARGP_KEY_ARG:
      if(p->up.inputname)
        argp_error(state, "Only one input file (argument) is required.");
      p->up.inputname=arg;
      break;






    /* The command line options and arguments are finished. */
    case ARGP_KEY_END:
      if(p->cp.setdirconf==0 && p->cp.setusrconf==0
	 && p->cp.printparams==0)
	{
	  if(state->arg_num==0)
	    argp_error(state, "No argument given!");
	  if(p->up.inputname==NULL)
	    argp_error(state, "No input files provided!");
	}
      break;






    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}





/* Specify the children parsers: */
struct argp_child children[]=
  {
    {&commonargp, 0, NULL, 0},
    {0, 0, 0, 0}
  };





/* Basic structure defining the whole argument reading process. */
static struct argp thisargp = {options, parse_opt, args_doc,
			       doc, children, NULL, NULL};

#endif
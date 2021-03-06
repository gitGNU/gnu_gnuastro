/*********************************************************************
Fits - View and manipulate FITS extensions and/or headers.
Fits is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2017, Free Software Foundation, Inc.

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

#include <errno.h>
#include <error.h>
#include <string.h>

#include <gnuastro/list.h>
#include <gnuastro/fits.h>
#include <gnuastro/blank.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "fits.h"
#include "keywords.h"



int
fits_has_error(struct fitsparams *p, int actioncode, char *string, int status)
{
  char *action=NULL;
  int r=EXIT_SUCCESS;

  switch(actioncode)
    {
    case FITS_ACTION_DELETE:        action="deleted";      break;
    case FITS_ACTION_RENAME:        action="renamed";      break;
    case FITS_ACTION_UPDATE:        action="updated";      break;
    case FITS_ACTION_WRITE:         action="written";      break;
    case FITS_ACTION_COPY:          action="copied";       break;
    case FITS_ACTION_REMOVE:        action="renoved";      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at `%s' so we "
            "can fix this problem. The value of `actioncode' must not be %d",
            __func__, PACKAGE_BUGREPORT, actioncode);
    }

  if(p->quitonerror)
    {
      fits_report_error(stderr, status);
      error(EXIT_FAILURE, 0, "%s: not %s: %s\n", __func__, action, string);
    }
  else
    {
      fprintf(stderr, "Not %s: %s\n", action, string);
      r=EXIT_FAILURE;
    }
  return r;
}





/* Print all the extension informations. */
void
fits_print_extension_info(struct fitsparams *p)
{
  uint16_t *ui16;
  fitsfile *fptr;
  gal_data_t *cols=NULL, *tmp;
  char **tstra, **estra, **sstra;
  size_t i, numext, *dsize, ndim;
  int j, nc, numhdu, hdutype, status=0, type;
  char *msg, *tstr=NULL, sstr[1000], extname[FLEN_VALUE];


  /* Open the FITS file and read the first extension type, upon moving to
     the next extension, we will read its type, so for the first we will
     need to do it explicitly. */
  fptr=gal_fits_hdu_open(p->filename, "0", READONLY);
  if (fits_get_hdu_type(fptr, &hdutype, &status) )
    gal_fits_io_error(status, "reading first extension");


  /* Get the number of HDUs. */
  if( fits_get_num_hdus(fptr, &numhdu, &status) )
    gal_fits_io_error(status, "finding number of HDUs");
  numext=numhdu;


  /* Allocate all the columns (in reverse order, since this is a simple
     linked list). */
  gal_list_data_add_alloc(&cols, NULL, GAL_TYPE_STRING, 1, &numext, NULL, 1,
                          p->cp.minmapsize, "HDU_SIZE", "name", "Size of "
                          "image or table number of rows and columns.");
  gal_list_data_add_alloc(&cols, NULL, GAL_TYPE_STRING, 1, &numext, NULL, 1,
                          p->cp.minmapsize, "HDU_TYPE", "name", "Image "
                          "data type or `table' format (ASCII or binary).");
  gal_list_data_add_alloc(&cols, NULL, GAL_TYPE_STRING, 1, &numext, NULL, 1,
                          p->cp.minmapsize, "EXTNAME", "name",
                          "Extension name of this HDU (EXTNAME in FITS).");
  gal_list_data_add_alloc(&cols, NULL, GAL_TYPE_UINT16, 1, &numext, NULL, 1,
                          p->cp.minmapsize, "HDU_INDEX", "count", "Index "
                          "(starting from zero) of each HDU (extension).");


  /* Keep pointers to the array of each column for easy writing. */
  ui16  = cols->array;
  estra = cols->next->array;
  tstra = cols->next->next->array;
  sstra = cols->next->next->next->array;

  cols->next->disp_width=15;
  cols->next->next->disp_width=15;


  /* Fill in each column. */
  for(i=0;i<numext;++i)
    {
      /* Work based on the type of the extension. */
      switch(hdutype)
        {
        case IMAGE_HDU:
          gal_fits_img_info(fptr, &type, &ndim, &dsize);
          tstr=gal_type_name(type , 1);
          break;

        case ASCII_TBL:
        case BINARY_TBL:
          ndim=2;
          tstr = hdutype==ASCII_TBL ? "table_ascii" : "table_binary";
          dsize=gal_data_malloc_array(GAL_TYPE_SIZE_T, 2);
          gal_fits_tab_size(fptr, dsize+1, dsize);
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! the `hdutype' code %d not "
                "recognized", __func__, hdutype);
        }


      /* Read the extension name*/
      fits_read_keyword(fptr, "EXTNAME", extname, NULL, &status);
      switch(status)
        {
        case 0:
          gal_fits_key_clean_str_value(extname);
          break;

        case KEY_NO_EXIST:
          sprintf(extname, "%s", GAL_BLANK_STRING);
          status=0;
          break;

        default:
          gal_fits_io_error(status, "reading EXTNAME keyword");
        }
      status=0;


      /* Write the size into a string. `sprintf' returns the number of
         written characters (excluding the `\0'). So for each dimension's
         size that is written, we add to `nc' (the number of
         characters). Note that FITS allows blank extensions, in those
         cases, return "0". */
      if(ndim>0)
        {
          nc=0;
          for(j=ndim-1;j>=0;--j)
            nc += sprintf(sstr+nc, "%zux", dsize[j]);
          sstr[nc-1]='\0';
          free(dsize);
        }
      else
        {
          sstr[0]='0';
          sstr[1]='\0';
        }


      /* Write the strings into the columns. */
      j=0;
      for(tmp=cols; tmp!=NULL; tmp=tmp->next)
        {
          switch(j)
            {
            case 0: ui16[i]=i;                                    break;
            case 1: gal_checkset_allocate_copy(extname, estra+i); break;
            case 2: gal_checkset_allocate_copy(tstr, tstra+i);    break;
            case 3: gal_checkset_allocate_copy(sstr, sstra+i);    break;
            }
          ++j;
        }


      /* Move to the next extension if we aren't on the last extension. */
      if( i!=numext-1 && fits_movrel_hdu(fptr, 1, &hdutype, &status) )
        {
          asprintf(&msg, "moving to hdu %zu", i+1);
          gal_fits_io_error(status, msg);
        }
    }


  /* Print the resutls. */
  if(!p->cp.quiet)
    {
      printf("%s\nRun on %s-----\n", PROGRAM_STRING, ctime(&p->rawtime));
      printf("HDU (extension) information: `%s'.\n", p->filename);
      printf(" Column 1: Index (counting from 0, usable with `--hdu').\n");
      printf(" Column 2: Name (`EXTNAME' in FITS standard, usable with "
             "`--hdu').\n");
      printf(" Column 3: Image data type or `table' format (ASCII or "
             "binary).\n");
      printf(" Column 4: Size of data in HDU.\n");
      printf("-----\n");
    }
  gal_table_write(cols, NULL, GAL_TABLE_FORMAT_TXT, NULL, 0);
  gal_list_data_free(cols);
}





static void
fits_hdu_remove(struct fitsparams *p, int *r)
{
  char *hdu;
  fitsfile *fptr;
  int status=0, hdutype;

  while(p->remove)
    {
      /* Pop-out the top element. */
      hdu=gal_list_str_pop(&p->remove);

      /* Open the FITS file at the specified HDU. */
      fptr=gal_fits_hdu_open(p->filename, hdu, READWRITE);

      /* Delete the extension. */
      if( fits_delete_hdu(fptr, &hdutype, &status) )
        *r=fits_has_error(p, FITS_ACTION_REMOVE, hdu, status);

      /* Close the file. */
      fits_close_file(fptr, &status);
    }
}





static void
fits_hdu_copy(struct fitsparams *p, int cut1_copy0, int *r)
{
  char *hdu;
  fitsfile *in, *out;
  int status=0, hdutype;
  gal_list_str_t *list = cut1_copy0 ? p->cut : p->copy;

  /* Open the output file. */
  out=gal_fits_open_to_write(p->cp.output);

  /* Copy all the given extensions. */
  while(list)
    {
      /* Pop-out the top element. */
      hdu=gal_list_str_pop(&list);

      /* Open the FITS file at the specified HDU. */
      in=gal_fits_hdu_open(p->filename, hdu, READWRITE);

      /* Copy to the extension. */
      if( fits_copy_hdu(in, out, 0, &status) )
        *r=fits_has_error(p, FITS_ACTION_COPY, hdu, status);

      /* If this is a `cut' operation, then remove the extension. */
      if(cut1_copy0)
        if( fits_delete_hdu(in, &hdutype, &status) )
          *r=fits_has_error(p, FITS_ACTION_REMOVE, hdu, status);

      /* Close the input file. */
      fits_close_file(in, &status);
    }

  /* Close the output file. */
  fits_close_file(out, &status);
}





int
fits(struct fitsparams *p)
{
  int r=EXIT_SUCCESS, printhduinfo=1;;

  switch(p->mode)
    {
    /* Keywords, we have a separate set of functions in `keywords.c'. */
    case FITS_MODE_KEY:
      r=keywords(p);
      break;

    /* HDU, functions defined here. */
    case FITS_MODE_HDU:
      if(p->copy)
        {
          fits_hdu_copy(p, 0, &r);
          printhduinfo=0;
        }
      if(p->cut)
        {
          fits_hdu_copy(p, 1, &r);
          printhduinfo=0;
        }
      if(p->remove)
        {
          fits_hdu_remove(p, &r);
          printhduinfo=0;
        }

      if(printhduinfo)
        fits_print_extension_info(p);
      break;

    /* Not recognized. */
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to address "
            "the problem. The code %d is not recognized for p->mode",
            __func__, PACKAGE_BUGREPORT, p->mode);
    }

  return r;
}

/*********************************************************************
blank -- Deal with blank values in a datasets
This is part of GNU Astronomy Utilities (Gnuastro) package.

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gnuastro/data.h>
#include <gnuastro/tile.h>
#include <gnuastro/blank.h>

#include <gnuastro-internal/checkset.h>




/* Write the blank value of the type into an already allocate space. Note
   that for STRINGS, pointer should actually be `char **'. */
void
gal_blank_write(void *ptr, uint8_t type)
{
  switch(type)
    {
    /* Numeric types */
    case GAL_TYPE_UINT8:   *(uint8_t  *)ptr = GAL_BLANK_UINT8;    break;
    case GAL_TYPE_INT8:    *(int8_t   *)ptr = GAL_BLANK_INT8;     break;
    case GAL_TYPE_UINT16:  *(uint16_t *)ptr = GAL_BLANK_UINT16;   break;
    case GAL_TYPE_INT16:   *(int16_t  *)ptr = GAL_BLANK_INT16;    break;
    case GAL_TYPE_UINT32:  *(uint32_t *)ptr = GAL_BLANK_UINT32;   break;
    case GAL_TYPE_INT32:   *(int32_t  *)ptr = GAL_BLANK_INT32;    break;
    case GAL_TYPE_UINT64:  *(uint64_t *)ptr = GAL_BLANK_UINT64;   break;
    case GAL_TYPE_INT64:   *(int64_t  *)ptr = GAL_BLANK_INT64;    break;
    case GAL_TYPE_FLOAT32: *(float    *)ptr = GAL_BLANK_FLOAT32;  break;
    case GAL_TYPE_FLOAT64: *(double   *)ptr = GAL_BLANK_FLOAT64;  break;

    /* String type. */
    case GAL_TYPE_STRING:
      gal_checkset_allocate_copy(GAL_BLANK_STRING, ptr);
      break;

    /* Complex types */
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: complex types are not yet supported",
            __func__);

    /* Unrecognized type. */
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__,
            type);
    }
}





/* Allocate some space for the given type and put the blank value into
   it. */
void *
gal_blank_alloc_write(uint8_t type)
{
  void *out;

  /* Allocate the space to keep the blank value. */
  out=gal_data_malloc_array(type, 1);

  /* Put the blank value in the allcated space. */
  gal_blank_write(out, type);

  /* Return the allocated space. */
  return out;
}





/* Initialize (set all the values in the array) with the blank value of the
   given type. */
void
gal_blank_initialize(gal_data_t *input)
{
  GAL_TILE_PARSE_OPERATE(input, NULL, 0, 0, {*i=b;});
}





/* Return 1 if the dataset has a blank value and zero if it doesn't. Before
   checking the dataset, this function will look at its flags. If the
   `GAL_DATA_FLAG_HASBLANK' or `GAL_DATA_FLAG_DONT_CHECK_ZERO' bits of
   `input->flag' are set to 1, this function will not do any check and will
   just use the information in the flags.

   If you want to re-check a dataset which has non-zero flags, then
   explicitly set the appropriate flag to zero before calling this
   function. When there are no other flags, you can just set `input->flags'
   to zero, otherwise you can use this expression:

       input->flags &= ~ (GAL_DATA_FLAG_HASBLANK | GAL_DATA_FLAG_USE_ZERO);

   This function has no side-effects on the dataset: it will not toggle the
   flags, to avoid repeating parsing of the full dataset multiple times
   (when it occurs), please toggle the flags your self after the first
   check. */
#define HAS_BLANK(IT) {                                                 \
    IT b, *a=input->array, *af=a+input->size, *start;                   \
    gal_blank_write(&b, block->type);                                   \
                                                                        \
    /* If this is a tile, not a full block. */                          \
    if(input!=block)                                                    \
      start=gal_tile_start_end_ind_inclusive(input, block, start_end_inc); \
                                                                        \
    /* Go over all the elements. */                                     \
    while( start_end_inc[0] + increment <= start_end_inc[1] )           \
      {                                                                 \
        /* Necessary when we are on a tile. */                          \
        if(input!=block)                                                \
          af = ( a = start + increment ) + input->dsize[input->ndim-1]; \
                                                                        \
        /* Check for blank values. */                                   \
        if(b==b) do if(*a==b)  { hasblank=1; break; } while(++a<af);    \
        else     do if(*a!=*a) { hasblank=1; break; } while(++a<af);    \
                                                                        \
        /* Necessary when we are on a tile. */                          \
        if(input!=block)                                                \
          increment += gal_tile_block_increment(block, input->dsize,    \
                                                num_increment++, NULL); \
        else break;                                                     \
      }                                                                 \
  }
int
gal_blank_present(gal_data_t *input, int updateflag)
{
  int hasblank=0;
  char **str, **strf;
  size_t increment=0, num_increment=1;
  gal_data_t *block=gal_tile_block(input);
  size_t start_end_inc[2]={0,block->size-1}; /* -1: this is INCLUSIVE. */

  /* If there is nothing in the array (its size is zero), then return 0 (no
     blank is present. */
  if(input->size==0) return 0;

  /* From the user's flags, it might not be necessary to go through the
     dataset any more.  */
  if( ( input->flag & GAL_DATA_FLAG_HASBLANK )
      || ( input->flag & GAL_DATA_FLAG_BLANK_CH ) )
    return input->flag & GAL_DATA_FLAG_HASBLANK;

  /* Go over the pixels and check: */
  switch(block->type)
    {
    /* Numeric types */
    case GAL_TYPE_UINT8:     HAS_BLANK( uint8_t  );    break;
    case GAL_TYPE_INT8:      HAS_BLANK( int8_t   );    break;
    case GAL_TYPE_UINT16:    HAS_BLANK( uint16_t );    break;
    case GAL_TYPE_INT16:     HAS_BLANK( int16_t  );    break;
    case GAL_TYPE_UINT32:    HAS_BLANK( uint32_t );    break;
    case GAL_TYPE_INT32:     HAS_BLANK( int32_t  );    break;
    case GAL_TYPE_UINT64:    HAS_BLANK( uint64_t );    break;
    case GAL_TYPE_INT64:     HAS_BLANK( int64_t  );    break;
    case GAL_TYPE_FLOAT32:   HAS_BLANK( float    );    break;
    case GAL_TYPE_FLOAT64:   HAS_BLANK( double   );    break;

    /* String. */
    case GAL_TYPE_STRING:
      if(input!=block)
        error(EXIT_FAILURE, 0, "%s: tile mode is currently not supported for "
              "strings", __func__);
      strf = (str=input->array) + input->size;
      do if(!strcmp(*str++,GAL_BLANK_STRING)) return 1; while(str<strf);
      break;

    /* Complex types */
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: complex types are not yet supported",
            __func__);

    /* Bit */
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit type datasets are not yet supported",
            __func__);

    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, block->type);
    }

  /* Update the flag if requested. */
  if(updateflag)
    {
      input->flag |= GAL_DATA_FLAG_BLANK_CH;
      if(hasblank) input->flag |= GAL_DATA_FLAG_HASBLANK;
      else         input->flag &= ~GAL_DATA_FLAG_HASBLANK;
    }

  /* If there was a blank value, then the function would have returned with
     a value of 1. So if it reaches here, then we can be sure that there
     was no blank values, hence, return 0. */
  return hasblank;
}







/* Create a dataset of the the same size as the input, but with an uint8_t
   type that has a value of 1 for data that are blank and 0 for those that
   aren't. */
#define FLAG_BLANK(IT) {                                                \
    IT b, *a=input->array;                                              \
    gal_blank_write(&b, input->type);                                   \
    if(b==b) /* Blank value can be checked with the equal comparison */ \
      do { *o = *a==b;  ++a; } while(++o<of);                           \
    else     /* Blank value will fail with the equal comparison */      \
      do { *o = *a!=*a; ++a; } while(++o<of);                           \
  }
gal_data_t *
gal_blank_flag(gal_data_t *input)
{
  uint8_t *o, *of;
  gal_data_t *out;
  char **str=input->array, **strf=str+input->size;

  if( gal_blank_present(input, 0) )
    {
      /* Allocate a non-cleared output array, we are going to parse the
         input and fill in each element. */
      out=gal_data_alloc(NULL, GAL_TYPE_UINT8, input->ndim, input->dsize,
                         input->wcs, 0, input->minmapsize, NULL, "bool",
                         NULL);

      /* Set the pointers for easy looping. */
      of=(o=out->array)+input->size;

      /* Go over the pixels and set the output values. */
      switch(input->type)
        {
          /* Numeric types */
        case GAL_TYPE_UINT8:     FLAG_BLANK( uint8_t  );    break;
        case GAL_TYPE_INT8:      FLAG_BLANK( int8_t   );    break;
        case GAL_TYPE_UINT16:    FLAG_BLANK( uint16_t );    break;
        case GAL_TYPE_INT16:     FLAG_BLANK( int16_t  );    break;
        case GAL_TYPE_UINT32:    FLAG_BLANK( uint32_t );    break;
        case GAL_TYPE_INT32:     FLAG_BLANK( int32_t  );    break;
        case GAL_TYPE_UINT64:    FLAG_BLANK( uint64_t );    break;
        case GAL_TYPE_INT64:     FLAG_BLANK( int64_t  );    break;
        case GAL_TYPE_FLOAT32:   FLAG_BLANK( float    );    break;
        case GAL_TYPE_FLOAT64:   FLAG_BLANK( double   );    break;

          /* String. */
        case GAL_TYPE_STRING:
          do *o++ = !strcmp(*str,GAL_BLANK_STRING); while(++str<strf);
          break;

          /* Currently unsupported types. */
        case GAL_TYPE_BIT:
        case GAL_TYPE_COMPLEX32:
        case GAL_TYPE_COMPLEX64:
          error(EXIT_FAILURE, 0, "%s: %s type not yet supported",
                __func__, gal_type_name(input->type, 1));

          /* Bad input. */
        default:
          error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
                __func__, input->type);
        }
    }
  else
    /* Allocate a CLEAR data structure (all zeros). */
    out=gal_data_alloc(NULL, GAL_TYPE_UINT8, input->ndim, input->dsize,
                       input->wcs, 1, input->minmapsize, NULL, "bool", NULL);

  /* Return */
  return out;
}





/* Remove blank elements from a dataset, convert it to a 1D dataset and
   adjust the size properly. In practice this function doesn't `realloc'
   the input array, all it does is to shift the blank eleemnts to the end
   and adjust the size elements of the `gal_data_t'. */
#define BLANK_REMOVE(IT) {                                              \
    IT b, *a=input->array, *af=a+input->size, *o=input->array;          \
    gal_blank_write(&b, input->type);                                   \
    if(b==b)       /* Blank value can be be checked with equal. */      \
      do if(*a!=b)  { ++num; *o++=*a; } while(++a<af);                  \
    else           /* Blank value will fail on equal comparison. */     \
      do if(*a==*a) { ++num; *o++=*a; } while(++a<af);                  \
  }
void
gal_blank_remove(gal_data_t *input)
{
  size_t num=0;

  /* This function currently assumes a contiguous patch of memory. */
  if(input->block && input->ndim!=1 )
    error(EXIT_FAILURE, 0, "%s: tiles in datasets with more dimensions than "
          "1 are not yet supported. Your input has %zu dimensions",
          __func__, input->ndim);

  /* If the dataset doesn't have blank values, then just get the size. */
  if( gal_blank_present(input, 0) )
    {
      /* Shift all non-blank elements to the start of the array. */
      switch(input->type)
        {
        case GAL_TYPE_UINT8:    BLANK_REMOVE( uint8_t  );    break;
        case GAL_TYPE_INT8:     BLANK_REMOVE( int8_t   );    break;
        case GAL_TYPE_UINT16:   BLANK_REMOVE( uint16_t );    break;
        case GAL_TYPE_INT16:    BLANK_REMOVE( int16_t  );    break;
        case GAL_TYPE_UINT32:   BLANK_REMOVE( uint32_t );    break;
        case GAL_TYPE_INT32:    BLANK_REMOVE( int32_t  );    break;
        case GAL_TYPE_UINT64:   BLANK_REMOVE( uint64_t );    break;
        case GAL_TYPE_INT64:    BLANK_REMOVE( int64_t  );    break;
        case GAL_TYPE_FLOAT32:  BLANK_REMOVE( float    );    break;
        case GAL_TYPE_FLOAT64:  BLANK_REMOVE( double   );    break;
        default:
          error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
                __func__, input->type);
        }
    }
  else num=input->size;

  /* Adjust the size elements of the dataset. */
  input->ndim=1;
  input->dsize[0]=input->size=num;

  /* Set the flags to mark that there is no blanks. */
  input->flag |= GAL_DATA_FLAG_BLANK_CH;
  input->flag &= ~GAL_DATA_FLAG_HASBLANK;
}





/* Print the blank value as a string. For the integer types, we'll use the
   PRIxNN keywords of `inttypes.h' (which is imported into Gnuastro from
   Gnulib, so we don't necessarily rely on the host system having it). */
char *
gal_blank_as_string(uint8_t type, int width)
{
  char *blank=NULL, *fmt;

  /* Print the given value. */
  switch(type)
    {
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit types are not implemented yet",
            __func__);
      break;

    case GAL_TYPE_STRING:
      if(width) asprintf(&blank, "%*s", width,  GAL_BLANK_STRING);
      else      asprintf(&blank, "%s",          GAL_BLANK_STRING);
      break;

    case GAL_TYPE_UINT8:
      fmt = width ? "%*"PRIu8 : "%*"PRIu8;
      if(width) asprintf(&blank, fmt, width, (uint8_t)GAL_BLANK_UINT8);
      else      asprintf(&blank, fmt,        (uint8_t)GAL_BLANK_UINT8);
      break;

    case GAL_TYPE_INT8:
      fmt = width ? "%*"PRId8 : "%*"PRId8;
      if(width) asprintf(&blank, fmt, width, (int8_t)GAL_BLANK_INT8);
      else      asprintf(&blank, fmt,        (int8_t)GAL_BLANK_INT8);
      break;

    case GAL_TYPE_UINT16:
      fmt = width ? "%*"PRIu16 : "%*"PRIu16;
      if(width) asprintf(&blank, fmt, width, (uint16_t)GAL_BLANK_UINT16);
      else      asprintf(&blank, fmt,        (uint16_t)GAL_BLANK_UINT16);
      break;

    case GAL_TYPE_INT16:
      fmt = width ? "%*"PRId16 : "%*"PRId16;
      if(width) asprintf(&blank, fmt, width, (int16_t)GAL_BLANK_INT16);
      else      asprintf(&blank, fmt,        (int16_t)GAL_BLANK_INT16);
      break;

    case GAL_TYPE_UINT32:
      fmt = width ? "%*"PRIu32 : "%*"PRIu32;
      if(width) asprintf(&blank, fmt, width, (uint32_t)GAL_BLANK_UINT32);
      else      asprintf(&blank, fmt,        (uint32_t)GAL_BLANK_UINT32);
      break;

    case GAL_TYPE_INT32:
      fmt = width ? "%*"PRId32 : "%*"PRId32;
      if(width) asprintf(&blank, fmt, width, (int32_t)GAL_BLANK_INT32);
      else      asprintf(&blank, fmt,        (int32_t)GAL_BLANK_INT32);
      break;

    case GAL_TYPE_UINT64:
      fmt = width ? "%*"PRIu64 : "%*"PRIu64;
      if(width) asprintf(&blank, fmt, width, (uint64_t)GAL_BLANK_UINT64);
      else      asprintf(&blank, fmt,        (uint64_t)GAL_BLANK_UINT64);
      break;

    case GAL_TYPE_INT64:
      fmt = width ? "%*"PRId64 : "%*"PRId64;
      if(width) asprintf(&blank, fmt, width, (int64_t)GAL_BLANK_INT64);
      else      asprintf(&blank, fmt,        (int64_t)GAL_BLANK_INT64);
      break;

    case GAL_TYPE_FLOAT32:
      if(width) asprintf(&blank, "%*f", width,  (float)GAL_BLANK_FLOAT32);
      else      asprintf(&blank, "%f",          (float)GAL_BLANK_FLOAT32);
      break;

    case GAL_TYPE_FLOAT64:
      if(width) asprintf(&blank, "%*f", width,  (double)GAL_BLANK_FLOAT64);
      else      asprintf(&blank, "%f",          (double)GAL_BLANK_FLOAT64);
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__,
            type);
    }
  return blank;
}

/* OP_CHNGE.C
 * CHANGE opcode
 * Copyright (c) 2005 Ladybridge Systems, All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 * Ladybridge Systems can be contacted via the www.openqm.com web site.
 * 
 * START-HISTORY:
 * 01 Jul 07  2.5-7 Extensive change for PDA merge.
 * 09 Dec 05  2.3-2 Allow new string to be too long for continguous handling
 *                  in op_change().
 * 13 May 05  2.1-15 Added error arg to s_make_contiguous().
 * 06 May 05  2.1-13 Added case insensitive string handling.
 * 16 Sep 04  2.0-1 OpenQM launch. Earlier history details suppressed.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#include "qm.h"
#include "header.h"

Private STRING_CHUNK * change (STRING_CHUNK * src_hdr,
  STRING_CHUNK * old_hdr, STRING_CHUNK * new_hdr, long int change_count,
  long int skip_count);

/* ======================================================================
   op_change()  -  Change substrings - function                           */

void op_change()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  First occurrence to change |  New string                 |
     |-----------------------------|-----------------------------|
     |  Occurences to change       |                             |
     |-----------------------------|-----------------------------|
     |  New substring              |                             |
     |-----------------------------|-----------------------------|
     |  Old substring              |                             |
     |-----------------------------|-----------------------------|
     |  Source string              |                             |
     |=============================|=============================|
 */

 long int skip_count;            /* Occurrences to skip before change */
 long int change_count;          /* Number of occurrences to change */
 STRING_CHUNK * tgt_hdr;
 STRING_CHUNK * src_hdr;
 STRING_CHUNK * old_hdr;
 STRING_CHUNK * new_hdr;
 DESCRIPTOR * descr;

 /* Fetch start occurrence number */

 descr = e_stack - 1;
 GetInt(descr);
 skip_count = descr->data.value;

 /* Fetch count of occurrences to change */

 descr = e_stack - 2;
 GetInt(descr);
 change_count = descr->data.value;

 /* Find details of new substring */
 
 descr = e_stack - 3;
 k_get_string(descr);
 new_hdr = descr->data.str.saddr;

 /* Make old substring contiguous and find details */
 
 descr = e_stack - 4;
 k_get_string(descr);
 descr->data.str.saddr = s_make_contiguous(descr->data.str.saddr, NULL);
 old_hdr = descr->data.str.saddr;

 /* Find source string */

 descr = e_stack - 5;
 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 /* Change string */

 tgt_hdr = change (src_hdr, old_hdr, new_hdr, change_count, skip_count);

 /* Remove parameters */

 k_pop(1);
 k_pop(1);
 k_dismiss();       /* New substring */
 k_dismiss();       /* Old substring */

 if (tgt_hdr)
  {
   k_dismiss();       /* Source string */

   InitDescr(e_stack, STRING);
   (e_stack++)->data.str.saddr = tgt_hdr;
  }
 /* If not changed (tgt == NULL), leave source as result */
}

/* ======================================================================
   op_schange()  -  Change substrings - statement                         */

void op_schange()
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  String to modify           |                             |
     |-----------------------------|-----------------------------| 
     |  Characters to replace      |                             |
     |-----------------------------|-----------------------------| 
     |  Replacement characters     |                             |
     |=============================|=============================|
 */

 STRING_CHUNK * tgt_hdr;
 STRING_CHUNK * src_hdr;
 STRING_CHUNK * old_hdr;
 STRING_CHUNK * new_hdr;
 DESCRIPTOR * descr;
 DESCRIPTOR * tgt_descr;

 /* Find reference to the source string original descriptor and
    remember its location                                               */

 descr = e_stack - 1;
 tgt_descr = descr;
 while (tgt_descr->type == ADDR)
 {
  tgt_descr = tgt_descr->data.d_addr;
 }

 k_get_string(descr);
 src_hdr = descr->data.str.saddr;

 /* Find details of new substring */
 
 descr = e_stack - 2;
 k_get_string(descr);
 new_hdr = descr->data.str.saddr;

 /* Make old substring contiguous and find details */
 
 descr = e_stack - 3;
 k_get_string(descr);
 descr->data.str.saddr = s_make_contiguous(descr->data.str.saddr, NULL);
 old_hdr = descr->data.str.saddr;

 /* Change string */

 tgt_hdr = change (src_hdr, old_hdr, new_hdr, -1, 1);

 /* Remove parameters */

 k_dismiss();       /* Old substring */
 k_dismiss();       /* New substring */
 k_dismiss();       /* Source string */

 if (tgt_hdr)
  {
   /* Now save the result in the original data item */

   k_release(tgt_descr);
   InitDescr(tgt_descr, STRING);
   tgt_descr->data.str.saddr = tgt_hdr;
  }
}

/* ===================================================================
   change()  -  Common path for op_change() and op_schange()           */

Private STRING_CHUNK * change (STRING_CHUNK * src_hdr,
  STRING_CHUNK * old_hdr, STRING_CHUNK * new_hdr, long int change_count,
  long int skip_count)
{
 /* Stack:

     |=============================|=============================|
     |            BEFORE           |           AFTER             |
     |=============================|=============================|
 top |  New substring              |                             |
     |-----------------------------|-----------------------------|
     |  Old substring              |                             |
     |-----------------------------|-----------------------------|
     |  Source string              |                             |
     |=============================|=============================|
 */

 STRING_CHUNK * str;

 char * old_str;                   /* old substring */
 short int old_len;                /* Length of old substring */

 STRING_CHUNK * result_hdr = NULL; /* Modified string chunk pointer */

 short int n;
 char * p;

 /* Outer loop control */
 short int src_bytes_remaining;
 char * src;

 /* Inner loop control */
 STRING_CHUNK * isrc_hdr;
 short int isrc_bytes_remaining;
 char * isrc;
 char * substr;
 short int i_len;

 bool nocase;


 nocase = (process.program.flags & HDR_NOCASE) != 0;

 if (skip_count < 1) skip_count = 1;
 skip_count--;

 /* Find details of old substring */
 
 if (old_hdr == NULL) /* Null old string - return source unchanged */
  {
   return NULL;
  }
 old_str = old_hdr->data;
 old_len = (short int)(old_hdr->string_len);

 if (src_hdr == NULL) /* Null source string - return source unchanged */
  {
   return NULL;
  }

 ts_init(&result_hdr, src_hdr->string_len);

 /* Outer loop - Scan source string for initial character of substring */

 while(src_hdr != NULL)
  {
   src = src_hdr->data;
   src_bytes_remaining = src_hdr->bytes;

   while(src_bytes_remaining > 0)
    {
     if (nocase) p = (char *)memichr(src, *old_str, src_bytes_remaining);
     else p = (char *)memchr(src, *old_str, src_bytes_remaining);

     if (p == NULL)
      {
       /* First character not present. Copy all of chunk and move on */
       ts_copy(src, src_bytes_remaining);
       break;
      }

     /* First character is present. Copy up to this character before
        looking any further.                                          */

     n = p - src;
     if (n) ts_copy(src, n);
     src_bytes_remaining -= n;
     src = p;                    /* Leave pointing at first character */

     if (old_len > 1)       /* Multi-byte search item */
      {
       /* Inner loop - match remaining characters of the old substring */

       isrc_hdr = src_hdr;
       isrc = src + 1;          /* Point to second character of source... */
       isrc_bytes_remaining = src_bytes_remaining - 1;
       substr = old_str + 1;    /* ...and of old string */
       i_len = old_len - 1;
       do {
           if (isrc_bytes_remaining-- == 0)
            {
             isrc_hdr = isrc_hdr->next;
             if (isrc_hdr == NULL) goto no_match;

             isrc = isrc_hdr->data;
             isrc_bytes_remaining = isrc_hdr->bytes - 1;
               /* -1 allows for decrement for this cycle */
            }

           if (nocase)
            {
             if (UpperCase(*(isrc++)) != UpperCase(*(substr++))) goto no_match;
            }
           else
            {
             if (*(isrc++) != *(substr++)) goto no_match;
            }
          } while(--i_len);

       /* Match found  -  do we want this one? */

       if (skip_count-- > 0) goto no_match;

       /* Insert replacement substring */

       for(str = new_hdr; str != NULL; str = str->next)
        {
         ts_copy(str->data, str->bytes);
        }

       /* Leave outer loop variables pointing after the substring */

       src_bytes_remaining = isrc_bytes_remaining;
       src = isrc;
       src_hdr = isrc_hdr;
       if (--change_count == 0) goto copy_remainder;
       continue;
      }
     else         /* Matching single character substring */
      {
       if (skip_count-- > 0) goto no_match;

       for(str = new_hdr; str != NULL; str = str->next)
        {
         ts_copy(str->data, str->bytes);
        }

       src++;                      /* Skip old substring */
       src_bytes_remaining--;
       if (--change_count == 0) goto copy_remainder;
       continue;
      }

no_match:
     /* Move on one byte, copying to new string */
      ts_copy_byte(*(src++));
      src_bytes_remaining--;
    }  /* while src_bytes_remaining */

   /* Advance to next chunk */
   
   src_hdr = src_hdr->next;
  }

copy_remainder:
 if (src_hdr != NULL)
  {
   if (src_bytes_remaining) ts_copy(src, src_bytes_remaining);

   while((src_hdr = src_hdr->next) != NULL)
    {
     ts_copy(src_hdr->data, src_hdr->bytes);
    }
  }

 ts_terminate();

 return result_hdr;
}

/* END-CODE */

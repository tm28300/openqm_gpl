/* QMCLILIB.C
 * QM Client C library
 * Copyright (c) 2007 Ladybridge Systems, All Rights Reserved
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
 * 24 Feb 20        Remove match_template function and create new match_template.c file used in qm and qmclilib.
 * END-HISTORY
 *
 * START-DESCRIPTION:
 *
 *
 *
 * Match function
 * ==============
 * match_template()
 *
 * END-DESCRIPTION
 *
 * START-CODE
 */

#define Public
#include <qm.h>

/* Matching data */
const char * component_start;
const char * component_end;

/* ======================================================================
   match_template()  -  Match string against template                     */

Public bool match_template(string, template, component, return_component)
   const char * string;
   const char * template;
   int16 component;        /* Current component number - 1 (incremented) */
   int16 return_component; /* Required component number */
{
 bool not;
 int16 n;
 int16 m;
 int16 z;
 const char * p;
 char delimiter;
 const char * start;


 while(*template != '\0')
  {
   component++;
   if (component == return_component) component_start = string;
   else if (component == return_component + 1) component_end = string;

   start = template;

   if (*template == '~')
    {
     not = TRUE;
     template++;
    }
   else not = FALSE;

   if (IsDigit(*template))
    {
     n = 0;
     do {
         n = (n * 10) + (*(template++) - '0');
        } while(IsDigit(*template));

     switch(UpperCase(*(template++)))
      {
       case '\0':              /* String longer than template */
          /* 0115 rewritten */
          n = --template - start;
          if (n == 0) return FALSE;
          if ((!memcmp(start, string, n)) == not) return FALSE;
          string += n;
          break;
 
       case 'A':               /* nA  Alphabetic match */
          if (n)
           {
            while(n--)
             {
              if (*string == '\0') return FALSE;
              if ((IsAlpha(*string) != 0) == not) return FALSE;
              string++;
             }
           }
          else                 /* 0A */
           {
            if (*template != '\0') /* Further template items exist */
             {
              /* Match as many as possible further chaarcters */

              for (z = 0, p = string; ; z++, p++)
               {
                if ((*p == '\0') || ((IsAlpha(*p) != 0) == not)) break;
               }

              /* z now holds number of contiguous alphabetic characters ahead
                 of current position. Starting one byte after the final
                 alphabetic character, backtrack until the remainder of the
                 string matches the remainder of the template.               */

              for (p = string + z; z-- >= 0; p--)
               {
                if (match_template(p, template, component, return_component))
                 {
                  goto match_found;
                 }
               }
              return FALSE;
             }
            else
             {
              while((*string != '\0') && ((IsAlpha(*string) != 0) != not))
               {
                string++;
               }
             }
           }
          break;

       case 'N':               /* nN  Numeric match */
          if (n)
           {
            while(n--)
             {
              if (*string == '\0') return FALSE;
              if ((IsDigit(*string) != 0) == not) return FALSE;
              string++;
             }
           }
          else                 /* 0N */
           {
            if (*template != '\0') /* Further template items exist */
             {
              /* Match as many as possible further chaarcters */
  
              for (z = 0, p = string; ; z++, p++)
               {
                if ((*p == '\0') || ((IsDigit(*p) != 0) == not)) break;
               }

              /* z now holds number of contiguous numeric characters ahead
                 of current position. Starting one byte after the final
                 numeric character, backtrack until the remainder of the
                 string matches the remainder of the template.               */

              for (p = string + z; z-- >= 0; p--)
               {
                if (match_template(p, template, component, return_component))
                 {
                  goto match_found;
                 }
               }
              return FALSE;
             }
            else
             {
              while((*string != '\0') && ((IsDigit(*string) != 0) != not))
               {
                string++;
               }
             }
           }
          break;

       case 'X':               /* nX  Unrestricted match */
          if (n)
           {
            while(n--)
             {
              if (*(string++) == '\0') return FALSE;
             }
           }
          else                 /* 0X */
           {
            if (*template != '\0')    /* Further template items exist */
             {
              /* Match as few as possible further characters */
  
              do {
                  if (match_template(string, template, component, return_component))
                   {
                    goto match_found;
                   }
                 } while(*(string++) != '\0');
              return FALSE;
             }
            goto match_found;
           }
          break;

       case '-':               /* Count range */
          if (!IsDigit(*template)) return FALSE;
          m = 0;
          do {
              m = (m * 10) + (*(template++) - '0');
             } while(IsDigit(*template));
          m -= n;
          if (m < 0) return FALSE;

          switch(UpperCase(*(template++)))
           {
            case '\0':              /* String longer than template */
               n = --template - start;
               if (n)          /* We have found a trailing unquoted literal */
                {
                 if ((memcmp(start, string, n) == 0) != not) return TRUE;
                }
               return FALSE;

            case 'A':               /* n-mA  Alphabetic match */
               /* Match n alphabetic items */
 
              while(n--)
                {
                 if (*string == '\0') return FALSE;
                 if ((IsAlpha(*string) != 0) == not) return FALSE;
                 string++;
                }

               /* We may match up to m further alphabetic characters but must
                  also match as many as possible.  Check how many alphabetic
                  characters there are (up to m) and then backtrack trying
                  matches against the remaining template (if any).           */

               for(z = 0, p = string; z < m; z++, p++)
                {
                 if ((*p == '\0') || ((IsAlpha(*p) != 0) == not)) break;
                }

               /* z now holds max number of matchable characters.
                  Try match at each of these positions and also at the next
                  position (Even if it is the end of the string)            */

               if (*template != '\0')    /* Further template items exist */
                {
                 for (p = string + z; z-- >= 0; p--)
                  {
                   if (match_template(p, template, component, return_component))
                    {
                     goto match_found;
                    }
                  }
                 return FALSE;
                }
               else string += z;
               break;

            case 'N':               /* n-mN  Numeric match */
               /* Match n numeric items */
 
              while(n--)
                {
                 if (*string == '\0') return FALSE;
                 if ((IsDigit(*string) != 0) == not) return FALSE;
                 string++;
                }

               /* We may match up to m further numeric characters but must
                  also match as many as possible.  Check how many numeric
                  characters there are (up to m) and then backtrack trying
                  matches against the remaining template (if any).           */

               for(z = 0, p = string; z < m; z++, p++)
                {
                 if ((*p == '\0') || ((IsDigit(*p) != 0) == not)) break;
                }
             
               /* z now holds max number of matchable characters.
                  Try match at each of these positions and also at the next
                  position (Even if it is the end of the string)            */

               if (*template != '\0')    /* Further template items exist */
                {
                 for (p = string + z; z-- >= 0; p--)
                  {
                   if (match_template(p, template, component, return_component))
                    {
                     goto match_found;
                    }
                  }
                 return FALSE;
                }
               else string += z;
               break;

            case 'X':               /* n-mX  Unrestricted match */
               /* Match n items of any type */

               while(n--)
                {
                 if (*(string++) == '\0') return FALSE;
                }

               /* Match as few as possible up to m further characters */

               if (*template != '\0')
                {
                 while(m--)
                  {
                   if (match_template(string, template, component, return_component))
                    {
                     goto match_found;
                    }
                   string++;
                  }
                 return FALSE;
                }
               else
                {
                 if ((int32)strlen(string) > m) return FALSE;
                 goto match_found;
                }

            default:
               /* We have found an unquoted literal */
               n = --template - start;
               if ((memcmp(start, string, n) == 0) == not) return FALSE;
               string += n;
               break;
           }
          break;

       default:
          /* We have found an unquoted literal */
          n = --template - start;
          if ((memcmp(start, string, n) == 0) == not) return FALSE;
          string += n;
          break;
      }
    }
   else if (memcmp(template, "...", 3) == 0)   /* ... same as 0X */
    {
     template += 3;
     if (not) return FALSE;
     if (*template != '\0')    /* Further template items exist */
      {
       /* Match as few as possible further characters */

       while(*string != '\0')
        {
         if (match_template(string, template, component, return_component))
          {
           goto match_found;
          }
         string++;
        }
       return FALSE;
      }
     goto match_found;
    }
   else /* Must be literal text item */
    {
     delimiter = *template;
     if ((delimiter == '\'') || (delimiter == '"')) /* Quoted literal */
      {
       template++;
       p = strchr(template, (char)delimiter);
       if (p == NULL) return FALSE;
       n = p - template;
       if (n)
        {
         if ((memcmp(template, string, n) == 0) == not) return FALSE;
         string += n;
        }
       template += n + 1;
      }
     else                /* Unquoted literal. Treat as single character */
      {
       if ((*(template++) == *(string++)) == not) return FALSE;
      }
    }
  }

 if (*string != '\0') return FALSE;  /* String longer than template */
 
match_found:
 return TRUE;
}

/* END-CODE */

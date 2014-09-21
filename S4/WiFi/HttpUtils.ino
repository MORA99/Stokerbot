char specials[] = "$&+/\";=?@ <>#%{}|~[]`";


static  char hex_digit( char c)
{  return "0123456789ABCDEF"[c & 0x0F];
}

 char *urlencode( char *dst, char *src)
{   char c,*d = dst;
   while (c = *src++)
   {  if (strchr(specials,c))
      {  *d++ = '%';
         *d++ = hex_digit(c >> 4);
         *d++ = hex_digit(c);
      }
      else *d++ = c;
   }
   *d = '\0';
   return dst;
}



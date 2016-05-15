#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#include "common.h"
#include "SDL_filesystem.h"

LPCSTR
UTIL_BasePath(
   VOID
)
{
   static char buf[4096] = "";

   if (buf[0] == '\0')
   {
#ifdef CYDIA
      char *p = SDL_GetBasePath();
      if (p != NULL)
      {
         strcpy(buf, p);
         free(p);
      }
#else
      NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
      NSString *documentsDirectory = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
      strcpy(buf, [documentsDirectory UTF8String]);
#endif
   }

   return buf;
}

LPCSTR
UTIL_SavePath(
   VOID
)
{
   static char buf[4096] = "";

   if (buf[0] == '\0')
   {
      NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
      NSString *documentsDirectory = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
      strcpy(buf, [documentsDirectory UTF8String]);
   }

   return buf;
}

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
   if (!pdwScreenWidth || !pdwScreenHeight) return FALSE;
    
   CGRect rect = [UIScreen mainScreen].nativeBounds;
   float scale = 1.0f;
#if SDL_VERSION_ATLEAST(2,0,4)
   scale = [UIScreen mainScreen].nativeScale;
#endif
   *pdwScreenWidth = rect.size.height/scale;
   *pdwScreenHeight = rect.size.width/scale;
    
   return TRUE;
}

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"

#include <emscripten.h>
#include <fcntl.h>

extern "C" char *_stringtolower(char *s) 
{
	char *orig = s;
	do{	*s=tolower(*s); }while(*++s);
	return orig;
}

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
    return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}

BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return FALSE;
}

extern "C" int UTIL_Platform_Startup(int argc, char *argv[]) {
	// Defaults log to debug output
	UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
		EM_ASM(Module.print(UTF8ToString($0)), str);
	}, LOGLEVEL_MIN);

	return 0;
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
	gConfig.fLaunchSetting = FALSE;

	return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
}

extern "C" FILE *
EMSCRIPTEN_fopen(
    char *fname,
	char *opts
)
{
	return fopen(_stringtolower(fname),opts);
}
extern "C" int
EMSCRIPTEN_fclose(
    FILE *stream
)
{
	int mode = fcntl(fileno(stream), F_GETFL);
	int ret = fclose(stream);
	/*
	if ((mode & O_ACCMODE) != O_RDONLY) {
		EM_ASM({FS.syncfs(false, function (err) {});});
	}*/
	return ret;
}

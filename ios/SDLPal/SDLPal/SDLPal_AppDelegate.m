/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#import "SDLPal_AppDelegate.h"
#import "../src/video/uikit/SDL_uikitwindow.h"

#include "../src/events/SDL_events_c.h"

#ifdef main
#undef main
#endif

#include "palcfg.h"

static int forward_argc;
static char **forward_argv;
static int exit_status;

int sdlpal_main(int argc, char **argv)
{
    int i;

    /* store arguments */
    forward_argc = argc;
    forward_argv = (char **)malloc((argc+1) * sizeof(char *));
    for (i = 0; i < argc; i++) {
        forward_argv[i] = malloc( (strlen(argv[i])+1) * sizeof(char));
        strcpy(forward_argv[i], argv[i]);
    }
    forward_argv[i] = NULL;

    /* Give over control to run loop, SDLPalAppDelegate will handle most things from here */
    @autoreleasepool {
        UIApplicationMain(argc, argv, nil, [SDLPalAppDelegate getAppDelegateClassName]);
    }

    /* free the memory we used to hold copies of argc and argv */
    for (i = 0; i < forward_argc; i++) {
        free(forward_argv[i]);
    }
    free(forward_argv);

    return exit_status;
}

@interface SDLPalAppDelegate ()
/** in settings or in game? */
@property (nonatomic) BOOL isInGame;

@end

@implementation SDLPalAppDelegate
{
    UIWindow *launchWindow;
}

/* convenience method */
+ (id)sharedAppDelegate
{
    /* the delegate is set in UIApplicationMain(), which is guaranteed to be
     * called before this method */
    return [UIApplication sharedApplication].delegate;
}

+ (NSString *)getAppDelegateClassName
{
    /* subclassing notice: when you subclass this appdelegate, make sure to add
     * a category to override this method and return the actual name of the
     * delegate */
    return @"SDLPalAppDelegate";
}

- (void)hideLaunchScreen
{
    UIWindow *window = launchWindow;

    if (!window || window.hidden) {
        return;
    }

    launchWindow = nil;

    // Do a nice animated fade-out (roughly matches the real launch behavior.)
    [UIView animateWithDuration:0.2
        animations:^{
          window.alpha = 0.0;
        }
        completion:^(BOOL finished) {
          window.hidden = YES;
          UIKit_ForceUpdateHomeIndicator(); // Wait for launch screen to hide so settings are applied to the actual view controller.
        }];
}

- (void)postFinishLaunch
{
    /* Hide the launch screen the next time the run loop is run. SDL apps will
     * have a chance to load resources while the launch screen is still up. */
    [self performSelector:@selector(hideLaunchScreen) withObject:nil afterDelay:0.0];

    // run the user's application, passing argc and argv
    SDL_SetiOSEventPump(true);
    exit_status = sdlpal_main(forward_argc, forward_argv);
    SDL_SetiOSEventPump(false);

    /* exit, passing the return status from the user's application */
    /* We don't actually exit to support applications that do setup in their
     * main function and then allow the Cocoa event loop to run. */
    /* exit(exit_status); */
    [self restart];
}

- (void)launchGame {
    self.isInGame = YES;
    
    SDL_SetMainReady();
    [self performSelector:@selector(postFinishLaunch) withObject:nil afterDelay:0.0];
    [self.window setBackgroundColor:[UIColor blackColor]];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // iOS Files app will not show other app with no file inside documents. Create placeholder if needed.
    NSFileManager *fileMgr = [NSFileManager defaultManager];
    NSString *docPath = [NSString stringWithUTF8String:UTIL_BasePath()];
    NSString *placeholderPath =[NSString stringWithFormat:@"%@/placeholder", docPath];
    NSError *err = nil;
    NSArray *contents = [fileMgr contentsOfDirectoryAtPath:docPath error:&err];
    if( contents == nil || contents.count == 0 )
        [fileMgr createFileAtPath:placeholderPath contents:nil attributes:nil];
    else
        if( [fileMgr fileExistsAtPath:placeholderPath] )
            [fileMgr removeItemAtPath:placeholderPath error:&err];

    [self restart];
    return YES;
}
- (void)restart {
    PAL_LoadConfig(YES);
    const char *cachePath = UTIL_CachePath();
    if( getppid() != 1)
        NSLog(@"cache path:%s",cachePath);
    BOOL crashed = [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithFormat:@"%s/running", cachePath]];
    if( gConfig.fLaunchSetting || crashed ) {
        self.isInGame = NO;
        
        self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
        UIStoryboard *sb = [UIStoryboard storyboardWithName:@"Settings" bundle:nil];
        UIViewController *vc = [sb instantiateInitialViewController];
        self.window.rootViewController = vc;
        [self.window makeKeyAndVisible];
    }else{
        [self launchGame];
    }
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    SDL_SendAppEvent(SDL_EVENT_TERMINATING);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    SDL_SendAppEvent(SDL_EVENT_LOW_MEMORY);
}

#if !TARGET_OS_TV
- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window {
    // if in game.only support landscape
    return self.isInGame ? UIInterfaceOrientationMaskLandscape : UIInterfaceOrientationMaskAll;
}

- (void)application:(UIApplication *)application didChangeStatusBarOrientation:(UIInterfaceOrientation)oldStatusBarOrientation
{
    BOOL isLandscape = UIInterfaceOrientationIsLandscape(application.statusBarOrientation);
    SDL_VideoDevice *_this = SDL_GetVideoDevice();

    if (_this && _this->num_displays > 0) {
        SDL_DisplayMode *desktopmode = &_this->displays[0]->desktop_mode;
        SDL_DisplayMode *currentmode = _this->displays[0]->current_mode;

        /* The desktop display mode should be kept in sync with the screen
         * orientation so that updating a window's fullscreen state to
         * SDL_WINDOW_FULLSCREEN_DESKTOP keeps the window dimensions in the
         * correct orientation. */
        if (isLandscape != (desktopmode->w > desktopmode->h)) {
            int height = desktopmode->w;
            desktopmode->w = desktopmode->h;
            desktopmode->h = height;
        }

        /* Same deal with the current mode + SDL_GetCurrentDisplayMode. */
        if (isLandscape != (currentmode->w > currentmode->h)) {
            int height = currentmode->w;
            currentmode->w = currentmode->h;
            currentmode->h = height;
        }
    }
}
#endif

- (void)applicationWillResignActive:(UIApplication*)application
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (_this) {
        SDL_Window *window;
        for (window = _this->windows; window != nil; window = window->next) {
            SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_FOCUS_LOST, 0, 0);
            SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_MINIMIZED, 0, 0);
        }
    }
    SDL_SendAppEvent(SDL_APP_WILLENTERBACKGROUND);
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    SDL_SendAppEvent(SDL_APP_DIDENTERBACKGROUND);
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    SDL_SendAppEvent(SDL_EVENT_WILL_ENTER_FOREGROUND);
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    SDL_SendAppEvent(SDL_EVENT_DID_ENTER_FOREGROUND);

    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (_this) {
        SDL_Window *window;
        for (window = _this->windows; window != nil; window = window->next) {
            SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_FOCUS_GAINED, 0, 0);
            SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_RESTORED, 0, 0);
        }
    }
}

- (UIWindow *)window
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();
    if (_this) {
        SDL_Window *window = NULL;
        for (window = _this->windows; window != NULL; window = window->next) {
            SDL_UIKitWindowData *data = (__bridge SDL_UIKitWindowData *)window->internal;
            if (data != nil) {
                return data.uiwindow;
            }
        }
    }
    return nil;
}

- (void)setWindow:(UIWindow *)window
{
    // Do nothing.
}

- (void)sendDropFileForURL:(NSURL *)url fromSourceApplication:(NSString *)sourceApplication
{
    NSURL *fileURL = url.filePathURL;
    const char *sourceApplicationCString = sourceApplication ? [sourceApplication UTF8String] : NULL;
    if (fileURL != nil) {
        SDL_SendDropFile(NULL, sourceApplicationCString, fileURL.path.UTF8String);
    } else {
        SDL_SendDropFile(NULL, sourceApplicationCString, url.absoluteString.UTF8String);
    }
    SDL_SendDropComplete(NULL);
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    [self sendDropFileForURL:url fromSourceApplication:NULL];
    return YES;
}

@end

/* vi: set ts=4 sw=4 expandtab: */

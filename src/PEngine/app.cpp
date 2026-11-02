#include "../../engine/rhi/core_device.h"
#include <filesystem>
#include "../../engine/core/frame_pacer.h"
#include <memory>
#include <sstream>
#if __has_include("trackrs_version.h")
#include "trackrs_version.h"
#else
#define TRACKRS_VERSION PACKAGE_VERSION
#endif
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "app.h"
#include "audio.h"
#include "exception.h"
#include "pengine.h"
#include "physfs_utils.h"
#include "render.h"
#include "subsys.h"
#include "validation.h"
#include <SDL2/SDL_image.h>
#include <fstream>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if 0
#ifndef DATADIR
//#error DATADIR not defined! Use ./configure --datadir=...
//#define DATADIR "../data"
#endif
#endif

#if 0
#ifndef LOCALSTATEDIR
#warning LOCALSTATEDIR not defined! Will attempt to determine at run time.
#warning Use ./configure --localstatedir=...
#endif
#endif


/*
FIXME: not bothering to close joysticks because I
suspect SDL does it for you on quit. Am I right?
*/


int PUtil::deblev = DEBUGLEVEL_ENDUSER;

PApp::PApp(const std::string &title, const std::string &name):
        best_times("/players"),
        appname(name), // for ~/.name
        apptitle(title) // for window title
{
    //PUtil::outLog() << "Initialising SDL" << std::endl;
    const int si = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

    if (si < 0)
    {
        PUtil::outLog() << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    }

    cx = cy = 0;
    bpp = 0;
    fullscr = false;
    noframe = false;
    exit_requested = false;
    screenshot_requested = false;
    reqRGB = reqAlpha = reqDepth = reqStencil = false;
    stereo = StereoNone;
    stereoEyeTranslation = 0.0f;
    grabinput = false;
}

void PApp::setScreenModeAutoWindow()
{
#ifdef WIN32
  cy = GetSystemMetrics(SM_CYSCREEN) * 6 / 7;
  cx = cy * 4 / 3;
#else
  //#error COMPLETE FOR PLATFORM
  cy = 1024 * 6 / 7;
  cx = cy * 4 / 3;
#endif

  fullscr = false;
}

void PApp::setScreenModeFastFullScreen()
{
#ifdef WIN32
  cx = GetSystemMetrics(SM_CXSCREEN);
  cy = GetSystemMetrics(SM_CYSCREEN);
#else
  //#error COMPLETE FOR PLATFORM
  cx = 1280;
  cy = 1024;
#endif

  fullscr = false;

  noframe = true;
}

/* This routine performs the perspective projection for one eye's subfield.
   The projection is in the direction of the negative z axis.

   xmin, ymax, ymin, ymax = the coordinate range, in the plane of zero
   parallax setting, that will be displayed on the screen. The ratio between
   (xmax-xmin) and (ymax-ymin) should equal the aspect ration of the display.

   znear, zfar = the z-coordinate values of the clipping planes.

   zzps = the z-coordinate of the plane of zero parallax setting.

   dist = the distance from the center of projection to the plane of zero
   parallax.

   eye = half the eye separation; positive for the right eye subfield,
   negative for the left eye subfield.
*/
void PApp::stereoGLProject(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float dist, float eye)
{
  float xmid, ymid, clip_near, clip_far, top, bottom, left, right, dx, dy, n_over_d;

  dx = xmax - xmin;
  dy = ymax - ymin;

  xmid = (xmax + xmin) / 2.0;
  ymid = (ymax + ymin) / 2.0;

  clip_near = dist + zzps - znear;
  clip_far = dist + zzps - zfar;

  n_over_d = clip_near / dist;

  top = n_over_d * dy / 2.0;
  bottom = -top;
  right = n_over_d * (dx / 2.0 - eye);
  left = n_over_d * (-dx / 2.0 - eye);

  trackrs::draw::matrix(trackrs::draw::Projection);
  trackrs::draw::identity();
  trackrs::draw::frustum(left, right, bottom, top, clip_near, clip_far);
  trackrs::draw::translate(-xmid - eye, -ymid, -zzps - dist);
}

// I'm afraid I didn't understand how stereoGLProject worked, so I rewrote it

void PApp::stereoFrustum(float xmin, float xmax, float ymin, float ymax, float znear, float zfar, float zzps, float eye)
{
  // xmove = eye * (zzps - znear) / zzps - eye, simplifies to
  
  float xmove = -eye * znear / zzps;
  
  trackrs::draw::frustum(xmin + xmove, xmax + xmove, ymin, ymax, znear, zfar);
}

///
/// @brief Set size of game screen
/// @param [in] w           Pixel width of screen
/// @param [in] h           Pixel height of screen
/// @param [in] fullScreen  Fullscreen mode
/// @param [in] hideFrame   Display frame of OS
///
void PApp::setScreenMode(int w, int h, bool fullScreen, bool hideFrame)
{
    // use automatic video mode
    if (autoVideo)
    {
        SDL_DisplayMode dm;

        if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
        {
            cx = dm.w;
            cy = dm.h;
            fullscr = fullScreen;
            noframe = hideFrame;
            PUtil::outLog() << "Automatic video mode resolution: " << cx << 'x' << cy << std::endl;
        }
        else
        {
            PUtil::outLog() << "SDL error, SDL_GetCurrentDisplayMode(): " << SDL_GetError() << std::endl;
            autoVideo = false;
        }
    }

    // not written as an `else` branch because `autoVideo` may have
    // been updated in the case that automatic video mode failed
    if (!autoVideo)
    {
        cx = w;
        cy = h;
        fullscr = fullScreen;
        noframe = hideFrame;
    }
}

int PApp::run(int argc, char *argv[])
{
  PUtil::outLog() << apptitle << " init" << std::endl;
  
  PUtil::outLog() << "Build: " << TRACKRS_VERSION << " on " << __DATE__ << " at " << __TIME__ << std::endl;
  
  if (exit_requested) {
    PUtil::outLog() << "Exit requested" << std::endl;
    return 0;
  }
  
  PUtil::outLog() << "Initialising PhysFS" << std::endl;
  
  if (PHYSFS_init((argc >= 1) ? argv[0] : nullptr) == 0) {
    PUtil::outLog() << "PhysFS failed to initialise" << std::endl;
    PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    return 1;
  }
  
  PHYSFS_permitSymbolicLinks(1);
  
  {
    std::string lsdbuff;
    
    lsdbuff = physfs_getDir();
    
    PUtil::outLog() << "Setting writable user directory to \"" << lsdbuff << "\"" << std::endl;
    
    if (PHYSFS_setWriteDir(lsdbuff.c_str()) == 0) {
      PUtil::outLog() << "Failed to set PhysFS writable directory to \"" << lsdbuff << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    
    if (PHYSFS_mkdir("/players") == 0)
    {
        PUtil::outLog() << "Failed to create directory \"/players\"" << std::endl
            << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    
    std::string basedir = PHYSFS_getBaseDir();
    PUtil::outLog() << "Application base directory \"" << basedir << '\"' << std::endl;
    if (PHYSFS_mount(basedir.c_str(), NULL, 1) == 0) {
      PUtil::outLog() << "Failed to add PhysFS search directory \"" << basedir << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    
    if (PHYSFS_mount(lsdbuff.c_str(), NULL, 1) == 0) {
      PUtil::outLog() << "Failed to add PhysFS search directory \"" << lsdbuff << "\"" << std::endl
          << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    // we run MainApp::config() here in order to add data dirs to PhysFS search path
    try
    {
        config();
    }
    catch (PException &e)
    {
        PUtil::outLog() << "Config failed: " << e.what() << std::endl;
        
        if (PHYSFS_deinit() == 0)
        {
            PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
        }

        return 1;
    }

    // Find any .zip files and add them to search path
    std::list<std::string> zipfiles = PUtil::findFiles("", ".zip");
    
    for (std::list<std::string>::iterator i = zipfiles.begin();
      i != zipfiles.end(); ++i) {
      
      const char *realpath = PHYSFS_getRealDir(i->c_str());
      
      if (realpath) {
        std::string fullpath = (std::string)realpath + *i;
        
        if (PHYSFS_mount(fullpath.c_str(), NULL, 1) == 0) {
          PUtil::outLog() << "Failed to add archive \"" << fullpath << "\"" << std::endl
              << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
      } else {
        PUtil::outLog() << "Failed to find path of archive \"" << *i << "\"" << std::endl
            << "PhysFS: " << physfs_getErrorString() << std::endl;
      }
    }
  }

  
  PUtil::outLog() << "Create window and set video mode" << std::endl;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  
  if (reqRGB) {
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
  }
  
  if (reqAlpha) {
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
  }
  
  if (reqDepth) {
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
  }
  
  if (reqStencil)
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
  
  if (stereo == StereoQuadBuffer) {
    SDL_GL_SetAttribute( SDL_GL_STEREO, 1 );
  }
  
  if (cx <= 0 || cy <= 0) setScreenModeAutoWindow();

    if (!autoVideo)
    {
        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            cx, cy,
            SDL_WINDOW_OPENGL |
            (fullscr ? SDL_WINDOW_FULLSCREEN : 0)
            /* | (noframe ? SDL_NOFRAME : 0) */ ); // broken by SDL2, no more `SDL_NOFRAME`
    }
    else // automatic video mode
    {
        //
        // NOTE:
        //  The `SDL_GetDesktopMode()` function is used instead of the automatic
        //  flag `SDL_WINDOW_FULLSCREEN_DESKTOP` because the latter fails to work
        //  reliably on Linux in certain versions of SDL2.
        //

#define SDL_WINDOW_FULLSCREEN_DESKTOP_IS_STILL_BROKEN

#ifdef SDL_WINDOW_FULLSCREEN_DESKTOP_IS_STILL_BROKEN
        SDL_DisplayMode dm;

        if (SDL_GetDesktopDisplayMode(0, &dm) == 0)
        {
            cx = dm.w;
            cy = dm.h;
            PUtil::outLog() << "Desktop video mode resolution: " << cx << 'x' << cy << std::endl;
        }
        else
        {
            PUtil::outLog() << "SDL error, SDL_GetDesktopDisplayMode(): " << SDL_GetError() << std::endl;
            cx = 800;
            cy = 600;
        }

        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            cx, cy,
            SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
#else
        //
        // NOTE:
        //  This would be the "proper" way of doing things, but at the time of this
        //  writing it only works reliably on Windows 7 using SDL2 version 2.0.5.
        //
        screen = SDL_CreateWindow(apptitle.c_str(),
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            0, 0,
            SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
    }
  
  if (!screen) {
    PUtil::outLog() << "Failed to create window or set video mode" << std::endl;
    PUtil::outLog() << "SDL error: " << SDL_GetError() << std::endl;
    PUtil::outLog() << "Try changing your video settings in trigger-rally.config" << std::endl;
    SDL_Quit();
    if (PHYSFS_deinit() == 0) {
      PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    }
    return 1;
  }

  PHYSFS_file *iconFile = PHYSFS_openRead("icon/logo.png");
  if (iconFile) {
    SDL_RWops *iconOps = PUtil::allocPhysFSops(iconFile);
    if (iconOps) {
      SDL_Surface *icon = IMG_Load_RW(iconOps, 1);
      if (icon) {
        SDL_SetWindowIcon(screen, icon);
        SDL_FreeSurface(icon);
      }
    }
  }
  
  context = SDL_GL_CreateContext(screen);
  
    if (context == NULL)
    {
        PUtil::outLog() << "Failed to create OpenGL context for game window\n";
        PUtil::outLog() << "SDL error: " << SDL_GetError() << std::endl;
        destroyGraphics();
        SDL_Quit();

        if (PHYSFS_deinit() == 0)
		{
            PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
		}
        return 1;
    }
  
  sdl_mousemap = 0;
  
  sdl_joy.resize(SDL_NumJoysticks());
  
  PUtil::outLog() << "Found " << sdl_joy.size() << " joystick" <<
    (sdl_joy.size() == 1 ? "" : "s") << std::endl;
  
  for (unsigned int i=0; i<sdl_joy.size(); i++) {
    PUtil::outLog() << "Joystick " << (i+1) << ": ";
    sdl_joy[i].sdl_joystick = SDL_JoystickOpen(i);
    if (sdl_joy[i].sdl_joystick == nullptr) {
      PUtil::outLog() << "failed to open joystick" << std::endl;
      destroyGraphics();
      SDL_Quit();
      if (PHYSFS_deinit() == 0) {
        PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
      }
      return 1;
    }
    sdl_joy[i].name = SDL_JoystickName(sdl_joy[i].sdl_joystick);
    sdl_joy[i].axis.resize(SDL_JoystickNumAxes(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].axis.size(); j++)
      sdl_joy[i].axis[j] = ((float)SDL_JoystickGetAxis(sdl_joy[i].sdl_joystick, j) + 0.5f) / 32767.5f;
    sdl_joy[i].button.resize(SDL_JoystickNumButtons(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].button.size(); j++)
      sdl_joy[i].button[j] = (SDL_JoystickGetButton(sdl_joy[i].sdl_joystick, j) != 0);
    sdl_joy[i].hat.resize(SDL_JoystickNumHats(sdl_joy[i].sdl_joystick));
    for (unsigned int j=0; j<sdl_joy[i].hat.size(); j++) {
      Uint8 state = SDL_JoystickGetHat(sdl_joy[i].sdl_joystick, j);
      sdl_joy[i].hat[j] = vec2i::zero();
      if (state & SDL_HAT_RIGHT) sdl_joy[i].hat[j].x = 1;
      else if (state & SDL_HAT_LEFT) sdl_joy[i].hat[j].x = -1;
      if (state & SDL_HAT_UP) sdl_joy[i].hat[j].y = 1;
      else if (state & SDL_HAT_DOWN) sdl_joy[i].hat[j].y = -1;
    }
    
    PUtil::outLog() << sdl_joy[i].name << ", " <<
      sdl_joy[i].axis.size() << " axis, " <<
      sdl_joy[i].button.size() << " button, " <<
      sdl_joy[i].hat.size() << " hat" << std::endl;

    sdl_joy[i].sdl_haptic = SDL_HapticOpenFromJoystick(sdl_joy[i].sdl_joystick);
    if (sdl_joy[i].sdl_haptic) {
      if (SDL_HapticRumbleInit(sdl_joy[i].sdl_haptic) != 0) {
        SDL_HapticClose(sdl_joy[i].sdl_haptic);
        sdl_joy[i].sdl_haptic = nullptr;
      }
    }
  }
  
  SDL_JoystickEventState(SDL_ENABLE);
  
  if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
    PUtil::outLog() << "glad failed to load OpenGL\n";
    destroyGraphics(); SDL_Quit(); return 1;
  }
  try {
    const char* directory = PHYSFS_getRealDir("/shaders/basic/basic.vert");
    if (!directory) throw std::runtime_error("Missing basic core shaders");
    rhiDevice = std::make_unique<trackrs::rhi::CoreDevice>(screen,context,cx,cy,
        std::filesystem::u8path(directory)/"shaders/basic",!validation::run.options.enabled);
  } catch (const std::exception& error) {
    PUtil::outLog() << "Core RHI startup failed: " << error.what() << '\n';
    destroyGraphics(); SDL_Quit(); PHYSFS_deinit(); return 1;
  }
  PUtil::outLog() << "Graphics: " <<
    glGetString(GL_VENDOR) << " " << glGetString(GL_RENDERER) << std::endl;
  
  PUtil::outLog() << "Using OpenGL " << glGetString(GL_VERSION) << std::endl;

  switch (stereo) {
  default: break;
  case StereoQuadBuffer:
    PUtil::outLog() << "Using hardware quad buffer stereo, separation ="
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoRedBlue:
    PUtil::outLog() << "Using red-blue anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoRedGreen:
    PUtil::outLog() << "Using red-green anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoRedCyan:
    PUtil::outLog() << "Using red-cyan anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  case StereoYellowBlue:
    PUtil::outLog() << "Using yellow-blue anaglyph stereo, separation = "
      << stereoEyeTranslation * 2.0f << std::endl;
    break;
  }

  std::vector<std::unique_ptr<PSubsystem>> sslist;

  try
  {
    sslist.emplace_back(ssrdr = new PSSRender(*this));
    sslist.emplace_back(sstex = new PSSTexture(*this));
    sslist.emplace_back(ssfx = new PSSEffect(*this));
    sslist.emplace_back(ssmod = new PSSModel(*this));
    sslist.emplace_back(ssaud = new PSSAudio(*this));
  }
  catch (PException &e)
  {
    PUtil::outLog () << "Subsystem failed to init: " << e.what() << std::endl;
    
    while (!sslist.empty())
    {
      sslist.pop_back();
    }

    destroyGraphics();
    SDL_Quit();

    if (PHYSFS_deinit() == 0)
    {
      PUtil::outLog () << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    return 1;
  }

  PUtil::outLog() << "Performing app load" << std::endl;
  
  try
  {
    load();
  }
  catch (PException &e)
  {
    PUtil::outLog() << "App load failed: " << e.what () << std::endl;

    while (!sslist.empty()) {
      sslist.pop_back();
    }

    destroyGraphics();
    SDL_Quit();

    if (PHYSFS_deinit() == 0) {
      PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
    }

    return 1;
  }

  //SDL_ShowCursor(SDL_DISABLE);
  //SDL_WM_GrabInput(SDL_GRAB_ON);
  //SDL_WM_GrabInput(SDL_GRAB_OFF);
  //SDL_SetWindowGrab(screen, SDL_TRUE);
  //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
  //SDL_EnableUNICODE(1);

  sdl_keymap = SDL_GetKeyboardState(&sdl_numkeys);

  resize();
  playIntro();

  PUtil::outLog() << "Initialisation complete, entering main loop" << std::endl;


  bool active = true, repaint = true, axis_down = false;
  const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());
  Uint64 curtime = SDL_GetPerformanceCounter();
  trackrs::FramePacer pacer;
  const int requestedInterval = validation::run.options.enabled ? 0 : swapInterval;
  rhiDevice->setSwapInterval(requestedInterval);

  while (1) {
    pacer.begin();
    const auto validationFrameStart = validation::run.options.enabled ? SDL_GetPerformanceCounter() : 0;
    const bool validationWasStarted = validation::run.started;
    validation::run.capture = validationWasStarted;
    validation::run.frameDraws = validation::run.frameTriangles = 0;
    double validationCpuMs = 0;
    GLuint validationQuery = 0;
    SDL_Event event;

    while ( SDL_PollEvent(&event) ) {
      if (validation::run.options.enabled && event.type != SDL_QUIT)
        continue;
      switch(event.type) {
      
      // Using ACTIVEEVENT only seems to cause trouble.
      
      /*
      case SDL_ACTIVEEVENT:
        active = event.active.gain;
        if (active) {
          SDL_ShowCursor(SDL_DISABLE);
          //SDL_WM_GrabInput(SDL_GRAB_ON);
          PUtil::outLog() << "Window made active" << std::endl;
        } else {
          SDL_ShowCursor(SDL_ENABLE);
          //SDL_WM_GrabInput(SDL_GRAB_OFF);
          PUtil::outLog() << "Window made inactive" << std::endl;
        }
        break;
      */

      // unavailable (and unneeded?) in SDL2
#if 0
      case SDL_VIDEOEXPOSE:
        repaint = true;
        break;
#endif

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        keyEvent(event.key);
        break;

      case SDL_MOUSEMOTION:
        if (grabinput)
          mouseMoveEvent(event.motion.xrel, -event.motion.yrel);
        else
          cursorMoveEvent(event.motion.x, event.motion.y);
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        mouseButtonEvent(event.button);
        break;

      case SDL_JOYAXISMOTION:
        sdl_joy[event.jaxis.which].axis[event.jaxis.axis] =
          ((float)event.jaxis.value + 0.5f) / 32767.5f;
        axis_down = joyAxisEvent(event.jaxis.which,event.jaxis.axis,sdl_joy[event.jaxis.which].axis[event.jaxis.axis],axis_down);
        break;

      case SDL_JOYBUTTONDOWN:
        sdl_joy[event.jbutton.which].button[event.jbutton.button] = true;
        joyButtonEvent(event.jbutton.which,event.jbutton.button,true);
        break;

      case SDL_JOYBUTTONUP:
        sdl_joy[event.jbutton.which].button[event.jbutton.button] = false;
        joyButtonEvent(event.jbutton.which,event.jbutton.button,false);
        break;

      case SDL_JOYHATMOTION:
        sdl_joy[event.jhat.which].hat[event.jhat.hat] = vec2i::zero();
        if (event.jhat.value & SDL_HAT_RIGHT) sdl_joy[event.jhat.which].hat[event.jhat.hat].x = 1;
        else if (event.jhat.value & SDL_HAT_LEFT) sdl_joy[event.jhat.which].hat[event.jhat.hat].x = -1;
        if (event.jhat.value & SDL_HAT_UP) sdl_joy[event.jhat.which].hat[event.jhat.hat].y = 1;
        else if (event.jhat.value & SDL_HAT_DOWN) sdl_joy[event.jhat.which].hat[event.jhat.hat].y = -1;
        break;

      case SDL_QUIT:
        requestExit();
        break;
      }
      if (exit_requested) break;
    }

    if (exit_requested) {
      break;
    }

    sdl_mousemap = SDL_GetMouseState(nullptr, nullptr);

#define TIMESCALE 1.0

    const Uint64 nowtime = SDL_GetPerformanceCounter();

    if (1) {//if (active) {
      const double timepassed = std::min(static_cast<double>(nowtime - curtime) / frequency, 0.1);
      if (timepassed > 0 || validation::run.options.enabled) {
        float delta = validation::run.options.enabled ? 0.004f : static_cast<float>(timepassed * TIMESCALE);

        const auto validationUpdateStart = validation::run.options.enabled ? SDL_GetPerformanceCounter() : 0;
        tick(delta);

        for (auto& subsystem : sslist) subsystem->tick(delta, cam_pos, cam_orimat, cam_linvel);
        if (validation::run.options.enabled)
          validationCpuMs = 1000.0 * static_cast<double>(SDL_GetPerformanceCounter() - validationUpdateStart) /
              static_cast<double>(SDL_GetPerformanceFrequency());
      }
    }

    curtime = nowtime;

    if (exit_requested) break;

    if (active || repaint) {
      switch (stereo) {
        
      case StereoNone: // Normal, non-stereo rendering
        if (validationWasStarted && (GLAD_GL_VERSION_3_3 || GLAD_GL_ARB_timer_query)) {
          glGenQueries(1, &validationQuery);
          glBeginQuery(GL_TIME_ELAPSED, validationQuery);
        }
        render(0.0f);
        if (validationQuery) glEndQuery(GL_TIME_ELAPSED);
        presentFrame();
        if (validationWasStarted) {
          // MainApp's core batch drain/fence has completed this frame. Collect all
          // pass timestamps here rather than stalling after each scope.
          for (const auto& query : validation::run.queries) {
            GLuint64 a = 0, b = 0;
            glGetQueryObjectui64v(query.begin, GL_QUERY_RESULT, &a);
            glGetQueryObjectui64v(query.end, GL_QUERY_RESULT, &b);
            query.pass->gpu.push_back(static_cast<double>(b - a) / 1000000.0);
            const GLuint pair[] = {query.begin, query.end};
            glDeleteQueries(2, pair);
          }
          validation::run.queries.clear();
          if (validationQuery) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(validationQuery, GL_QUERY_RESULT, &ns);
            validation::run.gpu.push_back(static_cast<double>(ns) / 1000000.0);
            glDeleteQueries(1, &validationQuery);
          }
          validation::run.frames.push_back(1000.0 * static_cast<double>(SDL_GetPerformanceCounter() - validationFrameStart) /
              static_cast<double>(SDL_GetPerformanceFrequency()));
          validation::run.cpu.push_back(validationCpuMs);
          validation::run.draws.push_back(validation::run.frameDraws);
          validation::run.triangles.push_back(validation::run.frameTriangles);
          if (validation::run.complete) {
            try { validation::writeResults(); }
            catch (const std::exception& e) { PUtil::outLog() << e.what() << std::endl; validation::run.result = 1; }
            requestExit();
          }
        }
        break;
        
      case StereoQuadBuffer: // Hardware quad buffer stereo
        
        trackrs::draw::drawBuffer(GL_BACK_LEFT);
        render(-stereoEyeTranslation);
        
        trackrs::draw::drawBuffer(GL_BACK_RIGHT);
        render(stereoEyeTranslation);
        
        presentFrame();
        break;
        
      case StereoRedBlue: // Red-blue anaglyph stereo
        
        // Green will not be rendered to, so clear it
        trackrs::draw::colorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
        glClearColor(0.5, 0.5, 0.5, 0.5);
        trackrs::draw::clear(GL_COLOR_BUFFER_BIT);
        
        trackrs::draw::colorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        
        trackrs::draw::colorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        
        presentFrame();
        break;
        
      case StereoRedGreen: // Red-green anaglyph stereo
        
        // Blue will not be rendered to, so clear it
        trackrs::draw::colorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        glClearColor(0.5, 0.5, 0.5, 0.5);
        trackrs::draw::clear(GL_COLOR_BUFFER_BIT);
        
        trackrs::draw::colorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        
        trackrs::draw::colorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
        render(stereoEyeTranslation);
        
        presentFrame();
        break;
        
      case StereoRedCyan: // Red-cyan anaglyph stereo
        
        trackrs::draw::colorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        
        trackrs::draw::colorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        
        presentFrame();
        break;
        
      case StereoYellowBlue: // Yellow-blue anaglyph stereo
        
        trackrs::draw::colorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
        render(-stereoEyeTranslation);
        
        trackrs::draw::colorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
        render(stereoEyeTranslation);
        
        presentFrame();
        break;
      }
      repaint = false;
      

      
    } else {
      SDL_WaitEvent(nullptr);
    }

    if (exit_requested) break;
    if (!validation::run.options.enabled) pacer.wait(frameCap);
  }

  PUtil::outLog() << "Exit requested" << std::endl;

  unload();
  
  while (!sslist.empty()) {
    sslist.pop_back();
  }
  
  //SDL_WM_GrabInput(SDL_GRAB_OFF);
  //SDL_SetWindowGrab(screen, SDL_FALSE);
  SDL_ShowCursor(SDL_ENABLE);
  
    destroyGraphics();
    SDL_Quit();
  
  best_times.savePlayer();
  
  if (PHYSFS_deinit() == 0) {
    PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
  }
  
  PUtil::outLog() << "Shutdown complete" << std::endl;
  
  return 0;
}

void PApp::grabMouse(bool grab)
{
  //SDL_WM_GrabInput(grab ? SDL_GRAB_ON : SDL_GRAB_OFF);
  SDL_ShowCursor(grab ? SDL_DISABLE : SDL_ENABLE);

  grabinput = grab;
}

void PApp::drawModel(PModel &model, float alpha)
{
  validation::PassScope timing("mesh_submit");
  for (std::vector<PMesh>::iterator mesh = model.mesh.begin();
    mesh != model.mesh.end();
    ++mesh) {
    if (!mesh->effect)
      mesh->effect = getSSEffect().loadEffect(mesh->fxname);

    for (int j=0; j<mesh->effect->getNumTechniques(); j++) {
      mesh->effect->setCurrentTechnique(j);

      int numPasses = 0;
      if (mesh->effect->renderBegin(&numPasses, getSSTexture())) {
        if (mesh->effect->getTechniqueName(j) == "EmissionTechMTL"
            && (getCtrlActionBackValue() < 0.5f || getVehicleCurrentGear() == -1 || alpha < 1.0f)) {
          continue;
        }
        if (alpha < 1.0f) {
          trackrs::draw::blend(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
          trackrs::draw::enable(GL_BLEND);
          trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.5f);
        }

        for (int i=0; i<numPasses; i++) {
          mesh->effect->renderPass(i);
          if (!mesh->gpu) {
            std::vector<trackrs::draw::MeshVertex> vertices;
            vertices.reserve(mesh->face.size()*3);
            for (const auto& face : mesh->face) for (int corner=0;corner<3;++corner) {
              const auto& p=mesh->vert[face.vt[corner]];
              const auto& n=mesh->norm[face.nr[corner]];
              const auto& uv=mesh->texco[face.tc[corner]];
              vertices.push_back({{p.x,p.y,p.z},{n.x,n.y,n.z},{uv.x,uv.y}});
            }
            mesh->gpu=std::make_shared<trackrs::draw::Mesh>(vertices);
          }
          mesh->gpu->draw();
        }
        mesh->effect->renderEnd();
      }
      if (alpha < 1.0f)
        trackrs::draw::blend(GL_ONE,GL_ZERO);
    }
  }
}


// default callback functions

void PApp::config()
{
}

void PApp::load()
{
}

void PApp::unload()
{
}

void PApp::tick(float delta)
{
  delta = delta;
}

void PApp::resize()
{
}

void PApp::render(float eyetranslation)
{
  eyetranslation = eyetranslation;
  
  glClearColor(0.5, 0.5, 0.5, 0.0);
  
  trackrs::draw::clear(GL_COLOR_BUFFER_BIT);
}

void PApp::keyEvent(const SDL_KeyboardEvent &ke)
{
  if (ke.type != SDL_KEYDOWN) return;

  switch (ke.keysym.sym) {
  case SDLK_ESCAPE:
    requestExit();
    break;
  default:
    break;
  }
}

void PApp::mouseButtonEvent(const SDL_MouseButtonEvent &mbe)
{
  int unused = mbe.type; unused = unused;
}

void PApp::mouseMoveEvent(int dx, int dy)
{
  dx = dx; dy = dy;
}

void PApp::cursorMoveEvent(int posx, int posy)
{
  posx = posx; posy = posy;
}

void PApp::joyButtonEvent(int which, int button, bool down)
{
  which = which; button = button; down = down;
}

bool PApp::joyAxisEvent(int which, int axis, float value, bool down)
{
  which = which; axis = axis; value = value; down = down;
  return false;
}

float PApp::getCtrlActionBackValue()
{
  return 0.0f;
}

int PApp::getVehicleCurrentGear() {
  return 0;
}

void PApp::captureScreenshot() {
        glReadBuffer(GL_BACK);
        std::vector<unsigned char> pixels(static_cast<std::size_t>(cx) * (cy + 1) * 3);
        auto* data1 = pixels.data();
        glReadPixels(0, 0, cx, cy, GL_RGB, GL_UNSIGNED_BYTE, data1);
        glReadBuffer(GL_BACK);
        const int rowsize = cx * 3;
        for(int i = 0; i < cy/2; ++i) {
          memcpy(&data1[(cy) * rowsize], &data1[(cy-1-i) * rowsize], rowsize);
          memcpy(&data1[(cy-1-i) * rowsize], &data1[(i) * rowsize], rowsize);
          memcpy(&data1[(i) * rowsize], &data1[(cy) * rowsize], rowsize);
          //memset(&data1[(i) * rowsize], 128, rowsize);
        }
        const std::string header = "P6\n# CREATOR: TrackRS\n" + std::to_string(cx) + " " + std::to_string(cy) + "\n255\n";
        const std::string filename = "screen-" + std::to_string(SDL_GetTicks()) + ".ppm";
        PUtil::outLog() << "Writing screenshot \"" << filename << "\"" << std::endl;
        PHYSFS_file* pfile = PHYSFS_openWrite(filename.c_str());
        if (pfile) {
          physfs_write(pfile, header.data(), sizeof(char), header.size());
          physfs_write(pfile, data1, sizeof(char), cx*cy*3);
          PHYSFS_close(pfile);
        } else {
          PUtil::outLog() << "Screenshot write failed" << std::endl;
          PUtil::outLog() << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
        screenshot_requested = false;
      }

PApp::~PApp() { destroyGraphics(); }
void PApp::destroyGraphics() {
    rhiDevice.reset();
    if(context)SDL_GL_DeleteContext(context);
    if(screen)SDL_DestroyWindow(screen);
    context=nullptr;screen=nullptr;
}
void PApp::presentFrame() {
    rhiDevice->present([this] {
        captureValidationScreenshot();
        if(screenshot_requested)captureScreenshot();
    });
}
void PApp::captureValidationScreenshot() {
        if (validation::run.complete && !validation::run.options.screenshot.empty()) {
          // Capture the visible core back buffer before swapping.
          std::vector<unsigned char> pixels(static_cast<std::size_t>(cx) * static_cast<std::size_t>(cy) * 3);
          glPixelStorei(GL_PACK_ALIGNMENT, 1);
          glReadBuffer(GL_BACK);
          glReadPixels(0, 0, cx, cy, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
          std::ofstream out(validation::run.options.screenshot, std::ios::binary);
          out << "P6\n" << cx << ' ' << cy << "\n255\n";
          for (int y = cy - 1; y >= 0; --y)
            out.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y * cx * 3)), cx * 3);
          out.close();
          if (!out) { validation::run.result = 1; requestExit(); }
        }

}

///////////////////////////
// TrackRS v0.6.x README //
///////////////////////////


Thanks for downloading TrackRS! I hope you enjoy the game.

Creator:
  Saad AIT YAHIA - @github: Saad-programmer

Before playing, building, or copying, please read the license later in this file.


Look at ~/.trackrs/trackrs-0.6.x.config
in your favourite text editor for configuration options.


///////////////////
// Configuration //
///////////////////


Defaults:

key     action
Up      Accelerate
Down    Foot brake / Reverse
Left    Steer left
Right   Steer right
Space   Handbrake
C       Change camera view
M       Toggle map
N       Toggle user interface
K       Toggle checkpoints
R       Recover car
Q       Recover car at previous checkpoint
P       Pause race
< >     Rotate camera when in 3rd person cam
F12     Save a screenshot to ~/.trackrs/


Joystick support can be enabled in the config file.


Stereo support is available, for both quad buffer hardware and anaglyph glasses. See README-stereo.txt


/////////////
// License //
/////////////


TrackRS is released under the GPL version 2 (see COPYING,
or http://www.gnu.org/licenses/gpl-2.0.html ).

Some of the data files have different authors and licenses.
Please see DATA_AUTHORS.txt for this information.


//////////////////////
// Content Creation //
//////////////////////

A. Car models
-------------

TrackRS ships car models in .obj formats.

You can add completely new cars in the Wavefront .obj format with the
following restrictions:
 
  - Use only one material per .obj.
  - All faces must be triangles.
  - The material is ignored, only the texture defined by it gets loaded.
  - All meshes, besides the wheels, need to be on a single object.
  - Wheels are in their own files.
 
When exporting from Blender, following settings work:

  - "Apply Modifiers"
  - "Include Normals"
  - "Include Edges"
  - "Write Materials"
  - "Triangulate Faces"
  - "Objects as OBJ Objects"
 
To test the model, replace the file name in an existing .vehicle file inside

data/vehicles/VEHICLENAME/
 

B. Levels/maps/tracks
---------------------
 
Use any tool that saves .png or .jpg images, for example GIMP and Inkscape.

You will need to create a heightmap, color map and need to assign coordinates.
You can create optional foliage, hud-map, road-map, terrain-map image files.

View existing .level files in data/maps or data/plugins/ to learn.

Best use .png for heightmaps, as .jpg can cause artefacts, which will change
the level geometry.

C. Car textures
---------------

We used Inkscape to create the .svg files for the car textures.

You might have to get the following freely licensed fonts from sites like http://www.google.com/fonts/ to be able to render the currently used .svg files to .png correctly:
  - TeX Gyre Heros
  - Roboto
  - Bowlby One
  - URW Gothic L


/////////////////////
// Troubleshooting //
/////////////////////


If something goes wrong, you should check your settings in
~/.trackrs/trackrs-0.6.x.config. You can delete that file to reset everything to default. (It will be recreated next time you run TrackRS.)


/////////////////////
// Version history //
/////////////////////

04/03/2019 - TrackRS 0.6.6.1
  - Released Windows binaries
  - Optimized data for release
  - Removed internal TinyXML-2 source from code tree

01/02/2019 - TrackRS 0.6.6
  - Documented and reorganized physic engine code
  - Different tuning: Fox offroad, Evo circuit, Cordo in between
  - Wheel sinking on the different types of terrain
  - New freely licensed font
  - Wheel-ground contact is now computed along wheel plane, not just straight down
  - Vehicle selection screen shows real values of the car
  - Vehicles can have different friction of wheels defined in the .vehicle file
  - Optimized .obj mesh loader
  - Updated libPhysfs code
  - Other fixes
  - Added 2 new events and 20 new single races (36 new maps)
  - Added many new vegetation sprites

18/12/2016 - TrackRS 0.6.5
  - Fixed framerate drop caused by codriver voice on certain post-XP Windows'
  - Fixed compilation error of "hiscore1.h" by C++11 compilers
  - Added 2 new events and 13 new single races (total 25 new races)
  - Added "road sign" option for map creators
  - Updated code to use TinyXML-2, SDL2 and SDL2_image
  - Improved code friendliness to Linux when searching for the default
    configuration file "trackrs.config.defs"
  - Released 64-bit binaries for Windows x64, optimized for AMD K8 

23/04/2016 - TrackRS 0.6.4
  - Added recording of best times
  - Added unlocking of vehicles and events
  - Optimized game data for smaller size and faster loading
  - Added "Pause" key (default `P`)
  - Added "Recover At Checkpoint" key (default `Q`)
  - Added multiple laps option for map creators
  - Added time penalty for offroad driving
  - Improved the "AB" codriver voice
  - Fixed sound bug in the Windows version
  - Changed default resolution to native fullscreen

30/01/2016 - TrackRS 0.6.3
  - Improve menu and in-race OSD
  - Add codriver for 75% of maps
  - Tweak terrain physics
  - Add many new maps
  - Change vehicle skins and presets
  - Made various minor code improvements

05/05/2015 - TrackRS 0.6.2
  - Fix several Windows bugs and compilation issues
  - Fix compilation warnings
  - Windows binaries

25/10/2014 - TrackRS 0.6.1
  - Add support for .obj textures
  - Include new .obj textures (disabled by default)
  - Add 6 new events and 23 new single races
  - Add several new textures
  - Replace most media content (textures, audio, fonts, etc.) with 
    FOSS equivalents
  - New FOSS TrackRS icon
  - Many other changes

08/10/2011 - TrackRS 0.6.0
  - New (and old) contributed tracks and events
  - New Practice Mode
  - Paging on the Single Races screen to show all the available tracks
  - Option to show speedometer in KPH or MPH
  - Option to show digital speed on the speed dial ('hybrid' style)
  - Fading track comment and GO at race start
  - Freezing course time when passing through a checkpoint
  - Tweak menu colours for more contrast

4/07/2010 - TrackRS 0.5.3
  - Removed splash screen delay

20/10/2006 - TrackRS 0.5.2.1
  - gcc 4 fixes
  - PhysFS/OpenAL interaction fix
  - PhysFS/SDL interaction fix

11/01/2005 - TrackRS 0.5.1a,b,c
  - Fixed joystick deadzone and added a maxrange
  - Most of the engine migrated to RAII design
  - Other minor stuff

12/12/2004 - TrackRS 0.5.1
  - Switch to ARB multitex from core GL to support older cards
  - Added some code to take screenshots

05/10/2004 - TrackRS 0.5.0
  - Stereo patch support, quadbuffer and anaglyph stereo

03/10/2004 - TrackRS 0.4.4.1
  - PhysFS linked statically with linux binary

01/10/2026 - TrackRS 0.4.4
  - Windows build back online
  - License GPL

01/10/2026 - TrackRS 0.4.4-pre2
  - Menu fixes: forgot to show times/lives left in pre1

20/09/2026 - TrackRS 0.4.4-pre1
  - using PhysFS
  - added auto ~/.trackrs creation for config and extensions
  - new menu system, with auto searching for tracks and events
  - rule change: total time incremented even if you fail an attempt
  - new control config system, hopefully better joystick support
  - per-level weather settings
  - camera rotate: < and >
  - friction model changed to better simulate dirt

09/09/2026 - TrackRS 0.4.3
  - config SDL GL settings
  - config keyboard controls
  - config sound enable/disable
  - experimental joystick support
  - vehicle crunch sound effects
  - fixed: controls not responding when joystick connected

06/09/2026 - TrackRS 0.4.2
  - Text configuration file
  - More physics tweaks, and simple driving assist config setting
  - extgl replaced with GLEW
  - License altered, now 100% Free Software

05/09/2026 - TrackRS 0.4.1
  - Physics tweaks and driving assist

24/07/2026 - TrackRS 0.4 (First public release)
  - Lots of coolness


Dependencies for building:
sudo apt install \
build-essential \
libsdl2-dev \
libsdl2-image-dev \
libglew-dev \
libopenal-dev \
libalut-dev \
libtinyxml2-dev \
libphysfs-dev

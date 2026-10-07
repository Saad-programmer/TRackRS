TRACKRS-STEREO 

Creator:
  Saad AIT YAHIA - @github: Saad-programmer


A. INTRODUCTION

   TrackRS is an awesome game visually speaking, and supporting 3D stereo hooks allows exploring the
maps and scenes in 3D (Virtual Reality) like stereo graphics.

   This guide helps configure your system for hardware accelerated "quadbuffer" stereo performance.


B. SUPPORTED CARDS:

Nvidia Quadro series
ATI FireGL series  
Nvidia hardware hacked GeForce2 cards.  

  Newer consumer cards like GeForce or Radeon are generally not officially supported for active shutter stereo by their drivers on Linux, but anaglyph stereo works on all systems.


C. CONFIGURE YOUR CARD

 Read the GPU driver release notes on how to configure your XF86Config or Xorg config file to support stereo with the option "Stereo". This is added in the section "Device" of the config file.


D. LCD SHUTTER GLASSES

If you have a supported card, you need a pair of LCD shutter glasses and a Sync Doubler.


E. SUPPORTED DISPLAYS:

  LCDs:
If you have a standard LCD monitor, active stereo is not possible without polarizers or using anaglyph glasses. Autostereo LCDs that render 3D stereo graphics without glasses are also supported by TrackRS.

  CRTs:
For real 'Quadbuffer' stereo, a high vertical refresh rate CRT monitor is key. You really need a CRT that will support a high vertical refresh rate (75 - 85Hz or better).


F. ANAGLYPH AND OTHER OPTIONS

Stereo anaglyph viewing is where the left and right eye views are color filtered and by wearing a pair of special colored glasses (like red-cyan), the corresponding views appear in three dimensions.


G. TIPS.

You can toggle display modes or configure a default resolution in your Xorg configuration file.  


H. TRACKRS STEREO SUPPORT.

Stereo is configured within the trackrs.config file under the video settings:

stereo="none"
    or "quadbuffer"
    or "red-blue"
    or "red-green"
    or "red-cyan"
    or "yellow-blue"
    - Enables stereo and selects a mode.

stereoeyeseperation="0.07"
    - Changes the images based on how far apart your eyes are.
      The value is the distance between your eyes in meters.
      Decrease this value if you feel uncomfortable.

stereoswapeyes="no" or "yes"
    - Set this to "yes" if the image appears to be swapped.


I. WARNING

  If you tend to get motion sickness, you should really not use the stereo mode on TrackRS. If you have any health concerns like Epilepsy, you should not use this option.


J. PERFORMANCE NOTES

Out of the five stereo modes available, quadbuffer (with appropriate hardware) is likely to be the most efficient. However, many people will use the anaglyph stereo modes (red-cyan, etc.).

It's worth noting that red-cyan and yellow-blue modes will have the best performance.

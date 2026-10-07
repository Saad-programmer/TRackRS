// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "config.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

class Gui;
class PConfig;

///
/// @brief Controls menu
///
class PControl {
public:
  PControl(Gui &parent, PConfig &config);

  void render();
  void select(int index);
  bool handleKey(const SDL_KeyboardEvent &ke);

private:
  /// Data of one control item
  struct Control {
    PConfig::Action action; ///< Action associated with a key
    std::string text;       ///< Text displayed in control menu
  };

  PControl();
  PControl(const PControl&);
  PControl& operator=(const PControl&);

  void addControl(Control &control, bool active);
  void unassignKey(SDL_Keycode keycode);

  Gui &parent;                      ///< Holds functions to add visual elements
  PConfig &cfg;                     ///< Configuration data of the game
  int pos;                          ///< Index of of each option row
  int activepos;                    ///< Index of item waiting for key
  std::vector<Control> controls;    ///< Data of each control item
};

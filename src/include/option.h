// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include <string>
#include <vector>

#include "config.h"

class Gui;
class GuiWidget;

///
/// @brief Options menu
///
class POption {
public:
  POption(Gui &parent, PConfig &config);

  void render();
  void select(int index);

private:
  enum OptionId {
    OptionPlayerName,
    OptionEngineVolume,
    OptionSfxVolume,
    OptionCodriverVolume,
    OptionCodriverVoice,
    OptionCodriverSigns,
    OptionTextureQuality,
    OptionSnowflakes,
    OptionSpeedUnits,
    OptionGhostCars,
    OptionDisplayFps,
    OptionFoliage,
    OptionRoadsigns,
    OptionDirtEffect,
    OptionMaxSize
  };
  /// Data of one option item
  struct Option {
    OptionId id;                        ///< Identifier of option from enum
    std::string text;                   ///< Text displayed in options menu
    std::vector<std::string> values;    ///< Options that can be selected in menu
    unsigned int select;                ///< Index of option currently selected
  };

  POption();
  POption(const POption&);
  POption& operator=(const POption&);

  void addOption(Option &option);
  void updateSelect(Option &option);
  unsigned int findStringPos(const std::string &str, std::vector<std::string> &vec);

  Gui &parent;                  ///< Holds functions to add labels
  PConfig &cfg;                 ///< Configuration data of the game
  unsigned int pos;             ///< Index of of each option row
  std::vector<Option> options;  ///< Data of each option item
};

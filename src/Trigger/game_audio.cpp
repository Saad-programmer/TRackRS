// SPDX-License-Identifier: GPL-2.0-or-later
#include "main.h"
#include "vehicle.h"
#include "validation.h"
void MainApp::toggleSounds(bool to)
{
    if (cfg.getEnableSound())
    {
        if (audinst_engine != nullptr)
        {
            if (!to)
            {
                audinst_engine->setGain(0.0f);
                audinst_engine->play();
            }
        }

        if (audinst_wind != nullptr)
        {
            if (!to)
            {
                audinst_wind->setGain(0.0f);
                audinst_wind->play();
            }
        }

        if (audinst_gravel != nullptr)
        {
            if (!to)
            {
                audinst_gravel->setGain(0.0f);
                audinst_gravel->play();
            }
        }
    }
}

///
/// @brief Initialize game sounds instances
///
void MainApp::initAudio()
{
  if (cfg.getEnableSound()) {
	// engine sound
    audinst_engine = std::make_unique<PAudioInstance>(aud_engine, true);
    audinst_engine->setGain(0.0);
    audinst_engine->play();

	// wind sound
    audinst_wind = std::make_unique<PAudioInstance>(aud_wind, true);
    audinst_wind->setGain(0.0);
    audinst_wind->play();

	// terrain sound
    audinst_gravel = std::make_unique<PAudioInstance>(aud_gravel, true);
    audinst_gravel->setGain(0.0);
    audinst_gravel->play();
  }
}


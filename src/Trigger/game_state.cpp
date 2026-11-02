// SPDX-License-Identifier: GPL-2.0-or-later
#include "main.h"
#include "vehicle.h"
#include "validation.h"
void MainApp::tick(float delta)
{
    if (validation::run.options.enabled && appstate == AS_LEVEL_SCREEN && !validation::run.started) {
        const auto begin = SDL_GetPerformanceCounter();
        if (!startGame(validation::run.options.stage)) {
            validation::run.result = 1;
            requestExit();
            return;
        }
        const std::string path = "/vehicles/" + validation::run.options.vehicle + "/" +
            validation::run.options.vehicle + ".vehicle";
        auto* type = game->sim->loadVehicleType(path, getSSModel());
        if (!type || !game->vehicle.empty()) {
            PUtil::outLog() << "Validation requires an available vehicle and a stage without pre-spawned vehicles\n";
            validation::run.result = 1;
            requestExit();
            return;
        }
        game->chooseVehicle(type);
        initAudio();
        appstate = AS_IN_GAME;
        validation::run.started = true;
        validation::run.loadMs = 1000.0 * static_cast<double>(SDL_GetPerformanceCounter() - begin) /
            static_cast<double>(SDL_GetPerformanceFrequency());
        validation::run.renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        SDL_GL_SetSwapInterval(0);
    }
    getSSAudio().tick();

    if (audinst_intro) {
      audinst_intro->setGain(0.5f * cfg.getVolumeSfx());
    }

    menu_time += delta;

  switch (appstate) {
  case AS_LOAD_1:
    splashtimeout -= delta;
    if (--loadscreencount <= 0)
      appstate = AS_LOAD_2;
    break;
  case AS_LOAD_2:
    splashtimeout -= delta;
    if (!loadAll()) {
      requestExit();
      return;
    }
    appstate = AS_LOAD_3;
    break;
  case AS_LOAD_3:
    splashtimeout -= delta;
    if (splashtimeout <= 0.0f)
      levelScreenAction(AA_INIT, 0);
    break;

  case AS_LEVEL_SCREEN:
    tickStateLevel(delta);
    break;

  case AS_CHOOSE_VEHICLE:
    tickStateChoose(delta);
    break;

  case AS_IN_GAME:
      if (!pauserace)
        tickStateGame(delta);
    break;

  case AS_END_SCREEN:
    splashtimeout += delta * 0.04f;
    if (splashtimeout >= 1.0f)
      requestExit();
    break;
  }
}

void MainApp::tickStateChoose(float delta)
{
  choose_spin += delta * 2.0f;
}


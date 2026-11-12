// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include <sstream>
#include "main.h"
#include "option.h"

const int MAX_RACES_ON_SCREEN   = 12;
const int MAX_TIMES_ON_SCREEN   = 13;

// Best Times table font information, must be kept updated
const float fa      = 8.0f / 12.0f;     // Font Aspect
const float fs      = 20.0f;            // Font Size
const float fw      = fa * fs;          // Font Width

// X coordinate values for Best Times table labels
const float XTIMES_PLAYERNAME   = 100.0f + 4 * fw; // 150.0f;
const float XTIMES_CARNAME      = 350.0f;
const float XTIMES_CARCLASS     = 475.0f;
const float XTIMES_TOTALTIME    = 700.0f;

void MainApp::levelScreenAction(int action, int index)
{
  appstate = AS_LEVEL_SCREEN;

  switch (action) {
  case AA_INIT:
    lss.state = AM_TOP;
    break;
  case AA_RESUME:
    // lss.state should be AM_TOP_EVT_PREP, continuing event
    break;
  case AA_RELOAD_ALL:
    reloadAll();
    cfg.storeConfig();
    lss.state = AM_TOP;
    break;
  case AA_GO_TOP:
    lss.state = AM_TOP;
    break;
  case AA_GO_EVT:
    if (lss.state == AM_TOP_EVT_PREP &&
      lss.currentlevel > 0 &&
      lss.currentlevel < (int)events[lss.currentevent].levels.size()) {
      lss.state = AM_TOP_EVT_ABANDON;
    } else {
      lss.currentevent = index;
      lss.state = AM_TOP_EVT;
    }
    break;
  case AA_PICK_EVT:
    lss.currentevent = index;
    lss.currentlevel = 0;
    lss.livesleft = 3;
    lss.leveltimes.clear();
    lss.totaltime = 0.0f;
    lss.state = AM_TOP_EVT_PREP;
    break;
  case AA_RESUME_EVT:
    lss.state = AM_TOP_EVT_PREP;
    break;
  case AA_RESTART_EVT:
    lss.currentlevel = 0;
    lss.livesleft = 3;
    lss.leveltimes.clear();
    lss.totaltime = 0.0f;
    lss.state = AM_TOP_EVT_PREP;
    break;
  case AA_GO_PRAC:
    lss.currentevent = index;
    lss.state = AM_TOP_PRAC;
    break;
  case AA_PICK_PRAC:
    lss.currentevent = index;
    lss.state = AM_TOP_PRAC_SEL;
    break;
  case AA_PICK_PRAC_LVL:
    lss.currentlevel = index;
    lss.state = AM_TOP_PRAC_SEL_PREP;
    break;
  case AA_GO_LVL:
    lss.currentlevel = index;
    lss.state = AM_TOP_LVL;
    break;
  case AA_PICK_LVL:
    if (editionPolicy.stage(levels[index].filename)) {
      lss.currentlevel = index;
      lss.state = AM_TOP_LVL_PREP;
    }
    break;
  case AA_GO_QUIT:
    lss.state = AM_TOP_QUIT;
    break;
  case AA_QUIT_CONFIRM:
    quitGame();
    break;

  case AA_START_EVT:
  case AA_START_PRAC:
    startGame(events[lss.currentevent].levels[lss.currentlevel].filename);
    return;
  case AA_START_LVL:
    if (editionPolicy.stage(levels[lss.currentlevel].filename)) {
      startGame(levels[lss.currentlevel].filename);
    }
    return;

    case AA_SHOWTIMES_LVL:
        lss.currentplayer = index;
        break;

    case AA_SHOWTIMES_PRAC:
        lss.currentplayer = index;
        break;

    case AA_BSHOWTIMES_LVL:
        lss.currentplayer = index;
        lss.state = AM_TOP_LVL_BTIMES;
        current_times = best_times.getCurrentTimes(
            levels[lss.currentlevel].filename,
            HISCORE1_SORT::BY_TOTALTIME_ASC);
        break;

    case AA_BSHOWTIMES_PRAC:
        lss.currentplayer = index;
        lss.state = AM_TOP_PRAC_BTIMES;
        current_times = best_times.getCurrentTimes(
            events[lss.currentevent].levels[lss.currentlevel].filename,
            HISCORE1_SORT::BY_TOTALTIME_ASC);
        break;

    case AA_SORT_BY_PLAYERNAME:
    {
        lss.currentplayer = 0;

        if (hs_sort_method == HISCORE1_SORT::BY_PLAYERNAME_ASC)
            hs_sort_method = HISCORE1_SORT::BY_PLAYERNAME_DESC;
        else
            hs_sort_method = HISCORE1_SORT::BY_PLAYERNAME_ASC;

        break;
    }

    case AA_SORT_BY_CARNAME:
    {
        lss.currentplayer = 0;

        if (hs_sort_method == HISCORE1_SORT::BY_CARNAME_ASC)
            hs_sort_method = HISCORE1_SORT::BY_CARNAME_DESC;
        else
            hs_sort_method = HISCORE1_SORT::BY_CARNAME_ASC;

        break;
    }

    case AA_SORT_BY_CARCLASS:
    {
        lss.currentplayer = 0;

        if (hs_sort_method == HISCORE1_SORT::BY_CARCLASS_ASC)
            hs_sort_method = HISCORE1_SORT::BY_CARCLASS_DESC;
        else
            hs_sort_method = HISCORE1_SORT::BY_CARCLASS_ASC;

        break;
    }

    case AA_SORT_BY_TOTALTIME:
    {
        lss.currentplayer = 0;

        if (hs_sort_method == HISCORE1_SORT::BY_TOTALTIME_ASC)
            hs_sort_method = HISCORE1_SORT::BY_TOTALTIME_DESC;
        else
            hs_sort_method = HISCORE1_SORT::BY_TOTALTIME_ASC;

        break;
    }

    case AA_GO_OPT:
      lss.state = AM_TOP_OPT;
      break;

    case AA_PICK_OPT:
      option.select(index);
      break;

    case AA_GO_CTRL:
      lss.state = AM_TOP_CTRL;
      break;

    case AA_PICK_CTRL:
      control.select(index);
      break;

  default:
    PUtil::outLog() << "ERROR: invalid action code " << action << std::endl;
    requestExit();
    return;
  }

  gui.setSSRender(getSSRender());
  gui.setFont(tex_fontSourceCodeShadowed);
  grabMouse(false);
  gui.clear();
  gui.addLabel(10.0f,570.0f, "TrackRS", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Weak);

  switch (lss.state) {
  case AM_TOP:
    for (int practice = 0; practice < 2; ++practice) {
      const bool enabled = editionPolicy.events();
      const auto label = gui.addLabel(400.0f, 400.0f - practice * 50.0f,
        std::string(practice ? "practice" : "events") + (enabled ? "" : " (locked)"),
        PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f, enabled ? LabelStyle::Strong : LabelStyle::Weak);
      if (enabled) gui.makeClickable(label, practice ? AA_GO_PRAC : AA_GO_EVT, 0);
    }
    gui.makeClickable(
      gui.addLabel(400.0f,300.0f, "single race", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_LVL, 0);
    gui.makeClickable(
      gui.addLabel(400.0f,250.0f, "options", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_OPT, 0);
    gui.makeClickable(
      gui.addLabel(400.0f,200.0f, "controls", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_CTRL, 0);
    gui.makeClickable(
      gui.addLabel(10.0f,30.0f, "quit", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 40.0f), AA_GO_QUIT, 0);

    gui.addLabel(790.0f, 570.0f, "version " PACKAGE_VERSION, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Weak);
    gui.addLabel(790.0f, 30.0f, "Build: " __DATE__ " at " __TIME__, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
    // M0: broken content packs are reported here, never fatal (first four shown).
    for (std::size_t i = 0; i < packNotices.size() && i < 4; ++i)
      gui.addLabel(400.0f, 140.0f - static_cast<float>(i) * 18.0f, packNotices[i].substr(0, 90),
                   PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 16.0f, LabelStyle::Weak);
    break;
  case AM_TOP_EVT:
  {
    gui.makeClickable(
      gui.addLabel(10.0f,30.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 40.0f), AA_GO_TOP, 0);
    gui.addLabel(100.0f,470.0f, "Choose Event:", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Header);

      int firstraceindex = index;
      const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
      const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

      if (firstraceindex > 0) { // FIXME: originally was AA_GO_PRAC?
        gui.makeClickable(prevbutton, AA_GO_EVT, firstraceindex - MAX_RACES_ON_SCREEN);
      }

      int racesonscreencount = events.size() - firstraceindex;

      if (racesonscreencount > MAX_RACES_ON_SCREEN) {
        racesonscreencount = MAX_RACES_ON_SCREEN;
        gui.makeClickable(nextbutton, AA_GO_EVT, firstraceindex + MAX_RACES_ON_SCREEN);
      }

      std::stringstream racecountmsg;
      racecountmsg << "events " << firstraceindex + 1 << '-' << firstraceindex + racesonscreencount << '/' << events.size();
      gui.addLabel(790.0f, 570.0f, racecountmsg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
        gui.addLabel(700, 470, "races (timelimit)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20);

    for (int i = firstraceindex; i < firstraceindex + racesonscreencount; i++) {

        const int eventlabel = gui.addLabel(100.0f,440.0f - (float)(i - firstraceindex) * 30.0f,
            events[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);

        if (editionPolicy.event(events[i].locked, player_unlocks.count(events[i].filename) != 0))
            gui.makeClickable(eventlabel, AA_PICK_EVT, i);

      gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            PUtil::formatInt(events[i].levels.size()) + " (" + events[i].totaltime + ')',
            PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
    }
    break;
  }
  case AM_TOP_EVT_PREP:
    gui.makeClickable(
      gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
      AA_GO_EVT, 0);
    gui.addLabel(790.0f, 570.0f, events[lss.currentevent].name, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
    gui.addLabel(100.0f,470.0f, "Races:", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Header);
    gui.addLabel(700, 470, "status/time", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20);

    for (unsigned int i = 0; i < events[lss.currentevent].levels.size(); i++) {

      LabelStyle namestyle = LabelStyle::List;

      if (lss.currentlevel > (int)i)
        namestyle = LabelStyle::Strong;
      else
      if (lss.currentlevel == (int)i)
        namestyle = LabelStyle::Marked;

      gui.addLabel(100.0f,440.0f - (float)i * 30.0f,
        events[lss.currentevent].levels[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, namestyle);

      std::string infotext = "not yet raced";
      LabelStyle infostyle = LabelStyle::List;

      if (lss.currentlevel > (int)i)
      {
        infotext = PUtil::formatTime(lss.leveltimes[i]);
        infostyle = LabelStyle::Strong;
      }
      else if (lss.currentlevel == (int)i)
      {
        infotext = "next";
        infostyle = LabelStyle::Marked;
      }
      gui.addLabel(700.0f,440.0f - (float)i * 30.0f,
        infotext, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, infostyle);
    }
    gui.addLabel(700.0f,430.0f - (float)events[lss.currentevent].levels.size() * 30.0f,
      "Total: " + PUtil::formatTime(lss.totaltime), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::Strong);
    if (lss.livesleft <= 0) {
      gui.addLabel(400.0f, 10.0f, "no tries remaining", PTEXT_HZA_CENTER | PTEXT_VTA_BOTTOM, 20.0f, LabelStyle::Strong);
      gui.makeClickable(
        gui.addLabel(790.0f,10.0f, "restart", PTEXT_HZA_RIGHT | PTEXT_VTA_BOTTOM, 40.0f),
        AA_RESTART_EVT, 0);
    } else if (lss.currentlevel >= (int)events[lss.currentevent].levels.size()) {
      gui.addLabel(400.0f,10.0f, "EVENT COMPLETED!", PTEXT_HZA_CENTER | PTEXT_VTA_BOTTOM, 30.0f, LabelStyle::Marked);
    } else {
      gui.addLabel(400.0f,10.0f, PUtil::formatInt(lss.livesleft) + " tries remaining",
        PTEXT_HZA_CENTER | PTEXT_VTA_BOTTOM, 20.0f, LabelStyle::Strong);

      for (int i = 0; i < lss.livesleft; i++) {
        gui.addGraphic(325.0f + i * 50.0f,30.0f, 50.0f,50.0f, tex_hud_life);
      }
      gui.makeDefault(
        gui.makeClickable(
          gui.addLabel(790.0f,10.0f, "race", PTEXT_HZA_RIGHT | PTEXT_VTA_BOTTOM, 40.0f),
          AA_START_EVT, 0));
    }
    break;
  case AM_TOP_EVT_ABANDON:
    gui.addLabel(400.0f,350.0f, "Really leave Event?", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f, LabelStyle::Marked);
    gui.makeClickable(
      gui.addLabel(300.0f,250.0f, "Yes", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f),
      AA_GO_EVT, 0);
    gui.makeClickable(
      gui.addLabel(500.0f,250.0f, "No", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f),
      AA_RESUME_EVT, 0);
    break;
  case AM_TOP_PRAC:
  {
    gui.makeClickable(
      gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f), AA_GO_TOP, 0);
    gui.addLabel(100.0f,470.0f, "Practice Event:", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Header);

      int firstraceindex = index;
      const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
      const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

      if (firstraceindex > 0) {
        gui.makeClickable(prevbutton, AA_GO_PRAC, firstraceindex - MAX_RACES_ON_SCREEN);
      }

      int racesonscreencount = events.size() - firstraceindex;

      if (racesonscreencount > MAX_RACES_ON_SCREEN) {
        racesonscreencount = MAX_RACES_ON_SCREEN;
        gui.makeClickable(nextbutton, AA_GO_PRAC, firstraceindex + MAX_RACES_ON_SCREEN);
      }

      std::stringstream racecountmsg;
      racecountmsg << "events " << firstraceindex + 1 << '-' << firstraceindex + racesonscreencount << '/' << events.size();
      gui.addLabel(790.0f, 570.0f, racecountmsg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
      gui.addLabel(700, 470, "races (timelimit)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20);

    for (int i = firstraceindex; i < firstraceindex + racesonscreencount; i++) {

        const int eventlabel = gui.addLabel(100.0f,440.0f - (float)(i - firstraceindex) * 30.0f,
            events[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);

        if (editionPolicy.event(events[i].locked, player_unlocks.count(events[i].filename) != 0))
            gui.makeClickable(eventlabel, AA_PICK_PRAC, i);

      gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            PUtil::formatInt(events[i].levels.size()) + " (" + events[i].totaltime + ')',
            PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
    }
    break;
  }
  case AM_TOP_PRAC_SEL:
  {
    gui.makeClickable(
      gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
      AA_GO_PRAC, 0);
    gui.addLabel(790.0f, 570.0f, events[lss.currentevent].name, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
    gui.addLabel(100.0f,470.0f, "Choose Race:", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Header);
    gui.addLabel(700, 470, "timelimit", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20);

    for (unsigned int i = 0; i < events[lss.currentevent].levels.size(); i++) {
        gui.makeClickable(
          gui.addLabel(100.0f, 440.0f - (float)i * 30.0f,
          events[lss.currentevent].levels[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List),
          AA_PICK_PRAC_LVL, i);

        gui.addLabel(700.0f, 440.0f - (float)i * 30.0f,
            events[lss.currentevent].levels[i].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
    }
    break;
  }
  case AM_TOP_PRAC_SEL_PREP:
  {
      const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
      const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

      int idxnext = lss.currentlevel + 1;
      int idxprev = lss.currentlevel - 1;

      CLAMP(idxnext, 0, static_cast<int> (events[lss.currentevent].levels.size() - 1));
      CLAMP(idxprev, 0, static_cast<int> (events[lss.currentevent].levels.size() - 1));

    if (lss.currentlevel < static_cast<int> (events[lss.currentevent].levels.size() - 1))
      gui.makeClickable(nextbutton, AA_PICK_PRAC_LVL, idxnext);

    if (lss.currentlevel > 0)
      gui.makeClickable(prevbutton, AA_PICK_PRAC_LVL, idxprev);

    gui.makeClickable(
      gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
      AA_PICK_PRAC, lss.currentevent);

    gui.makeClickable(
        gui.addLabel(400.0f, 10.0f, "best times", PTEXT_HZA_CENTER | PTEXT_VTA_BOTTOM, 40.0f),
        AA_BSHOWTIMES_PRAC, 0);

    gui.addLabel(790.0f, 570.0f, events[lss.currentevent].name + " (" +
        PUtil::formatInt(lss.currentlevel + 1) + '/' + PUtil::formatInt(events[lss.currentevent].levels.size()) + ')',
        PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
    gui.addLabel(100.0f,500.0f, events[lss.currentevent].levels[lss.currentlevel].name,
        PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
    gui.addLabel(100.0f,462.5f,
        std::string("by ") + events[lss.currentevent].levels[lss.currentlevel].author,
        PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
    gui.addLabel(700.0f, 462.5f, events[lss.currentevent].levels[lss.currentlevel].targettimeshort,
        PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);

    if (events[lss.currentevent].levels[lss.currentlevel].tex_screenshot != nullptr)
        gui.addGraphic(100, 175, 250.0f * 4/3, 250, events[lss.currentevent].levels[lss.currentlevel].tex_screenshot);
    else
        gui.addGraphic(100, 175, 250.0f * 4/3, 250, tex_race_no_screenshot);

    if (events[lss.currentevent].levels[lss.currentlevel].tex_minimap != nullptr)
        gui.addGraphic(450, 175, 250, 250, events[lss.currentevent].levels[lss.currentlevel].tex_minimap);
    else
        gui.addGraphic(450, 175, 250, 250, tex_race_no_minimap);

    gui.addLabel(100.0f,150.0f, events[lss.currentevent].levels[lss.currentlevel].description,
        PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f);
    gui.makeDefault(
      gui.makeClickable(
        gui.addLabel(790.0f,10.0f, "race", PTEXT_HZA_RIGHT | PTEXT_VTA_BOTTOM, 40.0f),
        AA_START_PRAC, 0));
    break;
  }
  case AM_TOP_LVL:
    {
      gui.makeClickable(
        gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
        AA_GO_TOP, 0);
      gui.addLabel(100.0f,470.0f, "Choose Race:", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Header);

      int firstraceindex = index;
      const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
      const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

      if (firstraceindex > 0) {
        gui.makeClickable(prevbutton, AA_GO_LVL, firstraceindex - MAX_RACES_ON_SCREEN);
      }

      int racesonscreencount = levels.size() - firstraceindex;

      if (racesonscreencount > MAX_RACES_ON_SCREEN) {
        racesonscreencount = MAX_RACES_ON_SCREEN;
        gui.makeClickable(nextbutton, AA_GO_LVL, firstraceindex + MAX_RACES_ON_SCREEN);
      }
      std::stringstream racecountmsg;
      racecountmsg << "single races " << firstraceindex + 1 << '-' << firstraceindex + racesonscreencount << '/' << levels.size();
      gui.addLabel(790.0f, 570.0f, racecountmsg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);

        gui.addLabel(700, 470, "timelimit", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20);

      for (int i = firstraceindex; i < firstraceindex + racesonscreencount; i++) {
        if (editionPolicy.stage(levels[i].filename)) {
          gui.makeClickable(
            gui.addLabel(100.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            levels[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List),
            AA_PICK_LVL, i);

          gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
              levels[i].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
        } else {
          gui.addLabel(100.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            levels[i].name + " (locked)", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::Weak);

          gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
              levels[i].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::Weak);
        }
      }
    }
    break;
  case AM_TOP_LVL_PREP:
  {
      const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
      const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

      int idxnext = lss.currentlevel + 1;
      int idxprev = lss.currentlevel - 1;

      CLAMP(idxnext, 0, static_cast<int> (levels.size() - 1));
      CLAMP(idxprev, 0, static_cast<int> (levels.size() - 1));

      if (lss.currentlevel < static_cast<int> (levels.size() - 1) && editionPolicy.stage(levels[idxnext].filename))
        gui.makeClickable(nextbutton, AA_PICK_LVL, idxnext);

      if (lss.currentlevel > 0 && editionPolicy.stage(levels[idxprev].filename))
        gui.makeClickable(prevbutton, AA_PICK_LVL, idxprev);

    gui.makeClickable(
      gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
      AA_GO_LVL, (lss.currentlevel / MAX_RACES_ON_SCREEN) * MAX_RACES_ON_SCREEN);

    gui.makeClickable(
        gui.addLabel(400.0f, 10.0f, "best times", PTEXT_HZA_CENTER | PTEXT_VTA_BOTTOM, 40.0f),
        AA_BSHOWTIMES_LVL, 0);

    std::stringstream racenummsg;

    racenummsg << "single race " << lss.currentlevel+1 << '/' << levels.size();
    gui.addLabel(790.0f,570.0f, racenummsg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);
    gui.addLabel(100.0f,500.0f, levels[lss.currentlevel].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
    gui.addLabel(100.0f,462.5f,
        std::string("by ") + levels[lss.currentlevel].author,
        PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
    gui.addLabel(700.0f, 462.5f, levels[lss.currentlevel].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);

    if (levels[lss.currentlevel].tex_screenshot != nullptr)
        gui.addGraphic(100, 175, 250.0f * 4/3, 250, levels[lss.currentlevel].tex_screenshot);
    else
        gui.addGraphic(100, 175, 250.0f * 4/3, 250, tex_race_no_screenshot);

    if (levels[lss.currentlevel].tex_minimap != nullptr)
        gui.addGraphic(450, 175, 250, 250, levels[lss.currentlevel].tex_minimap);
    else
        gui.addGraphic(450, 175, 250, 250, tex_race_no_minimap);

    gui.addLabel(100.0f,150.0f, levels[lss.currentlevel].description, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f);
    gui.makeDefault(
      gui.makeClickable(
        gui.addLabel(790.0f,10.0f, "race", PTEXT_HZA_RIGHT | PTEXT_VTA_BOTTOM, 40.0f),
        AA_START_LVL, 0));
  }
    break;
  case AM_TOP_QUIT:
    gui.addLabel(400.0f,350.0f, "Really quit?", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f, LabelStyle::Marked);
    gui.makeClickable(
      gui.addLabel(300.0f,250.0f, "Yes", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f),
      AA_QUIT_CONFIRM, 0);
    gui.makeClickable(
      gui.addLabel(500.0f,250.0f, "No", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 40.0f),
      AA_GO_TOP, 0);
    break;

    case AM_TOP_LVL_TIMES:
    {
        gui.addLabel(100.0f,500.0f, levels[lss.currentlevel].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
        gui.addLabel(100.0f,462.5f,
            std::string("by ") + levels[lss.currentlevel].author,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
        gui.addLabel(700.0f, 462.5f, levels[lss.currentlevel].targettime, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);
        gui.makeClickable(
            gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
            AA_PICK_LVL, lss.currentlevel);

        current_times = best_times.getCurrentTimesHL(hs_sort_method);

        int first_time_index = index;

        const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
        const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

        if (first_time_index > 0)
            gui.makeClickable(prevbutton, AA_SHOWTIMES_LVL, first_time_index - MAX_TIMES_ON_SCREEN);

        int times_on_screen_count = current_times.size() - first_time_index;

        if (times_on_screen_count > MAX_TIMES_ON_SCREEN)
        {
            times_on_screen_count = MAX_TIMES_ON_SCREEN;
            gui.makeClickable(nextbutton, AA_SHOWTIMES_LVL, first_time_index + MAX_TIMES_ON_SCREEN);
        }

        std::stringstream times_count_msg;

        times_count_msg << "best times " << first_time_index + 1 << '-'
            << first_time_index + times_on_screen_count << '/' << current_times.size();
        gui.addLabel(790.0f, 570.0f, times_count_msg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);

        // column buttons
        gui.makeClickable(
            gui.addLabel(XTIMES_PLAYERNAME, 420.0f, "player", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_PLAYERNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARNAME, 420.0f, "car", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARCLASS, 420.0f, "class", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARCLASS, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_TOTALTIME, 420.0f, "time", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_TOTALTIME, 0);

        for (int i = first_time_index; i < first_time_index + times_on_screen_count; ++i)
        {
            LabelStyle ls;

            if (current_times[i].highlighted)
                ls = LabelStyle::Marked;
            else
                ls = LabelStyle::List;

            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                std::to_string(current_times[i].place) + ". ", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.playername.substr(0, 14), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_CARNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carname.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_CARCLASS, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carclass.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_TOTALTIME, 395.0f - (float)(i - first_time_index) * 25.0f,
                PUtil::formatTime(current_times[i].rd.totaltime), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, ls);
        }

        break;
    }

    case AM_TOP_LVL_BTIMES:
    {
        gui.addLabel(100.0f,500.0f, levels[lss.currentlevel].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
        gui.addLabel(100.0f,462.5f,
            std::string("by ") + levels[lss.currentlevel].author,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
        gui.addLabel(700.0f, 462.5f, levels[lss.currentlevel].targettime, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);
        gui.makeClickable(
            gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
            AA_PICK_LVL, lss.currentlevel);

        current_times = best_times.getCurrentTimes("", hs_sort_method);

        int first_time_index = index;

        const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
        const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

        // NOTE: not using AA_BSHOWTIMES_LVL intentionally!
        if (first_time_index > 0)
            gui.makeClickable(prevbutton, AA_SHOWTIMES_LVL, first_time_index - MAX_TIMES_ON_SCREEN);

        int times_on_screen_count = current_times.size() - first_time_index;

        // NOTE: not using AA_BSHOWTIMES_LVL intentionally!
        if (times_on_screen_count > MAX_TIMES_ON_SCREEN)
        {
            times_on_screen_count = MAX_TIMES_ON_SCREEN;
            gui.makeClickable(nextbutton, AA_SHOWTIMES_LVL, first_time_index + MAX_TIMES_ON_SCREEN);
        }

        std::stringstream times_count_msg;

        times_count_msg << "best times";

        if (!current_times.empty())
            times_count_msg << ' ' << first_time_index + 1 << '-'
            << first_time_index + times_on_screen_count << '/' << current_times.size();

        gui.addLabel(790.0f, 570.0f, times_count_msg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);

        // column buttons
        gui.makeClickable(
            gui.addLabel(XTIMES_PLAYERNAME, 420.0f, "player", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_PLAYERNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARNAME, 420.0f, "car", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARCLASS, 420.0f, "class", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARCLASS, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_TOTALTIME, 420.0f, "time", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_TOTALTIME, 0);

        for (int i = first_time_index; i < first_time_index + times_on_screen_count; ++i)
        {
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                std::to_string(current_times[i].place) + ". ", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.playername.substr(0, 14), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_CARNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carname.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_CARCLASS, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carclass.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_TOTALTIME, 395.0f - (float)(i - first_time_index) * 25.0f,
                PUtil::formatTime(current_times[i].rd.totaltime), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
        }

        break;
    }

    case AM_TOP_PRAC_TIMES:
    {
        gui.addLabel(100.0f,500.0f, events[lss.currentevent].levels[lss.currentlevel].name,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
        gui.addLabel(100.0f,462.5f,
            std::string("by ") + events[lss.currentevent].levels[lss.currentlevel].author,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
        gui.addLabel(700.0f, 462.5f, events[lss.currentevent].levels[lss.currentlevel].targettime,
            PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);
        gui.makeClickable(
            gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
            AA_PICK_PRAC_LVL, lss.currentlevel);

        current_times = best_times.getCurrentTimesHL(hs_sort_method);

        int first_time_index = index;

        const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
        const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

        if (first_time_index > 0)
            gui.makeClickable(prevbutton, AA_SHOWTIMES_PRAC, first_time_index - MAX_TIMES_ON_SCREEN);

        int times_on_screen_count = current_times.size() - first_time_index;

        if (times_on_screen_count > MAX_TIMES_ON_SCREEN)
        {
            times_on_screen_count = MAX_TIMES_ON_SCREEN;
            gui.makeClickable(nextbutton, AA_SHOWTIMES_PRAC, first_time_index + MAX_TIMES_ON_SCREEN);
        }

        std::stringstream times_count_msg;

        times_count_msg << "best times " << first_time_index + 1 << '-'
            << first_time_index + times_on_screen_count << '/' << current_times.size();
        gui.addLabel(790.0f, 570.0f, times_count_msg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);

        // column buttons
        gui.makeClickable(
            gui.addLabel(XTIMES_PLAYERNAME, 420.0f, "player", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_PLAYERNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARNAME, 420.0f, "car", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARCLASS, 420.0f, "class", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARCLASS, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_TOTALTIME, 420.0f, "time", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_TOTALTIME, 0);

        for (int i = first_time_index; i < first_time_index + times_on_screen_count; ++i)
        {
            LabelStyle ls;

            if (current_times[i].highlighted)
                ls = LabelStyle::Marked;
            else
                ls = LabelStyle::List;

            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                std::to_string(current_times[i].place) + ". ", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.playername.substr(0, 14), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_CARNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carname.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_CARCLASS, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carclass.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, ls);
            gui.addLabel(XTIMES_TOTALTIME, 395.0f - (float)(i - first_time_index) * 25.0f,
                PUtil::formatTime(current_times[i].rd.totaltime), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, ls);
        }

        break;
    }

    case AM_TOP_PRAC_BTIMES:
    {
        gui.addLabel(100.0f,500.0f, events[lss.currentevent].levels[lss.currentlevel].name,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 35.0f, LabelStyle::Header);
        gui.addLabel(100.0f,462.5f,
            std::string("by ") + events[lss.currentevent].levels[lss.currentlevel].author,
            PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::Weak);
        gui.addLabel(700.0f, 462.5f, events[lss.currentevent].levels[lss.currentlevel].targettime,
            PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f);
        gui.makeClickable(
            gui.addLabel(10.0f, 10.0f, "back", PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM, 40.0f),
            AA_PICK_PRAC_LVL, lss.currentlevel);

        current_times = best_times.getCurrentTimes("", hs_sort_method);

        int first_time_index = index;

        const int prevbutton = gui.addGraphic(20.0f, 275.0f, 50.0f, 50.0f, tex_button_prev, GraphicStyle::Button);
        const int nextbutton = gui.addGraphic(730.0f, 275.0f, 50.0f, 50.0f, tex_button_next, GraphicStyle::Button);

        // NOTE: not using AA_BSHOWTIMES_PRAC intentionally!
        if (first_time_index > 0)
            gui.makeClickable(prevbutton, AA_SHOWTIMES_PRAC, first_time_index - MAX_TIMES_ON_SCREEN);

        int times_on_screen_count = current_times.size() - first_time_index;

        // NOTE: not using AA_BSHOWTIMES_PRAC intentionally!
        if (times_on_screen_count > MAX_TIMES_ON_SCREEN)
        {
            times_on_screen_count = MAX_TIMES_ON_SCREEN;
            gui.makeClickable(nextbutton, AA_SHOWTIMES_PRAC, first_time_index + MAX_TIMES_ON_SCREEN);
        }

        std::stringstream times_count_msg;

        times_count_msg << "best times";

        if (!current_times.empty())
            times_count_msg << ' ' << first_time_index + 1 << '-'
            << first_time_index + times_on_screen_count << '/' << current_times.size();

        gui.addLabel(790.0f, 570.0f, times_count_msg.str(), PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER, 20.0f, LabelStyle::Weak);

        // column buttons
        gui.makeClickable(
            gui.addLabel(XTIMES_PLAYERNAME, 420.0f, "player", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_PLAYERNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARNAME, 420.0f, "car", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARNAME, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_CARCLASS, 420.0f, "class", PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_CARCLASS, 0);
        gui.makeClickable(
            gui.addLabel(XTIMES_TOTALTIME, 420.0f, "time", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f),
            AA_SORT_BY_TOTALTIME, 0);

        for (int i = first_time_index; i < first_time_index + times_on_screen_count; ++i)
        {
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                std::to_string(current_times[i].place) + ". ", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_PLAYERNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.playername.substr(0, 14), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_CARNAME, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carname.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_CARCLASS, 395.0f - (float)(i - first_time_index) * 25.0f,
                current_times[i].rd.carclass.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
            gui.addLabel(XTIMES_TOTALTIME, 395.0f - (float)(i - first_time_index) * 25.0f,
                PUtil::formatTime(current_times[i].rd.totaltime), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 20.0f, LabelStyle::List);
        }

        break;
    }

  case AM_TOP_OPT:
    option.render();
    break;

  case AM_TOP_CTRL:
    control.render();
    break;

  default:
    gui.addLabel(400.0f,300.0f, "Error in menu system, sorry", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 30.0f, LabelStyle::Marked);
    gui.makeClickable(
      gui.addLabel(400.0f,150.0f, "Go to top menu", PTEXT_HZA_CENTER | PTEXT_VTA_TOP, 30.0f),
      AA_GO_TOP, 0);
    break;
  }

  //gui.doLayout();
}

void MainApp::finishRace(Gamefinish state, float coursetime)
{
	switch (lss.state)
	{
		case AM_TOP_EVT_PREP:
			switch (state)
			{
				case Gamefinish::pass:
					lss.leveltimes.resize(events[lss.currentevent].levels.size(), 0.0f);
					lss.leveltimes[lss.currentlevel] += coursetime;
					lss.totaltime += coursetime;
					lss.currentlevel++;

					// event was completed so save unlock data
					if (lss.currentlevel >= (int)events[lss.currentevent].levels.size())
					{
						for (const std::string &s: events[lss.currentevent].unlocks)
							best_times.addNewUnlock(s);

						player_unlocks = best_times.getUnlockData();
						best_times.skipSavePlayer();
					}
					break;

				case Gamefinish::fail:
					lss.totaltime += coursetime;
					lss.livesleft--;
					break;

				default:
					break;
			}
			levelScreenAction(AA_RESUME, 0);
			break;

		case AM_TOP_PRAC_SEL_PREP:
			levelScreenAction(AA_PICK_PRAC_LVL, lss.currentlevel);
			break;

		case AM_TOP_LVL_PREP:
			// Calculate the index of first level in the page by truncating the current level index to the nearest 10
			//levelScreenAction(AA_GO_LVL, (lss.currentlevel / MAX_RACES_ON_SCREEN) * MAX_RACES_ON_SCREEN );
			levelScreenAction(AA_PICK_LVL, lss.currentlevel);
			break;

		case AM_TOP_LVL_TIMES:
			levelScreenAction(AA_SHOWTIMES_LVL, 0);
			break;

		case AM_TOP_PRAC_TIMES:
			levelScreenAction(AA_SHOWTIMES_PRAC, 0);
			break;

		default:
			PUtil::outLog() << "Race finished in invalid state " << lss.state << std::endl;
			break;
	}
}

void MainApp::tickStateLevel(float delta)
{
  gui.tick(delta);
}

// TODO: fix this code
void MainApp::cursorMoveEvent(int posx, int posy)
{
  if (appstate != AS_LEVEL_SCREEN) return;

  const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

  gui.setCursorPos(
    (float)posx / (float)getWidth() * (600.0 * cx / cy) + margin,
    (1.0f - (float)posy / (float)getHeight()) * 600.0f);
}

void MainApp::mouseButtonEvent(const SDL_MouseButtonEvent &mbe)
{
  if (mbe.type != SDL_MOUSEBUTTONDOWN) return;

  switch (appstate) {
  case AS_LEVEL_SCREEN:
    break;
  case AS_LOAD_3:
    levelScreenAction(AA_INIT, 0);
    break;
  default:
    return;
  }

  // TODO: fix this code

  const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

  int action, index;

  if (!gui.getClickAction(action, index)) return;

  levelScreenAction(action, index);

  gui.setCursorPos(
    (float)mbe.x / (float)getWidth() * (600.0 * cx / cy) + margin,
    (1.0f - (float)mbe.y / (float)getHeight()) * 600.0f);
}

//
// TODO: use ActionLeft and ActionRight instead of hardcoded right/left arrow
//
void MainApp::handleLevelScreenKey(const SDL_KeyboardEvent &ke)
{
  if (lss.state == AM_TOP_CTRL)
    if (control.handleKey(ke)) {
      levelScreenAction(AA_GO_CTRL, 0);
      return;
    }

  switch (ke.keysym.sym) {
  case SDLK_ESCAPE:
    switch(lss.state) {
    case AM_TOP:
      levelScreenAction(AA_GO_QUIT, 0);
      break;
    case AM_TOP_EVT_PREP:
    case AM_TOP_EVT_ABANDON:
      levelScreenAction(AA_GO_EVT, 0);
      break;
    case AM_TOP_PRAC_SEL:
      levelScreenAction(AA_GO_PRAC, 0);
      break;
    case AM_TOP_PRAC_SEL_PREP:
      levelScreenAction(AA_PICK_PRAC, lss.currentevent);
      break;
    case AM_TOP_LVL_PREP:
      levelScreenAction(AA_GO_LVL, (lss.currentlevel / MAX_RACES_ON_SCREEN) * MAX_RACES_ON_SCREEN);
      break;
    case AM_TOP_QUIT:
      quitGame();
      break;
    case AM_TOP_LVL_TIMES:
    case AM_TOP_LVL_BTIMES:
        levelScreenAction(AA_PICK_LVL, lss.currentlevel);
        break;
    case AM_TOP_PRAC_TIMES:
    case AM_TOP_PRAC_BTIMES:
        levelScreenAction(AA_PICK_PRAC_LVL, lss.currentlevel);
        break;
    case AM_TOP_OPT:
      levelScreenAction(AA_RELOAD_ALL, 0);
      break;
    case AM_TOP_CTRL:
      levelScreenAction(AA_RELOAD_ALL, 0);
      break;
    default:
      levelScreenAction(AA_GO_TOP, 0);
      break;
    }
    break;
  case SDLK_RETURN:
  case SDLK_KP_ENTER: {
      int data1, data2;

      if (gui.getDefaultAction(data1, data2))
        levelScreenAction(data1, data2);
    } break;

    case SDLK_LEFT:
    {
        int pidx; // previous index

        switch (lss.state)
        {
            case AM_TOP_LVL_PREP:
            {
                pidx = lss.currentlevel - 1;
                CLAMP_LOWER(pidx, 0);
                if (editionPolicy.stage(levels[pidx].filename)) {
                    levelScreenAction(AA_PICK_LVL, pidx);
                }
                break;
            }

            case AM_TOP_LVL:
            {
                pidx = (lss.currentlevel / MAX_RACES_ON_SCREEN - 1) * MAX_RACES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_GO_LVL, pidx);
                break;
            }

            case AM_TOP_EVT:
            {
                pidx = (lss.currentevent / MAX_RACES_ON_SCREEN - 1) * MAX_RACES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_GO_EVT, pidx);
                break;
            }

            case AM_TOP_PRAC:
            {
                pidx = (lss.currentevent / MAX_RACES_ON_SCREEN - 1) * MAX_RACES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_GO_PRAC, pidx);
                break;
            }

            case AM_TOP_PRAC_SEL_PREP:
            {
                pidx = lss.currentlevel - 1;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_PICK_PRAC_LVL, pidx);
                break;
            }

            case AM_TOP_LVL_TIMES:
            {
                pidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN - 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_SHOWTIMES_LVL, pidx);
                break;
            }

            case AM_TOP_PRAC_TIMES:
            {
                pidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN - 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_SHOWTIMES_PRAC, pidx);
                break;
            }

            case AM_TOP_LVL_BTIMES:
            {
                // NOTE: not using AA_BSHOWTIMES_LVL intentionally!
                pidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN - 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_SHOWTIMES_LVL, pidx);
                break;
            }

            case AM_TOP_PRAC_BTIMES:
            {
                // NOTE: not using AA_BSHOWTIMES_PRAC intentionally!
                pidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN - 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_LOWER(pidx, 0);
                levelScreenAction(AA_SHOWTIMES_PRAC, pidx);
                break;
            }
        }

        break;
    }

    case SDLK_RIGHT:
    {
        int nidx; // next index

        switch (lss.state)
        {
            case AM_TOP_LVL_PREP:
            {
                nidx = lss.currentlevel + 1;
                CLAMP_UPPER(nidx, static_cast<int> (levels.size() - 1));
                if (editionPolicy.stage(levels[nidx].filename)) {
                    levelScreenAction(AA_PICK_LVL, nidx);
                }
                break;
            }

            case AM_TOP_LVL:
            {
                if (levels.size() - lss.currentlevel <= MAX_RACES_ON_SCREEN)
                    break;

                nidx = (lss.currentlevel / MAX_RACES_ON_SCREEN + 1) * MAX_RACES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (levels.size() - 1));
                levelScreenAction(AA_GO_LVL, nidx);
                break;
            }

            case AM_TOP_EVT:
            {
                if (events.size() - lss.currentevent <= MAX_RACES_ON_SCREEN)
                    break;

                nidx = (lss.currentevent / MAX_RACES_ON_SCREEN + 1) * MAX_RACES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (events.size() - 1));
                levelScreenAction(AA_GO_EVT, nidx);
                break;
            }

            case AM_TOP_PRAC:
            {
                if (events.size() - lss.currentevent <= MAX_RACES_ON_SCREEN)
                    break;

                nidx = (lss.currentevent / MAX_RACES_ON_SCREEN + 1) * MAX_RACES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (events.size() - 1));
                levelScreenAction(AA_GO_PRAC, nidx);
                break;
            }

            case AM_TOP_PRAC_SEL_PREP:
            {
                nidx = lss.currentlevel + 1;
                CLAMP_UPPER(nidx, static_cast<int> (events[lss.currentevent].levels.size() - 1));
                levelScreenAction(AA_PICK_PRAC_LVL, nidx);
                break;
            }

            case AM_TOP_LVL_TIMES:
            {
                if (current_times.size() - lss.currentplayer <= MAX_TIMES_ON_SCREEN)
                    break;

                nidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN + 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (current_times.size() - 1));
                levelScreenAction(AA_SHOWTIMES_LVL, nidx);
                break;
            }

            case AM_TOP_PRAC_TIMES:
            {
                if (current_times.size() - lss.currentplayer <= MAX_TIMES_ON_SCREEN)
                    break;

                nidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN + 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (current_times.size() - 1));
                levelScreenAction(AA_SHOWTIMES_PRAC, nidx);
                break;
            }

            case AM_TOP_LVL_BTIMES:
            {
                if (current_times.size() - lss.currentplayer <= MAX_TIMES_ON_SCREEN)
                    break;

                // NOTE: not using AA_BSHOWTIMES_LVL intentionally!
                nidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN + 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (current_times.size() - 1));
                levelScreenAction(AA_SHOWTIMES_LVL, nidx);
                break;
            }

            case AM_TOP_PRAC_BTIMES:
            {
                if (current_times.size() - lss.currentplayer <= MAX_TIMES_ON_SCREEN)
                    break;

                // NOTE: not using AA_BSHOWTIMES_PRAC intentionally!
                nidx = (lss.currentplayer / MAX_TIMES_ON_SCREEN + 1) * MAX_TIMES_ON_SCREEN;
                CLAMP_UPPER(nidx, static_cast<int> (current_times.size() - 1));
                levelScreenAction(AA_SHOWTIMES_PRAC, nidx);
                break;
            }
        }

        break;
    }

  default:
    break;
  }
}


void MainApp::renderStateLevel(float eyetranslation)
{
  eyetranslation = eyetranslation;

  trackrs::draw::matrix(trackrs::draw::Projection);
  trackrs::draw::push();
  trackrs::draw::identity();

  const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

  trackrs::draw::ortho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

  trackrs::draw::push();
  trackrs::draw::identity();
  trackrs::draw::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  trackrs::draw::matrix(trackrs::draw::View);

  // draw background image

  trackrs::draw::blend(GL_ONE, GL_ZERO);
  trackrs::draw::disable(GL_DEPTH_TEST);
  trackrs::draw::disable(trackrs::draw::Fog);
  trackrs::draw::disable(trackrs::draw::Lighting);

  tex_splash_screen->bind();

  //trackrs::draw::color(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //trackrs::draw::color(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

  // Old-school rally Ken Burns effect
  float zoom = 0.82f + 0.04f * sinf(menu_time * 0.2f);
  float dx = 0.04f * cosf(menu_time * 0.15f);
  float dy = 0.04f * sinf(menu_time * 0.11f);

  auto map_tx = [&](float tx) { return 0.5f + dx + (tx - 0.5f) * zoom; };
  auto map_ty = [&](float ty) { return 0.5f + dy + (ty - 0.5f) * zoom; };

  trackrs::draw::begin(trackrs::draw::Quads);
  // the background image is square and cut out a piece based on aspect ratio
  // -------- if aspect ratio is larger than 4:3
  // if aspect ratio is larger than 1:1
  if ((float)getWidth()/(float)getHeight() > 1.0f)
  {
    // lower and upper offset based on aspect ratio
    float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
    float off_u = 1 - off_l;
    trackrs::draw::uv(map_tx(1.0f), map_ty(off_u)); trackrs::draw::vertex(1.0f, 1.0f);
    trackrs::draw::uv(map_tx(0.0f), map_ty(off_u)); trackrs::draw::vertex(-1.0f, 1.0f);
    trackrs::draw::uv(map_tx(0.0f), map_ty(off_l)); trackrs::draw::vertex(-1.0f, -1.0f);
    trackrs::draw::uv(map_tx(1.0f), map_ty(off_l)); trackrs::draw::vertex(1.0f, -1.0f);
  }
  // other cases (including 4:3, in which case off_l and off_u are = 1)
  else
  {
    float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
    float off_u = 1 - off_l;
    trackrs::draw::uv(map_tx(off_u), map_ty(1.0f)); trackrs::draw::vertex(1.0f, 1.0f);
    trackrs::draw::uv(map_tx(off_l), map_ty(1.0f)); trackrs::draw::vertex(-1.0f, 1.0f);
    trackrs::draw::uv(map_tx(off_l), map_ty(0.0f)); trackrs::draw::vertex(-1.0f, -1.0f);
    trackrs::draw::uv(map_tx(off_u), map_ty(0.0f)); trackrs::draw::vertex(1.0f, -1.0f);
  }
  trackrs::draw::end();

  // Draw animated gradient, scanlines, and particles
  trackrs::draw::disable(GL_TEXTURE_2D);
  trackrs::draw::enable(GL_BLEND);
  trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 1. Dark colored gradient overlay
  trackrs::draw::begin(trackrs::draw::Quads);
  // Top: deep dark blue
  trackrs::draw::color(0.02f, 0.05f, 0.12f, 0.65f + 0.05f * sinf(menu_time * 0.5f));
  trackrs::draw::vertex(-1.0f, 1.0f);
  trackrs::draw::vertex(1.0f, 1.0f);
  // Bottom: dark warm charcoal/reddish-purple glow
  float r_glow = 0.08f + 0.04f * sinf(menu_time * 0.3f);
  float b_glow = 0.04f + 0.02f * cosf(menu_time * 0.4f);
  trackrs::draw::color(r_glow, 0.01f, b_glow, 0.85f);
  trackrs::draw::vertex(1.0f, -1.0f);
  trackrs::draw::vertex(-1.0f, -1.0f);
  trackrs::draw::end();

  // 2. Subtle scrolling retro CRT scanlines
  trackrs::draw::lineWidth(1.0f);
  trackrs::draw::begin(GL_LINES);
  for (int j = 0; j < 40; ++j) {
      float y = -1.0f + (j / 20.0f);
      float scan_alpha = 0.03f + 0.01f * sinf(menu_time * 2.0f + y * 5.0f);
      trackrs::draw::color(1.0f, 1.0f, 1.0f, scan_alpha);
      trackrs::draw::vertex(-1.0f, y);
      trackrs::draw::vertex(1.0f, y);
  }
  trackrs::draw::end();

  // 3. Stateless speed particles/spark drift
  trackrs::draw::begin(trackrs::draw::Quads);
  for (int k = 0; k < 40; ++k) {
      float speed = 0.25f + 0.2f * sinf(k * 1.45f);
      float size = 0.003f + 0.003f * cosf(k * 3.21f) * cosf(k * 3.21f);
      
      float start_x = 1.2f;
      float end_x = -1.2f;
      float width = start_x - end_x;
      float x = start_x - fmodf(menu_time * speed + (k * 0.17f), 1.0f) * width;
      float y = -1.0f + 2.0f * (0.5f + 0.5f * sinf(k * 7.89f));
      
      float length = size * (4.0f + 3.0f * sinf(k * 2.1f));
      float spark_alpha = 0.1f + 0.1f * sinf(menu_time * 1.5f + k);
      
      if (k % 2 == 0) {
          trackrs::draw::color(1.0f, 0.65f, 0.15f, spark_alpha);
      } else {
          trackrs::draw::color(0.8f, 0.9f, 1.0f, spark_alpha);
      }
      
      trackrs::draw::vertex(x - length, y + size * 0.5f);
      trackrs::draw::vertex(x + length, y + size * 0.5f);
      trackrs::draw::vertex(x + length, y - size * 0.5f);
      trackrs::draw::vertex(x - length, y - size * 0.5f);
  }
  trackrs::draw::end();

  trackrs::draw::enable(GL_TEXTURE_2D);


  trackrs::draw::matrix(trackrs::draw::Projection);
  trackrs::draw::pop();

  trackrs::draw::matrix(trackrs::draw::View);
  // draw GUI

  trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.2f);

  tex_fontSourceCodeOutlined->bind();

  trackrs::draw::push(); // 0

  gui.render();

  trackrs::draw::pop(); // 0

  trackrs::draw::blend(GL_ONE, GL_ZERO);
  trackrs::draw::enable(GL_DEPTH_TEST);
  trackrs::draw::enable(trackrs::draw::Fog);
  trackrs::draw::enable(trackrs::draw::Lighting);

  trackrs::draw::matrix(trackrs::draw::Projection);
  trackrs::draw::pop();
  trackrs::draw::matrix(trackrs::draw::View);
}

/// @see GuiWidgetColors
#define LIST_OF_GUIWIDGETCOLORS_FIELDS  \
    X(normal)                           \
    X(click)                            \
    X(hover)                            \
    X(listnormal)                       \
    X(listclick)                        \
    X(listhover)                        \
    X(weak)                             \
    X(strong)                           \
    X(marked)                           \
    X(header)                           \
    X(bnormal)                          \
    X(bclick)                           \
    X(bhover)

///
/// @brief Loads the widget colors from the specified filename.
/// @todo Should check `sscanf()` calls for success.
/// @param [in] filename    Name of the XML file to be read.
/// @returns Whether or not the operation was successful.
/// @retval true            The colors were read successfully.
/// @retval false           Some (or none) colors could not be read.
///
bool Gui::loadColors(const std::string &filename)
{
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
        PUtil::outLog() << "Loading GUI colors from \"" << filename << "\"\n";

    XMLDocument xmlfile;
    XMLElement *rootelem = PUtil::loadRootElement(xmlfile, filename, "menucolors");

    if (rootelem == nullptr)
        return false;

    bool r = true;
    const char *val;

#define X(ColorField) \
    val = rootelem->Attribute(#ColorField); \
    if (val != nullptr) \
        sscanf(val, "%f, %f, %f, %f", &colors.ColorField.x, &colors.ColorField.y, &colors.ColorField.z, &colors.ColorField.w); \
    else \
        r = false;

    LIST_OF_GUIWIDGETCOLORS_FIELDS

#undef X
    return r;
}

///
/// @brief GUI tick
///

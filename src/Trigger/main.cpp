#include "engine/renderer/renderer.h"
#include "engine/world/state.h"
#include "engine/car/car_renderer.h"
#include "../../engine/application/edition_loader.h"
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "exception.h"
#include "main.h"
#include "physfs_utils.h"
#include "vehicle.h"
#include "validation.h"

#include <SDL2/SDL_main.h>
#include <SDL2/SDL_thread.h>

#include <cctype>
#include <regex>
#include <fstream>
#include <cstdlib>
#include <filesystem>

void MainApp::config()
{
    PUtil::setDebugLevel(DEBUGLEVEL_DEVELOPER);// serve pour 

    cfg.loadConfig();
    if(!validation::run.options.quality.empty())cfg.setRenderQuality(trackrs::parseQuality(validation::run.options.quality));
    if(!validation::run.options.renderer.empty())cfg.setPbrTerrain(validation::run.options.renderer=="pbr");
    if(!validation::run.options.timeOfDay.empty())cfg.setTimeOfDay(validation::run.options.timeOfDay);
    if(!validation::run.options.shadows.empty())cfg.setCsm(validation::run.options.shadows=="csm");
    if(!validation::run.options.crowd.empty())cfg.setCrowd(validation::run.options.crowd=="on");
    if(!validation::run.options.cars.empty())cfg.setCarRenderer(validation::run.options.cars=="on");
    if(!validation::run.options.post.empty())cfg.setPostProcessing(validation::run.options.post=="on");
    if(!validation::run.options.aa.empty())cfg.setTemporalAA(validation::run.options.aa=="taa");
    if(!validation::run.options.weather.empty())cfg.setWeatherPreset(validation::run.options.weather);
    if(!validation::run.options.vegetation.empty())cfg.setVegetationMeshes(validation::run.options.vegetation=="3d");
    setTextureAnisotropy(cfg.getAnisotropy());
    setScreenMode(cfg.getVideoCx(), cfg.getVideoCy(), cfg.getVideoFullscreen());
    if (validation::run.options.enabled) {
        automaticVideoMode(false);
        setScreenMode(static_cast<int>(validation::run.options.width), static_cast<int>(validation::run.options.height), false);
        setStereoMode(StereoNone);
    }
    calcScreenRatios();
    if (validation::run.options.enabled && !validation::run.options.data.empty() &&
        !PHYSFS_mount(validation::run.options.data.c_str(), nullptr, 0))
        throw MakePException("Cannot mount validation --data-dir");

    if (cfg.getDatadirs().empty())
        throw MakePException("Data directory paths are empty: check your trackrs.config file.");

    for (const std::string &configured: cfg.getDatadirs()) {
        const auto path = std::filesystem::path(configured);
        const std::string datadir = (path.is_absolute() ? path : std::filesystem::path(PHYSFS_getBaseDir()) / path).lexically_normal().string();
        if (PHYSFS_mount(datadir.c_str(), NULL, 1) == 0)
        {
            PUtil::outLog() << "Failed to add PhysFS search directory \"" << datadir << "\"" << std::endl
                << "PhysFS: " << physfs_getErrorString() << std::endl;
        }
        else
        {
            PUtil::outLog() << "Main game data directory datadir=\"" << datadir << "\"" << std::endl;
            break;
        }

    }
#ifdef TRACKRS_BUILD_DATA_DIR
    // An existing 0.6.x profile may still use "../data". CMake puts the binary
    // deeper in the build tree, so use source content only if no valid pack mounted.
    if (!PHYSFS_exists("/textures/splash/loading.png") &&
        !PHYSFS_mount(TRACKRS_BUILD_DATA_DIR, nullptr, 1))
        throw MakePException("Cannot mount CMake build-tree data directory");
#endif

    std::unique_ptr<PHYSFS_File, decltype(&PHYSFS_close)> policyFile(
        PHYSFS_openRead("/policy/edition.json"), &PHYSFS_close);
    if (!policyFile) throw MakePException("Missing edition policy");
    const auto policyLength = PHYSFS_fileLength(policyFile.get());
    if (policyLength < 0 || policyLength > 65536) throw MakePException("Invalid edition policy size");
    std::string policyText(static_cast<std::size_t>(policyLength), '\0');
    if (PHYSFS_readBytes(policyFile.get(), policyText.data(), policyLength) != policyLength)
        throw MakePException("Cannot read edition policy");
    editionPolicy = trackrs::loadEditionPolicy(policyText);
    loadPacks();

    if (cfg.getCopydefplayers())
        copyDefaultPlayers();

    best_times.loadAllTimes();
    player_unlocks = best_times.getUnlockData();

#ifndef NDEBUG
    PUtil::outLog() << "Player \"" << cfg.getPlayername() << "\" unlocks:\n";

    for (const auto &s: player_unlocks)
        PUtil::outLog() << '\t' << s << '\n';
#endif
}

void MainApp::load()
{
  psys_dirt = nullptr;

  audinst_engine = nullptr;
  audinst_wind = nullptr;
  audinst_gravel = nullptr;
  audinst_intro = nullptr;
  aud_intro = nullptr;
  menu_time = 0.0f;
  game = nullptr;

  // use PUtil, not boost
  //if (!(tex_splash_screen = getSSTexture().loadTexture(buff))) return false;

  if (!(tex_loading_screen = getSSTexture().loadTexture("/textures/splash/loading.png")))
    throw MakePException("Failed to load the Loading screen");

  if (!(tex_splash_screen = getSSTexture().loadTexture("/textures/splash/splash.jpg")))
    throw MakePException("Failed to load the Splash screen");

  appstate = AS_LOAD_1;

  loadscreencount = 3;

  splashtimeout = 0.0f;

  // Check that controls are available where requested
  // (can't be done in config because joy info not available)

  for (int i = 0; i < PConfig::ActionCount; i++) {

    switch(cfg.getCtrl().map[i].type) {
    case PConfig::UserControl::TypeUnassigned:
      break;

    case PConfig::UserControl::TypeKey:
      if (cfg.getCtrl().map[i].key.sym <= 0 /* || ctrl.map[i].key.sym >= SDLK_LAST */) // `SDLK_LAST` unavailable in SDL2
        cfg.getCtrl().map[i].type = PConfig::UserControl::TypeUnassigned;
      break;

    case PConfig::UserControl::TypeJoyButton:
      if (0 >= getNumJoysticks() || cfg.getCtrl().map[i].joybutton.button >= getJoyNumButtons(0))
        cfg.getCtrl().map[i].type = PConfig::UserControl::TypeUnassigned;
      break;

    case PConfig::UserControl::TypeJoyAxis:
      if (0 >= getNumJoysticks() || cfg.getCtrl().map[i].joyaxis.axis >= getJoyNumAxes(0))
        cfg.getCtrl().map[i].type = PConfig::UserControl::TypeUnassigned;
      break;
    }
  }
}

///
/// @brief Copies default players from data to user directory.
///
void MainApp::copyDefaultPlayers() const
{
    const std::string dppsearchdir = "/defplayers"; // Default Player Profiles Search Directory
    const std::string dppdestdir = "/players"; // Default Player Profiles Destination Directory

    char **rc = PHYSFS_enumerateFiles(dppsearchdir.c_str());

    for (char **fname = rc; *fname != nullptr; ++fname)
    {
        // reject files that are already in the user directory
        if (PHYSFS_exists((dppdestdir + '/' + *fname).c_str()))
        {
            PUtil::outLog() << "Skipping copy of default player \"" << *fname << "\"" << std::endl;
            continue;
        }

        // reject files without .PLAYER extension (lowercase)
        std::smatch mr; // Match Results
        std::regex pat(R"(^([\s\w]+)(\.player)$)"); // Pattern
        std::string fn(*fname); // Filename

        if (!std::regex_search(fn, mr, pat))
            continue;

        if (!PUtil::copyFile(dppsearchdir + '/' + *fname, dppdestdir + '/' + *fname))
            PUtil::outLog() << "Couldn't copy default player \"" << *fname << "\"" << std::endl;
    }

    PHYSFS_freeList(rc);
}

///
/// @brief Return volume of co-driver voice
/// @return Volume from 0.0 to 1.0
///
float MainApp::getCodriverVolume() const
{
    return cfg.getVolumeCodriver();
}

///
/// @brief Return co-driver signs visual settings
/// @return Data structure with settings data
///
PCodriverUserConfig MainApp::getCodriverUserConfig() const
{
    return cfg.getCodriveruserconfig();
}

///
/// @brief Returns event that unlocks the vehicle
/// @param [in] vehiclename  Vehicle name
/// @returns Event name
///
std::string MainApp::getVehicleUnlockEvent(const std::string &vehiclename) const
{
    for (unsigned int i = 0; i < events.size(); i++) {
        for (UnlockData::const_iterator iter = events[i].unlocks.begin(); iter != events[i].unlocks.end(); ++iter) {
            if (*iter == vehiclename) {
                return events[i].name;
            }
        }
    }
    return std::string();
}

bool MainApp::loadLevel(TriggerLevel &tl)
{
  tl.name = "Untitled";
  tl.description = "(no description)";
  tl.comment = "";
  tl.author = "";
  tl.targettime = "";
  tl.targettimeshort = "";
  tl.targettimefloat = 0.0f;
  tl.tex_minimap = nullptr;
  tl.tex_screenshot = nullptr;

  XMLDocument xmlfile;
  XMLElement *rootelem = PUtil::loadRootElement(xmlfile, tl.filename, "level");
  if (!rootelem) {
    PUtil::outLog() << "Couldn't read level \"" << tl.filename << "\"" << std::endl;
    return false;
  }

  const char *val;

  val = rootelem->Attribute("name");
  if (val) tl.name = val;

  val = rootelem->Attribute("description");

  if (val != nullptr)
    tl.description = val;

  val = rootelem->Attribute("comment");
  if (val) tl.comment = val;
  val = rootelem->Attribute("author");
  if (val) tl.author = val;

  val = rootelem->Attribute("screenshot");

  if (val != nullptr)
    tl.tex_screenshot = getSSTexture().loadTexture(PUtil::assemblePath(val, tl.filename));

  val = rootelem->Attribute("minimap");

  if (val != nullptr)
    tl.tex_minimap = getSSTexture().loadTexture(PUtil::assemblePath(val, tl.filename));

  for (XMLElement *walk = rootelem->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (!strcmp(walk->Value(), "race")) {
      val = walk->Attribute("targettime");
      if (val)
      {
        tl.targettime = PUtil::formatTime(atof(val));
        tl.targettimeshort = PUtil::formatTimeShort(atof(val));
        tl.targettimefloat = atof(val);
      }
    }
  }

  return true;
}

// M0 pack-lite: mount and validate content packs. Never throws for bad
// content: a broken pack is disabled and reported (log, menu, validation JSON).
void MainApp::loadPacks()
{
    if (!validation::run.options.packRoot.empty() &&
        !PHYSFS_mount(validation::run.options.packRoot.c_str(), "/packs", 0))
        PUtil::outLog() << "Cannot mount --pack-root " << validation::run.options.packRoot << std::endl;
    // Zipped packs: /packs/<id>.trpack is mounted as /packs/<id>/.
    for (const auto &file : PUtil::findFiles("/packs", ".trpack")) {
        const auto name = std::filesystem::path(file).filename().string();
        const char *real = PHYSFS_getRealDir(file.c_str());
        if (!real) continue;
        const auto archive = (std::filesystem::path(real) / file.substr(1)).string();
        const auto mountPoint = "/packs/" + name.substr(0, name.size() - 7);
        if (!PHYSFS_mount(archive.c_str(), mountPoint.c_str(), 1))
            packNotices.push_back("pack archive " + name + " cannot be opened: " + physfs_getErrorString());
    }
    trackrs::content::Source source;
    source.list = [](const std::string &dir) {
        std::vector<std::string> out;
        char **entries = PHYSFS_enumerateFiles(dir.c_str());
        if (!entries) return out;
        for (char **e = entries; *e; ++e) {
            const std::string name = *e;
            if (name.size() > 7 && name.compare(name.size() - 7, 7, ".trpack") == 0) continue;
            const auto path = dir + "/" + name;
            PHYSFS_Stat stat{};
            const bool directory = PHYSFS_stat(path.c_str(), &stat) && stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
            out.push_back(name + (directory ? "/" : ""));
        }
        PHYSFS_freeList(entries);
        return out;
    };
    source.read = [](const std::string &path) -> std::optional<std::string> {
        std::unique_ptr<PHYSFS_File, decltype(&PHYSFS_close)> f(PHYSFS_openRead(path.c_str()), &PHYSFS_close);
        if (!f) return std::nullopt;
        const auto length = PHYSFS_fileLength(f.get());
        if (length < 0 || length > 16 * 1024 * 1024) return std::nullopt;
        std::string text(static_cast<std::size_t>(length), '\0');
        if (PHYSFS_readBytes(f.get(), text.data(), length) != length) return std::nullopt;
        return text;
    };
    trackrs::content::DiscoverOptions options;
    options.edition = editionPolicy.edition == trackrs::EditionPolicy::Edition::Demo ? trackrs::content::Edition::Demo
                    : editionPolicy.edition == trackrs::EditionPolicy::Edition::Dev ? trackrs::content::Edition::Dev
                    : trackrs::content::Edition::Full;
    options.demoPacks.assign(editionPolicy.demoPacks.begin(), editionPolicy.demoPacks.end());
    options.demoPacks.push_back("classic-legacy-stages");
    if (validation::run.options.enabled) options.edition = trackrs::content::Edition::Dev; // harness sees test stages
    packCatalog = trackrs::content::discover(source, options);
    for (const auto &pack : packCatalog.packs) {
        if (pack.id == "classic-legacy-stages") continue;
        if (pack.enabled) {
            PUtil::outLog() << "Content pack " << pack.id << " " << pack.manifest.version.str() << ": "
                            << pack.stages.size() << " stage(s), " << pack.championships.size() << " championship(s)" << std::endl;
        } else if (pack.disabledReason != "not part of the demo edition") {
            packNotices.push_back("pack " + pack.id + " disabled: " + pack.disabledReason);
            PUtil::outLog() << "Content pack " << pack.id << " DISABLED: " << pack.disabledReason << std::endl;
            for (const auto &issue : pack.issues) PUtil::outLog() << "  " << issue.message << std::endl;
        }
    }
    if (validation::run.options.enabled && !validation::run.options.json.empty())
        std::ofstream(validation::run.options.json + ".packs.json") << trackrs::content::summaryJson(packCatalog) << '\n';
}

bool MainApp::loadLevelsAndEvents()
{
  PUtil::outLog() << "Loading levels and events" << std::endl;

  // Find levels

  std::list<std::string> results = PUtil::findFiles("/maps", ".level");

  for (std::list<std::string>::iterator i = results.begin();
    i != results.end(); ++i) {

    TriggerLevel tl;
    tl.filename = *i;

    if (!loadLevel(tl)) continue;

    // Insert level in alphabetical order
    std::vector<TriggerLevel>::iterator j = levels.begin();
    while (j != levels.end() && j->name < tl.name) ++j;
    levels.insert(j, tl);
  }

  // Content-pack stages (M0): loaded by the same legacy level loader, so a
  // pack stage behaves exactly like a /maps stage once listed.
  for (const auto &pack : packCatalog.packs) {
    if (!pack.enabled || pack.id == "classic-legacy-stages") continue;
    for (const auto &stage : pack.stages) {
      TriggerLevel tl;
      tl.filename = pack.root + "/stages/" + stage.id + "/" + stage.level;
      if (!loadLevel(tl)) {
        packNotices.push_back("stage " + pack.id + ":" + stage.id + " failed to load");
        continue;
      }
      tl.name = stage.name;
      if (editionPolicy.edition == trackrs::EditionPolicy::Edition::Demo) editionPolicy.demoStages.insert(tl.filename);
      std::vector<TriggerLevel>::iterator j = levels.begin();
      while (j != levels.end() && j->name < tl.name) ++j;
      levels.insert(j, tl);
    }
  }
  if (validation::run.options.enabled && validation::run.options.stage.find(':') != std::string::npos) {
    const auto path = packCatalog.levelPath(validation::run.options.stage);
    if (!path) {
      PUtil::outLog() << "Unknown or disabled pack stage " << validation::run.options.stage << std::endl;
      return false;
    }
    validation::run.options.stage = *path;
  }

  // Find events

  results = PUtil::findFiles("/events", ".event");

  for (std::list<std::string>::iterator i = results.begin();
    i != results.end(); ++i) {

    TriggerEvent te;

    te.filename = *i;

    XMLDocument xmlfile;
    XMLElement *rootelem = PUtil::loadRootElement(xmlfile, *i, "event");
    if (!rootelem) {
      PUtil::outLog() << "Couldn't read event \"" << *i << "\"" << std::endl;
      continue;
    }

    const char *val;

    val = rootelem->Attribute("name");
    if (val) te.name = val;
    val = rootelem->Attribute("comment");
    if (val) te.comment = val;
    val = rootelem->Attribute("author");
    if (val) te.author = val;

    val = rootelem->Attribute("locked");

    if (val != nullptr && strcmp(val, "yes") == 0)
        te.locked = true;
    else
        te.locked = false; // FIXME: redundant but clearer?

    float evtotaltime = 0.0f;

    for (XMLElement *walk = rootelem->FirstChildElement();
      walk; walk = walk->NextSiblingElement()) {

      if (strcmp(walk->Value(), "unlocks") == 0)
      {
          val = walk->Attribute("file");

          if (val == nullptr)
          {
              PUtil::outLog() << "Warning: Event has empty unlock" << std::endl;
              continue;
          }

          te.unlocks.insert(val);
      }
      else
      if (!strcmp(walk->Value(), "level")) {

        TriggerLevel tl;

        val = walk->Attribute("file");
        if (!val) {
          PUtil::outLog() << "Warning: Event level has no filename" << std::endl;
          continue;
        }
        tl.filename = PUtil::assemblePath(val, *i);

        if (loadLevel(tl))
        {
          te.levels.push_back(tl);
          evtotaltime += tl.targettimefloat;
        }

        PUtil::outLog() << tl.filename << std::endl;
      }
    }

    if (te.levels.size() <= 0) {
      PUtil::outLog() << "Warning: Event has no levels" << std::endl;
      continue;
    }

    te.totaltime = PUtil::formatTimeShort(evtotaltime);

    // Insert event in alphabetical order
    std::vector<TriggerEvent>::iterator j = events.begin();
    while (j != events.end() && j->name < te.name) ++j;
    events.insert(j, te);
  }

  return true;
}

///
/// @TODO: should also load all vehicles here, then if needed filter which
///  of them should be made available to the player -- it makes no sense
///  to reload vehicles for each race, over and over again
///
bool MainApp::loadAll()
{
  if (!(tex_fontSourceCodeBold = getSSTexture().loadTexture("/textures/font-SourceCodeProBold.png")))
    return false;

  if (!(tex_fontSourceCodeOutlined = getSSTexture().loadTexture("/textures/font-SourceCodeProBoldOutlined.png")))
    return false;

  if (!(tex_fontSourceCodeShadowed = getSSTexture().loadTexture("/textures/font-SourceCodeProBoldShadowed.png")))
    return false;

  if (!(tex_end_screen = getSSTexture().loadTexture("/textures/splash/endgame.jpg"))) return false;

  if (!(tex_hud_life = getSSTexture().loadTexture("/textures/life_helmet.png"))) return false;

  if (!(tex_detail = getSSTexture().loadTexture("/textures/detail.jpg"))) return false;
  if (!(tex_dirt = getSSTexture().loadTexture("/textures/dust.png"))) return false;
  if (!(tex_shadow = getSSTexture().loadTexture("/textures/shadow.png", true, true))) return false;

  if (!(tex_hud_revneedle = getSSTexture().loadTexture("/textures/rev_needle.png"))) return false;

  if (!(tex_hud_revs = getSSTexture().loadTexture("/textures/dial_rev.png"))) return false;

  if (!(tex_hud_offroad = getSSTexture().loadTexture("/textures/offroad.png"))) return false;

  if (!(tex_race_no_screenshot = getSSTexture().loadTexture("/textures/no_screenshot.png"))) return false;

  if (!(tex_race_no_minimap = getSSTexture().loadTexture("/textures/no_minimap.png"))) return false;

  if (!(tex_button_next = getSSTexture().loadTexture("/textures/button_next.png"))) return false;
  if (!(tex_button_prev = getSSTexture().loadTexture("/textures/button_prev.png"))) return false;

  if (!(tex_waterdefault = getSSTexture().loadTexture("/textures/water/default.png"))) return false;

  if (!(tex_snowflake = getSSTexture().loadTexture("/textures/snowflake.png"))) return false;

  if (!(tex_damage_front_left = getSSTexture().loadTexture("/textures/damage_front_left.png"))) return false;
  if (!(tex_damage_front_right = getSSTexture().loadTexture("/textures/damage_front_right.png"))) return false;
  if (!(tex_damage_rear_left = getSSTexture().loadTexture("/textures/damage_rear_left.png"))) return false;
  if (!(tex_damage_rear_right = getSSTexture().loadTexture("/textures/damage_rear_right.png"))) return false;

  loadCodriversigns();

  if (cfg.getEnableSound()) {
    if (!(aud_engine = getSSAudio().loadSample("/sounds/engine.wav", false))) return false;
    if (!(aud_wind = getSSAudio().loadSample("/sounds/wind.wav", false))) return false;
    if (!(aud_shiftup = getSSAudio().loadSample("/sounds/shiftup.wav", false))) return false;
    if (!(aud_shiftdown = getSSAudio().loadSample("/sounds/shiftdown.wav", false))) return false;
    if (!(aud_gravel = getSSAudio().loadSample("/sounds/gravel.wav", false))) return false;
    if (!(aud_crash1 = getSSAudio().loadSample("/sounds/bang.wav", false))) return false;
    if (!(aud_intro = getSSAudio().loadSample("/sounds/intro.wav", false))) return false;

    loadCodrivername();

    audinst_intro = std::make_unique<PAudioInstance>(aud_intro, true);
    audinst_intro->setGain(0.5f * cfg.getVolumeSfx());
    audinst_intro->play();
  }

  if (!gui.loadColors("/menu.colors"))
    PUtil::outLog() << "Couldn't load (all) menu colors, continuing with defaults" << std::endl;

  if (!loadLevelsAndEvents()) {
    PUtil::outLog() << "Couldn't load levels/events" << std::endl;
    return false;
  }

  //quatf tempo;
  //tempo.fromThreeAxisAngle(vec3f(-0.38, -0.38, 0.0));
  //vehic->getBody().setOrientation(tempo);

  campos = campos_prev = vec3f(-15.0,0.0,30.0);
  //camori.fromThreeAxisAngle(vec3f(-1.0,0.0,1.5));
  camori = quatf::identity();

  camvel = vec3f::zero();

  cloudscroll = 0.0f;

  cprotate = 0.0f;

  cameraview = CameraMode::chase;
  tripMetres = 0;
  tracksideSet = false;
  {
    static const std::pair<const char*, CameraMode> modes[] = {{"chase", CameraMode::chase}, {"bumper", CameraMode::bumper},
        {"side", CameraMode::side}, {"hood", CameraMode::hood}, {"periscope", CameraMode::periscope}, {"cockpit", CameraMode::cockpit},
        {"trackside", CameraMode::trackside}};
    for (const auto& m : modes)
      if (validation::run.options.camera == m.first) cameraview = m.second;
  }
  camera_user_angle = 0.0f;

  showmap = true;

  pauserace = false;

  showui = true;

  showcheckpoint = true;

  crashnoise_timeout = 0.0f;

    if (cfg.getDirteffect())
    {
        psys_dirt = std::make_unique<DirtParticleSystem>();
        psys_dirt->setColorStart(0.5f, 0.4f, 0.2f, 1.0f);
        psys_dirt->setColorEnd(0.5f, 0.4f, 0.2f, 0.0f);
        psys_dirt->setSize(0.1f, 0.5f);
        psys_dirt->setDecay(6.0f);
        psys_dirt->setTexture(tex_dirt);
        psys_dirt->setBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        psys_dirt = nullptr;

  //

  choose_type = 0;

  choose_spin = 0.0f;

  return true;
}

///
/// @brief Load configured set of co-driver signs
///
void MainApp::loadCodriversigns()
{
  if (cfg.getEnableCodriversigns() && !cfg.getCodriversigns().empty())
  {
    const std::string origdir(std::string("/textures/CodriverSigns/") + cfg.getCodriversigns());
    char **rc = PHYSFS_enumerateFiles(origdir.c_str());

    for (char **fname = rc; *fname != nullptr; ++fname)
    {
      PTexture *tex_cdsign = getSSTexture().loadTexture(origdir + '/' + *fname);

      if (tex_cdsign != nullptr) // failed loads are ignored
      {
        // remove the extension from the filename
        std::smatch mr; // Match Results
        std::regex pat(R"(^(\w+)(\..+)$)"); // Pattern
        std::string fn(*fname); // Filename

        if (!std::regex_search(fn, mr, pat))
          continue;

        std::string basefname = mr[1];

        // make the base filename lowercase
        for (char &c: basefname)
          c = std::tolower(static_cast<unsigned char> (c));

        tex_codriversigns[basefname] = tex_cdsign;
        //PUtil::outLog() << "Loaded codriver sign for: \"" << basefname << '"' << std::endl;
      }
    }
    PHYSFS_freeList(rc);
  }
}

///
/// @brief Load configured samples of co-driver voice
///
void MainApp::loadCodrivername()
{
  if (!cfg.getCodrivername().empty() && cfg.getCodrivername() != "mime")
  {
    const std::string origdir(std::string("/sounds/codriver/") + cfg.getCodrivername());
    char **rc = PHYSFS_enumerateFiles(origdir.c_str());

    for (char **fname = rc; *fname != nullptr; ++fname)
    {
      PAudioSample *aud_cdword = getSSAudio().loadSample(origdir + '/' + *fname, false);

      if (aud_cdword != nullptr) // failed loads are ignored
      {
        // remove the extension from the filename
        std::smatch mr; // Match Results
        std::regex pat(R"(^(\w+)(\..+)$)"); // Pattern
        std::string fn(*fname); // Filename

        if (!std::regex_search(fn, mr, pat))
          continue;

        std::string basefname = mr[1];

        // make the base filename lowercase
        for (char &c: basefname)
          c = std::tolower(static_cast<unsigned char> (c));

        aud_codriverwords[basefname] = aud_cdword;
        //PUtil::outLog() << "Loaded codriver word for: \"" << basefname << '"' << std::endl;
      }
    }
    PHYSFS_freeList(rc);
  }
}

void MainApp::reloadAll()
{
  setTextureAnisotropy(cfg.getAnisotropy());
  tex_codriversigns.clear();
  loadCodriversigns();

  aud_codriverwords.clear();
  loadCodrivername();
}

void MainApp::unload()
{
  endGame(Gamefinish::not_finished);

  if (audinst_intro) {
    audinst_intro.reset();
    audinst_intro = nullptr;
  }

  psys_dirt.reset();
}

///
/// @brief Prepare to start a new game (a race)
/// @param filename = filename of the level (track) to load
///
bool MainApp::startGame(const std::string &filename)
{
  if (!validation::run.options.enabled && !editionPolicy.stage(filename)) return false;
  PUtil::outLog() << "Starting level \"" << filename << "\"" << std::endl;

  if (audinst_intro) {
    audinst_intro->stop();
  }

  // mouse is grabbed during the race
  grabMouse(true);

  effectsRng.reseed(trackrs::stageSeed(filename) ^ (validation::run.options.enabled ? validation::run.options.seed : 0), 5);
  // the game
  game = std::make_unique<TriggerGame>(this);

  // load vehicles
  if (!game->loadVehicles())
  {
      PUtil::outLog() << "Error: failed to load vehicles" << std::endl;
      return false;
  }

  // load the vehicle
  if (!game->loadLevel(filename)) {
    PUtil::outLog() << "Error: failed to load level" << std::endl;
    return false;
  }
  // Desert stages (level <weather wind/heat>): long-lived drifting dust plumes.
  if (psys_dirt) {
    const auto &climate = trackrs::world::state().settings.weather;
    const bool desert = climate.wind > 0.f || climate.heat > 0.f || climate.dust > 0.f;
    const float a = glm::radians(climate.windDegrees), speed = 1.5f + 7.0f * climate.wind;
    psys_dirt->wind = desert ? vec3f(std::cos(a) * speed, std::sin(a) * speed, 0.35f) : vec3f::zero();
    psys_dirt->settle = desert ? 2.5f : 25.0f;
    psys_dirt->setDecay(desert ? 1.8f : 6.0f);
    psys_dirt->setSize(desert ? 0.25f : 0.1f, desert ? 2.0f : 0.5f);
    psys_dirt->setColorStart(desert ? 0.62f : 0.5f, desert ? 0.5f : 0.4f, desert ? 0.36f : 0.2f, desert ? 0.18f : 1.0f);
    psys_dirt->setColorEnd(desert ? 0.62f : 0.5f, desert ? 0.5f : 0.4f, desert ? 0.36f : 0.2f, 0.0f);
  }

  // useful datas
  race_data.playername  = cfg.getPlayername(); // TODO: move to a better place
  race_data.mapname     = filename;
  choose_type = 0;

  // if there is more than a vehicle to choose from, choose it
  if (game->vehiclechoices.size() > 1) {
    appstate = AS_CHOOSE_VEHICLE;
  } else {
    game->chooseVehicle(game->vehiclechoices[choose_type]);
    if (cfg.getEnableGhost())
      ghost.recordStart(filename, game->vehiclechoices[choose_type]->getName());

    if (lss.state == AM_TOP_LVL_PREP)
    {
        const float bct = best_times.getBestClassTime(
            filename,
            game->vehicle.front()->type->proper_class);

        if (bct >= 0.0f)
            game->targettime = bct;
    }

    initAudio();
    appstate = AS_IN_GAME;
  }

  // load the sky texture
  tex_sky[0] = nullptr;

  if (game->weather.cloud.texname.length() > 0)
    tex_sky[0] = getSSTexture().loadTexture(game->weather.cloud.texname);

  // if there is none load default
  if (tex_sky[0] == nullptr) {
    tex_sky[0] = getSSTexture().loadTexture("/textures/sky/blue.jpg");

    if (tex_sky[0] == nullptr) tex_sky[0] = tex_detail; // last fallback...
  }

  // load water texture
  tex_water = nullptr;

  if (!game->water.texname.empty())
    tex_water = getSSTexture().loadTexture(game->water.texname);

  // if there is none load water default
  if (tex_water == nullptr)
    tex_water = tex_waterdefault;

  fpstime = 0.0f;
  fpscount = 0;
  fps = 0.0f;

  return true;
}

///
/// @brief Turns game sound effects on or off.
/// @note Codriver voice unaffected.
/// @param to       State to switch to (true is on, false is off).
///
void MainApp::endGame(Gamefinish state)
{
  float coursetime = (state == Gamefinish::not_finished) ? 0.0f :
    game->coursetime + game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

    if (state != Gamefinish::not_finished && lss.state != AM_TOP_EVT_PREP)
    {
        race_data.carname   = game->vehicle.front()->type->proper_name;
        race_data.carclass  = game->vehicle.front()->type->proper_class;
        race_data.totaltime = game->coursetime + game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;
        race_data.maxspeed  = 0.0f; // TODO: measure this too
        //PUtil::outLog() << race_data;
        current_times = best_times.insertAndGetCurrentTimesHL(race_data);
        best_times.skipSavePlayer();

        // show the best times
        if (lss.state == AM_TOP_LVL_PREP)
            lss.state = AM_TOP_LVL_TIMES;
        else
        if (lss.state == AM_TOP_PRAC_SEL_PREP)
            lss.state = AM_TOP_PRAC_TIMES;
    }

  if (cfg.getEnableGhost() && state != Gamefinish::not_finished) {
    ghost.recordStop(race_data.totaltime);
  }

  if (audinst_engine) {
    audinst_engine.reset();
    audinst_engine = nullptr;
  }

  if (audinst_wind) {
    audinst_wind.reset();
    audinst_wind = nullptr;
  }

  if (audinst_gravel) {
    audinst_gravel.reset();
    audinst_gravel = nullptr;
  }

  audinst.clear();

  if (game) {
    game.reset();
  }

  if (audinst_intro) {
    audinst_intro->play();
  }

  if (!validation::run.options.enabled)
    finishRace(state, coursetime);
}

///
/// @brief Calculate screen ratios from the current screen width and height.
/// @details Sets `hratio` and `vratio` member data in accordance to the values of
///  `getWidth()` (screen width) and `getHeight()` (screen height).
///  This data is important for proper scaling on widescreen monitors.
///
void MainApp::calcScreenRatios()
{
    const int cx = getWidth();
    const int cy = getHeight();

    if (cx > cy)
    {
        hratio = static_cast<double> (cx) / cy;
        vratio = 1.0;
    }
    else
    if (cx < cy)
    {
        hratio = 1.0;
        vratio = static_cast<double> (cy) / cx;
    }
    else
    {
        hratio = 1.0;
        vratio = 1.0;
    }
}

void MainApp::tickCalculateFps(float delta)
{
  fpstime += delta;
  fpscount++;
  
  if (fpstime >= 0.1) {
    fps = fpscount / fpstime;
    fpstime = 0.0f;
    fpscount = 0;
  }
}

void MainApp::tickStateGame(float delta)
{
  PVehicle *vehic = game->vehicle[0];

  // Validation runs own their length (--frames): past the finish line the
  // game keeps simulating instead of tearing itself down mid-run.
  if (game->isFinished() && !validation::run.options.enabled)
  {
    endGame(game->getFinishState());
    return;
  }

  cloudscroll = fmodf(cloudscroll + delta * game->weather.cloud.scrollrate, 1.0f);

  cprotate = fmodf(cprotate + delta * 1.0f, 1000.0f);

  // Do input/control processing

  for (int a = 0; a < PConfig::ActionCount; a++) {

    switch(cfg.getCtrl().map[a].type) {
    case PConfig::UserControl::TypeUnassigned:
      break;

    case PConfig::UserControl::TypeKey:
      cfg.getCtrl().map[a].value = keyDown(SDL_GetScancodeFromKey(cfg.getCtrl().map[a].key.sym)) ? 1.0f : 0.0f;
      break;

    case PConfig::UserControl::TypeJoyButton:
      cfg.getCtrl().map[a].value = getJoyButton(0, cfg.getCtrl().map[a].joybutton.button) ? 1.0f : 0.0f;
      break;

    case PConfig::UserControl::TypeJoyAxis:
      cfg.getCtrl().map[a].value = cfg.getCtrl().map[a].joyaxis.sign *
        getJoyAxis(0, cfg.getCtrl().map[a].joyaxis.axis);

      RANGEADJUST(cfg.getCtrl().map[a].value, cfg.getCtrl().map[a].joyaxis.deadzone,
          cfg.getCtrl().map[a].joyaxis.maxrange, 0.0f, 1.0f);

      CLAMP_LOWER(cfg.getCtrl().map[a].value, 0.0f);
      break;
    }
  }

  // Bit of a hack for turning, because you simply can't handle analogue
  // and digital steering the same way, afaics

  if (cfg.getCtrl().map[PConfig::ActionLeft].type == PConfig::UserControl::TypeJoyAxis ||
      cfg.getCtrl().map[PConfig::ActionRight].type == PConfig::UserControl::TypeJoyAxis) {

    // Analogue mode

    vehic->ctrl.turn.z = 0.0f;
    vehic->ctrl.turn.z -= cfg.getCtrl().map[PConfig::ActionLeft].value;
    vehic->ctrl.turn.z += cfg.getCtrl().map[PConfig::ActionRight].value;

  } else {

    // Digital mode

    static float turnaccel = 0.0f;

    if (cfg.getCtrl().map[PConfig::ActionLeft].value > 0.0f) {
      if (turnaccel > -0.0f) turnaccel = -0.0f;
      turnaccel -= 8.0f * delta;
      vehic->ctrl.turn.z += turnaccel * delta;
    } else if (cfg.getCtrl().map[PConfig::ActionRight].value > 0.0f) {
      if (turnaccel < 0.0f) turnaccel = 0.0f;
      turnaccel += 8.0f * delta;
      vehic->ctrl.turn.z += turnaccel * delta;
    } else {
      PULLTOWARD(turnaccel, 0.0f, delta * 5.0f);
      PULLTOWARD(vehic->ctrl.turn.z, 0.0f, delta * 5.0f);
    }
  }

  // Computer aided steering
  if (vehic->forwardspeed > 1.0f)
    vehic->ctrl.turn.z -= vehic->body->getAngularVel().z * cfg.getDrivingassist() / (1.0f + vehic->forwardspeed);


  float throttletarget = 0.0f;
  float braketarget = 0.0f;

  if (cfg.getCtrl().map[PConfig::ActionForward].value > 0.0f) {
    if (vehic->wheel_angvel > -10.0f)
      throttletarget = cfg.getCtrl().map[PConfig::ActionForward].value;
    else
      braketarget = cfg.getCtrl().map[PConfig::ActionForward].value;
  }
  if (cfg.getCtrl().map[PConfig::ActionBack].value > 0.0f) {
    if (vehic->wheel_angvel < 10.0f)
      throttletarget = -cfg.getCtrl().map[PConfig::ActionBack].value;
    else
      braketarget = cfg.getCtrl().map[PConfig::ActionBack].value;
  }

  PULLTOWARD(vehic->ctrl.throttle, throttletarget, delta * 15.0f);
  PULLTOWARD(vehic->ctrl.brake1, braketarget, delta * 25.0f);

  vehic->ctrl.brake2 = cfg.getCtrl().map[PConfig::ActionHandbrake].value;


  //PULLTOWARD(vehic->ctrl.aim.x, 0.0, delta * 2.0);
  //PULLTOWARD(vehic->ctrl.aim.y, 0.0, delta * 2.0);

  if (validation::run.options.enabled) {
      const auto input = validation::inputAt(validation::run.inputs, validation::run.ticks);
      vehic->ctrl.setZero();
      vehic->ctrl.throttle = input.throttle;
      vehic->ctrl.turn.z = input.steering;
      vehic->ctrl.brake1 = input.brake;
      vehic->ctrl.brake2 = input.handbrake;
      // Deterministic autopilot (validation only): pure pursuit along the road
      // centreline traced on the roadmap, a curvature-limited target speed and
      // an optional hard stop. Inputs depend only on the simulated state.
      const auto &opts = validation::run.options;
      if (opts.autopilotKmh > 0.f && game->drivingLine.size() > 1 && game->gamestate != Gamestate::countdown) {
          static std::size_t cursor = 0;
          const auto &line = game->drivingLine;
          const vec3f pos = vehic->body->getPosition();
          const glm::vec2 p(pos.x, pos.y);
          // World direction of the body's local +y (forward) = column 1 of the rotation.
          const auto &orientation = vehic->body->getOrientationMatrix();
          const glm::vec2 forward = glm::normalize(glm::vec2(orientation.row[0].y, orientation.row[1].y) + glm::vec2(1e-6f, 0.f));
          // Nearest centreline point, searched forwards from the last one.
          float best = 1e30f;
          for (std::size_t i = cursor; i < std::min(line.size(), cursor + 60); ++i) {
              const float d = glm::length(glm::vec2(line[i]) - p);
              if (d < best) { best = d; cursor = i; }
          }
          const float speed = std::abs(vehic->forwardspeed);
          const float lookahead = 7.f + 0.75f * speed;
          std::size_t target = cursor;
          for (float run = 0.f; target + 1 < line.size() && run < lookahead; ++target)
              run += glm::length(glm::vec2(line[target + 1]) - glm::vec2(line[target]));
          const glm::vec2 to = glm::vec2(line[target]) - p;
          const float angle = std::atan2(forward.x * to.y - forward.y * to.x, glm::dot(forward, to));
          // Slow down for the sharpest heading change within ~45 m ahead.
          float bend = 0.f, run = 0.f;
          // Heading change between consecutive ~12 m chords (the traced line has
          // small 4 m steps that zig-zag; single segments overstate the bend).
          for (std::size_t i = cursor; i + 6 < line.size() && run < 60.f; ++i) {
              const glm::vec2 a = glm::vec2(line[i + 3]) - glm::vec2(line[i]), b = glm::vec2(line[i + 6]) - glm::vec2(line[i + 3]);
              run += glm::length(glm::vec2(line[i + 1]) - glm::vec2(line[i]));
              bend = std::max(bend, std::abs(std::atan2(a.x * b.y - a.y * b.x, glm::dot(a, b))));
          }
          const float target_ms = std::min(opts.autopilotKmh / 3.6f, 8.f + 4.5f / std::max(bend, 0.05f));
          vehic->ctrl.turn.z = std::clamp(-angle * 2.4f, -1.f, 1.f); // +turn.z steers right
          vehic->ctrl.throttle = std::clamp((target_ms - speed) * 0.6f, 0.f, 1.f);
          vehic->ctrl.brake1 = speed > target_ms + 2.f ? std::clamp((speed - target_ms) * 0.25f, 0.f, 1.f) : 0.f;
      }
      if (opts.brakeFrom > 0 && validation::run.ticks >= opts.brakeFrom) {
          // Hard-braking test. --brake-cycles N: N-1 times brake to 30 km/h and
          // re-accelerate to 70 km/h (steering stays on the autopilot), then a
          // final full stop. Deterministic: depends only on the simulated speed.
          static unsigned stops = 0;
          static bool braking = true;
          const vec3f lv = vehic->body->getLinearVel(); // full speed: the car may slide
          const float kmh = std::sqrt(lv.x * lv.x + lv.y * lv.y + lv.z * lv.z) * 3.6f;
          if (stops + 1 < opts.brakeCycles) {
              if (braking && kmh < 30.f) { braking = false; ++stops; }
              else if (!braking && kmh > 70.f) braking = true;
          } else braking = true;
          vehic->ctrl.throttle = braking ? 0.f : 1.f;
          vehic->ctrl.brake1 = braking ? 1.f : 0.f;
      }
  }
  game->tick(delta);
  if (game->gamestate == Gamestate::racing) tripMetres += std::fabs(vehic->forwardspeed) * delta;
  // Car wear (dust, mud, brake heat) follows the simulation, not the frame rate.
  if (game->terrain)
      for (auto *car : game->vehicle)
          trackrs::Renderer::observeCar(*car, game->terrain->getRoadSurface(car->body->getPosition()), delta);
  if (validation::run.options.brakeHeat > 0 && validation::run.options.brakeFrom > 0 &&
      validation::run.ticks >= validation::run.options.brakeFrom)
      trackrs::car::forceBrakeTemperature(game->vehicle[0], validation::run.options.brakeHeat);
  if (game->terrain)
      for (unsigned i = 0; i < game->vehicle.size(); ++i)
          trackrs::Renderer::observeTracks(*game->vehicle[i], i, *game->terrain);
  if (validation::run.options.enabled) {
      try {
          auto& hash = validation::run.hash;
          const auto p = vehic->body->getPosition();
          const auto q = vehic->body->getOrientation();
          const auto v = vehic->body->getLinearVel();
          const auto w = vehic->body->getAngularVel();
          for (float value : {p.x, p.y, p.z, q.x, q.y, q.z, q.w, v.x, v.y, v.z, w.x, w.y, w.z})
              validation::hashFloat(hash, value);
          if (!validation::run.options.telemetry.empty()) { // calibration: observation only
              static std::ofstream telemetry(validation::run.options.telemetry);
              if (validation::run.ticks == 0) telemetry << "tick,x,y,z,speed_ms,rpm,gear,surface\n";
              telemetry << validation::run.ticks << ',' << p.x << ',' << p.y << ',' << p.z << ','
                        << std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) << ',' << vehic->getEngineRPM() << ','
                        << vehic->getCurrentGear() << ',' << PUtil::getTerrainInfo(game->terrain->getRoadSurface(p)) << '\n';
          }
          if (++validation::run.ticks >= validation::run.options.frames)
              validation::run.complete = true;
      } catch (const std::exception& e) {
          PUtil::outLog() << e.what() << std::endl;
          validation::run.result = 1;
          requestExit();
      }
  }

  // Record ghost car (assumes first vehicle is player vehicle)
  if (cfg.getEnableGhost() && game->vehicle[0]) {
    ghost.recordSample(delta, game->vehicle[0]->part[0]);
  }

    if (cfg.getDirteffect())
    {

#define BRIGHTEN_ADD        0.20f

  for (unsigned int i=0; i<game->vehicle.size(); i++) {
    for (unsigned int j=0; j<game->vehicle[i]->part.size(); j++) {
      //const vec3f bodydirtpos = game->vehicle[i]->part[j].ref_world.getPosition();
      const vec3f bodydirtpos = game->vehicle[i]->body->getPosition();
      const dirtinfo bdi = PUtil::getDirtInfo(game->terrain->getRoadSurface(bodydirtpos));

    if (bdi.startsize >= 0.30f && game->vehicle[i]->forwardspeed > 23.0f)
    {
        if (game->vehicle[i]->canHaveDustTrail())
        {
            const float sizemult = game->vehicle[i]->forwardspeed * 0.035f;
            const vec3f bodydirtvec = {0, 0, 1}; // game->vehicle[i]->body->getLinearVelAtPoint(bodydirtpos);
            vec3f bodydirtcolor = game->terrain->getCmapColor(bodydirtpos);

            bodydirtcolor.x += BRIGHTEN_ADD;
            bodydirtcolor.y += BRIGHTEN_ADD;
            bodydirtcolor.z += BRIGHTEN_ADD;

            CLAMP(bodydirtcolor.x, 0.0f, 1.0f);
            CLAMP(bodydirtcolor.y, 0.0f, 1.0f);
            CLAMP(bodydirtcolor.z, 0.0f, 1.0f);
            psys_dirt->setColorStart(bodydirtcolor.x, bodydirtcolor.y, bodydirtcolor.z, 1.0f);
            psys_dirt->setColorEnd(bodydirtcolor.x, bodydirtcolor.y, bodydirtcolor.z, 0.0f);
            psys_dirt->setSize(bdi.startsize * sizemult, bdi.endsize * sizemult);
            psys_dirt->setDecay(bdi.decay);
            psys_dirt->addParticle(bodydirtpos, bodydirtvec);
        }
    }
    else
      for (unsigned int k=0; k<game->vehicle[i]->part[j].wheel.size(); k++) {
        if (effectsRng.unit() * 20.0f < game->vehicle[i]->part[j].wheel[k].dirtthrow)
        {
            const vec3f dirtpos = game->vehicle[i]->part[j].wheel[k].dirtthrowpos;
            const vec3f dirtvec = game->vehicle[i]->part[j].wheel[k].dirtthrowvec;
            const dirtinfo di = PUtil::getDirtInfo(game->terrain->getRoadSurface(dirtpos));
            vec3f dirtcolor = game->terrain->getCmapColor(dirtpos);

            dirtcolor.x += BRIGHTEN_ADD;
            dirtcolor.y += BRIGHTEN_ADD;
            dirtcolor.z += BRIGHTEN_ADD;
            CLAMP(dirtcolor.x, 0.0f, 1.0f);
            CLAMP(dirtcolor.y, 0.0f, 1.0f);
            CLAMP(dirtcolor.z, 0.0f, 1.0f);
            psys_dirt->setColorStart(dirtcolor.x, dirtcolor.y, dirtcolor.z, 1.0f);
            psys_dirt->setColorEnd(dirtcolor.x, dirtcolor.y, dirtcolor.z, 0.0f);
            psys_dirt->setSize(di.startsize, di.endsize);
            psys_dirt->setDecay(di.decay);
            psys_dirt->addParticle(dirtpos, dirtvec /*+ vec3f::random(effectsRng) * 10.0f*/);
        }
      }
    }
  }

  #undef BRIGHTEN_ADD

    }

  float angtarg = 0.0f;
  angtarg -= cfg.getCtrl().map[PConfig::ActionCamLeft].value;
  angtarg += cfg.getCtrl().map[PConfig::ActionCamRight].value;
  angtarg *= PI*0.75f;

  PULLTOWARD(camera_user_angle, angtarg, delta * 4.0f);

  quatf tempo;
  //tempo.fromThreeAxisAngle(vec3f(-1.3,0.0,0.0));

  // allow temporary camera view changes for this frame
  CameraMode cameraview_mod = cameraview;

  if (game->gamestate == Gamestate::finished) {
    cameraview_mod = CameraMode::chase;
    static float spinner = 0.0f;
    spinner += 1.4f * delta;
    tempo.fromThreeAxisAngle(vec3f(-PI*0.5f,0.0f,spinner));
  } else {
    tempo.fromThreeAxisAngle(vec3f(-PI*0.5f,0.0f,0.0f));
  }

  renderowncar = (cameraview_mod != CameraMode::hood && cameraview_mod != CameraMode::bumper && cameraview_mod != CameraMode::cockpit);

  campos_prev = campos;

  //PReferenceFrame *rf = &vehic->part[2].ref_world;
  PReferenceFrame *rf = &vehic->getBody();

  vec3f forw = makevec3f(rf->getOrientationMatrix().row[0]);
  float forwangle = atan2(forw.y, forw.x);

  mat44f cammat;

  switch (cameraview_mod) {

	default:
	case CameraMode::chase: {
    quatf temp2;
    temp2.fromZAngle(forwangle + camera_user_angle);

    quatf target = tempo * temp2;

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 3.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(cammat.row[1]) * 1.6f +
      makevec3f(cammat.row[2]) * 5.0f;
    } break;

	case CameraMode::bumper: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->getOrientation();

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 25.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 1.7f +
      makevec3f(rfmat.row[2]) * 0.4f;
    } break;

    // Right wheel
	case CameraMode::side: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->getOrientation();

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    //PULLTOWARD(camori, target, delta * 25.0f);
    camori = target;

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[0]) * 1.1f +
      makevec3f(rfmat.row[1]) * 0.3f +
      makevec3f(rfmat.row[2]) * 0.1f;
    } break;

	case CameraMode::hood: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->getOrientation();

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    //PULLTOWARD(camori, target, delta * 25.0f);
    camori = target;

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 0.50f +
      makevec3f(rfmat.row[2]) * 0.85f;
    } break;

    // Periscope view
	case CameraMode::periscope:{
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);

    quatf target = tempo * temp2 * rf->getOrientation();

    if (target.dot(camori) < 0.0f) target = target * -1.0f;

    PULLTOWARD(camori, target, delta * 25.0f);

    camori.normalize();

    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    //campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
    campos = rf->getPosition() +
      makevec3f(rfmat.row[1]) * 1.7f +
      makevec3f(rfmat.row[2]) * 5.0f;
    } break;

    // Cockpit: the driver's eye, left-hand seat, rigidly attached to the body.
	case CameraMode::cockpit: {
    quatf temp2;
    temp2.fromZAngle(camera_user_angle);
    camori = tempo * temp2 * rf->getOrientation();
    camori.normalize();
    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    campos = rf->getPosition() +
      makevec3f(rfmat.row[0]) * -0.36f +
      makevec3f(rfmat.row[1]) * -0.15f +
      makevec3f(rfmat.row[2]) * 0.62f;
    } break;

    // Trackside: a camera on a tripod at the roadside, 7 m ahead of the car
    // and 4 m to its right, turning to follow it; re-placed ahead once the
    // car is more than 14 m away.
	case CameraMode::trackside: {
    const mat44f &rfmat = rf->getInverseOrientationMatrix();
    const vec3f carpos = rf->getPosition();
    if (!tracksideSet || (carpos - tracksideAnchor).length() > 14.0f) {
      tracksideAnchor = carpos + makevec3f(rfmat.row[1]) * 7.0f + makevec3f(rfmat.row[0]) * 4.0f;
      tracksideAnchor.z = game->terrain->getHeight(tracksideAnchor.x, tracksideAnchor.y) + 1.1f;
      tracksideSet = true;
    }
    const vec3f look = carpos - tracksideAnchor;
    quatf temp2;
    temp2.fromZAngle(std::atan2(look.x, look.y) + camera_user_angle);
    camori = tempo * temp2;
    camori.normalize();
    cammat = camori.getMatrix();
    cammat = cammat.transpose();
    campos = tracksideAnchor;
    } break;

    // Piggyback (fixed chase)
    //
    // TODO: broken because of "world turns upside down" bug
	//		the problem is in noseangle
    /*
	case CameraMode::piggyback:
	{
		vec3f nose = makevec3f(rf->getOrientationMatrix().row[1]);
		float noseangle = atan2(nose.z, nose.y);

		quatf temp2,temp3,temp4;
		temp2.fromZAngle(forwangle + camera_user_angle);
		//temp3.fromXAngle(noseangle);
		temp3.fromXAngle
		(
			atan2
			(
				rf->getWorldToLocPoint(rf->getPosition()).z,
				rf->getWorldToLocPoint(rf->getPosition()).x
				//(rf->getLocToWorldPoint(vec3f(1,0,0))-rf->getPosition()).x,
				//(rf->getLocToWorldPoint(vec3f(0,1,0))-rf->getPosition()).y
			)
		);

		temp4 = temp3;// * temp2;

		quatf target = tempo * temp4;

		if (target.dot(camori) < 0.0f)
			target = target * -1.0f;
		//if (camori.dot(target) < 0.0f) camori = camori * -1.0f;

		PULLTOWARD(camori, target, delta * 3.0f);

		camori.normalize();

		cammat = camori.getMatrix();
		cammat = cammat.transpose();
		//campos = rf->getPosition() + makevec3f(cammat.row[2]) * 100.0;
		campos = rf->getPosition() +
			makevec3f(cammat.row[1]) * 1.6f +
			makevec3f(cammat.row[2]) * 6.5f;
	}
	break;
	*/
  }

  forw = makevec3f(cammat.row[0]);
  camera_angle = atan2(forw.y, forw.x);

  vec2f diff = makevec2f(game->checkpt[vehic->nextcp].pt) - makevec2f(vehic->body->getPosition());
  nextcpangle = -atan2(diff.y, diff.x) - forwangle + PI*0.5f;

  if (cfg.getEnableSound()) {
    SDL_Haptic *haptic = nullptr;

    if (getNumJoysticks() > 0)
      haptic = getJoyHaptic(0);

    audinst_engine->setGain(cfg.getVolumeEngine());
    audinst_engine->setPitch(vehic->getEngineRPM() / 9000.0f);

    float windlevel = fabsf(vehic->forwardspeed) * 0.6f;

    audinst_wind->setGain(windlevel * 0.03f * cfg.getVolumeSfx());
    audinst_wind->setPitch(windlevel * 0.02f + 0.9f);

    float skidlevel = vehic->getSkidLevel();

    audinst_gravel->setGain(skidlevel * 0.1f * cfg.getVolumeSfx());
    audinst_gravel->setPitch(1.0f);//vehic->getEngineRPM() / 7500.0f);

    if(haptic != nullptr && skidlevel > 500.0f)
      SDL_HapticRumblePlay(haptic, skidlevel * 0.0001f, MAX(1000, (unsigned int)(skidlevel * 0.05f)));

    if (vehic->getFlagGearChange()) {
      switch (vehic->iengine.getShiftDirection())
      {
        case 1: // Shift up
        {
            audinst.push_back(std::make_unique<PAudioInstance>(aud_shiftup));
            audinst.back()->setPitch(0.7f + effectsRng.symmetric()*0.02f);
            audinst.back()->setGain(1.0f * cfg.getVolumeSfx());
            audinst.back()->play();
            break;
        }
        case -1: // Shift down
        {
            audinst.push_back(std::make_unique<PAudioInstance>(aud_shiftdown));
            audinst.back()->setPitch(0.8f + effectsRng.symmetric()*0.12f);
            audinst.back()->setGain(1.0f * cfg.getVolumeSfx());
            audinst.back()->play();
            break;
        }
        default: // Shift flag but neither up nor down?
            break;
      }
    }

    if (crashnoise_timeout <= 0.0f) {
      float crashlevel = vehic->getCrashNoiseLevel();
      if (crashlevel > 0.0f) {
        audinst.push_back(std::make_unique<PAudioInstance>(aud_crash1));
        audinst.back()->setPitch(1.0f + effectsRng.symmetric()*0.02f);
        audinst.back()->setGain(logf(1.0f + crashlevel) * cfg.getVolumeSfx());
        audinst.back()->play();

        if (haptic != nullptr)
          SDL_HapticRumblePlay(haptic, crashlevel * 0.2f, MAX(1000, (unsigned int)(crashlevel * 20.0f)));
      }
      crashnoise_timeout = effectsRng.unit() * 0.1f + 0.01f;
    } else {
      crashnoise_timeout -= delta;
    }

    for (unsigned int i=0; i<audinst.size(); i++) {
      if (!audinst[i]->isPlaying()) {
        audinst.erase(audinst.begin() + i);
        i--;
        continue;
      }
    }
  }

  if (psys_dirt != nullptr)
    psys_dirt->tick(delta);

#define RAIN_START_LIFE         0.6f
#define RAIN_POS_RANDOM         15.0f
#define RAIN_VEL_RANDOM         2.0f

  vec3f camvel = (campos - campos_prev) * (1.0f / delta);

  {
  const vec3f def_drop_vect(2.5f,0.0f,17.0f);

  // randomised number of drops calculation
  float numdrops = game->weather.precip.rain * delta;
  int inumdrops = (int)numdrops;
  if (effectsRng.unit() < numdrops - inumdrops) inumdrops++;
  for (int i=0; i<inumdrops; i++) {
    rain.push_back(RainDrop());
    rain.back().drop_pt = vec3f(campos.x,campos.y,0);
    rain.back().drop_pt += camvel * RAIN_START_LIFE;
    rain.back().drop_pt += vec3f::random(effectsRng) * RAIN_POS_RANDOM;
    rain.back().drop_pt.z = game->terrain->getHeight(rain.back().drop_pt.x, rain.back().drop_pt.y);

    if (game->water.enabled && rain.back().drop_pt.z < game->water.height)
        rain.back().drop_pt.z = game->water.height;

    rain.back().drop_vect = def_drop_vect + vec3f::random(effectsRng) * RAIN_VEL_RANDOM;
    rain.back().life = RAIN_START_LIFE;
  }

  // update life and delete dead raindrops
  unsigned int j=0;
  for (unsigned int i = 0; i < rain.size(); i++) {
    if (rain[i].life <= 0.0f) continue;
    rain[j] = rain[i];
    rain[j].prevlife = rain[j].life;
    rain[j].life -= delta;
    if (rain[j].life < 0.0f)
      rain[j].life = 0.0f; // will be deleted next time round
    j++;
  }
  rain.resize(j);
  }

#define SNOWFALL_START_LIFE     6.5f
#define SNOWFALL_POS_RANDOM     110.0f
#define SNOWFALL_VEL_RANDOM     0.8f

  // snowfall logic; this is rain logic CPM'd (Copied, Pasted and Modified) -- A.B.
  {
    const vec3f def_drop_vect(1.3f, 0.0f, 6.0f);

  // randomised number of flakes calculation
  float numflakes = game->weather.precip.snowfall * delta;
  int inumflakes = (int)numflakes;
  if (effectsRng.unit() < numflakes - inumflakes) inumflakes++;
  for (int i=0; i<inumflakes; i++) {
    snowfall.push_back(SnowFlake());
    snowfall.back().drop_pt = vec3f(campos.x,campos.y,0);
    snowfall.back().drop_pt += camvel * SNOWFALL_START_LIFE / 2;
    snowfall.back().drop_pt += vec3f::random(effectsRng) * SNOWFALL_POS_RANDOM;
    snowfall.back().drop_pt.z = game->terrain->getHeight(snowfall.back().drop_pt.x, snowfall.back().drop_pt.y);

    if (game->water.enabled && snowfall.back().drop_pt.z < game->water.height)
        snowfall.back().drop_pt.z = game->water.height;

    snowfall.back().drop_vect = def_drop_vect + vec3f::random(effectsRng) * SNOWFALL_VEL_RANDOM;
    snowfall.back().life = SNOWFALL_START_LIFE * effectsRng.unit();
  }

  // update life and delete dead snowflakes
  unsigned int j=0;
  for (unsigned int i = 0; i < snowfall.size(); i++) {
    if (snowfall[i].life <= 0.0f) continue;
    snowfall[j] = snowfall[i];
    snowfall[j].prevlife = snowfall[j].life;
    snowfall[j].life -= delta;
    if (snowfall[j].life < 0.0f)
      snowfall[j].life = 0.0f; // will be deleted next time round
    j++;
  }
  snowfall.resize(j);
  }

  // update stuff for SSRender

  cam_pos = campos;
  cam_orimat = cammat;
  cam_linvel = camvel;

  tickCalculateFps(delta);
}

// TODO: mark instant events with flags, deal with them in tick()
// this will get rid of the silly doubling up between keyEvent and joyButtonEvent
// and possibly mouseButtonEvent in future

void MainApp::keyEvent(const SDL_KeyboardEvent &ke)
{
  if (ke.type == SDL_KEYDOWN) {

    if (ke.keysym.sym == SDLK_F12) {
      saveScreenshot();
      return;
    }

    switch (appstate) {
    case AS_LOAD_1:
    case AS_LOAD_2:
      // no hitting escape allowed... end screen not loaded!
      return;
    case AS_LOAD_3:
      levelScreenAction(AA_INIT, 0);
      return;
    case AS_LEVEL_SCREEN:
      handleLevelScreenKey(ke);
      return;
    case AS_CHOOSE_VEHICLE:

      if (cfg.getCtrl().map[PConfig::ActionLeft].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionLeft].key.sym == ke.keysym.sym) {
        if (--choose_type < 0)
          choose_type = (int)game->vehiclechoices.size()-1;
        return;
      }
      if ((cfg.getCtrl().map[PConfig::ActionRight].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionRight].key.sym == ke.keysym.sym) ||
        (cfg.getCtrl().map[PConfig::ActionNext].type == PConfig::UserControl::TypeKey &&
            cfg.getCtrl().map[PConfig::ActionNext].key.sym == ke.keysym.sym)) {
        if (++choose_type >= (int)game->vehiclechoices.size())
          choose_type = 0;
        return;
      }

      switch (ke.keysym.sym) {
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
      {
        if (!game->vehiclechoices[choose_type]->getLocked()) {
          initAudio();
          game->chooseVehicle(game->vehiclechoices[choose_type]);
          if (cfg.getEnableGhost())
            ghost.recordStart(race_data.mapname, game->vehiclechoices[choose_type]->getName());

          if (lss.state == AM_TOP_LVL_PREP)
          {
              const float bct = best_times.getBestClassTime(
                  race_data.mapname,
                  game->vehicle.front()->type->proper_class);

              if (bct >= 0.0f)
                  game->targettime = bct;
          }

          appstate = AS_IN_GAME;
          return;
        }
        break;
      }
      case SDLK_ESCAPE:
        endGame(Gamefinish::not_finished);
        return;
      default:
        break;
      }
      break;
    case AS_IN_GAME:

      if (cfg.getCtrl().map[PConfig::ActionRecover].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionRecover].key.sym == ke.keysym.sym) {
        game->vehicle[0]->doReset();
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionRecoverAtCheckpoint].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionRecoverAtCheckpoint].key.sym == ke.keysym.sym)
      {
          game->resetAtCheckpoint(game->vehicle[0]);
          return;
      }
      if (cfg.getCtrl().map[PConfig::ActionCamMode].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionCamMode].key.sym == ke.keysym.sym) {
        cameraview = static_cast<CameraMode>((static_cast<int>(cameraview) + 1) % static_cast<int>(CameraMode::count));
        camera_user_angle = 0.0f;
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionShowMap].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionShowMap].key.sym == ke.keysym.sym) {
        showmap = !showmap;
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionPauseRace].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionPauseRace].key.sym == ke.keysym.sym)
      {
          toggleSounds(pauserace);
          pauserace = !pauserace;
          return;
      }
      if (cfg.getCtrl().map[PConfig::ActionShowUi].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionShowUi].key.sym == ke.keysym.sym) {
        showui = !showui;
        return;
      }

      if (cfg.getCtrl().map[PConfig::ActionShowCheckpoint].type == PConfig::UserControl::TypeKey &&
          cfg.getCtrl().map[PConfig::ActionShowCheckpoint].key.sym == ke.keysym.sym) {
            showcheckpoint = !showcheckpoint;
            return;
      }


      switch (ke.keysym.sym) {
      case SDLK_ESCAPE:
          endGame(game->getFinishState());
          pauserace = false;
/*
          if (game->getFinishState() == GF_PASS)
            endGame(GF_PASS);
          else // GF_FAIL or GF_NOT_FINISHED
            endGame(GF_FAIL);
*/
        return;
      default:
        break;
      }
      break;
    case AS_END_SCREEN:
      requestExit();
      return;
    }

    switch (ke.keysym.sym) {
    case SDLK_ESCAPE:
      quitGame();
      return;
    default:
      break;
    }
  }
}

void MainApp::mouseMoveEvent(int dx, int dy)
{
  //PVehicle *vehic = game->vehicle[0];

  //vehic->ctrl.tank.turret_turn.x += dx * -0.002;
  //vehic->ctrl.tank.turret_turn.y += dy * 0.002;

  //vehic->ctrl.turn.x += dy * 0.005;
  //vehic->ctrl.turn.y += dx * -0.005;

  dy = dy;

  if (appstate == AS_IN_GAME) {
    PVehicle *vehic = game->vehicle[0];
    vehic->ctrl.turn.z += dx * 0.01f;
  }
}

void MainApp::joyButtonEvent(int which, int button, bool down)
{
  if (which == 0 && down) {

    switch (appstate) {
    case AS_CHOOSE_VEHICLE:

      if (cfg.getCtrl().map[PConfig::ActionLeft].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionLeft].joybutton.button == button) {
        if (--choose_type < 0)
          choose_type = (int)game->vehiclechoices.size()-1;
        return;
      }
      if ((cfg.getCtrl().map[PConfig::ActionRight].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionRight].joybutton.button == button) ||
        (cfg.getCtrl().map[PConfig::ActionNext].type == PConfig::UserControl::TypeJoyButton &&
            cfg.getCtrl().map[PConfig::ActionNext].joybutton.button == button)) {
        if (++choose_type >= (int)game->vehiclechoices.size())
          choose_type = 0;
        return;
      }

      break;

    case AS_IN_GAME:

      if (cfg.getCtrl().map[PConfig::ActionRecover].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionRecover].joybutton.button == button) {
        game->vehicle[0]->doReset();
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionRecoverAtCheckpoint].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionRecoverAtCheckpoint].joybutton.button == button)
      {
          game->resetAtCheckpoint(game->vehicle[0]);
          return;
      }
      if (cfg.getCtrl().map[PConfig::ActionCamMode].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionCamMode].joybutton.button == button) {
		cameraview = static_cast<CameraMode>((static_cast<int>(cameraview) + 1) % static_cast<int>(CameraMode::count));
        camera_user_angle = 0.0f;
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionShowMap].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionShowMap].joybutton.button == button) {
        showmap = !showmap;
        return;
      }
      if (cfg.getCtrl().map[PConfig::ActionPauseRace].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionPauseRace].joybutton.button == button)
        {
            toggleSounds(pauserace);
            pauserace = !pauserace;
            return;
        }
      if (cfg.getCtrl().map[PConfig::ActionShowUi].type == PConfig::UserControl::TypeJoyButton &&
          cfg.getCtrl().map[PConfig::ActionShowUi].joybutton.button == button) {
        showui = !showui;
        return;
      }
    }
  }
}

bool MainApp::joyAxisEvent(int which, int axis, float value, bool down)
{
  if (which == 0) {

    switch (appstate) {
    case AS_CHOOSE_VEHICLE:

      if (cfg.getCtrl().map[PConfig::ActionLeft].type == PConfig::UserControl::TypeJoyAxis &&
          cfg.getCtrl().map[PConfig::ActionLeft].joyaxis.axis == axis &&
          cfg.getCtrl().map[PConfig::ActionLeft].joyaxis.sign * value > 0.5) {
        if (!down)
          if (--choose_type < 0)
            choose_type = (int)game->vehiclechoices.size()-1;
        return true;
      }
      else if (cfg.getCtrl().map[PConfig::ActionRight].type == PConfig::UserControl::TypeJoyAxis &&
          cfg.getCtrl().map[PConfig::ActionRight].joyaxis.axis == axis &&
          cfg.getCtrl().map[PConfig::ActionRight].joyaxis.sign * value > 0.5) {
        if (!down)
          if (++choose_type >= (int)game->vehiclechoices.size())
            choose_type = 0;
        return true;
      }
      else if ((cfg.getCtrl().map[PConfig::ActionLeft].type == PConfig::UserControl::TypeJoyAxis &&
          cfg.getCtrl().map[PConfig::ActionLeft].joyaxis.axis == axis &&
          cfg.getCtrl().map[PConfig::ActionLeft].joyaxis.sign * value <= 0.5) ||
        (cfg.getCtrl().map[PConfig::ActionRight].type == PConfig::UserControl::TypeJoyAxis &&
            cfg.getCtrl().map[PConfig::ActionRight].joyaxis.axis == axis &&
            cfg.getCtrl().map[PConfig::ActionRight].joyaxis.sign * value <= 0.5)) {
          return false;
      }

      break;
    }
  }
  return down;
}

float MainApp::getCtrlActionBackValue() {
  return cfg.getCtrl().map[PConfig::ActionBack].value;
}

int MainApp::getVehicleCurrentGear() {
  return game->vehicle.front()->getCurrentGear();
}

int main(int argc, char *argv[])
{
    try {
        validation::run.options = validation::parse(argc, argv);
        if (validation::run.options.help) {
            std::cout << "TrackRS --headless|--benchmark --stage <id|virtual.level> --vehicle <id>\n"
                << "  --data-dir <data directory>\n"
                << "  --frames <250-Hz ticks> --render-from <tick> --seed <integer> --replay <csv>\n"
                << "  --autopilot <km/h> (follow the traced road) --brake-from <tick> (hard braking)\n"
                << "  --camera chase|bumper|side|hood|periscope|cockpit|trackside --brake-cycles <n> (repeated hard stops: 30-70 km/h, then a full stop)\n"
                << "  --dump-state <hash-file> --json <report.json> --screenshot <image.ppm>\n"
                << "  --renderer legacy|pbr --quality low|medium|high|ultra --shadows blob|csm\n"
                << "  --time-of-day dawn|noon|golden-hour|dusk|night|midnight --vegetation 3d|cards\n"
                << "  --weather level|clear|rain|snow|fog --post on|off --aa taa|off --crowd on|off --cars on|off\n"
                << "  --telemetry <csv> (per-tick position, speed, rpm, gear)\n"
                << "Headless mode uses a real OpenGL context; run under Xvfb on headless Linux.\n";
            return 0;
        }
        if (validation::run.options.enabled) {
            validation::run.inputs = validation::readReplay(validation::run.options.replay);
            validation::run.frames.reserve(validation::run.options.frames);
            validation::run.cpu.reserve(validation::run.options.frames);
            validation::run.gpu.reserve(validation::run.options.frames);
            validation::run.draws.reserve(validation::run.options.frames);
            validation::run.triangles.reserve(validation::run.options.frames);
            validation::run.queries.reserve(128);
            for (const char* name : {"terrain", "terrain_probe", "sky", "water", "mesh_submit", "particles", "ui_text", "sky_ibl_bake", "shadow_terrain", "post", "spectators", "shadow_spectators", "props", "shadow_props", "tracks", "cars", "shadow_cars", "vegetation", "vegetation_probe", "shadow_vegetation", "shadow_cascade_0", "shadow_cascade_1", "shadow_cascade_2", "shadow_cascade_3"}) {
                auto& pass = validation::run.passes[name];
                pass.cpu.reserve(validation::run.options.frames * 32);
                pass.gpu.reserve(validation::run.options.frames * 32);
            }
        }
#ifdef SDL_MAIN_HANDLED
        SDL_SetMainReady();
#endif
    const int result = MainApp("TrackRS", ".trackrs").run(argc, argv);
    if (validation::run.options.enabled && (!validation::run.complete || validation::run.result))
        return 1;
    return result;
    } catch (const std::exception& e) {
        std::cerr << "TrackRS: " << e.what() << '\n';
        return 1;
    }
}

# Walkthrough: TrackRS Professional Rebrand & UI/Menu Upgrades

This walkthrough details the major modernization and professional rebranding work completed for the rally game **TrackRS** (formerly *Trigger Rally*).

---

## 1. Key Accomplishments

### 1.1. Professional Rebranding & Attribution
- Renamed the game's executable, configuration folders, application metadata, and build system variables to **TrackRS**.
- Deleted all old copyright statements and header attributions, replacing them with a single developer comment crediting the sole creator: **Saad AIT YAHIA - @github: Saad-programmer**.
- Updated documentation files (`README.txt`, `BUILDING.txt`, etc.), desktop files, configuration templates, and metainfo files.

### 1.2. Synchronous Intro Video
- Copies `inVideo.mp4` to the binary directory.
- Verifies existence of the video at runtime and plays it in fullscreen mode via `ffplay` prior to initializing the main game window.
- Allows skipping the intro video at any time by pressing **ESC** or **Q**.

### 1.3. Loopable Background Music
- Converted `intro.mp3` to `data/sounds/intro.wav` (16-bit PCM Mono) to ensure native OpenAL/ALUT compatibility.
- Bound music playback to the main menu states, starting immediately upon launch, pausing during gameplay, and resuming looping upon race completion or abort.

### 1.4. Modernized Arcade-Style Menu Theme & Displays
- **Ken Burns Animation**: Panning/zooming effects on the background splash screen texture coordinates in `MainApp::renderStateLevel` and `MainApp::renderStateChoose`.
- **Animated Gradient Overlay**: Added a warm pulsing gradient overlay (deep dark navy top to gold/red glowing charcoal bottom) that enhances readability and adds warmth.
- **Retro CRT Scanlines**: Rendered subtle, crawling horizontal scanlines across the menu background.
- **Spark/Dust Particle System**: Implemented a stateless drift particle effect projecting 40 glowing orange sparks and cool white dust streaks across the screen.
- **Interactive Button Micro-Animations**:
  - Hovering over a button shifts it smoothly to the right by up to 12 pixels.
  - A glowing, angled neon orange/red gradient racing stripe/banner slides in behind the hovered option.
  - Double pulsating chevrons (`>` and `<`) appear dynamically sized around centered buttons, and a single chevron on left-aligned buttons.
- **Main Logo Upgrade**: Overrode the default weak "TrackRS" title label to render a massive, bold, pulsing neon orange title with a dark drop shadow and double underline racing stripe.
- **Section Title Underlines**: Placed neat double underlines below all menu section headers.
- **Glassmorphic HUD Panels**: Replaced raw gray untextured graphic widgets with glassmorphic semi-transparent navy containers framed by fine orange-pulsing borders.
- **Neon Orange Color Scheme**: Shifted all menu widget colors to a cohesive arcade palette consisting of clean steel-silver text and high-contrast neon-orange highlights.

---

## 2. Code Modifications

### [main.h](file:///home/alan/Downloads/trigger-rally-code-r1032/src/include/main.h)
```diff:main.h
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "app.h"
#include "codriver.h"
#include "config.h"
#include "control.h"
#include "ghost.h"
#include "hiscore1.h"
#include "option.h"
#include "rigidity.h"
#include "vmath.h"
#include <unordered_map>

// Forward declaration for TriggerGame to use
class MainApp;
class PParticleSystem;
class PSim;
class PTerrain;
class PVehicle;
class PVehicleType;
class PVehicleTypePart;

///
/// @brief This struct stores a checkpoint
/// @details basically just a coordinate
///
struct CheckPoint {
	vec3f pt;

	CheckPoint(const vec3f &_pt) : pt(_pt) { }
};

///
/// @brief Holds codriver checkpoint information.
///
struct CodriverCP
{
	// Where this checkpoint is on the map
    vec3f pt;
    // What the codriver should say.
    std::string notes;

    CodriverCP(const vec3f &pt, const std::string &notes):
        pt(pt),
        notes(notes)
    {
    }
};

///
/// @brief The status of the race ending
///
enum class Gamefinish {
	// race not finished yet
	not_finished,
	// race passed
	pass,
	// race failed
	fail
};

///
/// @brief Current state of the game
///
enum class Gamestate {
	// the few seconds before start
	countdown,
	// during racing
	racing,
	// race ended
	finished
};
///
/// @brief Camera view mode
///
enum class CameraMode{
	chase = 0,
	bumper,
	side,
	hood,
	periscope,
	// disabled
	//piggyback,
	count
};

///
/// @brief class containing information about the race is being played
///
class TriggerGame {

	friend class MainApp;

public:

    // Sets the codriver checkpoints to "ordered" (true) or "free" (false).
    bool cdcheckpt_ordered = false;

private:
	MainApp *app;

	// simulation context
	PSim *sim;

	// current state of the race
	Gamestate gamestate;

	int randomseed;

	// the vehicles
	std::vector<PVehicle *> vehicle;

	// User's vehicle
	PVehicle *uservehicle;

	// the terrain
	PTerrain *terrain;

	// Checkpoints
	std::vector<CheckPoint> checkpt;
	// Codriver checkpoints
	std::vector<CodriverCP> codrivercheckpt;

	int number_of_laps = 1;

	// Codriver voice and sign set
	PCodriverVoice cdvoice;
	PCodriverSigns cdsigns;

	// Rigidity map for foliage and road signs
	PRigidity rigidity;

public:
    const float offroadtime_penalty_multiplier = 2.5f;

    ///
    /// @brief Used for "real-time" counting of seconds, to scare the player.
    /// @details it read offroad time of the user vehicle
    ///
    float getOffroadTime() const;

private:

	// Time passed since the race started
	float coursetime;
	// time variable used for pre and post race (i.e. Countdown)
	float othertime;
	// checkpoint time
	float cptime;
	// the time needed to win
	float targettime;

	// level comment string
	std::string comment;

	vec3f start_pos;
	quatf start_ori;

    // used to reset vehicle at last passed checkpoint
    vec3f lastCkptPos;
    quatf lastCkptOri;

	// Structure that stores the current weather
	struct {
		struct {
			std::string texname;
			float scrollrate;
		} cloud;
		struct {
			vec3f color;
			float density;
			float density_sky;
		} fog;
		struct {
			float rain;
			float snowfall;
		} precip;
	} weather;

    ///
    /// @brief Water information.
    /// @todo Maybe remove defaults, used in `game.cpp`.
    ///
    struct
    {
        bool        enabled     = false;    ///< Enables the water?
        float       height      = 0.0f;     ///< The height of the water.
        std::string texname     = "";       ///< Custom water texture.
        bool        useralpha   = false;    ///< Use alpha provided by user?
        bool        fixedalpha  = false;    ///< Use the same alpha for all the water?
        float       alpha       = 1.0f;     ///< Default user alpha value.
    } water;

public:
	std::vector<PVehicleType *> vehiclechoices;

public:
	TriggerGame(MainApp *parent);
	~TriggerGame();

	void resetAtCheckpoint(PVehicle *veh);

	void renderCodriverSigns();

	bool loadVehicles();

	bool loadLevel(const std::string &filename);

	void chooseVehicle(PVehicleType *type);

	void tick(float delta);

	bool isFinished() const;

	bool isRacing() const;

	Gamefinish getFinishState();
};


#include "menu.h"


#define AS_LOAD_1           1
#define AS_LOAD_2           2
#define AS_LOAD_3           3
#define AS_LEVEL_SCREEN     10
#define AS_CHOOSE_VEHICLE   11
#define AS_IN_GAME          12
#define AS_END_SCREEN       13



struct TriggerLevel {
	std::string filename, name, description, comment, author, targettime, targettimeshort;

	float targettimefloat;

	PTexture *tex_minimap = nullptr;
	PTexture *tex_screenshot = nullptr;
};

///
/// @brief an Event with his levels
///
struct TriggerEvent {
	std::string filename, name, comment, author, totaltime;

	bool locked = false;
	UnlockData unlocks; ///< @see `HiScore1`

	// Note that levels are not linked to... they are
	// stored in the event because an event may have
	// "hidden" levels not otherwise available

	std::vector<TriggerLevel> levels;
};


class DirtParticleSystem : public PParticleSystem {
public:

	void tick(float delta)
	{
		PParticleSystem::tick(delta);

		for (unsigned int i=0; i<part.size(); i++)
		{
			PULLTOWARD(part[i].linvel, vec3f::zero(), delta * 25.0f);
		}
	}
};


struct RainDrop
{
	vec3f drop_pt, drop_vect;
	float life, prevlife;
};

struct SnowFlake
{
    vec3f drop_pt;
    vec3f drop_vect;
    float life;
    float prevlife;
};

///
/// @brief this class is the whole Trigger Rally game. Create a MainApp object is the only thing main() does
///
class MainApp : public PApp {
private:
	int appstate;

	// TODO: use `aspect` instead of these?
	// TODO: type should be GLdouble instead of double
	double hratio; ///< Horizontal ratio.
	double vratio; ///< Vertical ratio.

	UnlockData player_unlocks; ///< Unlocks for current player, see `HiScore1`.

public:
    ///
    /// @brief Checks if the given data was unlocked by the current player.
    /// @param [in] udata       Unlock data to be checked.
    /// @retval true            The player has unlocked the data.
    /// @retval false           The player has not unlocked the data.
    ///
    bool isUnlockedByPlayer(const std::string &udata) const
    {
        return player_unlocks.count(udata) != 0;
    }

    ///
    /// @brief Checks if the given vehicle is locked.
    /// @param [in] vefi        Filename of the vehicle.
    /// @retval true            Vehicle is marked as locked.
    /// @retval false           Vehicle is not marked as locked.
    ///
    bool isVehicleLocked(const std::string &vefi) const
    {
        XMLDocument xmlfile;
        XMLElement *rootelem = PUtil::loadRootElement(xmlfile, vefi, "vehicle");

        if (rootelem == nullptr)
        {
            PUtil::outLog() << "Couldn't read vehicle \"" << vefi << "\"" << std::endl;
            return false;
        }

        const char *val = rootelem->Attribute("locked");

        if (val != nullptr && std::string(val) == "yes")
            return true;

        return false;
    }

private:
	float splashtimeout;

	std::vector<TriggerLevel> levels;
	std::vector<TriggerEvent> events;

	std::string getVehicleUnlockEvent(const std::string &vehiclename) const;

	// for level screen
	Gui gui;
	// for option screen
	POption option;
    // for control screen
    PControl control;

public:
	LevelState lss;

	// for handling configuration
	PConfig cfg;

private:
	HISCORE1_SORT hs_sort_method = HISCORE1_SORT::BY_TOTALTIME_ASC;
	RaceData race_data;
	std::vector<TimeEntry> current_times;

	TriggerGame *game;

	PVehicleType *vt_tank;

	PTexture *tex_fontSourceCodeBold,
			*tex_fontSourceCodeOutlined,
			*tex_fontSourceCodeShadowed;

	PTexture *tex_detail,
			*tex_sky[1],
			*tex_water,
			*tex_waterdefault,
			*tex_dirt,
			*tex_snowflake,
			*tex_shadow,
			*tex_hud_revs,
			*tex_hud_revneedle,
			*tex_hud_life,
			*tex_hud_offroad,
			*tex_loading_screen,
			*tex_splash_screen,
			*tex_end_screen,
			*tex_race_no_screenshot,
			*tex_race_no_minimap,
			*tex_button_next,
			*tex_button_prev,
			*tex_damage_front_left,
			*tex_damage_front_right,
			*tex_damage_rear_left,
			*tex_damage_rear_right;

	std::unordered_map<std::string, PTexture *> tex_codriversigns;
	std::unordered_map<std::string, PAudioSample *> aud_codriverwords;

	DirtParticleSystem *psys_dirt;

	// Tones
	PAudioSample *aud_engine,
				*aud_wind,
				*aud_shiftup,
				*aud_shiftdown,
				*aud_gravel,
				*aud_crash1;

	// Audio instances
	PAudioInstance *audinst_engine, *audinst_wind, *audinst_gravel;

	std::vector<PAudioInstance *> audinst;

	float cloudscroll;

	vec3f campos, campos_prev;
	quatf camori;

	vec3f camvel;

	float nextcpangle;

	float cprotate;

	// what view mode the camera is
	CameraMode cameraview;
	float camera_angle;
	float camera_user_angle;

	// If with the the vehicle should be rendered (depends on cameraview)
	bool renderowncar;

	bool showmap;

	bool pauserace;

	bool showui;

	bool showcheckpoint;

	float crashnoise_timeout;

	std::vector<RainDrop> rain;
	std::vector<SnowFlake> snowfall;

	int loadscreencount;

	float choose_spin;

	int choose_type;

	// Time and count to calculate frames per second
	float fpstime;
	unsigned int fpscount;
	float fps;
	void tickCalculateFps(float delta);

	// Record and display of ghost vehicles
	PGhost ghost;

	void loadCodriversigns();
	void loadCodrivername();
    void reloadAll();

protected:
	void renderWater();
	void renderSky(const mat44f &cammat);

	bool startGame(const std::string &filename);
	void toggleSounds(bool to);
	void initAudio();
	void endGame(Gamefinish state);

	void quitGame()
	{
		endGame(Gamefinish::not_finished);
		splashtimeout = 0.0f;
		appstate = AS_END_SCREEN;
	}

	void levelScreenAction(int action, int index);
	void handleLevelScreenKey(const SDL_KeyboardEvent &ke);
	void finishRace(Gamefinish state, float coursetime);

public:
	MainApp(const std::string &title, const std::string &name) :
            PApp(title, name),
            option(gui, cfg),
            control(gui, cfg),
            cfg(this),
            ghost(0.1f)
	{
	}
	//MainApp::~MainApp(); // should not have destructor, use unload

	float getCodriverVolume() const;

	void config();
	void load();
	void unload();

	void copyDefaultPlayers() const;
	bool loadAll();
	bool loadLevelsAndEvents();
	bool loadLevel(TriggerLevel &tl);

	void calcScreenRatios();

	void tick(float delta);

	void resize();
	void render(float eyetranslation);

	void renderStateLoading(float eyetranslation);
	void renderStateEnd(float eyetranslation);
	void tickStateLevel(float delta);
	void renderStateLevel(float eyetranslation);
	void tickStateChoose(float delta);
	void renderStateChoose(float eyetranslation);
	void tickStateGame(float delta);
	void renderStateGame(float eyetranslation);

	void renderDamageIndicator(
	    const PTexture *texture, float posx, float posy, float scalex, float scaley, float damage);
	void renderDamageIndicatorGroup();
	void renderVehiclePart(const PVehicleType &type, const PVehiclePart &part,
	    const PVehicleTypePart &typepart, float alpha);

	void keyEvent(const SDL_KeyboardEvent &ke);
	void mouseMoveEvent(int dx, int dy);
	void cursorMoveEvent(int posx, int posy);
	void mouseButtonEvent(const SDL_MouseButtonEvent &mbe);
	void joyButtonEvent(int which, int button, bool down);
	bool joyAxisEvent(int which, int axis, float value, bool down);

	float getCtrlActionBackValue();
	int getVehicleCurrentGear();

    std::unordered_map<std::string, PAudioSample *> getCodriverWords() const
    {
        return aud_codriverwords;
    }

    std::unordered_map<std::string, PTexture *> getCodriverSigns() const
    {
        return tex_codriversigns;
    }

    PCodriverUserConfig getCodriverUserConfig() const;
};
===
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "app.h"
#include "codriver.h"
#include "config.h"
#include "control.h"
#include "ghost.h"
#include "hiscore1.h"
#include "option.h"
#include "rigidity.h"
#include "vmath.h"
#include <unordered_map>

// Forward declaration for TriggerGame to use
class MainApp;
class PParticleSystem;
class PSim;
class PTerrain;
class PVehicle;
class PVehicleType;
class PVehicleTypePart;

///
/// @brief This struct stores a checkpoint
/// @details basically just a coordinate
///
struct CheckPoint {
	vec3f pt;

	CheckPoint(const vec3f &_pt) : pt(_pt) { }
};

///
/// @brief Holds codriver checkpoint information.
///
struct CodriverCP
{
	// Where this checkpoint is on the map
    vec3f pt;
    // What the codriver should say.
    std::string notes;

    CodriverCP(const vec3f &pt, const std::string &notes):
        pt(pt),
        notes(notes)
    {
    }
};

///
/// @brief The status of the race ending
///
enum class Gamefinish {
	// race not finished yet
	not_finished,
	// race passed
	pass,
	// race failed
	fail
};

///
/// @brief Current state of the game
///
enum class Gamestate {
	// the few seconds before start
	countdown,
	// during racing
	racing,
	// race ended
	finished
};
///
/// @brief Camera view mode
///
enum class CameraMode{
	chase = 0,
	bumper,
	side,
	hood,
	periscope,
	// disabled
	//piggyback,
	count
};

///
/// @brief class containing information about the race is being played
///
class TriggerGame {

	friend class MainApp;

public:

    // Sets the codriver checkpoints to "ordered" (true) or "free" (false).
    bool cdcheckpt_ordered = false;

private:
	MainApp *app;

	// simulation context
	PSim *sim;

	// current state of the race
	Gamestate gamestate;

	int randomseed;

	// the vehicles
	std::vector<PVehicle *> vehicle;

	// User's vehicle
	PVehicle *uservehicle;

	// the terrain
	PTerrain *terrain;

	// Checkpoints
	std::vector<CheckPoint> checkpt;
	// Codriver checkpoints
	std::vector<CodriverCP> codrivercheckpt;

	int number_of_laps = 1;

	// Codriver voice and sign set
	PCodriverVoice cdvoice;
	PCodriverSigns cdsigns;

	// Rigidity map for foliage and road signs
	PRigidity rigidity;

public:
    const float offroadtime_penalty_multiplier = 2.5f;

    ///
    /// @brief Used for "real-time" counting of seconds, to scare the player.
    /// @details it read offroad time of the user vehicle
    ///
    float getOffroadTime() const;

private:

	// Time passed since the race started
	float coursetime;
	// time variable used for pre and post race (i.e. Countdown)
	float othertime;
	// checkpoint time
	float cptime;
	// the time needed to win
	float targettime;

	// level comment string
	std::string comment;

	vec3f start_pos;
	quatf start_ori;

    // used to reset vehicle at last passed checkpoint
    vec3f lastCkptPos;
    quatf lastCkptOri;

	// Structure that stores the current weather
	struct {
		struct {
			std::string texname;
			float scrollrate;
		} cloud;
		struct {
			vec3f color;
			float density;
			float density_sky;
		} fog;
		struct {
			float rain;
			float snowfall;
		} precip;
	} weather;

    ///
    /// @brief Water information.
    /// @todo Maybe remove defaults, used in `game.cpp`.
    ///
    struct
    {
        bool        enabled     = false;    ///< Enables the water?
        float       height      = 0.0f;     ///< The height of the water.
        std::string texname     = "";       ///< Custom water texture.
        bool        useralpha   = false;    ///< Use alpha provided by user?
        bool        fixedalpha  = false;    ///< Use the same alpha for all the water?
        float       alpha       = 1.0f;     ///< Default user alpha value.
    } water;

public:
	std::vector<PVehicleType *> vehiclechoices;

public:
	TriggerGame(MainApp *parent);
	~TriggerGame();

	void resetAtCheckpoint(PVehicle *veh);

	void renderCodriverSigns();

	bool loadVehicles();

	bool loadLevel(const std::string &filename);

	void chooseVehicle(PVehicleType *type);

	void tick(float delta);

	bool isFinished() const;

	bool isRacing() const;

	Gamefinish getFinishState();
};


#include "menu.h"


#define AS_LOAD_1           1
#define AS_LOAD_2           2
#define AS_LOAD_3           3
#define AS_LEVEL_SCREEN     10
#define AS_CHOOSE_VEHICLE   11
#define AS_IN_GAME          12
#define AS_END_SCREEN       13



struct TriggerLevel {
	std::string filename, name, description, comment, author, targettime, targettimeshort;

	float targettimefloat;

	PTexture *tex_minimap = nullptr;
	PTexture *tex_screenshot = nullptr;
};

///
/// @brief an Event with his levels
///
struct TriggerEvent {
	std::string filename, name, comment, author, totaltime;

	bool locked = false;
	UnlockData unlocks; ///< @see `HiScore1`

	// Note that levels are not linked to... they are
	// stored in the event because an event may have
	// "hidden" levels not otherwise available

	std::vector<TriggerLevel> levels;
};


class DirtParticleSystem : public PParticleSystem {
public:

	void tick(float delta)
	{
		PParticleSystem::tick(delta);

		for (unsigned int i=0; i<part.size(); i++)
		{
			PULLTOWARD(part[i].linvel, vec3f::zero(), delta * 25.0f);
		}
	}
};


struct RainDrop
{
	vec3f drop_pt, drop_vect;
	float life, prevlife;
};

struct SnowFlake
{
    vec3f drop_pt;
    vec3f drop_vect;
    float life;
    float prevlife;
};

///
/// @brief this class is the whole Trigger Rally game. Create a MainApp object is the only thing main() does
///
class MainApp : public PApp {
private:
	int appstate;

	// TODO: use `aspect` instead of these?
	// TODO: type should be GLdouble instead of double
	double hratio; ///< Horizontal ratio.
	double vratio; ///< Vertical ratio.

	UnlockData player_unlocks; ///< Unlocks for current player, see `HiScore1`.

public:
    ///
    /// @brief Checks if the given data was unlocked by the current player.
    /// @param [in] udata       Unlock data to be checked.
    /// @retval true            The player has unlocked the data.
    /// @retval false           The player has not unlocked the data.
    ///
    bool isUnlockedByPlayer(const std::string &udata) const
    {
        return player_unlocks.count(udata) != 0;
    }

    ///
    /// @brief Checks if the given vehicle is locked.
    /// @param [in] vefi        Filename of the vehicle.
    /// @retval true            Vehicle is marked as locked.
    /// @retval false           Vehicle is not marked as locked.
    ///
    bool isVehicleLocked(const std::string &vefi) const
    {
        XMLDocument xmlfile;
        XMLElement *rootelem = PUtil::loadRootElement(xmlfile, vefi, "vehicle");

        if (rootelem == nullptr)
        {
            PUtil::outLog() << "Couldn't read vehicle \"" << vefi << "\"" << std::endl;
            return false;
        }

        const char *val = rootelem->Attribute("locked");

        if (val != nullptr && std::string(val) == "yes")
            return true;

        return false;
    }

private:
	float splashtimeout;

	std::vector<TriggerLevel> levels;
	std::vector<TriggerEvent> events;

	std::string getVehicleUnlockEvent(const std::string &vehiclename) const;

	// for level screen
	Gui gui;
	// for option screen
	POption option;
    // for control screen
    PControl control;

public:
	LevelState lss;

	// for handling configuration
	PConfig cfg;

private:
	HISCORE1_SORT hs_sort_method = HISCORE1_SORT::BY_TOTALTIME_ASC;
	RaceData race_data;
	std::vector<TimeEntry> current_times;

	TriggerGame *game;

	PVehicleType *vt_tank;

	PTexture *tex_fontSourceCodeBold,
			*tex_fontSourceCodeOutlined,
			*tex_fontSourceCodeShadowed;

	PTexture *tex_detail,
			*tex_sky[1],
			*tex_water,
			*tex_waterdefault,
			*tex_dirt,
			*tex_snowflake,
			*tex_shadow,
			*tex_hud_revs,
			*tex_hud_revneedle,
			*tex_hud_life,
			*tex_hud_offroad,
			*tex_loading_screen,
			*tex_splash_screen,
			*tex_end_screen,
			*tex_race_no_screenshot,
			*tex_race_no_minimap,
			*tex_button_next,
			*tex_button_prev,
			*tex_damage_front_left,
			*tex_damage_front_right,
			*tex_damage_rear_left,
			*tex_damage_rear_right;

	std::unordered_map<std::string, PTexture *> tex_codriversigns;
	std::unordered_map<std::string, PAudioSample *> aud_codriverwords;

	DirtParticleSystem *psys_dirt;

	// Tones
	PAudioSample *aud_engine,
				*aud_wind,
				*aud_shiftup,
				*aud_shiftdown,
				*aud_gravel,
				*aud_crash1,
				*aud_intro;

	// Audio instances
	PAudioInstance *audinst_engine, *audinst_wind, *audinst_gravel, *audinst_intro;

	std::vector<PAudioInstance *> audinst;

	float cloudscroll;

	vec3f campos, campos_prev;
	quatf camori;

	vec3f camvel;

	float nextcpangle;

	float cprotate;

	// what view mode the camera is
	CameraMode cameraview;
	float camera_angle;
	float camera_user_angle;

	// If with the the vehicle should be rendered (depends on cameraview)
	bool renderowncar;

	bool showmap;

	bool pauserace;

	bool showui;

	bool showcheckpoint;

	float crashnoise_timeout;

	std::vector<RainDrop> rain;
	std::vector<SnowFlake> snowfall;

	int loadscreencount;

	float choose_spin;
	float menu_time;

	int choose_type;

	// Time and count to calculate frames per second
	float fpstime;
	unsigned int fpscount;
	float fps;
	void tickCalculateFps(float delta);

	// Record and display of ghost vehicles
	PGhost ghost;

	void loadCodriversigns();
	void loadCodrivername();
    void reloadAll();

protected:
	void renderWater();
	void renderSky(const mat44f &cammat);

	bool startGame(const std::string &filename);
	void toggleSounds(bool to);
	void initAudio();
	void endGame(Gamefinish state);

	void quitGame()
	{
		endGame(Gamefinish::not_finished);
		splashtimeout = 0.0f;
		appstate = AS_END_SCREEN;
	}

	void levelScreenAction(int action, int index);
	void handleLevelScreenKey(const SDL_KeyboardEvent &ke);
	void finishRace(Gamefinish state, float coursetime);

public:
	MainApp(const std::string &title, const std::string &name) :
            PApp(title, name),
            option(gui, cfg),
            control(gui, cfg),
            cfg(this),
            ghost(0.1f)
	{
	}
	//MainApp::~MainApp(); // should not have destructor, use unload

	float getCodriverVolume() const;

	void config();
	void load();
	void unload();

	void copyDefaultPlayers() const;
	bool loadAll();
	bool loadLevelsAndEvents();
	bool loadLevel(TriggerLevel &tl);

	void calcScreenRatios();

	void tick(float delta);

	void resize();
	void render(float eyetranslation);

	void renderStateLoading(float eyetranslation);
	void renderStateEnd(float eyetranslation);
	void tickStateLevel(float delta);
	void renderStateLevel(float eyetranslation);
	void tickStateChoose(float delta);
	void renderStateChoose(float eyetranslation);
	void tickStateGame(float delta);
	void renderStateGame(float eyetranslation);

	void renderDamageIndicator(
	    const PTexture *texture, float posx, float posy, float scalex, float scaley, float damage);
	void renderDamageIndicatorGroup();
	void renderVehiclePart(const PVehicleType &type, const PVehiclePart &part,
	    const PVehicleTypePart &typepart, float alpha);

	void keyEvent(const SDL_KeyboardEvent &ke);
	void mouseMoveEvent(int dx, int dy);
	void cursorMoveEvent(int posx, int posy);
	void mouseButtonEvent(const SDL_MouseButtonEvent &mbe);
	void joyButtonEvent(int which, int button, bool down);
	bool joyAxisEvent(int which, int axis, float value, bool down);

	float getCtrlActionBackValue();
	int getVehicleCurrentGear();

    std::unordered_map<std::string, PAudioSample *> getCodriverWords() const
    {
        return aud_codriverwords;
    }

    std::unordered_map<std::string, PTexture *> getCodriverSigns() const
    {
        return tex_codriversigns;
    }

    PCodriverUserConfig getCodriverUserConfig() const;
};
```

### [main.cpp](file:///home/alan/Downloads/trigger-rally-code-r1032/src/Trigger/main.cpp)
```diff:main.cpp
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "exception.h"
#include "main.h"
#include "physfs_utils.h"
#include "vehicle.h"

#include <SDL2/SDL_main.h>
#include <SDL2/SDL_thread.h>

#include <cctype>
#include <regex>

void MainApp::config()
{
    PUtil::setDebugLevel(DEBUGLEVEL_DEVELOPER);// serve pour 

    cfg.loadConfig();
    setScreenMode(cfg.getVideoCx(), cfg.getVideoCy(), cfg.getVideoFullscreen());
    calcScreenRatios();

    if (cfg.getDatadirs().empty())
        throw MakePException("Data directory paths are empty: check your trigger-rally.config file.");

    for (const std::string &datadir: cfg.getDatadirs())
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
  game = nullptr;

  // use PUtil, not boost
  //std::string buff = boost::str(boost::format("textures/splash/splash%u.jpg") % ((rand() % 3) + 1));
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

    loadCodrivername();
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
  camera_user_angle = 0.0f;

  showmap = true;

  pauserace = false;

  showui = true;

  showcheckpoint = true;

  crashnoise_timeout = 0.0f;

    if (cfg.getDirteffect())
    {
        psys_dirt = new DirtParticleSystem();
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
  tex_codriversigns.clear();
  loadCodriversigns();

  aud_codriverwords.clear();
  loadCodrivername();
}

void MainApp::unload()
{
  endGame(Gamefinish::not_finished);

  delete psys_dirt;
}

///
/// @brief Prepare to start a new game (a race)
/// @param filename = filename of the level (track) to load
///
bool MainApp::startGame(const std::string &filename)
{
  PUtil::outLog() << "Starting level \"" << filename << "\"" << std::endl;

  // mouse is grabbed during the race
  grabMouse(true);

  // the game
  game = new TriggerGame(this);

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
    audinst_engine = new PAudioInstance(aud_engine, true);
    audinst_engine->setGain(0.0);
    audinst_engine->play();

	// wind sound
    audinst_wind = new PAudioInstance(aud_wind, true);
    audinst_wind->setGain(0.0);
    audinst_wind->play();

	// terrain sound
    audinst_gravel = new PAudioInstance(aud_gravel, true);
    audinst_gravel->setGain(0.0);
    audinst_gravel->play();
  }
}

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
    delete audinst_engine;
    audinst_engine = nullptr;
  }

  if (audinst_wind) {
    delete audinst_wind;
    audinst_wind = nullptr;
  }

  if (audinst_gravel) {
    delete audinst_gravel;
    audinst_gravel = nullptr;
  }

  for (unsigned int i=0; i<audinst.size(); i++) {
    delete audinst[i];
  }
  audinst.clear();

  if (game) {
    delete game;
    game = nullptr;
  }

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

void MainApp::tick(float delta)
{
    getSSAudio().tick();

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

  if (game->isFinished())
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

  game->tick(delta);

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
        if (rand01 * 20.0f < game->vehicle[i]->part[j].wheel[k].dirtthrow)
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
            psys_dirt->addParticle(dirtpos, dirtvec /*+ vec3f::rand() * 10.0f*/);
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

  renderowncar = (cameraview_mod != CameraMode::hood && cameraview_mod != CameraMode::bumper);

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
            audinst.push_back(new PAudioInstance(aud_shiftup));
            audinst.back()->setPitch(0.7f + randm11*0.02f);
            audinst.back()->setGain(1.0f * cfg.getVolumeSfx());
            audinst.back()->play();
            break;
        }
        case -1: // Shift down
        {
            audinst.push_back(new PAudioInstance(aud_shiftdown));
            audinst.back()->setPitch(0.8f + randm11*0.12f);
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
        audinst.push_back(new PAudioInstance(aud_crash1));
        audinst.back()->setPitch(1.0f + randm11*0.02f);
        audinst.back()->setGain(logf(1.0f + crashlevel) * cfg.getVolumeSfx());
        audinst.back()->play();

        if (haptic != nullptr)
          SDL_HapticRumblePlay(haptic, crashlevel * 0.2f, MAX(1000, (unsigned int)(crashlevel * 20.0f)));
      }
      crashnoise_timeout = rand01 * 0.1f + 0.01f;
    } else {
      crashnoise_timeout -= delta;
    }

    for (unsigned int i=0; i<audinst.size(); i++) {
      if (!audinst[i]->isPlaying()) {
        delete audinst[i];
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
  if (rand01 < numdrops - inumdrops) inumdrops++;
  for (int i=0; i<inumdrops; i++) {
    rain.push_back(RainDrop());
    rain.back().drop_pt = vec3f(campos.x,campos.y,0);
    rain.back().drop_pt += camvel * RAIN_START_LIFE;
    rain.back().drop_pt += vec3f::rand() * RAIN_POS_RANDOM;
    rain.back().drop_pt.z = game->terrain->getHeight(rain.back().drop_pt.x, rain.back().drop_pt.y);

    if (game->water.enabled && rain.back().drop_pt.z < game->water.height)
        rain.back().drop_pt.z = game->water.height;

    rain.back().drop_vect = def_drop_vect + vec3f::rand() * RAIN_VEL_RANDOM;
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
  if (rand01 < numflakes - inumflakes) inumflakes++;
  for (int i=0; i<inumflakes; i++) {
    snowfall.push_back(SnowFlake());
    snowfall.back().drop_pt = vec3f(campos.x,campos.y,0);
    snowfall.back().drop_pt += camvel * SNOWFALL_START_LIFE / 2;
    snowfall.back().drop_pt += vec3f::rand() * SNOWFALL_POS_RANDOM;
    snowfall.back().drop_pt.z = game->terrain->getHeight(snowfall.back().drop_pt.x, snowfall.back().drop_pt.y);

    if (game->water.enabled && snowfall.back().drop_pt.z < game->water.height)
        snowfall.back().drop_pt.z = game->water.height;

    snowfall.back().drop_vect = def_drop_vect + vec3f::rand() * SNOWFALL_VEL_RANDOM;
    snowfall.back().life = SNOWFALL_START_LIFE * rand01;
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
    return MainApp("Trigger Rally", ".trigger-rally").run(argc, argv);
}
===
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "exception.h"
#include "main.h"
#include "physfs_utils.h"
#include "vehicle.h"

#include <SDL2/SDL_main.h>
#include <SDL2/SDL_thread.h>

#include <cctype>
#include <regex>
#include <fstream>
#include <cstdlib>

void MainApp::config()
{
    PUtil::setDebugLevel(DEBUGLEVEL_DEVELOPER);// serve pour 

    cfg.loadConfig();
    setScreenMode(cfg.getVideoCx(), cfg.getVideoCy(), cfg.getVideoFullscreen());
    calcScreenRatios();

    if (cfg.getDatadirs().empty())
        throw MakePException("Data directory paths are empty: check your trackrs.config file.");

    for (const std::string &datadir: cfg.getDatadirs())
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
  //std::string buff = boost::str(boost::format("textures/splash/splash%u.jpg") % ((rand() % 3) + 1));
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

    audinst_intro = new PAudioInstance(aud_intro, true);
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
  camera_user_angle = 0.0f;

  showmap = true;

  pauserace = false;

  showui = true;

  showcheckpoint = true;

  crashnoise_timeout = 0.0f;

    if (cfg.getDirteffect())
    {
        psys_dirt = new DirtParticleSystem();
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
  tex_codriversigns.clear();
  loadCodriversigns();

  aud_codriverwords.clear();
  loadCodrivername();
}

void MainApp::unload()
{
  endGame(Gamefinish::not_finished);

  if (audinst_intro) {
    delete audinst_intro;
    audinst_intro = nullptr;
  }

  delete psys_dirt;
}

///
/// @brief Prepare to start a new game (a race)
/// @param filename = filename of the level (track) to load
///
bool MainApp::startGame(const std::string &filename)
{
  PUtil::outLog() << "Starting level \"" << filename << "\"" << std::endl;

  if (audinst_intro) {
    audinst_intro->stop();
  }

  // mouse is grabbed during the race
  grabMouse(true);

  // the game
  game = new TriggerGame(this);

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
    audinst_engine = new PAudioInstance(aud_engine, true);
    audinst_engine->setGain(0.0);
    audinst_engine->play();

	// wind sound
    audinst_wind = new PAudioInstance(aud_wind, true);
    audinst_wind->setGain(0.0);
    audinst_wind->play();

	// terrain sound
    audinst_gravel = new PAudioInstance(aud_gravel, true);
    audinst_gravel->setGain(0.0);
    audinst_gravel->play();
  }
}

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
    delete audinst_engine;
    audinst_engine = nullptr;
  }

  if (audinst_wind) {
    delete audinst_wind;
    audinst_wind = nullptr;
  }

  if (audinst_gravel) {
    delete audinst_gravel;
    audinst_gravel = nullptr;
  }

  for (unsigned int i=0; i<audinst.size(); i++) {
    delete audinst[i];
  }
  audinst.clear();

  if (game) {
    delete game;
    game = nullptr;
  }

  if (audinst_intro) {
    audinst_intro->play();
  }

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

void MainApp::tick(float delta)
{
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

  if (game->isFinished())
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

  game->tick(delta);

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
        if (rand01 * 20.0f < game->vehicle[i]->part[j].wheel[k].dirtthrow)
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
            psys_dirt->addParticle(dirtpos, dirtvec /*+ vec3f::rand() * 10.0f*/);
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

  renderowncar = (cameraview_mod != CameraMode::hood && cameraview_mod != CameraMode::bumper);

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
            audinst.push_back(new PAudioInstance(aud_shiftup));
            audinst.back()->setPitch(0.7f + randm11*0.02f);
            audinst.back()->setGain(1.0f * cfg.getVolumeSfx());
            audinst.back()->play();
            break;
        }
        case -1: // Shift down
        {
            audinst.push_back(new PAudioInstance(aud_shiftdown));
            audinst.back()->setPitch(0.8f + randm11*0.12f);
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
        audinst.push_back(new PAudioInstance(aud_crash1));
        audinst.back()->setPitch(1.0f + randm11*0.02f);
        audinst.back()->setGain(logf(1.0f + crashlevel) * cfg.getVolumeSfx());
        audinst.back()->play();

        if (haptic != nullptr)
          SDL_HapticRumblePlay(haptic, crashlevel * 0.2f, MAX(1000, (unsigned int)(crashlevel * 20.0f)));
      }
      crashnoise_timeout = rand01 * 0.1f + 0.01f;
    } else {
      crashnoise_timeout -= delta;
    }

    for (unsigned int i=0; i<audinst.size(); i++) {
      if (!audinst[i]->isPlaying()) {
        delete audinst[i];
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
  if (rand01 < numdrops - inumdrops) inumdrops++;
  for (int i=0; i<inumdrops; i++) {
    rain.push_back(RainDrop());
    rain.back().drop_pt = vec3f(campos.x,campos.y,0);
    rain.back().drop_pt += camvel * RAIN_START_LIFE;
    rain.back().drop_pt += vec3f::rand() * RAIN_POS_RANDOM;
    rain.back().drop_pt.z = game->terrain->getHeight(rain.back().drop_pt.x, rain.back().drop_pt.y);

    if (game->water.enabled && rain.back().drop_pt.z < game->water.height)
        rain.back().drop_pt.z = game->water.height;

    rain.back().drop_vect = def_drop_vect + vec3f::rand() * RAIN_VEL_RANDOM;
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
  if (rand01 < numflakes - inumflakes) inumflakes++;
  for (int i=0; i<inumflakes; i++) {
    snowfall.push_back(SnowFlake());
    snowfall.back().drop_pt = vec3f(campos.x,campos.y,0);
    snowfall.back().drop_pt += camvel * SNOWFALL_START_LIFE / 2;
    snowfall.back().drop_pt += vec3f::rand() * SNOWFALL_POS_RANDOM;
    snowfall.back().drop_pt.z = game->terrain->getHeight(snowfall.back().drop_pt.x, snowfall.back().drop_pt.y);

    if (game->water.enabled && snowfall.back().drop_pt.z < game->water.height)
        snowfall.back().drop_pt.z = game->water.height;

    snowfall.back().drop_vect = def_drop_vect + vec3f::rand() * SNOWFALL_VEL_RANDOM;
    snowfall.back().life = SNOWFALL_START_LIFE * rand01;
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

static bool fileExists(const std::string &path) {
  std::ifstream f(path.c_str());
  return f.good();
}

int main(int argc, char *argv[])
{
    std::string video_path = "";
    if (fileExists("inVideo.mp4")) {
      video_path = "inVideo.mp4";
    } else if (fileExists("bin/inVideo.mp4")) {
      video_path = "bin/inVideo.mp4";
    } else if (fileExists("../inVideo.mp4")) {
      video_path = "../inVideo.mp4";
    }

    if (!video_path.empty()) {
      std::string cmd = "ffplay -fs -autoexit -noborder -loglevel quiet \"" + video_path + "\"";
      int ret = std::system(cmd.c_str());
      (void)ret;
    }

    return MainApp("TrackRS", ".trackrs").run(argc, argv);
}
```

### [menu.cpp](file:///home/alan/Downloads/trigger-rally-code-r1032/src/Trigger/menu.cpp)
```diff:menu.cpp
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
    lss.currentlevel = index;
    lss.state = AM_TOP_LVL_PREP;
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
    startGame(levels[lss.currentlevel].filename);
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
  gui.addLabel(10.0f,570.0f, "Trigger Rally", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER, 30.0f, LabelStyle::Weak);

  switch (lss.state) {
  case AM_TOP:
    gui.makeClickable(
      gui.addLabel(400.0f,400.0f, "events", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_EVT, 0);
    gui.makeClickable(
      gui.addLabel(400.0f,350.0f, "practice", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_PRAC, 0);
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

        if (!events[i].locked || player_unlocks.count(events[i].filename) != 0)
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

        if (!events[i].locked || player_unlocks.count(events[i].filename) != 0)
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
        gui.makeClickable(
          gui.addLabel(100.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
          levels[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List),
          AA_PICK_LVL, i);

        gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            levels[i].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
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

      if (lss.currentlevel < static_cast<int> (levels.size() - 1))
        gui.makeClickable(nextbutton, AA_PICK_LVL, idxnext);

      if (lss.currentlevel > 0)
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
                levelScreenAction(AA_PICK_LVL, pidx);
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
                levelScreenAction(AA_PICK_LVL, nidx);
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

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

  glOrtho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);

  tex_splash_screen->bind();

  //glColor4f(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();


  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  // draw GUI

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0f, 1.0f, 1.0f, 0.2f);

  tex_fontSourceCodeOutlined->bind();

  glPushMatrix(); // 0

  gui.render();

  glPopMatrix(); // 0

  glBlendFunc(GL_ONE, GL_ZERO);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_FOG);
  glEnable(GL_LIGHTING);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
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
void Gui::tick(float delta)
{
  float decay = delta * 3.0f;

  // gradually unglow all widgets
  for (unsigned int i = 0; i < widget.size(); i++)
  {
    widget[i].glow -= decay;
    CLAMP_LOWER(widget[i].glow, 0.0f);
  }

  // keep the highlighted widget fully glowing
  if (highlight != -1) {
    widget[highlight].glow = 1.0f;
  }

  defflash = fmodf(defflash + delta * 50.0f, PI*2.0f);
}

void Gui::setCursorPos(float x, float y)
{
  highlight = -1;

  for (unsigned int i = 0; i < widget.size(); i++) {

    if (!widget[i].clickable) continue;

    if (x >= widget[i].pos.x &&
      y >= widget[i].pos.y &&
      x < widget[i].pos.x + widget[i].dims_min.x &&
      y < widget[i].pos.y + widget[i].dims_min.y)
      highlight = i;
  }
}

bool Gui::getClickAction(int &data1, int &data2)
{
  if (highlight == -1) return false;

  data1 = widget[highlight].d1;
  data2 = widget[highlight].d2;

  return true;
}

bool Gui::getDefaultAction(int &data1, int &data2)
{
  if (defwidget == -1) return false;

  data1 = widget[defwidget].d1;
  data2 = widget[defwidget].d2;

  return true;
}

void Gui::render()
{
  for (unsigned int i = 0; i < widget.size(); i++) {

    switch(widget[i].type) {
    case GWT_LABEL: {
      vec4f colc;
      uint32 flags = PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM;

      if (widget[i].selectable && !widget[i].selected) {
        colc = INTERP(widget[i].colnormal, widget[i].colhover, widget[i].glow);
      } else if (widget[i].clickable) {
        colc = INTERP(widget[i].colclick, widget[i].colhover, widget[i].glow);
      } else {
        colc = widget[i].colnormal;
      }

      if ((int)i == defwidget)
        colc += vec4f(0.1f, -0.1f, -0.1f, 0.0f) * sinf(defflash);

      if (widget[i].selectable && widget[i].selected)
        flags |= PTEXT_HIGHLIGHT;

      glPushMatrix();

      vec2f ctr = widget[i].pos;
      glTranslatef(ctr.x, ctr.y, 0.0f);

      glScalef(widget[i].fontsize, widget[i].fontsize, 1.0f);

      fonttex->bind();

      glColor4fv(colc);
      ssRender->drawText(widget[i].text, flags);
      glPopMatrix();
      } break;

    case GWT_GRAPHIC: {
      vec4f colc = vec4f(1.0f, 1.0f, 1.0f, 1.0f);

      // Work-around for drawing transparent background
      if (!widget[i].tex) {
        colc = vec4f(0.0f, 0.0f, 0.0f, 0.25f);
      }
      else if (widget[i].clickable) {
        colc = INTERP(widget[i].colclick, widget[i].colhover, widget[i].glow);
      } else {
        colc = widget[i].colnormal;
      }

      vec2f min = widget[i].pos;
      vec2f max = widget[i].pos + widget[i].dims_min;

      if (widget[i].tex)
        widget[i].tex->bind();
      else
        glDisable(GL_TEXTURE_2D);

      glColor4fv(colc);

      glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(min.x, min.y);
      glTexCoord2f(1.0f, 0.0f); glVertex2f(max.x, min.y);
      glTexCoord2f(1.0f, 1.0f); glVertex2f(max.x, max.y);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(min.x, max.y);
      glEnd();

      if (!widget[i].tex)
        glEnable(GL_TEXTURE_2D);
      } break;
    }
  }
}

// Widget tree stuff wasn't working properly, so I removed it for
// now. If I need ultra-snazzy menus, I may finish this code

#if 0

void Gui::doLayout()
{
  // Calculate sizes
  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].parent == GWPARENT_NONE) {
      measureWidgetTree(i);
      placeWidgetTree(i);
    }
  }
}

void Gui::measureWidgetTree(int w)
{
  widget[w].childcount = 0;
  widget[w].fillercount = 0;

  switch (widget[w].type) {
  default:

    widget[w].dims_measure = widget[w].dims_min;

    break;

  case GWT_CONTAINER: {

    vec2f measure = vec2f(0.0f, 0.0f);

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {
        measureWidgetTree(i);

        widget[w].childcount++;

        if (widget[i].type == GWT_FILLER)
          widget[w].fillercount++;

        if (widget[w].vert) {
          CLAMP_LOWER(measure.x, widget[i].dims_measure.x);
          measure.y += widget[i].dims_measure.y;
        } else {
          measure.x += widget[i].dims_measure.x;
          CLAMP_LOWER(measure.y, widget[i].dims_measure.y);
        }
      }
    }

    widget[w].dims_measure = measure;

    } break;
  }
}

void Gui::placeWidgetTree(int w)
{
  if (widget[w].childcount <= 0) return;

  float extraspace = widget[w].vert ?
    - widget[w].dims_measure.x :
    - widget[w].dims_measure.y;
  if (widget[w].parent == GWPARENT_NONE) {
    extraspace += widget[w].vert ?
      widget[w].dims_min.x :
      widget[w].dims_min.y;
  }

  CLAMP_LOWER(extraspace, 0.0f);

  //CLAMP_LOWER(widget[w].dims_measure.x, widget[w].dims_min.x);
  //CLAMP_LOWER(widget[w].dims_measure.y, widget[w].dims_min.y);

  float
    addtofillers = 0.0f,
    addtochildren = 0.0f;
  /*
  if (widget[w].fillercount > 0)
    addtofillers = extraspace / (float)widget[w].fillercount;
  else
    addtochildren = extraspace / (float)widget[w].childcount;*/

  if (widget[w].vert) {
    float distrib = widget[w].pos.y;

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {

        widget[i].pos.x = widget[w].pos.x;
        widget[i].pos.y = distrib;

        widget[i].dims_measure.x = widget[w].dims_measure.x;

        switch (widget[i].type) {
        case GWT_FILLER:
          widget[i].dims_measure.y += addtofillers;
          break;
        case GWT_CONTAINER:
          widget[i].dims_measure.y += addtochildren;
          placeWidgetTree(i);
          break;
        default:
          widget[i].dims_measure.y += addtochildren;
          break;
        }

        distrib += widget[i].dims_measure.y;
      }
    }
  } else {
    float distrib = widget[w].pos.x;

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {

        widget[i].pos.x = distrib;
        widget[i].pos.y = widget[w].pos.y;

        widget[i].dims_measure.y = widget[w].dims_measure.y;

        switch (widget[i].type) {
        case GWT_FILLER:
          widget[i].dims_measure.x += addtofillers;
          break;
        case GWT_CONTAINER:
          widget[i].dims_measure.x += addtochildren;
          placeWidgetTree(i);
          break;
        default:
          widget[i].dims_measure.x += addtochildren;
          break;
        }

        distrib += widget[i].dims_measure.x;
      }
    }
  }
}

void Gui::render()
{
  // Render trees of all root containers

  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].parent == GWPARENT_NONE)
      renderWidgetTree(i);
  }
}

void Gui::renderWidgetTree(int w)
{
  vec2f min, max;

  switch (widget[w].type) {
  case GWT_CONTAINER:
    glColor4f(1.0f,0.0f,0.0f,0.2f);
    break;
  case GWT_FILLER:
    glColor4f(0.0f,1.0f,0.0f,0.2f);
    break;
  case GWT_LABEL:
    glColor4f(0.0f,0.0f,1.0f,0.2f);
    break;
  }

  min = widget[w].pos;
  max = widget[w].pos + widget[w].dims_measure;

  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glVertex2f(min.x, min.y);
  glVertex2f(max.x, min.y);
  glVertex2f(max.x, max.y);
  glVertex2f(min.x, max.y);
  glEnd();
  glEnable(GL_TEXTURE_2D);

  // Render this widget
  switch (widget[w].type) {
  default:
    break;

  case GWT_LABEL: {
    glPushMatrix();
    vec2f ctr = widget[w].pos + widget[w].dims_measure * 0.5f;
    glTranslatef(ctr.x, ctr.y, 0.0f);
    glScalef(widget[w].fontsize, widget[w].fontsize, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ssRender->drawText(widget[w].text, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    glPopMatrix();
    } break;
  }

  // Render children
  switch (widget[w].type) {
  case GWT_CONTAINER:
    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w)
        renderWidgetTree(i);
    }
    break;
  }
}

#endif

int Gui::getFreeWidget()
{
  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].type == GWT_FREE)
      return i;
  }
  widget.push_back(GuiWidget(GWT_FREE));
  return widget.size() - 1;
}

#if 0
int Gui::addRootContainer(float x, float y, float width, float height, bool vert)
{
  int w = getFreeWidget();
  widget[w].type = GWT_CONTAINER;
  widget[w].vert = vert;
  widget[w].parent = GWPARENT_NONE;
  widget[w].dims_min = vec2f(width, height);
  widget[w].pos = vec2f(x, y);

  return w;
}

int Gui::addFiller(int parent, float minwidth, float minheight)
{
  int w = getFreeWidget();
  widget[w].type = GWT_FILLER;
  widget[w].parent = parent;
  widget[w].dims_min = vec2f(minwidth, minheight);

  return w;
}

int Gui::addContainer(float x, float y, float width, float height, bool vert)
{
  int w = getFreeWidget();
  widget[w].type = GWT_CONTAINER;
  widget[w].vert = vert;
  widget[w].parent = parent;
  widget[w].dims_min = vec2f(minwidth, minheight);

  return w;
}
#endif

int Gui::addLabel(float x, float y, const std::string &text, uint32 flags, float fontsize, LabelStyle ls)
{
  int w = getFreeWidget();
  widget[w].type = GWT_LABEL;
  widget[w].text = text;
  widget[w].fontsize = fontsize;
  widget[w].dims_min = ssRender->getTextDims(text) * fontsize;
  widget[w].pos = vec2f(x, y);

  if (ls == LabelStyle::Regular)
  {
      widget[w].colnormal   = colors.normal;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Weak)
  {
      widget[w].colnormal   = colors.weak;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Strong)
  {
      widget[w].colnormal   = colors.strong;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Marked)
  {
      widget[w].colnormal   = colors.marked;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Header)
  {
      widget[w].colnormal   = colors.header;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::List)
  {
      widget[w].colnormal   = colors.listnormal;
      widget[w].colclick    = colors.listclick;
      widget[w].colhover    = colors.listhover;
  }

  if (flags & PTEXT_HZA_CENTER)
    widget[w].pos.x -= widget[w].dims_min.x * 0.5f;
  else if (flags & PTEXT_HZA_RIGHT)
    widget[w].pos.x -= widget[w].dims_min.x;

  if (flags & PTEXT_VTA_CENTER)
    widget[w].pos.y -= widget[w].dims_min.y * 0.5f;
  else if (flags & PTEXT_VTA_TOP)
    widget[w].pos.y -= widget[w].dims_min.y;

  return w;
}

int Gui::addGraphic(float x, float y, float width, float height, PTexture *tex, GraphicStyle gs)
{
  int w = getFreeWidget();
  widget[w].type = GWT_GRAPHIC;
  widget[w].dims_min = vec2f(width, height);
  widget[w].pos = vec2f(x, y);
  widget[w].tex = tex;

    if (gs == GraphicStyle::Button)
    {
        widget[w].colnormal = colors.bnormal;
        widget[w].colclick  = colors.bclick;
        widget[w].colhover  = colors.bhover;
    }
    else
    if (gs == GraphicStyle::Image)
    {
        widget[w].colnormal = {1.00f, 1.00f, 1.00f, 1.00f};
        widget[w].colclick  = {1.00f, 1.00f, 1.00f, 1.00f};
        widget[w].colhover  = {1.00f, 1.00f, 1.00f, 1.00f};
    }

  return w;
}
===
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
    lss.currentlevel = index;
    lss.state = AM_TOP_LVL_PREP;
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
    startGame(levels[lss.currentlevel].filename);
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
    gui.makeClickable(
      gui.addLabel(400.0f,400.0f, "events", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_EVT, 0);
    gui.makeClickable(
      gui.addLabel(400.0f,350.0f, "practice", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER, 40.0f), AA_GO_PRAC, 0);
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

        if (!events[i].locked || player_unlocks.count(events[i].filename) != 0)
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

        if (!events[i].locked || player_unlocks.count(events[i].filename) != 0)
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
        gui.makeClickable(
          gui.addLabel(100.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
          levels[i].name, PTEXT_HZA_LEFT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List),
          AA_PICK_LVL, i);

        gui.addLabel(700.0f, 440.0f - (float)(i - firstraceindex) * 30.0f,
            levels[i].targettimeshort, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP, 25.0f, LabelStyle::List);
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

      if (lss.currentlevel < static_cast<int> (levels.size() - 1))
        gui.makeClickable(nextbutton, AA_PICK_LVL, idxnext);

      if (lss.currentlevel > 0)
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
                levelScreenAction(AA_PICK_LVL, pidx);
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
                levelScreenAction(AA_PICK_LVL, nidx);
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

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

  glOrtho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

  glMatrixMode(GL_MODELVIEW);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);

  tex_splash_screen->bind();

  //glColor4f(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

  // Old-school rally Ken Burns effect
  float zoom = 0.82f + 0.04f * sinf(menu_time * 0.2f);
  float dx = 0.04f * cosf(menu_time * 0.15f);
  float dy = 0.04f * sinf(menu_time * 0.11f);

  auto map_tx = [&](float tx) { return 0.5f + dx + (tx - 0.5f) * zoom; };
  auto map_ty = [&](float ty) { return 0.5f + dy + (ty - 0.5f) * zoom; };

  glBegin(GL_QUADS);
  // the background image is square and cut out a piece based on aspect ratio
  // -------- if aspect ratio is larger than 4:3
  // if aspect ratio is larger than 1:1
  if ((float)getWidth()/(float)getHeight() > 1.0f)
  {
    // lower and upper offset based on aspect ratio
    float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
    float off_u = 1 - off_l;
    glTexCoord2f(map_tx(1.0f), map_ty(off_u)); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(map_tx(0.0f), map_ty(off_u)); glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(map_tx(0.0f), map_ty(off_l)); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(map_tx(1.0f), map_ty(off_l)); glVertex2f(1.0f, -1.0f);
  }
  // other cases (including 4:3, in which case off_l and off_u are = 1)
  else
  {
    float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
    float off_u = 1 - off_l;
    glTexCoord2f(map_tx(off_u), map_ty(1.0f)); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(map_tx(off_l), map_ty(1.0f)); glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(map_tx(off_l), map_ty(0.0f)); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(map_tx(off_u), map_ty(0.0f)); glVertex2f(1.0f, -1.0f);
  }
  glEnd();

  // Draw animated gradient, scanlines, and particles
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 1. Dark colored gradient overlay
  glBegin(GL_QUADS);
  // Top: deep dark blue
  glColor4f(0.02f, 0.05f, 0.12f, 0.65f + 0.05f * sinf(menu_time * 0.5f));
  glVertex2f(-1.0f, 1.0f);
  glVertex2f(1.0f, 1.0f);
  // Bottom: dark warm charcoal/reddish-purple glow
  float r_glow = 0.08f + 0.04f * sinf(menu_time * 0.3f);
  float b_glow = 0.04f + 0.02f * cosf(menu_time * 0.4f);
  glColor4f(r_glow, 0.01f, b_glow, 0.85f);
  glVertex2f(1.0f, -1.0f);
  glVertex2f(-1.0f, -1.0f);
  glEnd();

  // 2. Subtle scrolling retro CRT scanlines
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  for (int j = 0; j < 40; ++j) {
      float y = -1.0f + (j / 20.0f);
      float scan_alpha = 0.03f + 0.01f * sinf(menu_time * 2.0f + y * 5.0f);
      glColor4f(1.0f, 1.0f, 1.0f, scan_alpha);
      glVertex2f(-1.0f, y);
      glVertex2f(1.0f, y);
  }
  glEnd();

  // 3. Stateless speed particles/spark drift
  glBegin(GL_QUADS);
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
          glColor4f(1.0f, 0.65f, 0.15f, spark_alpha);
      } else {
          glColor4f(0.8f, 0.9f, 1.0f, spark_alpha);
      }
      
      glVertex2f(x - length, y + size * 0.5f);
      glVertex2f(x + length, y + size * 0.5f);
      glVertex2f(x + length, y - size * 0.5f);
      glVertex2f(x - length, y - size * 0.5f);
  }
  glEnd();

  glEnable(GL_TEXTURE_2D);


  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  // draw GUI

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0f, 1.0f, 1.0f, 0.2f);

  tex_fontSourceCodeOutlined->bind();

  glPushMatrix(); // 0

  gui.render();

  glPopMatrix(); // 0

  glBlendFunc(GL_ONE, GL_ZERO);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_FOG);
  glEnable(GL_LIGHTING);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
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
void Gui::tick(float delta)
{
  float decay = delta * 3.0f;

  // gradually unglow all widgets
  for (unsigned int i = 0; i < widget.size(); i++)
  {
    widget[i].glow -= decay;
    CLAMP_LOWER(widget[i].glow, 0.0f);
  }

  // keep the highlighted widget fully glowing
  if (highlight != -1) {
    widget[highlight].glow = 1.0f;
  }

  defflash = fmodf(defflash + delta * 50.0f, PI*2.0f);
}

void Gui::setCursorPos(float x, float y)
{
  highlight = -1;

  for (unsigned int i = 0; i < widget.size(); i++) {

    if (!widget[i].clickable) continue;

    if (x >= widget[i].pos.x &&
      y >= widget[i].pos.y &&
      x < widget[i].pos.x + widget[i].dims_min.x &&
      y < widget[i].pos.y + widget[i].dims_min.y)
      highlight = i;
  }
}

bool Gui::getClickAction(int &data1, int &data2)
{
  if (highlight == -1) return false;

  data1 = widget[highlight].d1;
  data2 = widget[highlight].d2;

  return true;
}

bool Gui::getDefaultAction(int &data1, int &data2)
{
  if (defwidget == -1) return false;

  data1 = widget[defwidget].d1;
  data2 = widget[defwidget].d2;

  return true;
}

void Gui::render()
{
  for (unsigned int i = 0; i < widget.size(); i++) {

    switch(widget[i].type) {
    case GWT_LABEL: {
      vec4f colc;
      uint32 flags = PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM;
      vec2f ctr = widget[i].pos;

      if (widget[i].selectable && !widget[i].selected) {
        colc = INTERP(widget[i].colnormal, widget[i].colhover, widget[i].glow);
      } else if (widget[i].clickable) {
        colc = INTERP(widget[i].colclick, widget[i].colhover, widget[i].glow);
      } else {
        colc = widget[i].colnormal;
      }

      if (widget[i].text == "TrackRS") {
         glPushMatrix();
         glTranslatef(15.0f, ctr.y - 12.0f, 0.0f);
         glScalef(55.0f, 55.0f, 1.0f);
         fonttex->bind();
         glColor4f(0.1f, 0.0f, 0.05f, 0.8f);
         ssRender->drawText(widget[i].text, flags);
         glPopMatrix();

         glPushMatrix();
         glTranslatef(10.0f, ctr.y - 10.0f, 0.0f);
         glScalef(55.0f, 55.0f, 1.0f);
         fonttex->bind();
         float pulse = 0.5f + 0.5f * sinf((float)SDL_GetTicks() / 300.0f);
         glColor4f(1.0f, 0.2f + 0.2f * pulse, 0.0f, 1.0f);
         ssRender->drawText(widget[i].text, flags);
         glPopMatrix();

         glDisable(GL_TEXTURE_2D);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         glBegin(GL_QUADS);
         glColor4f(1.0f, 0.2f, 0.0f, 0.9f);
         glVertex2f(10.0f, ctr.y - 20.0f);
         glColor4f(1.0f, 0.6f, 0.0f, 0.5f);
         glVertex2f(300.0f, ctr.y - 20.0f);
         glColor4f(1.0f, 0.6f, 0.0f, 0.0f);
         glVertex2f(450.0f, ctr.y - 24.0f);
         glColor4f(1.0f, 0.2f, 0.0f, 0.9f);
         glVertex2f(10.0f, ctr.y - 24.0f);
         glEnd();
         
         glEnable(GL_TEXTURE_2D);
         continue;
      }

      if ((int)i == defwidget)
        colc += vec4f(0.1f, -0.1f, -0.1f, 0.0f) * sinf(defflash);

      if (widget[i].selectable && widget[i].selected)
        flags |= PTEXT_HIGHLIGHT;

      float shift_x = 0.0f;
      if (widget[i].clickable && widget[i].glow > 0.0f) {
        shift_x = widget[i].glow * 12.0f;
      }

      glPushMatrix();
      glTranslatef(ctr.x + shift_x, ctr.y, 0.0f);

      bool is_centered = (flags & PTEXT_HZA_CENTER) != 0;

      if (widget[i].clickable && widget[i].glow > 0.01f) {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        float alpha = widget[i].glow * 0.32f;
        glBegin(GL_QUADS);
        if (is_centered) {
          glColor4f(0.9f, 0.25f, 0.0f, 0.0f);
          glVertex2f(-220.0f, 25.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.35f, 0.0f, alpha);
          glVertex2f(0.0f, 25.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.5f, 0.0f, 0.0f);
          glVertex2f(220.0f, -5.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.35f, 0.0f, alpha);
          glVertex2f(0.0f, -5.0f * widget[i].fontsize * 0.05f);
        } else {
          glColor4f(0.9f, 0.35f, 0.0f, alpha);
          glVertex2f(-20.0f, 25.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.5f, 0.0f, alpha * 0.3f);
          glVertex2f(350.0f, 25.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.5f, 0.0f, 0.0f);
          glVertex2f(450.0f, -5.0f * widget[i].fontsize * 0.05f);
          glColor4f(0.9f, 0.35f, 0.0f, alpha);
          glVertex2f(-30.0f, -5.0f * widget[i].fontsize * 0.05f);
        }
        glEnd();

        float time_sec = (float)SDL_GetTicks() / 1000.0f;
        float arrow_size = 5.0f * widget[i].fontsize * 0.05f;
        float arrow_y = 8.0f * widget[i].fontsize * 0.05f;

        if (is_centered) {
          float est_w = widget[i].text.length() * widget[i].fontsize * 0.28f;
          float arrow_pulse_l = -est_w - 18.0f - 3.0f * sinf(time_sec * 8.0f);
          glBegin(GL_TRIANGLES);
          glColor4f(1.0f, 0.5f, 0.0f, widget[i].glow);
          glVertex2f(arrow_pulse_l - arrow_size, arrow_y + arrow_size);
          glVertex2f(arrow_pulse_l, arrow_y);
          glVertex2f(arrow_pulse_l - arrow_size, arrow_y - arrow_size);
          glEnd();

          float arrow_pulse_r = est_w + 18.0f + 3.0f * sinf(time_sec * 8.0f);
          glBegin(GL_TRIANGLES);
          glColor4f(1.0f, 0.5f, 0.0f, widget[i].glow);
          glVertex2f(arrow_pulse_r + arrow_size, arrow_y + arrow_size);
          glVertex2f(arrow_pulse_r, arrow_y);
          glVertex2f(arrow_pulse_r + arrow_size, arrow_y - arrow_size);
          glEnd();
        } else {
          float arrow_pulse = -20.0f + 3.0f * sinf(time_sec * 8.0f);
          glBegin(GL_TRIANGLES);
          glColor4f(1.0f, 0.5f, 0.0f, widget[i].glow);
          glVertex2f(arrow_pulse - arrow_size, arrow_y + arrow_size);
          glVertex2f(arrow_pulse, arrow_y);
          glVertex2f(arrow_pulse - arrow_size, arrow_y - arrow_size);
          glEnd();
        }
        
        glEnable(GL_TEXTURE_2D);
      }

      if (widget[i].text.length() > 0 &&
          (widget[i].colnormal.x == colors.header.x &&
           widget[i].colnormal.y == colors.header.y &&
           widget[i].colnormal.z == colors.header.z)) {
         glDisable(GL_TEXTURE_2D);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glColor4f(colors.header.x, colors.header.y, colors.header.z, 0.8f);
         glLineWidth(2.0f);
         glBegin(GL_LINES);
         glVertex2f(0.0f, -6.0f);
         glVertex2f(350.0f, -6.0f);
         glVertex2f(0.0f, -10.0f);
         glVertex2f(200.0f, -10.0f);
         glEnd();
         glEnable(GL_TEXTURE_2D);
      }

      glScalef(widget[i].fontsize, widget[i].fontsize, 1.0f);

      fonttex->bind();

      glColor4fv(colc);
      ssRender->drawText(widget[i].text, flags);
      glPopMatrix();
      } break;

    case GWT_GRAPHIC: {
      vec2f min = widget[i].pos;
      vec2f max = widget[i].pos + widget[i].dims_min;

      if (!widget[i].tex) {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // 1. Semi-transparent dark background panel
        glColor4f(0.02f, 0.04f, 0.08f, 0.65f);
        glBegin(GL_QUADS);
        glVertex2f(min.x, min.y);
        glVertex2f(max.x, min.y);
        glVertex2f(max.x, max.y);
        glVertex2f(min.x, max.y);
        glEnd();
        
        // 2. Neon/silver thin outline border
        glLineWidth(1.5f);
        if (widget[i].clickable && widget[i].glow > 0.01f) {
          float pulse = 0.7f + 0.3f * sinf((float)SDL_GetTicks() / 150.0f);
          glColor4f(1.0f, 0.55f, 0.0f, widget[i].glow * pulse);
        } else {
          glColor4f(0.35f, 0.4f, 0.45f, 0.4f);
        }
        
        glBegin(GL_LINE_LOOP);
        glVertex2f(min.x, min.y);
        glVertex2f(max.x, min.y);
        glVertex2f(max.x, max.y);
        glVertex2f(min.x, max.y);
        glEnd();
        
        glEnable(GL_TEXTURE_2D);
      }
      else {
        vec4f colc = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (widget[i].clickable) {
          colc = INTERP(widget[i].colclick, widget[i].colhover, widget[i].glow);
        } else {
          colc = widget[i].colnormal;
        }

        widget[i].tex->bind();
        glColor4fv(colc);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(min.x, min.y);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(max.x, min.y);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(max.x, max.y);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(min.x, max.y);
        glEnd();
      }
      } break;
    }
  }
}

// Widget tree stuff wasn't working properly, so I removed it for
// now. If I need ultra-snazzy menus, I may finish this code

#if 0

void Gui::doLayout()
{
  // Calculate sizes
  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].parent == GWPARENT_NONE) {
      measureWidgetTree(i);
      placeWidgetTree(i);
    }
  }
}

void Gui::measureWidgetTree(int w)
{
  widget[w].childcount = 0;
  widget[w].fillercount = 0;

  switch (widget[w].type) {
  default:

    widget[w].dims_measure = widget[w].dims_min;

    break;

  case GWT_CONTAINER: {

    vec2f measure = vec2f(0.0f, 0.0f);

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {
        measureWidgetTree(i);

        widget[w].childcount++;

        if (widget[i].type == GWT_FILLER)
          widget[w].fillercount++;

        if (widget[w].vert) {
          CLAMP_LOWER(measure.x, widget[i].dims_measure.x);
          measure.y += widget[i].dims_measure.y;
        } else {
          measure.x += widget[i].dims_measure.x;
          CLAMP_LOWER(measure.y, widget[i].dims_measure.y);
        }
      }
    }

    widget[w].dims_measure = measure;

    } break;
  }
}

void Gui::placeWidgetTree(int w)
{
  if (widget[w].childcount <= 0) return;

  float extraspace = widget[w].vert ?
    - widget[w].dims_measure.x :
    - widget[w].dims_measure.y;
  if (widget[w].parent == GWPARENT_NONE) {
    extraspace += widget[w].vert ?
      widget[w].dims_min.x :
      widget[w].dims_min.y;
  }

  CLAMP_LOWER(extraspace, 0.0f);

  //CLAMP_LOWER(widget[w].dims_measure.x, widget[w].dims_min.x);
  //CLAMP_LOWER(widget[w].dims_measure.y, widget[w].dims_min.y);

  float
    addtofillers = 0.0f,
    addtochildren = 0.0f;
  /*
  if (widget[w].fillercount > 0)
    addtofillers = extraspace / (float)widget[w].fillercount;
  else
    addtochildren = extraspace / (float)widget[w].childcount;*/

  if (widget[w].vert) {
    float distrib = widget[w].pos.y;

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {

        widget[i].pos.x = widget[w].pos.x;
        widget[i].pos.y = distrib;

        widget[i].dims_measure.x = widget[w].dims_measure.x;

        switch (widget[i].type) {
        case GWT_FILLER:
          widget[i].dims_measure.y += addtofillers;
          break;
        case GWT_CONTAINER:
          widget[i].dims_measure.y += addtochildren;
          placeWidgetTree(i);
          break;
        default:
          widget[i].dims_measure.y += addtochildren;
          break;
        }

        distrib += widget[i].dims_measure.y;
      }
    }
  } else {
    float distrib = widget[w].pos.x;

    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w) {

        widget[i].pos.x = distrib;
        widget[i].pos.y = widget[w].pos.y;

        widget[i].dims_measure.y = widget[w].dims_measure.y;

        switch (widget[i].type) {
        case GWT_FILLER:
          widget[i].dims_measure.x += addtofillers;
          break;
        case GWT_CONTAINER:
          widget[i].dims_measure.x += addtochildren;
          placeWidgetTree(i);
          break;
        default:
          widget[i].dims_measure.x += addtochildren;
          break;
        }

        distrib += widget[i].dims_measure.x;
      }
    }
  }
}

void Gui::render()
{
  // Render trees of all root containers

  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].parent == GWPARENT_NONE)
      renderWidgetTree(i);
  }
}

void Gui::renderWidgetTree(int w)
{
  vec2f min, max;

  switch (widget[w].type) {
  case GWT_CONTAINER:
    glColor4f(1.0f,0.0f,0.0f,0.2f);
    break;
  case GWT_FILLER:
    glColor4f(0.0f,1.0f,0.0f,0.2f);
    break;
  case GWT_LABEL:
    glColor4f(0.0f,0.0f,1.0f,0.2f);
    break;
  }

  min = widget[w].pos;
  max = widget[w].pos + widget[w].dims_measure;

  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glVertex2f(min.x, min.y);
  glVertex2f(max.x, min.y);
  glVertex2f(max.x, max.y);
  glVertex2f(min.x, max.y);
  glEnd();
  glEnable(GL_TEXTURE_2D);

  // Render this widget
  switch (widget[w].type) {
  default:
    break;

  case GWT_LABEL: {
    glPushMatrix();
    vec2f ctr = widget[w].pos + widget[w].dims_measure * 0.5f;
    glTranslatef(ctr.x, ctr.y, 0.0f);
    glScalef(widget[w].fontsize, widget[w].fontsize, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    ssRender->drawText(widget[w].text, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    glPopMatrix();
    } break;
  }

  // Render children
  switch (widget[w].type) {
  case GWT_CONTAINER:
    for (unsigned int i = 0; i < widget.size(); i++) {
      if (widget[i].parent == w)
        renderWidgetTree(i);
    }
    break;
  }
}

#endif

int Gui::getFreeWidget()
{
  for (unsigned int i = 0; i < widget.size(); i++) {
    if (widget[i].type == GWT_FREE)
      return i;
  }
  widget.push_back(GuiWidget(GWT_FREE));
  return widget.size() - 1;
}

#if 0
int Gui::addRootContainer(float x, float y, float width, float height, bool vert)
{
  int w = getFreeWidget();
  widget[w].type = GWT_CONTAINER;
  widget[w].vert = vert;
  widget[w].parent = GWPARENT_NONE;
  widget[w].dims_min = vec2f(width, height);
  widget[w].pos = vec2f(x, y);

  return w;
}

int Gui::addFiller(int parent, float minwidth, float minheight)
{
  int w = getFreeWidget();
  widget[w].type = GWT_FILLER;
  widget[w].parent = parent;
  widget[w].dims_min = vec2f(minwidth, minheight);

  return w;
}

int Gui::addContainer(float x, float y, float width, float height, bool vert)
{
  int w = getFreeWidget();
  widget[w].type = GWT_CONTAINER;
  widget[w].vert = vert;
  widget[w].parent = parent;
  widget[w].dims_min = vec2f(minwidth, minheight);

  return w;
}
#endif

int Gui::addLabel(float x, float y, const std::string &text, uint32 flags, float fontsize, LabelStyle ls)
{
  int w = getFreeWidget();
  widget[w].type = GWT_LABEL;
  widget[w].text = text;
  widget[w].fontsize = fontsize;
  widget[w].dims_min = ssRender->getTextDims(text) * fontsize;
  widget[w].pos = vec2f(x, y);

  if (ls == LabelStyle::Regular)
  {
      widget[w].colnormal   = colors.normal;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Weak)
  {
      widget[w].colnormal   = colors.weak;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Strong)
  {
      widget[w].colnormal   = colors.strong;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Marked)
  {
      widget[w].colnormal   = colors.marked;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::Header)
  {
      widget[w].colnormal   = colors.header;
      widget[w].colclick    = colors.click;
      widget[w].colhover    = colors.hover;
  }
  else
  if (ls == LabelStyle::List)
  {
      widget[w].colnormal   = colors.listnormal;
      widget[w].colclick    = colors.listclick;
      widget[w].colhover    = colors.listhover;
  }

  if (flags & PTEXT_HZA_CENTER)
    widget[w].pos.x -= widget[w].dims_min.x * 0.5f;
  else if (flags & PTEXT_HZA_RIGHT)
    widget[w].pos.x -= widget[w].dims_min.x;

  if (flags & PTEXT_VTA_CENTER)
    widget[w].pos.y -= widget[w].dims_min.y * 0.5f;
  else if (flags & PTEXT_VTA_TOP)
    widget[w].pos.y -= widget[w].dims_min.y;

  return w;
}

int Gui::addGraphic(float x, float y, float width, float height, PTexture *tex, GraphicStyle gs)
{
  int w = getFreeWidget();
  widget[w].type = GWT_GRAPHIC;
  widget[w].dims_min = vec2f(width, height);
  widget[w].pos = vec2f(x, y);
  widget[w].tex = tex;

    if (gs == GraphicStyle::Button)
    {
        widget[w].colnormal = colors.bnormal;
        widget[w].colclick  = colors.bclick;
        widget[w].colhover  = colors.bhover;
    }
    else
    if (gs == GraphicStyle::Image)
    {
        widget[w].colnormal = {1.00f, 1.00f, 1.00f, 1.00f};
        widget[w].colclick  = {1.00f, 1.00f, 1.00f, 1.00f};
        widget[w].colhover  = {1.00f, 1.00f, 1.00f, 1.00f};
    }

  return w;
}
```

### [render.cpp](file:///home/alan/Downloads/trigger-rally-code-r1032/src/Trigger/render.cpp)
```diff:render.cpp
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "damage.h"
#include "main.h"
#include "vehicle.h"
#include <cmath>

void MainApp::resize()
{
    glClearColor(1.0,1.0,1.0,1.0);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ZERO);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    glEnable(GL_CULL_FACE);

	glViewport(0, 0, getWidth(), getHeight());

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    const GLfloat ambcol[] = {0.1f, 0.1f, 0.1f, 0.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambcol);

    float white[] = { 1.0,1.0,1.0,1.0 };
    //float black[] = { 0.0,0.0,0.0,1.0 };
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,white);

    float spec[] = { 0.3f, 0.5f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 6.0f);

    float litcol[] = { 0.6,0.6,0.6,0.0 };
    glLightfv(GL_LIGHT0,GL_DIFFUSE,litcol);
    glLightfv(GL_LIGHT0,GL_SPECULAR,litcol);

    glEnable(GL_NORMALIZE);
}

void drawBlades(float radius, float ang, float trace)
{
    float invtrace = 1.0 / trace;
    glPushMatrix();
    glScalef(radius, radius, 1.0);
    for (float ba=0; ba<PI*2.0-0.01; ba+=PI/2.0)
    {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.1,0.1,0.1,0.24 * invtrace);
        glVertex2f(0.0,0.0);
        glColor4f(0.1,0.1,0.1,0.06 * invtrace);
        int num = (int)(trace / 0.1);
        if (num < 2) num = 2;
        float mult = trace / (float)(num-1);
        float angadd = ba + ang;
        for (int i=0; i<num; ++i)
        {
            float a = (float)i * mult + angadd;
            glVertex2f(cos(a),sin(a));
        }
        glEnd();
    }
    glPopMatrix();
}

void MainApp::renderWater()
{
    tex_water->bind();
    {
        float tgens[] = { 0.5,0,0,0 };
        float tgent[] = { 0,0.5,0,0 };
        glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
        glTexGenfv(GL_S,GL_OBJECT_PLANE,tgens);
        glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
        glTexGenfv(GL_T,GL_OBJECT_PLANE,tgent);
    }
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glPushMatrix();
    glScalef(20.0,20.0,1.0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    {
        int minx = (int)(campos.x / 20.0)-20,
            maxx = minx + 40,
            miny = (int)(campos.y / 20.0)-20,
            maxy = miny + 40;
        for (int y=miny; y<maxy; ++y)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (int x=minx; x<=maxx; ++x)
            {
                float maxalpha = 0.5f;

                if (game->water.useralpha)
                    maxalpha = game->water.alpha;

                if (game->water.fixedalpha)
                {
                    glColor4f(1.0f, 1.0f, 1.0f, maxalpha);
                    glVertex3f(x, y+1, game->water.height);
                    glVertex3f(x, y, game->water.height);
                }
                else
                {
                    float ht,alpha;
                    ht = game->terrain->getHeight((x)*20.0,(y+1)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    glColor4f(1.0,1.0,1.0,alpha);
                    glVertex3f(x, y+1, game->water.height);
                    ht = game->terrain->getHeight((x)*20.0,(y)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    glColor4f(1.0,1.0,1.0,alpha);
                    glVertex3f(x, y, game->water.height);
                }
            }
            glEnd();
        }
    }
    glPopMatrix();
    glBlendFunc(GL_ONE,GL_ZERO);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

void MainApp::renderSky(const mat44f &cammat)
{
    glFogf(GL_FOG_DENSITY, game->weather.fog.density_sky);
    glDepthRange(0.999,1.0);
    glDisable(GL_CULL_FACE);
    glPushMatrix(); // 1
    glLoadIdentity();
    glMultMatrixf(cammat);
    tex_sky[0]->bind();
#define CLRANGE     10
#define CLFACTOR    0.02//0.014
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(cloudscroll,0.0,0.0);
    glRotatef(30.0,0.0,0.0,1.0);
    glScalef(0.4,0.4,1.0);
    for (int y=-CLRANGE; y<CLRANGE; y++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x=-CLRANGE; x<CLRANGE+1; x++)
        {
            glTexCoord2i(x,y);
            glVertex3f(x,y,0.3-(x*x+y*y)*CLFACTOR);
            glTexCoord2i(x,y+1);
            glVertex3f(x,y+1,0.3-(x*x+(y+1)*(y+1))*CLFACTOR);
        }
        glEnd();
    }
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(); // 1
    glEnable(GL_CULL_FACE);
    glDepthRange(0.0,0.999);
    glFogf(GL_FOG_DENSITY, game->weather.fog.density);
}

void MainApp::render(float eyetranslation)
{
    switch (appstate)
    {
        case AS_LOAD_1:
            renderStateLoading(eyetranslation);
            break;

        case AS_LOAD_2:
        case AS_LOAD_3:
            break;

        case AS_LEVEL_SCREEN:
            renderStateLevel(eyetranslation);
            break;

        case AS_CHOOSE_VEHICLE:
            renderStateChoose(eyetranslation);
            break;

        case AS_IN_GAME:
            renderStateGame(eyetranslation);
            break;

        case AS_END_SCREEN:
            renderStateEnd(eyetranslation);
            break;
    }

    glFinish();
}

void MainApp::renderStateLoading(float eyetranslation)
{
    UNREFERENCED_PARAMETER(eyetranslation);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    tex_splash_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();

    tex_loading_screen->bind();

    GLfloat logovratio = static_cast<float> (getWidth()) / getHeight();
    GLfloat logohratio = static_cast<float> (getHeight()) / getWidth();

    // FIXME: nasty, nasty code
    if (logovratio > 1.0f)
        logohratio = 1.0f;
    else
    if (logohratio > 1.0f)
        logovratio = 1.0f;

#define LOGO_VRATIO     (logovratio/3.5)
#define LOGO_HRATIO     (logohratio/3.5)
    glBegin(GL_QUADS);
      glTexCoord2f(1.0f, 1.0f); glVertex2f( LOGO_HRATIO,  LOGO_VRATIO);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-LOGO_HRATIO,  LOGO_VRATIO);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-LOGO_HRATIO, -LOGO_VRATIO);
      glTexCoord2f(1.0f, 0.0f); glVertex2f( LOGO_HRATIO, -LOGO_VRATIO);
    glEnd();
#undef LOGO_VRATIO
#undef LOGO_HRATIO

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

const char *creditstext[] =
{
    "TrackRally " PACKAGE_VERSION,
    "",
    "Copyright (C) JUly-2026",
    "Saad AIT YAHIA and Richard Langridge",
    "Posit Interactive",
    "",
    "Copyright (C) 2026",
    "Various Contributors",
    "(see DATA_AUTHORS.txt)",
    "",
    "",
    "",
    "Coding",
    "Saad AIT YAHIA",
    "Seckin Yasar",
    "",
    "Art & SFX",
    "Aza aza-ali",
    "",
    "",
    "",
    "Contributors",
    "",
    "Build system",
    "Max Hanna ",
    "",
    "Stereo support",
    "Muhammad Ali ",
    "",
    "Mac OS X porting",
    "Ahmad Abbas Hussain",
    "",
    "Fixes",
    "Saad AIT YAHIA",
    "Seckin Yasar",
    "",
    "New levels",
    "Seckin Yasar",
    "Ali Yaşar",
    "",
    "Graphics",
    "Saad AIT YAHIA",
    "Seckin Yasar",
    "",
    "",
    "",
    "",
    "",
    "Built by heart from ILISI Student",
    "",
    "",
    "",
    "",
    "Saad AIT YAHIA",
    "",
    "",
    "",
    "",
    "",
    "",
    "Thanks for playing Track, see you next time"
};

#define NUMCREDITSTRINGS (sizeof(creditstext) / sizeof(char*))

void MainApp::renderStateEnd(float eyetranslation)
{
    eyetranslation = eyetranslation;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    tex_end_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_ONE, GL_ZERO);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();

    tex_fontSourceCodeOutlined->bind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    //glOrtho(-1, 1, -1, 1, -1, 1);
    //glOrtho(800, 0, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();

    float scroll = splashtimeout;
    const float maxscroll = (float)(NUMCREDITSTRINGS - 1) * 2.0f;
    RANGEADJUST(scroll, 0.0f, 0.9f, -10.0f, maxscroll);
    CLAMP_UPPER(scroll, maxscroll);

    glScalef(0.1f, 0.1f, 1.0f);

    glTranslatef(0.0f, scroll, 0.0f);

    for (int i = 0; i < (int)NUMCREDITSTRINGS; i++)
    {
        float level = fabsf(scroll + (float)i * -2.0f);
        RANGEADJUST(level, 0.0f, 9.0f, 3.0f, 0.0f);

        if (level > 0.0f)
        {
            CLAMP_UPPER(level, 1.0f);

            glPushMatrix();
            glTranslatef(0.0f, (float)i * -2.0f, 0.0f);

            float enlarge = 1.0f;

#if 1
            if (splashtimeout > 0.9f)
            {
                float amt = (splashtimeout - 0.9f) * 10.0f;
                float amt2 = amt * amt;

                enlarge += amt2 / ((1.0001f - amt) * (1.0001f - amt));
                level -= amt2;
            }
#endif

            glScalef(enlarge, enlarge, 0.0f);
            glColor4f(1.0f, 1.0f, 1.0f, level);

            getSSRender().drawText(creditstext[i], PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix();
        }
    }

    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// render the car selection menu
void MainApp::renderStateChoose(float eyetranslation)
{
    PVehicleType *vtype = game->vehiclechoices[choose_type];

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glMatrixMode(GL_PROJECTION);

  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);

  tex_splash_screen->bind();

  //glColor4f(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    glMatrixMode(GL_MODELVIEW);


    glPushMatrix(); // 0

//    glTranslatef(-eyetranslation, 0.5f, -5.0f);
    glTranslatef(-eyetranslation, 0.9f, -5.0f);
    glRotatef(28.0f, 1.0f, 0.0f, 0.0f);

    glDisable(GL_FOG);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    vec4f lpos = vec4f(0.0f, 1.0f, 0.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    //float tmp = 1.0f;
    //float tmp = sinf(choose_spin * 2.0f) * 0.5f;
    float tmp = cosf(choose_spin * 2.0f) * 0.5f;
    tmp += choose_spin;
    glRotatef(90.0f, -1.0f, 0.0f, 0.0f);
    glRotatef(DEGREES(tmp), 0.0f, 0.0f, 1.0f);

    // render vehicle
    for (unsigned int i=0; i<vtype->part.size(); ++i)
    {
		glPushMatrix(); // 1

		vec3f vpos = vtype->part[i].render_ref_local.getPosition();
		glTranslatef(vpos.x, vpos.y, vpos.z);

		mat44f vorim = vtype->part[i].render_ref_local.getInverseOrientationMatrix();
		glMultMatrixf(vorim);
		if (vtype->part[i].model)
		{
			glPushMatrix(); // 2

			float scale = vtype->part[i].scale;
			glScalef(scale,scale,scale);
			drawModel(*vtype->part[i].model, 1.0f);

			glPopMatrix(); // 2
		}

		// render wheels
		if (vtype->wheelmodel)
		{
			for (unsigned int j=0; j<vtype->part[i].wheel.size(); j++)
			{

				glPushMatrix(); // 2

				vec3f &wpos = vtype->part[i].wheel[j].pt;
				glTranslatef(wpos.x, wpos.y, wpos.z);

				float scale = vtype->wheelscale * vtype->part[i].wheel[j].radius;
				glScalef(scale,scale,scale);

				drawModel(*vtype->wheelmodel, 1.0f);

				glPopMatrix(); // 2
			}
		}

		glPopMatrix(); // 1
	}

    glPopMatrix(); // 0

    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    // use the same colors as the menu
    const GuiWidgetColors gwc = gui.getColors();

    tex_fontSourceCodeShadowed->bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glPushMatrix(); // 0

    const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

    glOrtho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

    glPushMatrix(); // 1
    glTranslatef(10.0f, 570.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Trigger Rally", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(790.0f, 570.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(
        "car selection " + std::to_string(choose_type + 1) + '/' + std::to_string(game->vehiclechoices.size()),
        PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(100.0f, 230.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.header.x, gwc.header.y, gwc.header.z, gwc.header.w);
    getSSRender().drawText(vtype->proper_name.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(100.0f, 200.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->proper_class.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 230.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Weight (Kg)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 190.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Engine (BHP)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 150.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Wheel drive", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 110.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Roadholding", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 230.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(std::to_string(static_cast<int>(vtype->mass)), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 190.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_enginepower, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 150.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_wheeldrive, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 110.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_roadholding, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    std::string racename;

    if (lss.state == AM_TOP_EVT_PREP || lss.state == AM_TOP_PRAC_SEL_PREP)
        racename = events[lss.currentevent].name + ": " + events[lss.currentevent].levels[lss.currentlevel].name;
    else
    if (lss.state == AM_TOP_LVL_PREP)
        racename = levels[lss.currentlevel].name;

    glPushMatrix(); // 1
    glTranslatef(400.0f, 30.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(racename, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    if (vtype->getLocked()) {
      const std::string unlockevent = getVehicleUnlockEvent(vtype->getName());

      glPushMatrix(); // 1
      glTranslatef(400.0f, 425.0f, 0.0f);
      glScalef(40.0f, 40.0f, 1.0f);
      glColor4f(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
      getSSRender().drawText("Locked", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
      glPopMatrix(); // 1

      if (unlockevent != "") {
        glPushMatrix(); // 1
        glTranslatef(400.0f, 375.0f, 0.0f);
        glScalef(20.0f, 20.0f, 20.0f);
        glColor4f(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
        getSSRender().drawText("Complete event " + unlockevent + " to unlock", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        glPopMatrix(); // 1
      }
    }

    glPopMatrix(); // 0

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void MainApp::renderStateGame(float eyetranslation)
{
    PVehicle *vehic = game->vehicle[0];

    glClear(GL_DEPTH_BUFFER_BIT);
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    glMatrixMode(GL_MODELVIEW);

    glColor3f(1.0,1.0,1.0);

    vec4f fogcolor(game->weather.fog.color, 1.0f);
    glFogfv(GL_FOG_COLOR, fogcolor);

    glDepthRange(0.0,0.999);

    glPushMatrix(); // 0

    mat44f cammat = camori.getMatrix();
    mat44f cammat_inv = cammat.transpose();

    //glTranslatef(0.0,0.0,-40.0);
    glTranslatef(-eyetranslation, 0.0f, 0.0f);

    glMultMatrixf(cammat);

    glTranslatef(-campos.x, -campos.y, -campos.z);

    float lpos[] = { 0.2, 0.5, 1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    glColor3ub(255,255,255);

    glDisable(GL_LIGHTING);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD_SIGNED);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
    tex_detail->bind();
    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    float tgens[] = { 0.05, 0.0, 0.0, 0.0 };
    float tgent[] = { 0.0, 0.05, 0.0, 0.0 };
    glTexGenfv(GL_S,GL_OBJECT_PLANE,tgens);
    glTexGenfv(GL_T,GL_OBJECT_PLANE,tgent);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    // draw terrain
    game->terrain->render(campos, cammat_inv);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    if (renderowncar)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tex_shadow->bind();

        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);

        vec3f vpos = game->vehicle[0]->body->getPosition();
        vec3f forw = makevec3f(game->vehicle[0]->body->getOrientationMatrix().row[0]);
        float forwangle = atan2(forw.y, forw.x);
        game->terrain->drawSplat(vpos.x, vpos.y, 1.4f, forwangle + PI*0.5f);

        glBlendFunc(GL_ONE, GL_ZERO);
    }

    renderSky(cammat);

    glEnable(GL_LIGHTING);

    for (unsigned int v=0; v<game->vehicle.size(); ++v)
    {
        if (!renderowncar && v == 0) continue;

        PVehicle *vehic = game->vehicle[v];
        for (unsigned int i=0; i<vehic->part.size(); ++i)
        {
            renderVehiclePart(*vehic->type, vehic->part[i], vehic->type->part[i], 1.0f);
        }
    }

    glDisable(GL_LIGHTING);

    PGhost::GhostData ghostdata;
    std::string vehiclename = "";

    if (cfg.getEnableGhost() && ghost.getReplayData(ghostdata, vehiclename))
    {
        PVehiclePart vehiclepart;

        vehiclepart.ref_world.setPosition(ghostdata.pos);
        vehiclepart.ref_world.setOrientation(ghostdata.ori);
        vehiclepart.ref_world.updateMatrices();

        for (unsigned int i = 0; i < ghostdata.wheel.size(); ++i)
        {
            PVehicleWheel wheel;

            wheel.ref_world.setPosition(ghostdata.wheel[i].pos);
            wheel.ref_world.setOrientation(ghostdata.wheel[i].ori);
            wheel.ref_world.updateMatrices();
            vehiclepart.wheel.push_back(wheel);
        }

        // Theoretically there can be multiple vehicles with multiple parts.
        // However, the assumption is, that the model of the first part is relevant.
        for (unsigned int i = 0; i < game->vehiclechoices.size(); ++i) {
          if (game->vehiclechoices[i]->getName() == vehiclename) {
            renderVehiclePart(*game->vehiclechoices[i], vehiclepart,
                game->vehiclechoices[i]->part[0], 0.5f);
            break;
          }
        }
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glDisable(GL_TEXTURE_2D);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define RAINDROP_WIDTH          0.015
    const vec4f raindrop_col(0.5,0.5,0.5,0.4);

    vec3f offsetdrops = campos - campos_prev;

    for (unsigned int i = 0; i < rain.size(); i++)
    {
        vec3f tempv;
        const float prevlife = rain[i].prevlife;
        vec3f pt1 = rain[i].drop_pt + rain[i].drop_vect * prevlife + offsetdrops;
        vec3f pt2 = rain[i].drop_pt + rain[i].drop_vect * rain[i].life;
        vec3f zag = campos - rain[i].drop_pt;
        zag = zag.cross(rain[i].drop_vect);
        zag *= RAINDROP_WIDTH / zag.length();
        glBegin(GL_TRIANGLE_STRIP);
        glColor4f(raindrop_col[0],raindrop_col[1],raindrop_col[2],0.0);
        tempv = pt1 - zag;
        glVertex3fv(tempv);
        tempv = pt2 - zag;
        glVertex3fv(tempv);

        glColor4fv(raindrop_col);
        glVertex3fv(pt1);
        glVertex3fv(pt2);

        glColor4f(raindrop_col[0],raindrop_col[1],raindrop_col[2],0.0);
        tempv = pt1 + zag;
        glVertex3fv(tempv);
        tempv = pt2 + zag;
        glVertex3fv(tempv);
        glEnd();
    }

#define SNOWFLAKE_POINT_SIZE        3.0f
#define SNOWFLAKE_BOX_SIZE          0.175f

// NOTE: must be greater than 1.0f
#define SNOWFLAKE_MAXLIFE           4.5f

    GLfloat ops; // Original Point Size, for to be restored

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
    {
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glGetFloatv(GL_POINT_SIZE, &ops);
        glPointSize(SNOWFLAKE_POINT_SIZE);
    }
    else
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        tex_snowflake->bind();
    }

    for (const SnowFlake &sf: snowfall)
    {
        const vec3f pt = sf.drop_pt + sf.drop_vect * sf.life;
        GLfloat alpha;

        if (sf.life > SNOWFLAKE_MAXLIFE)
        {
            alpha = 0.0f;
        }
        else
        if (sf.life > 1.0f)
        {
#define ML      SNOWFLAKE_MAXLIFE
            // this equation ensures that snowflaks fade in
            alpha = (sf.life - ML) / (1 - ML);
#undef ML
        }
        else
            alpha = 1.0f;

        if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
        {
            glBegin(GL_POINTS);
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glVertex3fv(pt);
            glEnd();
        }
        else
        {
#define SBS     SNOWFLAKE_BOX_SIZE
            vec3f zag = campos - sf.drop_pt;

            zag = zag.cross(sf.drop_vect);
            zag.normalize();
            zag *= SBS;

            if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::square)
            {
                glBegin(GL_TRIANGLE_STRIP);
                glColor4f(1.0f, 1.0f, 1.0f, alpha);
                glVertex3f(pt.x,            pt.y,           pt.z                );
                glVertex3f(pt.x,            pt.y,           pt.z + zag.z + SBS  );
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z                );
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z + zag.z + SBS  );
                glEnd();
            }
            else // cfg_snowflaketype == SnowFlakeType::textured
            {
                glBegin(GL_TRIANGLE_STRIP);
                glColor4f(1.0f, 1.0f, 1.0f, alpha);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(pt.x,            pt.y,           pt.z                );
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(pt.x,            pt.y,           pt.z + zag.z + SBS  );
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z                );
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z + zag.z + SBS  );
                glEnd();
            }
#undef SBS
        }
    }

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
        glPointSize(ops); // restore original point size

    // disable textures
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    const vec4f checkpoint_col[3] =
    {
        vec4f(1.0f, 0.0f, 0.0f, 0.8f),  // 0 = next checkpoint
        vec4f(0.7f, 0.7f, 0.1f, 0.6f),  // 1 = checkpoint after next
        vec4f(0.2f, 0.8f, 0.2f, 0.4f)  // 2 = all other checkpoints
    };

    if (showcheckpoint)
    {
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            vec4f colr = checkpoint_col[2];

            if ((int)i == vehic->nextcp)
                colr = checkpoint_col[0];
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
                colr = checkpoint_col[1];

            glPushMatrix(); // 1
            glTranslatef(game->checkpt[i].pt.x, game->checkpt[i].pt.y, game->checkpt[i].pt.z);
            glScalef(25.0f, 25.0f, 1.0f);

#if 0 // Checkpoint style one
            glBegin(GL_TRIANGLE_STRIP);

            for (float a = 0.0f; a < 0.99f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * a);
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            for (float a = 1.0f; a < 2.01f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * (2.0f - a));
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            glEnd();
#else // Regular checkpoint style
            glBegin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            glEnd();
#endif
            glPopMatrix(); // 1
        }

// codriver checkpoints rendering
#ifdef INDEVEL

    // codriver checkpoints for debugging purposes
    const vec4f cdcheckpoint_col[3] =
    {
        {0.0f, 0.0f, 1.0f, 0.8f},       // 0 = next checkpoint
        {0.3f, 0.3f, 1.0f, 0.6f},       // 1 = checkpoint after next
        {0.6f, 0.6f, 1.0f, 0.4f}        // 2 = all other checkpoints
    };

        for (unsigned int i=0; i<game->codrivercheckpt.size(); i++)
        {
            vec4f colr = cdcheckpoint_col[2];

            if (game->cdcheckpt_ordered)
            {
                if ((int)i == vehic->nextcdcp)
                    colr = cdcheckpoint_col[0];
                else if ((int)i == (vehic->nextcdcp + 1) % (int)game->codrivercheckpt.size())
                    colr = cdcheckpoint_col[1];
            }
            else
                colr = cdcheckpoint_col[1];

            glPushMatrix(); // 1
            glTranslatef(game->codrivercheckpt[i].pt.x, game->codrivercheckpt[i].pt.y, game->codrivercheckpt[i].pt.z);
            glScalef(15.0f, 15.0f, 1.0f);

            glBegin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            glEnd();
            glPopMatrix(); // 1
        }
#endif
    }

    glEnable(GL_TEXTURE_2D);

    if (game->water.enabled)
        renderWater();

    if (psys_dirt != nullptr) // cfg_dirteffect == false
        getSSRender().render(psys_dirt);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE,GL_ZERO);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FOG);

    glDisable(GL_LIGHTING);

    glPopMatrix(); // 0

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); // 0
    glLoadIdentity();

    glOrtho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix(); // 1

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showui)
    {
        game->renderCodriverSigns();

        glPushMatrix(); // 2
        // position of rpm dial and needle
        //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.22f, 0.0f);
        glTranslatef( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.22f, 0.0f);
        glScalef(0.30f, 0.30f, 1.0f);

        tex_hud_revs->bind();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f);
        glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f);
        glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f);
        glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f);
        glVertex2f(1.0f,-1.0f);
        glEnd();

        // draw the needle of the RPM dial
        glRotatef(225.0f - vehic->getEngineRPM() * 15.0f / 1000.0f, 0.0f, 0.0f, 1.0f);
        tex_hud_revneedle->bind();
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix(); // 3
        glTranslatef(0.62f, 0.0f, 0.0f);
        glScalef(0.16f, 0.16f, 0.16f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f);
        glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f);
        glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f);
        glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f);
        glVertex2f(1.0f,-1.0f);
        glEnd();
        glPopMatrix(); // 3
        glDisable(GL_TEXTURE_2D);
        glPopMatrix(); // 2
    }

    // checkpoint pointing arrow thing
#if 0
    glPushMatrix(); // 2

    glTranslatef(0.0f, 0.8f, 0.0f);

    glScalef(0.2f, 0.2f, 0.2f);

    glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(DEGREES(nextcpangle), 0.0f, -1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(-1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glEnd();

    glPopMatrix(); // 2
#endif

    if (showmap)
    {
        // position and size of map
        //glViewport(getWidth() * (5.75f/100.f), getHeight() * (6.15f/100.f), getHeight()/3.5f, getHeight()/3.5f);
        glViewport(getWidth() * (2.5f/100.f), getHeight() * (2.5f/100.f), getHeight()/3.5f, getHeight()/3.5f);

        glPushMatrix(); // 2
        glScalef(hratio, vratio, 1.0f);

        if (game->terrain->getHUDMapTexture())
        {
            glEnable(GL_TEXTURE_2D);
            game->terrain->getHUDMapTexture()->bind();
        }

        glMatrixMode(GL_TEXTURE);
        glPushMatrix(); // 3
        float scalefac = 1.0f / game->terrain->getMapSize();
        glScalef(scalefac, scalefac, 1.0f);
        glTranslatef(campos.x, campos.y, 0.0f);
        glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
        glScalef(1.0f / 0.003f, 1.0f / 0.003f, 1.0f);

        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(-1.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(-1.0f, -1.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, -1.0f);
        glVertex2f(1.0f, -1.0f);
        glEnd();

        glPopMatrix(); // 3
        glMatrixMode(GL_MODELVIEW);

        glDisable(GL_TEXTURE_2D);

        glPushMatrix(); // 3
        glScalef(0.003f, 0.003f, 1.0f);
        glRotatef(DEGREES(-camera_angle), 0.0f, 0.0f, 1.0f);
        glTranslatef(-campos.x, -campos.y, 0.0f);
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            glPushMatrix(); // 4
            vec3f vpos = game->checkpt[i].pt;
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            vec4f colr = checkpoint_col[2];
            if ((int)i == vehic->nextcp)
            {
                float sc = 1.5f + sinf(cprotate * 10.0f) * 0.5f;
                glScalef(sc, sc, 1.0f);
                colr = checkpoint_col[0];
            }
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
            {
                colr = checkpoint_col[1];
            }
            glBegin(GL_TRIANGLE_FAN);
            glColor4fv(colr);
            glVertex2f(0.0f, 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            //glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix(); // 4
        }
        for (unsigned int i=0; i<game->vehicle.size(); i++)
        {
            glPushMatrix(); // 4
            vec3f vpos = game->vehicle[i]->body->getPosition();
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix(); // 4
        }
        glPopMatrix(); // 3

        glPopMatrix(); // 2

        glViewport(0, 0, getWidth(), getHeight());
    }

    glEnable(GL_TEXTURE_2D);

    if (showui)
    {
        // Work-around for the "TIME" label once penalty time is displayed
        float time_offset = 0.0f;
        /*
        tex_hud_gear->bind();
        glPushMatrix(); // 2

        glTranslatef(1.0f, 0.35f, 0.0f);
        glScalef(0.2f, 0.2f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f); glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f); glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,-1.0f);
        glEnd();

        glPopMatrix(); // 2
        */

        tex_fontSourceCodeOutlined->bind();

        // time counter
        glPushMatrix(); // 2

        // time position (other time strings inherit this position)
        // -hratio is left border, 0 is center, +hratio is right
        // hratio * (1/50) gives 1% of the entire width
        // +vratio is top border, 0 is middle, -vratio is bottom
        glTranslatef(-hratio + hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);

        // time label
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glScalef(0.125f, 0.125f, 1.0f);
        if (game->gamestate == Gamestate::finished)
        {
            getSSRender().drawText(
                PUtil::formatTime(game->coursetime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else if (game->coursetime < game->cptime + 1.50f)
        {
            getSSRender().drawText(
                PUtil::formatTime(game->cptime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else if (game->coursetime < game->cptime + 3.50f)
        {
            float a = (((game->cptime + 3.50f) - game->coursetime) / 2);
            glColor4f(1.0f, 1.0f, 1.0f, a);
            getSSRender().drawText(
                PUtil::formatTime(game->cptime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else
        {
            getSSRender().drawText(
                PUtil::formatTime(game->coursetime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }

        // show target time
        glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
        glTranslatef(0.0f, -0.8f, 0.0f);
        getSSRender().drawText(PUtil::formatTime(game->targettime), PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        {
            // show the time penalty if there is any
            const float timepen = game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

            if (timepen >= 0.1f)
            {
                glColor4f(1.0f, 1.0f, 0.5f, 1.0f);
                glTranslatef(0.0f, -0.8f, 0.0f);
                getSSRender().drawText(PUtil::formatTime(timepen) + '+', PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
                time_offset = 0.8;
            }
        }

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTranslatef(0.0f, 1.32f + time_offset, 0.0f);
        glScalef(0.65f, 0.65f, 1.0f);
        getSSRender().drawText("TIME", PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        glPopMatrix(); // 2

        // show Next/Total checkpoints
        {
            // checkpoint counter
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            const std::string totalcp = std::to_string(game->checkpt.size());
            const std::string nextcp = std::to_string(vehic->nextcp);

            // checkpoint position
            glTranslatef(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);

              if (game->getFinishState() != Gamefinish::not_finished)
                  getSSRender().drawText(totalcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
              else
                  getSSRender().drawText(nextcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            // checkpoint label
            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("CKPT", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glPopMatrix(); // 2
        }

        // show Current/Total laps
        if (game->number_of_laps > 1)
        {
            const std::string currentlap = std::to_string(vehic->currentlap);
            const std::string number_of_laps = std::to_string(game->number_of_laps);

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            glTranslatef(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f) - 0.20f, 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(number_of_laps + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
            else
                getSSRender().drawText(currentlap + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("LAP", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glPopMatrix(); // 2
        }

        if (cfg.getEnableFps())
        {
            std::stringstream stream;

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            glTranslatef(0.0f, vratio - vratio * (5.5f/50.f), 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);
            stream << std::fixed << std::setprecision(1) << fps;
            getSSRender().drawText(stream.str(), PTEXT_HZA_CENTER | PTEXT_VTA_TOP);

            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("FPS", PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            glPopMatrix(); // 2
        }

#ifdef INDEVEL
        // show codriver checkpoint text (the pace notes)
        if (!game->codrivercheckpt.empty() && vehic->nextcdcp != 0)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix(); // 2
            glTranslatef(0.0f, 0.3f, 0.0f);
            glScalef(0.1f, 0.1f, 1.0f);
            getSSRender().drawText(game->codrivercheckpt[vehic->nextcdcp - 1].notes, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix(); // 2
        }
#endif

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        tex_fontSourceCodeBold->bind();

        // show current gear and speed
        {
            // gear number
            const int gear = vehic->getCurrentGear();
            const std::string buff = (gear >= 0) ? PUtil::formatInt(gear + 1, 1) : "R";

            glPushMatrix(); // 2
            // position of gear & speed number & label
            //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.21f, 0.0f);
            glTranslatef( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.21f, 0.0f);
            glScalef(0.20f, 0.20f, 1.0f);
            getSSRender().drawText(buff, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);

            // speed number
            const int speed = std::fabs(vehic->getWheelSpeed()) * cfg.getHudSpeedoMpsSpeedMult();
            std::string speedstr = std::to_string(speed);

            //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTranslatef(1.1f, -0.625f, 0.0f);
            glScalef(0.5f, 0.5f, 1.0f);
            getSSRender().drawText(speedstr, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            // speed label
            glTranslatef(0.0f, -0.82f, 0.0f);
            glScalef(0.5f, 0.5f, 1.0f);

            if (cfg.getSpeedUnit() == PConfig::Speedunit::mph)
                getSSRender().drawText("MPH", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
            else
                getSSRender().drawText("km/h", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            glPopMatrix(); // 2
        }

        renderDamageIndicatorGroup();

#ifndef NDEBUG
        // draw revs for debugging
        glPushMatrix(); // 2
        glTranslatef(1.17f, 0.52f, 0.0f);
        glScalef(0.2f, 0.2f, 1.0f);
        getSSRender().drawText(std::to_string(vehic->getEngineRPM()), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
#endif

#ifndef NDEBUG
        // draw real time penalty for debugging
        glPushMatrix(); // 2
        glScalef(0.1f, 0.1f, 1.0f);
        glTranslatef(0.0f, -4.0f, 0.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        tex_fontSourceCodeOutlined->bind();
        getSSRender().drawText(std::string("true time penalty: ") +
            std::to_string(game->getOffroadTime() * game->offroadtime_penalty_multiplier),
            PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
#endif
    }

    tex_fontSourceCodeShadowed->bind();

    // draw "off road" warning sign and text
    if (game->isRacing())
    {
        //const vec3f bodypos = vehic->part[0].ref_world.getPosition();
        const vec3f bodypos = vehic->body->getPosition();

        if (!game->terrain->getRmapOnRoad(bodypos))
        {
            glPushMatrix(); // 2
            glLoadIdentity();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glScalef(0.25f, 0.25f, 1.0f);
            tex_hud_offroad->bind();
            glBegin(GL_QUADS);
                glTexCoord2f(   1.0f,   1.0f);
                glVertex2f(     1.0f,   1.0f);
                glTexCoord2f(   0.0f,   1.0f);
                glVertex2f(    -1.0f,   1.0f);
                glTexCoord2f(   0.0f,   0.0f);
                glVertex2f(    -1.0f,  -1.0f);
                glTexCoord2f(   1.0f,   0.0f);
                glVertex2f(     1.0f,  -1.0f);
            glEnd();
            glPopMatrix(); // 2
            glPushMatrix(); // 2
            glScalef(0.1f, 0.1f, 1.0f);
            glTranslatef(0.0f, -2.5f, 0.0f);
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            tex_fontSourceCodeOutlined->bind();
            getSSRender().drawText(
                std::to_string(static_cast<int> (game->getOffroadTime() * game->offroadtime_penalty_multiplier)) +
                " seconds",
                PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            glPopMatrix(); // 2
        }
    }

    // draw terrain info for debugging
    #ifdef INDEVEL
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        const TerrainType tt = game->terrain->getRoadSurface(wheelpos);
        const rgbcolor c = PUtil::getTerrainColor(tt);
        const std::string s = PUtil::getTerrainInfo(tt);

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.5f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);

        if (tt != TerrainType::Unknown)
        {
            const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

            glPushMatrix(); // 3
            glDisable(GL_TEXTURE_2D);
            glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -1.0f, 0.0f);
            glBegin(GL_TRIANGLE_STRIP);
                glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
                glVertex2f(-0.2f,   0.0f);
                glVertex2f(endx,    0.0f);
                glVertex2f(-0.2f,   1.1f);
                glVertex2f(endx,    1.1f);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glPopMatrix(); // 3
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif

    // draw if we're on road for debugging
    //#ifdef INDEVEL
    #if 0
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        std::string s;
        rgbcolor c;

        if (game->terrain->getRmapOnRoad(wheelpos))
        {
            c = rgbcolor(0xFF, 0xFF, 0xFF);
            s = "on the road";
        }
        else
        {
            c = rgbcolor(0x00, 0x00, 0x00);
            s = "off-road";
        }

        const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.25f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);
        glPushMatrix(); // 3
        glDisable(GL_TEXTURE_2D);
        glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -1.0f, 0.0f);
        glBegin(GL_TRIANGLE_STRIP);
            glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
            glVertex2f(-0.2f,   0.0f);
            glVertex2f(endx,    0.0f);
            glVertex2f(-0.2f,   1.1f);
            glVertex2f(endx,    1.1f);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glPopMatrix(); // 3
        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif
    {
        tex_fontSourceCodeOutlined->bind();

        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.2f, 0.0f);
        glScalef(0.6f, 0.6f, 1.0f);
        if (game->gamestate == Gamestate::countdown)
        {
            float sizer = fmodf(game->othertime, 1.0f) + 0.5f;
            glScalef(sizer, sizer, 1.0f);
            getSSRender().drawText(
                PUtil::formatInt(((int)game->othertime + 1), 1),
                PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->gamestate == Gamestate::finished)
        {
            if (game->getFinishState() == Gamefinish::pass)
            {
                glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
                getSSRender().drawText("WIN", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                glScalef(0.5f, 0.5f, 1.0f);
                glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
                getSSRender().drawText("TIME EXCEEDED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
        }
        else if (game->coursetime < 1.0f)
        {
            glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->coursetime < 2.0f)
        {
            float a = 1.0f - (game->coursetime - 1.0f);
            glColor4f(0.5f, 1.0f, 0.5f, a);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        glPopMatrix(); // 2

        if (game->gamestate == Gamestate::countdown)
        {
            glPushMatrix(); // 2
            glTranslatef(0.0f, 0.6f, 0.0f);
            glScalef(0.08f, 0.08f, 1.0f);
            if (game->othertime < 1.0f)
            {
                glColor4f(1.0f, 1.0f, 1.0f, game->othertime);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            glPopMatrix(); // 2
        }

        if (pauserace)
        {
            glPushMatrix(); // 2
            glColor4f(0.25f, 0.25f, 1.0f, 1.0f);
            glScalef(0.25f, 0.25f, 1.0f);
            getSSRender().drawText("PAUSED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix(); // 2
        }
    }

    glPopMatrix(); // 1

    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // 0
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_ONE, GL_ZERO);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void MainApp::renderDamageIndicator(
        const PTexture *texture, float posx, float posy, float scalex, float scaley, float damage)
{
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;

    if (damage > 1.0f)
        damage = 1.0f;

    if (damage < 0.0f)
    {
        red = 1.0;
        green = 1.0;
        blue = 1.0;
    }
    else if (damage < 0.5f)
    {
        red = 2.0f * damage;
        green = 1.0;
    }
    else
    {
        red = 1.0;
        green = 2.0 * (1.0 - damage);
    }

    glPushMatrix(); // 2

    glTranslatef(posx, posy, 0.0f);
    glScalef(scalex, scaley, 1.0f);
    texture->bind();
    glColor4f(red, green, blue, 0.5f);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glTexCoord2f(0.0f,1.0f);
    glVertex2f(-1.0f,1.0f);
    glTexCoord2f(0.0f,0.0f);
    glVertex2f(-1.0f,-1.0f);
    glTexCoord2f(1.0f,0.0f);
    glVertex2f(1.0f,-1.0f);
    glEnd();

    glPopMatrix(); // 2
}

void MainApp::renderDamageIndicatorGroup()
{
    // Theoretically there can be multiple vehicles with multiple parts.
    // However, the assumption is, that the damage of the first part of the first vehicle is relevant.
    PVehiclePart &part = game->vehicle[0]->part[0];

    renderDamageIndicator(
        tex_damage_front_left, hratio * 45.0f/50.0f - 0.075f, -vratio * 32.5f/50.0f + 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageFrontLeft));
    renderDamageIndicator(
        tex_damage_front_right, hratio * 45.0f/50.0f - 0.025f, -vratio * 32.5f/50.0f + 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageFrontRight));
    renderDamageIndicator(
        tex_damage_rear_left, hratio * 45.0f/50.0f - 0.075f, -vratio * 32.5f/50.0f - 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageRearLeft));
    renderDamageIndicator(
        tex_damage_rear_right, hratio * 45.0f/50.0f - 0.025f, -vratio * 32.5f/50.0f - 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageRearRight));
}

void MainApp::renderVehiclePart(const PVehicleType &type, const PVehiclePart &part,
    const PVehicleTypePart &typepart, float alpha)
{
    if (typepart.model)
    {
        glPushMatrix();

        vec3f vpos = part.ref_world.getPosition();
        glTranslatef(vpos.x, vpos.y, vpos.z);

        mat44f vorim = part.ref_world.getInverseOrientationMatrix();
        glMultMatrixf(vorim);

        float scale = typepart.scale;
        glScalef(scale,scale,scale);

        drawModel(*typepart.model, alpha);

        glPopMatrix();
    }

    if (type.wheelmodel)
    {
        for (unsigned int i=0; i<typepart.wheel.size(); ++i)
        {
            glPushMatrix();

            vec3f wpos = part.wheel[i].ref_world.getPosition();
            glTranslatef(wpos.x,wpos.y,wpos.z);

            mat44f worim = part.wheel[i].ref_world.getInverseOrientationMatrix();
            glMultMatrixf(worim);

            float scale = type.wheelscale * typepart.wheel[i].radius;
            glScalef(scale,scale,scale);

            drawModel(*type.wheelmodel, alpha);

            glPopMatrix();
        }
    }
}
===
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "damage.h"
#include "main.h"
#include "vehicle.h"
#include <cmath>

void MainApp::resize()
{
    glClearColor(1.0,1.0,1.0,1.0);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ZERO);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    glEnable(GL_CULL_FACE);

	glViewport(0, 0, getWidth(), getHeight());

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    const GLfloat ambcol[] = {0.1f, 0.1f, 0.1f, 0.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambcol);

    float white[] = { 1.0,1.0,1.0,1.0 };
    //float black[] = { 0.0,0.0,0.0,1.0 };
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,white);

    float spec[] = { 0.3f, 0.5f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 6.0f);

    float litcol[] = { 0.6,0.6,0.6,0.0 };
    glLightfv(GL_LIGHT0,GL_DIFFUSE,litcol);
    glLightfv(GL_LIGHT0,GL_SPECULAR,litcol);

    glEnable(GL_NORMALIZE);
}

void drawBlades(float radius, float ang, float trace)
{
    float invtrace = 1.0 / trace;
    glPushMatrix();
    glScalef(radius, radius, 1.0);
    for (float ba=0; ba<PI*2.0-0.01; ba+=PI/2.0)
    {
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.1,0.1,0.1,0.24 * invtrace);
        glVertex2f(0.0,0.0);
        glColor4f(0.1,0.1,0.1,0.06 * invtrace);
        int num = (int)(trace / 0.1);
        if (num < 2) num = 2;
        float mult = trace / (float)(num-1);
        float angadd = ba + ang;
        for (int i=0; i<num; ++i)
        {
            float a = (float)i * mult + angadd;
            glVertex2f(cos(a),sin(a));
        }
        glEnd();
    }
    glPopMatrix();
}

void MainApp::renderWater()
{
    tex_water->bind();
    {
        float tgens[] = { 0.5,0,0,0 };
        float tgent[] = { 0,0.5,0,0 };
        glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
        glTexGenfv(GL_S,GL_OBJECT_PLANE,tgens);
        glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
        glTexGenfv(GL_T,GL_OBJECT_PLANE,tgent);
    }
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glPushMatrix();
    glScalef(20.0,20.0,1.0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    {
        int minx = (int)(campos.x / 20.0)-20,
            maxx = minx + 40,
            miny = (int)(campos.y / 20.0)-20,
            maxy = miny + 40;
        for (int y=miny; y<maxy; ++y)
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (int x=minx; x<=maxx; ++x)
            {
                float maxalpha = 0.5f;

                if (game->water.useralpha)
                    maxalpha = game->water.alpha;

                if (game->water.fixedalpha)
                {
                    glColor4f(1.0f, 1.0f, 1.0f, maxalpha);
                    glVertex3f(x, y+1, game->water.height);
                    glVertex3f(x, y, game->water.height);
                }
                else
                {
                    float ht,alpha;
                    ht = game->terrain->getHeight((x)*20.0,(y+1)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    glColor4f(1.0,1.0,1.0,alpha);
                    glVertex3f(x, y+1, game->water.height);
                    ht = game->terrain->getHeight((x)*20.0,(y)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    glColor4f(1.0,1.0,1.0,alpha);
                    glVertex3f(x, y, game->water.height);
                }
            }
            glEnd();
        }
    }
    glPopMatrix();
    glBlendFunc(GL_ONE,GL_ZERO);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

void MainApp::renderSky(const mat44f &cammat)
{
    glFogf(GL_FOG_DENSITY, game->weather.fog.density_sky);
    glDepthRange(0.999,1.0);
    glDisable(GL_CULL_FACE);
    glPushMatrix(); // 1
    glLoadIdentity();
    glMultMatrixf(cammat);
    tex_sky[0]->bind();
#define CLRANGE     10
#define CLFACTOR    0.02//0.014
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(cloudscroll,0.0,0.0);
    glRotatef(30.0,0.0,0.0,1.0);
    glScalef(0.4,0.4,1.0);
    for (int y=-CLRANGE; y<CLRANGE; y++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x=-CLRANGE; x<CLRANGE+1; x++)
        {
            glTexCoord2i(x,y);
            glVertex3f(x,y,0.3-(x*x+y*y)*CLFACTOR);
            glTexCoord2i(x,y+1);
            glVertex3f(x,y+1,0.3-(x*x+(y+1)*(y+1))*CLFACTOR);
        }
        glEnd();
    }
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(); // 1
    glEnable(GL_CULL_FACE);
    glDepthRange(0.0,0.999);
    glFogf(GL_FOG_DENSITY, game->weather.fog.density);
}

void MainApp::render(float eyetranslation)
{
    switch (appstate)
    {
        case AS_LOAD_1:
            renderStateLoading(eyetranslation);
            break;

        case AS_LOAD_2:
        case AS_LOAD_3:
            break;

        case AS_LEVEL_SCREEN:
            renderStateLevel(eyetranslation);
            break;

        case AS_CHOOSE_VEHICLE:
            renderStateChoose(eyetranslation);
            break;

        case AS_IN_GAME:
            renderStateGame(eyetranslation);
            break;

        case AS_END_SCREEN:
            renderStateEnd(eyetranslation);
            break;
    }

    glFinish();
}

void MainApp::renderStateLoading(float eyetranslation)
{
    UNREFERENCED_PARAMETER(eyetranslation);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    tex_splash_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();

    tex_loading_screen->bind();

    GLfloat logovratio = static_cast<float> (getWidth()) / getHeight();
    GLfloat logohratio = static_cast<float> (getHeight()) / getWidth();

    // FIXME: nasty, nasty code
    if (logovratio > 1.0f)
        logohratio = 1.0f;
    else
    if (logohratio > 1.0f)
        logovratio = 1.0f;

#define LOGO_VRATIO     (logovratio/3.5)
#define LOGO_HRATIO     (logohratio/3.5)
    glBegin(GL_QUADS);
      glTexCoord2f(1.0f, 1.0f); glVertex2f( LOGO_HRATIO,  LOGO_VRATIO);
      glTexCoord2f(0.0f, 1.0f); glVertex2f(-LOGO_HRATIO,  LOGO_VRATIO);
      glTexCoord2f(0.0f, 0.0f); glVertex2f(-LOGO_HRATIO, -LOGO_VRATIO);
      glTexCoord2f(1.0f, 0.0f); glVertex2f( LOGO_HRATIO, -LOGO_VRATIO);
    glEnd();
#undef LOGO_VRATIO
#undef LOGO_HRATIO

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

const char *creditstext[] =
{
    "TrackRS " PACKAGE_VERSION,
    "",
    "Created by",
    "Saad AIT YAHIA",
    "@github: Saad-programmer",
    "",
    "",
    "",
    "",
    "",
    "Thanks for playing TrackRS, see you next time"
};

#define NUMCREDITSTRINGS (sizeof(creditstext) / sizeof(char*))

void MainApp::renderStateEnd(float eyetranslation)
{
    eyetranslation = eyetranslation;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    tex_end_screen->bind();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_LIGHTING);
    glBlendFunc(GL_ONE, GL_ZERO);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(1.0f,off_u); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(0.0f,off_u); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(0.0f,off_l); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(1.0f,off_l); glVertex2f(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      glTexCoord2f(off_u,1.0f); glVertex2f(1.0f, 1.0f);
      glTexCoord2f(off_l,1.0f); glVertex2f(-1.0f, 1.0f);
      glTexCoord2f(off_l,0.0f); glVertex2f(-1.0f, -1.0f);
      glTexCoord2f(off_u,0.0f); glVertex2f(1.0f, -1.0f);
    }
    glEnd();

    tex_fontSourceCodeOutlined->bind();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    //glOrtho(-1, 1, -1, 1, -1, 1);
    //glOrtho(800, 0, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();

    float scroll = splashtimeout;
    const float maxscroll = (float)(NUMCREDITSTRINGS - 1) * 2.0f;
    RANGEADJUST(scroll, 0.0f, 0.9f, -10.0f, maxscroll);
    CLAMP_UPPER(scroll, maxscroll);

    glScalef(0.1f, 0.1f, 1.0f);

    glTranslatef(0.0f, scroll, 0.0f);

    for (int i = 0; i < (int)NUMCREDITSTRINGS; i++)
    {
        float level = fabsf(scroll + (float)i * -2.0f);
        RANGEADJUST(level, 0.0f, 9.0f, 3.0f, 0.0f);

        if (level > 0.0f)
        {
            CLAMP_UPPER(level, 1.0f);

            glPushMatrix();
            glTranslatef(0.0f, (float)i * -2.0f, 0.0f);

            float enlarge = 1.0f;

#if 1
            if (splashtimeout > 0.9f)
            {
                float amt = (splashtimeout - 0.9f) * 10.0f;
                float amt2 = amt * amt;

                enlarge += amt2 / ((1.0001f - amt) * (1.0001f - amt));
                level -= amt2;
            }
#endif

            glScalef(enlarge, enlarge, 0.0f);
            glColor4f(1.0f, 1.0f, 1.0f, level);

            getSSRender().drawText(creditstext[i], PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix();
        }
    }

    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// render the car selection menu
void MainApp::renderStateChoose(float eyetranslation)
{
    PVehicleType *vtype = game->vehiclechoices[choose_type];

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glMatrixMode(GL_PROJECTION);

  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

  // draw background image

  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);

  tex_splash_screen->bind();

  //glColor4f(0.0f, 0.0f, 0.2f, 1.0f); // make image dark blue
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // use image's normal colors
  //glColor4f(0.5f, 0.5f, 0.5f, 1.0f); // make image darker

  // Old-school rally Ken Burns effect
  float zoom = 0.82f + 0.04f * sinf(menu_time * 0.2f);
  float dx = 0.04f * cosf(menu_time * 0.15f);
  float dy = 0.04f * sinf(menu_time * 0.11f);

  auto map_tx = [&](float tx) { return 0.5f + dx + (tx - 0.5f) * zoom; };
  auto map_ty = [&](float ty) { return 0.5f + dy + (ty - 0.5f) * zoom; };

  glBegin(GL_QUADS);
  // the background image is square and cut out a piece based on aspect ratio
  // -------- if aspect ratio is larger than 4:3
  // if aspect ratio is larger than 1:1
  if ((float)getWidth()/(float)getHeight() > 1.0f)
  {
    // lower and upper offset based on aspect ratio
    float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
    float off_u = 1 - off_l;
    glTexCoord2f(map_tx(1.0f), map_ty(off_u)); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(map_tx(0.0f), map_ty(off_u)); glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(map_tx(0.0f), map_ty(off_l)); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(map_tx(1.0f), map_ty(off_l)); glVertex2f(1.0f, -1.0f);
  }
  // other cases (including 4:3, in which case off_l and off_u are = 1)
  else
  {
    float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
    float off_u = 1 - off_l;
    glTexCoord2f(map_tx(off_u), map_ty(1.0f)); glVertex2f(1.0f, 1.0f);
    glTexCoord2f(map_tx(off_l), map_ty(1.0f)); glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(map_tx(off_l), map_ty(0.0f)); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(map_tx(off_u), map_ty(0.0f)); glVertex2f(1.0f, -1.0f);
  }
  glEnd();

  // Draw animated gradient, scanlines, and particles
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 1. Dark colored gradient overlay
  glBegin(GL_QUADS);
  // Top: deep dark blue
  glColor4f(0.02f, 0.05f, 0.12f, 0.65f + 0.05f * sinf(menu_time * 0.5f));
  glVertex2f(-1.0f, 1.0f);
  glVertex2f(1.0f, 1.0f);
  // Bottom: dark warm charcoal/reddish-purple glow
  float r_glow = 0.08f + 0.04f * sinf(menu_time * 0.3f);
  float b_glow = 0.04f + 0.02f * cosf(menu_time * 0.4f);
  glColor4f(r_glow, 0.01f, b_glow, 0.85f);
  glVertex2f(1.0f, -1.0f);
  glVertex2f(-1.0f, -1.0f);
  glEnd();

  // 2. Subtle scrolling retro CRT scanlines
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  for (int j = 0; j < 40; ++j) {
      float y = -1.0f + (j / 20.0f);
      float scan_alpha = 0.03f + 0.01f * sinf(menu_time * 2.0f + y * 5.0f);
      glColor4f(1.0f, 1.0f, 1.0f, scan_alpha);
      glVertex2f(-1.0f, y);
      glVertex2f(1.0f, y);
  }
  glEnd();

  // 3. Stateless speed particles/spark drift
  glBegin(GL_QUADS);
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
          glColor4f(1.0f, 0.65f, 0.15f, spark_alpha);
      } else {
          glColor4f(0.8f, 0.9f, 1.0f, spark_alpha);
      }
      
      glVertex2f(x - length, y + size * 0.5f);
      glVertex2f(x + length, y + size * 0.5f);
      glVertex2f(x + length, y - size * 0.5f);
      glVertex2f(x - length, y - size * 0.5f);
  }
  glEnd();

  glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    glMatrixMode(GL_MODELVIEW);


    glPushMatrix(); // 0

//    glTranslatef(-eyetranslation, 0.5f, -5.0f);
    glTranslatef(-eyetranslation, 0.9f, -5.0f);
    glRotatef(28.0f, 1.0f, 0.0f, 0.0f);

    glDisable(GL_FOG);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    vec4f lpos = vec4f(0.0f, 1.0f, 0.0f, 0.0f);
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    //float tmp = 1.0f;
    //float tmp = sinf(choose_spin * 2.0f) * 0.5f;
    float tmp = cosf(choose_spin * 2.0f) * 0.5f;
    tmp += choose_spin;
    glRotatef(90.0f, -1.0f, 0.0f, 0.0f);
    glRotatef(DEGREES(tmp), 0.0f, 0.0f, 1.0f);

    // render vehicle
    for (unsigned int i=0; i<vtype->part.size(); ++i)
    {
		glPushMatrix(); // 1

		vec3f vpos = vtype->part[i].render_ref_local.getPosition();
		glTranslatef(vpos.x, vpos.y, vpos.z);

		mat44f vorim = vtype->part[i].render_ref_local.getInverseOrientationMatrix();
		glMultMatrixf(vorim);
		if (vtype->part[i].model)
		{
			glPushMatrix(); // 2

			float scale = vtype->part[i].scale;
			glScalef(scale,scale,scale);
			drawModel(*vtype->part[i].model, 1.0f);

			glPopMatrix(); // 2
		}

		// render wheels
		if (vtype->wheelmodel)
		{
			for (unsigned int j=0; j<vtype->part[i].wheel.size(); j++)
			{

				glPushMatrix(); // 2

				vec3f &wpos = vtype->part[i].wheel[j].pt;
				glTranslatef(wpos.x, wpos.y, wpos.z);

				float scale = vtype->wheelscale * vtype->part[i].wheel[j].radius;
				glScalef(scale,scale,scale);

				drawModel(*vtype->wheelmodel, 1.0f);

				glPopMatrix(); // 2
			}
		}

		glPopMatrix(); // 1
	}

    glPopMatrix(); // 0

    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    // use the same colors as the menu
    const GuiWidgetColors gwc = gui.getColors();

    tex_fontSourceCodeShadowed->bind();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glPushMatrix(); // 0

    const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

    glOrtho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

    glPushMatrix(); // 1
    glTranslatef(10.0f, 570.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("TrackRS", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(790.0f, 570.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(
        "car selection " + std::to_string(choose_type + 1) + '/' + std::to_string(game->vehiclechoices.size()),
        PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(100.0f, 230.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.header.x, gwc.header.y, gwc.header.z, gwc.header.w);
    getSSRender().drawText(vtype->proper_name.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(100.0f, 200.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->proper_class.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 230.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Weight (Kg)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 190.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Engine (BHP)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 150.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Wheel drive", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(500.0f, 110.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Roadholding", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 230.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(std::to_string(static_cast<int>(vtype->mass)), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 190.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_enginepower, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 150.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_wheeldrive, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    glPushMatrix(); // 1
    glTranslatef(520.0f, 110.0f, 0.0f);
    glScalef(30.0f, 30.0f, 1.0f);
    glColor4f(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_roadholding, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    std::string racename;

    if (lss.state == AM_TOP_EVT_PREP || lss.state == AM_TOP_PRAC_SEL_PREP)
        racename = events[lss.currentevent].name + ": " + events[lss.currentevent].levels[lss.currentlevel].name;
    else
    if (lss.state == AM_TOP_LVL_PREP)
        racename = levels[lss.currentlevel].name;

    glPushMatrix(); // 1
    glTranslatef(400.0f, 30.0f, 0.0f);
    glScalef(20.0f, 20.0f, 1.0f);
    glColor4f(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(racename, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    glPopMatrix(); // 1

    if (vtype->getLocked()) {
      const std::string unlockevent = getVehicleUnlockEvent(vtype->getName());

      glPushMatrix(); // 1
      glTranslatef(400.0f, 425.0f, 0.0f);
      glScalef(40.0f, 40.0f, 1.0f);
      glColor4f(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
      getSSRender().drawText("Locked", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
      glPopMatrix(); // 1

      if (unlockevent != "") {
        glPushMatrix(); // 1
        glTranslatef(400.0f, 375.0f, 0.0f);
        glScalef(20.0f, 20.0f, 20.0f);
        glColor4f(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
        getSSRender().drawText("Complete event " + unlockevent + " to unlock", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        glPopMatrix(); // 1
      }
    }

    glPopMatrix(); // 0

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void MainApp::renderStateGame(float eyetranslation)
{
    PVehicle *vehic = game->vehicle[0];

    glClear(GL_DEPTH_BUFFER_BIT);
    //glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    glMatrixMode(GL_MODELVIEW);

    glColor3f(1.0,1.0,1.0);

    vec4f fogcolor(game->weather.fog.color, 1.0f);
    glFogfv(GL_FOG_COLOR, fogcolor);

    glDepthRange(0.0,0.999);

    glPushMatrix(); // 0

    mat44f cammat = camori.getMatrix();
    mat44f cammat_inv = cammat.transpose();

    //glTranslatef(0.0,0.0,-40.0);
    glTranslatef(-eyetranslation, 0.0f, 0.0f);

    glMultMatrixf(cammat);

    glTranslatef(-campos.x, -campos.y, -campos.z);

    float lpos[] = { 0.2, 0.5, 1.0, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);

    glColor3ub(255,255,255);

    glDisable(GL_LIGHTING);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB,GL_ADD_SIGNED);
    glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA,GL_MODULATE);
    tex_detail->bind();
    glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
    float tgens[] = { 0.05, 0.0, 0.0, 0.0 };
    float tgent[] = { 0.0, 0.05, 0.0, 0.0 };
    glTexGenfv(GL_S,GL_OBJECT_PLANE,tgens);
    glTexGenfv(GL_T,GL_OBJECT_PLANE,tgent);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    // draw terrain
    game->terrain->render(campos, cammat_inv);

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);

    if (renderowncar)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tex_shadow->bind();

        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);

        vec3f vpos = game->vehicle[0]->body->getPosition();
        vec3f forw = makevec3f(game->vehicle[0]->body->getOrientationMatrix().row[0]);
        float forwangle = atan2(forw.y, forw.x);
        game->terrain->drawSplat(vpos.x, vpos.y, 1.4f, forwangle + PI*0.5f);

        glBlendFunc(GL_ONE, GL_ZERO);
    }

    renderSky(cammat);

    glEnable(GL_LIGHTING);

    for (unsigned int v=0; v<game->vehicle.size(); ++v)
    {
        if (!renderowncar && v == 0) continue;

        PVehicle *vehic = game->vehicle[v];
        for (unsigned int i=0; i<vehic->part.size(); ++i)
        {
            renderVehiclePart(*vehic->type, vehic->part[i], vehic->type->part[i], 1.0f);
        }
    }

    glDisable(GL_LIGHTING);

    PGhost::GhostData ghostdata;
    std::string vehiclename = "";

    if (cfg.getEnableGhost() && ghost.getReplayData(ghostdata, vehiclename))
    {
        PVehiclePart vehiclepart;

        vehiclepart.ref_world.setPosition(ghostdata.pos);
        vehiclepart.ref_world.setOrientation(ghostdata.ori);
        vehiclepart.ref_world.updateMatrices();

        for (unsigned int i = 0; i < ghostdata.wheel.size(); ++i)
        {
            PVehicleWheel wheel;

            wheel.ref_world.setPosition(ghostdata.wheel[i].pos);
            wheel.ref_world.setOrientation(ghostdata.wheel[i].ori);
            wheel.ref_world.updateMatrices();
            vehiclepart.wheel.push_back(wheel);
        }

        // Theoretically there can be multiple vehicles with multiple parts.
        // However, the assumption is, that the model of the first part is relevant.
        for (unsigned int i = 0; i < game->vehiclechoices.size(); ++i) {
          if (game->vehiclechoices[i]->getName() == vehiclename) {
            renderVehiclePart(*game->vehiclechoices[i], vehiclepart,
                game->vehiclechoices[i]->part[0], 0.5f);
            break;
          }
        }
    }

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glDisable(GL_TEXTURE_2D);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define RAINDROP_WIDTH          0.015
    const vec4f raindrop_col(0.5,0.5,0.5,0.4);

    vec3f offsetdrops = campos - campos_prev;

    for (unsigned int i = 0; i < rain.size(); i++)
    {
        vec3f tempv;
        const float prevlife = rain[i].prevlife;
        vec3f pt1 = rain[i].drop_pt + rain[i].drop_vect * prevlife + offsetdrops;
        vec3f pt2 = rain[i].drop_pt + rain[i].drop_vect * rain[i].life;
        vec3f zag = campos - rain[i].drop_pt;
        zag = zag.cross(rain[i].drop_vect);
        zag *= RAINDROP_WIDTH / zag.length();
        glBegin(GL_TRIANGLE_STRIP);
        glColor4f(raindrop_col[0],raindrop_col[1],raindrop_col[2],0.0);
        tempv = pt1 - zag;
        glVertex3fv(tempv);
        tempv = pt2 - zag;
        glVertex3fv(tempv);

        glColor4fv(raindrop_col);
        glVertex3fv(pt1);
        glVertex3fv(pt2);

        glColor4f(raindrop_col[0],raindrop_col[1],raindrop_col[2],0.0);
        tempv = pt1 + zag;
        glVertex3fv(tempv);
        tempv = pt2 + zag;
        glVertex3fv(tempv);
        glEnd();
    }

#define SNOWFLAKE_POINT_SIZE        3.0f
#define SNOWFLAKE_BOX_SIZE          0.175f

// NOTE: must be greater than 1.0f
#define SNOWFLAKE_MAXLIFE           4.5f

    GLfloat ops; // Original Point Size, for to be restored

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
    {
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
        glGetFloatv(GL_POINT_SIZE, &ops);
        glPointSize(SNOWFLAKE_POINT_SIZE);
    }
    else
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_COLOR, GL_ONE);
        tex_snowflake->bind();
    }

    for (const SnowFlake &sf: snowfall)
    {
        const vec3f pt = sf.drop_pt + sf.drop_vect * sf.life;
        GLfloat alpha;

        if (sf.life > SNOWFLAKE_MAXLIFE)
        {
            alpha = 0.0f;
        }
        else
        if (sf.life > 1.0f)
        {
#define ML      SNOWFLAKE_MAXLIFE
            // this equation ensures that snowflaks fade in
            alpha = (sf.life - ML) / (1 - ML);
#undef ML
        }
        else
            alpha = 1.0f;

        if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
        {
            glBegin(GL_POINTS);
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
            glVertex3fv(pt);
            glEnd();
        }
        else
        {
#define SBS     SNOWFLAKE_BOX_SIZE
            vec3f zag = campos - sf.drop_pt;

            zag = zag.cross(sf.drop_vect);
            zag.normalize();
            zag *= SBS;

            if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::square)
            {
                glBegin(GL_TRIANGLE_STRIP);
                glColor4f(1.0f, 1.0f, 1.0f, alpha);
                glVertex3f(pt.x,            pt.y,           pt.z                );
                glVertex3f(pt.x,            pt.y,           pt.z + zag.z + SBS  );
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z                );
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z + zag.z + SBS  );
                glEnd();
            }
            else // cfg_snowflaketype == SnowFlakeType::textured
            {
                glBegin(GL_TRIANGLE_STRIP);
                glColor4f(1.0f, 1.0f, 1.0f, alpha);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(pt.x,            pt.y,           pt.z                );
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(pt.x,            pt.y,           pt.z + zag.z + SBS  );
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z                );
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(pt.x + zag.x,    pt.y + zag.y,   pt.z + zag.z + SBS  );
                glEnd();
            }
#undef SBS
        }
    }

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
        glPointSize(ops); // restore original point size

    // disable textures
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        glDisable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    const vec4f checkpoint_col[3] =
    {
        vec4f(1.0f, 0.0f, 0.0f, 0.8f),  // 0 = next checkpoint
        vec4f(0.7f, 0.7f, 0.1f, 0.6f),  // 1 = checkpoint after next
        vec4f(0.2f, 0.8f, 0.2f, 0.4f)  // 2 = all other checkpoints
    };

    if (showcheckpoint)
    {
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            vec4f colr = checkpoint_col[2];

            if ((int)i == vehic->nextcp)
                colr = checkpoint_col[0];
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
                colr = checkpoint_col[1];

            glPushMatrix(); // 1
            glTranslatef(game->checkpt[i].pt.x, game->checkpt[i].pt.y, game->checkpt[i].pt.z);
            glScalef(25.0f, 25.0f, 1.0f);

#if 0 // Checkpoint style one
            glBegin(GL_TRIANGLE_STRIP);

            for (float a = 0.0f; a < 0.99f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * a);
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            for (float a = 1.0f; a < 2.01f; a += 0.05f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3] * (2.0f - a));
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                glVertex3f(cosf(ang), sinf(ang), ht - 1.0f);
                glVertex3f(cosf(ang), sinf(ang), ht + 1.0f);
            }

            glEnd();
#else // Regular checkpoint style
            glBegin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            glEnd();
#endif
            glPopMatrix(); // 1
        }

// codriver checkpoints rendering
#ifdef INDEVEL

    // codriver checkpoints for debugging purposes
    const vec4f cdcheckpoint_col[3] =
    {
        {0.0f, 0.0f, 1.0f, 0.8f},       // 0 = next checkpoint
        {0.3f, 0.3f, 1.0f, 0.6f},       // 1 = checkpoint after next
        {0.6f, 0.6f, 1.0f, 0.4f}        // 2 = all other checkpoints
    };

        for (unsigned int i=0; i<game->codrivercheckpt.size(); i++)
        {
            vec4f colr = cdcheckpoint_col[2];

            if (game->cdcheckpt_ordered)
            {
                if ((int)i == vehic->nextcdcp)
                    colr = cdcheckpoint_col[0];
                else if ((int)i == (vehic->nextcdcp + 1) % (int)game->codrivercheckpt.size())
                    colr = cdcheckpoint_col[1];
            }
            else
                colr = cdcheckpoint_col[1];

            glPushMatrix(); // 1
            glTranslatef(game->codrivercheckpt[i].pt.x, game->codrivercheckpt[i].pt.y, game->codrivercheckpt[i].pt.z);
            glScalef(15.0f, 15.0f, 1.0f);

            glBegin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht - 1.0f);
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht + 0.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht - 1.0f);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht + 0.0f);
            glEnd();

            glBegin(GL_TRIANGLE_STRIP);
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                glColor4f(colr[0], colr[1], colr[2], colr[3]);
                glVertex3f(cosf(a), sinf(a), ht - 0.0f);
                glColor4f(colr[0], colr[1], colr[2], 0.0f);
                glVertex3f(cosf(a), sinf(a), ht + 1.0f);
            }
            glColor4f(colr[0], colr[1], colr[2], colr[3]);
            glVertex3f(1.0f, 0.0f, ht - 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            glVertex3f(1.0f, 0.0f, ht + 1.0f);
            glEnd();
            glPopMatrix(); // 1
        }
#endif
    }

    glEnable(GL_TEXTURE_2D);

    if (game->water.enabled)
        renderWater();

    if (psys_dirt != nullptr) // cfg_dirteffect == false
        getSSRender().render(psys_dirt);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE,GL_ZERO);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FOG);

    glDisable(GL_LIGHTING);

    glPopMatrix(); // 0

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); // 0
    glLoadIdentity();

    glOrtho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix(); // 1

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showui)
    {
        game->renderCodriverSigns();

        glPushMatrix(); // 2
        // position of rpm dial and needle
        //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.22f, 0.0f);
        glTranslatef( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.22f, 0.0f);
        glScalef(0.30f, 0.30f, 1.0f);

        tex_hud_revs->bind();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f);
        glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f);
        glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f);
        glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f);
        glVertex2f(1.0f,-1.0f);
        glEnd();

        // draw the needle of the RPM dial
        glRotatef(225.0f - vehic->getEngineRPM() * 15.0f / 1000.0f, 0.0f, 0.0f, 1.0f);
        tex_hud_revneedle->bind();
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix(); // 3
        glTranslatef(0.62f, 0.0f, 0.0f);
        glScalef(0.16f, 0.16f, 0.16f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f);
        glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f);
        glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f);
        glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f);
        glVertex2f(1.0f,-1.0f);
        glEnd();
        glPopMatrix(); // 3
        glDisable(GL_TEXTURE_2D);
        glPopMatrix(); // 2
    }

    // checkpoint pointing arrow thing
#if 0
    glPushMatrix(); // 2

    glTranslatef(0.0f, 0.8f, 0.0f);

    glScalef(0.2f, 0.2f, 0.2f);

    glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(DEGREES(nextcpangle), 0.0f, -1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.8f, 0.8f, 0.6f);
    glVertex3f(-1.0f, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    glVertex3f(-1.0f, 0.2f, 1.0f);
    glColor4f(0.8f, 0.4f, 0.4f, 0.6f);
    glVertex3f(0.0f, 0.0f, -2.0f);
    glColor4f(1.0f, 0.5f, 0.5f, 0.6f);
    glVertex3f(0.0f, 0.2f, -2.0f);
    glEnd();

    glPopMatrix(); // 2
#endif

    if (showmap)
    {
        // position and size of map
        //glViewport(getWidth() * (5.75f/100.f), getHeight() * (6.15f/100.f), getHeight()/3.5f, getHeight()/3.5f);
        glViewport(getWidth() * (2.5f/100.f), getHeight() * (2.5f/100.f), getHeight()/3.5f, getHeight()/3.5f);

        glPushMatrix(); // 2
        glScalef(hratio, vratio, 1.0f);

        if (game->terrain->getHUDMapTexture())
        {
            glEnable(GL_TEXTURE_2D);
            game->terrain->getHUDMapTexture()->bind();
        }

        glMatrixMode(GL_TEXTURE);
        glPushMatrix(); // 3
        float scalefac = 1.0f / game->terrain->getMapSize();
        glScalef(scalefac, scalefac, 1.0f);
        glTranslatef(campos.x, campos.y, 0.0f);
        glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
        glScalef(1.0f / 0.003f, 1.0f / 0.003f, 1.0f);

        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(-1.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
        glTexCoord2f(-1.0f, -1.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, -1.0f);
        glVertex2f(1.0f, -1.0f);
        glEnd();

        glPopMatrix(); // 3
        glMatrixMode(GL_MODELVIEW);

        glDisable(GL_TEXTURE_2D);

        glPushMatrix(); // 3
        glScalef(0.003f, 0.003f, 1.0f);
        glRotatef(DEGREES(-camera_angle), 0.0f, 0.0f, 1.0f);
        glTranslatef(-campos.x, -campos.y, 0.0f);
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            glPushMatrix(); // 4
            vec3f vpos = game->checkpt[i].pt;
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            vec4f colr = checkpoint_col[2];
            if ((int)i == vehic->nextcp)
            {
                float sc = 1.5f + sinf(cprotate * 10.0f) * 0.5f;
                glScalef(sc, sc, 1.0f);
                colr = checkpoint_col[0];
            }
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
            {
                colr = checkpoint_col[1];
            }
            glBegin(GL_TRIANGLE_FAN);
            glColor4fv(colr);
            glVertex2f(0.0f, 0.0f);
            glColor4f(colr[0], colr[1], colr[2], 0.0f);
            //glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix(); // 4
        }
        for (unsigned int i=0; i<game->vehicle.size(); i++)
        {
            glPushMatrix(); // 4
            vec3f vpos = game->vehicle[i]->body->getPosition();
            glTranslatef(vpos.x, vpos.y, 0.0f);
            glRotatef(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            glScalef(30.0f, 30.0f, 1.0f);
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glVertex2f(0.0f, 0.0f);
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glVertex2f(0.0f, 1.0f);
            glVertex2f(-1.0f, 0.0f);
            glVertex2f(0.0f, -1.0f);
            glVertex2f(1.0f, 0.0f);
            glEnd();
            glPopMatrix(); // 4
        }
        glPopMatrix(); // 3

        glPopMatrix(); // 2

        glViewport(0, 0, getWidth(), getHeight());
    }

    glEnable(GL_TEXTURE_2D);

    if (showui)
    {
        // Work-around for the "TIME" label once penalty time is displayed
        float time_offset = 0.0f;
        /*
        tex_hud_gear->bind();
        glPushMatrix(); // 2

        glTranslatef(1.0f, 0.35f, 0.0f);
        glScalef(0.2f, 0.2f, 1.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(1.0f,1.0f); glVertex2f(1.0f,1.0f);
        glTexCoord2f(0.0f,1.0f); glVertex2f(-1.0f,1.0f);
        glTexCoord2f(0.0f,0.0f); glVertex2f(-1.0f,-1.0f);
        glTexCoord2f(1.0f,0.0f); glVertex2f(1.0f,-1.0f);
        glEnd();

        glPopMatrix(); // 2
        */

        tex_fontSourceCodeOutlined->bind();

        // time counter
        glPushMatrix(); // 2

        // time position (other time strings inherit this position)
        // -hratio is left border, 0 is center, +hratio is right
        // hratio * (1/50) gives 1% of the entire width
        // +vratio is top border, 0 is middle, -vratio is bottom
        glTranslatef(-hratio + hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);

        // time label
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glScalef(0.125f, 0.125f, 1.0f);
        if (game->gamestate == Gamestate::finished)
        {
            getSSRender().drawText(
                PUtil::formatTime(game->coursetime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else if (game->coursetime < game->cptime + 1.50f)
        {
            getSSRender().drawText(
                PUtil::formatTime(game->cptime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else if (game->coursetime < game->cptime + 3.50f)
        {
            float a = (((game->cptime + 3.50f) - game->coursetime) / 2);
            glColor4f(1.0f, 1.0f, 1.0f, a);
            getSSRender().drawText(
                PUtil::formatTime(game->cptime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }
        else
        {
            getSSRender().drawText(
                PUtil::formatTime(game->coursetime),
                PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
        }

        // show target time
        glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
        glTranslatef(0.0f, -0.8f, 0.0f);
        getSSRender().drawText(PUtil::formatTime(game->targettime), PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        {
            // show the time penalty if there is any
            const float timepen = game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

            if (timepen >= 0.1f)
            {
                glColor4f(1.0f, 1.0f, 0.5f, 1.0f);
                glTranslatef(0.0f, -0.8f, 0.0f);
                getSSRender().drawText(PUtil::formatTime(timepen) + '+', PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
                time_offset = 0.8;
            }
        }

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTranslatef(0.0f, 1.32f + time_offset, 0.0f);
        glScalef(0.65f, 0.65f, 1.0f);
        getSSRender().drawText("TIME", PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        glPopMatrix(); // 2

        // show Next/Total checkpoints
        {
            // checkpoint counter
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            const std::string totalcp = std::to_string(game->checkpt.size());
            const std::string nextcp = std::to_string(vehic->nextcp);

            // checkpoint position
            glTranslatef(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);

              if (game->getFinishState() != Gamefinish::not_finished)
                  getSSRender().drawText(totalcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
              else
                  getSSRender().drawText(nextcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            // checkpoint label
            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("CKPT", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glPopMatrix(); // 2
        }

        // show Current/Total laps
        if (game->number_of_laps > 1)
        {
            const std::string currentlap = std::to_string(vehic->currentlap);
            const std::string number_of_laps = std::to_string(game->number_of_laps);

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            glTranslatef(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f) - 0.20f, 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(number_of_laps + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
            else
                getSSRender().drawText(currentlap + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("LAP", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            glPopMatrix(); // 2
        }

        if (cfg.getEnableFps())
        {
            std::stringstream stream;

            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glPushMatrix(); // 2
            glTranslatef(0.0f, vratio - vratio * (5.5f/50.f), 0.0f);
            glScalef(0.125f, 0.125f, 1.0f);
            stream << std::fixed << std::setprecision(1) << fps;
            getSSRender().drawText(stream.str(), PTEXT_HZA_CENTER | PTEXT_VTA_TOP);

            glTranslatef(0.0f, 0.52f, 0.0f);
            glScalef(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("FPS", PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            glPopMatrix(); // 2
        }

#ifdef INDEVEL
        // show codriver checkpoint text (the pace notes)
        if (!game->codrivercheckpt.empty() && vehic->nextcdcp != 0)
        {
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix(); // 2
            glTranslatef(0.0f, 0.3f, 0.0f);
            glScalef(0.1f, 0.1f, 1.0f);
            getSSRender().drawText(game->codrivercheckpt[vehic->nextcdcp - 1].notes, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix(); // 2
        }
#endif

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        tex_fontSourceCodeBold->bind();

        // show current gear and speed
        {
            // gear number
            const int gear = vehic->getCurrentGear();
            const std::string buff = (gear >= 0) ? PUtil::formatInt(gear + 1, 1) : "R";

            glPushMatrix(); // 2
            // position of gear & speed number & label
            //glTranslatef( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.21f, 0.0f);
            glTranslatef( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.21f, 0.0f);
            glScalef(0.20f, 0.20f, 1.0f);
            getSSRender().drawText(buff, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);

            // speed number
            const int speed = std::fabs(vehic->getWheelSpeed()) * cfg.getHudSpeedoMpsSpeedMult();
            std::string speedstr = std::to_string(speed);

            //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTranslatef(1.1f, -0.625f, 0.0f);
            glScalef(0.5f, 0.5f, 1.0f);
            getSSRender().drawText(speedstr, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            // speed label
            glTranslatef(0.0f, -0.82f, 0.0f);
            glScalef(0.5f, 0.5f, 1.0f);

            if (cfg.getSpeedUnit() == PConfig::Speedunit::mph)
                getSSRender().drawText("MPH", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
            else
                getSSRender().drawText("km/h", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            glPopMatrix(); // 2
        }

        renderDamageIndicatorGroup();

#ifndef NDEBUG
        // draw revs for debugging
        glPushMatrix(); // 2
        glTranslatef(1.17f, 0.52f, 0.0f);
        glScalef(0.2f, 0.2f, 1.0f);
        getSSRender().drawText(std::to_string(vehic->getEngineRPM()), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
#endif

#ifndef NDEBUG
        // draw real time penalty for debugging
        glPushMatrix(); // 2
        glScalef(0.1f, 0.1f, 1.0f);
        glTranslatef(0.0f, -4.0f, 0.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        tex_fontSourceCodeOutlined->bind();
        getSSRender().drawText(std::string("true time penalty: ") +
            std::to_string(game->getOffroadTime() * game->offroadtime_penalty_multiplier),
            PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
#endif
    }

    tex_fontSourceCodeShadowed->bind();

    // draw "off road" warning sign and text
    if (game->isRacing())
    {
        //const vec3f bodypos = vehic->part[0].ref_world.getPosition();
        const vec3f bodypos = vehic->body->getPosition();

        if (!game->terrain->getRmapOnRoad(bodypos))
        {
            glPushMatrix(); // 2
            glLoadIdentity();
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glScalef(0.25f, 0.25f, 1.0f);
            tex_hud_offroad->bind();
            glBegin(GL_QUADS);
                glTexCoord2f(   1.0f,   1.0f);
                glVertex2f(     1.0f,   1.0f);
                glTexCoord2f(   0.0f,   1.0f);
                glVertex2f(    -1.0f,   1.0f);
                glTexCoord2f(   0.0f,   0.0f);
                glVertex2f(    -1.0f,  -1.0f);
                glTexCoord2f(   1.0f,   0.0f);
                glVertex2f(     1.0f,  -1.0f);
            glEnd();
            glPopMatrix(); // 2
            glPushMatrix(); // 2
            glScalef(0.1f, 0.1f, 1.0f);
            glTranslatef(0.0f, -2.5f, 0.0f);
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
            tex_fontSourceCodeOutlined->bind();
            getSSRender().drawText(
                std::to_string(static_cast<int> (game->getOffroadTime() * game->offroadtime_penalty_multiplier)) +
                " seconds",
                PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            glPopMatrix(); // 2
        }
    }

    // draw terrain info for debugging
    #ifdef INDEVEL
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        const TerrainType tt = game->terrain->getRoadSurface(wheelpos);
        const rgbcolor c = PUtil::getTerrainColor(tt);
        const std::string s = PUtil::getTerrainInfo(tt);

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.5f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);

        if (tt != TerrainType::Unknown)
        {
            const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

            glPushMatrix(); // 3
            glDisable(GL_TEXTURE_2D);
            glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
            glTranslatef(0.0f, -1.0f, 0.0f);
            glBegin(GL_TRIANGLE_STRIP);
                glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
                glVertex2f(-0.2f,   0.0f);
                glVertex2f(endx,    0.0f);
                glVertex2f(-0.2f,   1.1f);
                glVertex2f(endx,    1.1f);
            glEnd();
            glEnable(GL_TEXTURE_2D);
            glPopMatrix(); // 3
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif

    // draw if we're on road for debugging
    //#ifdef INDEVEL
    #if 0
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        std::string s;
        rgbcolor c;

        if (game->terrain->getRmapOnRoad(wheelpos))
        {
            c = rgbcolor(0xFF, 0xFF, 0xFF);
            s = "on the road";
        }
        else
        {
            c = rgbcolor(0x00, 0x00, 0x00);
            s = "off-road";
        }

        const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.25f, 0.0f);
        glScalef(0.1f, 0.1f, 1.0f);
        glPushMatrix(); // 3
        glDisable(GL_TEXTURE_2D);
        glTranslatef(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -1.0f, 0.0f);
        glBegin(GL_TRIANGLE_STRIP);
            glColor3f(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
            glVertex2f(-0.2f,   0.0f);
            glVertex2f(endx,    0.0f);
            glVertex2f(-0.2f,   1.1f);
            glVertex2f(endx,    1.1f);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glPopMatrix(); // 3
        glColor3f(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        glPopMatrix(); // 2
    }
    #endif
    {
        tex_fontSourceCodeOutlined->bind();

        glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        glPushMatrix(); // 2
        glTranslatef(0.0f, 0.2f, 0.0f);
        glScalef(0.6f, 0.6f, 1.0f);
        if (game->gamestate == Gamestate::countdown)
        {
            float sizer = fmodf(game->othertime, 1.0f) + 0.5f;
            glScalef(sizer, sizer, 1.0f);
            getSSRender().drawText(
                PUtil::formatInt(((int)game->othertime + 1), 1),
                PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->gamestate == Gamestate::finished)
        {
            if (game->getFinishState() == Gamefinish::pass)
            {
                glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
                getSSRender().drawText("WIN", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                glScalef(0.5f, 0.5f, 1.0f);
                glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
                getSSRender().drawText("TIME EXCEEDED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
        }
        else if (game->coursetime < 1.0f)
        {
            glColor4f(0.5f, 1.0f, 0.5f, 1.0f);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->coursetime < 2.0f)
        {
            float a = 1.0f - (game->coursetime - 1.0f);
            glColor4f(0.5f, 1.0f, 0.5f, a);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        glPopMatrix(); // 2

        if (game->gamestate == Gamestate::countdown)
        {
            glPushMatrix(); // 2
            glTranslatef(0.0f, 0.6f, 0.0f);
            glScalef(0.08f, 0.08f, 1.0f);
            if (game->othertime < 1.0f)
            {
                glColor4f(1.0f, 1.0f, 1.0f, game->othertime);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            glPopMatrix(); // 2
        }

        if (pauserace)
        {
            glPushMatrix(); // 2
            glColor4f(0.25f, 0.25f, 1.0f, 1.0f);
            glScalef(0.25f, 0.25f, 1.0f);
            getSSRender().drawText("PAUSED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            glPopMatrix(); // 2
        }
    }

    glPopMatrix(); // 1

    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); // 0
    glMatrixMode(GL_MODELVIEW);

    glBlendFunc(GL_ONE, GL_ZERO);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void MainApp::renderDamageIndicator(
        const PTexture *texture, float posx, float posy, float scalex, float scaley, float damage)
{
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;

    if (damage > 1.0f)
        damage = 1.0f;

    if (damage < 0.0f)
    {
        red = 1.0;
        green = 1.0;
        blue = 1.0;
    }
    else if (damage < 0.5f)
    {
        red = 2.0f * damage;
        green = 1.0;
    }
    else
    {
        red = 1.0;
        green = 2.0 * (1.0 - damage);
    }

    glPushMatrix(); // 2

    glTranslatef(posx, posy, 0.0f);
    glScalef(scalex, scaley, 1.0f);
    texture->bind();
    glColor4f(red, green, blue, 0.5f);

    glBegin(GL_QUADS);
    glTexCoord2f(1.0f,1.0f);
    glVertex2f(1.0f,1.0f);
    glTexCoord2f(0.0f,1.0f);
    glVertex2f(-1.0f,1.0f);
    glTexCoord2f(0.0f,0.0f);
    glVertex2f(-1.0f,-1.0f);
    glTexCoord2f(1.0f,0.0f);
    glVertex2f(1.0f,-1.0f);
    glEnd();

    glPopMatrix(); // 2
}

void MainApp::renderDamageIndicatorGroup()
{
    // Theoretically there can be multiple vehicles with multiple parts.
    // However, the assumption is, that the damage of the first part of the first vehicle is relevant.
    PVehiclePart &part = game->vehicle[0]->part[0];

    renderDamageIndicator(
        tex_damage_front_left, hratio * 45.0f/50.0f - 0.075f, -vratio * 32.5f/50.0f + 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageFrontLeft));
    renderDamageIndicator(
        tex_damage_front_right, hratio * 45.0f/50.0f - 0.025f, -vratio * 32.5f/50.0f + 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageFrontRight));
    renderDamageIndicator(
        tex_damage_rear_left, hratio * 45.0f/50.0f - 0.075f, -vratio * 32.5f/50.0f - 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageRearLeft));
    renderDamageIndicator(
        tex_damage_rear_right, hratio * 45.0f/50.0f - 0.025f, -vratio * 32.5f/50.0f - 0.032f, 0.025f, 0.032f,
        part.damage.getDamage(PDamage::DamageSide::DamageRearRight));
}

void MainApp::renderVehiclePart(const PVehicleType &type, const PVehiclePart &part,
    const PVehicleTypePart &typepart, float alpha)
{
    if (typepart.model)
    {
        glPushMatrix();

        vec3f vpos = part.ref_world.getPosition();
        glTranslatef(vpos.x, vpos.y, vpos.z);

        mat44f vorim = part.ref_world.getInverseOrientationMatrix();
        glMultMatrixf(vorim);

        float scale = typepart.scale;
        glScalef(scale,scale,scale);

        drawModel(*typepart.model, alpha);

        glPopMatrix();
    }

    if (type.wheelmodel)
    {
        for (unsigned int i=0; i<typepart.wheel.size(); ++i)
        {
            glPushMatrix();

            vec3f wpos = part.wheel[i].ref_world.getPosition();
            glTranslatef(wpos.x,wpos.y,wpos.z);

            mat44f worim = part.wheel[i].ref_world.getInverseOrientationMatrix();
            glMultMatrixf(worim);

            float scale = type.wheelscale * typepart.wheel[i].radius;
            glScalef(scale,scale,scale);

            drawModel(*type.wheelmodel, alpha);

            glPopMatrix();
        }
    }
}
```

### [menu.colors](file:///home/alan/Downloads/trigger-rally-code-r1032/data/menu.colors)
```diff:menu.colors
<?xml version="1.0" ?>
<!-- This file sets the colors of the menu widgets:

	normal      = non-clickable labels
	click       = clickable labels
	hover       = clickable labels with mouse hovering on top
	listnormal  = non-clickable list items
	listclick   = clickable list items
	listhover   = clickable list items with mouse hovering on top
	weak        = non-clickable labels that should be discreet
	strong      = non-clickable labels that should be obvious
	marked      = non-clickable labels that should draw attention
	header      = non-clickable labels that are used as section title
	bnormal     = disabled button
	bclick      = clickable button
	bhover      = clickable button with mouse hovering on top

	The format of the colors is OpenGL-style RGBA (Red Green Blue Alpha)
	with the components being floating point numbers ranging from 0 to 1.
-->
<menucolors
	normal="1, 1, 1, 1"
	click="1, 1, 1, 1"
	hover="1, 0.4, 0, 1"
	listnormal="0.7, 0.7, 0.7, 1"
	listclick="1, 1, 1, 1"
	listhover="1, 0.4, 0, 1"
	weak="0.7, 0.7, 0.7, 1"
	strong="1, 1, 1, 1"
	marked="1, 0.2, 0.2, 1"
	header="0.8, 0.8, 0.4, 1"
	bnormal="1, 1, 1, 0.2"
	bclick="1, 1, 1, 1"
	bhover="1, 0.4, 0, 1"
	/>
===
<?xml version="1.0" ?>
<!-- This file sets the colors of the menu widgets:

	normal      = non-clickable labels
	click       = clickable labels
	hover       = clickable labels with mouse hovering on top
	listnormal  = non-clickable list items
	listclick   = clickable list items
	listhover   = clickable list items with mouse hovering on top
	weak        = non-clickable labels that should be discreet
	strong      = non-clickable labels that should be obvious
	marked      = non-clickable labels that should draw attention
	header      = non-clickable labels that are used as section title
	bnormal     = disabled button
	bclick      = clickable button
	bhover      = clickable button with mouse hovering on top

	The format of the colors is OpenGL-style RGBA (Red Green Blue Alpha)
	with the components being floating point numbers ranging from 0 to 1.
-->
<menucolors
	normal="0.9, 0.92, 0.95, 0.9"
	click="0.95, 0.95, 0.98, 0.95"
	hover="1.0, 0.55, 0.0, 1.0"
	listnormal="0.75, 0.78, 0.82, 0.8"
	listclick="0.9, 0.92, 0.95, 0.9"
	listhover="1.0, 0.55, 0.0, 1.0"
	weak="0.6, 0.63, 0.68, 0.7"
	strong="1.0, 1.0, 1.0, 1.0"
	marked="1.0, 0.2, 0.1, 1.0"
	header="1.0, 0.65, 0.0, 1.0"
	bnormal="1.0, 1.0, 1.0, 0.15"
	bclick="1.0, 1.0, 1.0, 0.8"
	bhover="1.0, 0.55, 0.0, 1.0"
	/>

```

---

## 3. Verification & Build
The codebase compiles cleanly with the upgrades using the following commands:
```bash
cd src
make -j$(nproc)
```
The game compiles without error, outputting the binary `bin/trackrs` which successfully plays the intro video and then starts the loop of the intro music and interactive graphics.

#include "engine/render/basic.h"
#include "engine/core/lru_cache.h"
namespace trackrs::terrain { class Renderer; }
namespace trackrs::vegetation { class Renderer; }
namespace trackrs::mesh { struct MeshData; }
namespace trackrs::pbr { class Model; }
#include "../../engine/core/pcg32.h"
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#pragma once

#include "subsys.h"
#include "vbuffer.h"
#include <cmath>

class PEffect;
class PException;
class PModel;
class PRigidity;
class PSSEffect;
class PSSTexture;
class PTexture;

struct PParticle_s {
  vec3f pos,linvel;
  float life;

  vec2f orix,oriy; // orientation vectors (2d)
};

class PParticleSystem {
  trackrs::Pcg32 particleRng;
protected:
  float colorstart[4],colorend[4];
  float startsize, endsize;
  float decay;
  const PTexture *tex;
  GLenum blendparam1, blendparam2;

  std::vector<PParticle_s> part;

public:
  PParticleSystem() {
    colorstart[0] = colorstart[1] = colorstart[2] = colorstart[3] = 1.0;
    colorend[0] = colorend[1] = colorend[2] = 1.0; colorend[3] = 0.0;
    startsize = 0.0;
    endsize = 1.0;
    decay = 1.0;
    tex = nullptr;
    blendparam1 = GL_SRC_ALPHA;
    blendparam2 = GL_ONE;
  }

public:
  void setColorStart(float r, float g, float b, float a) {
    colorstart[0] = r; colorstart[1] = g; colorstart[2] = b; colorstart[3] = a;
  }
  void setColorEnd(float r, float g, float b, float a) {
    colorend[0] = r; colorend[1] = g; colorend[2] = b; colorend[3] = a;
  }
  void setColor(float r, float g, float b) {
    colorstart[0] = colorend[0] = r;
    colorstart[1] = colorend[1] = g;
    colorstart[2] = colorend[2] = b;
  }
  void setSize(float start, float end) {
    startsize = start; endsize = end;
  }
  void setDecay(float _decay) {
    decay = _decay;
  }
  void setTexture(const PTexture *texptr) {
    tex = texptr;
  }
  void setBlend(GLenum b1, GLenum b2) {
    blendparam1 = b1;
    blendparam2 = b2;
  }

  void addParticle(const vec3f &pos, const vec3f &linvel);

  void tick(float delta);

  friend class PSSRender;
};

struct PVert_tv {
  vec2f st;
  vec3f xyz;
};

#define PTEXT_HZA_LEFT    0x00000000 // default
#define PTEXT_HZA_CENTER  0x00000001
#define PTEXT_HZA_RIGHT   0x00000002
#define PTEXT_VTA_BOTTOM  0x00000000 // default
#define PTEXT_VTA_CENTER  0x00000100
#define PTEXT_VTA_TOP     0x00000200
#define PTEXT_HIGHLIGHT   0x00010000

class PSSRender : public PSubsystem {
private:
  vec3f cam_pos;
  mat44f cam_orimat;

public:
  PSSRender(PApp &parentApp);
  ~PSSRender();

  void tick(float delta, const vec3f &eyepos, const mat44f &eyeori, const vec3f &eyevel);

  void render(PParticleSystem *psys);

  void drawModel(PModel &model, PSSEffect &ssEffect, PSSTexture &ssTexture);

  void drawText(const std::string &text, uint32 flags);
  vec2f getTextDims(const std::string &text);
};



class PSSTexture : public PSubsystem {
private:
  PResourceList<PTexture> texlist;

public:
  PSSTexture(PApp &parentApp);
  ~PSSTexture();

  PTexture *loadTexture(const std::string &name, bool genMipmaps = true, bool clamp = false);
};


class PImage {
private:
  uint8 *data;
  int cx,cy,cc;

public:
  PImage () : data (nullptr) { }
  PImage (const std::string &filename) : data (nullptr) { load (filename); }
  PImage (int _cx, int _cy, int _cc) : data (nullptr) { load (_cx, _cy, _cc); }
  ~PImage ();

  void load (const std::string &filename);
  void load (int _cx, int _cy, int _cc);
  void unload ();

  void expandChannels();

  int getcx() const { return cx; }
  int getcy() const { return cy; }
  int getcc() const { return cc; }
  uint8 *getData() { return data; }

  const uint8 * getData() const
  {
    return data;
  }

  uint8 & getByte(int i)
  {
      return data[i];
  }

  uint8 getByte(int i) const
  {
      return data[i];
  }

  void swap (PImage &other) noexcept
  {
    { uint8 *tmp = data; data = other.data; other.data = tmp; }
    { int tmp = cx; cx = other.cx; other.cx = tmp; }
    { int tmp = cy; cy = other.cy; other.cy = tmp; }
    { int tmp = cc; cc = other.cc; other.cc = tmp; }
  }
};


class PTexture : public PResource {
private:
  GLuint texid;
  GLenum textarget;

  void scaleImage(GLuint format, 
    GLsizei width_in, GLsizei height_in, GLenum type_in, const void* data_in,
    GLsizei width_out, GLsizei height_out, GLenum type_out, void* data_out
    );

public:
  PTexture () : texid (0) { }
  PTexture (const std::string &filename, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp) : texid (0) {
    load (filename, cfgAnisotropy, genMipmaps, clamp); }
  PTexture (PImage &img, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp) : texid (0) {
    load (img, cfgAnisotropy, genMipmaps, clamp); }
  ~PTexture() { unload (); }

  void load (const std::string &filename, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp);
  void load(PImage &img, GLfloat cfgAnisotropy, bool genMipmaps = true, bool clamp = false);
  void loadPiece(PImage &img, int offx, int offy, int sizex, int sizey, bool genMipmaps = true, bool clamp = false);
  void loadAlpha(const std::string &filename, bool genMipmaps = true, bool clamp = false);
  void loadAlpha(PImage &img, bool genMipmaps = true, bool clamp = false);
  void loadCubeMap(const std::string &filenamePrefix, const std::string &filenameSuffix, bool genMipmaps = true);
  void unload();

  void bind() const;
  GLuint id() const { return texid; } ///< borrowed GL name (instanced impostors)

  static void unbind();
};




class PSSEffect : public PSubsystem {
private:
  PResourceList<PEffect> fxlist;

public:
  PSSEffect(PApp &parentApp);
  ~PSSEffect();

  PEffect *loadEffect(const std::string &name);
};



#define CULLFACE_NONE       0
#define CULLFACE_CW         1
#define CULLFACE_CCW        2

#define BLEND_NONE          0
#define BLEND_ADD           1
#define BLEND_MULTIPLY      2
#define BLEND_ALPHA         3
#define BLEND_PREMULTALPHA  4


struct fx_renderstate_s {
  bool depthtest;

  bool lighting;
  bool lightmodeltwoside;

  struct {
    GLenum func;
    float ref;
  } alphatest;

  int cullface;

  int blendmode;

  struct {
    int texindex;
  } texunit[1];
};


struct fx_pass_s {
  fx_renderstate_s rs;
};

struct fx_technique_s {
  std::string name;

  std::vector<fx_pass_s> pass;

  bool validated;
  bool textures_ready;

  fx_technique_s() {
    validated = false;
    textures_ready = false;
  }
};

struct fx_texture_s {
  std::string name;

  std::string filename;

  GLenum type;

  // This is filled in just before rendering
  PTexture *texobject;

  fx_texture_s() {
    texobject = nullptr;
  }
};


class PEffect : public PResource {
private:
  // resources
  std::vector<fx_texture_s> tex;

  // techniques
  std::vector<fx_technique_s> tech;

  int cur_tech;

public:
  PEffect(const std::string &filename);
  ~PEffect();

  void unload();

  void loadFX(const std::string &filename);
  void loadMTL(const std::string &filename);

  int getNumTechniques();
  bool validateTechnique(int technique);
  const std::string &getTechniqueName(int technique);
  bool findTechnique(const std::string &techname, int *technique);

  bool setCurrentTechnique(int technique);
  int getCurrentTechnique();

  bool setFirstValidTechnique();

  bool renderBegin(int *numPasses, PSSTexture &sstex);
  void renderPass(int pass);
  void renderEnd();

private:
  void migrateRenderState(fx_renderstate_s *rs_old, fx_renderstate_s *rs_new);
};




class PSSModel : public PSubsystem {
private:
  PResourceList<PModel> modlist;

public:
  PSSModel(PApp &parentApp);
  ~PSSModel();

  PModel *loadModel(const std::string &name);
};


class PFace {
public:
  vec3f facenormal;
  uint32 vt[3];
  uint32 tc[3];
  uint32 nr[3];

public:
//    PFace(vec
};


class PMesh {
public:
  std::vector<vec3f> vert;
  std::vector<vec2f> texco;
  std::vector<vec3f> norm;
  std::vector<PFace> face;

  std::string fxname;
  PEffect *effect = nullptr;
  std::shared_ptr<trackrs::draw::Mesh> gpu;
};


class PModel : public PResource {
	void loadASE (const std::string &filename, float globalScale);
	void loadOBJ (const std::string &filename, float globalScale);

public:
	PModel (const std::string &filename, float globalScale = 1.0);
	std::vector<PMesh> mesh;
  std::shared_ptr<trackrs::mesh::MeshData> imported;
  std::shared_ptr<trackrs::pbr::Model> materialGpu;
  bool drawImported(float alpha=1);
  bool drawImportedInstances(const std::vector<glm::mat4> &models, float alpha=1);
	std::pair<vec3f, vec3f> getExtents() const;
};

struct PTerrainFoliageBand {
  float middle, range;
  float density;
  int trycount;
  float scale;

  /*
  float scalemin;
  float scalemax;
  */

  PTexture *sprite_tex;
  int sprite_count;
};

struct PTerrainFoliage {
  vec3f pos;
  float ang;
  float scale;
  float rigidity;
};

struct PTerrainFoliageSet {
  std::vector<PTerrainFoliage> inst;

  PVBuffer buff[2];
  int numvert, numelem;
};

struct PRoadSignSet {
    std::vector<PTerrainFoliage> inst;
    PVBuffer buff[2];
    int numvert;
    int numelem;
};

struct PTerrainTile {
  int posx, posy;
  int lru_counter;

  PVBuffer vert;
  int numverts;

  PTexture tex;

  vec3f mins,maxs; // AABB

  std::vector<PTerrainFoliageSet> foliage;
  std::vector<PRoadSignSet> roadsignset;
  // Straight vector for rapid search by collision detection
  std::vector<PTerrainFoliage> straight;
};

///
/// @brief Loads road information.
///
class RoadMap
{
public:

    RoadMap() = default;

    bool load(const PImage &img)
    {
        if (img.getData() == nullptr)
            return false;

        bx = img.getcx();
        by = img.getcy();
        bitmap.clear();
        bitmap.reserve(img.getcx() * img.getcy());

        const std::size_t tb = img.getcx() * img.getcy() * img.getcc(); // Total Bytes

        for (auto pb = img.getData(); pb != img.getData() + tb; pb += img.getcc())
        {
            bool is_road = true;

            for (int i=0; i < img.getcc(); ++i)
                if (pb[i] != 0xFF) // the road is white
                {
                    is_road = false;
                    break;
                }

            bitmap.push_back(is_road);
        }

        return true;
    }

    bool is_loaded() const
    {
        return !bitmap.empty();
    }

    ///
    /// @brief Decides if provided point is on the road.
    /// @param px           X coordinate of the point.
    /// @param py           Y coordinate of the point.
    /// @param ms           Map size.
    /// @see `getMapSize()`.
    /// @returns Whether or not the point is on the road.
    /// @retval true        If no roadmap was loaded.
    ///
    bool isOnRoad(float px, float py, float ms) const;

private:

    std::vector<bool> bitmap;
    int bx;
    int by;
};

struct road_sign
{
public:

//
// being smart here makes the rest of the code more complicated;
// so let's be stupid for now...
//
#if 0
  struct road_sign_location
  {
    public:
      float x;
      float y;
      float deg;

      road_sign_location(float x=0, float y=0, float deg=0):
          x(x), y(y), deg(deg)
      {
      }
  };
  std::vector<road_sign_location> location;
#endif

  PTexture *sprite  = nullptr;
  float scale   = 1.0f;
  float x = 0.0f;
  float y = 0.0f;
  float deg = 0.0f;
  int sprite_count = 1;
};

class PTerrain
{
  std::uint64_t foliageSeed = 0;
protected:
  bool loaded;

  int tilesize, tilecount, totsize, totmask, totsizesq;

  float scale_hz, scale_vt, scale_hz_inv, scale_vt_inv, scale_tile_inv;

  int cmaptotsize, cmaptilesize, cmaptotmask;

  //std::vector<uint8> hmap;
  std::vector<float> hmap;

  // color map
  PImage cmap;
  // terrain map
  PImage tmap;
  // road map
  RoadMap rmap;

  std::vector<float> fmap;
  std::vector<PTerrainFoliageBand> foliageband;
  std::vector<road_sign> roadsigns;

  trackrs::LruCache<trackrs::TileKey, std::unique_ptr<PTerrainTile>, trackrs::TileHash> tile{128};
  std::unique_ptr<trackrs::terrain::Renderer> worldRenderer;
  std::unique_ptr<trackrs::vegetation::Renderer> vegetationRenderer;

  // tiles share index buffers
  PVBuffer ind;
  int numinds;

  PTexture *tex_hud_map;

protected:

  PTerrainTile *getTile(int x, int y);

  float getInterp(float x, float y, float *data);

public:
  PTerrain(XMLElement *element, const std::string &filepath, PSSTexture &ssTexture,
      const PRigidity &rigidity, bool cfgFoliage, bool cfgRoadsigns);
  ~PTerrain();

  void unload();

  void render(const vec3f &campos, const mat44f &camorim, bool reflectionCapture = false);

  void drawSplat(float x, float y, float scale, float angle);

  const std::vector<PTerrainFoliage> *getFoliageAtPos(const vec3f &pos) const;

  struct ContactInfo {
    vec3f pos;
    vec3f normal;
  };

    ///
    /// @brief Returns whether or not the given position is on road.
    /// @param [in] pos         Position to be checked.
    /// @returns Whether or not `pos` is on the road.
    /// @retval true            If no roadmap was loaded.
    /// @see `RoadMap`.
    ///
    bool getRmapOnRoad(const vec3f &pos) const
    {
        return rmap.isOnRoad(pos.x, pos.y, getMapSize());
    }

    ///
    /// @brief Returns the color of the pixel in the colormap that corresponds
    ///  to the given position in the terrain.
    /// @note The height component Z is ignored.
    /// @todo Should check if cmap.getcc() returns at least 3?
    /// @todo Should check if cmap.getcx() == cmap.getcy()?
    /// @todo Should remove paranoid clampings?
    /// @todo Should actually measure performance of float vs int.
    /// @param [in] pos   Position in the terrain.
    /// @returns Color in OpenGL-style RGB.
    ///
    vec3f getCmapColor(const vec3f &pos) const;
    TerrainType getRoadSurface(const vec3f &pos) const;

  ///
  /// @brief get the information about a contact point (its coordinates and normal) with the ground
  ///
  void getContactInfo(ContactInfo &tci);

  float getHeight(float x, float y) {
    return getInterp(x, y, &hmap[0]);
  }

  float getFoliageLevel(float x, float y) {
    return getInterp(x, y, &fmap[0]);
  }

  PTexture *getHUDMapTexture() { return tex_hud_map; }

  float getMapSize() const { return totsize * scale_hz; }

private:
  const PTerrainTile *getTileAtPos(const vec3f &pos) const;
  void ensureWorldRenderer();

  // Rigidity map for foliage and road signs
  const PRigidity &rigidity;
};

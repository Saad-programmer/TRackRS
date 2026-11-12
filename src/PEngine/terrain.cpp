#include <sstream>
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "exception.h"
#include "render.h"
#include "rigidity.h"
#include "pengine.h"
#include "../../engine/core/wrap.h"
#include "engine/terrain/renderer.h"
#include "engine/terrain/tracks_renderer.h"
#include "engine/vegetation/renderer.h"
#include "engine/dressing/renderer.h"
#include "engine/dressing/props_renderer.h"
#include "engine/world/state.h"
#include "engine/pbr/renderer.h"
#include <glm/gtc/type_ptr.hpp>

PTerrain::~PTerrain ()
{
  unload();
}


void PTerrain::unload()
{
  loaded = false;

  tile.clear();
  dressingRenderer.reset();
  vegetationRenderer.reset();
  worldRenderer.reset();

  hmap.clear();
}


PTerrain::PTerrain (XMLElement *element, const std::string &filepath, PSSTexture &ssTexture,
    const PRigidity &rigidity, bool cfgFoliage, bool cfgRoadsigns) :
    loaded (false), rigidity(rigidity)
{
  unload();
  foliageSeed = trackrs::stageSeed(filepath);
  if (auto* level = element->Parent()->ToElement())
    level->QueryUnsigned64Attribute("stage_seed", &foliageSeed);
  if (validation::run.options.enabled) foliageSeed ^= validation::run.options.seed;

  std::string heightmap, colormap, terrainmap, roadmap, foliagemap, hudmap;

  scale_hz = 1.0;
  scale_vt = 1.0;

  const char *val;

  val = element->Attribute("tilesize");
  if (val) tilesize = atoi(val);

  val = element->Attribute("horizontalscale");
  if (val) scale_hz = atof(val);

  val = element->Attribute("verticalscale");
  if (val) scale_vt = atof(val);

  val = element->Attribute("heightmap");
  if (val) heightmap = val;

  val = element->Attribute("colormap");
  if (val) colormap = val;

  val = element->Attribute("terrainmap");
  if (val != nullptr) terrainmap = val;

  val = element->Attribute("roadmap");
  if (val != nullptr) roadmap = val;

  val = element->Attribute("foliagemap");
  if (val && cfgFoliage) foliagemap = val;

  val = element->Attribute("hudmap");
  if (val) hudmap = val;

  XMLElement *node = element->FirstChildElement("blurfilter");
  std::vector<std::vector<float> > blurfilter;

  if (node != nullptr) {
    for (XMLElement *walk = node->FirstChildElement("row");
        walk != nullptr;
        walk = walk->NextSiblingElement("row")) {
      const char *srow = walk->Attribute("data");

      if (srow == nullptr)
          continue;

      std::stringstream bfrow(srow);
      float coef;
      std::vector<float> row;

      while (bfrow >> coef)
        row.push_back(coef);

      blurfilter.push_back(row);
    }
  }
  else {
    blurfilter = {
        {0.03f, 0.12f, 0.03f},
        {0.12f, 0.40f, 0.12f},
        {0.03f, 0.12f, 0.03f}
    };
  }

  for (XMLElement *walk = element->FirstChildElement();
    walk; walk = walk->NextSiblingElement()) {

    if (strcmp(walk->Value(), "roadsign") == 0 && cfgRoadsigns) {
      road_sign temprs;

      val = walk->Attribute("sprite");
      if (val != nullptr)
        temprs.sprite = ssTexture.loadTexture(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("scale");
      if (val != nullptr)
        temprs.scale = atof(val);

      val = walk->Attribute("spritecount");
      if (val)
        temprs.sprite_count = atoi(val);

      for (XMLElement *walk2 = walk->FirstChildElement();
          walk2 != nullptr;
          walk2 = walk2->NextSiblingElement()) {
        if (strcmp(walk2->Value(), "location") == 0) {
          float deg = 0;

          val = walk2->Attribute("oridegrees");
          if (val != nullptr)
            deg = RADIANS(atof(val));

          val = walk2->Attribute("coords");
          if (val != nullptr) {
            float x, y;

            if (sscanf(val, "%f, %f", &x, &y) == 2) {
              temprs.x = x;
              temprs.y = y;
              temprs.deg = deg;

              if (temprs.sprite != nullptr)
                roadsigns.push_back(temprs);
            }
          }
        }
      }
    }
    else if (!strcmp(walk->Value(), "foliageband") && cfgFoliage) {
      PTerrainFoliageBand tfb;
      tfb.middle = 0.5f;
      tfb.range = 0.5f;
      tfb.density = 1.0f;
      tfb.scale = 1.0f;
      //tfb.scalemin = 1.0f;
      //tfb.scalemax = 1.4f;
      //tfb.model = nullptr;
      //tfb.modelscale = 1.0f;
      tfb.sprite_tex = nullptr;
      tfb.sprite_count = 1;

      val = walk->Attribute("middle");
      if (val) tfb.middle = atof(val);

      val = walk->Attribute("range");
      if (val) tfb.range = atof(val);

      val = walk->Attribute("density");
      if (val) tfb.density = atof(val);

      val = walk->Attribute("scale");
      if (val) tfb.scale = atof(val);

      /*
      val = walk->Attribute("scalemin");
      if (val) tfb.scalemin = atof(val);

      val = walk->Attribute("scalemax");
      if (val) tfb.scalemax = atof(val);
        */
      /*
      val = walk->Attribute("model");
      if (val) tfb.model = ssModel.loadModel(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("modelscale");
      if (val) tfb.modelscale = atof(val);
      */

      val = walk->Attribute("sprite");
      if (val) tfb.sprite_tex = ssTexture.loadTexture(PUtil::assemblePath(val, filepath));

      val = walk->Attribute("spritecount");
      if (val) tfb.sprite_count = atoi(val);

      foliageband.push_back(tfb);
    }
  }


  if (!heightmap.length()) {
    throw MakePException ("Load failed: terrain has no heightmap");
  }

  if (!colormap.length()) {
    throw MakePException ("Load failed: terrain has no colormap");
  }

  if (tilesize != (tilesize & (-tilesize)) ||
    tilesize < 4) {
    throw MakePException ("Load failed: tile size not power of two dimension, or too small");
  }

  if (scale_hz <= 0.0 || scale_vt == 0.0) {
    throw MakePException ("Load failed: invalid scale value");
  }

  scale_hz_inv = 1.0 / scale_hz;
  scale_vt_inv = 1.0 / scale_vt;
  scale_tile_inv = scale_hz_inv / (float)tilesize;

  PImage img;
  try
  {
    img.load (PUtil::assemblePath (heightmap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open heightmap \"" << heightmap << "\"\n";
    throw;
  }

  totsize = img.getcx();
  if (totsize != img.getcy() ||
    totsize != (totsize & (-totsize)) ||
    totsize < 16) {
    throw MakePException ("Load failed: heightmap not square, or not power of two dimension, or too small");
  }

  totsizesq = totsize * totsize;

  if (tilesize > totsize) tilesize = totsize;

  tilecount = totsize / tilesize;
  totmask = totsize - 1;

  //PUtil::outLog() << "img: " << totsize << " squared, " << img.getcc() << " cc\n";

  hmap.resize(totsizesq);

#if 0
  if (img.getcc() != 1) {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Warning: heightmap is not single channel\n";
    int cc = img.getcc();
    uint8 *dat = img.getData();
    for (int s=0, d=0; d<totsizesq; s+=cc, d+=1) hmap[d] = dat[s];
  } else {
    std::copy(img.getData(), img.getData() + totsize * totsize, &hmap[0]);
  }
#else
  if (img.getcc() != 1) {
    if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
      PUtil::outLog() << "Warning: heightmap is not single channel\n";
  }

  int cc = img.getcc();
  uint8 *dat = img.getData();

  for (int y=0; y<totsize; ++y) {
    for (int x=0; x<totsize; ++x) {
      float accum = 0.0;
      for (int yi=0; yi < static_cast<int> (blurfilter.size()); ++yi) {
        for (int xi=0; xi < static_cast<int> (blurfilter[yi].size()); ++xi) {
          accum += (float)dat[
            (((y + yi - (blurfilter.size()-1)/2) & totmask) * totsize +
            ((x + xi - (blurfilter[yi].size()-1)/2) & totmask)) * cc] * blurfilter[yi][xi];
        }
      }
      hmap[y*totsize + x] = accum * scale_vt;
    }
  }
#endif

  img.unload();

  try
  {
    cmap.load(PUtil::assemblePath(colormap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open colormap \"" << colormap << "\"\n";
    throw;
  }

  cmaptotsize = cmap.getcx();
  if (cmaptotsize != cmap.getcy() ||
    cmaptotsize != (cmaptotsize & (-cmaptotsize)) ||
    cmaptotsize < tilecount) {
    throw MakePException ("Load failed: colormap not square, or not power of two dimension, or too small");
  }

  cmaptilesize = cmaptotsize / tilecount;
  cmaptotmask = cmaptotsize - 1;

  // load terrain map image
  try
  {
      if (!terrainmap.empty())
        tmap.load(PUtil::assemblePath(terrainmap, filepath));
  }
  catch (...)
  {
    PUtil::outLog() << "Load failed: couldn't open terrainmap \"" << terrainmap << "\"\n";
    throw;
  }

    if (tmap.getData() != nullptr && tmap.getcx() != tmap.getcy())
        throw MakePException("Load failed: terrainmap not square");

    PImage rmap_img;

    // load road map image
    try
    {
        if (!roadmap.empty())
            rmap_img.load(PUtil::assemblePath(roadmap, filepath));
    }
    catch (...)
    {
        PUtil::outLog() << "Load failed: couldn't open roadmap \"" << roadmap << "\"\n";
        throw;
    }

    if (rmap_img.getData() != nullptr)
    {
        if (rmap_img.getcx() != rmap_img.getcy())
            throw MakePException("Load failed: roadmap not square");
        else
        if (!rmap.load(rmap_img))
            throw MakePException("Load failed: bad roadmap image");
    }

  // calculate foliage try counts for tile size

  for (unsigned int b = 0; b < foliageband.size(); b++) {
    foliageband[b].trycount =
      (int) (foliageband[b].density * (float)totsizesq * scale_hz * scale_hz);
  }

  // load foliage map

  fmap.resize(totsizesq, 0.0f);

  if (foliagemap.length()) {
    try
    {
      img.load(PUtil::assemblePath(foliagemap, filepath));
    }
    catch (...)
    {
      PUtil::outLog() << "Load failed: couldn't open foliage map \"" << foliagemap << "\"\n";
      throw;
    }

    if (totsize != img.getcy() ||
      totsize != img.getcx()) {
      throw MakePException ("Load failed: foliage map size doesn't match heightmap");
    }

    int cc = img.getcc();
    uint8 *dat = img.getData();

    if (cc != 1) {
      if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
        PUtil::outLog() << "Warning: foliage map is not single channel\n";
    }

    for (int i = 0; i < totsizesq; i++) {
      fmap[i] = (float) dat[i * cc] / 255.0f;
    }
  }

  // load hud map

  tex_hud_map = nullptr;

  if (hudmap.length()) {
    tex_hud_map = ssTexture.loadTexture(PUtil::assemblePath(hudmap, filepath));
  }

  // prepare shared index buffers

  int tilesizep1 = tilesize + 1;

  numinds = 0;

  PRamFile ramfile;
  uint16 index;
  for (int y=0; y<tilesize; ++y) {
    int add1 = (y+1) * tilesizep1;
    int add2 = (y+0) * tilesizep1;
    if (y > 0) {
      index = 0 + add1;
      ramfile.write(&index, sizeof(uint16));
      numinds += 1;
    }
    for (int x=0; x<tilesizep1; ++x) {
      index = x + add1;
      ramfile.write(&index, sizeof(uint16));
      index = x + add2;
      ramfile.write(&index, sizeof(uint16));
      numinds += 2;
    }
    if (y+1 < tilesize) {
      ramfile.write(&index, sizeof(uint16));
      numinds += 1;
    }
  }

  ind.create(ramfile.getSize(), PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile.getData());
  ramfile.clear();

  loaded = true;
  if(trackrs::terrain::settings().enabled)ensureWorldRenderer();
}

///
/// @brief Gets terrain tile for each world position
/// @param pos = world position
/// @retval Pointer to terrain tile
///
const PTerrainTile *PTerrain::getTileAtPos(const vec3f &pos) const
{
  int tilex = pos.x * scale_tile_inv;
  int tiley = pos.y * scale_tile_inv;

  if (pos.x < 0.0)
    --tilex;
  if (pos.y < 0.0)
    --tiley;

  const auto *cached = tile.find({tilex,tiley,0});
  return cached ? cached->get() : nullptr;
}

///
/// @brief Gets vector of objects on tile at world position
/// @param pos = world position
/// @retval Pointer to world objects on terrain tile
///
const std::vector<PTerrainFoliage> *PTerrain::getFoliageAtPos(const vec3f &pos) const
{
  const PTerrainTile *tile = getTileAtPos(pos);

  if (tile) {
    return &tile->straight;
  }
  return nullptr;
}

PTerrainTile *PTerrain::getTile(int tilex, int tiley)
{
  bool created = false;
  auto &entry = tile.get({tilex,tiley,0}, [&] { created = true; return std::make_unique<PTerrainTile>(); });
  auto *tileptr = entry.get();
  if (!created) return tileptr;

  tileptr->posx = tilex;
  tileptr->posy = tiley;
  tileptr->lru_counter = 0;

  tileptr->mins = vec3f((float)(tilex * tilesize) * scale_hz, (float)(tiley * tilesize) * scale_hz, 1000000000.0);
  tileptr->maxs = vec3f((float)((tilex+1) * tilesize) * scale_hz, (float)((tiley+1) * tilesize) * scale_hz, -1000000000.0);

  // TODO: quadtree based thing

  //std::vector<bool>

  static PRamFile ramfile1, ramfile2;

  ramfile1.clear();

  int tileoffsety = tiley * tilesize;
  int tileoffsetx = tilex * tilesize;
  int tilesizep1 = tilesize + 1;
  for (int y=0; y<tilesizep1; ++y) {
    int posy = tileoffsety + y;
    int sampley = posy & totmask;
    for (int x=0; x<tilesizep1; ++x) {
      int posx = tileoffsetx + x;
      int samplex = posx & totmask;
      vec3f vert = vec3f(
        (float)posx * scale_hz,
        (float)posy * scale_hz,
        (float)hmap[(sampley * totsize) + samplex]);
      ramfile1.write(vert, sizeof(vec3f));
      if (tileptr->mins.z > vert.z)
        tileptr->mins.z = vert.z;
      if (tileptr->maxs.z < vert.z)
        tileptr->maxs.z = vert.z;
    }
  }
  tileptr->vert.create(ramfile1.getSize(), PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());

  //tileptr->maxs.z += 10.0;

  //tileptr->mins = vec3f((float)(tilex * tilesize) * scale_hz, (float)(tiley * tilesize) * scale_hz, 0.0);
  //tileptr->maxs = vec3f((float)((tilex+1) * tilesize) * scale_hz, (float)((tiley+1) * tilesize) * scale_hz, 100.0);

  tileptr->numverts = tilesizep1 * tilesizep1;

  tileptr->tex.loadPiece(cmap,
    (tilex * cmaptilesize) & cmaptotmask, (tiley * cmaptilesize) & cmaptotmask,
    cmaptilesize, cmaptilesize, true, true);

  // Create foliage

  const auto tileSeed = foliageSeed ^ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(tilex)) << 32u)
                                    ^ static_cast<std::uint32_t>(tiley);
  trackrs::Pcg32 tileRng(tileSeed, 11);

  tileptr->foliage.resize(foliageband.size());

  for (unsigned int b = 0; b < foliageband.size(); b++) {

    tileptr->foliage[b].inst.clear();

    // Create foliage instances

    for (int i = 0; i < foliageband[b].trycount; i++) {
      float rigidityvalue = 0.0f;
      const float sampleX = tileRng.unit(), sampleY = tileRng.unit();
      vec2f ftry = vec2f(
        (float)((tileptr->posx * tilesize) + sampleX * tilesize) * scale_hz,
        (float)((tileptr->posy * tilesize) + sampleY * tilesize) * scale_hz);

      float fol = getFoliageLevel(ftry.x, ftry.y);

      if ((1.0 - fabs((fol - foliageband[b].middle) / foliageband[b].range)) < tileRng.unit()) continue;

      tileptr->foliage[b].inst.push_back(PTerrainFoliage());
      tileptr->foliage[b].inst.back().pos.x = ftry.x;
      tileptr->foliage[b].inst.back().pos.y = ftry.y;
      tileptr->foliage[b].inst.back().pos.z = getHeight(ftry.x, ftry.y);
      tileptr->foliage[b].inst.back().ang = tileRng.unit() * PI*2.0f;
      //tileptr->foliage[b].inst.back().scale = (1.0f + fol * 0.5f) * (tileRng.unit() * tileRng.unit() + 0.5) * 1.4;
      //tileptr->foliage[b].inst.back().scale = (foliageband[b].scalemin + fol * 0.5f) * (tileRng.unit() * tileRng.unit() + 0.5) * foliageband[b].scalemax;
      tileptr->foliage[b].inst.back().scale = (foliageband[b].scale + fol * 0.5f) * (tileRng.unit() * tileRng.unit() + 0.5) * 1.4;

      const auto &instance=tileptr->foliage[b].inst.back();
      const float pad=std::abs(instance.scale)*3.f;
      tileptr->mins.x=std::min(tileptr->mins.x,instance.pos.x-pad);
      tileptr->mins.y=std::min(tileptr->mins.y,instance.pos.y-pad);
      tileptr->mins.z=std::min(tileptr->mins.z,instance.pos.z-pad);
      tileptr->maxs.x=std::max(tileptr->maxs.x,instance.pos.x+pad);
      tileptr->maxs.y=std::max(tileptr->maxs.y,instance.pos.y+pad);
      tileptr->maxs.z=std::max(tileptr->maxs.z,instance.pos.z+pad);
      rigidityvalue = rigidity.getRigidity(foliageband[b].sprite_tex->getName());
      if (rigidityvalue != 0.0f) {
        tileptr->foliage[b].inst.back().rigidity = rigidityvalue;
        tileptr->straight.push_back(tileptr->foliage[b].inst.back());
      }
    }

    // Create vertex buffers for rendering

#define HMULT   1.0
#define VMULT   2.0

    ramfile1.clear();
    ramfile2.clear();

    tileptr->foliage[b].numvert = 0;
    tileptr->foliage[b].numelem = 0;

    float angincr = PI / (float)foliageband[b].sprite_count;
    for (unsigned int j=0; j<tileptr->foliage[b].inst.size(); j++) {
      for (float anga = 0.0f; anga < PI - 0.01f; anga += angincr) {
        float interang = tileptr->foliage[b].inst[j].ang + anga;
        int stv = tileptr->foliage[b].numvert;
        PVert_tv tmpv;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(cos(interang)*HMULT,sin(interang)*HMULT,0.0f) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(1.0f,0.0f);
        ramfile1.write(&tmpv,sizeof(PVert_tv));
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,0.0f) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(0.0f,0.0f);
        ramfile1.write(&tmpv,sizeof(PVert_tv));
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,VMULT) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(0.0f,1.0f/*-1.0f/32.0f*/);
        ramfile1.write(&tmpv,sizeof(PVert_tv));
        tileptr->foliage[b].numvert++;

        tmpv.xyz = tileptr->foliage[b].inst[j].pos +
          vec3f(cos(interang)*HMULT,sin(interang)*HMULT,VMULT) * tileptr->foliage[b].inst[j].scale;
        tmpv.st = vec2f(1.0f,1.0f/*-1.0f/32.0f*/);
        ramfile1.write(&tmpv,sizeof(PVert_tv));
        tileptr->foliage[b].numvert++;

        int ind;
        ind = stv + 0;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
        ind = stv + 1;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
        ind = stv + 2;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
        ind = stv + 0;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
        ind = stv + 2;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
        ind = stv + 3;
        ramfile2.write(&ind,sizeof(uint32));
        tileptr->foliage[b].numelem++;
      }
    }

    if (tileptr->foliage[b].numelem) {
      tileptr->foliage[b].buff[0].create(ramfile1.getSize(),
        PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());
      tileptr->foliage[b].buff[1].create(ramfile2.getSize(),
        PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile2.getData());
    }
  }

  tileptr->roadsignset.resize(roadsigns.size());

  for (unsigned int b=0; b < roadsigns.size(); ++b) {
    float rigidityvalue = 0.0f;
    vec2f ftry = vec2f(
      roadsigns[b].x * scale_hz,
      roadsigns[b].y * scale_hz);
    vec2f tilemin = vec2f(
      tileptr->posx * tilesize * scale_hz,
      tileptr->posy * tilesize * scale_hz);
    vec2f tilemax = vec2f(
      (tileptr->posx * tilesize + tilesize) * scale_hz,
      (tileptr->posy * tilesize + tilesize) * scale_hz);

    if (ftry.x >= tilemin.x && ftry.x <= tilemax.x && ftry.y >= tilemin.y && ftry.y <= tilemax.y) {
      tileptr->roadsignset[b].inst.clear();
      tileptr->roadsignset[b].inst.push_back(PTerrainFoliage());
      tileptr->roadsignset[b].inst.back().pos.x    = ftry.x;
      tileptr->roadsignset[b].inst.back().pos.y    = ftry.y;
      tileptr->roadsignset[b].inst.back().pos.z    = getHeight(ftry.x, ftry.y);
      tileptr->roadsignset[b].inst.back().ang      = roadsigns[b].deg;
      tileptr->roadsignset[b].inst.back().scale    = roadsigns[b].scale;

      rigidityvalue = rigidity.getRigidity(roadsigns[b].sprite->getName());
      if (rigidityvalue != 0.0f) {
        tileptr->roadsignset[b].inst.back().rigidity = rigidityvalue;
        tileptr->straight.push_back(tileptr->roadsignset[b].inst.back());
      }

      ramfile1.clear();
      ramfile2.clear();

      tileptr->roadsignset[b].numvert = 0;
      tileptr->roadsignset[b].numelem = 0;

      float angincr = PI / (float)roadsigns[b].sprite_count;
      for (unsigned int j=0; j<tileptr->roadsignset[b].inst.size(); j++) {
        for (float anga = 0.0f; anga < PI - 0.01f; anga += angincr) {
          float interang = tileptr->roadsignset[b].inst[j].ang + anga;
          int stv = tileptr->roadsignset[b].numvert;
          PVert_tv tmpv;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(cos(interang)*HMULT,sin(interang)*HMULT,0.0f) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(1.0f,0.0f);
          ramfile1.write(&tmpv,sizeof(PVert_tv));
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,0.0f) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(0.0f,0.0f);
          ramfile1.write(&tmpv,sizeof(PVert_tv));
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(-cos(interang)*HMULT,-sin(interang)*HMULT,VMULT) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(0.0f,1.0f/*-1.0f/32.0f*/);
          ramfile1.write(&tmpv,sizeof(PVert_tv));
          tileptr->roadsignset[b].numvert++;

          tmpv.xyz = tileptr->roadsignset[b].inst[j].pos +
            vec3f(cos(interang)*HMULT,sin(interang)*HMULT,VMULT) *
            tileptr->roadsignset[b].inst[j].scale;
          tmpv.st = vec2f(1.0f,1.0f/*-1.0f/32.0f*/);
          ramfile1.write(&tmpv,sizeof(PVert_tv));
          tileptr->roadsignset[b].numvert++;

          int ind;
          ind = stv + 0;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
          ind = stv + 1;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
          ind = stv + 2;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
          ind = stv + 0;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
          ind = stv + 2;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
          ind = stv + 3;
          ramfile2.write(&ind,sizeof(uint32));
          tileptr->roadsignset[b].numelem++;
        }
      }

      if (tileptr->roadsignset[b].numelem) {
          tileptr->roadsignset[b].buff[0].create(ramfile1.getSize(),
            PVBuffer::VertexContent, PVBuffer::StaticUsage, ramfile1.getData());
          tileptr->roadsignset[b].buff[1].create(ramfile2.getSize(),
            PVBuffer::IndexContent, PVBuffer::StaticUsage, ramfile2.getData());
      }
    }
  }
  return tileptr;
}


void PTerrain::ensureWorldRenderer() {
  if(worldRenderer)return;

      const unsigned dimension = tmap.getData() ? static_cast<unsigned>(tmap.getcx()) : static_cast<unsigned>(cmap.getcx());
      std::vector<std::uint8_t> types(std::size_t(dimension)*dimension);
      for (unsigned y=0;y<dimension;++y) for(unsigned x=0;x<dimension;++x) {
        TerrainType surface = TerrainType::Unknown;
        if(tmap.getData()) {
          const auto offset=(std::size_t(y)*dimension+x)*static_cast<unsigned>(tmap.getcc());
          surface=PUtil::decideRoadSurface(rgbcolor(tmap.getByte(static_cast<int>(offset)),tmap.getByte(static_cast<int>(offset+1)),tmap.getByte(static_cast<int>(offset+2))));
        }
        std::uint8_t layer=2;
        switch(surface) {
        case TerrainType::RacingTarmac: case TerrainType::CountryRoad: case TerrainType::SlipperyRoad: layer=0;break;
        case TerrainType::FineGravel: case TerrainType::RoughGravel: layer=1;break;
        case TerrainType::PressedEarth: case TerrainType::LooseEarth: case TerrainType::MoistEarth: layer=2;break;
        case TerrainType::Grass: case TerrainType::Shrubbery: layer=3;break;
        case TerrainType::FirmSand: case TerrainType::LooseSand: layer=4;break;
        case TerrainType::DeepSnow: layer=5;break;
        case TerrainType::Ice: layer=6;break;
        case TerrainType::Mud: layer=7;break;
        case TerrainType::DeepWater: layer=8;break;
        // Desert set (amendment A3.1): array layers 9..17, data/terrain/desert.
        case TerrainType::SandDune: layer=9;break;
        case TerrainType::FeshFesh: layer=10;break;
        case TerrainType::HardSand: layer=11;break;
        case TerrainType::HamadaStone: layer=12;break;
        case TerrainType::OuedGravel: layer=13;break;
        case TerrainType::DryLakeClay: layer=14;break;
        case TerrainType::PalmeraieDirt: layer=15;break;
        case TerrainType::Rock: layer=16;break;
        case TerrainType::TarmacN9: layer=17;break;
        default: {
          auto colour=getCmapColor(vec3f(static_cast<float>(x)*getMapSize()/dimension,static_cast<float>(y)*getMapSize()/dimension,0));
          if(colour.x>.7f&&colour.y>.7f&&colour.z>.7f)layer=5;
          else if(colour.y>colour.x*1.1f&&colour.y>colour.z*1.1f)layer=3;
          else if(colour.x>.55f&&colour.y>.4f)layer=4;
          break;
        }
        }
        types[std::size_t(y)*dimension+x]=layer;
      }
      auto data=trackrs::pbr::shaderDirectory().parent_path().parent_path();
      worldRenderer=std::make_unique<trackrs::terrain::Renderer>(hmap,static_cast<unsigned>(totsize),scale_hz,types,dimension,data);
    
}

void PTerrain::setProps(std::vector<trackrs::dressing::Prop> placed, const trackrs::dressing::PlacementReport &report)
{
  props = std::move(placed);
  propsPlacement = std::make_unique<trackrs::dressing::PlacementReport>(report);
  propsRenderer.reset(); // rebuilt lazily on the GL thread with the new set
}

void PTerrain::setSignAtlas(std::vector<std::uint8_t> rgba, int width, int height)
{
  signAtlas = std::move(rgba);
  signAtlasWidth = width;
  signAtlasHeight = height;
  propsRenderer.reset();
}

void PTerrain::setPeople(std::vector<trackrs::dressing::Person> placed)
{
  people = std::move(placed);
  if (dressingRenderer) dressingRenderer->setPeople(people);
}

void PTerrain::render(const vec3f &campos, const mat44f &camorim, bool reflectionCapture)
{
  validation::PassScope timing(trackrs::world::state().depthPass ? "shadow_terrain" : reflectionCapture ? "terrain_probe" : "terrain");
  float blah = camorim.row[0][0]; blah = blah; // unused

  // get frustum
  frustumf frust;
  {
    mat44f mat_mv, mat_p, mat_c;

    trackrs::draw::read(trackrs::draw::ViewMatrix, mat_mv);
    trackrs::draw::read(trackrs::draw::ProjectionMatrix, mat_p);

    mat_c = mat_mv.concatenate(mat_p);

    frust.construct(mat_c);
  }

  int ctx = (int)(campos.x * scale_tile_inv);
  if (campos.x < 0.0) --ctx;
  int cty = (int)(campos.y * scale_tile_inv);
  if (campos.y < 0.0) --cty;

  int mintx = ctx - 3,
    maxtx = ctx + 4,
    minty = cty - 3,
    maxty = cty + 4;

  // Determine list of tiles to draw

  std::list<PTerrainTile *> drawtile;

  for (int ty = minty; ty < maxty; ++ty) {
    for (int tx = mintx; tx < maxtx; ++tx) {
      drawtile.push_back(getTile(tx,ty));
    }
  }

  if (trackrs::terrain::settings().enabled) {
    ensureWorldRenderer();
    glm::mat4 view,projection;
    trackrs::draw::read(trackrs::draw::ViewMatrix,glm::value_ptr(view));
    trackrs::draw::read(trackrs::draw::ProjectionMatrix,glm::value_ptr(projection));
    auto draws=worldRenderer->draw({campos.x,campos.y,campos.z},projection*view,reflectionCapture);
    if(validation::run.capture) {
      validation::run.frameDraws+=draws;
      validation::run.frameTriangles+=draws*trackrs::terrain::GridCells*trackrs::terrain::GridCells*2;
    }
    trackrs::draw::disable(trackrs::draw::GenerateS);
    trackrs::draw::disable(trackrs::draw::GenerateT);
    PVBuffer::unbind();
    // Persistent ruts and print trails (amendment A3.3), multiplied over the
    // lit ground before vegetation and props cover it.
    if(!trackrs::world::state().depthPass&&!reflectionCapture) {
      validation::PassScope tracksTiming("tracks");
      if(!tracksRenderer)
        tracksRenderer=std::make_unique<trackrs::terrain::TracksRenderer>(trackrs::pbr::shaderDirectory().parent_path().parent_path(),[this](float x,float y){return getHeight(x,y);});
      tracksRenderer->draw(trackrs::terrain::tracks(),{campos.x,campos.y,campos.z},projection*view); // counts its draws
    }
    // Keep every nearby tile resident for CPU foliage contacts, but submit only
    // visible ones. Foliage is either native instanced 3D vegetation or the
    // retained legacy cards (graphics.vegetation).
    for(auto it=drawtile.begin();it!=drawtile.end();) {
      const auto &t=**it;
      const float dx=std::max({t.mins.x-campos.x,0.f,campos.x-t.maxs.x});
      const float dy=std::max({t.mins.y-campos.y,0.f,campos.y-t.maxs.y});
      const float range=trackrs::carProbeResolution(trackrs::terrain::settings().quality)==128?384.f:192.f;
      const bool outside=trackrs::terrain::outside(projection*view,{t.mins.x,t.mins.y,t.mins.z},{t.maxs.x,t.maxs.y,t.maxs.z});
      if(outside||(reflectionCapture&&dx*dx+dy*dy>range*range))it=drawtile.erase(it);
      else ++it;
    }
    if(trackrs::vegetation::settings().meshes&&!foliageband.empty()) {
      validation::PassScope vegetationTiming(trackrs::world::state().depthPass ? "shadow_vegetation" : reflectionCapture ? "vegetation_probe" : "vegetation");
      if(!vegetationRenderer) {
        auto data=trackrs::pbr::shaderDirectory().parent_path().parent_path();
        vegetationRenderer=std::make_unique<trackrs::vegetation::Renderer>(data);
        for(const auto &band:foliageband)
          vegetationRenderer->addBand(band.sprite_tex->getName(),band.sprite_tex->id(),static_cast<unsigned>(std::max(band.sprite_count,1)));
      }
      static_assert(sizeof(vec3f)==12&&offsetof(PTerrainFoliage,pos)==0&&offsetof(PTerrainFoliage,ang)==12&&offsetof(PTerrainFoliage,scale)==16,
                    "vegetation reads PTerrainFoliage in place as {x,y,z,yaw,scale}");
      const auto pass=trackrs::world::state().depthPass?trackrs::vegetation::Pass::Shadow:reflectionCapture?trackrs::vegetation::Pass::Probe:trackrs::vegetation::Pass::Main;
      auto &plants=vegetationRenderer->begin({campos.x,campos.y,campos.z},projection*view,pass,trackrs::terrain::settings().quality);
      for(unsigned b=0;b<foliageband.size();++b)
        for(const auto *t:drawtile)
          if(!t->foliage[b].inst.empty())
            plants.add(b,vegetationRenderer->form(b),t->foliage[b].inst.data(),t->foliage[b].inst.size(),sizeof(PTerrainFoliage));
      vegetationRenderer->submit(projection*view,reflectionCapture||trackrs::world::state().linearCapture);
    }
    if(trackrs::dressing::settings().enabled&&!people.empty()&&!reflectionCapture) {
      validation::PassScope dressingTiming(trackrs::world::state().depthPass ? "shadow_spectators" : "spectators");
      if(!dressingRenderer) {
        dressingRenderer=std::make_unique<trackrs::dressing::Renderer>(trackrs::pbr::shaderDirectory().parent_path().parent_path());
        dressingRenderer->setPeople(people);
      }
      const auto pass=trackrs::world::state().depthPass?trackrs::vegetation::Pass::Shadow:trackrs::vegetation::Pass::Main;
      dressingRenderer->draw({campos.x,campos.y,campos.z},projection*view,pass,trackrs::terrain::settings().quality,trackrs::world::state().linearCapture);
    }
    if(!props.empty()&&!reflectionCapture) {
      validation::PassScope propsTiming(trackrs::world::state().depthPass ? "shadow_props" : "props");
      if(!propsRenderer) {
        propsRenderer=std::make_unique<trackrs::dressing::PropsRenderer>(trackrs::pbr::shaderDirectory().parent_path().parent_path());
        propsRenderer->setProps(props,[this](float x,float y){return getHeight(x,y);},propsPlacement?*propsPlacement:trackrs::dressing::PlacementReport{});
        if(!signAtlas.empty()) propsRenderer->setSignAtlas(signAtlas,signAtlasWidth,signAtlasHeight);
      }
      const auto pass=trackrs::world::state().depthPass?trackrs::vegetation::Pass::Shadow:trackrs::vegetation::Pass::Main;
      propsRenderer->draw({campos.x,campos.y,campos.z},projection*view,pass,trackrs::terrain::settings().quality,trackrs::world::state().linearCapture,trackrs::dressing::frameTime());
    }
  } else {
  // Draw terrain

  trackrs::draw::enable(trackrs::draw::GenerateS);
  trackrs::draw::enable(trackrs::draw::GenerateT);

  float tgens[] = { scale_tile_inv, 0.0, 0.0, 0.0 };
  float tgent[] = { 0.0, scale_tile_inv, 0.0, 0.0 };


  for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {
    //if (frust.isAABBOutside(tileptr->mins, tileptr->maxs))
    //    trackrs::draw::color(1,0,0);
    //else
    //    trackrs::draw::color(1,1,1);

    tgens[3] = (float) (- (*t)->posx);
    tgent[3] = (float) (- (*t)->posy);

    trackrs::draw::uvPlane(trackrs::draw::S, tgens);
    trackrs::draw::uvPlane(trackrs::draw::T, tgent);

    // Texture
    (*t)->tex.bind();

    // Vertex buffers
    (*t)->vert.bind();
    ind.bind();

    trackrs::draw::indexed({GL_TRIANGLE_STRIP, (*t)->vert.id(), ind.id(), numinds, GL_UNSIGNED_SHORT, false});
  }


  PVBuffer::unbind();

  trackrs::draw::disable(trackrs::draw::GenerateS);
  trackrs::draw::disable(trackrs::draw::GenerateT);

  }

  // Don't apply terrain detail texture to foliage.
  // http://sourceforge.net/p/trigger-rally/discussion/527953/thread/b53361ba/
  trackrs::draw::activeTexture(GL_TEXTURE1);
  trackrs::draw::disable(GL_TEXTURE_2D);
  trackrs::draw::activeTexture(GL_TEXTURE0);

  // Draw foliage
  #if 1
  trackrs::draw::alpha({GL_GEQUAL, 0.5});
  trackrs::draw::enable(trackrs::draw::AlphaTest);
  trackrs::draw::disable(GL_CULL_FACE);
  trackrs::draw::color(1.0f, 1.0f, 1.0f);

  const bool nativeVegetation = trackrs::terrain::settings().enabled && trackrs::vegetation::settings().meshes;
  for (unsigned int b = 0; !nativeVegetation && b < foliageband.size(); b++) {

    foliageband[b].sprite_tex->bind();

    for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {

      if ((*t)->foliage[b].numelem) {
        (*t)->foliage[b].buff[0].bind(); // vert data
        (*t)->foliage[b].buff[1].bind(); // indices


        trackrs::draw::indexed({GL_TRIANGLES, (*t)->foliage[b].buff[0].id(), (*t)->foliage[b].buff[1].id(), (*t)->foliage[b].numelem, GL_UNSIGNED_INT, true});
      }

      #if 0
      for (std::vector<PTerrainFoliage>::iterator f = (*t)->foliage.begin(); f != (*t)->foliage.end(); f++) {

        #if 0
        trackrs::draw::begin(GL_LINES);
        vec3f pos = f->pos;
        trackrs::draw::vertex(pos);
        pos += vec3f(0.0f, 0.0f, 2.0f);
        trackrs::draw::vertex(pos);
        trackrs::draw::end();
        #endif

        #if 0
        if (!f->tfb->model) continue;

        trackrs::draw::push();
        vec3f &pos = f->pos;
        trackrs::draw::translate(pos.x, pos.y, pos.z);
        trackrs::draw::scale(f->tfb->modelscale, f->tfb->modelscale, f->tfb->modelscale);
        ssRender.drawModel(*f->tfb->model, ssEffect, ssTexture);
        trackrs::draw::pop();
        #endif
      }
      #endif
    }
  }

  PVBuffer::unbind();

  // draw road signs
  for (unsigned int b=0; b < roadsigns.size(); ++b) {
    roadsigns[b].sprite->bind();

    for (std::list<PTerrainTile *>::iterator t = drawtile.begin(); t != drawtile.end(); t++) {
      if ((*t)->roadsignset[b].numelem) {
        (*t)->roadsignset[b].buff[0].bind();
        (*t)->roadsignset[b].buff[1].bind();


        trackrs::draw::indexed({GL_TRIANGLES, (*t)->roadsignset[b].buff[0].id(), (*t)->roadsignset[b].buff[1].id(), (*t)->roadsignset[b].numelem, GL_UNSIGNED_INT, true});
      }
    }
  }

  #endif

  PVBuffer::unbind();
  trackrs::draw::enable(GL_CULL_FACE);
  trackrs::draw::disable(trackrs::draw::AlphaTest);
}

void PTerrain::drawSplat(float x, float y, float scale, float angle)
{
  // New terrain and the legacy blob use different vertex programs. Bias only
  // this retained decal so coplanar raster/depth rounding cannot form stripes.
  const bool bias = trackrs::terrain::settings().enabled;
  GLboolean oldOffset = GL_FALSE;
  GLfloat oldFactor=0,oldUnits=0;
  if(bias) {
    trackrs::draw::flush();
    oldOffset=glIsEnabled(GL_POLYGON_OFFSET_FILL);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&oldFactor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&oldUnits);
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-1.f,-1.f);
  }

  float *hmd = &hmap[0];
  int cx = totsize;
  int cy = totsize;

  x *= scale_hz_inv;
  y *= scale_hz_inv;

  scale *= 0.5f;

  int miny = (int)(y - scale);
  if ((y - scale) < 0.0f) miny--;
  int maxy = (int)(y + scale) + 1;
  if ((y + scale) < 0.0f) maxy--;
  int minx = (int)(x - scale);
  if ((x - scale) < 0.0f) minx--;
  int maxx = (int)(x + scale) + 2;
  if ((x + scale) < 0.0f) maxx--;

  // Coplanar shadows must use the same GPU depth arithmetic as terrain.
  trackrs::draw::worldSpace(true);
  trackrs::draw::matrix(trackrs::draw::TextureTransform);

  trackrs::draw::push();
  trackrs::draw::translate(0.5f, 0.5f, 0.0f);
  trackrs::draw::rotate(DEGREES(angle), 0.0f, 0.0f, 1.0f);
  trackrs::draw::scale(0.5f / scale, 0.5f / scale, 1.0f);
  trackrs::draw::translate(-x, -y, 0.0f);

  for (int y2=miny; y2<maxy; y2++) {
    int yc = y2 & (cy-1),
      ycp1 = (yc+1) & (cy-1),
      yc_cx = yc * cx,
      ycp1_cx = ycp1 * cx;

    trackrs::draw::begin(GL_TRIANGLE_STRIP);
    for (int x2=minx; x2<maxx; x2++) {
      int xc = x2 & (cx - 1);
      trackrs::draw::uv(x2, y2 + 1);
      trackrs::draw::vertex(x2 * scale_hz, (y2 + 1) * scale_hz, hmd[ycp1_cx + xc]);
      trackrs::draw::uv(x2, y2);
      trackrs::draw::vertex(x2 * scale_hz, y2 * scale_hz, hmd[yc_cx + xc]);
    }
    trackrs::draw::end();
  }
  trackrs::draw::worldSpace(false);
  trackrs::draw::pop();

  trackrs::draw::matrix(trackrs::draw::View);
  if(bias) {
    trackrs::draw::flush();glPolygonOffset(oldFactor,oldUnits);
    if(!oldOffset)glDisable(GL_POLYGON_OFFSET_FILL);
    trackrs::draw::invalidateGpuState();
  }
}




// Integer-world quantization remains intentional for legacy colour/material maps;
// wrapping precedes floor so negative fractions and exact periods are correct.
bool RoadMap::isOnRoad(float px, float py, float ms) const {
  if (bitmap.empty()) return true;
  const int x = trackrs::mapSample(px, ms, bx);
  const int y = trackrs::mapSample(py, ms, by);
  return bitmap.at(static_cast<std::size_t>(y) * static_cast<std::size_t>(bx) + static_cast<std::size_t>(x));
}
vec3f PTerrain::getCmapColor(const vec3f& pos) const {
  const int x = trackrs::mapSample(pos.x, getMapSize(), cmap.getcx(), true);
  const int y = trackrs::mapSample(pos.y, getMapSize(), cmap.getcy(), true);
  const int index = (y * cmap.getcx() + x) * cmap.getcc();
  return vec3f(cmap.getByte(index) / 255.0f, cmap.getByte(index + 1) / 255.0f,
               cmap.getByte(index + 2) / 255.0f);
}
TerrainType PTerrain::getRoadSurface(const vec3f& pos) const {
  if (!tmap.getData()) return TerrainType::Unknown;
  const int x = trackrs::mapSample(pos.x, getMapSize(), tmap.getcx(), true);
  const int y = trackrs::mapSample(pos.y, getMapSize(), tmap.getcy(), true);
  const int index = (y * tmap.getcx() + x) * tmap.getcc();
  return PUtil::decideRoadSurface(rgbcolor(tmap.getByte(index), tmap.getByte(index + 1), tmap.getByte(index + 2)));
}

float PTerrain::getInterp(float x, float y, float *data) {
    x *= scale_hz_inv;
    const auto cellX = trackrs::periodicCell(x, totsize);
    x = cellX.fraction;
    const int xiw = cellX.index, xiw2 = (xiw + 1) & totmask;

    y *= scale_hz_inv;
    const auto cellY = trackrs::periodicCell(y, totsize);
    y = cellY.fraction;
    const int yiw = cellY.index, yiw2 = (yiw + 1) & totmask;

    const int cx = totsize;

    float xv1,xv2;
    if (y > 0.0) {
      if (y < 1.0) {
        if (x < y) {
          xv1 = data[yiw*cx+xiw];
          xv2 = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x/y);
        } else {
          xv1 = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],(x-y)/(1.0-y));
          xv2 = data[yiw2*cx+xiw2];
        }
        return INTERP(xv1,xv2,y);
      } else {
        return INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x);
      }
    } else {
      return INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],x);
    }
  }

void PTerrain::getContactInfo(ContactInfo &tci) {

    float x = tci.pos.x * scale_hz_inv;
    const auto cellX = trackrs::periodicCell(x, totsize);
    x = cellX.fraction;
    const int xiw = cellX.index, xiw2 = (xiw + 1) & totmask;

    float y = tci.pos.y * scale_hz_inv;
    const auto cellY = trackrs::periodicCell(y, totsize);
    y = cellY.fraction;
    const int yiw = cellY.index, yiw2 = (yiw + 1) & totmask;

    float *data = &hmap[0];
    const int cx = totsize;

    float xv1,xv2;
    if (y > 0.0) {
      if (y < 1.0) {
        if (x < y) {
          tci.normal.x = data[yiw2*cx+xiw] - data[yiw2*cx+xiw2];
          tci.normal.y = data[yiw*cx+xiw] - data[yiw2*cx+xiw];
          xv1 = data[yiw*cx+xiw];
          xv2 = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x/y);
        } else {
          tci.normal.x = data[yiw*cx+xiw] - data[yiw*cx+xiw2];
          tci.normal.y = data[yiw*cx+xiw2] - data[yiw2*cx+xiw2];
          xv1 = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],(x-y)/(1.0-y));
          xv2 = data[yiw2*cx+xiw2];
        }
        tci.pos.z = INTERP(xv1,xv2,y);
      } else {
        tci.normal.x = data[yiw2*cx+xiw] - data[yiw2*cx+xiw2];
        tci.normal.y = data[yiw*cx+xiw] - data[yiw2*cx+xiw];
        tci.pos.z = INTERP(data[yiw2*cx+xiw],data[yiw2*cx+xiw2],x);
      }
    } else {
      tci.normal.x = data[yiw*cx+xiw] - data[yiw*cx+xiw2];
      tci.normal.y = data[yiw*cx+xiw2] - data[yiw2*cx+xiw2];
      tci.pos.z = INTERP(data[yiw*cx+xiw],data[yiw*cx+xiw2],x);
    }
    tci.normal.z = scale_hz;
    tci.normal.normalize();
  }

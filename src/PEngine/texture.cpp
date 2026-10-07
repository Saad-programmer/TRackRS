// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "exception.h"
#include "main.h"
#include "pengine.h"
#include "physfs_utils.h"
#include <SDL2/SDL_image.h>

// SDL_image doesn't need init/shutdown code

PSSTexture::PSSTexture(PApp &parentApp) : PSubsystem(parentApp)
{
  PUtil::outLog() << "Initialising texture subsystem [SDL_Image]" << std::endl;
}

PSSTexture::~PSSTexture()
{
  PUtil::outLog() << "Shutting down texture subsystem" << std::endl;
  
  texlist.clear();
}



PTexture *PSSTexture::loadTexture(const std::string &name, bool genMipmaps, bool clamp)
{
  PTexture *tex = texlist.find(name);
  if (!tex) {
    try
    {
      GLfloat cfgAnisotropy = static_cast<MainApp &>(app).cfg.getAnisotropy();
      tex = new PTexture(name, cfgAnisotropy, genMipmaps, clamp);
    }
    catch (PException &e)
    {
      if (PUtil::isDebugLevel(DEBUGLEVEL_ENDUSER))
        PUtil::outLog() << "Failed to load " << name << ": " << e.what () << std::endl;
      return nullptr;
    }
    texlist.add(tex);
  }
  return tex;
}



PImage::~PImage()
{
  unload ();
}

void PImage::unload ()
{
  delete[] data;
  data = nullptr;
}

void PImage::load (const std::string &filename)
{
  data = nullptr;
  
  if (PUtil::isDebugLevel(DEBUGLEVEL_TEST))
    PUtil::outLog() << "Loading image \"" << filename << "\"" << std::endl;

  // PhysFS / SDL integration with SDL_rwops
  
  PHYSFS_file *pfile = PHYSFS_openRead(filename.c_str());
  
  if (pfile == nullptr) {
    throw MakePException (filename + ", PhysFS: " + physfs_getErrorString());
  }
  
  SDL_RWops *rwops = PUtil::allocPhysFSops(pfile);
  
  SDL_Surface *img = IMG_Load_RW(rwops, 1); // this closes file and frees rwops
  
  if (!img) {
    throw MakePException (filename + ", SDL_image: " + IMG_GetError ());
  }
  
  if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);
  
  // TGA COLOUR SWITCH HACK
  int colmap_normal[] = { 0,1,2,3 };
  int colmap_flipped[] = { 2,1,0,3 };
  int *colmap = colmap_normal;
  const char *fname = filename.c_str();
  int len = strlen(fname);
  if (len > 4) {
    if (!strcmp(fname+len-4,".tga")) colmap = colmap_flipped;
  }
  
  cx = img->w;
  cy = img->h;
  cc = img->format->BytesPerPixel;
  data = new uint8 [cx * cy * cc];
  
  for (int y=0; y<cy; y++) {
    for (int x=0; x<cx; x++) {
      for (int c=0; c<cc; c++) {
        //data[(y*cx+x)*cc+c] = ((uint8*)img->pixels)[(cy-y-1)*img->pitch + x*cc + c];
        data[(y*cx+x)*cc+c] = ((uint8*)img->pixels)[(cy-y-1)*img->pitch + x*cc + colmap[c]];
      }
    }
  }
  
  if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);
  SDL_FreeSurface(img);
}

void PImage::load (int _cx, int _cy, int _cc)
{
  cx = _cx;
  cy = _cy;
  cc = _cc;
  
  data = new uint8 [cx * cy * cc];
}



void PTexture::unload()
{
  if (texid)
  {
    glDeleteTextures (1, &texid);
    texid = 0;
  }
}

void PTexture::load (const std::string &filename, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp)
{
  PImage image (filename);
  load (image, cfgAnisotropy, genMipmaps, clamp);
  name = filename;
}

void PTexture::load (PImage &img, GLfloat cfgAnisotropy, bool genMipmaps, bool clamp)
{
  unload();

  textarget = GL_TEXTURE_2D;

#ifdef USE_GEN_MIPMAPS
  if (genMipmaps && !SDL_GL_ExtensionSupported("GL_SGIS_generate_mipmap")) {
    PUtil::outLog() << "warning: can't generate mipmaps for texture" << std::endl;
    genMipmaps = false;
  }
#endif

  GLuint fmt,fmt2;

  switch (img.getcc()) {
  case 1:
    fmt = GL_LUMINANCE; fmt2 = GL_LUMINANCE; break;
  case 2:
    fmt = GL_LUMINANCE_ALPHA; fmt2 = GL_LUMINANCE_ALPHA; break;
  case 3:
    fmt = GL_RGB; fmt2 = GL_RGB; break;
  case 4:
    fmt = GL_RGBA; fmt2 = GL_RGBA; break;
  default:
    throw MakePException ("loading texture failed, unknown image format");
  }

  int cx = img.getcx(), cy = img.getcy();
  int newcx=1, newcy=1, max;
  while (newcx < cx) newcx *= 2;
  while (newcy < cy) newcy *= 2;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
  if (newcx > max) newcx = max;
  if (newcy > max) newcy = max;
  
  //PImage *useimg = &img;
  
  if (newcx != cx || newcy != cy) {
    PImage newimage (newcx, newcy, img.getcc ());
    
    scaleImage (fmt,
        cx, cy, GL_UNSIGNED_BYTE, img.getData (),
        newcx, newcy, GL_UNSIGNED_BYTE, newimage.getData ());
    
    img.swap (newimage);
  }

  glGenTextures(1,&texid);
  bind();

    if (SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic"))
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cfgAnisotropy);
    else
        PUtil::outLog() << "Warning: anisotropic filtering is not supported." << std::endl;

  glTexParameteri(textarget,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  if (genMipmaps)
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  else
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

  if (clamp) {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  } else {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_REPEAT);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (genMipmaps) {
#ifdef USE_GEN_MIPMAPS

    glTexParameteri(textarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

    glTexImage2D(GL_TEXTURE_2D,0,fmt2,
      newcx,newcy,
      0,fmt,GL_UNSIGNED_BYTE,img.getData());

#else

    int level = 0;
    uint8 *imd = img.getData();
    int cc = img.getcc();
    while (1) {
      glTexImage2D(GL_TEXTURE_2D,level,fmt2,
        newcx,newcy,
        0,fmt,GL_UNSIGNED_BYTE,imd);

      if (newcx <= 1 && newcy <= 1) break;

      if (newcx > 1) newcx /= 2;
      if (newcy > 1) newcy /= 2;
      level++;

      for (int y=0; y<newcy; y++) {
        for (int x=0; x<newcx; x++) {
          for (int z=0; z<cc; z++) {
            imd[(y*newcx+x)*cc+z] = (uint8)(((int)
              imd[(y*newcx*4+x*2+0)*cc+z] +
              imd[(y*newcx*4+x*2+1)*cc+z] +
              imd[(y*newcx*4+x*2+0+newcx*2)*cc+z] +
              imd[(y*newcx*4+x*2+1+newcx*2)*cc+z]) / 4);
          }
        }
      }
    }

#endif
  } else {
    glTexImage2D(GL_TEXTURE_2D,0,fmt2,
      newcx,newcy,
      0,fmt,GL_UNSIGNED_BYTE,img.getData());
  }
}

void PTexture::loadPiece(PImage &img, int offx, int offy, int sizex, int sizey, bool genMipmaps, bool clamp)
{
  unload();

  textarget = GL_TEXTURE_2D;

#ifdef USE_GEN_MIPMAPS
  if (genMipmaps && !SDL_GL_ExtensionSupported("GL_SGIS_generate_mipmap")) {
    PUtil::outLog() << "warning: can't generate mipmaps for texture" << std::endl;
    genMipmaps = false;
  }
#else
  genMipmaps = false;
#endif

  GLuint fmt,fmt2;

  switch (img.getcc()) {
  case 1:
    fmt = GL_LUMINANCE; fmt2 = GL_LUMINANCE; break;
  case 2:
    fmt = GL_LUMINANCE_ALPHA; fmt2 = GL_LUMINANCE_ALPHA; break;
  case 3:
    fmt = GL_RGB; fmt2 = GL_RGB; break;
  case 4:
    fmt = GL_RGBA; fmt2 = GL_RGBA; break;
  default:
    throw MakePException ("loading texture failed, unknown image format");
  }

  int cx = sizex, cy = sizey;
  int newcx=1, newcy=1, max;
  while (newcx < cx) newcx *= 2;
  while (newcy < cy) newcy *= 2;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
  if (newcx > max) newcx = max;
  if (newcy > max) newcy = max;

  int cc = img.getcc();

  uint8 *offsetdata = img.getData() + ((offy*img.getcx())+offx)*cc;

  char* buffer = new char[sizex * sizey * cc];

  for (int i = 0; i < sizey; i++) {
    memcpy(buffer + i * sizex * cc, offsetdata + i * img.getcx() * cc, sizey * cc);
  }

  if (newcx != cx || newcy != cy) {
    PImage newimage (newcx, newcy, img.getcc ());

    scaleImage (fmt,
        cx, cy, GL_UNSIGNED_BYTE, buffer,
        newcx, newcy, GL_UNSIGNED_BYTE, newimage.getData ());

    img.swap (newimage);

    memcpy(buffer, img.getData(), newcx * newcy * cc);
  }

  glGenTextures(1,&texid);
  bind();

  glTexParameteri(textarget,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  if (genMipmaps)
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  else
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

  if (clamp) {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  } else {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_REPEAT);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifdef USE_GEN_MIPMAPS
  if (genMipmaps)
    glTexParameteri(textarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
#endif

  glTexImage2D(GL_TEXTURE_2D,0,fmt2,
    newcx,newcy,
    0,fmt,GL_UNSIGNED_BYTE,buffer);

  delete[] buffer;
}

void PTexture::loadAlpha(const std::string &filename, bool genMipmaps, bool clamp)
{
  PImage image (filename);
  loadAlpha (image, genMipmaps, clamp);
  name = filename;
}

void PTexture::loadAlpha(PImage &img, bool genMipmaps, bool clamp)
{
  unload();

  textarget = GL_TEXTURE_2D;

#ifdef USE_GEN_MIPMAPS
  if (genMipmaps && !SDL_GL_ExtensionSupported("GL_SGIS_generate_mipmap")) {
    PUtil::outLog() << "warning: can't generate mipmaps for texture" << std::endl;
    genMipmaps = false;
  }
#endif

  GLuint fmt,fmt2;

  switch (img.getcc()) {
  case 1:
    fmt = GL_ALPHA; fmt2 = GL_ALPHA; break;
  case 2:
    fmt = GL_LUMINANCE_ALPHA; fmt2 = GL_LUMINANCE_ALPHA;
    PUtil::outLog() << "Warning: loadAlpha() has been used for image with 2 channels" << std::endl;
    break;
  case 3:
    fmt = GL_RGB; fmt2 = GL_RGB;
    PUtil::outLog() << "Warning: loadAlpha() has been used for RGB image" << std::endl;
    break;
  case 4:
    fmt = GL_RGBA; fmt2 = GL_RGBA;
    PUtil::outLog() << "Warning: loadAlpha() has been used for RGBA image" << std::endl;
    break;
  default:
    throw MakePException ("loading texture failed, unknown image format");
  }

  int cx = img.getcx(), cy = img.getcy();
  int newcx=1, newcy=1, max;
  while (newcx < cx) newcx *= 2;
  while (newcy < cy) newcy *= 2;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
  if (newcx > max) newcx = max;
  if (newcy > max) newcy = max;

  if (newcx != cx || newcy != cy) {
    PImage newimage (newcx, newcy, img.getcc ());

    scaleImage (fmt,
        cx, cy, GL_UNSIGNED_BYTE, img.getData (),
        newcx, newcy, GL_UNSIGNED_BYTE, newimage.getData ());

    img.swap (newimage);
  }

  glGenTextures(1,&texid);
  bind();

  glTexParameteri(textarget,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  if (genMipmaps)
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  else
    glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

  if (clamp) {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  } else {
    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_REPEAT);
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT,1);

  if (genMipmaps) {
#ifdef USE_GEN_MIPMAPS

    glTexParameteri(textarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

    glTexImage2D(GL_TEXTURE_2D,0,fmt2,
      newcx,newcy,
      0,fmt,GL_UNSIGNED_BYTE,img.getData());

#else

    int level = 0;
    uint8 *imd = img.getData();
    int cc = img.getcc();
    while (1) {
      glTexImage2D(GL_TEXTURE_2D,level,fmt2,
        newcx,newcy,
        0,fmt,GL_UNSIGNED_BYTE,imd);

      if (newcx <= 1 && newcy <= 1) break;

      if (newcx > 1) newcx /= 2;
      if (newcy > 1) newcy /= 2;
      level++;

      for (int y=0; y<newcy; y++) {
        for (int x=0; x<newcx; x++) {
          for (int z=0; z<cc; z++) {
            imd[(y*newcx+x)*cc+z] = (uint8)(((int)
              imd[(y*newcx*4+x*2+0)*cc+z] +
              imd[(y*newcx*4+x*2+1)*cc+z] +
              imd[(y*newcx*4+x*2+0+newcx*2)*cc+z] +
              imd[(y*newcx*4+x*2+1+newcx*2)*cc+z]) / 4);
          }
        }
      }
    }

#endif
  } else {
    glTexImage2D(GL_TEXTURE_2D,0,fmt2,
      newcx,newcy,
      0,fmt,GL_UNSIGNED_BYTE,img.getData());
  }
}

void PTexture::loadCubeMap(const std::string &filenamePrefix, const std::string &filenameSuffix, bool genMipmaps)
{
  PUtil::outLog() << "loading \"" << filenamePrefix << "*" << filenameSuffix << "\" (cube map)" << std::endl;
  unload();

  textarget = GL_TEXTURE_CUBE_MAP;

#ifdef USE_GEN_MIPMAPS
  if (genMipmaps && !SDL_GL_ExtensionSupported("GL_SGIS_generate_mipmap")) {
    PUtil::outLog() << "warning: can't generate mipmaps for texture" << std::endl;
    genMipmaps = false;
  }
#endif

  PImage img;

  glGenTextures(1,&texid);
  bind();

#ifdef USE_GEN_MIPMAPS
  GLenum sidetarget[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
  };
#endif

  const char *middlename[6] = { "px", "nx", "py", "ny", "pz", "nz" };

  for (int side=0; side<6; side++) {
    std::string filename = filenamePrefix + middlename[side] + filenameSuffix;

    img.load(filename);

    GLuint fmt,fmt2;

    switch (img.getcc()) {
    case 1:
      fmt = GL_LUMINANCE; fmt2 = GL_LUMINANCE; break;
    case 2:
      fmt = GL_LUMINANCE_ALPHA; fmt2 = GL_LUMINANCE_ALPHA; break;
    case 3:
      fmt = GL_RGB; fmt2 = GL_RGB; break;
    case 4:
      fmt = GL_RGBA; fmt2 = GL_RGBA; break;
    default:
      throw MakePException (filename + " load failed, unknown image format");
    }

    int cx = img.getcx(), cy = img.getcy();
    int newcx=1, newcy=1, max;
    while (newcx < cx) newcx *= 2;
    while (newcy < cy) newcy *= 2;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
    if (newcx > max) newcx = max;
    if (newcy > max) newcy = max;

    if (newcx != cx || newcy != cy) {
      PImage newimage (newcx, newcy, img.getcc ());

      scaleImage (fmt,
          cx, cy, GL_UNSIGNED_BYTE, img.getData (),
          newcx, newcy, GL_UNSIGNED_BYTE, newimage.getData ());

      img.swap (newimage);
    }

    glTexParameteri(textarget,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    if (genMipmaps)
      glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    else
      glTexParameteri(textarget,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glTexParameteri(textarget,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(textarget,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    if (genMipmaps) {
#ifdef USE_GEN_MIPMAPS

      glTexParameteri(textarget, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

      glTexImage2D(sidetarget[side],0,fmt2,
        newcx,newcy,
        0,fmt,GL_UNSIGNED_BYTE,img.getData());

#else

      int level = 0;
      uint8 *imd = img.getData();
      int cc = img.getcc();
      while (1) {
        glTexImage2D(GL_TEXTURE_2D,level,fmt2,
          newcx,newcy,
          0,fmt,GL_UNSIGNED_BYTE,imd);
        //break;
        if (newcx <= 1 && newcy <= 1) break;

        if (newcx > 1) newcx /= 2;
        if (newcy > 1) newcy /= 2;
        level++;

        for (int y=0; y<newcy; y++) {
          for (int x=0; x<newcx; x++) {
            for (int z=0; z<cc; z++) {
              imd[(y*newcx+x)*cc+z] = (uint8)(((int)
                imd[(y*newcx*4+x*2+0)*cc+z] +
                imd[(y*newcx*4+x*2+1)*cc+z] +
                imd[(y*newcx*4+x*2+0+newcx*2)*cc+z] +
                imd[(y*newcx*4+x*2+1+newcx*2)*cc+z]) / 4);
            }
          }
        }
      }

#endif
    } else {
      glTexImage2D(GL_TEXTURE_2D,0,fmt2,
        newcx,newcy,
        0,fmt,GL_UNSIGNED_BYTE,img.getData());
    }

    img.unload();
  }

  name = filenamePrefix;
}

void PTexture::bind() const
{
  glBindTexture(textarget, texid);
}

// static
void PTexture::unbind()
{
  // clear both used texture targets
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void PTexture::scaleImage(GLuint format,
    GLsizei width_in, GLsizei height_in, GLenum type_in, const void* data_in,
    GLsizei width_out, GLsizei height_out, GLenum type_out, void* data_out
    )
{
    int depth = 0;
    int pitch_in = 0, pitch_out = 0;

    unsigned int rm = 0, gm = 0, bm = 0, am = 0; // Masks for colors

    // TODO: those masks may be incorrect in terms of color order, but we don't really care about it as we're just performing scaling
    switch (format) {
    case GL_LUMINANCE: // 1 component
      depth = 8;
      rm = 0xFF;
      break;
    case GL_LUMINANCE_ALPHA: // 2 components
      depth = 16;
      rm = 0x00FF;
      gm = 0xFF00;
      break;
    case GL_RGB: // 3 components
      depth = 24;
      rm = 0x0000FF;
      gm = 0x00FF00;
      bm = 0xFF0000;
      break;
    case GL_RGBA: // 4 components
      depth = 32;
      rm = 0x000000FF;
      gm = 0x0000FF00;
      bm = 0x00FF0000;
      am = 0xFF000000;
      break;
    default:
      throw MakePException ("Scaling failed, unknown image format");
    }

    int bpp = depth >> 3;

    pitch_in = width_in * bpp;
    pitch_out = width_out * bpp;

	//std::cout << "Resize!" << width_in << " " << width_out << " " << height_in << " " << height_out << std::endl;

    SDL_Rect srcrect = {.x = 0, .y = 0, .w = width_in, .h = height_in};
    SDL_Rect dstrect = {.x = 0, .y = 0, .w = width_out, .h = height_out};
    SDL_Surface* src = SDL_CreateRGBSurfaceFrom((void*)data_in, width_in, height_in, depth, pitch_in, rm, gm, bm, am);
    SDL_Surface* dst = SDL_CreateRGBSurfaceFrom(data_out, width_out, height_out, depth, pitch_out, rm, gm, bm, am);

	SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE); // Disable alpha blending with DST values

    if (SDL_BlitScaled(src, &srcrect, dst, &dstrect) < 0)
        throw MakePException ("Scaling failed, error during scaling");

    // This won't affect the data
    SDL_FreeSurface(src);
    SDL_FreeSurface(dst);
}


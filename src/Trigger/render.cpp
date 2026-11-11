#include "../../engine/renderer/renderer.h"
#include "../../engine/core/interpolation.h"
#include "validation.h"
#include "engine/pbr/renderer.h"
#include "engine/world/renderer.h"
#include "engine/vegetation/renderer.h"
#include "engine/post/post.h"
#include "engine/car/car_renderer.h"
#include "engine/dressing/renderer.h"
#include "engine/world/state.h"
#include <glm/gtc/type_ptr.hpp>
// Creator: Saad AIT YAHIA - @github: Saad-programmer
#include "damage.h"
#include "main.h"
#include "vehicle.h"
#include <cmath>

void MainApp::resize()
{
    glClearColor(1.0,1.0,1.0,1.0);
    trackrs::draw::enable(GL_TEXTURE_2D);

    trackrs::draw::enable(GL_BLEND);
    trackrs::draw::blend(GL_ONE,GL_ZERO);

    trackrs::draw::depthFunc(GL_LEQUAL);
    trackrs::draw::enable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    trackrs::draw::enable(GL_CULL_FACE);

	trackrs::draw::viewport(0, 0, getWidth(), getHeight());

    trackrs::draw::enable(trackrs::draw::Fog);

    trackrs::draw::enable(trackrs::draw::Lighting);

    const GLfloat ambcol[] = {0.1f, 0.1f, 0.1f, 0.0f};
    trackrs::draw::lightModel(trackrs::draw::GlobalAmbient, ambcol);

    float white[] = { 1.0,1.0,1.0,1.0 };
    //float black[] = { 0.0,0.0,0.0,1.0 };
    trackrs::draw::material(trackrs::draw::MaterialColor,white);

    float spec[] = { 0.3f, 0.5f, 0.5f, 1.0f };
    trackrs::draw::material( trackrs::draw::Specular, spec);
    trackrs::draw::material( trackrs::draw::Shininess, 6.0f);

    float litcol[] = { 0.6,0.6,0.6,0.0 };
    trackrs::draw::light(trackrs::draw::Diffuse,litcol);
    trackrs::draw::light(trackrs::draw::Specular,litcol);

}

void drawBlades(float radius, float ang, float trace)
{
    float invtrace = 1.0 / trace;
    trackrs::draw::push();
    trackrs::draw::scale(radius, radius, 1.0);
    for (float ba=0; ba<PI*2.0-0.01; ba+=PI/2.0)
    {
        trackrs::draw::begin(GL_TRIANGLE_FAN);
        trackrs::draw::color(0.1,0.1,0.1,0.24 * invtrace);
        trackrs::draw::vertex(0.0,0.0);
        trackrs::draw::color(0.1,0.1,0.1,0.06 * invtrace);
        int num = (int)(trace / 0.1);
        if (num < 2) num = 2;
        float mult = trace / (float)(num-1);
        float angadd = ba + ang;
        for (int i=0; i<num; ++i)
        {
            float a = (float)i * mult + angadd;
            trackrs::draw::vertex(cos(a),sin(a));
        }
        trackrs::draw::end();
    }
    trackrs::draw::pop();
}

void MainApp::renderWater()
{
    validation::PassScope timing("water");
    if (cfg.getPbrTerrain()) {
        glm::mat4 view, projection;
        trackrs::draw::read(trackrs::draw::ViewMatrix, glm::value_ptr(view));
        trackrs::draw::read(trackrs::draw::ProjectionMatrix, glm::value_ptr(projection));
        const glm::vec3 eye(glm::inverse(view)[3]);
        trackrs::world::drawWater(projection * view, eye, game->water.height,
                                  [this](float x, float y) { return game->terrain->getHeight(x, y); },
                                  game->animationtime);
        return;
    }
    tex_water->bind();
    {
        float tgens[] = { 0.5,0,0,0 };
        float tgent[] = { 0,0.5,0,0 };
        trackrs::draw::uvPlane(trackrs::draw::S, tgens);
        trackrs::draw::uvPlane(trackrs::draw::T, tgent);
    }
    trackrs::draw::enable(trackrs::draw::GenerateS);
    trackrs::draw::enable(trackrs::draw::GenerateT);
    trackrs::draw::push();
    trackrs::draw::scale(20.0,20.0,1.0);
    trackrs::draw::blend(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    trackrs::draw::enable(GL_BLEND);
    // Keep coplanar terrain/water depth arithmetic on the GPU.
    trackrs::draw::worldSpace(true);
    {
        int minx = (int)(campos.x / 20.0)-20,
            maxx = minx + 40,
            miny = (int)(campos.y / 20.0)-20,
            maxy = miny + 40;
        for (int y=miny; y<maxy; ++y)
        {
            trackrs::draw::begin(GL_TRIANGLE_STRIP);
            for (int x=minx; x<=maxx; ++x)
            {
                float maxalpha = 0.5f;

                if (game->water.useralpha)
                    maxalpha = game->water.alpha;

                if (game->water.fixedalpha)
                {
                    trackrs::draw::color(1.0f, 1.0f, 1.0f, maxalpha);
                    trackrs::draw::vertex(x, y+1, game->water.height);
                    trackrs::draw::vertex(x, y, game->water.height);
                }
                else
                {
                    float ht,alpha;
                    ht = game->terrain->getHeight((x)*20.0,(y+1)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    trackrs::draw::color(1.0,1.0,1.0,alpha);
                    trackrs::draw::vertex(x, y+1, game->water.height);
                    ht = game->terrain->getHeight((x)*20.0,(y)*20.0);
                    alpha = 1.0 - exp(ht - game->water.height);
                    CLAMP(alpha, 0.0f, maxalpha);
                    trackrs::draw::color(1.0,1.0,1.0,alpha);
                    trackrs::draw::vertex(x, y, game->water.height);
                }
            }
            trackrs::draw::end();
        }
    }
    trackrs::draw::worldSpace(false);
    trackrs::draw::pop();
    trackrs::draw::blend(GL_ONE,GL_ZERO);

    trackrs::draw::disable(trackrs::draw::GenerateS);
    trackrs::draw::disable(trackrs::draw::GenerateT);
}

void MainApp::renderSky(const mat44f &cammat)
{
    validation::PassScope timing("sky");
    if(cfg.getPbrTerrain()){
      glm::mat4 projection,view;trackrs::draw::read(trackrs::draw::ProjectionMatrix,glm::value_ptr(projection));trackrs::draw::read(trackrs::draw::ViewMatrix,glm::value_ptr(view));
      trackrs::world::drawSky(projection,view,{campos.x,campos.y,campos.z},trackrs::world::state().linearCapture);return;
    }
    trackrs::draw::fog(trackrs::draw::FogDensity, game->weather.fog.density_sky);
    trackrs::draw::depthRange(0.999,1.0);
    trackrs::draw::disable(GL_CULL_FACE);
    trackrs::draw::push(); // 1
    trackrs::draw::identity();
    trackrs::draw::multiply(cammat);
    tex_sky[0]->bind();
#define CLRANGE     10
#define CLFACTOR    0.02//0.014
    trackrs::draw::matrix(trackrs::draw::TextureTransform);
    trackrs::draw::push();
    trackrs::draw::translate(cloudscroll,0.0,0.0);
    trackrs::draw::rotate(30.0,0.0,0.0,1.0);
    trackrs::draw::scale(0.4,0.4,1.0);
    for (int y=-CLRANGE; y<CLRANGE; y++)
    {
        trackrs::draw::begin(GL_TRIANGLE_STRIP);
        for (int x=-CLRANGE; x<CLRANGE+1; x++)
        {
            trackrs::draw::uv(x,y);
            trackrs::draw::vertex(x,y,0.3-(x*x+y*y)*CLFACTOR);
            trackrs::draw::uv(x,y+1);
            trackrs::draw::vertex(x,y+1,0.3-(x*x+(y+1)*(y+1))*CLFACTOR);
        }
        trackrs::draw::end();
    }
    trackrs::draw::pop();
    trackrs::draw::matrix(trackrs::draw::View);
    trackrs::draw::pop(); // 1
    trackrs::draw::enable(GL_CULL_FACE);
    trackrs::draw::depthRange(0.0,0.999);
    trackrs::draw::fog(trackrs::draw::FogDensity, game->weather.fog.density);
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

    trackrs::draw::finish();
}

void MainApp::renderStateLoading(float eyetranslation)
{
    UNREFERENCED_PARAMETER(eyetranslation);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::push();
    trackrs::draw::identity();
    trackrs::draw::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    trackrs::draw::matrix(trackrs::draw::View);

    tex_splash_screen->bind();

    trackrs::draw::disable(GL_DEPTH_TEST);
    trackrs::draw::disable(trackrs::draw::Fog);
    trackrs::draw::disable(trackrs::draw::Lighting);
    trackrs::draw::enable(GL_BLEND);
    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);

    trackrs::draw::begin(trackrs::draw::Quads);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      trackrs::draw::uv(1.0f,off_u); trackrs::draw::vertex(1.0f, 1.0f);
      trackrs::draw::uv(0.0f,off_u); trackrs::draw::vertex(-1.0f, 1.0f);
      trackrs::draw::uv(0.0f,off_l); trackrs::draw::vertex(-1.0f, -1.0f);
      trackrs::draw::uv(1.0f,off_l); trackrs::draw::vertex(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      trackrs::draw::uv(off_u,1.0f); trackrs::draw::vertex(1.0f, 1.0f);
      trackrs::draw::uv(off_l,1.0f); trackrs::draw::vertex(-1.0f, 1.0f);
      trackrs::draw::uv(off_l,0.0f); trackrs::draw::vertex(-1.0f, -1.0f);
      trackrs::draw::uv(off_u,0.0f); trackrs::draw::vertex(1.0f, -1.0f);
    }
    trackrs::draw::end();

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
    trackrs::draw::begin(trackrs::draw::Quads);
      trackrs::draw::uv(1.0f, 1.0f); trackrs::draw::vertex( LOGO_HRATIO,  LOGO_VRATIO);
      trackrs::draw::uv(0.0f, 1.0f); trackrs::draw::vertex(-LOGO_HRATIO,  LOGO_VRATIO);
      trackrs::draw::uv(0.0f, 0.0f); trackrs::draw::vertex(-LOGO_HRATIO, -LOGO_VRATIO);
      trackrs::draw::uv(1.0f, 0.0f); trackrs::draw::vertex( LOGO_HRATIO, -LOGO_VRATIO);
    trackrs::draw::end();
#undef LOGO_VRATIO
#undef LOGO_HRATIO

    trackrs::draw::enable(GL_DEPTH_TEST);
    trackrs::draw::enable(trackrs::draw::Fog);
    trackrs::draw::enable(trackrs::draw::Lighting);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::pop();
    trackrs::draw::matrix(trackrs::draw::View);
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

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::push();
    trackrs::draw::identity();
    trackrs::draw::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    trackrs::draw::matrix(trackrs::draw::View);

    tex_end_screen->bind();

    trackrs::draw::disable(GL_DEPTH_TEST);
    trackrs::draw::disable(trackrs::draw::Fog);
    trackrs::draw::disable(trackrs::draw::Lighting);
    trackrs::draw::blend(GL_ONE, GL_ZERO);

    trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);

    trackrs::draw::begin(trackrs::draw::Quads);
    // the background image is square and cut out a piece based on aspect ratio
    // -------- if aspect ratio is larger than 4:3
    // if aspect ratio is larger than 1:1
    if ((float)getWidth()/(float)getHeight() > 1.0f)
    {

      // lower and upper offset based on aspect ratio
      float off_l = (1 - ((float)getHeight() / (float)getWidth())) / 2.f;
      float off_u = 1 - off_l;
      trackrs::draw::uv(1.0f,off_u); trackrs::draw::vertex(1.0f, 1.0f);
      trackrs::draw::uv(0.0f,off_u); trackrs::draw::vertex(-1.0f, 1.0f);
      trackrs::draw::uv(0.0f,off_l); trackrs::draw::vertex(-1.0f, -1.0f);
      trackrs::draw::uv(1.0f,off_l); trackrs::draw::vertex(1.0f, -1.0f);
    }
    // other cases (including 4:3, in which case off_l and off_u are = 1)
    else
    {

      float off_l = (1 - ((float)getWidth() / (float)getHeight())) / 2.f;
      float off_u = 1 - off_l;
      trackrs::draw::uv(off_u,1.0f); trackrs::draw::vertex(1.0f, 1.0f);
      trackrs::draw::uv(off_l,1.0f); trackrs::draw::vertex(-1.0f, 1.0f);
      trackrs::draw::uv(off_l,0.0f); trackrs::draw::vertex(-1.0f, -1.0f);
      trackrs::draw::uv(off_u,0.0f); trackrs::draw::vertex(1.0f, -1.0f);
    }
    trackrs::draw::end();

    tex_fontSourceCodeOutlined->bind();

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::pop();
    trackrs::draw::push();
    trackrs::draw::identity();
    trackrs::draw::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    //trackrs::draw::ortho(-1, 1, -1, 1, -1, 1);
    //trackrs::draw::ortho(800, 0, 600, 0, -1, 1);
    trackrs::draw::matrix(trackrs::draw::View);

    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    trackrs::draw::push();

    float scroll = splashtimeout;
    const float maxscroll = (float)(NUMCREDITSTRINGS - 1) * 2.0f;
    RANGEADJUST(scroll, 0.0f, 0.9f, -10.0f, maxscroll);
    CLAMP_UPPER(scroll, maxscroll);

    trackrs::draw::scale(0.1f, 0.1f, 1.0f);

    trackrs::draw::translate(0.0f, scroll, 0.0f);

    for (int i = 0; i < (int)NUMCREDITSTRINGS; i++)
    {
        float level = fabsf(scroll + (float)i * -2.0f);
        RANGEADJUST(level, 0.0f, 9.0f, 3.0f, 0.0f);

        if (level > 0.0f)
        {
            CLAMP_UPPER(level, 1.0f);

            trackrs::draw::push();
            trackrs::draw::translate(0.0f, (float)i * -2.0f, 0.0f);

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

            trackrs::draw::scale(enlarge, enlarge, 0.0f);
            trackrs::draw::color(1.0f, 1.0f, 1.0f, level);

            getSSRender().drawText(creditstext[i], PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            trackrs::draw::pop();
        }
    }

    trackrs::draw::pop();

    trackrs::draw::enable(GL_DEPTH_TEST);
    trackrs::draw::enable(trackrs::draw::Fog);
    trackrs::draw::enable(trackrs::draw::Lighting);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::pop();
    trackrs::draw::matrix(trackrs::draw::View);
}

// render the car selection menu
void MainApp::renderStateChoose(float eyetranslation)
{
    PVehicleType *vtype = game->vehiclechoices[choose_type];

    glClearColor(0.0, 0.0, 0.0, 1.0);
    trackrs::draw::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

trackrs::draw::matrix(trackrs::draw::Projection);

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

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::identity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    trackrs::draw::matrix(trackrs::draw::View);


    trackrs::draw::push(); // 0

//    trackrs::draw::translate(-eyetranslation, 0.5f, -5.0f);
    trackrs::draw::translate(-eyetranslation, 0.9f, -5.0f);
    trackrs::draw::rotate(28.0f, 1.0f, 0.0f, 0.0f);

    trackrs::draw::disable(trackrs::draw::Fog);
    trackrs::draw::enable(trackrs::draw::Lighting);
    trackrs::draw::enable(GL_DEPTH_TEST);

    vec4f lpos = vec4f(0.0f, 1.0f, 0.0f, 0.0f);
    trackrs::draw::light( trackrs::draw::LightPosition, lpos);

    //float tmp = 1.0f;
    //float tmp = sinf(choose_spin * 2.0f) * 0.5f;
    float tmp = cosf(choose_spin * 2.0f) * 0.5f;
    tmp += choose_spin;
    trackrs::draw::rotate(90.0f, -1.0f, 0.0f, 0.0f);
    trackrs::draw::rotate(DEGREES(tmp), 0.0f, 0.0f, 1.0f);

    // render vehicle
    for (unsigned int i=0; i<vtype->part.size(); ++i)
    {
		trackrs::draw::push(); // 1

		vec3f vpos = vtype->part[i].render_ref_local.getPosition();
		trackrs::draw::translate(vpos.x, vpos.y, vpos.z);

		mat44f vorim = vtype->part[i].render_ref_local.getInverseOrientationMatrix();
		trackrs::draw::multiply(vorim);
		if (vtype->part[i].model)
		{
			trackrs::draw::push(); // 2

			float scale = vtype->part[i].scale;
			trackrs::draw::scale(scale,scale,scale);
			drawModel(*vtype->part[i].model, 1.0f);

			trackrs::draw::pop(); // 2
		}

		// render wheels
		if (vtype->wheelmodel)
		{
			for (unsigned int j=0; j<vtype->part[i].wheel.size(); j++)
			{

				trackrs::draw::push(); // 2

				vec3f &wpos = vtype->part[i].wheel[j].pt;
				trackrs::draw::translate(wpos.x, wpos.y, wpos.z);

				float scale = vtype->wheelscale * vtype->part[i].wheel[j].radius;
				trackrs::draw::scale(scale,scale,scale);

				drawModel(*vtype->wheelmodel, 1.0f);

				trackrs::draw::pop(); // 2
			}
		}

		trackrs::draw::pop(); // 1
	}

    trackrs::draw::pop(); // 0

    trackrs::draw::disable(trackrs::draw::Lighting);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::push();
    trackrs::draw::identity();
    trackrs::draw::ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    trackrs::draw::matrix(trackrs::draw::View);

    // use the same colors as the menu
    const GuiWidgetColors gwc = gui.getColors();

    tex_fontSourceCodeShadowed->bind();

    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    trackrs::draw::disable(GL_DEPTH_TEST);
    trackrs::draw::disable(trackrs::draw::Lighting);

    trackrs::draw::push(); // 0

    const GLdouble margin = (800.0 - 600.0 * cx / cy) / 2.0;

    trackrs::draw::ortho(margin, 600.0 * cx / cy + margin, 0.0, 600.0, -1.0, 1.0);

    trackrs::draw::push(); // 1
    trackrs::draw::translate(10.0f, 570.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("TrackRS", PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(790.0f, 570.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(
        "car selection " + std::to_string(choose_type + 1) + '/' + std::to_string(game->vehiclechoices.size()),
        PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(100.0f, 230.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.header.x, gwc.header.y, gwc.header.z, gwc.header.w);
    getSSRender().drawText(vtype->proper_name.substr(0, 9), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(100.0f, 200.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->proper_class.substr(0, 8), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(500.0f, 230.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Weight (Kg)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(500.0f, 190.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Engine (BHP)", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(500.0f, 150.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Wheel drive", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(500.0f, 110.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText("Roadholding", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(520.0f, 230.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(std::to_string(static_cast<int>(vtype->mass)), PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(520.0f, 190.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_enginepower, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(520.0f, 150.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_wheeldrive, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    trackrs::draw::push(); // 1
    trackrs::draw::translate(520.0f, 110.0f, 0.0f);
    trackrs::draw::scale(30.0f, 30.0f, 1.0f);
    trackrs::draw::color(gwc.strong.x, gwc.strong.y, gwc.strong.z, gwc.strong.w);
    getSSRender().drawText(vtype->pstat_roadholding, PTEXT_HZA_LEFT | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    std::string racename;

    if (lss.state == AM_TOP_EVT_PREP || lss.state == AM_TOP_PRAC_SEL_PREP)
        racename = events[lss.currentevent].name + ": " + events[lss.currentevent].levels[lss.currentlevel].name;
    else
    if (lss.state == AM_TOP_LVL_PREP)
        racename = levels[lss.currentlevel].name;

    trackrs::draw::push(); // 1
    trackrs::draw::translate(400.0f, 30.0f, 0.0f);
    trackrs::draw::scale(20.0f, 20.0f, 1.0f);
    trackrs::draw::color(gwc.weak.x, gwc.weak.y, gwc.weak.z, gwc.weak.w);
    getSSRender().drawText(racename, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    trackrs::draw::pop(); // 1

    if (vtype->getLocked()) {
      const std::string unlockevent = getVehicleUnlockEvent(vtype->getName());

      trackrs::draw::push(); // 1
      trackrs::draw::translate(400.0f, 425.0f, 0.0f);
      trackrs::draw::scale(40.0f, 40.0f, 1.0f);
      trackrs::draw::color(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
      getSSRender().drawText("Locked", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
      trackrs::draw::pop(); // 1

      if (unlockevent != "") {
        trackrs::draw::push(); // 1
        trackrs::draw::translate(400.0f, 375.0f, 0.0f);
        trackrs::draw::scale(20.0f, 20.0f, 20.0f);
        trackrs::draw::color(gwc.marked.x, gwc.marked.y, gwc.marked.z, gwc.marked.w);
        getSSRender().drawText("Complete event " + unlockevent + " to unlock", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        trackrs::draw::pop(); // 1
      }
    }

    trackrs::draw::pop(); // 0

    trackrs::draw::blend(GL_ONE, GL_ZERO);
    trackrs::draw::enable(GL_DEPTH_TEST);
    trackrs::draw::enable(trackrs::draw::Fog);
    trackrs::draw::enable(trackrs::draw::Lighting);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::pop();
    trackrs::draw::matrix(trackrs::draw::View);
}

void MainApp::renderStateGame(float eyetranslation)
{
    PVehicle *vehic = game->vehicle[0];

    // PBR + post: the 3D scene renders linear HDR into an offscreen target that
    // trackrs::post resolves (exposure, bloom, tone map) before the HUD.
    const bool postProcess = cfg.getPbrTerrain() && trackrs::post::settings().enabled;
    if (postProcess) {
        trackrs::post::begin(getWidth(), getHeight(), trackrs::pbr::shaderDirectory().parent_path().parent_path());
        trackrs::world::state().linearCapture = true;
    }

    trackrs::draw::clear(GL_DEPTH_BUFFER_BIT);
    //trackrs::draw::clear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::identity();

    float fnear = 0.1f, fov = 0.6f;
    float aspect = (float)getWidth() / (float)getHeight();
    stereoFrustum(-fnear*aspect*fov,fnear*aspect*fov,-fnear*fov,fnear*fov,fnear,100000.0f,
                  0.8f, eyetranslation);
    if (postProcess && trackrs::post::temporal()) {
        // Phase 3e TAA: sub-pixel jitter on the scene projection only (HUD,
        // shadow cascades and probes use their own projections).
        glm::mat4 projection;
        trackrs::draw::read(trackrs::draw::ProjectionMatrix, glm::value_ptr(projection));
        const glm::mat4 jittered = trackrs::post::jitterMatrix() * projection;
        trackrs::draw::identity();
        trackrs::draw::multiply(glm::value_ptr(jittered));
    }
    trackrs::world::state().temporalFrame = postProcess ? trackrs::post::temporalFrame() : 0u;
    trackrs::draw::matrix(trackrs::draw::View);

    trackrs::draw::color(1.0,1.0,1.0);

    vec4f fogcolor(game->weather.fog.color, 1.0f);
    trackrs::draw::fog(trackrs::draw::FogColor, fogcolor);

    trackrs::draw::depthRange(0.0,0.999);

    trackrs::draw::push(); // 0

    mat44f cammat = camori.getMatrix();
    mat44f cammat_inv = cammat.transpose();

    //trackrs::draw::translate(0.0,0.0,-40.0);
    trackrs::draw::translate(-eyetranslation, 0.0f, 0.0f);

    trackrs::draw::multiply(cammat);

    trackrs::draw::translate(-campos.x, -campos.y, -campos.z);
    glm::mat4 pbrView;trackrs::draw::read(trackrs::draw::ViewMatrix,glm::value_ptr(pbrView));
    trackrs::pbr::setWorldView(pbrView);
    glm::mat4 worldProjection;trackrs::draw::read(trackrs::draw::ProjectionMatrix,glm::value_ptr(worldProjection));
    trackrs::world::prepare(pbrView,worldProjection,cfg.getRenderQuality(),trackrs::pbr::shaderDirectory().parent_path().parent_path());
    trackrs::vegetation::setAnimationTime(game->animationtime); // deterministic under replays
    {
        std::vector<glm::vec3> cars;
        for (auto *v : game->vehicle) { const vec3f c = v->body->getPosition(); cars.push_back({c.x, c.y, c.z}); }
        trackrs::dressing::setFrame(game->animationtime, cars);
    }
    {
        auto &world = trackrs::world::state();
        if (world.settings.enabled && trackrs::world::headlightsOn(world.sun, world.settings.weather.overcast))
            for (auto *vehic : game->vehicle) {
                const vec3f p = vehic->body->getPosition();
                const mat44f &axes = vehic->body->getInverseOrientationMatrix(); // rows: right, forward, up
                auto g = [](const vec3f &v) { return glm::vec3(v.x, v.y, v.z); };
                const trackrs::world::BodyFrame frame{g(p), g(makevec3f(axes.row[0])), g(makevec3f(axes.row[1])),
                                                      g(makevec3f(axes.row[2]))};
                for (const auto &lamp : trackrs::world::headlights(frame))
                    trackrs::world::addLight(lamp);
            }
    }
    trackrs::pbr::setProbeScene([this](const glm::mat4& projection,const glm::mat4& view,const glm::vec3& center){
        if(!game||!game->terrain)return;
        trackrs::draw::SavedState saved;
        trackrs::draw::matrix(trackrs::draw::Projection);trackrs::draw::identity();trackrs::draw::multiply(glm::value_ptr(projection));
        trackrs::draw::matrix(trackrs::draw::View);trackrs::draw::identity();trackrs::draw::multiply(glm::value_ptr(view));
        const auto previousWorldView=trackrs::world::state().view;
        const bool previousLinear=trackrs::world::state().linearCapture;trackrs::world::state().linearCapture=true;
        trackrs::world::state().view=view;
        float sun[]={.2f,.5f,1,0};trackrs::draw::light(trackrs::draw::LightPosition,sun);
        game->terrain->render(vec3f(center.x,center.y,center.z),mat44f::identity(),true);
        trackrs::draw::flush();
        trackrs::world::state().view=previousWorldView;trackrs::world::state().linearCapture=previousLinear;
    });


    auto drawVehicles = [&]() {
    for (unsigned int v=0; v<game->vehicle.size(); ++v)
    {
        if (!renderowncar && v == 0 && !trackrs::world::state().depthPass) continue;

        PVehicle *vehic = game->vehicle[v];
        trackrs::draw::push();
        if (!validation::run.options.enabled) {
            const float alpha = game->sim->interpolationAlpha();
            const auto& previous = vehic->body->previous();
            const auto currentPosition = vehic->body->getPosition();
            const auto oldPosition = previous.getPosition();
            const auto a = previous.getOrientation(), b = vehic->body->getOrientation();
            const auto pose = trackrs::interpolatePose(
                {{oldPosition.x,oldPosition.y,oldPosition.z}, glm::quat(a.w,a.x,a.y,a.z)},
                {{currentPosition.x,currentPosition.y,currentPosition.z}, glm::quat(b.w,b.x,b.y,b.z)}, alpha);
            const auto q = pose.rotation;
            const auto orientation = quatf(q.x,q.y,q.z,q.w).getMatrix().transpose();
            trackrs::draw::translate(pose.position.x, pose.position.y, pose.position.z);
            trackrs::draw::multiply(orientation);
            trackrs::draw::multiply(vehic->body->getOrientationMatrix());
            trackrs::draw::translate(-currentPosition.x, -currentPosition.y, -currentPosition.z);
        }
        if (!trackrs::Renderer::car(*vehic, game->terrain->getRoadSurface(vehic->body->getPosition()), game->animationtime))
        for (unsigned int i=0; i<vehic->part.size(); ++i)
        {
            trackrs::Renderer::vehiclePart(*this, *vehic->type, vehic->part[i], vehic->type->part[i], 1.0f);
        }
        trackrs::draw::pop();
    }

    };

    if(cfg.getPbrTerrain() && cfg.getCsm()) {
      trackrs::world::renderShadows([&](const glm::mat4 &light){
        trackrs::draw::SavedState saved;
        trackrs::draw::matrix(trackrs::draw::Projection);trackrs::draw::identity();trackrs::draw::multiply(glm::value_ptr(light));
        trackrs::draw::matrix(trackrs::draw::View);trackrs::draw::identity();
        game->terrain->render(campos,cammat_inv);
        drawVehicles();trackrs::draw::flush();
      });
    }
    float lpos[] = { 0.2, 0.5, 1.0, 0.0 };
    trackrs::draw::light( trackrs::draw::LightPosition, lpos);

    trackrs::draw::colorBytes(255,255,255);

    trackrs::draw::disable(trackrs::draw::Lighting);

    trackrs::draw::activeTexture(GL_TEXTURE1);
    trackrs::draw::enable(GL_TEXTURE_2D);
    tex_detail->bind();
    float tgens[] = { 0.05, 0.0, 0.0, 0.0 };
    float tgent[] = { 0.0, 0.05, 0.0, 0.0 };
    trackrs::draw::uvPlane(trackrs::draw::S, tgens);
    trackrs::draw::uvPlane(trackrs::draw::T, tgent);
    trackrs::draw::enable(trackrs::draw::GenerateS);
    trackrs::draw::enable(trackrs::draw::GenerateT);
    trackrs::draw::activeTexture(GL_TEXTURE0);

    // draw terrain
    game->terrain->render(campos, cammat_inv);

    trackrs::draw::disable(trackrs::draw::GenerateS);
    trackrs::draw::disable(trackrs::draw::GenerateT);

    trackrs::draw::activeTexture(GL_TEXTURE1);
    trackrs::draw::disable(GL_TEXTURE_2D);
    trackrs::draw::activeTexture(GL_TEXTURE0);

    if (renderowncar && !(cfg.getPbrTerrain() && cfg.getCsm()))
    {
        if(cfg.getPbrTerrain())++trackrs::world::state().blobDraws;
        trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tex_shadow->bind();

        trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.7f);

        vec3f vpos = game->vehicle[0]->body->getPosition();
        vec3f forw = makevec3f(game->vehicle[0]->body->getOrientationMatrix().row[0]);
        float forwangle = atan2(forw.y, forw.x);
        game->terrain->drawSplat(vpos.x, vpos.y, 1.4f, forwangle + PI*0.5f);

        trackrs::draw::blend(GL_ONE, GL_ZERO);
    }

    renderSky(cammat);

    trackrs::draw::enable(trackrs::draw::Lighting);

    drawVehicles();

    trackrs::draw::disable(trackrs::draw::Lighting);

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
            trackrs::Renderer::vehiclePart(*this, *game->vehiclechoices[i], vehiclepart,
                game->vehiclechoices[i]->part[0], 0.5f);
            break;
          }
        }
    }

    trackrs::draw::depthMask(GL_FALSE);
    trackrs::draw::disable(GL_CULL_FACE);

    trackrs::draw::disable(GL_TEXTURE_2D);

    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#define RAINDROP_WIDTH          0.015
    const vec4f raindrop_col(0.5,0.5,0.5,0.4);

    vec3f offsetdrops = campos - campos_prev;

    std::vector<trackrs::draw::Particle> rainInstances;
    rainInstances.reserve(rain.size()*2);
    auto gpuVector=[](const vec3f& v){return glm::vec3(v.x,v.y,v.z);};
    for (const auto& drop : rain) {
        const auto first=drop.drop_pt+drop.drop_vect*drop.prevlife+offsetdrops;
        const auto last=drop.drop_pt+drop.drop_vect*drop.life;
        auto width=(campos-drop.drop_pt).cross(drop.drop_vect);
        const float length=width.length();
        if (length==0) continue;
        width*=RAINDROP_WIDTH/length;
        const auto middle=(first+last)*.5f;
        const auto along=(last-first)*.5f;
        const glm::vec4 edge(raindrop_col[0],raindrop_col[1],raindrop_col[2],0);
        const glm::vec4 center(raindrop_col[0],raindrop_col[1],raindrop_col[2],raindrop_col[3]);
        rainInstances.push_back({gpuVector(middle-width*.5f),gpuVector(width*.5f),gpuVector(along),edge,center});
        rainInstances.push_back({gpuVector(middle+width*.5f),gpuVector(width*.5f),gpuVector(along),center,edge});
    }
    trackrs::draw::particles(rainInstances);

#define SNOWFLAKE_POINT_SIZE        3.0f
#define SNOWFLAKE_BOX_SIZE          0.175f

// NOTE: must be greater than 1.0f
#define SNOWFLAKE_MAXLIFE           4.5f

    GLfloat ops; // Original Point Size, for to be restored

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
    {
        trackrs::draw::read(GL_POINT_SIZE, &ops);
        trackrs::draw::pointSize(SNOWFLAKE_POINT_SIZE);
    }
    else
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        trackrs::draw::enable(GL_TEXTURE_2D);
        trackrs::draw::blend(GL_SRC_COLOR, GL_ONE);
        tex_snowflake->bind();
    }

    std::vector<trackrs::draw::Particle> snowInstances;
    snowInstances.reserve(snowfall.size());
    for (const auto& sf : snowfall) {
        const auto point=sf.drop_pt+sf.drop_vect*sf.life;
        const float alpha=sf.life>SNOWFLAKE_MAXLIFE?0.f:sf.life>1.f?(sf.life-SNOWFLAKE_MAXLIFE)/(1.f-SNOWFLAKE_MAXLIFE):1.f;
        const glm::vec4 color(1,1,1,alpha);
        if (cfg.getSnowflaketype()==PConfig::SnowFlakeType::point) {
            snowInstances.push_back({gpuVector(point),{0,0,0},{0,0,0},color,color});
        } else {
            auto width=(campos-sf.drop_pt).cross(sf.drop_vect);
            const float length=width.length();
            if(length==0)continue;
            width*=SNOWFLAKE_BOX_SIZE/length;
            const vec3f x(width.x*.5f,width.y*.5f,0);
            const vec3f y(0,0,(width.z+SNOWFLAKE_BOX_SIZE)*.5f);
            snowInstances.push_back({gpuVector(point+x+y),gpuVector(x),gpuVector(y),color,color,{1,1,0,0},1});
        }
    }
    trackrs::draw::particles(snowInstances,cfg.getSnowflaketype()==PConfig::SnowFlakeType::point);

    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::point)
        trackrs::draw::pointSize(ops); // restore original point size

    // disable textures
    if (cfg.getSnowflaketype() == PConfig::SnowFlakeType::textured)
    {
        trackrs::draw::disable(GL_TEXTURE_2D);
        trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

            trackrs::draw::push(); // 1
            trackrs::draw::translate(game->checkpt[i].pt.x, game->checkpt[i].pt.y, game->checkpt[i].pt.z);
            trackrs::draw::scale(25.0f, 25.0f, 1.0f);

#if 0 // Checkpoint style one
            trackrs::draw::begin(GL_TRIANGLE_STRIP);

            for (float a = 0.0f; a < 0.99f; a += 0.05f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3] * a);
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                trackrs::draw::vertex(cosf(ang), sinf(ang), ht - 1.0f);
                trackrs::draw::vertex(cosf(ang), sinf(ang), ht + 1.0f);
            }

            for (float a = 1.0f; a < 2.01f; a += 0.05f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3] * (2.0f - a));
                float ang = cprotate + a * 6.0f;
                float ht = sinf(ang * 1.7f) * 7.0f + 8.0f;
                trackrs::draw::vertex(cosf(ang), sinf(ang), ht - 1.0f);
                trackrs::draw::vertex(cosf(ang), sinf(ang), ht + 1.0f);
            }

            trackrs::draw::end();
#else // Regular checkpoint style
            trackrs::draw::begin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 1.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
                trackrs::draw::vertex(cosf(a), sinf(a), ht - 1.0f);
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
                trackrs::draw::vertex(cosf(a), sinf(a), ht + 0.0f);
            }
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 1.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 0.0f);
            trackrs::draw::end();

            trackrs::draw::begin(GL_TRIANGLE_STRIP);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 0.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
                trackrs::draw::vertex(cosf(a), sinf(a), ht - 0.0f);
                trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
                trackrs::draw::vertex(cosf(a), sinf(a), ht + 1.0f);
            }
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 0.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 1.0f);
            trackrs::draw::end();
#endif
            trackrs::draw::pop(); // 1
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

            trackrs::draw::push(); // 1
            trackrs::draw::translate(game->codrivercheckpt[i].pt.x, game->codrivercheckpt[i].pt.y, game->codrivercheckpt[i].pt.z);
            trackrs::draw::scale(15.0f, 15.0f, 1.0f);

            trackrs::draw::begin(GL_TRIANGLE_STRIP);
            float ht = sinf(cprotate * 6.0f) * 7.0f + 8.0f;
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 1.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 0.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
                trackrs::draw::vertex(cosf(a), sinf(a), ht - 1.0f);
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
                trackrs::draw::vertex(cosf(a), sinf(a), ht + 0.0f);
            }
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 1.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 0.0f);
            trackrs::draw::end();

            trackrs::draw::begin(GL_TRIANGLE_STRIP);
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 0.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 1.0f);
            for (float a = PI/10.0f; a < PI*2.0f-0.01f; a += PI/10.0f)
            {
                trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
                trackrs::draw::vertex(cosf(a), sinf(a), ht - 0.0f);
                trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
                trackrs::draw::vertex(cosf(a), sinf(a), ht + 1.0f);
            }
            trackrs::draw::color(colr[0], colr[1], colr[2], colr[3]);
            trackrs::draw::vertex(1.0f, 0.0f, ht - 0.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f, ht + 1.0f);
            trackrs::draw::end();
            trackrs::draw::pop(); // 1
        }
#endif
    }

    trackrs::draw::enable(GL_TEXTURE_2D);

    if (game->water.enabled)
        renderWater();

    if (psys_dirt != nullptr) // cfg_dirteffect == false
        getSSRender().render(psys_dirt.get());

    trackrs::draw::depthMask(GL_TRUE);
    trackrs::draw::blend(GL_ONE,GL_ZERO);
    trackrs::draw::enable(trackrs::draw::Lighting);
    trackrs::draw::enable(GL_CULL_FACE);
    trackrs::draw::enable(trackrs::draw::Fog);

    trackrs::draw::disable(trackrs::draw::Lighting);

    if(trackrs::world::state().scene){trackrs::draw::flush();trackrs::world::state().scene=false;}
    trackrs::draw::pop(); // 0
    if (postProcess) {
        trackrs::world::state().linearCapture = false;
        glm::mat4 sceneProjection;
        trackrs::draw::read(trackrs::draw::ProjectionMatrix, glm::value_ptr(sceneProjection));
        // Phase 3e: motion vectors for the cars drawn this frame, then resolve.
        const auto velocity = trackrs::post::beginVelocity(pbrView, sceneProjection);
        if (velocity.active) {
            trackrs::car::drawVelocity(velocity.jittered, velocity.current, velocity.previous);
            trackrs::post::endVelocity();
        }
        trackrs::post::end(game->animationtime, sceneProjection, pbrView);
    }

    trackrs::draw::disable(GL_DEPTH_TEST);

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::push(); // 0
    trackrs::draw::identity();

    trackrs::draw::ortho(0 - hratio, hratio, 0 - vratio, vratio, 0 - 1.0, 1.0);
    trackrs::draw::matrix(trackrs::draw::View);

    trackrs::draw::push(); // 1

    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (showui)
    {
        game->renderCodriverSigns();

        trackrs::draw::push(); // 2
        // position of rpm dial and needle
        //trackrs::draw::translate( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.22f, 0.0f);
        trackrs::draw::translate( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.22f, 0.0f);
        trackrs::draw::scale(0.30f, 0.30f, 1.0f);

        tex_hud_revs->bind();
        trackrs::draw::color(1.0f, 1.0f, 1.0f);
        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::uv(1.0f,1.0f);
        trackrs::draw::vertex(1.0f,1.0f);
        trackrs::draw::uv(0.0f,1.0f);
        trackrs::draw::vertex(-1.0f,1.0f);
        trackrs::draw::uv(0.0f,0.0f);
        trackrs::draw::vertex(-1.0f,-1.0f);
        trackrs::draw::uv(1.0f,0.0f);
        trackrs::draw::vertex(1.0f,-1.0f);
        trackrs::draw::end();

        // draw the needle of the RPM dial
        trackrs::draw::rotate(225.0f - vehic->getEngineRPM() * 15.0f / 1000.0f, 0.0f, 0.0f, 1.0f);
        tex_hud_revneedle->bind();
        trackrs::draw::color(1.0f, 1.0f, 1.0f);
        trackrs::draw::push(); // 3
        trackrs::draw::translate(0.62f, 0.0f, 0.0f);
        trackrs::draw::scale(0.16f, 0.16f, 0.16f);
        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::uv(1.0f,1.0f);
        trackrs::draw::vertex(1.0f,1.0f);
        trackrs::draw::uv(0.0f,1.0f);
        trackrs::draw::vertex(-1.0f,1.0f);
        trackrs::draw::uv(0.0f,0.0f);
        trackrs::draw::vertex(-1.0f,-1.0f);
        trackrs::draw::uv(1.0f,0.0f);
        trackrs::draw::vertex(1.0f,-1.0f);
        trackrs::draw::end();
        trackrs::draw::pop(); // 3
        trackrs::draw::disable(GL_TEXTURE_2D);
        trackrs::draw::pop(); // 2
    }

    // checkpoint pointing arrow thing
#if 0
    trackrs::draw::push(); // 2

    trackrs::draw::translate(0.0f, 0.8f, 0.0f);

    trackrs::draw::scale(0.2f, 0.2f, 0.2f);

    trackrs::draw::rotate(-30.0f, 1.0f, 0.0f, 0.0f);
    trackrs::draw::rotate(DEGREES(nextcpangle), 0.0f, -1.0f, 0.0f);

    trackrs::draw::begin(GL_TRIANGLES);
    trackrs::draw::color(0.8f, 0.4f, 0.4f, 0.6f);
    trackrs::draw::vertex(0.0f, 0.0f, -2.0f);
    trackrs::draw::color(0.8f, 0.8f, 0.8f, 0.6f);
    trackrs::draw::vertex(1.0f, 0.0f, 1.0f);
    trackrs::draw::vertex(-1.0f, 0.0f, 1.0f);
    trackrs::draw::end();
    trackrs::draw::begin(GL_TRIANGLE_STRIP);
    trackrs::draw::color(0.8f, 0.4f, 0.4f, 0.6f);
    trackrs::draw::vertex(0.0f, 0.0f, -2.0f);
    trackrs::draw::color(1.0f, 0.5f, 0.5f, 0.6f);
    trackrs::draw::vertex(0.0f, 0.2f, -2.0f);
    trackrs::draw::color(0.8f, 0.8f, 0.8f, 0.6f);
    trackrs::draw::vertex(1.0f, 0.0f, 1.0f);
    trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.6f);
    trackrs::draw::vertex(1.0f, 0.2f, 1.0f);
    trackrs::draw::color(0.8f, 0.8f, 0.8f, 0.6f);
    trackrs::draw::vertex(-1.0f, 0.0f, 1.0f);
    trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.6f);
    trackrs::draw::vertex(-1.0f, 0.2f, 1.0f);
    trackrs::draw::color(0.8f, 0.4f, 0.4f, 0.6f);
    trackrs::draw::vertex(0.0f, 0.0f, -2.0f);
    trackrs::draw::color(1.0f, 0.5f, 0.5f, 0.6f);
    trackrs::draw::vertex(0.0f, 0.2f, -2.0f);
    trackrs::draw::end();

    trackrs::draw::pop(); // 2
#endif

    if (showmap)
    {
        // position and size of map
        //trackrs::draw::viewport(getWidth() * (5.75f/100.f), getHeight() * (6.15f/100.f), getHeight()/3.5f, getHeight()/3.5f);
        trackrs::draw::viewport(getWidth() * (2.5f/100.f), getHeight() * (2.5f/100.f), getHeight()/3.5f, getHeight()/3.5f);

        trackrs::draw::push(); // 2
        trackrs::draw::scale(hratio, vratio, 1.0f);

        if (game->terrain->getHUDMapTexture())
        {
            trackrs::draw::enable(GL_TEXTURE_2D);
            game->terrain->getHUDMapTexture()->bind();
        }

        trackrs::draw::matrix(trackrs::draw::TextureTransform);
        trackrs::draw::push(); // 3
        float scalefac = 1.0f / game->terrain->getMapSize();
        trackrs::draw::scale(scalefac, scalefac, 1.0f);
        trackrs::draw::translate(campos.x, campos.y, 0.0f);
        trackrs::draw::rotate(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
        trackrs::draw::scale(1.0f / 0.003f, 1.0f / 0.003f, 1.0f);

        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.7f);
        trackrs::draw::uv(1.0f, 1.0f);
        trackrs::draw::vertex(1.0f, 1.0f);
        trackrs::draw::uv(-1.0f, 1.0f);
        trackrs::draw::vertex(-1.0f, 1.0f);
        trackrs::draw::uv(-1.0f, -1.0f);
        trackrs::draw::vertex(-1.0f, -1.0f);
        trackrs::draw::uv(1.0f, -1.0f);
        trackrs::draw::vertex(1.0f, -1.0f);
        trackrs::draw::end();

        trackrs::draw::pop(); // 3
        trackrs::draw::matrix(trackrs::draw::View);

        trackrs::draw::disable(GL_TEXTURE_2D);

        trackrs::draw::push(); // 3
        trackrs::draw::scale(0.003f, 0.003f, 1.0f);
        trackrs::draw::rotate(DEGREES(-camera_angle), 0.0f, 0.0f, 1.0f);
        trackrs::draw::translate(-campos.x, -campos.y, 0.0f);
        for (unsigned int i=0; i<game->checkpt.size(); i++)
        {
            trackrs::draw::push(); // 4
            vec3f vpos = game->checkpt[i].pt;
            trackrs::draw::translate(vpos.x, vpos.y, 0.0f);
            trackrs::draw::rotate(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            trackrs::draw::scale(30.0f, 30.0f, 1.0f);
            vec4f colr = checkpoint_col[2];
            if ((int)i == vehic->nextcp)
            {
                float sc = 1.5f + sinf(cprotate * 10.0f) * 0.5f;
                trackrs::draw::scale(sc, sc, 1.0f);
                colr = checkpoint_col[0];
            }
            else if ((int)i == (vehic->nextcp + 1) % (int)game->checkpt.size())
            {
                colr = checkpoint_col[1];
            }
            trackrs::draw::begin(GL_TRIANGLE_FAN);
            trackrs::draw::color(colr);
            trackrs::draw::vertex(0.0f, 0.0f);
            trackrs::draw::color(colr[0], colr[1], colr[2], 0.0f);
            //trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f);
            trackrs::draw::vertex(0.0f, 1.0f);
            trackrs::draw::vertex(-1.0f, 0.0f);
            trackrs::draw::vertex(0.0f, -1.0f);
            trackrs::draw::vertex(1.0f, 0.0f);
            trackrs::draw::end();
            trackrs::draw::pop(); // 4
        }
        for (unsigned int i=0; i<game->vehicle.size(); i++)
        {
            trackrs::draw::push(); // 4
            vec3f vpos = game->vehicle[i]->body->getPosition();
            trackrs::draw::translate(vpos.x, vpos.y, 0.0f);
            trackrs::draw::rotate(DEGREES(camera_angle), 0.0f, 0.0f, 1.0f);
            trackrs::draw::scale(30.0f, 30.0f, 1.0f);
            trackrs::draw::begin(GL_TRIANGLE_FAN);
            trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::vertex(0.0f, 0.0f);
            trackrs::draw::color(1.0f, 1.0f, 1.0f, 0.0f);
            trackrs::draw::vertex(1.0f, 0.0f);
            trackrs::draw::vertex(0.0f, 1.0f);
            trackrs::draw::vertex(-1.0f, 0.0f);
            trackrs::draw::vertex(0.0f, -1.0f);
            trackrs::draw::vertex(1.0f, 0.0f);
            trackrs::draw::end();
            trackrs::draw::pop(); // 4
        }
        trackrs::draw::pop(); // 3

        trackrs::draw::pop(); // 2

        trackrs::draw::viewport(0, 0, getWidth(), getHeight());
    }

    trackrs::draw::enable(GL_TEXTURE_2D);

    if (showui)
    {
        // Work-around for the "TIME" label once penalty time is displayed
        float time_offset = 0.0f;
        /*
        tex_hud_gear->bind();
        trackrs::draw::push(); // 2

        trackrs::draw::translate(1.0f, 0.35f, 0.0f);
        trackrs::draw::scale(0.2f, 0.2f, 1.0f);
        trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::uv(1.0f,1.0f); trackrs::draw::vertex(1.0f,1.0f);
        trackrs::draw::uv(0.0f,1.0f); trackrs::draw::vertex(-1.0f,1.0f);
        trackrs::draw::uv(0.0f,0.0f); trackrs::draw::vertex(-1.0f,-1.0f);
        trackrs::draw::uv(1.0f,0.0f); trackrs::draw::vertex(1.0f,-1.0f);
        trackrs::draw::end();

        trackrs::draw::pop(); // 2
        */

        tex_fontSourceCodeOutlined->bind();

        // time counter
        trackrs::draw::push(); // 2

        // time position (other time strings inherit this position)
        // -hratio is left border, 0 is center, +hratio is right
        // hratio * (1/50) gives 1% of the entire width
        // +vratio is top border, 0 is middle, -vratio is bottom
        trackrs::draw::translate(-hratio + hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);

        // time label
        trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
        trackrs::draw::scale(0.125f, 0.125f, 1.0f);
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
            trackrs::draw::color(1.0f, 1.0f, 1.0f, a);
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
        trackrs::draw::color(0.5f, 1.0f, 0.5f, 1.0f);
        trackrs::draw::translate(0.0f, -0.8f, 0.0f);
        getSSRender().drawText(PUtil::formatTime(game->targettime), PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        {
            // show the time penalty if there is any
            const float timepen = game->uservehicle->offroadtime_total * game->offroadtime_penalty_multiplier;

            if (timepen >= 0.1f)
            {
                trackrs::draw::color(1.0f, 1.0f, 0.5f, 1.0f);
                trackrs::draw::translate(0.0f, -0.8f, 0.0f);
                getSSRender().drawText(PUtil::formatTime(timepen) + '+', PTEXT_HZA_LEFT | PTEXT_VTA_TOP);
                time_offset = 0.8;
            }
        }

        trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
        trackrs::draw::translate(0.0f, 1.32f + time_offset, 0.0f);
        trackrs::draw::scale(0.65f, 0.65f, 1.0f);
        getSSRender().drawText("TIME", PTEXT_HZA_LEFT | PTEXT_VTA_TOP);

        trackrs::draw::pop(); // 2

        // show Next/Total checkpoints
        {
            // checkpoint counter
            trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::push(); // 2
            const std::string totalcp = std::to_string(game->checkpt.size());
            const std::string nextcp = std::to_string(vehic->nextcp);

            // checkpoint position
            trackrs::draw::translate(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f), 0.0f);
            trackrs::draw::scale(0.125f, 0.125f, 1.0f);

              if (game->getFinishState() != Gamefinish::not_finished)
                  getSSRender().drawText(totalcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
              else
                  getSSRender().drawText(nextcp + '/' + totalcp, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            // checkpoint label
            trackrs::draw::translate(0.0f, 0.52f, 0.0f);
            trackrs::draw::scale(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("CKPT", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            trackrs::draw::pop(); // 2
        }

        // show Current/Total laps
        if (game->number_of_laps > 1)
        {
            const std::string currentlap = std::to_string(vehic->currentlap);
            const std::string number_of_laps = std::to_string(game->number_of_laps);

            trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::push(); // 2
            trackrs::draw::translate(hratio - hratio * (2.5f/50.f), vratio - vratio * (5.5f/50.f) - 0.20f, 0.0f);
            trackrs::draw::scale(0.125f, 0.125f, 1.0f);

            if (game->getFinishState() != Gamefinish::not_finished)
                getSSRender().drawText(number_of_laps + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
            else
                getSSRender().drawText(currentlap + '/' + number_of_laps, PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            trackrs::draw::translate(0.0f, 0.52f, 0.0f);
            trackrs::draw::scale(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("LAP", PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);

            trackrs::draw::pop(); // 2
        }

        if (cfg.getEnableFps())
        {
            std::stringstream stream;

            trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::push(); // 2
            trackrs::draw::translate(0.0f, vratio - vratio * (5.5f/50.f), 0.0f);
            trackrs::draw::scale(0.125f, 0.125f, 1.0f);
            stream << std::fixed << std::setprecision(1) << fps;
            getSSRender().drawText(stream.str(), PTEXT_HZA_CENTER | PTEXT_VTA_TOP);

            trackrs::draw::translate(0.0f, 0.52f, 0.0f);
            trackrs::draw::scale(0.65f, 0.65f, 1.0f);
            getSSRender().drawText("FPS", PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            trackrs::draw::pop(); // 2
        }

#ifdef INDEVEL
        // show codriver checkpoint text (the pace notes)
        if (!game->codrivercheckpt.empty() && vehic->nextcdcp != 0)
        {
            trackrs::draw::color(1.0f, 1.0f, 0.0f);
            trackrs::draw::push(); // 2
            trackrs::draw::translate(0.0f, 0.3f, 0.0f);
            trackrs::draw::scale(0.1f, 0.1f, 1.0f);
            getSSRender().drawText(game->codrivercheckpt[vehic->nextcdcp - 1].notes, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            trackrs::draw::pop(); // 2
        }
#endif

        trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);

        tex_fontSourceCodeBold->bind();

        // show current gear and speed
        {
            // gear number
            const int gear = vehic->getCurrentGear();
            const std::string buff = (gear >= 0) ? PUtil::formatInt(gear + 1, 1) : "R";

            trackrs::draw::push(); // 2
            // position of gear & speed number & label
            //trackrs::draw::translate( hratio * (1.f - (5.75f/50.f)) - 0.3f, -vratio * (40.f/50.f) + 0.21f, 0.0f);
            trackrs::draw::translate( hratio * (1.f - (2.5f/50.f)) - 0.3f, -vratio * (43.5f/50.f) + 0.21f, 0.0f);
            trackrs::draw::scale(0.20f, 0.20f, 1.0f);
            getSSRender().drawText(buff, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);

            // speed number
            const int speed = std::fabs(vehic->getWheelSpeed()) * cfg.getHudSpeedoMpsSpeedMult();
            std::string speedstr = std::to_string(speed);

            //trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::translate(1.1f, -0.625f, 0.0f);
            trackrs::draw::scale(0.5f, 0.5f, 1.0f);
            getSSRender().drawText(speedstr, PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            // speed label
            trackrs::draw::translate(0.0f, -0.82f, 0.0f);
            trackrs::draw::scale(0.5f, 0.5f, 1.0f);

            if (cfg.getSpeedUnit() == PConfig::Speedunit::mph)
                getSSRender().drawText("MPH", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);
            else
                getSSRender().drawText("km/h", PTEXT_HZA_RIGHT | PTEXT_VTA_CENTER);

            trackrs::draw::pop(); // 2
        }

        renderDamageIndicatorGroup();

#ifndef NDEBUG
        // draw revs for debugging
        trackrs::draw::push(); // 2
        trackrs::draw::translate(1.17f, 0.52f, 0.0f);
        trackrs::draw::scale(0.2f, 0.2f, 1.0f);
        getSSRender().drawText(std::to_string(vehic->getEngineRPM()), PTEXT_HZA_RIGHT | PTEXT_VTA_TOP);
        trackrs::draw::pop(); // 2
#endif

#ifndef NDEBUG
        // draw real time penalty for debugging
        trackrs::draw::push(); // 2
        trackrs::draw::scale(0.1f, 0.1f, 1.0f);
        trackrs::draw::translate(0.0f, -4.0f, 0.0f);
        trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
        tex_fontSourceCodeOutlined->bind();
        getSSRender().drawText(std::string("true time penalty: ") +
            std::to_string(game->getOffroadTime() * game->offroadtime_penalty_multiplier),
            PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        trackrs::draw::pop(); // 2
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
            trackrs::draw::push(); // 2
            trackrs::draw::identity();
            trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
            trackrs::draw::scale(0.25f, 0.25f, 1.0f);
            tex_hud_offroad->bind();
            trackrs::draw::begin(trackrs::draw::Quads);
                trackrs::draw::uv(   1.0f,   1.0f);
                trackrs::draw::vertex(     1.0f,   1.0f);
                trackrs::draw::uv(   0.0f,   1.0f);
                trackrs::draw::vertex(    -1.0f,   1.0f);
                trackrs::draw::uv(   0.0f,   0.0f);
                trackrs::draw::vertex(    -1.0f,  -1.0f);
                trackrs::draw::uv(   1.0f,   0.0f);
                trackrs::draw::vertex(     1.0f,  -1.0f);
            trackrs::draw::end();
            trackrs::draw::pop(); // 2
            trackrs::draw::push(); // 2
            trackrs::draw::scale(0.1f, 0.1f, 1.0f);
            trackrs::draw::translate(0.0f, -2.5f, 0.0f);
            trackrs::draw::color(1.0f, 1.0f, 0.0f, 1.0f);
            tex_fontSourceCodeOutlined->bind();
            getSSRender().drawText(
                std::to_string(static_cast<int> (game->getOffroadTime() * game->offroadtime_penalty_multiplier)) +
                " seconds",
                PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
            trackrs::draw::pop(); // 2
        }
    }

    // draw terrain info for debugging
    #ifdef INDEVEL
    {
        const vec3f wheelpos = vehic->part[0].wheel[0].ref_world.getPosition(); // wheel 0
        const TerrainType tt = game->terrain->getRoadSurface(wheelpos);
        const rgbcolor c = PUtil::getTerrainColor(tt);
        const std::string s = PUtil::getTerrainInfo(tt);

        trackrs::draw::push(); // 2
        trackrs::draw::translate(0.0f, 0.5f, 0.0f);
        trackrs::draw::scale(0.1f, 0.1f, 1.0f);

        if (tt != TerrainType::Unknown)
        {
            const GLfloat endx = s.length() * 8.0f / 12.0f + 0.1f;

            trackrs::draw::push(); // 3
            trackrs::draw::disable(GL_TEXTURE_2D);
            trackrs::draw::translate(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
            trackrs::draw::translate(0.0f, -1.0f, 0.0f);
            trackrs::draw::begin(GL_TRIANGLE_STRIP);
                trackrs::draw::color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
                trackrs::draw::vertex(-0.2f,   0.0f);
                trackrs::draw::vertex(endx,    0.0f);
                trackrs::draw::vertex(-0.2f,   1.1f);
                trackrs::draw::vertex(endx,    1.1f);
            trackrs::draw::end();
            trackrs::draw::enable(GL_TEXTURE_2D);
            trackrs::draw::pop(); // 3
        }

        trackrs::draw::color(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        trackrs::draw::pop(); // 2
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

        trackrs::draw::push(); // 2
        trackrs::draw::translate(0.0f, 0.25f, 0.0f);
        trackrs::draw::scale(0.1f, 0.1f, 1.0f);
        trackrs::draw::push(); // 3
        trackrs::draw::disable(GL_TEXTURE_2D);
        trackrs::draw::translate(-0.5f * s.length() * 8.0f / 12.0f, 0.0f, 0.0f);
        trackrs::draw::translate(0.0f, -1.0f, 0.0f);
        trackrs::draw::begin(GL_TRIANGLE_STRIP);
            trackrs::draw::color(c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
            trackrs::draw::vertex(-0.2f,   0.0f);
            trackrs::draw::vertex(endx,    0.0f);
            trackrs::draw::vertex(-0.2f,   1.1f);
            trackrs::draw::vertex(endx,    1.1f);
        trackrs::draw::end();
        trackrs::draw::enable(GL_TEXTURE_2D);
        trackrs::draw::pop(); // 3
        trackrs::draw::color(1.0f, 1.0f, 1.0f);
        getSSRender().drawText(s, PTEXT_HZA_CENTER | PTEXT_VTA_TOP);
        trackrs::draw::pop(); // 2
    }
    #endif
    {
        tex_fontSourceCodeOutlined->bind();

        trackrs::draw::color(1.0f, 0.0f, 0.0f, 1.0f);
        trackrs::draw::push(); // 2
        trackrs::draw::translate(0.0f, 0.2f, 0.0f);
        trackrs::draw::scale(0.6f, 0.6f, 1.0f);
        if (game->gamestate == Gamestate::countdown)
        {
            float sizer = fmodf(game->othertime, 1.0f) + 0.5f;
            trackrs::draw::scale(sizer, sizer, 1.0f);
            getSSRender().drawText(
                PUtil::formatInt(((int)game->othertime + 1), 1),
                PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->gamestate == Gamestate::finished)
        {
            if (game->getFinishState() == Gamefinish::pass)
            {
                trackrs::draw::color(0.5f, 1.0f, 0.5f, 1.0f);
                getSSRender().drawText("WIN", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                trackrs::draw::scale(0.5f, 0.5f, 1.0f);
                trackrs::draw::color(0.5f, 0.0f, 0.0f, 1.0f);
                getSSRender().drawText("TIME EXCEEDED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
        }
        else if (game->coursetime < 1.0f)
        {
            trackrs::draw::color(0.5f, 1.0f, 0.5f, 1.0f);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        else if (game->coursetime < 2.0f)
        {
            float a = 1.0f - (game->coursetime - 1.0f);
            trackrs::draw::color(0.5f, 1.0f, 0.5f, a);
            getSSRender().drawText("GO!", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
        }
        trackrs::draw::pop(); // 2

        if (game->gamestate == Gamestate::countdown)
        {
            trackrs::draw::push(); // 2
            trackrs::draw::translate(0.0f, 0.6f, 0.0f);
            trackrs::draw::scale(0.08f, 0.08f, 1.0f);
            if (game->othertime < 1.0f)
            {
                trackrs::draw::color(1.0f, 1.0f, 1.0f, game->othertime);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            else
            {
                trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
                getSSRender().drawText(game->comment, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            }
            trackrs::draw::pop(); // 2
        }

        if (pauserace)
        {
            trackrs::draw::push(); // 2
            trackrs::draw::color(0.25f, 0.25f, 1.0f, 1.0f);
            trackrs::draw::scale(0.25f, 0.25f, 1.0f);
            getSSRender().drawText("PAUSED", PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
            trackrs::draw::pop(); // 2
        }
    }

    trackrs::draw::pop(); // 1

    trackrs::draw::matrix(trackrs::draw::Projection);
    trackrs::draw::pop(); // 0
    trackrs::draw::matrix(trackrs::draw::View);

    trackrs::draw::blend(GL_ONE, GL_ZERO);

    trackrs::draw::enable(GL_DEPTH_TEST);
    trackrs::draw::enable(trackrs::draw::Lighting);
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

    trackrs::draw::push(); // 2

    trackrs::draw::translate(posx, posy, 0.0f);
    trackrs::draw::scale(scalex, scaley, 1.0f);
    texture->bind();
    trackrs::draw::color(red, green, blue, 0.5f);

    trackrs::draw::begin(trackrs::draw::Quads);
    trackrs::draw::uv(1.0f,1.0f);
    trackrs::draw::vertex(1.0f,1.0f);
    trackrs::draw::uv(0.0f,1.0f);
    trackrs::draw::vertex(-1.0f,1.0f);
    trackrs::draw::uv(0.0f,0.0f);
    trackrs::draw::vertex(-1.0f,-1.0f);
    trackrs::draw::uv(1.0f,0.0f);
    trackrs::draw::vertex(1.0f,-1.0f);
    trackrs::draw::end();

    trackrs::draw::pop(); // 2
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

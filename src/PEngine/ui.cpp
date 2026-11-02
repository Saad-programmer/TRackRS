// SPDX-License-Identifier: GPL-2.0-or-later
#include "pengine.h"
#include "menu.h"
#include "render.h"
#include <algorithm>
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

static void drawMoroccanStar(float cx, float cy, float r, const vec4f &color) {
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4fv(color);
  glLineWidth(1.5f);

  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < 4; ++i) {
    float angle = (float)i * PI / 2.0f;
    glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
  }
  glEnd();

  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < 4; ++i) {
    float angle = (float)i * PI / 2.0f + PI / 4.0f;
    glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
  }
  glEnd();
}

void Gui::render()
{
  bool has_left_menu = false;
  for (const auto &w : widget) {
    if (w.selectable && w.pos.x < 100.0f) {
      has_left_menu = true;
      break;
    }
  }

  if (has_left_menu) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.08f, 0.04f, 0.03f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(35.0f, 75.0f);
    glVertex2f(310.0f, 75.0f);
    glVertex2f(310.0f, 470.0f);
    glVertex2f(35.0f, 470.0f);
    glEnd();

    glLineWidth(2.0f);
    glColor4f(0.851f, 0.643f, 0.255f, 0.40f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(35.0f, 75.0f);
    glVertex2f(310.0f, 75.0f);
    glVertex2f(310.0f, 470.0f);
    glVertex2f(35.0f, 470.0f);
    glEnd();

    drawMoroccanStar(35.0f, 470.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(310.0f, 470.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(310.0f, 75.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(35.0f, 75.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));

    glEnable(GL_TEXTURE_2D);
  }

  for (unsigned int i = 0; i < widget.size(); i++) {

    switch(widget[i].type) {
    case GWT_LABEL: {
      vec4f colc = widget[i].colnormal;
      uint32 flags = PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM;
      vec2f ctr = widget[i].pos;
      float shift_x = 0.0f;

      if (widget[i].text == "TrackRS") {
         glPushMatrix();
         glTranslatef(15.0f, ctr.y - 12.0f, 0.0f);
         glScalef(55.0f, 55.0f, 1.0f);
         glEnable(GL_TEXTURE_2D);
         fonttex->bind();
         glColor4f(0.10f, 0.05f, 0.02f, 0.9f);
         ssRender->drawText(widget[i].text, flags);
         glPopMatrix();

         glPushMatrix();
         glTranslatef(10.0f, ctr.y - 10.0f, 0.0f);
         glScalef(55.0f, 55.0f, 1.0f);
         glEnable(GL_TEXTURE_2D);
         fonttex->bind();
         glColor4f(0.98f, 0.85f, 0.35f, 1.0f);
         ssRender->drawText(widget[i].text, flags);
         glPopMatrix();

         glPushMatrix();
         glTranslatef(12.0f, ctr.y - 32.0f, 0.0f);
         glScalef(13.0f, 13.0f, 1.0f);
         glEnable(GL_TEXTURE_2D);
         fonttex->bind();
         glColor4f(0.90f, 0.90f, 0.95f, 0.9f);
         ssRender->drawText("MERZOUGA DUNE STAGE - ERG CHEBBI, MOROCCO", flags);
         glPopMatrix();

         glDisable(GL_TEXTURE_2D);
         glEnable(GL_BLEND);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         glBegin(GL_QUADS);
         glColor4f(0.98f, 0.85f, 0.35f, 0.9f);
         glVertex2f(10.0f, ctr.y - 38.0f);
         glColor4f(0.85f, 0.55f, 0.20f, 0.7f);
         glVertex2f(340.0f, ctr.y - 38.0f);
         glColor4f(0.85f, 0.55f, 0.20f, 0.0f);
         glVertex2f(440.0f, ctr.y - 40.0f);
         glColor4f(0.98f, 0.85f, 0.35f, 0.9f);
         glVertex2f(10.0f, ctr.y - 40.0f);
         glEnd();
         
         glEnable(GL_TEXTURE_2D);
         drawMoroccanStar(355.0f, ctr.y - 38.0f, 6.0f, vec4f(0.98f, 0.85f, 0.35f, 0.9f));
         continue;
      }

      if (widget[i].selectable) {
        colc = INTERP(widget[i].colnormal, widget[i].colhover, widget[i].glow);
        shift_x = 22.0f * widget[i].glow;

        if (widget[i].glow > 0.01f) {
          glDisable(GL_TEXTURE_2D);
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          glColor4f(0.98f, 0.85f, 0.35f, 0.15f * widget[i].glow);
          glBegin(GL_QUADS);
          glVertex2f(ctr.x + shift_x - 14.0f, ctr.y - 10.0f);
          glVertex2f(ctr.x + shift_x + 230.0f, ctr.y - 10.0f);
          glVertex2f(ctr.x + shift_x + 230.0f, ctr.y + 26.0f);
          glVertex2f(ctr.x + shift_x - 14.0f, ctr.y + 26.0f);
          glEnd();

          glLineWidth(3.0f);
          glColor4f(0.98f, 0.85f, 0.35f, 0.90f * widget[i].glow);
          glBegin(GL_LINES);
          glVertex2f(ctr.x + shift_x - 14.0f, ctr.y - 10.0f);
          glVertex2f(ctr.x + shift_x - 14.0f, ctr.y + 26.0f);
          glEnd();

          glEnable(GL_TEXTURE_2D);
          drawMoroccanStar(ctr.x + shift_x - 26.0f, ctr.y + 8.0f, 5.0f, vec4f(0.98f, 0.85f, 0.35f, widget[i].glow));
        }
      }

      if (widget[i].selectable && widget[i].selected)
        flags |= PTEXT_HIGHLIGHT;

      glPushMatrix();
      glTranslatef(ctr.x + shift_x, ctr.y, 0.0f);

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
        drawMoroccanStar(365.0f, -6.0f, 4.0f, vec4f(colors.header.x, colors.header.y, colors.header.z, 0.8f));
      }

      float current_fontsize = widget[i].fontsize * (widget[i].selectable ? (1.0f + 0.06f * widget[i].glow) : 1.0f);
      glScalef(current_fontsize, current_fontsize, 1.0f);

      glEnable(GL_TEXTURE_2D);
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
        
        glColor4f(0.14f, 0.08f, 0.06f, 0.85f);
        glBegin(GL_QUADS);
        glVertex2f(min.x, min.y);
        glVertex2f(max.x, min.y);
        glVertex2f(max.x, max.y);
        glVertex2f(min.x, max.y);
        glEnd();
        
        glLineWidth(2.0f);
        if (widget[i].clickable && widget[i].glow > 0.01f) {
          float pulse = 0.7f + 0.3f * sinf((float)SDL_GetTicks() / 150.0f);
          glColor4f(0.95f, 0.80f, 0.40f, widget[i].glow * pulse);
        } else {
          glColor4f(0.851f, 0.643f, 0.255f, 0.5f);
        }
        
        glBegin(GL_LINE_LOOP);
        glVertex2f(min.x, min.y);
        glVertex2f(max.x, min.y);
        glVertex2f(max.x, max.y);
        glVertex2f(min.x, max.y);
        glEnd();

        glLineWidth(1.0f);
        glColor4f(0.851f, 0.643f, 0.255f, 0.25f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(min.x + 4.0f, min.y + 4.0f);
        glVertex2f(max.x - 4.0f, min.y + 4.0f);
        glVertex2f(max.x - 4.0f, max.y - 4.0f);
        glVertex2f(min.x + 4.0f, max.y - 4.0f);
        glEnd();

        vec4f star_col = vec4f(0.851f, 0.643f, 0.255f, 0.6f);
        drawMoroccanStar(min.x + 8.0f, min.y + 8.0f, 4.0f, star_col);
        drawMoroccanStar(max.x - 8.0f, min.y + 8.0f, 4.0f, star_col);
        drawMoroccanStar(max.x - 8.0f, max.y - 8.0f, 4.0f, star_col);
        drawMoroccanStar(min.x + 8.0f, max.y - 8.0f, 4.0f, star_col);
        
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

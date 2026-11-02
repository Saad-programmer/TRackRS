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
  trackrs::draw::disable(GL_TEXTURE_2D);
  trackrs::draw::enable(GL_BLEND);
  trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  trackrs::draw::color(color);
  trackrs::draw::lineWidth(1.5f);

  trackrs::draw::begin(GL_LINE_LOOP);
  for (int i = 0; i < 4; ++i) {
    float angle = (float)i * PI / 2.0f;
    trackrs::draw::vertex(cx + r * cosf(angle), cy + r * sinf(angle));
  }
  trackrs::draw::end();

  trackrs::draw::begin(GL_LINE_LOOP);
  for (int i = 0; i < 4; ++i) {
    float angle = (float)i * PI / 2.0f + PI / 4.0f;
    trackrs::draw::vertex(cx + r * cosf(angle), cy + r * sinf(angle));
  }
  trackrs::draw::end();
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
    trackrs::draw::disable(GL_TEXTURE_2D);
    trackrs::draw::enable(GL_BLEND);
    trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    trackrs::draw::color(0.08f, 0.04f, 0.03f, 0.55f);
    trackrs::draw::begin(trackrs::draw::Quads);
    trackrs::draw::vertex(35.0f, 75.0f);
    trackrs::draw::vertex(310.0f, 75.0f);
    trackrs::draw::vertex(310.0f, 470.0f);
    trackrs::draw::vertex(35.0f, 470.0f);
    trackrs::draw::end();

    trackrs::draw::lineWidth(2.0f);
    trackrs::draw::color(0.851f, 0.643f, 0.255f, 0.40f);
    trackrs::draw::begin(GL_LINE_LOOP);
    trackrs::draw::vertex(35.0f, 75.0f);
    trackrs::draw::vertex(310.0f, 75.0f);
    trackrs::draw::vertex(310.0f, 470.0f);
    trackrs::draw::vertex(35.0f, 470.0f);
    trackrs::draw::end();

    drawMoroccanStar(35.0f, 470.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(310.0f, 470.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(310.0f, 75.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));
    drawMoroccanStar(35.0f, 75.0f, 4.0f, vec4f(0.851f, 0.643f, 0.255f, 0.5f));

    trackrs::draw::enable(GL_TEXTURE_2D);
  }

  for (unsigned int i = 0; i < widget.size(); i++) {

    switch(widget[i].type) {
    case GWT_LABEL: {
      vec4f colc = widget[i].colnormal;
      uint32 flags = PTEXT_HZA_LEFT | PTEXT_VTA_BOTTOM;
      vec2f ctr = widget[i].pos;
      float shift_x = 0.0f;

      if (widget[i].text == "TrackRS") {
         trackrs::draw::push();
         trackrs::draw::translate(15.0f, ctr.y - 12.0f, 0.0f);
         trackrs::draw::scale(55.0f, 55.0f, 1.0f);
         trackrs::draw::enable(GL_TEXTURE_2D);
         fonttex->bind();
         trackrs::draw::color(0.10f, 0.05f, 0.02f, 0.9f);
         ssRender->drawText(widget[i].text, flags);
         trackrs::draw::pop();

         trackrs::draw::push();
         trackrs::draw::translate(10.0f, ctr.y - 10.0f, 0.0f);
         trackrs::draw::scale(55.0f, 55.0f, 1.0f);
         trackrs::draw::enable(GL_TEXTURE_2D);
         fonttex->bind();
         trackrs::draw::color(0.98f, 0.85f, 0.35f, 1.0f);
         ssRender->drawText(widget[i].text, flags);
         trackrs::draw::pop();

         trackrs::draw::push();
         trackrs::draw::translate(12.0f, ctr.y - 32.0f, 0.0f);
         trackrs::draw::scale(13.0f, 13.0f, 1.0f);
         trackrs::draw::enable(GL_TEXTURE_2D);
         fonttex->bind();
         trackrs::draw::color(0.90f, 0.90f, 0.95f, 0.9f);
         ssRender->drawText("MERZOUGA DUNE STAGE - ERG CHEBBI, MOROCCO", flags);
         trackrs::draw::pop();

         trackrs::draw::disable(GL_TEXTURE_2D);
         trackrs::draw::enable(GL_BLEND);
         trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         
         trackrs::draw::begin(trackrs::draw::Quads);
         trackrs::draw::color(0.98f, 0.85f, 0.35f, 0.9f);
         trackrs::draw::vertex(10.0f, ctr.y - 38.0f);
         trackrs::draw::color(0.85f, 0.55f, 0.20f, 0.7f);
         trackrs::draw::vertex(340.0f, ctr.y - 38.0f);
         trackrs::draw::color(0.85f, 0.55f, 0.20f, 0.0f);
         trackrs::draw::vertex(440.0f, ctr.y - 40.0f);
         trackrs::draw::color(0.98f, 0.85f, 0.35f, 0.9f);
         trackrs::draw::vertex(10.0f, ctr.y - 40.0f);
         trackrs::draw::end();
         
         trackrs::draw::enable(GL_TEXTURE_2D);
         drawMoroccanStar(355.0f, ctr.y - 38.0f, 6.0f, vec4f(0.98f, 0.85f, 0.35f, 0.9f));
         continue;
      }

      if (widget[i].selectable) {
        colc = INTERP(widget[i].colnormal, widget[i].colhover, widget[i].glow);
        shift_x = 22.0f * widget[i].glow;

        if (widget[i].glow > 0.01f) {
          trackrs::draw::disable(GL_TEXTURE_2D);
          trackrs::draw::enable(GL_BLEND);
          trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          trackrs::draw::color(0.98f, 0.85f, 0.35f, 0.15f * widget[i].glow);
          trackrs::draw::begin(trackrs::draw::Quads);
          trackrs::draw::vertex(ctr.x + shift_x - 14.0f, ctr.y - 10.0f);
          trackrs::draw::vertex(ctr.x + shift_x + 230.0f, ctr.y - 10.0f);
          trackrs::draw::vertex(ctr.x + shift_x + 230.0f, ctr.y + 26.0f);
          trackrs::draw::vertex(ctr.x + shift_x - 14.0f, ctr.y + 26.0f);
          trackrs::draw::end();

          trackrs::draw::lineWidth(3.0f);
          trackrs::draw::color(0.98f, 0.85f, 0.35f, 0.90f * widget[i].glow);
          trackrs::draw::begin(GL_LINES);
          trackrs::draw::vertex(ctr.x + shift_x - 14.0f, ctr.y - 10.0f);
          trackrs::draw::vertex(ctr.x + shift_x - 14.0f, ctr.y + 26.0f);
          trackrs::draw::end();

          trackrs::draw::enable(GL_TEXTURE_2D);
          drawMoroccanStar(ctr.x + shift_x - 26.0f, ctr.y + 8.0f, 5.0f, vec4f(0.98f, 0.85f, 0.35f, widget[i].glow));
        }
      }

      if (widget[i].selectable && widget[i].selected)
        flags |= PTEXT_HIGHLIGHT;

      trackrs::draw::push();
      trackrs::draw::translate(ctr.x + shift_x, ctr.y, 0.0f);

      if (widget[i].text.length() > 0 &&
          (widget[i].colnormal.x == colors.header.x &&
           widget[i].colnormal.y == colors.header.y &&
           widget[i].colnormal.z == colors.header.z)) {
         trackrs::draw::disable(GL_TEXTURE_2D);
         trackrs::draw::enable(GL_BLEND);
         trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         trackrs::draw::color(colors.header.x, colors.header.y, colors.header.z, 0.8f);
         trackrs::draw::lineWidth(2.0f);
         trackrs::draw::begin(GL_LINES);
         trackrs::draw::vertex(0.0f, -6.0f);
         trackrs::draw::vertex(350.0f, -6.0f);
         trackrs::draw::vertex(0.0f, -10.0f);
         trackrs::draw::vertex(200.0f, -10.0f);
         trackrs::draw::end();
         trackrs::draw::enable(GL_TEXTURE_2D);
        drawMoroccanStar(365.0f, -6.0f, 4.0f, vec4f(colors.header.x, colors.header.y, colors.header.z, 0.8f));
      }

      float current_fontsize = widget[i].fontsize * (widget[i].selectable ? (1.0f + 0.06f * widget[i].glow) : 1.0f);
      trackrs::draw::scale(current_fontsize, current_fontsize, 1.0f);

      trackrs::draw::enable(GL_TEXTURE_2D);
      fonttex->bind();

      trackrs::draw::color(colc);
      ssRender->drawText(widget[i].text, flags);
      trackrs::draw::pop();
      } break;

    case GWT_GRAPHIC: {
      vec2f min = widget[i].pos;
      vec2f max = widget[i].pos + widget[i].dims_min;

      if (!widget[i].tex) {
        trackrs::draw::disable(GL_TEXTURE_2D);
        trackrs::draw::enable(GL_BLEND);
        trackrs::draw::blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        trackrs::draw::color(0.14f, 0.08f, 0.06f, 0.85f);
        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::vertex(min.x, min.y);
        trackrs::draw::vertex(max.x, min.y);
        trackrs::draw::vertex(max.x, max.y);
        trackrs::draw::vertex(min.x, max.y);
        trackrs::draw::end();
        
        trackrs::draw::lineWidth(2.0f);
        if (widget[i].clickable && widget[i].glow > 0.01f) {
          float pulse = 0.7f + 0.3f * sinf((float)SDL_GetTicks() / 150.0f);
          trackrs::draw::color(0.95f, 0.80f, 0.40f, widget[i].glow * pulse);
        } else {
          trackrs::draw::color(0.851f, 0.643f, 0.255f, 0.5f);
        }
        
        trackrs::draw::begin(GL_LINE_LOOP);
        trackrs::draw::vertex(min.x, min.y);
        trackrs::draw::vertex(max.x, min.y);
        trackrs::draw::vertex(max.x, max.y);
        trackrs::draw::vertex(min.x, max.y);
        trackrs::draw::end();

        trackrs::draw::lineWidth(1.0f);
        trackrs::draw::color(0.851f, 0.643f, 0.255f, 0.25f);
        trackrs::draw::begin(GL_LINE_LOOP);
        trackrs::draw::vertex(min.x + 4.0f, min.y + 4.0f);
        trackrs::draw::vertex(max.x - 4.0f, min.y + 4.0f);
        trackrs::draw::vertex(max.x - 4.0f, max.y - 4.0f);
        trackrs::draw::vertex(min.x + 4.0f, max.y - 4.0f);
        trackrs::draw::end();

        vec4f star_col = vec4f(0.851f, 0.643f, 0.255f, 0.6f);
        drawMoroccanStar(min.x + 8.0f, min.y + 8.0f, 4.0f, star_col);
        drawMoroccanStar(max.x - 8.0f, min.y + 8.0f, 4.0f, star_col);
        drawMoroccanStar(max.x - 8.0f, max.y - 8.0f, 4.0f, star_col);
        drawMoroccanStar(min.x + 8.0f, max.y - 8.0f, 4.0f, star_col);
        
        trackrs::draw::enable(GL_TEXTURE_2D);
      }
      else {
        vec4f colc = vec4f(1.0f, 1.0f, 1.0f, 1.0f);
        if (widget[i].clickable) {
          colc = INTERP(widget[i].colclick, widget[i].colhover, widget[i].glow);
        } else {
          colc = widget[i].colnormal;
        }

        widget[i].tex->bind();
        trackrs::draw::color(colc);

        trackrs::draw::begin(trackrs::draw::Quads);
        trackrs::draw::uv(0.0f, 0.0f); trackrs::draw::vertex(min.x, min.y);
        trackrs::draw::uv(1.0f, 0.0f); trackrs::draw::vertex(max.x, min.y);
        trackrs::draw::uv(1.0f, 1.0f); trackrs::draw::vertex(max.x, max.y);
        trackrs::draw::uv(0.0f, 1.0f); trackrs::draw::vertex(min.x, max.y);
        trackrs::draw::end();
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
    trackrs::draw::color(1.0f,0.0f,0.0f,0.2f);
    break;
  case GWT_FILLER:
    trackrs::draw::color(0.0f,1.0f,0.0f,0.2f);
    break;
  case GWT_LABEL:
    trackrs::draw::color(0.0f,0.0f,1.0f,0.2f);
    break;
  }

  min = widget[w].pos;
  max = widget[w].pos + widget[w].dims_measure;

  trackrs::draw::disable(GL_TEXTURE_2D);
  trackrs::draw::begin(trackrs::draw::Quads);
  trackrs::draw::vertex(min.x, min.y);
  trackrs::draw::vertex(max.x, min.y);
  trackrs::draw::vertex(max.x, max.y);
  trackrs::draw::vertex(min.x, max.y);
  trackrs::draw::end();
  trackrs::draw::enable(GL_TEXTURE_2D);

  // Render this widget
  switch (widget[w].type) {
  default:
    break;

  case GWT_LABEL: {
    trackrs::draw::push();
    vec2f ctr = widget[w].pos + widget[w].dims_measure * 0.5f;
    trackrs::draw::translate(ctr.x, ctr.y, 0.0f);
    trackrs::draw::scale(widget[w].fontsize, widget[w].fontsize, 1.0f);
    trackrs::draw::color(1.0f, 1.0f, 1.0f, 1.0f);
    ssRender->drawText(widget[w].text, PTEXT_HZA_CENTER | PTEXT_VTA_CENTER);
    trackrs::draw::pop();
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

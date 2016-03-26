//
// "$Id$"
//
// Overlay window code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2016 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//


// A window using double-buffering and able to draw an overlay
// on top of that.  Uses the hardware to draw the overlay if
// possible, otherwise it just draws in the front buffer.

#include <config.h>
#include <FL/Fl.H>
#include <FL/Fl_Overlay_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window_Driver.H>


Fl_Overlay_Window::Fl_Overlay_Window(int W, int H, const char *l)
: Fl_Double_Window(0,0,W,H,l)
{
  overlay_ = 0;
  image(0);
  clear_flag(FORCE_POSITION);
}

Fl_Overlay_Window::Fl_Overlay_Window(int X, int Y, int W, int H, const char *l)
: Fl_Double_Window(X,Y,W,H,l)
{
  overlay_ = 0;
  image(0);
}

void Fl_Overlay_Window::show() {
  Fl_Double_Window::show();
  if (overlay_ && overlay_ != this) overlay_->show();
}

void Fl_Overlay_Window::hide() {
  Fl_Double_Window::hide();
}

void Fl_Overlay_Window::flush()
{
  driver()->flush_overlay();
}

void Fl_Overlay_Window::resize(int X, int Y, int W, int H) {
  Fl_Double_Window::resize(X,Y,W,H);
  if (overlay_ && overlay_!=this) overlay_->resize(0,0,w(),h());
}

/**
  Destroys the window and all child widgets.
*/
Fl_Overlay_Window::~Fl_Overlay_Window() {
  hide();
//  delete overlay; this is done by ~Fl_Group
}

#if !HAVE_OVERLAY

int Fl_Overlay_Window::can_do_overlay() {return 0;}

/**
  Call this to indicate that the overlay data has changed and needs to
  be redrawn.  The overlay will be clear until the first time this is
  called, so if you want an initial display you must call this after
  calling show().
*/
void Fl_Overlay_Window::redraw_overlay() {
  overlay_ = this;
  clear_damage((uchar)(damage()|FL_DAMAGE_OVERLAY));
  Fl::damage(FL_DAMAGE_CHILD);
}

#else
#include <FL/x.H>

extern XVisualInfo *fl_find_overlay_visual();
extern XVisualInfo *fl_overlay_visual;
extern Colormap fl_overlay_colormap;
extern unsigned long fl_transparent_pixel;
extern uchar fl_overlay; // changes how fl_color(x) works

class _Fl_Overlay : public Fl_Window {
  friend class Fl_Overlay_Window;
  void flush();
  void show();
public:
  _Fl_Overlay(int x, int y, int w, int h) :
    Fl_Window(x,y,w,h) {set_flag(INACTIVE);}
};

int Fl_Overlay_Window::can_do_overlay() {
  return fl_find_overlay_visual() != 0;
}

void _Fl_Overlay::show() {
  if (shown()) {Fl_Window::show(); return;}
  fl_background_pixel = int(fl_transparent_pixel);
  Fl_X::make_xid(this, fl_overlay_visual, fl_overlay_colormap);
  fl_background_pixel = -1;
  // find the outermost window to tell wm about the colormap:
  Fl_Window *w = window();
  for (;;) {Fl_Window *w1 = w->window(); if (!w1) break; w = w1;}
  XSetWMColormapWindows(fl_display, fl_xid(w), &(Fl_X::i(this)->xid), 1);
}

void _Fl_Overlay::flush() {
  fl_window = fl_xid(this);
#if defined(FLTK_USE_CAIRO)
      if (Fl::cairo_autolink_context()) Fl::cairo_make_current(this); // capture gc changes automatically to update the cairo context adequately
#endif
  fl_overlay = 1;
  Fl_Overlay_Window *w = (Fl_Overlay_Window *)parent();
  Fl_X *myi = Fl_X::i(this);
  if (damage() != FL_DAMAGE_EXPOSE) XClearWindow(fl_display, fl_xid(this));
  fl_clip_region(myi->region); myi->region = 0;
  w->draw_overlay();
  fl_overlay = 0;
}

void Fl_Overlay_Window::redraw_overlay() {
  if (!fl_display) return; // this prevents fluid -c from opening display
  if (!overlay_) {
    if (can_do_overlay()) {
      Fl_Group::current(this);
      overlay_ = new _Fl_Overlay(0,0,w(),h());
      Fl_Group::current(0);
    } else {
      overlay_ = this;	// fake the overlay
    }
  }
  if (shown()) {
    if (overlay_ == this) {
      clear_damage(damage()|FL_DAMAGE_OVERLAY);
      Fl::damage(FL_DAMAGE_CHILD);
    } else if (!overlay_->shown())
      overlay_->show();
    else
      overlay_->redraw();
  }
}

#endif

//
// End of "$Id$".
//

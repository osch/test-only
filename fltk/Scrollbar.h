//
// "$Id: Scrollbar.h,v 1.3 2003/04/20 03:17:49 easysw Exp $"
//
// Scrollbar, controls an integer position of a window of a given
// size inside a data set of a given total size.
//
// Copyright 1998-2003 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "fltk-bugs@fltk.org".
//

#ifndef fltk_Scrollbar_h
#define fltk_Scrollbar_h

#include "Slider.h"

namespace fltk {

class FL_API Scrollbar : public Slider {

public:
  Scrollbar(int x,int y,int w,int h, const char *l = 0);
  static NamedStyle* default_style;

  int value() {return int(Slider::value());}
  int value(int position, int size, int top, int total);
  int handle(int);

  int pagesize() const {return pagesize_;}
  void pagesize(int a) {pagesize_ = a;}

protected:
  void draw();

private:
  int pagesize_;
  static void timeout_cb(void*);
  void increment_cb();

};

}

#endif

//
// End of "$Id: Scrollbar.h,v 1.3 2003/04/20 03:17:49 easysw Exp $".
//

/*! \class fltk::MultiImage

  A Symbol containing a set of different Symbols.  The flags passed to
  draw() are used to select which one to draw.  This is most useful
  for making an image for a button that is different depending on
  whether it is pushed in, the current value is on, or when the mouse
  is over it.

*/

#include <fltk/MultiImage.h>
#include <fltk/draw.h>
#include <stdarg.h>

using namespace fltk;

/*! \fn MultiImage::MultiImage(Symbol& image0, Flags flags1, Symbol& image1);

  The set of images must be provided to the constructor. Only pointers
  to them are stored, you must not destroy them until after the MultiImage
  is destroyed (usually all of these are static variables).

  The flags select when the image is drawn. Useful flags are:
  - INACTIVE_R
  - VALUE True for button that is turned on
  - HIGHLIGHT True when the mouse is over widget or pushing it (you
  must also set highlight_color() to non-zero or most widgets will
  not redraw because they don't think they changed appearance).
  - PUSHED True if user is pushing button (it also
  inverts the VALUE setting, so you may want to swap your images)
  - FOCUSED True if button has the keyboard focus

  Typical example:

\code
  MultiImage buttonImage(OffImage,
		HIGHLIGHT, BrightOffImage,
		VALUE, OnImage,
		HIGHLIGHT|VALUE, BrightOnImage,
		INACTIVE_R, DimOffImage,
		INACTIVE_R|VALUE, DimOnImage);
\endcode
*/

void MultiImage::set(int count, Symbol* img0, ...) {
  int i;
  n_images = count>0 ? count : 1;
  pairs =  new MultiImagePair[n_images];
  pairs[0].image = img0;
  if (count>1) {
    va_list ap;
    va_start(ap, img0);
    for (i=1; i<count; i++) {
      pairs[i].flags = va_arg(ap,int);
      pairs[i].image = (Symbol*) va_arg(ap, void*);
    }
    va_end(ap);
  }
}


/*! It probably is useless for the images to be different sizes.
  However if they are, Image0 (the first
  one passed to the constructor) is used to figure out the size. */
void MultiImage::_measure(int& w, int& h) const {  pairs[0].image->measure(w,h); }

/*! Calls the same image that _draw() will call to get the inset. */
void MultiImage::inset(Rectangle& r) const {
  int which = 0;
  const Flags f = drawflags();
  for (int i = 1; i < n_images && pairs[i].image; i++) {
    if ((f & pairs[i].flags) == pairs[i].flags) {which = i; /*passed_flags = f&~flags[i];*/}
  }
  pairs[which].image->inset(r);
}

/*! Returns the info from the first image given to the constructor. */
bool MultiImage::fills_rectangle() const {return pairs[0].image->fills_rectangle();}

/*! Returns the info from the first image given to the constructor. */
bool MultiImage::is_frame() const {return pairs[0].image->is_frame();}

/*! Select one of the images and draw it. The last image with all the
  flags specified for it turned on will be drawn. If none of them match
  then Image0 is drawn.
*/
void MultiImage::_draw(const Rectangle& r) const
{
  int which = 0;
  const Flags f = drawflags();
  for (int i = 1; i < n_images && pairs[i].image; i++) {
    if ((f &  pairs[i].flags) == pairs[i].flags) {which = i; /*passed_flags = f&~flags[i];*/}
  }
  pairs[which].image->draw(r);
}

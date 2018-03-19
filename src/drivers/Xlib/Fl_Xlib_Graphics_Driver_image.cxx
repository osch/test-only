//
// "$Id$"
//
// Image drawing routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2018 by Bill Spitzak and others.
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

// I hope a simple and portable method of drawing color and monochrome
// images.  To keep this simple, only a single storage type is
// supported: 8 bit unsigned data, byte order RGB, and pixels are
// stored packed into rows with the origin at the top-left.  It is
// possible to alter the size of pixels with the "delta" argument, to
// add alpha or other information per pixel.  It is also possible to
// change the origin and direction of the image data by messing with
// the "delta" and "linedelta", making them negative, though this may
// defeat some of the shortcuts in translating the image for X.


// A list of assumptions made about the X display:

// bits_per_pixel must be one of 8, 16, 24, 32.

// scanline_pad must be a power of 2 and greater or equal to 8.

// PseudoColor visuals must have 8 bits_per_pixel (although the depth
// may be less than 8).  This is the only limitation that affects any
// modern X displays, you can't use 12 or 16 bit colormaps.

// The mask bits in TrueColor visuals for each color are
// contiguous and have at least one bit of each color.  This
// is not checked for.

// For 24 and 32 bit visuals there must be at least 8 bits of each color.

////////////////////////////////////////////////////////////////

#include <config.h>
#include "Fl_Xlib_Graphics_Driver.H"
#include "../X11/Fl_X11_Screen_Driver.H"
#include "../X11/Fl_X11_Window_Driver.H"
#  include <FL/Fl.H>
#  include <FL/fl_draw.H>
#  include <FL/platform.H>
#  include <FL/Fl_Image_Surface.H>
#  include <FL/Fl_Screen_Driver.H>
#  include "../../Fl_XColor.H"
#  include "../../flstring.h"
#if HAVE_XRENDER
#include <X11/extensions/Xrender.h>
// set this to 1 to activate experimental way to cache RGB images with Xrender Picture instead of X11 Pixmap
#define USE_XRENDER_PICTURE 0
#endif

static XImage xi;	// template used to pass info to X
static int bytes_per_pixel;
static int scanline_add;
static int scanline_mask;

static void (*converter)(const uchar *from, uchar *to, int w, int delta);
static void (*mono_converter)(const uchar *from, uchar *to, int w, int delta);

static int dir;		// direction-alternator
static int ri,gi,bi;	// saved error-diffusion value

#  if USE_COLORMAP
////////////////////////////////////////////////////////////////
// 8-bit converter with error diffusion

static void color8_converter(const uchar *from, uchar *to, int w, int delta) {
  int r=ri, g=gi, b=bi;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    to = to+(w-1);
    d = -delta;
    td = -1;
  } else {
    dir = 1;
    d = delta;
    td = 1;
  }
  for (; w--; from += d, to += td) {
    r += from[0]; if (r < 0) r = 0; else if (r>255) r = 255;
    g += from[1]; if (g < 0) g = 0; else if (g>255) g = 255;
    b += from[2]; if (b < 0) b = 0; else if (b>255) b = 255;
    Fl_Color i = fl_color_cube(r*FL_NUM_RED/256,g*FL_NUM_GREEN/256,b*FL_NUM_BLUE/256);
    Fl_XColor& xmap = fl_xmap[0][i];
    if (!xmap.mapped) {if (!fl_redmask) fl_xpixel(r,g,b); else fl_xpixel(i);}
    r -= xmap.r;
    g -= xmap.g;
    b -= xmap.b;
    *to = uchar(xmap.pixel);
  }
  ri = r; gi = g; bi = b;
}

static void mono8_converter(const uchar *from, uchar *to, int w, int delta) {
  int r=ri, g=gi, b=bi;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    to = to+(w-1);
    d = -delta;
    td = -1;
  } else {
    dir = 1;
    d = delta;
    td = 1;
  }
  for (; w--; from += d, to += td) {
    r += from[0]; if (r < 0) r = 0; else if (r>255) r = 255;
    g += from[0]; if (g < 0) g = 0; else if (g>255) g = 255;
    b += from[0]; if (b < 0) b = 0; else if (b>255) b = 255;
    Fl_Color i = fl_color_cube(r*FL_NUM_RED/256,g*FL_NUM_GREEN/256,b*FL_NUM_BLUE/256);
    Fl_XColor& xmap = fl_xmap[0][i];
    if (!xmap.mapped) {if (!fl_redmask) fl_xpixel(r,g,b); else fl_xpixel(i);}
    r -= xmap.r;
    g -= xmap.g;
    b -= xmap.b;
    *to = uchar(xmap.pixel);
  }
  ri = r; gi = g; bi = b;
}

#  endif

////////////////////////////////////////////////////////////////
// 16 bit TrueColor converters with error diffusion
// Cray computers have no 16-bit type, so we use character pointers
// (which may be slow)

#  ifdef U16
#    define OUTTYPE U16
#    define OUTSIZE 1
#    define OUTASSIGN(v) *t = v
#  else
#    define OUTTYPE uchar
#    define OUTSIZE 2
#    define OUTASSIGN(v) int tt=v; t[0] = uchar(tt>>8); t[1] = uchar(tt)
#  endif

static void color16_converter(const uchar *from, uchar *to, int w, int delta) {
  OUTTYPE *t = (OUTTYPE *)to;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    t = t+(w-1)*OUTSIZE;
    d = -delta;
    td = -OUTSIZE;
  } else {
    dir = 1;
    d = delta;
    td = OUTSIZE;
  }
  int r=ri, g=gi, b=bi;
  for (; w--; from += d, t += td) {
    r = (r&~fl_redmask)  +from[0]; if (r>255) r = 255;
    g = (g&~fl_greenmask)+from[1]; if (g>255) g = 255;
    b = (b&~fl_bluemask) +from[2]; if (b>255) b = 255;
    OUTASSIGN((
      ((r&fl_redmask)<<fl_redshift)+
      ((g&fl_greenmask)<<fl_greenshift)+
      ((b&fl_bluemask)<<fl_blueshift)
      ) >> fl_extrashift);
  }
  ri = r; gi = g; bi = b;
}

static void mono16_converter(const uchar *from,uchar *to,int w, int delta) {
  OUTTYPE *t = (OUTTYPE *)to;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    t = t+(w-1)*OUTSIZE;
    d = -delta;
    td = -OUTSIZE;
  } else {
    dir = 1;
    d = delta;
    td = OUTSIZE;
  }
  uchar mask = fl_redmask & fl_greenmask & fl_bluemask;
  int r=ri;
  for (; w--; from += d, t += td) {
    r = (r&~mask) + *from; if (r > 255) r = 255;
    uchar m = r&mask;
    OUTASSIGN((
      (m<<fl_redshift)+
      (m<<fl_greenshift)+
      (m<<fl_blueshift)
      ) >> fl_extrashift);
  }
  ri = r;
}

// special-case the 5r6g5b layout used by XFree86:

static void c565_converter(const uchar *from, uchar *to, int w, int delta) {
  OUTTYPE *t = (OUTTYPE *)to;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    t = t+(w-1)*OUTSIZE;
    d = -delta;
    td = -OUTSIZE;
  } else {
    dir = 1;
    d = delta;
    td = OUTSIZE;
  }
  int r=ri, g=gi, b=bi;
  for (; w--; from += d, t += td) {
    r = (r&7)+from[0]; if (r>255) r = 255;
    g = (g&3)+from[1]; if (g>255) g = 255;
    b = (b&7)+from[2]; if (b>255) b = 255;
    OUTASSIGN(((r&0xf8)<<8) + ((g&0xfc)<<3) + (b>>3));
  }
  ri = r; gi = g; bi = b;
}

static void m565_converter(const uchar *from,uchar *to,int w, int delta) {
  OUTTYPE *t = (OUTTYPE *)to;
  int d, td;
  if (dir) {
    dir = 0;
    from = from+(w-1)*delta;
    t = t+(w-1)*OUTSIZE;
    d = -delta;
    td = -OUTSIZE;
  } else {
    dir = 1;
    d = delta;
    td = OUTSIZE;
  }
  int r=ri;
  for (; w--; from += d, t += td) {
    r = (r&7) + *from; if (r > 255) r = 255;
    OUTASSIGN((r>>3) * 0x841);
  }
  ri = r;
}

////////////////////////////////////////////////////////////////
// 24bit TrueColor converters:

static void rgb_converter(const uchar *from, uchar *to, int w, int delta) {
  int d = delta-3;
  for (; w--; from += d) {
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
  }
}

static void bgr_converter(const uchar *from, uchar *to, int w, int delta) {
  for (; w--; from += delta) {
    uchar r = from[0];
    uchar g = from[1];
    *to++ = from[2];
    *to++ = g;
    *to++ = r;
  }
}

static void rrr_converter(const uchar *from, uchar *to, int w, int delta) {
  for (; w--; from += delta) {
    *to++ = *from;
    *to++ = *from;
    *to++ = *from;
  }
}

////////////////////////////////////////////////////////////////
// 32bit TrueColor converters on a 32 or 64-bit machine:

#  ifdef U64
#    define STORETYPE U64
#    if WORDS_BIGENDIAN
#      define INNARDS32(f) \
  U64 *t = (U64*)to; \
  int w1 = w/2; \
  for (; w1--; from += delta) {U64 i = f; from += delta; *t++ = (i<<32)|(f);} \
  if (w&1) *t++ = (U64)(f)<<32;
#    else
#      define INNARDS32(f) \
  U64 *t = (U64*)to; \
  int w1 = w/2; \
  for (; w1--; from += delta) {U64 i = f; from += delta; *t++ = ((U64)(f)<<32)|i;} \
  if (w&1) *t++ = (U64)(f);
#    endif
#  else
#    define STORETYPE U32
#    define INNARDS32(f) \
  U32 *t = (U32*)to; for (; w--; from += delta) *t++ = f
#  endif

static void rgbx_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((unsigned(from[0])<<24)+(from[1]<<16)+(from[2]<<8));
}

static void xbgr_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((from[0])+(from[1]<<8)+(from[2]<<16));
}

static void xrgb_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((from[0]<<16)+(from[1]<<8)+(from[2]));
}

static void argb_premul_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((unsigned(from[3]) << 24) +
             (((from[0] * from[3]) / 255) << 16) +
             (((from[1] * from[3]) / 255) << 8) +
             ((from[2] * from[3]) / 255));
}

static void depth2_to_argb_premul_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((unsigned(from[1]) << 24) +
            (((from[0] * from[1]) / 255) << 16) +
            (((from[0] * from[1]) / 255) << 8) +
            ((from[0] * from[1]) / 255));
}

static void bgrx_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32((from[0]<<8)+(from[1]<<16)+(unsigned(from[2])<<24));
}

static void rrrx_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32(unsigned(*from) * 0x1010100U);
}

static void xrrr_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32(*from * 0x10101U);
}

static void
color32_converter(const uchar *from, uchar *to, int w, int delta) {
  INNARDS32(
    (from[0]<<fl_redshift)+(from[1]<<fl_greenshift)+(from[2]<<fl_blueshift));
}

static void
mono32_converter(const uchar *from,uchar *to,int w, int delta) {
  INNARDS32(
    (*from << fl_redshift)+(*from << fl_greenshift)+(*from << fl_blueshift));
}

////////////////////////////////////////////////////////////////

static void figure_out_visual() {

  fl_xpixel(FL_BLACK); // setup fl_redmask, etc, in fl_color.cxx
  fl_xpixel(FL_WHITE); // also make sure white is allocated

  static XPixmapFormatValues *pfvlist;
  static int FL_NUM_pfv;
  if (!pfvlist) pfvlist = XListPixmapFormats(fl_display,&FL_NUM_pfv);
  XPixmapFormatValues *pfv;
  for (pfv = pfvlist; pfv < pfvlist+FL_NUM_pfv; pfv++)
    if (pfv->depth == fl_visual->depth) break;
  xi.format = ZPixmap;
  xi.byte_order = ImageByteOrder(fl_display);
//i.bitmap_unit = 8;
//i.bitmap_bit_order = MSBFirst;
//i.bitmap_pad = 8;
  xi.depth = fl_visual->depth;
  xi.bits_per_pixel = pfv->bits_per_pixel;

  if (xi.bits_per_pixel & 7) bytes_per_pixel = 0; // produce fatal error
  else bytes_per_pixel = xi.bits_per_pixel/8;

  unsigned int n = pfv->scanline_pad/8;
  if (pfv->scanline_pad & 7 || (n&(n-1)))
    Fl::fatal("Can't do scanline_pad of %d",pfv->scanline_pad);
  if (n < sizeof(STORETYPE)) n = sizeof(STORETYPE);
  scanline_add = n-1;
  scanline_mask = -n;

#  if USE_COLORMAP
  if (bytes_per_pixel == 1) {
    converter = color8_converter;
    mono_converter = mono8_converter;
    return;
  }
  if (!fl_visual->red_mask)
    Fl::fatal("Can't do %d bits_per_pixel colormap",xi.bits_per_pixel);
#  endif

  // otherwise it is a TrueColor visual:

  int rs = fl_redshift;
  int gs = fl_greenshift;
  int bs = fl_blueshift;

  switch (bytes_per_pixel) {

  case 2:
    // All 16-bit TrueColor visuals are supported on any machine with
    // 24 or more bits per integer.
#  ifdef U16
    xi.byte_order = WORDS_BIGENDIAN;
#  else
    xi.byte_order = 1;
#  endif
    if (rs == 11 && gs == 6 && bs == 0 && fl_extrashift == 3) {
      converter = c565_converter;
      mono_converter = m565_converter;
    } else {
      converter = color16_converter;
      mono_converter = mono16_converter;
    }
    break;

  case 3:
    if (xi.byte_order) {rs = 16-rs; gs = 16-gs; bs = 16-bs;}
    if (rs == 0 && gs == 8 && bs == 16) {
      converter = rgb_converter;
      mono_converter = rrr_converter;
    } else if (rs == 16 && gs == 8 && bs == 0) {
      converter = bgr_converter;
      mono_converter = rrr_converter;
    } else {
      Fl::fatal("Can't do arbitrary 24bit color");
    }
    break;

  case 4:
    if ((xi.byte_order!=0) != WORDS_BIGENDIAN)
      {rs = 24-rs; gs = 24-gs; bs = 24-bs;}
    if (rs == 0 && gs == 8 && bs == 16) {
      converter = xbgr_converter;
      mono_converter = xrrr_converter;
    } else if (rs == 24 && gs == 16 && bs == 8) {
      converter = rgbx_converter;
      mono_converter = rrrx_converter;
    } else if (rs == 8 && gs == 16 && bs == 24) {
      converter = bgrx_converter;
      mono_converter = rrrx_converter;
    } else if (rs == 16 && gs == 8 && bs == 0) {
      converter = xrgb_converter;
      mono_converter = xrrr_converter;
    } else {
      xi.byte_order = WORDS_BIGENDIAN;
      converter = color32_converter;
      mono_converter = mono32_converter;
    }
    break;

  default:
    Fl::fatal("Can't do %d bits_per_pixel",xi.bits_per_pixel);
  }

}

#  define MAXBUFFER 0x40000 // 256k

static void innards(const uchar *buf, int X, int Y, int W, int H,
		    int delta, int linedelta, int mono,
		    Fl_Draw_Image_Cb cb, void* userdata,
		    const bool alpha, GC gc)
{
  if (!linedelta) linedelta = W*abs(delta);

  int dx, dy, w, h;
  fl_clip_box(X,Y,W,H,dx,dy,w,h);
  if (w<=0 || h<=0) return;
  dx -= X;
  dy -= Y;
  if (!bytes_per_pixel) figure_out_visual();
  const unsigned oldbpp = bytes_per_pixel;
  static GC gc32 = None;
  xi.width = w;
  xi.height = h;

  void (*conv)(const uchar *from, uchar *to, int w, int delta) = converter;
  if (mono) conv = mono_converter;
  if (alpha) {
    // This flag states the destination format is ARGB32 (big-endian), pre-multiplied.
    bytes_per_pixel = 4;
    conv = (mono ? depth2_to_argb_premul_converter : argb_premul_converter);
    xi.depth = 32;
    xi.bits_per_pixel = 32;

    // Do we need a new GC?
    if (fl_visual->depth != 32) {
      if (gc32 == None)
        gc32 = XCreateGC(fl_display, fl_window, 0, NULL);
      gc = gc32;
    }
  }

  // See if the data is already in the right format.  Unfortunately
  // some 32-bit x servers (XFree86) care about the unknown 8 bits
  // and they must be zero.  I can't confirm this for user-supplied
  // data, so the 32-bit shortcut is disabled...
  // This can set bytes_per_line negative if image is bottom-to-top
  // I tested it on Linux, but it may fail on other Xlib implementations:
  if (buf && (
#  if 0	// set this to 1 to allow 32-bit shortcut
      delta == 4 &&
#    if WORDS_BIGENDIAN
      conv == rgbx_converter
#    else
      conv == xbgr_converter
#    endif
      ||
#  endif
      conv == rgb_converter && delta==3
      ) && !(linedelta&scanline_add)) {
    xi.data = (char *)(buf+delta*dx+linedelta*dy);
    xi.bytes_per_line = linedelta;

  } else {
    int linesize = ((w*bytes_per_pixel+scanline_add)&scanline_mask)/sizeof(STORETYPE);
    int blocking = h;
    static STORETYPE *buffer;	// our storage, always word aligned
    static long buffer_size;
    {int size = linesize*h;
    if (size > MAXBUFFER) {
      size = MAXBUFFER;
      blocking = MAXBUFFER/linesize;
    }
    if (size > buffer_size) {
      delete[] buffer;
      buffer_size = size;
      buffer = new STORETYPE[size];
    }}
    xi.data = (char *)buffer;
    xi.bytes_per_line = linesize*sizeof(STORETYPE);
    if (buf) {
      buf += delta*dx+linedelta*dy;
      for (int j=0; j<h; ) {
	STORETYPE *to = buffer;
	int k;
	for (k = 0; j<h && k<blocking; k++, j++) {
	  conv(buf, (uchar*)to, w, delta);
	  buf += linedelta;
	  to += linesize;
	}
	XPutImage(fl_display,fl_window,gc, &xi, 0, 0, X+dx, Y+dy+j-k, w, k);
      }
    } else {
      STORETYPE* linebuf = new STORETYPE[(W*delta+(sizeof(STORETYPE)-1))/sizeof(STORETYPE)];
      for (int j=0; j<h; ) {
	STORETYPE *to = buffer;
	int k;
	for (k = 0; j<h && k<blocking; k++, j++) {
	  cb(userdata, dx, dy+j, w, (uchar*)linebuf);
	  conv((uchar*)linebuf, (uchar*)to, w, delta);
	  to += linesize;
	}
	XPutImage(fl_display,fl_window,gc, &xi, 0, 0, X+dx, Y+dy+j-k, w, k);
      }

      delete[] linebuf;
    }
  }

  if (alpha) {
    bytes_per_pixel = oldbpp;
    xi.depth = fl_visual->depth;
    xi.bits_per_pixel = oldbpp * 8;
  }
}

void Fl_Xlib_Graphics_Driver::draw_image_unscaled(const uchar* buf, int x, int y, int w, int h, int d, int l){

  const bool alpha = !!(abs(d) & FL_IMAGE_WITH_ALPHA);
  if (alpha) d ^= FL_IMAGE_WITH_ALPHA;
  const int mono = (d>-3 && d<3);

  innards(buf,x+offset_x_*scale_,y+offset_y_*scale_,w,h,d,l,mono,0,0,alpha,gc_);
}

void Fl_Xlib_Graphics_Driver::draw_image_unscaled(Fl_Draw_Image_Cb cb, void* data,
		   int x, int y, int w, int h,int d) {

  const bool alpha = !!(abs(d) & FL_IMAGE_WITH_ALPHA);
  if (alpha) d ^= FL_IMAGE_WITH_ALPHA;
  const int mono = (d>-3 && d<3);

  innards(0,x+offset_x_*scale_,y+offset_y_*scale_,w,h,d,0,mono,cb,data,alpha,gc_);
}

void Fl_Xlib_Graphics_Driver::draw_image_mono_unscaled(const uchar* buf, int x, int y, int w, int h, int d, int l){
  innards(buf,x+offset_x_*scale_,y+offset_y_*scale_,w,h,d,l,1,0,0,0,gc_);
}

void Fl_Xlib_Graphics_Driver::draw_image_mono_unscaled(Fl_Draw_Image_Cb cb, void* data,
		   int x, int y, int w, int h,int d) {
  innards(0,x+offset_x_*scale_,y+offset_y_*scale_,w,h,d,0,1,cb,data,0,gc_);
}

void fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b) {
  if (fl_visual->depth > 16) {
    fl_color(r,g,b);
    fl_rectf(x,y,w,h);
  } else {
    uchar c[3];
    c[0] = r; c[1] = g; c[2] = b;
    innards(c,x,y,w,h,0,0,0,0,0,0,(GC)fl_graphics_driver->gc());
  }
}

Fl_Bitmask Fl_Xlib_Graphics_Driver::create_bitmask(int w, int h, const uchar *data) {
  return XCreateBitmapFromData(fl_display, fl_window, (const char *)data,
                               (w+7)&-8, h);
}

void Fl_Xlib_Graphics_Driver::delete_bitmask(Fl_Bitmask bm) {
  XFreePixmap(fl_display, bm);
}

void Fl_Xlib_Graphics_Driver::draw_unscaled(Fl_Bitmap *bm, float s, int X, int Y, int W, int H, int cx, int cy) {
  X = (X+offset_x_)*s;
  Y = (Y+offset_y_)*s;
  cache_size(bm, W, H);
  cx *= s; cy *= s;
  XSetStipple(fl_display, gc_, *Fl_Graphics_Driver::id(bm));
  int ox = X-cx; if (ox < 0) ox += bm->w()*s;
  int oy = Y-cy; if (oy < 0) oy += bm->h()*s;
  XSetTSOrigin(fl_display, gc_, ox, oy);
  XSetFillStyle(fl_display, gc_, FillStippled);
  XFillRectangle(fl_display, fl_window, gc_, X, Y, W, H);
  XSetFillStyle(fl_display, gc_, FillSolid);
}


// Composite an image with alpha on systems that don't have accelerated
// alpha compositing...
static void alpha_blend(Fl_RGB_Image *img, int X, int Y, int W, int H, int cx, int cy) {
  int ld = img->ld();
  if (ld == 0) ld = img->pixel_w() * img->d();
  uchar *srcptr = (uchar*)img->array + cy * ld + cx * img->d();
  int srcskip = ld - img->d() * W;

  uchar *dst = new uchar[W * H * 3];
  uchar *dstptr = dst;

  fl_read_image(dst, X, Y, W, H, 0);

  uchar srcr, srcg, srcb, srca;
  uchar dstr, dstg, dstb, dsta;

  if (img->d() == 2) {
    // Composite grayscale + alpha over RGB...
    for (int y = H; y > 0; y--, srcptr+=srcskip)
      for (int x = W; x > 0; x--) {
        srcg = *srcptr++;
        srca = *srcptr++;

        dstr = dstptr[0];
        dstg = dstptr[1];
        dstb = dstptr[2];
        dsta = 255 - srca;

        *dstptr++ = (srcg * srca + dstr * dsta) >> 8;
        *dstptr++ = (srcg * srca + dstg * dsta) >> 8;
        *dstptr++ = (srcg * srca + dstb * dsta) >> 8;
      }
  } else {
    // Composite RGBA over RGB...
    for (int y = H; y > 0; y--, srcptr+=srcskip)
      for (int x = W; x > 0; x--) {
        srcr = *srcptr++;
        srcg = *srcptr++;
        srcb = *srcptr++;
        srca = *srcptr++;

        dstr = dstptr[0];
        dstg = dstptr[1];
        dstb = dstptr[2];
        dsta = 255 - srca;

        *dstptr++ = (srcr * srca + dstr * dsta) >> 8;
        *dstptr++ = (srcg * srca + dstg * dsta) >> 8;
        *dstptr++ = (srcb * srca + dstb * dsta) >> 8;
      }
  }

  fl_draw_image(dst, X, Y, W, H, 3, 0);

  delete[] dst;
}

static Fl_Offscreen cache_rgb(Fl_RGB_Image *img) {
  Fl_Image_Surface *surface;
  int depth = img->d();
  if (depth == 1 || depth == 3) {
    surface = new Fl_Image_Surface(img->pixel_w(), img->pixel_h());
  } else if (fl_can_do_alpha_blending()) {
    Fl_Offscreen pixmap = XCreatePixmap(fl_display, RootWindow(fl_display, fl_screen), img->pixel_w(), img->pixel_h(), 32);
    surface = new Fl_Image_Surface(img->pixel_w(), img->pixel_h(), 0, pixmap);
    depth |= FL_IMAGE_WITH_ALPHA;
  } else {
    return 0;
  }
  Fl_Surface_Device::push_current(surface);
  fl_draw_image(img->array, 0, 0, img->pixel_w(), img->pixel_h(), depth, img->ld());
  Fl_Surface_Device::pop_current();
  Fl_Offscreen off = surface->get_offscreen_before_delete();
  delete surface;
#if HAVE_XRENDER && USE_XRENDER_PICTURE
  if (fl_can_do_alpha_blending()) {
    XRenderPictureAttributes srcattr;
    memset(&srcattr, 0, sizeof(XRenderPictureAttributes));
    static XRenderPictFormat *fmt32 = XRenderFindStandardFormat(fl_display, PictStandardARGB32);
    static XRenderPictFormat *fmt24 = XRenderFindStandardFormat(fl_display, PictStandardRGB24);
    Picture pict = XRenderCreatePicture(fl_display, off, (depth%2==0 ? fmt32:fmt24), 0, &srcattr);
    XFreePixmap(fl_display, off);
    off = pict;
  }
#endif
  return off;
}

// X,Y,W,H,cx,cy are in FLTK units
// if s != 1 and id(img) != 0, the offscreen has been previously scaled by s
// if s != 1 and id(img) == 0, img has been previously scaled by s
void Fl_Xlib_Graphics_Driver::draw_unscaled(Fl_RGB_Image *img, float s, int X, int Y, int W, int H, int cx, int cy) {
  X = (X+offset_x_)*s;
  Y = (Y+offset_y_)*s;
  cache_size(img, W, H);
  cx *= s; cy *= s;
  if (W + cx > img->pixel_w()) W = img->pixel_w() - cx;
  if (H + cy > img->pixel_h()) H = img->pixel_h() - cy;
  if (!*Fl_Graphics_Driver::id(img)) {
    *Fl_Graphics_Driver::id(img) = cache_rgb(img);
    *cache_scale(img) = 1;
  }
  Fl_Region r2 = scale_clip(s);
  if (*Fl_Graphics_Driver::id(img)) {
#if HAVE_XRENDER && USE_XRENDER_PICTURE
    int condition = can_do_alpha_blending();
#else
    int condition = 0;
#endif
    if (img->d() == 4 || img->d() == 2 || condition) {
#if HAVE_XRENDER
      scale_ = 1;
      scale_and_render_pixmap(*Fl_Graphics_Driver::id(img), img->d(), 1, 1, cx, cy, X, Y, W, H);
      scale_ = s;
#endif
    } else {
      XCopyArea(fl_display, *Fl_Graphics_Driver::id(img), fl_window, gc_, cx, cy, W, H, X, Y);
    }
  } else {
    // Composite image with alpha manually each time...
    scale_ = 1;
    int ox = offset_x_, oy = offset_y_;
    offset_x_ = offset_y_ = 0;
    Fl_X11_Screen_Driver *d = (Fl_X11_Screen_Driver*)Fl::screen_driver();
    int nscreen = Fl_Window::current()->driver()->screen_num();
    float keep = d->scale(nscreen);
    d->scale(nscreen, 1);
    alpha_blend(img, X, Y, W, H, cx, cy);
    d->scale(nscreen, keep);
    scale_ = s;
    offset_x_ = ox; offset_y_ = oy;
  }
  unscale_clip(r2);
}

void Fl_Xlib_Graphics_Driver::uncache(Fl_RGB_Image*, fl_uintptr_t &id_, fl_uintptr_t &mask_)
{
  if (id_) {
#if HAVE_XRENDER && USE_XRENDER_PICTURE
    if (can_do_alpha_blending()) XRenderFreePicture(fl_display, id_);
    else
#endif
      XFreePixmap(fl_display, (Fl_Offscreen)id_);
    id_ = 0;
  }
}

fl_uintptr_t Fl_Xlib_Graphics_Driver::cache(Fl_Bitmap *bm) {
  *cache_scale(bm) =  Fl_Scalable_Graphics_Driver::scale();
  return (fl_uintptr_t)create_bitmask(bm->pixel_w(), bm->pixel_h(), bm->array);
}

void Fl_Xlib_Graphics_Driver::draw_unscaled(Fl_Pixmap *pxm, float s, int X, int Y, int W, int H, int cx, int cy) {
  X = (X+offset_x_)*s;
  Y = (Y+offset_y_)*s;
  cache_size(pxm, W, H);
  cx *= s; cy *= s;
  Fl_Region r2 = scale_clip(s);
  if (*Fl_Graphics_Driver::mask(pxm)) {
    // make X use the bitmap as a mask:
    XSetClipMask(fl_display, gc_, *Fl_Graphics_Driver::mask(pxm));
    XSetClipOrigin(fl_display, gc_, X-cx, Y-cy);
    if (clip_region()) {
      // At this point, XYWH is the bounding box of the intersection between
      // the current clip region and the (portion of the) pixmap we have to draw.
      // The current clip region is often a rectangle. But, when a window with rounded
      // corners is moved above another window, expose events may create a complex clip
      // region made of several (e.g., 10) rectangles. We have to draw only in the clip
      // region, and also to mask out the transparent pixels of the image. This can't
      // be done in a single Xlib call for a multi-rectangle clip region. Thus, we
      // process each rectangle of the intersection between the clip region and XYWH.
      // See also STR #3206.
      Region r = XRectangleRegion(X,Y,W,H);
      XIntersectRegion(r, clip_region(), r);
      int X1, Y1, W1, H1;
      for (int i = 0; i < r->numRects; i++) {
        X1 = r->rects[i].x1;
        Y1 = r->rects[i].y1;
        W1 = r->rects[i].x2 - r->rects[i].x1;
        H1 = r->rects[i].y2 - r->rects[i].y1;
        XCopyArea(fl_display, *Fl_Graphics_Driver::id(pxm), fl_window, gc_, cx + (X1 - X), cy + (Y1 - Y), W1, H1, X1, Y1);
      }
      XDestroyRegion(r);
    } else {
      XCopyArea(fl_display, *Fl_Graphics_Driver::id(pxm), fl_window, gc_, cx, cy, W, H, X, Y);
    }
    // put the old clip region back
    XSetClipOrigin(fl_display, gc_, 0, 0);
    s = scale_; scale_ = 1;
    restore_clip();
    scale_ = s;
  }
  else XCopyArea(fl_display, *Fl_Graphics_Driver::id(pxm), fl_window, gc_, cx, cy, W, H, X, Y);
  unscale_clip(r2);
}


fl_uintptr_t Fl_Xlib_Graphics_Driver::cache(Fl_Pixmap *pxm) {
  Fl_Image_Surface *surf = new Fl_Image_Surface(pxm->pixel_w(), pxm->pixel_h());
  Fl_Surface_Device::push_current(surf);
  uchar *bitmap = 0;
  Fl_Surface_Device::surface()->driver()->mask_bitmap(&bitmap);
  fl_draw_pixmap(pxm->data(), 0, 0, FL_BLACK);
  Fl_Surface_Device::surface()->driver()->mask_bitmap(0);
  if (bitmap) {
    *Fl_Graphics_Driver::mask(pxm) = (fl_uintptr_t)create_bitmask(pxm->pixel_w(), pxm->pixel_h(), bitmap);
    delete[] bitmap;
  }
  Fl_Surface_Device::pop_current();
  Fl_Offscreen id = surf->get_offscreen_before_delete();
  delete surf;
  *cache_scale(pxm) =  1;
  return (fl_uintptr_t)id;
}

void Fl_Xlib_Graphics_Driver::uncache_pixmap(fl_uintptr_t offscreen) {
  XFreePixmap(fl_display, (Fl_Offscreen)offscreen);
}


#if HAVE_XRENDER
/* Draws with Xrender an Fl_Offscreen with optional scaling and accounting for transparency if necessary.
 XP,YP,WP,HP are in drawing units
 */
int Fl_Xlib_Graphics_Driver::scale_and_render_pixmap(Fl_Offscreen pixmap, int depth, double scale_x, double scale_y, int srcx, int srcy, int XP, int YP, int WP, int HP) {
  bool has_alpha = (depth == 2 || depth == 4);
  XRenderPictureAttributes srcattr;
  memset(&srcattr, 0, sizeof(XRenderPictureAttributes));
  static XRenderPictFormat *fmt24 = XRenderFindStandardFormat(fl_display, PictStandardRGB24);
#if USE_XRENDER_PICTURE
  Picture src = pixmap;
#else
  static XRenderPictFormat *fmt32 = XRenderFindStandardFormat(fl_display, PictStandardARGB32);
  Picture src = XRenderCreatePicture(fl_display, pixmap, has_alpha ?fmt32:fmt24, 0, &srcattr);
#endif
  Picture dst = XRenderCreatePicture(fl_display, fl_window, fmt24, 0, &srcattr);
  if (!src || !dst) {
    fprintf(stderr, "Failed to create Render pictures (%lu %lu)\n", src, dst);
    return 0;
  }
  Fl_Region r = scale_clip(scale_);
  const Fl_Region clipr = clip_region();
  if (clipr)
    XRenderSetPictureClipRegion(fl_display, dst, clipr);
  unscale_clip(r);
#if !USE_XRENDER_PICTURE
  if (scale_x != 1 || scale_y != 1) {
#endif
    XTransform mat = {{
      { XDoubleToFixed( scale_x ), XDoubleToFixed( 0 ),       XDoubleToFixed( 0 ) },
      { XDoubleToFixed( 0 ),       XDoubleToFixed( scale_y ), XDoubleToFixed( 0 ) },
      { XDoubleToFixed( 0 ),       XDoubleToFixed( 0 ),       XDoubleToFixed( 1 ) }
    }};
    XRenderSetPictureTransform(fl_display, src, &mat);
#if !USE_XRENDER_PICTURE
  }
#endif
  XRenderComposite(fl_display, (has_alpha ? PictOpOver : PictOpSrc), src, None, dst, srcx, srcy, 0, 0,
                   XP, YP, WP, HP);
#if !USE_XRENDER_PICTURE
  XRenderFreePicture(fl_display, src);
#endif
  XRenderFreePicture(fl_display, dst);
  return 1;
}

// XP,YP,WP,HP are in FLTK units
int Fl_Xlib_Graphics_Driver::draw_scaled(Fl_Image *img, int XP, int YP, int WP, int HP) {
  Fl_RGB_Image *rgb = img->as_rgb_image();
  if (!rgb || !can_do_alpha_blending()) return 0;
  if (!*Fl_Graphics_Driver::id(rgb)) {
    *Fl_Graphics_Driver::id(rgb) = cache_rgb(rgb);
    *cache_scale(rgb) = 1;
  }
  cache_size(img, WP, HP);
  return scale_and_render_pixmap( *Fl_Graphics_Driver::id(rgb), rgb->d(),
                                 rgb->pixel_w() / double(WP), rgb->pixel_h() / double(HP), 0, 0, (XP + offset_x_)*scale_, (YP + offset_y_)*scale_, WP, HP);
}
#endif // HAVE_XRENDER


//
// End of "$Id$".
//

/*
** mrb_cairo.c - Cairo class
**
** Copyright (c) Hiroki Mori 2017
**
** See Copyright Notice in LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <sys/consio.h>
#include <sys/fbio.h>

#include <sys/types.h>
#include <unistd.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mrb_cairo.h"

#include <cairo.h>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  int w;
  int h;
  int fbsize;
  unsigned char *fbuffer;
  int fb_fd;
  cairo_surface_t *cs;
  cairo_t *c;
} mrb_cairo_data;

static const struct mrb_data_type mrb_cairo_data_type = {
  "mrb_cairo_data", mrb_free,
};

void close_framebuffer(void *device)
{
  mrb_cairo_data *data = (mrb_cairo_data *)device;

  munmap(data->fbuffer, data->fbsize);
  close(data->fb_fd);
}

static mrb_value mrb_cairo_init(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data;
  mrb_int w, h;
  struct fbtype fb;
  int line_length;
  int pagemask;

  data = (mrb_cairo_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_cairo_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "|ii", &w, &h);
  data = (mrb_cairo_data *)mrb_malloc(mrb, sizeof(mrb_cairo_data));
  DATA_PTR(self) = data;
  if (mrb_get_argc(mrb) == 0) {
    data->fb_fd = open("/dev/fb0", O_RDWR);
    ioctl(data->fb_fd, FBIOGTYPE, &fb);
    ioctl(data->fb_fd, FBIO_GETLINEWIDTH, &line_length);
    data->w = fb.fb_width;
    data->h = fb.fb_height;
    pagemask = getpagesize() - 1;
    data->fbsize = ((int) line_length*data->h + pagemask) & ~pagemask;
    data->fbuffer = (unsigned char *)mmap(0, data-> fbsize,
      PROT_READ | PROT_WRITE, MAP_SHARED, data->fb_fd, 0);
    data->cs = cairo_image_surface_create_for_data (data->fbuffer,
       CAIRO_FORMAT_ARGB32, data->w, data->h, line_length);
    cairo_surface_set_user_data(data->cs, NULL, data, &close_framebuffer);
  } else {
    data->w = w;
    data->h = h;

    data->cs = cairo_image_surface_create(CAIRO_FORMAT_RGB24, data->w, data->h);
  }
  data->c = cairo_create(data->cs);

  return self;
}

static mrb_value mrb_cairo_set_source_rgb(mrb_state *mrb, mrb_value self)
{
  mrb_float r, g, b;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "fff", &r, &g, &b);
  cairo_set_source_rgba(data->c, r, g, b, 1);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_move_to(mrb_state *mrb, mrb_value self)
{
  mrb_float x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &x , &y);
  cairo_move_to(data->c, x, y);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_line_to(mrb_state *mrb, mrb_value self)
{
  mrb_float x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &x , &y);
  cairo_line_to(data->c, x, y);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_set_line_width(mrb_state *mrb, mrb_value self)
{
  mrb_float width;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "f", &width);
  cairo_set_line_width(data->c, width);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_rectangle(mrb_state *mrb, mrb_value self)
{
  mrb_float x, y, width, height;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ffff", &x, &y, &width, &height);
  cairo_rectangle(data->c, x, y, width, height);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_arc(mrb_state *mrb, mrb_value self)
{
  mrb_float xc, yc, radius, angle1, angle2;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "fffff", &xc, &yc, &radius, &angle1, &angle2);
  cairo_arc(data->c, xc, yc, radius, angle1, angle2);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_fill(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  cairo_fill(data->c);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_fill_preserve(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  cairo_fill_preserve(data->c);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_set_font_size(mrb_state *mrb, mrb_value self)
{
  mrb_float size;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "f", &size);
  cairo_set_font_size(data->c, size);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_show_text(mrb_state *mrb, mrb_value self)
{
  char *utf8;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "z", &utf8);
  cairo_show_text(data->c, utf8);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_text_extents(mrb_state *mrb, mrb_value self)
{
  char *utf8;
  mrb_cairo_data *data = DATA_PTR(self);
  cairo_text_extents_t exte;
  mrb_value res;

  mrb_get_args(mrb, "z", &utf8);
  cairo_text_extents(data->c, utf8, &exte);
  res = mrb_ary_new(mrb);
  mrb_ary_push(mrb, res, mrb_float_value(mrb, exte.width));
  mrb_ary_push(mrb, res, mrb_float_value(mrb, exte.height));

  return res;
}

static mrb_value mrb_cairo_ft_font_face_create(mrb_state *mrb, mrb_value self)
{
  char *filename;
  mrb_cairo_data *data = DATA_PTR(self);
  int face_count;
  FcPattern *pattern;
  cairo_font_face_t *font_face;

  mrb_get_args(mrb, "z", &filename);

  pattern = FcFreeTypeQuery ((unsigned char *)filename, 0, NULL, &face_count);
  if (! pattern) {
    return mrb_fixnum_value(0);
  }

  font_face = cairo_ft_font_face_create_for_pattern (pattern);
  FcPatternDestroy (pattern);
  cairo_set_font_face (data->c, font_face);
  cairo_font_face_destroy (font_face);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_print_png(mrb_state *mrb, mrb_value self)
{
  char *filename;
  mrb_int x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "iiz", &x, &y, &filename);
  cairo_surface_t *png = cairo_image_surface_create_from_png (filename);
  cairo_set_source_surface(data->c, png, x, y);
  cairo_paint(data->c);
  cairo_surface_flush(data->cs);
  cairo_surface_destroy(png);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_save_png(mrb_state *mrb, mrb_value self)
{
  char *filename;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "z", &filename);
  cairo_surface_write_to_png(data->cs, filename);

  return mrb_fixnum_value(0);
}

static cairo_status_t
write_png_stream_to_fd (void *in_closure, const unsigned char *data,
                                                unsigned int length)
{
  int fd = *((int *)in_closure);

  write(fd, data, length);

  return CAIRO_STATUS_SUCCESS;
}


static mrb_value mrb_cairo_write_png(mrb_state *mrb, mrb_value self)
{
  mrb_int fd;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "i", &fd);

  cairo_surface_write_to_png_stream (data->cs, write_png_stream_to_fd, &fd);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_translate(mrb_state *mrb, mrb_value self)
{
  mrb_float tx, ty;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &tx, &ty);
  cairo_translate(data->c, tx, ty);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_scale(mrb_state *mrb, mrb_value self)
{
  mrb_float sx, sy;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &sx, &sy);
  cairo_scale(data->c, sx, sy);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_rotate(mrb_state *mrb, mrb_value self)
{
  mrb_float angle;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "f", &angle);
  cairo_rotate(data->c, angle);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_paint(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  cairo_paint(data->c);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_stroke(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  cairo_stroke(data->c);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_restore(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  cairo_restore(data->c);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_getpix(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);
  mrb_value res;
  mrb_int x, y, c;
  int i;

  mrb_get_args(mrb, "iii", &x, &y, &c);

  cairo_surface_flush(data->cs);
  unsigned char *ptr = cairo_image_surface_get_data(data->cs);
  ptr += y * data->w * 4 + x * 4;
  res = mrb_ary_new(mrb);
  for (i = 0; i < 4*c; ++i)
    mrb_ary_push(mrb, res, mrb_fixnum_value(*ptr++));

  return res;
}

static mrb_value mrb_cairo_get_data(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);

  unsigned char *ptr = cairo_image_surface_get_data(data->cs);

  return mrb_fixnum_value((int)ptr);
}

static mrb_value mrb_cairo_stroke_extents(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);
  double x1, y1, x2, y2;
  mrb_value res;

  cairo_stroke_extents(data->c, &x1, &y1, &x2, &y2);
  res = mrb_ary_new(mrb);
  mrb_ary_push(mrb, res, mrb_float_value(mrb, x1));
  mrb_ary_push(mrb, res, mrb_float_value(mrb, y1));
  mrb_ary_push(mrb, res, mrb_float_value(mrb, x2));
  mrb_ary_push(mrb, res, mrb_float_value(mrb, y2));

  return res;
}

void mrb_mruby_cairo_gem_init(mrb_state *mrb)
{
  struct RClass *cairo;
  cairo = mrb_define_class(mrb, "Cairo", mrb->object_class);
  MRB_SET_INSTANCE_TT(cairo, MRB_TT_DATA);
  mrb_define_method(mrb, cairo, "initialize", mrb_cairo_init, MRB_ARGS_ARG(0,2));
  mrb_define_method(mrb, cairo, "set_source_rgb", mrb_cairo_set_source_rgb, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, cairo, "move_to", mrb_cairo_move_to, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "line_to", mrb_cairo_line_to, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "set_line_width", mrb_cairo_set_line_width, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "rectangle", mrb_cairo_rectangle, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, cairo, "arc", mrb_cairo_arc, MRB_ARGS_REQ(5));
  mrb_define_method(mrb, cairo, "fill", mrb_cairo_fill, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "fill_preserve", mrb_cairo_fill_preserve, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "set_font_size", mrb_cairo_set_font_size, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "show_text", mrb_cairo_show_text, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "text_extents", mrb_cairo_text_extents, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "font_create", mrb_cairo_ft_font_face_create, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "print_png", mrb_cairo_print_png, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, cairo, "save_png", mrb_cairo_save_png, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "write_png", mrb_cairo_write_png, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "translate", mrb_cairo_translate, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "scale", mrb_cairo_scale, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "rotate", mrb_cairo_rotate, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "paint", mrb_cairo_paint, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "stroke", mrb_cairo_stroke, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "restore", mrb_cairo_restore, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "getpix", mrb_cairo_getpix, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, cairo, "get_data", mrb_cairo_get_data, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "stroke_extents", mrb_cairo_stroke_extents, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_cairo_gem_final(mrb_state *mrb)
{
}


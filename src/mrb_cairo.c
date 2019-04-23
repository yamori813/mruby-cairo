/*
** mrb_cairo.c - Cairo class
**
** Copyright (c) Hiroki Mori 2017
**
** See Copyright Notice in LICENSE
*/

#include <sys/types.h>
#include <unistd.h>

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mrb_cairo.h"

#include <cairo.h>
#include <cairo-ft.h>
#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#define DONE mrb_gc_arena_restore(mrb, 0);

typedef struct {
  int w;
  int h;
  cairo_surface_t *cs;
  cairo_t *c;
} mrb_cairo_data;

static const struct mrb_data_type mrb_cairo_data_type = {
  "mrb_cairo_data", mrb_free,
};

static mrb_value mrb_cairo_init(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data;
  mrb_int w, h;

  data = (mrb_cairo_data *)DATA_PTR(self);
  if (data) {
    mrb_free(mrb, data);
  }
  DATA_TYPE(self) = &mrb_cairo_data_type;
  DATA_PTR(self) = NULL;

  mrb_get_args(mrb, "ii", &w, &h);
  data = (mrb_cairo_data *)mrb_malloc(mrb, sizeof(mrb_cairo_data));
  data->w = w;
  data->h = h;
  DATA_PTR(self) = data;

  data->cs = cairo_image_surface_create (CAIRO_FORMAT_RGB24, data->w, data->h);
  data->c = cairo_create(data->cs);

  return self;
}

static mrb_value mrb_cairo_set_source_rgb(mrb_state *mrb, mrb_value self)
{
  double r, g, b;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "fff", &r, &g, &b);
  cairo_set_source_rgba(data->c, r, g, b, 1);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_move_to(mrb_state *mrb, mrb_value self)
{
  double x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &x , &y);
  cairo_move_to(data->c, x, y);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_line_to(mrb_state *mrb, mrb_value self)
{
  double x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &x , &y);
  cairo_line_to(data->c, x, y);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_set_line_width(mrb_state *mrb, mrb_value self)
{
  double width;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "f", &width);
  cairo_set_line_width(data->c, width);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_rectangle(mrb_state *mrb, mrb_value self)
{
  double x, y, width, height;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ffff", &x, &y, &width, &height);
  cairo_rectangle(data->c, x, y, width, height);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_arc(mrb_state *mrb, mrb_value self)
{
  double xc, yc, radius, angle1, angle2;
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
  double size;
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
  double tx, ty;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &tx, &ty);
  cairo_translate(data->c, tx, ty);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_scale(mrb_state *mrb, mrb_value self)
{
  double sx, sy;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "ff", &sx, &sy);
  cairo_scale(data->c, sx, sy);

  return mrb_fixnum_value(0);
}

static mrb_value mrb_cairo_rotate(mrb_state *mrb, mrb_value self)
{
  double angle;
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
  mrb_define_method(mrb, cairo, "initialize", mrb_cairo_init, MRB_ARGS_REQ(2));
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
  mrb_define_method(mrb, cairo, "stroke_extents", mrb_cairo_stroke_extents, MRB_ARGS_NONE());
  DONE;
}

void mrb_mruby_cairo_gem_final(mrb_state *mrb)
{
}


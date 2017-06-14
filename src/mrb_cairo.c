/*
** mrb_cairo.c - Cairo class
**
** Copyright (c) Hiroki Mori 2017
**
** See Copyright Notice in LICENSE
*/

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/array.h"
#include "mrb_cairo.h"

#include <cairo.h>

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
  int w, h;

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

static mrb_value mrb_cairo_print_png(mrb_state *mrb, mrb_value self)
{
  char *filename;
  int x, y;
  mrb_cairo_data *data = DATA_PTR(self);

  mrb_get_args(mrb, "iiz", &x, &y, &filename);
  cairo_surface_t *png = cairo_image_surface_create_from_png (filename);
  cairo_set_source_surface(data->c, png, x, y);
  cairo_paint(data->c);
  cairo_surface_flush(data->cs);
  cairo_surface_destroy(png);

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

static mrb_value mrb_cairo_getpix(mrb_state *mrb, mrb_value self)
{
  mrb_cairo_data *data = DATA_PTR(self);
  mrb_value res;
  int x, y, c;
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

void mrb_mruby_cairo_gem_init(mrb_state *mrb)
{
  struct RClass *cairo;
  cairo = mrb_define_class(mrb, "Cairo", mrb->object_class);
  mrb_define_method(mrb, cairo, "initialize", mrb_cairo_init, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "set_source_rgb", mrb_cairo_set_source_rgb, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, cairo, "move_to", mrb_cairo_move_to, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "line_to", mrb_cairo_line_to, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, cairo, "rectangle", mrb_cairo_rectangle, MRB_ARGS_REQ(4));
  mrb_define_method(mrb, cairo, "arc", mrb_cairo_arc, MRB_ARGS_REQ(5));
  mrb_define_method(mrb, cairo, "set_font_size", mrb_cairo_set_font_size, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "show_text", mrb_cairo_show_text, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cairo, "print_png", mrb_cairo_print_png, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, cairo, "paint", mrb_cairo_paint, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "stroke", mrb_cairo_stroke, MRB_ARGS_NONE());
  mrb_define_method(mrb, cairo, "getpix", mrb_cairo_getpix, MRB_ARGS_REQ(3));
  DONE;
}

void mrb_mruby_cairo_gem_final(mrb_state *mrb)
{
}


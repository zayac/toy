#include "display.h"

static int frame_top = 0, frame_left = 0;
static int frame_height = ROW_NUMBER, frame_width = COL_NUMBER;
static int frame_fcolor = COLOR_WHITE, frame_bcolor = COLOR_BLACK;
static int caret_row = 0, caret_col = 0;
static bool cursor = false;

int get_frame_top () {
  return frame_top;
}

int get_frame_left () {
  return frame_left;
}

int get_frame_height () {
  return frame_height;
}

int get_frame_width () {
  return frame_width;
}

int get_frame_fcolor () {
  return frame_fcolor;
}

int get_frame_bcolor () {
  return frame_bcolor;
}

void set_frame (int top, int left, int height, int width, int fcolor,
                int bcolor) {
  if (top < 0 || top >= ROW_NUMBER || left < 0 || left >= COL_NUMBER ||
      height < 1 || height > ROW_NUMBER - top || width < 1 ||
      width > COL_NUMBER - top || fcolor < COLOR_BLACK || bcolor > COLOR_WHITE)
    return;
  frame_top = top, frame_left = left, frame_height = height,
    frame_width = width, frame_fcolor = fcolor, frame_bcolor = bcolor;
}

void clear_frame () {
  for (int row = 0; row < frame_height; row++)
    for (int col = 0; col < frame_width; col++)
      *get_chr_cell(frame_top + row, frame_left + col) =
        (struct chr_cell) { 0, frame_fcolor, frame_bcolor };
}

bool get_cursor () {
  return cursor;
}

static void put_cursor (int row, int col) {
  int off = row * COL_NUMBER + col;
  outb(0x3D4, 0x0F);
  outb(0x3D5, (unsigned char)(off & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (unsigned char)((off >> 8) & 0xFF));
}

void set_cursor (bool visible) {
  cursor = visible;
  if (visible)
    put_cursor(frame_top + caret_row, frame_left + caret_col);
  else
    put_cursor(ROW_NUMBER, 0);
}

int get_caret_row () {
  return caret_row;
}

int get_caret_col () {
  return caret_col;
}

void set_caret (int row, int col) {
  if (row < 0 || row >= frame_height || col < 0 || col >= frame_width)
    return;
  caret_row = row, caret_col = col;
  if (cursor)
    put_cursor(frame_top + row, frame_left + col);
}

static void scroll_frame () {
  for (int row = 1; row < frame_height; row++)
    for (int col = 0; col < frame_width; col++)
      *get_chr_cell(frame_top + row - 1, frame_left + col) =
        *get_chr_cell(frame_top + row, frame_left + col);

  for (int col = 0; col < frame_width; col++)
    *get_chr_cell(frame_top + frame_width - 1, frame_left + col) =
      (struct chr_cell) { 0, frame_fcolor, frame_bcolor };
}

static void put_char (char chr) {
  switch (chr) {
  case '\r': 
    caret_col = 0;
    break;
  case '\n':
  new_line:
    caret_col = 0, caret_row++;
    if (caret_row == frame_height) {
      scroll_frame();
      caret_row--;
    }
    break;
    // TODO: implement other escapes
  default:
    *get_chr_cell(frame_top + caret_row, frame_left + caret_col) =
      (struct chr_cell) { chr, frame_fcolor, frame_bcolor };
    if (++caret_col == frame_width) {
      caret_col = 0;
      goto new_line;
    }
    break;
  }
}

int putchar (int chr) {
  put_char(chr);
  if (cursor)
    put_cursor(frame_top + caret_row, frame_left + caret_col);
  return chr;
}

int printf (char *format, ...) {

}

/*
 * $Id$
 *
 */

#ifndef DESPOTIFY_UI_LOG_H
#define DESPOTIFY_UI_LOG_H

#include "ui.h"

void log_draw(ui_t *ui);
int  log_keypress(int ch);

void log_append(const char *fmt, ...);

#endif

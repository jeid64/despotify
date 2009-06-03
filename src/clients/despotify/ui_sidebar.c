/*
 * $Id$
 *
 */

#include "event.h"
#include "session.h"
#include "ui_sidebar.h"
#include "ui_tracklist.h"

extern session_t g_session;

// Cursor position.
static unsigned int g_pos = ~0;

void sidebar_init(ui_t *ui)
{
  ui->win          = newwin(0, 0, 0, 0);
  ui->flags        = 0;
  ui->set          = UI_SET_BROWSER;
  ui->fixed_width  = 25;
  ui->fixed_height = 0;
  ui->draw_cb      = sidebar_draw;
  ui->keypress_cb  = sidebar_keypress;
}

// Print list of search results.
void sidebar_draw(ui_t *ui)
{
  mvwprintw(ui->win, 0, 0, "Searches");
  mvwchgat(ui->win, 0, 0, -1, A_BOLD, UI_STYLE_DIM, NULL);

  unsigned int line = 1;

  // TODO: Scrolling.
  for (sess_search_t *s = g_session.search; s && line < ui->height; s = s->next) {
    mvwprintw(ui->win, line, 0, "%.24s", s->res->query);

    if (line - 1 == g_pos)
      mvwchgat(ui->win, line, 0, -1, (ui->flags & UI_FLAG_FOCUS ? A_REVERSE : A_BOLD), UI_STYLE_NORMAL, NULL);
    ++line;
  }
}

int sidebar_keypress(wint_t ch, bool code)
{
  (void)code;

  if (!g_session.search_len)
    return ch;

  switch (ch) {
    case KEY_ENTER:
    case '\n':
    case '\r':
    case KEY_RIGHT:
    case 'l':
      tracklist_set(g_pos, true);
      break;

    case KEY_UP:
    case 'k':
      g_pos = DSFY_MAX((signed)g_pos - 1, 0);
      tracklist_set(g_pos, false);
      break;

    case KEY_DOWN:
    case 'j':
      g_pos = DSFY_MIN(g_pos + 1, g_session.search_len - 1);
      tracklist_set(g_pos, false);
      break;

    case KEY_HOME:
      g_pos = 0;
      tracklist_set(g_pos, false);
      break;

    case KEY_END:
      g_pos = g_session.search_len - 1;
      tracklist_set(g_pos, false);
      break;

    default:
      return ch;
  }

  ui_dirty(UI_SIDEBAR);
  event_msg_post(MSG_CLASS_APP, MSG_APP_UPDATE, NULL);

  return 0;
}

// Set cursor to first position if any search results are available.
void sidebar_reset()
{
  g_pos = g_session.search_len ? 0 : ~0;

  ui_dirty(UI_SIDEBAR);
  tracklist_set(0, true);
}

/*

  Copyright (c) 2006-2008 uim Project http://code.google.com/p/uim/

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. Neither the name of authors nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <limits.h>

#include <chewing.h>

#include <uim-scm.h>
#include <plugin.h>

#include "keytab.h"


static int nr_chewing_context;

#define COMMIT_CMD	"chewing-commit"
#define PUSHBACK_CMD	"chewing-pushback-preedit"
#define CLEAR_CMD	"chewing-clear-preedit"
#define ACTIVATE_CMD	"chewing-activate-candidate-selector"
#define DEACTIVATE_CMD	"chewing-deactivate-candidate-selector"
#define SHIFT_CMD	"chewing-shift-page-candidate"
#define PREEDIT_ATTR_MAX_LEN	3
#define MAX_LENGTH_OF_INT_AS_STR	(((sizeof(int) == 4) ? sizeof("-2147483648") : sizeof("-9223372036854775808")) - sizeof((char)'\0'))


typedef struct chewing_context {
  ChewingContext *cc;
  int slot_id;
  int prev_page;
  int prev_cursor;
  int has_active_candwin;
} uim_chewing_context;

static struct context {
  uim_chewing_context *ucc;
} *context_slot;

/*
 * correspond to
 *  chewing-cand-selection-numkey   : 0
 *  chewing-cand-selection-asdfghjkl: 1
 * see chewing-custom.scm
 */

static int selkey_num[11] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0};
static int selkey_asdf[11] = {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 0};
static int *uim_chewing_selkeys[2] = { selkey_num, selkey_asdf };

static uim_chewing_context *
get_chewing_context(int id)
{
  if (id < 0 || id >= nr_chewing_context)
    return NULL;

  return context_slot[id].ucc;
}

static uim_lisp
init_chewing_lib(void)
{
  int len;
  char *home, *hashpath;

  if (context_slot)
    return uim_scm_t();

  home = getenv("HOME");
  if (!home)
    home = "";

  len = strlen(home) + strlen("/.chewing/");
  hashpath = malloc(len + 1);
  snprintf(hashpath, len + 1, "%s/.chewing", home);

  if (chewing_Init(CHEWING_DATADIR, hashpath) != 0) {
    free(hashpath);
    return uim_scm_f();
  }
  free(hashpath);

  nr_chewing_context = 1;
  context_slot = malloc(sizeof(struct context));
  if (!context_slot) {
    nr_chewing_context = 0;
    return uim_scm_f();
  }
  context_slot[0].ucc = NULL;

  return uim_scm_t();
}

static void
configure_kbd_type(uim_chewing_context *ucc)
{
  uim_lisp kbd_layout;

  kbd_layout = uim_scm_eval_c_string("(chewing-get-kbd-layout)");
  chewing_set_KBType(ucc->cc, uim_scm_c_int(kbd_layout));
}

static void
configure(uim_chewing_context *ucc)
{
  int i, style;
  int selkey[10];
  uim_lisp phrase_forward, esc_clean, space_as_selection, sel_style,
	   phrase_choice_rearward, auto_shift_cursor;

  chewing_set_candPerPage(ucc->cc, 10);
  chewing_set_maxChiSymbolLen(ucc->cc, 16);

  phrase_forward = uim_scm_eval_c_string("chewing-phrase-forward?");
  chewing_set_addPhraseDirection(ucc->cc, !uim_scm_c_bool(phrase_forward));

  phrase_choice_rearward = uim_scm_eval_c_string("chewing-phrase-choice-rearward?");
  chewing_set_phraseChoiceRearward(ucc->cc, uim_scm_c_bool(phrase_choice_rearward));

  auto_shift_cursor = uim_scm_eval_c_string("chewing-auto-shift-cursor?");
  chewing_set_autoShiftCur(ucc->cc, uim_scm_c_bool(auto_shift_cursor));

  space_as_selection = uim_scm_eval_c_string("chewing-space-as-selection?");
  chewing_set_spaceAsSelection(ucc->cc, uim_scm_c_bool(space_as_selection));

  esc_clean = uim_scm_eval_c_string("chewing-esc-clean?");
  chewing_set_escCleanAllBuf(ucc->cc, uim_scm_c_bool(esc_clean));

  sel_style =
    uim_scm_eval_c_string("(symbol-value chewing-candidate-selection-style)");
  style = uim_scm_c_int(sel_style);
  chewing_set_selKey(ucc->cc, uim_chewing_selkeys[style], 10);

  configure_kbd_type(ucc);
}

static uim_chewing_context *
chewing_context_new()
{
  uim_chewing_context *ucc;

  ucc = malloc(sizeof(uim_chewing_context));
  if (ucc) {
    ucc->cc = chewing_new();
    ucc->prev_page = -1;
    ucc->prev_cursor = -1;
    ucc->has_active_candwin = 0;
  }

  return ucc;
}

static void
activate_candwin(uim_chewing_context *ucc)
{
  char *buf;
  int len;

  len = strlen(ACTIVATE_CMD "(   )") + MAX_LENGTH_OF_INT_AS_STR * 3;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" ACTIVATE_CMD " %d %d %d)", ucc->slot_id,
	   chewing_cand_TotalChoice(ucc->cc),
	   chewing_cand_ChoicePerPage(ucc->cc));
  uim_scm_eval_c_string(buf);
  free(buf);
}

static void
deactivate_candwin(uim_chewing_context *ucc)
{
  char *buf;
  int len;

  len = strlen(DEACTIVATE_CMD "( )") + MAX_LENGTH_OF_INT_AS_STR;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" DEACTIVATE_CMD " %d)", ucc->slot_id);
  uim_scm_eval_c_string(buf);
  free(buf);
}

static void
shift_candwin(uim_chewing_context *ucc, int dir)
{
  char *buf;
  int len;

  len = strlen(SHIFT_CMD "(  )") + MAX_LENGTH_OF_INT_AS_STR + 3;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" SHIFT_CMD " %d %s)", ucc->slot_id,
	   dir ? "#t" : "#f");
  uim_scm_eval_c_string(buf);
  free(buf);
}

static void
pushback_preedit_string(uim_chewing_context *ucc, const char *str, int attr)
{
  char *buf;
  int len;

  len = strlen(PUSHBACK_CMD "   \"\")") + strlen(str) +
	       MAX_LENGTH_OF_INT_AS_STR + PREEDIT_ATTR_MAX_LEN;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" PUSHBACK_CMD " %d %d \"%s\")", ucc->slot_id,
	   attr, str);
  uim_scm_eval_c_string(buf);
  free(buf);
}

static void
clear_preedit(uim_chewing_context *ucc)
{
  char *buf;
  int len;

  len = strlen(CLEAR_CMD "( )") + MAX_LENGTH_OF_INT_AS_STR;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" CLEAR_CMD " %d)", ucc->slot_id);
  uim_scm_eval_c_string(buf);
  free(buf);
}

static void
commit_string(uim_chewing_context *ucc, const char *str)
{
  char *buf;
  int len;

  len = strlen(COMMIT_CMD "(  \"\")") + strlen(str) + MAX_LENGTH_OF_INT_AS_STR;
  buf = malloc(len + 1);
  snprintf(buf, len + 1, "(" COMMIT_CMD " %d \"%s\")", ucc->slot_id, str);
  uim_scm_eval_c_string(buf);
  free(buf);
}


static uim_lisp
check_output(uim_chewing_context *ucc)
{
  int i, j, len, preedit_cleared = 0, preedit_len = 0, buf_len = 0;
  int cursor_pos, zuin_len, n_page, page_no;
  char *buf, *locale;
  wchar_t *wcs;
  size_t wclen = 0;
  ChewingContext *cc = ucc->cc;

  buf = strdup("");

  if (chewing_commit_Check(cc)) {
    char *str = chewing_commit_String(cc);
    if (str) {
      clear_preedit(ucc);
      preedit_cleared = 1;
      commit_string(ucc, str);
      chewing_free(str);
    }
  }

  cursor_pos = chewing_cursor_Current(cc);

  /* preedit before cursor */
  if (chewing_buffer_Check(cc)) {
    char *str;

    str = chewing_buffer_String(cc);
    locale = strdup(setlocale(LC_CTYPE, NULL));
    wcs = malloc(sizeof(wchar_t) * (chewing_buffer_Len(cc) + 1));
    setlocale(LC_CTYPE, "en_US.UTF-8");
    wclen = mbstowcs(wcs, str, chewing_buffer_Len(cc));
    if (wclen != -1) {
      wcs[chewing_buffer_Len(cc)] = L'\0';

      buf_len = 0;
      for (i = 0; i < cursor_pos; i++) {
        char mb[MB_CUR_MAX];
        len = wctomb(mb, wcs[i]);
	mb[len] = '\0';
        buf_len += len;
        buf = realloc(buf, buf_len + 1);
        strncat(buf, mb, len);
      }
      if (i > 0) {
        if (!preedit_cleared) {
          clear_preedit(ucc);
          preedit_cleared = 1;
        }
        pushback_preedit_string(ucc, buf, UPreeditAttr_UnderLine);
        pushback_preedit_string(ucc, "", UPreeditAttr_Cursor);
        preedit_len += strlen(buf);
        buf[0] = '\0';
      }
    }

    chewing_free(str);

    setlocale(LC_CTYPE, locale);
    free(locale);
  }

  /* zuin */
  {
    char *str;
    str = chewing_zuin_String(cc, &zuin_len);
    if (str) {
      if (!preedit_cleared) {
	clear_preedit(ucc);
	preedit_cleared = 1;
      }
      pushback_preedit_string(ucc, str, UPreeditAttr_Reverse);
      preedit_len += zuin_len;
      chewing_free(str);
    }
  }

  /* XXX set dispInterval attributes */
#if 0
  for (i = 0; i < output->nDispInterval; i++) {
    if (output->dispInterval[i].to - output->dispInterval[i].from > 1) {
    }
  }
#endif

  /* preedit after the cursor */
  if (cursor_pos < chewing_buffer_Len(cc)) {
    locale = strdup(setlocale(LC_CTYPE, NULL));
    setlocale(LC_CTYPE, "en_US.UTF-8");

    buf_len = 0;
    for (i = cursor_pos; i < chewing_buffer_Len(cc); i++) {
      char mb[MB_CUR_MAX];
      len = wctomb(mb, wcs[i]);
      mb[len] = '\0';
      if (i == cursor_pos && zuin_len == 0) {
        if (!preedit_cleared) {
          clear_preedit(ucc);
          preedit_cleared = 1;
        }
        pushback_preedit_string(ucc, mb,
		       	UPreeditAttr_UnderLine | UPreeditAttr_Reverse);
        preedit_len += len;
      } else {
        buf_len += len;
	buf = realloc(buf, buf_len + 1);
	strncat(buf, mb, len);
      }
    }
    if (i > cursor_pos) {
      if (!preedit_cleared) {
        clear_preedit(ucc);
        preedit_cleared = 1;
      }
      pushback_preedit_string(ucc, buf, UPreeditAttr_UnderLine);
      preedit_len += strlen(buf);
      buf[0] = '\0';
    }

    setlocale(LC_CTYPE, locale);
    free(locale);
  }

  /* update candwin */
  n_page = chewing_cand_TotalPage(cc);
  page_no = chewing_cand_CurrentPage(cc);
  if (!chewing_cand_CheckDone(cc) && n_page != 0) {
    if (page_no == 0 && ucc->prev_page == -1 || ucc->prev_cursor != cursor_pos) {
      activate_candwin(ucc);
      ucc->has_active_candwin = 1;
    } else if ((page_no == ucc->prev_page + 1) ||
	       (page_no == 0 &&
		ucc->prev_page == n_page - 1)) {
      if (ucc->has_active_candwin) {
	shift_candwin(ucc, 1);
      }
    } else if ((page_no == ucc->prev_page - 1) ||
	       ((page_no == n_page - 1) &&
		ucc->prev_page == 0)) {
      if (ucc->has_active_candwin) {
	shift_candwin(ucc, 0);
      }
    }
    ucc->prev_page = page_no;
  } else {
    if (ucc->has_active_candwin)
      deactivate_candwin(ucc);
    ucc->prev_page = -1;
    ucc->has_active_candwin = 0;
  }
  ucc->prev_cursor = cursor_pos;

  /* msgs */
  buf_len = 0;
  if (chewing_aux_Check(cc)) {
    char *str = chewing_aux_String(cc);
    int len = chewing_aux_Length(cc);
    buf_len += len;

    buf_len += 2;
    buf = realloc(buf, buf_len + 1);
    strncat(buf, "; ", 2);
    strncat(buf, str, len);
    chewing_free(str);

    if (!preedit_cleared) {
      clear_preedit(ucc);
      preedit_cleared = 1;
    }
    pushback_preedit_string(ucc, buf, UPreeditAttr_None);
    preedit_len += strlen(buf);
    buf[0] = '\0';
  }
  free(buf);

  if (chewing_keystroke_CheckAbsorb(cc)) {
    if (preedit_len == 0 && !preedit_cleared)
      clear_preedit(ucc);
    return uim_scm_t();
  } else if (chewing_keystroke_CheckIgnore(cc)) {
    return uim_scm_f();
  }

  return uim_scm_t();
}

static uim_lisp
press_key_internal(uim_chewing_context *ucc, int ukey, int state,
		   uim_bool pressed)
{
  ChewingContext *cc = ucc->cc;

  if (!pressed)
    return uim_scm_t();

  if (state == 0) {
    switch (ukey) {
    case 32:
      chewing_handle_Space(cc);
      break;
    case UKey_Escape:
      chewing_handle_Esc(cc);
      break;
    case UKey_Return:
      chewing_handle_Enter(cc);
      break;
    case UKey_Delete:
      chewing_handle_Del(cc);
      break;
    case UKey_Backspace:
      chewing_handle_Backspace(cc);
      break;
    case UKey_Tab:
      chewing_handle_Tab(cc);
      break;
    case UKey_Left:
      chewing_handle_Left(cc);
      break;
    case UKey_Right:
      chewing_handle_Right(cc);
      break;
    case UKey_Up:
      chewing_handle_Up(cc);
      break;
    case UKey_Home:
      chewing_handle_Home(cc);
      break;
    case UKey_End:
      chewing_handle_End(cc);
      break;
    case UKey_Down:
      chewing_handle_Down(cc);
      break;
#if UIM_VERSION_REQUIRE(1, 3, 0)
    case UKey_Caps_Lock:
      chewing_handle_Capslock(cc);
      break;
#endif
    case UKey_Prior:
      chewing_handle_PageUp(cc);
      break;
    case UKey_Next:
      chewing_handle_PageDown(cc);
      break;
    default:
      if (ukey > 32 && ukey < 127) {
	chewing_handle_Default(cc, ukey);
      } else
	return uim_scm_f();
      break;
    }
  } else if (state & UMod_Shift) {
    switch (ukey) {
    case UKey_Left:
      chewing_handle_ShiftLeft(cc);
      break;
    case UKey_Right:
      chewing_handle_ShiftRight(cc);
      break;
    case 32:
      chewing_handle_ShiftSpace(cc);
      chewing_set_ShapeMode(cc, !chewing_get_ShapeMode(cc));
      break;
    default:
      if (ukey > 32 && ukey < 127)
	chewing_handle_Default(cc, ukey);
      else
	return uim_scm_f();
      break;
    }
  } else if (state & UMod_Control) {
    if (ukey >= UKey_0 && ukey <= UKey_9)
      chewing_handle_CtrlNum(cc, ukey);
    else
      return uim_scm_f();
  } else {
      return uim_scm_f();
  }

  return check_output(ucc);
}

static void
chewing_context_release(uim_chewing_context *ucc)
{
  chewing_delete(ucc->cc);
  free(ucc);
}


static uim_lisp
create_context(void)
{
  int i;
  uim_chewing_context *ucc;

  if (!context_slot)
    return uim_scm_f();

  for (i = 0; i < nr_chewing_context; i++) {
    if (!context_slot[i].ucc) {
      ucc = chewing_context_new();
      if (!ucc)
	return uim_scm_f();

      context_slot[i].ucc = ucc;
      ucc->slot_id = i;
      configure(ucc);
      return uim_scm_make_int(i);
    }
  }

  context_slot = realloc(context_slot, sizeof(struct context) *
					      nr_chewing_context + 1);
  if (!context_slot)
    return uim_scm_f();

  nr_chewing_context++;
  ucc = chewing_context_new();
  if (!ucc)
    return uim_scm_f();

  context_slot[i].ucc = ucc;
  ucc->slot_id = i;
  configure(ucc);

  return uim_scm_make_int(i);
}

static uim_lisp
release_context(uim_lisp id_)
{
  int id = uim_scm_c_int(id_);

  if (context_slot[id].ucc) {
    chewing_context_release(context_slot[id].ucc);
    context_slot[id].ucc = NULL;
  }

  return uim_scm_f();
}

static uim_lisp
reset_context(uim_lisp id_)
{
  int id = uim_scm_c_int(id_);

  if (context_slot[id].ucc) {
    chewing_Reset(context_slot[id].ucc->cc);
    configure_kbd_type(context_slot[id].ucc);
  }

  return uim_scm_f();
}

static uim_lisp
focus_in_context(uim_lisp id_)
{
  int id = uim_scm_c_int(id_);

  if (context_slot[id].ucc) {
    clear_preedit(context_slot[id].ucc);
    check_output(context_slot[id].ucc);
  }

  return uim_scm_f();
}

static uim_lisp
focus_out_context(uim_lisp id_)
{
  int id;
  
  id = uim_scm_c_int(id_);
  if (context_slot[id].ucc) {
    clear_preedit(context_slot[id].ucc);
    chewing_handle_Esc(context_slot[id].ucc->cc);
  }

  return uim_scm_f();
}

static uim_lisp
get_nr_candidates(uim_lisp id_)
{
  int id, nth;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  nth = uim_scm_c_int(id_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  return uim_scm_make_int(chewing_cand_TotalChoice(ucc->cc));
}

static uim_lisp
get_nth_candidate(uim_lisp id_, uim_lisp nth_)
{
  int id, nth, n;
  uim_chewing_context *ucc;
  char *cand;
  uim_lisp str_;

  id = uim_scm_c_int(id_);
  nth = uim_scm_c_int(nth_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  if (nth == 0)
    chewing_cand_Enumerate(ucc->cc);

  cand = chewing_cand_String(ucc->cc);
  str_ = uim_scm_make_str(cand);
  free(cand);

  return str_;
}

static uim_lisp
get_nr_candidates_per_page(uim_lisp id_)
{
  int id;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  return uim_scm_make_int(chewing_cand_ChoicePerPage(ucc->cc));
}


static uim_lisp
set_chi_eng_mode(uim_lisp id_, uim_lisp mode_)
{
  int id, mode;
  uim_chewing_context *ucc;
  
  id = uim_scm_c_int(id_);
  mode = uim_scm_c_int(mode_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  chewing_set_ChiEngMode(ucc->cc, mode);

  return uim_scm_t();
}

static uim_lisp
get_chi_eng_mode(uim_lisp id_)
{
  int id, mode;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  mode = chewing_get_ChiEngMode(ucc->cc);

  return uim_scm_make_int(mode);
}

static uim_lisp
set_shape_mode(uim_lisp id_, uim_lisp mode_)
{
  int id, mode;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  mode = uim_scm_c_int(mode_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  chewing_set_ShapeMode(ucc->cc, mode);

  return uim_scm_t();
}

static uim_lisp
get_shape_mode(uim_lisp id_)
{
  int id, mode;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  mode = chewing_get_ShapeMode(ucc->cc);

  return uim_scm_make_int(mode);
}

static uim_lisp
keysym_to_ukey(uim_lisp sym_)
{
  const char *sym;
  int key, i;

  sym = uim_scm_refer_c_str(sym_);
  key = 0;

  for (i = 0; key_tab[i].key; i++) {
    if (!strcmp(key_tab[i].str, sym)) {
      key = key_tab[i].key;
      break;
    }
  }

  return uim_scm_make_int(key);
}


static uim_lisp
press_key(uim_lisp id_, uim_lisp key_, uim_lisp state_, uim_lisp pressed_)
{
  int id, ukey, state;
  uim_bool pressed;
  uim_chewing_context *ucc;

  id = uim_scm_c_int(id_);
  ukey = uim_scm_c_int(key_);
  state = uim_scm_c_int(state_);
  pressed = uim_scm_c_bool(pressed_);

  ucc = get_chewing_context(id);

  if (!ucc)
    return uim_scm_f();

  return  press_key_internal(ucc, ukey, state, pressed);
}

static uim_lisp
reload_config()
{
  int i;

  for (i = 0; i < nr_chewing_context; i++) {
    if (context_slot[i].ucc)
      configure(context_slot[i].ucc);
  }

  return uim_scm_t();
}

void
uim_plugin_instance_init(void)
{
  uim_scm_init_subr_0("chewing-lib-init", init_chewing_lib);
  uim_scm_init_subr_0("chewing-lib-alloc-context", create_context);
  uim_scm_init_subr_1("chewing-lib-free-context", release_context);
  uim_scm_init_subr_1("chewing-lib-reset-context", reset_context);
  uim_scm_init_subr_1("chewing-lib-focus-in-context", focus_in_context);
  uim_scm_init_subr_1("chewing-lib-focus-out-context", focus_out_context);
  uim_scm_init_subr_1("chewing-lib-get-nr-candidates", get_nr_candidates);
  uim_scm_init_subr_2("chewing-lib-get-nth-candidate", get_nth_candidate);
  uim_scm_init_subr_1("chewing-lib-get-nr-candidates-per-page",
		      get_nr_candidates_per_page);

  uim_scm_init_subr_1("chewing-lib-keysym-to-ukey", keysym_to_ukey);
  uim_scm_init_subr_4("chewing-lib-press-key", press_key);

  uim_scm_init_subr_0("chewing-lib-reload-config", reload_config);

  uim_scm_init_subr_2("chewing-lib-set-chi-eng-mode", set_chi_eng_mode);
  uim_scm_init_subr_1("chewing-lib-get-chi-eng-mode", get_chi_eng_mode);
  uim_scm_init_subr_2("chewing-lib-set-shape-mode", set_shape_mode);
  uim_scm_init_subr_1("chewing-lib-get-shape-mode", get_shape_mode);
}

void
uim_plugin_instance_quit(void)
{
  int i;

  if (!context_slot)
    return;

  for (i = 0; i < nr_chewing_context; i++) {
    if (context_slot[i].ucc)
      chewing_context_release(context_slot[i].ucc);
  }

  chewing_Terminate();

  if (context_slot) {
    free(context_slot);
    context_slot = NULL;
  }
  nr_chewing_context = 0;
}

/*
 * mod_ui.c — Avisos de item recebido e código do seed na seleção de arquivo,
 * via RecompUI. Regras da API (RecompFrontend/recompui/src/core/ui_context.cpp):
 * toda criação/alteração de elemento ocorre entre recompui_open_context e
 * recompui_close_context; abrir duas vezes é erro fatal.
 */
#include "bkr_mod.h"
#include "recompui.h"
#include "bkr/bkr_text.h"

#define BKR_TOAST_FRAMES 180

static RecompuiContext g_ctx;
static RecompuiResource g_toast;
static RecompuiResource g_code;
static int g_ready;
static int g_toast_frames;
static int g_code_visible;
static char g_line[96];

static void bkr_ui_create(void) {
    static const RecompuiColor panel = { 0, 0, 0, 200 };
    static const RecompuiColor text = { 255, 230, 120, 255 };
    RecompuiResource root;
    g_ctx = recompui_create_context();
    recompui_set_context_captures_input(g_ctx, 0);
    recompui_set_context_captures_mouse(g_ctx, 0);
    recompui_open_context(g_ctx);
    root = recompui_context_root(g_ctx);
    recompui_set_position(root, POSITION_ABSOLUTE);
    recompui_set_top(root, 0, UNIT_DP);
    recompui_set_left(root, 0, UNIT_DP);
    recompui_set_right(root, 0, UNIT_DP);
    recompui_set_bottom(root, 0, UNIT_DP);
    recompui_set_display(root, DISPLAY_FLEX);
    recompui_set_flex_direction(root, FLEX_DIRECTION_COLUMN);
    recompui_set_justify_content(root, JUSTIFY_CONTENT_SPACE_BETWEEN);
    recompui_set_align_items(root, ALIGN_ITEMS_CENTER);
    recompui_set_padding(root, 32.0f, UNIT_DP);

    g_code = recompui_create_label(g_ctx, root, " ", LABELSTYLE_NORMAL);
    recompui_set_background_color(g_code, &panel);
    recompui_set_color(g_code, &text);
    recompui_set_padding(g_code, 12.0f, UNIT_DP);
    recompui_set_border_radius(g_code, 12.0f, UNIT_DP);
    recompui_set_display(g_code, DISPLAY_NONE);

    g_toast = recompui_create_label(g_ctx, root, " ", LABELSTYLE_LARGE);
    recompui_set_background_color(g_toast, &panel);
    recompui_set_color(g_toast, &text);
    recompui_set_padding(g_toast, 16.0f, UNIT_DP);
    recompui_set_border_radius(g_toast, 16.0f, UNIT_DP);
    recompui_set_display(g_toast, DISPLAY_NONE);
    recompui_close_context(g_ctx);
    recompui_show_context(g_ctx);
    g_ready = 1;
}

void bkr_ui_toast_text(const char* text) {
    if (!g_ready || !g_bkr.toasts) {
        return;
    }
    recompui_open_context(g_ctx);
    recompui_set_text(g_toast, text);
    recompui_set_display(g_toast, DISPLAY_BLOCK);
    recompui_close_context(g_ctx);
    g_toast_frames = BKR_TOAST_FRAMES;
}

void bkr_ui_toast_item(bkr_u8 item) {
    BkrText t;
    bkr_text_init(&t, g_line, sizeof(g_line));
    bkr_text_put(&t, g_bkr.lang == BKR_LANG_EN ? "You got: " : "Voce recebeu: ");
    bkr_text_put(&t, bkr_item_name(item, g_bkr.lang));
    bkr_ui_toast_text(g_line);
}

static void bkr_ui_update_code(int want_visible) {
    BkrText t;
    bkr_u32 i;
    if (want_visible == g_code_visible) {
        return;
    }
    recompui_open_context(g_ctx);
    if (want_visible) {
        bkr_text_init(&t, g_line, sizeof(g_line));
        if (g_bkr.active) {
            bkr_text_put(&t, g_bkr.share_code);
            bkr_text_put(&t, "  |  ");
            for (i = 0; i < 5; i++) {
                bkr_text_put(&t, bkr_hash_word(g_bkr.result.fingerprint >> (i * 5)));
                if (i < 4) {
                    bkr_text_putc(&t, ' ');
                }
            }
        } else {
            bkr_text_put(&t, g_bkr.lang == BKR_LANG_EN ? "Randomizer: invalid seed or code (see mod options)"
                                                        : "Randomizer: seed ou codigo invalido (veja as opcoes do mod)");
        }
        recompui_set_text(g_code, g_line);
        recompui_set_display(g_code, DISPLAY_BLOCK);
    } else {
        recompui_set_display(g_code, DISPLAY_NONE);
    }
    recompui_close_context(g_ctx);
    g_code_visible = want_visible;
}

void bkr_ui_tick(void) {
    if (!g_bkr.enabled) {
        return;
    }
    if (!g_ready) {
        bkr_ui_create();
    }
    bkr_ui_update_code(map_get() == MAP_91_FILE_SELECT);
    if (g_toast_frames > 0) {
        g_toast_frames--;
        if (g_toast_frames == 0) {
            recompui_open_context(g_ctx);
            recompui_set_display(g_toast, DISPLAY_NONE);
            recompui_close_context(g_ctx);
        }
    }
}

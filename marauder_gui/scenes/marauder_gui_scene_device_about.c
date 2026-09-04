#include "../marauder_gui_app_i.h"
#include <furi_hal_speaker.h>

#define REST 0
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 92
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880

/* Easter egg: Ok on this screen plays the user-provided Arduino buzzer arrangement. */
#define ABOUT_MELODY_TEMPO 120 /* source arrangement tempo */
#define ABOUT_MELODY_TICK_MS 100 /* matches the app's own tick_handler period */

static const int marauder_about_melody[] = {
    NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, REST, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, REST, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_C5, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4, NOTE_A5, NOTE_A4, NOTE_A4, NOTE_A5, NOTE_GS5, NOTE_G5, NOTE_DS5, NOTE_D5, NOTE_DS5, REST, NOTE_A4, NOTE_DS5, NOTE_D5, NOTE_CS5, NOTE_C5, NOTE_B4, NOTE_C5, REST, NOTE_F4, NOTE_GS4, NOTE_F4, NOTE_A4, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_A5, NOTE_A4, NOTE_A4, NOTE_A5, NOTE_GS5, NOTE_G5, NOTE_DS5, NOTE_D5, NOTE_DS5, REST, NOTE_A4, NOTE_DS5, NOTE_D5, NOTE_CS5, NOTE_C5, NOTE_B4, NOTE_C5, REST, NOTE_F4, NOTE_GS4, NOTE_F4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4
};

static const int marauder_about_durations[] = {
    -4, -4, 16, 16, 16, 16, 8, 8, -4, -4, 16, 16, 16, 16, 8, 8, 4, 4, 4, -8, 16, 4, -8, 16, 2, 4, 4, 4, -8, 16, 4, -8, 16, 2, 4, -8, 16, 4, -8, 16, 16, 16, 8, 8, 8, 4, -8, 16, 16, 16, 16, 8, 8, 4, -8, -16, 4, -8, 16, 2, 4, -8, 16, 4, -8, 16, 16, 16, 8, 8, 8, 4, -8, 16, 16, 16, 16, 8, 8, 4, -8, -16, 4, -8, 16, 2
};

#define ABOUT_MELODY_NOTE_COUNT (sizeof(marauder_about_melody) / sizeof(marauder_about_melody[0]))

static uint32_t marauder_about_note_ms(size_t i) {
    uint32_t whole_note_ms = (60000u * 4u) / ABOUT_MELODY_TEMPO;
    int divider = marauder_about_durations[i];
    uint32_t note_ms = whole_note_ms / (uint32_t)(divider < 0 ? -divider : divider);

    /* The Arduino source uses a negative divider for dotted notes (one-and-a-half duration). */
    if(divider < 0) note_ms += note_ms / 2;
    return note_ms;
}

static uint32_t marauder_about_ticks(uint32_t ms) {
    /* Round to the nearest app tick. At 150 BPM a sixteenth note is exactly one 100ms tick. */
    uint32_t ticks = (ms + ABOUT_MELODY_TICK_MS / 2) / ABOUT_MELODY_TICK_MS;
    return ticks > 0 ? ticks : 1;
}

static void marauder_gui_about_melody_stop(MarauderGuiApp* app) {
    if(furi_hal_speaker_is_mine()) {
        furi_hal_speaker_stop();
        furi_hal_speaker_release();
    }
    app->about_melody_playing = false;
    app->about_melody_in_gap = false;
}

static void marauder_gui_about_melody_start_note(MarauderGuiApp* app, size_t i) {
    int freq = marauder_about_melody[i];
    if(freq > 0) furi_hal_speaker_start((float)freq, 1.0f);
    app->about_melody_in_gap = false;
    app->about_melody_ticks_remaining = marauder_about_ticks(marauder_about_note_ms(i));
}

/* Driven by the app's own ~100ms tick_handler, same as every other timed animation in this app -
   NOT a separate FuriTimer. furi_hal_speaker_start/stop assert ownership from the calling
   thread, and a FuriTimer callback runs on the system timer-service thread rather than this
   app's own thread, which crashed the Flipper outright the first time this was tried. */
static void marauder_gui_scene_device_about_tick(MarauderGuiApp* app) {
    if(!app->about_melody_playing) return;

    if(app->about_melody_ticks_remaining > 1) {
        app->about_melody_ticks_remaining--;
        return;
    }

    /* Stop/restart at the boundary so repeated pitches remain distinct without inserting a
       full 100ms silent tick, which would halve the effective tempo of sixteenth notes. */
    furi_hal_speaker_stop();
    app->about_melody_index++;
    if(app->about_melody_index >= ABOUT_MELODY_NOTE_COUNT) {
        marauder_gui_about_melody_stop(app);
        return;
    }
    marauder_gui_about_melody_start_note(app, app->about_melody_index);
}

/* Widget has no hook for a raw Ok keypress (only registered button elements fire a callback -
   same limitation already worked around for log_scan's "Devam Et" resume button), so the
   easter egg is wired up as a center button element instead of a scene on_event custom event. */
static void marauder_gui_about_play_button_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    MarauderGuiApp* app = context;
    if(button_type != GuiButtonTypeRight || input_type != InputTypeShort) return;
    if(app->about_melody_playing) return;

    if(!furi_hal_speaker_acquire(1000)) return;

    app->about_melody_playing = true;
    app->about_melody_index = 0;
    marauder_gui_about_melody_start_note(app, 0);
}

void marauder_gui_scene_device_about_on_enter(void* context) {
    MarauderGuiApp* app = context;

    app->about_melody_playing = false;
    app->about_melody_in_gap = false;
    app->about_melody_index = 0;
    app->about_melody_ticks_remaining = 0;
    app->tick_handler = marauder_gui_scene_device_about_tick;

    widget_reset(app->widget);
    widget_add_string_element(
        app->widget, 2, 4, AlignLeft, AlignTop, FontPrimary, "Marauder GUI");
    widget_add_string_element(
        app->widget, 2, 20, AlignLeft, AlignTop, FontSecondary, "ESP32 Marauder");
    widget_add_string_element(
        app->widget,
        2,
        30,
        AlignLeft,
        AlignTop,
        FontSecondary,
        marauder_gui_text(app, "icin Flipper Zero", "GUI app"));
    widget_add_string_element(
        app->widget,
        2,
        40,
        AlignLeft,
        AlignTop,
        FontSecondary,
        marauder_gui_text(app, "GUI uygulamasi", "for Flipper Zero"));
    widget_add_string_element(
        app->widget,
        2,
        48,
        AlignLeft,
        AlignTop,
        FontSecondary,
        marauder_gui_text(app, "Gelistiren: mel4mi", "Developed by: mel4mi"));
    widget_add_button_element(
        app->widget,
        GuiButtonTypeRight,
        marauder_gui_text(app, "Cal", "Play"),
        marauder_gui_about_play_button_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, MarauderGuiViewWidget);
}

bool marauder_gui_scene_device_about_on_event(void* context, SceneManagerEvent event) {
    MarauderGuiApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeBack) {
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}

void marauder_gui_scene_device_about_on_exit(void* context) {
    MarauderGuiApp* app = context;

    marauder_gui_about_melody_stop(app);
    app->tick_handler = NULL;

    widget_reset(app->widget);
}

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define TAG "Flappy"

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} GameEvent;

typedef enum {
    DirectionUp,
    DirectionRight,
    DirectionDown,
    DirectionLeft,
} Direction;

static void flappy_game_render_callback(Canvas* const canvas, void* ctx) { 
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    // canvas_set_font(canvas, FontSecondary);
    // canvas_draw_str(canvas, 30, 20, "FLAPPY FLIPPER - BIRD");
}


static void flappy_game_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue);
    FURI_LOG_D(TAG, "flappy_game_input_callback: event_queue");

    GameEvent event = {.type = EventTypeKey, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}


int32_t flappy_game_app(void* p) {
    FURI_LOG_D(TAG, "Starting application ...");

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(GameEvent), NULL);
    FURI_LOG_D(TAG, "osMessageQueueNew: event_queue");

    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    FURI_LOG_D(TAG, "view_port_alloc: view_port");

    view_port_draw_callback_set(view_port, flappy_game_render_callback, NULL);
    view_port_input_callback_set(view_port, flappy_game_input_callback, &event_queue);
    FURI_LOG_D(TAG, "view_port_draw_callback_set / view_port_input_callback_set");

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    FURI_LOG_D(TAG, "furi_record_open");

    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    FURI_LOG_D(TAG, "gui_add_view_port: GuiLayerFullscreen");

    GameEvent event; 
    for(bool processing = true; processing;) {
        FURI_LOG_D(TAG, "osMessageQueueGet: event_status");
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        if(event_status == osOK) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    FURI_LOG_D(TAG, "InputTypePress: %d", event.input.type);

                    switch(event.input.key) {
                    case InputKeyUp: 
                    case InputKeyDown: 
                    case InputKeyRight: 
                    case InputKeyLeft: 
                    case InputKeyOk: 
                    case InputKeyBack:
                        FURI_LOG_D(TAG, "processing = false;");
                        processing = false;
                        break;
                    }
                }
            } 
        } else {
            FURI_LOG_D(TAG, "osMessageQueue: event timeout");
            // event timeout
        }

        view_port_update(view_port);
        FURI_LOG_D(TAG, "view_port_update");
    }

    FURI_LOG_D(TAG, "unload");
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue); 

    return 0;
}
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
    int x;
    int y;
} POINT;

typedef struct { 
    float gravity;
    POINT point;  
}BIRD;

typedef struct {
    BIRD bird; 
} GameState; 

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

#define BIRD_HEIGHT 15
#define BIRD_WIDTH  10
uint8_t bird_array[15][11] = {
    {0,0,0,0,0,0,1,1,0,0,0},
    {0,0,0,0,0,1,0,0,1,0,0},
    {0,0,0,0,0,1,0,0,0,1,0},
    {0,0,1,1,1,1,0,0,0,1,0},
    {0,1,0,0,0,1,0,0,0,1,0},
    {0,1,0,0,0,0,1,0,1,0,1},
    {1,0,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,1,0,0,1,0,0,1},
    {1,1,0,0,0,0,1,0,1,0,1},
    {1,0,0,0,0,1,0,1,0,1,0},
    {1,0,0,0,0,1,0,1,0,1,0},
    {0,1,0,1,1,1,0,1,0,1,0},
    {0,0,1,0,0,1,0,1,0,1,0},
    {0,0,0,1,1,1,0,1,0,1,0},
    {0,0,0,0,0,0,1,1,1,0,0},
};

static void flappy_game_state_init(GameState* const game_state) {
    BIRD bird; 
    bird.gravity = 0.0f; 
    bird.point.x = 20; 
    bird.point.y = 32;
    game_state->bird = bird; 
}


static void flappy_game_render_callback(Canvas* const canvas, void* ctx) { 
    const GameState* game_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(game_state == NULL) {
        return;
    }

    canvas_draw_frame(canvas, 0, 0, 128, 64);

    // Flappy 
    for (int h = 0; h < BIRD_HEIGHT; h++) {
        for (int w = 0; w < BIRD_WIDTH; w++) {
            int x = game_state->bird.point.x + w; 
            int y = game_state->bird.point.y + h; 

            canvas_draw_dot(canvas, x, y);
        }
    }
    

    release_mutex((ValueMutex*)ctx, game_state);
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

    GameState* game_state = malloc(sizeof(GameState));
    flappy_game_state_init(game_state); 

    ValueMutex state_mutex; 
    if (!init_mutex(&state_mutex, game_state, sizeof(GameState))) {
        FURI_LOG_E(TAG, "cannot create mutex\r\n");
        free(game_state); 
        return 255;
    }


    // Set system callbacks
    ViewPort* view_port = view_port_alloc();
    FURI_LOG_D(TAG, "view_port_alloc: view_port");

    view_port_draw_callback_set(view_port, flappy_game_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, flappy_game_input_callback, event_queue);
    
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

        GameState* game_state = (GameState*)acquire_mutex_block(&state_mutex);

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
        release_mutex(&state_mutex, game_state);

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
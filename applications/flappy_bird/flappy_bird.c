#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define TAG "Flappy"

#define FLAPPY_BIRD_HEIGHT  15
#define FLAPPY_BIRD_WIDTH   10
#define FLAPPY_PILAR_MAX    6
#define FLAPPY_GAB_HEIGHT   25
#define FLAPPY_GAB_WIDTH    3

#define FLIPPER_LCD_WIDTH   128
#define FLIPPER_LCD_HEIGHT  64

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
    POINT point; 
    int height;
    int visible;
    int passed; 
} PILAR; 

typedef struct {
    BIRD bird; 

    int pilars_count;
    PILAR pilars[FLAPPY_PILAR_MAX];
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

static void flappy_random_pilar(GameState* const game_state) {
    PILAR pilar; 

    pilar.visible = 1; 
    pilar.height = random() % (FLIPPER_LCD_HEIGHT - FLAPPY_GAB_HEIGHT) + 1;

    game_state->pilars_count++;
    game_state->pilars[game_state->pilars_count % FLAPPY_PILAR_MAX] = pilar;
}

static void flappy_game_state_init(GameState* const game_state) {
    BIRD bird; 
    bird.gravity = 0.0f; 
    bird.point.x = 20; 
    bird.point.y = 32;

    game_state->bird = bird; 
    game_state->pilars_count = 0;
    memset(game_state->pilars, 0, sizeof(game_state->pilars));

    flappy_random_pilar(game_state);
}

static void flappy_game_render_callback(Canvas* const canvas, void* ctx) { 
    const GameState* game_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(game_state == NULL) {
        return;
    }

    canvas_draw_frame(canvas, 0, 0, 128, 64);

    // Flappy 
    for (int h = 0; h < FLAPPY_BIRD_HEIGHT; h++) {
        for (int w = 0; w < FLAPPY_BIRD_WIDTH; w++) { 
            if (bird_array[h][w] == 1) {
                int x = game_state->bird.point.x + w; 
                int y = game_state->bird.point.y + h; 

                canvas_draw_dot(canvas, x, y);
            }
        }
    }

    release_mutex((ValueMutex*)ctx, game_state);
}


static void flappy_game_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue); 

    GameEvent event = {.type = EventTypeKey, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

// static void snake_game_update_timer_callback(osMessageQueueId_t event_queue) {
//     furi_assert(event_queue);

//     SnakeEvent event = {.type = EventTypeTick};
//     osMessageQueuePut(event_queue, &event, 0, 0);
// }

int32_t flappy_game_app(void* p) { 
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(GameEvent), NULL); 

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
    view_port_draw_callback_set(view_port, flappy_game_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, flappy_game_input_callback, event_queue);
 
    // osTimerId_t timer =
    //     osTimerNew(snake_game_update_timer_callback, osTimerPeriodic, event_queue, NULL);
    // osTimerStart(timer, osKernelGetTickFreq() / 4);
    
    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui"); 
    gui_add_view_port(gui, view_port, GuiLayerFullscreen); 

    GameEvent event; 
    for(bool processing = true; processing;) { 
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);
        GameState* game_state = (GameState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {  
                    switch(event.input.key) {
                    case InputKeyUp: 
                            game_state->bird.point.y++;
                        break; 
                    case InputKeyDown: 
                            game_state->bird.point.y--;
                        break; 
                    case InputKeyRight: 
                            game_state->bird.point.x++;
                        break; 
                    case InputKeyLeft:  
                            game_state->bird.point.y--;
                        break; 
                    case InputKeyOk: 
                    case InputKeyBack: 
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
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue); 

    return 0;
}
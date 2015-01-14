#include <pebble.h>
  
#define DEFAULT_DOLLAR_VALUE 0
#define DOLLAR_KEY 1
#define CENT_KEY 2
#define WAGE_KEY 3
#define SEC_KEY 4
#define MIN_KEY 5
#define HOUR_KEY 6
//#define START_KEY 7
#define TIME_KEY 7

static Window *window; 
  
static GBitmap *action_icon_plus;
static GBitmap *action_icon_minus;
static GBitmap *action_icon_play;

static ActionBarLayer *action_bar;

static TextLayer *header_text_layer;
static TextLayer *body_text_layer;
static TextLayer *label_text_layer;
static TextLayer *footer_text_layer;

static float wage = 0;
static float currentTotal = 0;
static bool startClock;

static int tSec = 0;

static void updateDisplay(){
  static char body_text[50];
  static char label_text[50];
  static char footer_text[50];
  
  int eHour = tSec / 3600;
  int eMin = (tSec % 3600)/60;
  int eSec = ((tSec % 3600)%60);
  
  /* DISPLAY THE WAGE */
  snprintf(label_text, sizeof(label_text), "Wage %d", (int)wage);
  text_layer_set_text(label_text_layer, label_text);
  
  snprintf(body_text, sizeof(body_text), "%d.%02d", (int)(currentTotal), (int)(currentTotal*100)%100);
  text_layer_set_text(body_text_layer, body_text);
  
  snprintf(footer_text, sizeof(footer_text), "Elapsed Time: %d:%d:%d", eHour, eMin, eSec);
  text_layer_set_text(footer_text_layer, footer_text);
}

static void resetTotal(){
  currentTotal = 0;
  tSec = 0;
  startClock = false;
  updateDisplay();
  persist_delete(DOLLAR_KEY);
  persist_delete(CENT_KEY);
  persist_delete(WAGE_KEY);
  persist_delete(SEC_KEY);
  persist_delete(MIN_KEY);
  persist_delete(HOUR_KEY);
  persist_delete(TIME_KEY);
  //persist_delete(START_KEY);
}
  
static void increment_click_handler(){
  startClock = false;
  wage++;
  updateDisplay();
}

static void decrement_click_handler(){
  if (wage > 0) {
    startClock = false;
    wage--;
    updateDisplay();
  }
  else if (wage < 1){
    resetTotal();
  }
}

static void select_click_handler(){
  //Toggle the start button
  startClock = !startClock;
}

//Called once per second
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed){
  
  if(startClock) {
    //Calculate new total
    currentTotal = currentTotal + wage/3600;
  
    tSec++;
    
    //update the screen
    updateDisplay(); 
  } 
}

//Calculate money gained over time outside of the app
static float updateTotals(int oldSec, int oldMin, int oldHour, int curSec, int curMin, int curHour){
  int secondsPast = ( ( abs((curHour) - (oldHour)) * 3600 ) + ( ( (curMin) - (oldMin) ) * 60 ) + ( (curSec) - (oldSec) ) );
  tSec = tSec + secondsPast;
  float newTotal = (float)secondsPast * wage/3600;
  return newTotal;
}

static void click_config_provider(void *context) {
  const uint16_t repeat_internal_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_internal_ms, (ClickHandler) increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_internal_ms, (ClickHandler) decrement_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, repeat_internal_ms, (ClickHandler) select_click_handler);
}

static void window_load(Window *me) {
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, me);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
  
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_play);
  
  Layer *layer = window_get_root_layer(me);
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;
  
  header_text_layer = text_layer_create(GRect(4,0, width, 60));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "On The Clock");
  layer_add_child(layer, text_layer_get_layer(header_text_layer));
  
  body_text_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(body_text_layer));
  
  label_text_layer = text_layer_create(GRect(4, 44 + 28, width, 60));
  text_layer_set_font(label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(label_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(label_text_layer));
  
  footer_text_layer = text_layer_create(GRect(4, 110, width, 60));
  text_layer_set_font(footer_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(footer_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(footer_text_layer));
  
  updateDisplay();
}

static void window_unload(Window *window){
  text_layer_destroy(header_text_layer);
  text_layer_destroy(body_text_layer);
  text_layer_destroy(label_text_layer);
  text_layer_destroy(footer_text_layer);
  
  action_bar_layer_destroy(action_bar);
}

//Initialize the App
static void init(void) {
  int oldSec;
  int oldMin;
  int oldHour;
  
  //load assets
  action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_ACTION_ICON_PLUS);
  action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_ACTION_ICON_MINUS);
  action_icon_play = gbitmap_create_with_resource(RESOURCE_ID_RESOURCE_ID_ACTION_ICON_PLAY);
  
  //Create the base Window
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  
  //Read the persistent data if it exists
  int dollarValue = persist_exists(DOLLAR_KEY) ? persist_read_int(DOLLAR_KEY) : DEFAULT_DOLLAR_VALUE;
  int centValue = persist_exists(CENT_KEY) ? persist_read_int(CENT_KEY) : DEFAULT_DOLLAR_VALUE;
  //startClock = persist_exists(START_KEY) ? persist_read_int(START_KEY) : DEFAULT_DOLLAR_VALUE;
  wage = persist_exists(WAGE_KEY) ? persist_read_int(WAGE_KEY) : DEFAULT_DOLLAR_VALUE;
  tSec = persist_exists(TIME_KEY) ? persist_read_int(TIME_KEY) : DEFAULT_DOLLAR_VALUE;
  
  //create the current time variable
  time_t now = time(NULL);
  struct tm * currentTime = localtime(&now);
  handle_second_tick(currentTime, SECOND_UNIT);
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  float awayTotal;
  
  if(persist_exists(SEC_KEY)){
    oldSec = persist_read_int(SEC_KEY);
    oldMin = persist_read_int(MIN_KEY);
    oldHour = persist_read_int(HOUR_KEY);
    awayTotal = updateTotals(oldSec, oldMin, oldHour, currentTime->tm_sec, currentTime->tm_min, currentTime->tm_hour);
    startClock = true;
  }
  else{
    awayTotal = 0;
  }
  
  //calculate the currentTotal
  currentTotal = (float)(dollarValue) + ((float)(centValue)) / 100 + awayTotal;
  
  window_stack_push(window, true /* Animated */);
}

//Close App
static void deinit(void) {
  
  int returnDollars = currentTotal;
  float ret = returnDollars;
  int returnCents = 100 * (currentTotal - ret);
  time_t now = time(NULL);
  struct tm * exit = localtime(&now);
  
  //Write the startClocks exit state into storage
  //persist_write_int(START_KEY, startClock);
  
  //record de-init time and persist write it
  persist_write_int(SEC_KEY, exit->tm_sec);
  persist_write_int(MIN_KEY, exit->tm_min);
  persist_write_int(HOUR_KEY, exit->tm_hour);
  persist_write_int(TIME_KEY, tSec);
  
  //Write the current total into persistent storage
  persist_write_int(DOLLAR_KEY, returnDollars);
  persist_write_int(CENT_KEY, returnCents);  
  persist_write_int(WAGE_KEY, (int)wage);
  
  //Clear the screen
  window_destroy(window);
  
  //Clear the assets
  gbitmap_destroy(action_icon_plus);
  gbitmap_destroy(action_icon_minus);
  gbitmap_destroy(action_icon_play);
}

//Main event loop
int main(void) {
  init();  
  app_event_loop();
  deinit();
  
  return 0;
}


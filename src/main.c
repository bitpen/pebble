#include <pebble.h>
 
/*
  Pointer to main watchface window
*/
static Window *s_main_window;  

/*
  Pointer to the main text layer of the watch
*/
static TextLayer *s_hour_layer;
static TextLayer *s_num_layer;
static TextLayer *s_den_layer;
static TextLayer *s_bar_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_bluetooth_layer;
static TextLayer *s_date_layer;
static TextLayer *s_day_layer;


/*
  Text layer font
*/
static GFont s_minute_font;
static GFont s_hour_font;
static GFont s_status_font;


static void format_aproximate_time(struct tm *tick_time, char hourBuf[], char numBuf[], char denBuf[], char dayBuf[], char dateBuf[]){ 
  int min = tick_time->tm_min;
  int hour = tick_time->tm_hour;
  
  char approxHour[4];
  char approxNum[4];
  char approxDen[4];
  char day[10];
  char date[6];
  
  //determine numerator by approximating around mutiples of 5
  int num = min / 5;
  if(min % 5 >= 3){
    num++;
  }
  //if approximated towards 12 (60 minutes), approximate as 0, increase hour
  if(num == 12){
    num = 0;
    hour++;
    
    //roll over if needed
    if(hour == 24){
      hour = 0;
    }
  }
  
  //now reduce fraction by finding common denominator
  int denom = 12;
  int common = 1;
  if(num > 0){
    if(num % 6 == 0){
      common = 6;
    }
    else if(num % 4 == 0){
      common = 4;
    }
    else if(num % 3 == 0){
      common = 3;
    }
    else if(num % 2 == 0){
      common = 2;
    }
  }
  
  //reduce calculation
  num = num / common;
  denom = denom / common;
  
  //fill in numerator and denominator buffers
  int index = 0;
  if(num >= 10){
    approxNum[index++] = 49;
    approxNum[index++] = 32;
    num -= 10;
  }
  approxNum[index++] = 48 + num;
  approxNum[index] = '\0';
  
  index = 0;
  if(denom >= 10){
    approxDen[index++] = 49;
    approxDen[index++] = 32;
    denom -=10;
  }
  
  approxDen[index++] = 48 + denom;
  approxDen[index] = '\0';
  
  
  
  //convert to 12 hour if necessary
  if(!clock_is_24h_style()){
    if(hour == 0){
      hour = 12;
    }
    else if(hour > 12){
      hour = hour - 12;
    }
  }
 
  //fill hour buffer
  index = 0;
  if(hour >= 20){
    approxHour[index++] = 50;
    approxHour[index++] = 32;
    hour -= 20;
  }
  else if(hour >= 10){
    approxHour[index++] = 49;
    approxHour[index++] = 32;
    hour -= 10;
  }
  
  approxHour[index++] = 48 + hour;
  approxHour[index] = '\0';
  
  snprintf(hourBuf, sizeof (approxHour), "%s", approxHour);
  snprintf(numBuf, sizeof (approxNum), "%s", approxNum);
  snprintf(denBuf, sizeof (approxDen), "%s", approxDen);
  strftime(dateBuf, sizeof("00.00"), "%m.%d", tick_time);
  strftime(dayBuf, 10, "%a", tick_time);
  return;
}
  

static void update_time(){
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char hourBuffer[4];
  static char numBuffer[4];
  static char denBuffer[4];
  static char dateBuffer[6];
  static char dayBuffer[10];

  
  format_aproximate_time(tick_time, hourBuffer, numBuffer,denBuffer, dayBuffer, dateBuffer);
  
  // Display this time on the TextLayer
  text_layer_set_text(s_hour_layer, hourBuffer);
  text_layer_set_text(s_num_layer, numBuffer);
  text_layer_set_text(s_den_layer, denBuffer);
  text_layer_set_text(s_day_layer, dayBuffer);
  text_layer_set_text(s_date_layer, dateBuffer);
}

static void battery_handler(BatteryChargeState state){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "state changed");
  static char batBuffer[5];
  if(state.is_charging){
    batBuffer[0] = 'C';
    batBuffer[1] = '\0';
  }
  else{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "not charging");
    int percent = state.charge_percent;
    int tens = percent / 10;
    int ones = percent % 10;
    int index = 0;
    if(tens == 10){
      batBuffer[index++] = 49;
      batBuffer[index++] = 48;
    }
    else{
      batBuffer[index++] = tens + 48;
    }
     batBuffer[index++] = ones + 48;
    
    batBuffer[index++] = '%';
    batBuffer[index] = '\0';
  }
  
  text_layer_set_text(s_battery_layer, batBuffer);
}

static void bluetooth_handler(bool connected){
  if(connected){
    text_layer_set_text(s_bluetooth_layer, "b");
  }
  else{
    text_layer_set_text(s_bluetooth_layer, "");
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

static void text_layer_setup(Window *window, TextLayer *text_layer){
  text_layer_set_background_color(text_layer, GColorBlack);
  text_layer_set_text_color(text_layer, GColorWhite);
 
  // Improve the layout to me more like a watchface
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(text_layer, GTextOverflowModeWordWrap);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
}

static void hour_text_layer_create(Window *window){
  // Create time TextLayer
  s_hour_layer = text_layer_create(GRect(0,45,72,60));
  text_layer_setup(window, s_hour_layer);
  
  // Create GFont
  s_hour_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_60));
  // Apply to TextLayer
  text_layer_set_font(s_hour_layer, s_hour_font);
}

static void num_text_layer_create(Window *window){
  // Create time TextLayer
  s_num_layer = text_layer_create(GRect(73,40,72,35));
  text_layer_setup(window, s_num_layer);
  
  // Create GFont
  s_minute_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_32));
  // Apply to TextLayer
  text_layer_set_font(s_num_layer, s_minute_font);
}

static void den_text_layer_create(Window *window){
  // Create time TextLayer
  s_den_layer = text_layer_create(GRect(73,85,72,35));
  text_layer_setup(window, s_den_layer);
  
  // Create GFont
  s_minute_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_32));
  // Apply to TextLayer
  text_layer_set_font(s_den_layer, s_minute_font);
}

static void bar_text_layer_create(Window *window){
  // Create time TextLayer
  s_bar_layer = text_layer_create(GRect(90,80,38,4));
  text_layer_setup(window, s_bar_layer);
  text_layer_set_background_color(s_bar_layer, GColorWhite);
}

static void bluetooth_text_layer_create(Window *window){
  // Create time TextLayer
  s_bluetooth_layer = text_layer_create(GRect(124,0,20,18));
  text_layer_setup(window, s_bluetooth_layer);
  
  // Create GFont
  s_status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_16));
  // Apply to TextLayer
  text_layer_set_font(s_bluetooth_layer, s_status_font);
}

static void battery_text_layer_create(Window *window){
  s_battery_layer = text_layer_create(GRect(3,0,50,18));
  text_layer_setup(window, s_battery_layer);
  
  // Create GFont
  s_status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_16));
  // Apply to TextLayer
  text_layer_set_font(s_battery_layer, s_status_font);
  
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
}

static void date_text_layer_create(Window *window){
  s_date_layer = text_layer_create(GRect(94,150,50,18));
  text_layer_setup(window, s_date_layer);
  
  // Create GFont
  s_status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_16));
  // Apply to TextLayer
  text_layer_set_font(s_date_layer, s_status_font);
  
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
}

static void day_text_layer_create(Window *window){
  s_day_layer = text_layer_create(GRect(5,150,50,18));
  text_layer_setup(window, s_day_layer);
  
  // Create GFont
  s_status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_16));
  // Apply to TextLayer
  text_layer_set_font(s_day_layer, s_status_font);
  
  text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
}

static void main_window_load(Window *window){
  window_set_background_color(window, GColorBlack);
   APP_LOG(APP_LOG_LEVEL_DEBUG, "window loading" );
  hour_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "hour loaded" );
  num_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "num loaded" );
  bar_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "bar loaded" );
  den_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "den loaded" );
  bluetooth_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "bluetooth loaded" );
  battery_text_layer_create(window);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery loaded" );
  date_text_layer_create(window);
  day_text_layer_create(window);
}

static void main_window_unload(Window *window){
  // Unload GFont
  fonts_unload_custom_font(s_hour_font);
  fonts_unload_custom_font(s_minute_font);
  fonts_unload_custom_font(s_status_font);
  
  // Destroy TextLayer
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_num_layer);
  text_layer_destroy(s_den_layer);
  text_layer_destroy(s_bar_layer);
  text_layer_destroy(s_bluetooth_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_day_layer);
}



static void init(){
  // Create main window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  
  //show bluetooth from the start
  bluetooth_handler(bluetooth_connection_service_peek());
  
  //show battery from the start
  battery_handler(battery_state_service_peek());
  
  // Register event services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  bluetooth_connection_service_subscribe(bluetooth_handler);
  battery_state_service_subscribe(battery_handler);
} 

static void deinit(){
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
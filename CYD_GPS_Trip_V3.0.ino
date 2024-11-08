#include "arduino_secrets.h"

// erstellt von stevedee78 am 08.11.2024 

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <TinyGPS++.h>
#include <lvgl.h>

// Pins fÃ¼r Touchscreen
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

#define GPS_BAUDRATE 9600

// Touchscreen-Kalibrierungswerte
#define XPT2046_XMIN 290
#define XPT2046_XMAX 3670
#define XPT2046_YMIN 230
#define XPT2046_YMAX 3860

// Display und Touchscreen UI-Elemente
lv_disp_draw_buf_t draw_buf;
lv_color_t buf[TFT_HEIGHT * 10];
lv_indev_drv_t touch_drv;
lv_indev_t *touch_indev;
lv_obj_t *scr, *label_speed, *label_odometer, *label_time, *label_date, *label_satellites, *label_compass, *resetButton;
double dailyOdometer = 0.0;
double lastLatitude = 0.0, lastLongitude = 0.0;

void resetOdometer() {
  dailyOdometer = 0.0;
  lv_label_set_text(label_odometer, "Tageskm: 0.00 km");
}

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(GPS_BAUDRATE, SERIAL_8N1, 16, 17);

  // SPI-Instanz initialisieren
  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Display initialisieren und invertieren
  tft.init();
  tft.setRotation(1);  // Ãndern Sie dies ggf. auf 2 oder 3 fÃ¼r gewÃ¼nschte Orientierung
  tft.fillScreen(TFT_BLACK);

  // LVGL initialisieren
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_HEIGHT * 10);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = TFT_HEIGHT;
  disp_drv.ver_res = TFT_WIDTH;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Touch-Driver initialisieren
  lv_indev_drv_init(&touch_drv);
  touch_drv.type = LV_INDEV_TYPE_POINTER;
  touch_drv.read_cb = touch_read_cb;
  touch_indev = lv_indev_drv_register(&touch_drv);

  // UI-Elemente einrichten
  scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_scr_load(scr);
  
  // Geschwindigkeit - GroÃe und fette Schrift
  label_speed = lv_label_create(scr);
  lv_obj_set_style_text_color(label_speed, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label_speed, &lv_font_montserrat_40, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_speed, LV_ALIGN_CENTER, 0, -30);

  // Tageskilometer
  label_odometer = lv_label_create(scr);
  lv_obj_set_style_text_color(label_odometer, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_odometer, LV_ALIGN_CENTER, 0, 20);

  // Zeit
  label_time = lv_label_create(scr);
  lv_obj_set_style_text_color(label_time, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_time, LV_ALIGN_TOP_RIGHT, -10, 10);

  // Datum
  label_date = lv_label_create(scr);
  lv_obj_set_style_text_color(label_date, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_date, LV_ALIGN_TOP_RIGHT, -10, 40);

  // Satellitenanzahl
  label_satellites = lv_label_create(scr);
  lv_obj_set_style_text_color(label_satellites, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_satellites, LV_ALIGN_TOP_LEFT, 10, 10);

  // Kompass - Textbasierte Richtung
  label_compass = lv_label_create(scr);
  lv_obj_set_style_text_color(label_compass, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(label_compass, LV_ALIGN_BOTTOM_MID, 0, -20);

  // Reset-Button
  resetButton = lv_btn_create(scr);
  lv_obj_set_size(resetButton, 80, 40);
  lv_obj_align(resetButton, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_add_event_cb(resetButton, [](lv_event_t* e) {
    resetOdometer();
  }, LV_EVENT_CLICKED, NULL);

  // Text auf dem Button hinzufÃ¼gen und zentrieren
  lv_obj_t* resetLabel = lv_label_create(resetButton);
  lv_label_set_text(resetLabel, "Reset");
  lv_obj_center(resetLabel);  // Text zentriert im Button ausrichten



}

void updateGPSData() {
  // GPS-Daten aktualisieren
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Geschwindigkeit anzeigen
  if (gps.location.isUpdated()) {
    char speed[10];
    sprintf(speed, "%d km/h", int(gps.speed.kmph()));
    lv_label_set_text(label_speed, speed);

    // TageskilometerzÃ¤hler aktualisieren
    if (gps.location.isValid()) {
      if (lastLatitude != 0 && lastLongitude != 0) {
        double distance = TinyGPSPlus::distanceBetween(
          lastLatitude, lastLongitude,
          gps.location.lat(), gps.location.lng()
        ) / 1000.0;
        dailyOdometer += distance;
      }
      lastLatitude = gps.location.lat();
      lastLongitude = gps.location.lng();
    }

    char odometerText[20];
    sprintf(odometerText, "Tageskm: %.2f km", dailyOdometer);
    lv_label_set_text(label_odometer, odometerText);
  }

  // Datum und Zeit
  if (gps.date.isValid()) {
    char date[12];
    sprintf(date, "%02d.%02d.%04d", gps.date.day(), gps.date.month(), gps.date.year());
    lv_label_set_text(label_date, date);
  }
  if (gps.time.isUpdated()) {
    int hour = gps.time.hour() + 1;  // Zeitzone anpassen
    if (hour >= 24) hour -= 24;
    char time[10];
    sprintf(time, "%02d:%02d:%02d", hour, gps.time.minute(), gps.time.second());
    lv_label_set_text(label_time, time);
  }

  // Satelliten anzeigen
  if (gps.satellites.isUpdated()) {
    char satellites[20];
    sprintf(satellites, "Satelliten: %d", gps.satellites.value());
    lv_label_set_text(label_satellites, satellites);
  }
}

void updateCompass() {
  // Kompassrichtung als Text aktualisieren
  if (gps.course.isUpdated()) {
    int angle = gps.course.deg();
    const char* direction = lv_chart_get_series_str_compass(angle);
    lv_label_set_text(label_compass, direction);
  }
}

// Touch-Eingabe Callback fÃ¼r LVGL
void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  if (ts.tirqTouched() && ts.touched()) {
    TS_Point point = ts.getPoint();
    data->point.x = map(point.x, XPT2046_XMIN, XPT2046_XMAX, 0, TFT_HEIGHT);
    data->point.y = map(point.y, XPT2046_YMIN, XPT2046_YMAX, 0, TFT_WIDTH);
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void loop() {
  updateGPSData();  // GPS-Daten aktualisieren und anzeigen
  updateCompass();   // Kompass-Richtung als Text aktualisieren
  lv_task_handler(); // LVGL-Anzeige aktualisieren
  delay(100);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      tft.pushColor(color_p->full);
      color_p++;
    }
  }
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

const char* lv_chart_get_series_str_compass(int angle) {
  if (angle < 22.5 || angle >= 337.5) return "N";
  if (angle >= 22.5 && angle < 67.5) return "NO";
  if (angle >= 67.5 && angle < 112.5) return "O";
  if (angle >= 112.5 && angle < 157.5) return "SO";
  if (angle >= 157.5 && angle < 202.5) return "S";
  if (angle >= 202.5 && angle < 247.5) return "SW";
  if (angle >= 247.5 && angle < 292.5) return "W";
  if (angle >= 292.5 && angle < 337.5) return "NW";
  return "Unbekannt";
}

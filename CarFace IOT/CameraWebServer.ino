#include <Preferences.h>

#include <EEPROM.h>
#include <WiFiManager.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <ETH.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include "esp_camera.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER  // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#define SERVER_NAME_ADDR 0   // Dirección de memoria donde se guarda serverName
#define SERVER_PORT_ADDR 40  // Dirección de memoria donde se guarda serverPort
#define SERVER_NAME_SIZE 40  // Tamaño en bytes de serverName
#define SERVER_PORT_SIZE 6   // Tamaño en bytes de serverPort


#include "camera_pins.h"

Preferences preferences;
char mqtt_server[40];
char mqtt_port[6] = "8080";
char mqtt_dispositivo[40];
char mqtt_password[40];

char servidor_CarFace[40];
char puerto_CarFace[6];
char dispositivo[40];
char password[40];

WiFiManagerParameter custom_mqtt_server("server", "Server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "Port", mqtt_port, 6);
WiFiManagerParameter custom_mqtt_dispositivo("dispositivo", "Usuario Dispositivo", mqtt_dispositivo, 40);
WiFiManagerParameter custom_mqtt_password("password", "Contraseña Dispositivo", mqtt_password, 40);

WiFiManager wifiManager;

void startCameraServer();
bool login();
//flag for saving data
bool shouldSaveConfig = false;
//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  preferences.begin("CarFace", false);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  strcpy(servidor_CarFace, preferences.getString("server", "NULO").c_str());
  strcpy(puerto_CarFace, preferences.getString("port", "8080").c_str());
  strcpy(dispositivo, preferences.getString("dispositivo", "NULO").c_str());
  strcpy(password, preferences.getString("password", "NULO").c_str());
  strcpy(password, preferences.getString("token", "NULO").c_str());
  conectar_wifi();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  /*EEPROM.put(SERVER_NAME_ADDR, servidor_CarFace);
  EEPROM.put(SERVER_PORT_ADDR, puerto_CarFace);*/
}


void conectar_wifi() {
  //wifiManager.resetSettings();
  Serial.println("Vamos a conectar WiFi");
  Serial.println("Servidor: " + String(servidor_CarFace));
  Serial.println("Puerto: " + String(puerto_CarFace));
  Serial.println("Dispositivo: " + String(dispositivo));
  Serial.println("Password: " + String(password));
  if (String(servidor_CarFace) == "NULO" || String(dispositivo) == "NULO" || String(password) == "NULO") {
    wifiManager.resetSettings();
  }
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_dispositivo);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.autoConnect("CarFace");
  // Obtener los valores ingresados por el usuario
  if (shouldSaveConfig) {
    Serial.println("GUARDANDO VARIABLES");
    String serverNameString = custom_mqtt_server.getValue();
    String serverPortString = custom_mqtt_port.getValue();
    String dispositivoString = custom_mqtt_dispositivo.getValue();
    String passwordString = custom_mqtt_password.getValue();
    // Asignar los valores a las variables externas
    serverNameString.toCharArray(servidor_CarFace, 40);
    serverPortString.toCharArray(puerto_CarFace, 6);
    dispositivoString.toCharArray(dispositivo, 40);
    passwordString.toCharArray(password, 40);
  }
  Serial.println("WiFi connected");
  preferences.putString("server", servidor_CarFace);
  preferences.putString("port", puerto_CarFace);
  preferences.putString("dispositivo", dispositivo);
  preferences.putString("password", password);
  preferences.end();
  Serial.println("Valores guardados en la memoria.");
  if (login() == false) {
    wifiManager.resetSettings();
  }
  startCameraServer();
}

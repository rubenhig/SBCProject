/*
ESP32-CAM Enroll faces by getting remote images from web server and recognize faces automatically.
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-6-29 21:30
https://www.facebook.com/francefu
*/

//WIFI
const char* ssid = "TekiTeki";
const char* password = "officeconnect123";

//AP  http://192.168.4.1
const char* apssid = "esp32-cam";
const char* appassword = "12345678";         //AP  

//Reconocimiento facial El número de imágenes registradas del mismo rostro
#define ENROLL_CONFIRM_TIMES 5
//Número de reconocimiento facial registrado
#define FACE_ID_SAVE_NUMBER 7

//Establecer el nombre de la persona que se muestra en el reconocimiento facial
String recognize_face_matched_name[7] = {"Name0","Name1","Name2","Name3","Name4","Name5","Name6"};    // 7 persons

//Toma el ejemplo oficial get-Still Botón para conseguirCIF(400x296)Se puede reconocer la resolución para cargar las fotos de la cara en el espacio del sitio web de github u otros espacios del sitio web
//Foto de rostro registrado 5 images * 7 person = 35 photos
String imageDomain[10] = {"raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com", "raw.githubusercontent.com"};
String imageRequest[10] = {"/rubenhig/SBCProject/master/foto/ruben1.jpg", "/rubenhig/SBCProject/master/foto/ruben2.jpg", "/rubenhig/SBCProject/master/foto/ruben3.jpg", "/rubenhig/SBCProject/master/foto/ruben4.jpg", "/rubenhig/SBCProject/master/foto/ruben5.jpg", "/rubenhig/SBCProject/master/foto/ruben1.jpg", "/rubenhig/SBCProject/master/foto/ruben1.jpg", "/rubenhig/SBCProject/master/foto/ruben1.jpg", "/rubenhig/SBCProject/master/foto/ruben1.jpg", "/rubenhig/SBCProject/master/foto/ruben1.jpg"};
int image_width = 400;  
int image_height = 296;
//CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120)

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>    //SPIFFS biblioteca de acceso
#include <FS.h>        //Biblioteca de acceso a archivos
#include "soc/soc.h"             //Se utiliza para suministro de energía inestable sin reiniciar
#include "soc/rtc_cntl_reg.h"    //Se utiliza para suministro de energía inestable sin reiniciar
#include "esp_camera.h"          //Función de video
#include "img_converters.h"      //Función de conversión de formato de imagen
#include "fb_gfx.h"              //Función de dibujo de imágenes
#include "fd_forward.h"          //Función de dibujo de imágenes
#include "fr_forward.h"          //Función de dibujo de imágenes

//ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//初始值
static mtmn_config_t mtmn_config = {0};
static face_id_list id_list = {0};
int8_t enroll_id = 0;

void FaceMatched(int faceid) {  //Rostro registrado reconocido
  if (faceid==0) {  
  } 
  else if (faceid==1) { 
  } 
  else if (faceid==2) { 
  } 
  else if (faceid==3) { 
  } 
  else if (faceid==4) { 
  } 
  else if (faceid==5) { 
  } 
  else if (faceid==6) {
  } 
  else {
  }   
}

void FaceNoMatched() {  //Reconocer el rostro de un extraño
  
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //Apague la energía cuando la energía sea inestable y reinicie la configuración
    
  Serial.begin(115200);
  Serial.setDebugOutput(true);  //Encienda la salida debug
  Serial.println();

  setup_camera();
  setup_wifi();
      
  
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);  

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mounted error");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }    

  enrollImageRemote();  //Leer rostros registrados en archivos de imágenes remotos
}

void loop() {
  faceRecognition();
  delay(1000);
}

void enrollImageRemote() {  //Obtener el rostro de registro de fotos remoto
  if (WiFi.status() != WL_CONNECTED) 
    ESP.restart();
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  //Configuración de parámetros de detección de rostros
  //https://github.com/espressif/esp-dl/blob/master/face_detection/README.md
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
    
  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);   
  
  String filename = "/enrollface.jpg";  //spiffs Guardar nombre de archivo de foto remoto
  String domain = "";
  String request = "";
  int port = 443;
      
  //Al obtener la cara registrada de la foto en la nube, a veces no se podrá leer. Es necesario agregar condiciones para determinar si el registro se ha completado o reiniciar la energía para volver a leerlo.
  int len = sizeof(imageDomain)/sizeof(*imageDomain);
  if (len>0) {
    for (int i=0;i<len;i++) {
      String domain = imageDomain[i];
      String request = imageRequest[i];
  
      Serial.println("");
      Serial.print("Connecting to ");
      Serial.println(domain);
        
      if (client_tcp.connect(domain.c_str(), port)) {
        Serial.println("GET " + request);
        client_tcp.println("GET " + request + " HTTP/1.1");
        client_tcp.println("Host: " + domain);
        client_tcp.println("Content-type:image/jpeg; charset=utf-8");
        client_tcp.println("Connection: close");
        client_tcp.println();
    
        String getResponse="";
        boolean state = false;
        int waitTime = 1000;   // timeout 1 seconds
        long startTime = millis();
        char c;
        File file = SPIFFS.open(filename, FILE_WRITE);  //spiffs Guardar fotos remotas
        while ((startTime + waitTime) > millis()) {
          while (client_tcp.available()) {
              c = client_tcp.read();
              if (state==false) {
                //Serial.print(String(c)); 
              }
              else if (state==true) {
                file.print(c);
              }
                         
              if (c == '\n') {
                if (getResponse.length()==0) state=true; 
                getResponse = "";
              } 
              else if (c != '\r') {
                getResponse += String(c);
              }
              startTime = millis();
           }
        }
        client_tcp.stop();
        
        file.close();
     
        file = SPIFFS.open(filename);  //Leer puesta en escena spiffs Registro de fotos
        if(!file){
          Serial.println("Failed to open file for reading");   
        } else {
          Serial.println("file size: "+String(file.size())); 
          char *buf;
          buf = (char*) malloc (sizeof(char)*file.size());
          long i = 0;
          while (file.available()) {
            buf[i] = file.read(); 
            i++;  
          }
        
          dl_matrix3du_t *aligned_face = NULL;
          int8_t left_sample_face = NULL;
          dl_matrix3du_t *image_matrix = NULL;
          
          image_matrix = dl_matrix3du_alloc(1, image_width, image_height, 3);  //Asignar memoria interna
          if (!image_matrix) {
              Serial.println("dl_matrix3du_alloc failed");
          } else { 
              fmt2rgb888((uint8_t*)buf, file.size(), PIXFORMAT_JPEG, image_matrix->item);  //Conversión de formato de imagen RGB
        
              box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);  // Realizar detección de rostros para obtener datos de fotogramas de rostros
              if (net_boxes){
                Serial.println("faces = " + String(net_boxes->len));  //Número de rostros detectados
                Serial.println();
                for (int i = 0; i < net_boxes->len; i++){  //Enumere la posición y el tamaño de la cara
                    Serial.println("index = " + String(i));
                    int x = (int)net_boxes->box[i].box_p[0];
                    Serial.println("x = " + String(x));
                    int y = (int)net_boxes->box[i].box_p[1];
                    Serial.println("y = " + String(y));
                    int w = (int)net_boxes->box[i].box_p[2] - x + 1;
                    Serial.println("width = " + String(w));
                    int h = (int)net_boxes->box[i].box_p[3] - y + 1;
                    Serial.println("height = " + String(h));
                    Serial.println();
      
                    //Rostro registrado
                    if (i==0) {
                      aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
                      if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
                        if(!aligned_face){
                            Serial.println("Could not allocate face recognition buffer");
                        } 
                        else {
                          int8_t left_sample_face = enroll_face(&id_list, aligned_face);
              
                          if(left_sample_face == (ENROLL_CONFIRM_TIMES - 1)){
                              enroll_id = id_list.tail;
                              Serial.printf("Enrolling Face ID: %d\n", enroll_id);
                          }
                          Serial.printf("Enrolling Face ID: %d sample %d\n", enroll_id, ENROLL_CONFIRM_TIMES - left_sample_face);
                          if (left_sample_face == 0){
                              enroll_id = id_list.tail;
                              //Serial.printf("Enrolled Face ID: %d\n", enroll_id);
                          }
                          Serial.println();
                        }
                        dl_matrix3du_free(aligned_face);
                      }
                    }
                } 
                dl_lib_free(net_boxes->score);
                dl_lib_free(net_boxes->box);
                dl_lib_free(net_boxes->landmark);
                dl_lib_free(net_boxes);                                
                net_boxes = NULL;
              }
              else {
                Serial.println("No Face");    //No se detectó rostro
                Serial.println();
              }
              dl_matrix3du_free(image_matrix);
            }
            free(buf);      
        }
        file.close();
      }  
    }
  }
}

void faceRecognition() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Camera capture failed");
      ESP.restart();
  }
  size_t out_len, out_width, out_height;
  uint8_t * out_buf;
  bool s;
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  if (!image_matrix) {
      esp_camera_fb_return(fb);
      Serial.println("dl_matrix3du_alloc failed");
      return;
  }
  out_buf = image_matrix->item;
  out_len = fb->width * fb->height * 3;
  out_width = fb->width;
  out_height = fb->height;
  s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
  esp_camera_fb_return(fb);
  if(!s){
      dl_matrix3du_free(image_matrix);
      Serial.println("to rgb888 failed");
      return;
  }
  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);  //Realizar detección de rostros
  if (net_boxes){
      run_face_recognition(image_matrix, net_boxes);  //Realizar reconocimiento facial
      dl_lib_free(net_boxes->score);
      dl_lib_free(net_boxes->box);
      dl_lib_free(net_boxes->landmark);
      dl_lib_free(net_boxes);                                
      net_boxes = NULL;
  }
  dl_matrix3du_free(image_matrix);
}

//Función de reconocimiento facial
static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes){  
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK){
        matched_id = recognize_face(&id_list, aligned_face);  //Reconocimiento facial
        if (matched_id >= 0) {  //Si se reconoce como rostro registrado
            Serial.printf("Match Face ID: %u\n", matched_id);
            int name_length = sizeof(recognize_face_matched_name) / sizeof(recognize_face_matched_name[0]);
            if (matched_id<name_length) {
              Serial.printf("Match Face Name: %s\n", recognize_face_matched_name[matched_id]);
            }
            else {
              Serial.printf("Match Face Name: No name");
            }  
            Serial.println();
            FaceMatched(matched_id);  //Reconocer la cara registrada para ejecutar el control de comando
        } else {  //Si se reconoce como una cara no registrada
            Serial.println("No Match Found");
            Serial.println();
            matched_id = -1;
            FaceNoMatched();  //Reconocer como una cara extraña y ejecutar el control de comando
        }
    } else {  //Si se detecta un rostro humano, pero no se puede reconocer
        Serial.println("Face Not Aligned");
        Serial.println();
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

void setup_camera() {
  //Ajustes de configuración de video  https://github.com/espressif/esp32-camera/blob/master/driver/include/esp_camera.h
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

  //
  // WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
  //            Ensure ESP32 Wrover Module or other board with PSRAM is selected
  //            Partial images will be transmitted if image exceeds buffer size
  //   
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){  //是否有PSRAM(Psuedo SRAM)記憶體IC
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  //Inicialización de video
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  //Tamaño predeterminado de fotograma de video personalizable (tamaño de resolución)
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_CIF);    //Resolucion UXGA(1600x1200), SXGA(1280x1024), XGA(1024x768), SVGA(800x600), VGA(640x480), CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120), QXGA(2048x1564 for OV3660)

  //s->set_vflip(s, 1);  //垂直翻轉
  //s->set_hmirror(s, 1);  //水平鏡像

  //flash(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);

  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));


  for (int i=0;i<2;i++) {
    WiFi.begin(ssid, password);    
  
    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;   
    } 
  
    if (WiFi.status() == WL_CONNECTED) {    
      WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //設定SSID顯示客戶端IP         
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
  
      for (int i=0;i<5;i++) {   //WIFI
        ledcWrite(4,10);
        delay(200);
        ledcWrite(4,0);
        delay(200);    
      }
      
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) { 
    ESP.restart();
  }
}

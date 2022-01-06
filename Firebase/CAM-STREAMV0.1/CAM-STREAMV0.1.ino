/*
ESP32-CAM CameraWebServer (No face detection)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-7-1 00:00
https://www.facebook.com/francefu

Face recognition works well in v1.0.5, v1.0.6 or above.

http://192.168.xxx.xxx:81/stream   //Obtener imagen de transmisión     Sintaxis de la página web <img src="http://192.168.xxx.xxx:81/stream">
http://192.168.xxx.xxx/status      //Obtener el valor del parámetro de video

Establecer parámetros de video (formato de comando oficial)  http://192.168.xxx.xxx/control?var=*****&val=*****

http://192.168.xxx.xxx/control?var=flash&val=value          //destello value= 0~255
http://192.168.xxx.xxx/control?var=framesize&val=value      //Resolución value = 10->UXGA(1600x1200), 9->SXGA(1280x1024), 8->XGA(1024x768) ,7->SVGA(800x600), 6->VGA(640x480), 5 selected=selected->CIF(400x296), 4->QVGA(320x240), 3->HQVGA(240x176), 0->QQVGA(160x120), 11->QXGA(2048x1564 for OV3660)
http://192.168.xxx.xxx/control?var=quality&val=value        //Calidad de imagen value = 10 ~ 63
http://192.168.xxx.xxx/control?var=brightness&val=value     //brillo value = -2 ~ 2
http://192.168.xxx.xxx/control?var=contrast&val=value       //Comparado value = -2 ~ 2
http://192.168.xxx.xxx/control?var=saturation&val=value     //saturación value = -2 ~ 2 
http://192.168.xxx.xxx/control?var=gainceiling&val=value    //Límite superior de ganancia automática (cuando está encendido) value = 0 ~ 6
http://192.168.xxx.xxx/control?var=colorbar&val=value       //Pantalla de barra de color value = 0 or 1
http://192.168.xxx.xxx/control?var=awb&val=value            //Balance de blancos value = 0 or 1 
http://192.168.xxx.xxx/control?var=agc&val=value            //Control de ganancia automática value = 0 or 1 
http://192.168.xxx.xxx/control?var=aec&val=value            //Sensor de exposición automático value = 0 or 1 
http://192.168.xxx.xxx/control?var=hmirror&val=value        //Reflejo horizontal value = 0 or 1 
http://192.168.xxx.xxx/control?var=vflip&val=value          //Voltear verticalmente value = 0 or 1 
http://192.168.xxx.xxx/control?var=awb_gain&val=value       //Ganancia automática del balance de blancos value = 0 or 1 
http://192.168.xxx.xxx/control?var=agc_gain&val=value       //Ganancia automática (cuando está apagado) value = 0 ~ 30
http://192.168.xxx.xxx/control?var=aec_value&val=value      //Valor de exposición value = 0 ~ 1200
http://192.168.xxx.xxx/control?var=aec2&val=value           //Control de exposición automático value = 0 or 1 
http://192.168.xxx.xxx/control?var=dcw&val=value            //Usar tamaño de imagen personalizado value = 0 or 1 
http://192.168.xxx.xxx/control?var=bpc&val=value            //Corrección de píxeles negros value = 0 or 1 
http://192.168.xxx.xxx/control?var=wpc&val=value            //Corrección de píxeles blancos value = 0 or 1 
http://192.168.xxx.xxx/control?var=raw_gma&val=value        //Gamma sin procesar value = 0 or 1 
http://192.168.xxx.xxx/control?var=lenc&val=value           //Corrección de lentes value = 0 or 1 
http://192.168.xxx.xxx/control?var=special_effect&val=value //Efectos especiales value = 0 ~ 6
http://192.168.xxx.xxx/control?var=wb_mode&val=value        //Modo de balance de blancos value = 0 ~ 4
http://192.168.xxx.xxx/control?var=ae_level&val=value       //Nivel de exposición automática value = -2 ~ 2 

Descripción de los parámetros de video
https://heyrick.eu/blog/index.php?diary=20210418
*/


const char* ssid = "TekiTeki";
const char* password = "officeconnect123";

#include "soc/soc.h"              
#include "soc/rtc_cntl_reg.h"     

#include <WiFi.h>
#include "esp_camera.h"          //Función de video
#include "esp_http_server.h"     //Función del servidor HTTP
#include "img_converters.h"      //Función de conversión de formato de imagen

//ESP32-CAM Configuración de pines del módulo 
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

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

//Configuración del encabezado web de transmisión de imágenes
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

void startCameraServer();
/*
 * 
 * 
 *  SETUP 
 * 
 */
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //Apague la energía cuando la energía sea inestable y reinicie la configuración
    
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  Serial.println();

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
  if(psramFound()){  //¿Hay un IC de memoria PSRAM (Psuedo SRAM)?
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

  //Tamaño de fotograma de video predeterminado personalizable (tamaño de resolución)
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_CIF);    //Resolución UXGA(1600x1200), SXGA(1280x1024), XGA(1024x768), SVGA(800x600), VGA(640x480), CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120), QXGA(2048x1564 for OV3660)

  //s->set_vflip(s, 1);  //Voltear verticalmente
  //s->set_hmirror(s, 1);  //Reflejo horizontal
          
  //destello(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8); 
  
  setup_wifi();
  
  startCameraServer();    //Inicie el servidor de video

  //Ponga el flash en un nivel bajo
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);              
}

/***************************************************************************************************************************
 * 
 * 
 * LOOP
 * 
 *
 ***************************************************************************************************************************/

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Hola");
  delay(10000);
}



void setup_wifi() {
WiFi.mode(WIFI_AP_STA);  //Otros modos WiFi.mode(WIFI_AP); WiFi.mode(WIFI_STA);

  //ClientIP
  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));

  for (int i=0;i<2;i++) {
    WiFi.begin(ssid, password);    //Realizar conexión de red
  
    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;    //Espere 10 segundos para conectarse
    } 
  
    if (WiFi.status() == WL_CONNECTED) {    //Si la conexión es exitosa
             
      Serial.println("");
      Serial.println("STAIP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");
  
      for (int i=0;i<5;i++) {   //Si está conectado a WIFI, configure el flash para que parpadee rápidamente
        ledcWrite(4,10);
        delay(200);
        ledcWrite(4,0);
        delay(200);    
      }
      break;
    }
  } 

  if (WiFi.status() != WL_CONNECTED) {    //Si falla la conexión
         

    for (int i=0;i<2;i++) {    //Si no puede conectarse a WIFI, configure el flash para que parpadee lentamente
      ledcWrite(4,10);
      delay(1000);
      ledcWrite(4,0);
      delay(1000);    
    }
  } 
  
  //Especifique la IP final del AP
  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  Serial.println("");
  Serial.println("APIP address: ");
  Serial.println(WiFi.softAPIP());  
  Serial.println("");
  
   
}





/*
 * 
 * 
 * 
 * FUNCION esp_err_t stream_handler
 * 
 * 
 * 
 */
//Vídeo transmitido en vivo
static esp_err_t stream_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
          if(fb->format != PIXFORMAT_JPEG){
              bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
              esp_camera_fb_return(fb);
              fb = NULL;
              if(!jpeg_converted){
                  Serial.println("JPEG compression failed");
                  res = ESP_FAIL;
              }
          } else {
              _jpg_buf_len = fb->len;
              _jpg_buf = fb->buf;
          }
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }                
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
    }

    return res;
}
/*
 * 
 * 
 * 
 * FUNCION cmd_handler
 * 
 * 
 * 
 */
//Procesamiento de parámetros de instrucciones de URL
static esp_err_t cmd_handler(httpd_req_t *req){
    char*  buf;       //La cadena de parámetros después de la URL de acceso
    size_t buf_len;
    char variable[32] = {0,};  //Acceda al valor de var del parámetro, puede modificar la longitud de la matriz.
    char value[32] = {0,};     //La longitud de la matriz se puede modificar accediendo al valor del parámetro val.

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {  //La cadena de parámetros después de desensamblar la URL.
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);  //Convierta el valor de val en un número entero, el formato de caracteres original
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    if(!strcmp(variable, "framesize")) {  //Resolución
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(variable, "quality")) res = s->set_quality(s, val);  //Calidad de imagen
    else if(!strcmp(variable, "contrast")) res = s->set_contrast(s, val);  //Comparado
    else if(!strcmp(variable, "brightness")) res = s->set_brightness(s, val);  //brillo
    else if(!strcmp(variable, "saturation")) res = s->set_saturation(s, val);  //saturación
    else if(!strcmp(variable, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);  //Límite superior de ganancia automática (cuando está encendido)
    else if(!strcmp(variable, "colorbar")) res = s->set_colorbar(s, val);  //Pantalla de barra de color
    else if(!strcmp(variable, "awb")) res = s->set_whitebal(s, val);  //Balance de blancos
    else if(!strcmp(variable, "agc")) res = s->set_gain_ctrl(s, val);  //Control de ganancia automática
    else if(!strcmp(variable, "aec")) res = s->set_exposure_ctrl(s, val);  //Sensor de exposición automático
    else if(!strcmp(variable, "hmirror")) res = s->set_hmirror(s, val);  //Reflejo horizontal
    else if(!strcmp(variable, "vflip")) res = s->set_vflip(s, val);  //Voltear verticalmente
    else if(!strcmp(variable, "awb_gain")) res = s->set_awb_gain(s, val);  //Ganancia automática del balance de blancos
    else if(!strcmp(variable, "agc_gain")) res = s->set_agc_gain(s, val);  //Ganancia automática (cuando está apagado)
    else if(!strcmp(variable, "aec_value")) res = s->set_aec_value(s, val);  //Valor de exposición
    else if(!strcmp(variable, "aec2")) res = s->set_aec2(s, val);  //Control de exposición automático
    else if(!strcmp(variable, "dcw")) res = s->set_dcw(s, val);  //Usar tamaño de imagen personalizado
    else if(!strcmp(variable, "bpc")) res = s->set_bpc(s, val);  //Corrección de píxeles negros
    else if(!strcmp(variable, "wpc")) res = s->set_wpc(s, val);  //Corrección de píxeles blancos
    else if(!strcmp(variable, "raw_gma")) res = s->set_raw_gma(s, val);  //Gamma sin procesar
    else if(!strcmp(variable, "lenc")) res = s->set_lenc(s, val);  //Corrección de lentes
    else if(!strcmp(variable, "special_effect")) res = s->set_special_effect(s, val);  //Efectos especiales
    else if(!strcmp(variable, "wb_mode")) res = s->set_wb_mode(s, val);  //Modo de balance de blancos
    else if(!strcmp(variable, "ae_level")) res = s->set_ae_level(s, val);  //Nivel de exposición automática
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}
/*
 * 
 * 
 * 
 * FUNCION esp_err_t status_handler
 * 
 * 
 * 
 */

//Establezca el valor inicial del menú para recuperar el formato json
static esp_err_t status_handler(httpd_req_t *req){
    static char json_response[1024];

    sensor_t * s = esp_camera_sensor_get();
    char * p = json_response;
    *p++ = '{';

    p+=sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p+=sprintf(p, "\"quality\":%u,", s->status.quality);
    p+=sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p+=sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p+=sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p+=sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p+=sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p+=sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p+=sprintf(p, "\"awb\":%u,", s->status.awb);
    p+=sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p+=sprintf(p, "\"aec\":%u,", s->status.aec);
    p+=sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p+=sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p+=sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p+=sprintf(p, "\"agc\":%u,", s->status.agc);
    p+=sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p+=sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p+=sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p+=sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p+=sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p+=sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p+=sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p+=sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p+=sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p+=sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

/*
 * 
 * 
 * 
 * FUNCION startCameraServerr
 * 
 * 
 * 
 */
//Inicie el servidor de video
void startCameraServer(){
    //https://github.com/espressif/esp-idf/blob/master/components/esp_http_server/include/esp_http_server.h
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();  //El puerto del servidor se puede configurar en HTTPD_DEFAULT_CONFIG ()
    //Ruta de URL personalizable correspondiente a la función ejecutada

    httpd_uri_t status_uri = {
        .uri       = "/status",       //http://192.168.xxx.xxx/status
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",      //http://192.168.xxx.xxx/control
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };


   httpd_uri_t stream_uri = {
        .uri       = "/stream",       //http://192.168.xxx.xxx:81/stream
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    Serial.printf("Starting web server on port: '%d'\n", config.server_port);  //TCP Port
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        //Registre una ruta URL personalizada correspondiente a la función ejecutada
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
    }

    config.server_port += 1;  //Stream Port
    config.ctrl_port += 1;  //UDP Port
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

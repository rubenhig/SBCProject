/*
ESP32-CAM CameraWebServer (No face detection)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-7-1 00:00
https://www.facebook.com/francefu

Face recognition works well in v1.0.5, v1.0.6 or above.


AP IP: 192.168.4.1
http://192.168.xxx.xxx             //Interfaz de gestión de la página de inicio
http://192.168.xxx.xxx:81/stream   //Obtener imagen de transmisión     Sintaxis de la página web <img src="http://192.168.xxx.xxx:81/stream">
http://192.168.xxx.xxx/capture     //Obtener la imagen           Sintaxis de la página web <img src="http://192.168.xxx.xxx/capture">
http://192.168.xxx.xxx/status      //Obtener el valor del parámetro de video

Establecer parámetros de video (formato de comando oficial)  http://192.168.xxx.xxx/control?var=*****&val=*****

http://192.168.xxx.xxx/control?var=flash&val=value          //閃光燈 value= 0~255
http://192.168.xxx.xxx/control?var=framesize&val=value      //解析度 value = 10->UXGA(1600x1200), 9->SXGA(1280x1024), 8->XGA(1024x768) ,7->SVGA(800x600), 6->VGA(640x480), 5 selected=selected->CIF(400x296), 4->QVGA(320x240), 3->HQVGA(240x176), 0->QQVGA(160x120), 11->QXGA(2048x1564 for OV3660)
http://192.168.xxx.xxx/control?var=quality&val=value        //畫質 value = 10 ~ 63
http://192.168.xxx.xxx/control?var=brightness&val=value     //亮度 value = -2 ~ 2
http://192.168.xxx.xxx/control?var=contrast&val=value       //對比 value = -2 ~ 2
http://192.168.xxx.xxx/control?var=saturation&val=value     //飽和度 value = -2 ~ 2 
http://192.168.xxx.xxx/control?var=gainceiling&val=value    //自動增益上限(開啟時) value = 0 ~ 6
http://192.168.xxx.xxx/control?var=colorbar&val=value       //顏色條畫面 value = 0 or 1
http://192.168.xxx.xxx/control?var=awb&val=value            //白平衡 value = 0 or 1 
http://192.168.xxx.xxx/control?var=agc&val=value            //自動增益控制 value = 0 or 1 
http://192.168.xxx.xxx/control?var=aec&val=value            //自動曝光感測器 value = 0 or 1 
http://192.168.xxx.xxx/control?var=hmirror&val=value        //水平鏡像 value = 0 or 1 
http://192.168.xxx.xxx/control?var=vflip&val=value          //垂直翻轉 value = 0 or 1 
http://192.168.xxx.xxx/control?var=awb_gain&val=value       //自動白平衡增益 value = 0 or 1 
http://192.168.xxx.xxx/control?var=agc_gain&val=value       //自動增益(關閉時) value = 0 ~ 30
http://192.168.xxx.xxx/control?var=aec_value&val=value      //曝光值 value = 0 ~ 1200
http://192.168.xxx.xxx/control?var=aec2&val=value           //自動曝光控制 value = 0 or 1 
http://192.168.xxx.xxx/control?var=dcw&val=value            //使用自訂影像尺寸 value = 0 or 1 
http://192.168.xxx.xxx/control?var=bpc&val=value            //黑色像素校正 value = 0 or 1 
http://192.168.xxx.xxx/control?var=wpc&val=value            //白色像素校正 value = 0 or 1 
http://192.168.xxx.xxx/control?var=raw_gma&val=value        //原始伽瑪 value = 0 or 1 
http://192.168.xxx.xxx/control?var=lenc&val=value           //鏡頭校正 value = 0 or 1 
http://192.168.xxx.xxx/control?var=special_effect&val=value //特效 value = 0 ~ 6
http://192.168.xxx.xxx/control?var=wb_mode&val=value        //白平衡模式 value = 0 ~ 4
http://192.168.xxx.xxx/control?var=ae_level&val=value       //自動曝光層級 value = -2 ~ 2 

Descripción del parámetro de video
https://heyrick.eu/blog/index.php?diary=20210418
*/

//輸入WIFI連線帳號密碼
const char* ssid = "TekiTeki";
const char* password = "officeconnect123";

//輸入AP端連線帳號密碼  http://192.168.4.1
const char* apssid = "esp32-cam";
const char* appassword = "12345678";         //AP密碼至少要8個字元以上  

#include "soc/soc.h"             //用於電源不穩不重開機 
#include "soc/rtc_cntl_reg.h"    //用於電源不穩不重開機 

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

  //Tamaño de fotograma de video predeterminado personalizable (tamaño de resolución)
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_CIF);    //解析度 UXGA(1600x1200), SXGA(1280x1024), XGA(1024x768), SVGA(800x600), VGA(640x480), CIF(400x296), QVGA(320x240), HQVGA(240x176), QQVGA(160x120), QXGA(2048x1564 for OV3660)

  //s->set_vflip(s, 1);  //Voltear verticalmente
  //s->set_hmirror(s, 1);  //Reflejo horizontal
          
  //destello(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8); 
  
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
      WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //Establecer SSID para mostrar la IP del cliente         
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
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);         

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
/*
 * 
 * 
 * 
 * FUNCION jpg_encode_stream
 * 
 * 
 * 
 */

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}
/*
 * 
 * 
 * 
 * FUNCION esp_err_t capture_handler
 * 
 * 
 * 
 */
//Captura de pantalla de la imagen
static esp_err_t capture_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t fb_len = 0;
    if(fb->format == PIXFORMAT_JPEG){
        fb_len = fb->len;
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
        jpg_chunking_t jchunk = {req, 0};
        res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
        httpd_resp_send_chunk(req, NULL, 0);
        fb_len = jchunk.len;
    }
    esp_camera_fb_return(fb);
    return res;
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
 * CONSTANTE index_ov3660_html[]
 * 
 * 
 * 
 */
//Variables de código de la página de inicio(ov3660)
static const char PROGMEM index_ov3660_html[] = R"rawliteral(<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>ESP32 OV3660</title>
        <style>
            body {
                font-family: Arial,Helvetica,sans-serif;
                background: #181818;
                color: #EFEFEF;
                font-size: 16px
            }

            h2 {
                font-size: 18px
            }

            section.main {
                display: flex
            }

            #menu,section.main {
                flex-direction: column
            }

            #menu {
                display: none;
                flex-wrap: nowrap;
                min-width: 340px;
                background: #363636;
                padding: 8px;
                border-radius: 4px;
                margin-top: -10px;
                margin-right: 10px;
            }

            #content {
                display: flex;
                flex-wrap: wrap;
                align-items: stretch
            }

            figure {
                padding: 0px;
                margin: 0;
                -webkit-margin-before: 0;
                margin-block-start: 0;
                -webkit-margin-after: 0;
                margin-block-end: 0;
                -webkit-margin-start: 0;
                margin-inline-start: 0;
                -webkit-margin-end: 0;
                margin-inline-end: 0
            }

            figure img {
                display: block;
                width: 100%;
                height: auto;
                border-radius: 4px;
                margin-top: 8px;
            }

            @media (min-width: 800px) and (orientation:landscape) {
                #content {
                    display:flex;
                    flex-wrap: nowrap;
                    align-items: stretch
                }

                figure img {
                    display: block;
                    max-width: 100%;
                    max-height: calc(100vh - 40px);
                    width: auto;
                    height: auto
                }

                figure {
                    padding: 0 0 0 0px;
                    margin: 0;
                    -webkit-margin-before: 0;
                    margin-block-start: 0;
                    -webkit-margin-after: 0;
                    margin-block-end: 0;
                    -webkit-margin-start: 0;
                    margin-inline-start: 0;
                    -webkit-margin-end: 0;
                    margin-inline-end: 0
                }
            }

            section#buttons {
                display: flex;
                flex-wrap: nowrap;
                justify-content: space-between
            }

            #nav-toggle {
                cursor: pointer;
                display: block
            }

            #nav-toggle-cb {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            #nav-toggle-cb:checked+#menu {
                display: flex
            }

            .input-group {
                display: flex;
                flex-wrap: nowrap;
                line-height: 22px;
                margin: 5px 0
            }

            .input-group>label {
                display: inline-block;
                padding-right: 10px;
                min-width: 47%
            }

            .input-group input,.input-group select {
                flex-grow: 1
            }

            .range-max,.range-min {
                display: inline-block;
                padding: 0 5px
            }

            button {
                display: block;
                margin: 5px;
                padding: 0 12px;
                border: 0;
                line-height: 28px;
                cursor: pointer;
                color: #fff;
                background: #ff3034;
                border-radius: 5px;
                font-size: 16px;
                outline: 0
            }

            button:hover {
                background: #ff494d
            }

            button:active {
                background: #f21c21
            }

            button.disabled {
                cursor: default;
                background: #a0a0a0
            }

            input[type=range] {
                -webkit-appearance: none;
                width: 100%;
                height: 22px;
                background: #363636;
                cursor: pointer;
                margin: 0
            }

            input[type=range]:focus {
                outline: 0
            }

            input[type=range]::-webkit-slider-runnable-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-webkit-slider-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                -webkit-appearance: none;
                margin-top: -11.5px
            }

            input[type=range]:focus::-webkit-slider-runnable-track {
                background: #EFEFEF
            }

            input[type=range]::-moz-range-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-moz-range-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer
            }

            input[type=range]::-ms-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: 0 0;
                border-color: transparent;
                color: transparent
            }

            input[type=range]::-ms-fill-lower {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-fill-upper {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                height: 2px
            }

            input[type=range]:focus::-ms-fill-lower {
                background: #EFEFEF
            }

            input[type=range]:focus::-ms-fill-upper {
                background: #363636
            }

            .switch {
                display: block;
                position: relative;
                line-height: 22px;
                font-size: 16px;
                height: 22px
            }

            .switch input {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            .slider {
                width: 50px;
                height: 22px;
                border-radius: 22px;
                cursor: pointer;
                background-color: grey
            }

            .slider,.slider:before {
                display: inline-block;
                transition: .4s
            }

            .slider:before {
                position: relative;
                content: "";
                border-radius: 50%;
                height: 16px;
                width: 16px;
                left: 4px;
                top: 3px;
                background-color: #fff
            }

            input:checked+.slider {
                background-color: #ff3034
            }

            input:checked+.slider:before {
                -webkit-transform: translateX(26px);
                transform: translateX(26px)
            }

            select {
                border: 1px solid #363636;
                font-size: 14px;
                height: 22px;
                outline: 0;
                border-radius: 5px
            }

            .image-container {
                position: relative;
                min-width: 160px
            }

            .close {
                position: absolute;
                right: 5px;
                top: 5px;
                background: #ff3034;
                width: 16px;
                height: 16px;
                border-radius: 100px;
                color: #fff;
                text-align: center;
                line-height: 18px;
                cursor: pointer
            }

            .hidden {
                display: none
            }

            input[type=text] {
                border: 1px solid #363636;
                font-size: 14px;
                height: 20px;
                margin: 1px;
                outline: 0;
                border-radius: 5px
            }

            .inline-button {
                line-height: 20px;
                margin: 2px;
                padding: 1px 4px 2px 4px;
            }

        </style>
    </head>
    <body>
        <figure>
            <div id="stream-container" class="image-container hidden">
                <div class="close" id="close-stream">×</div>
                <img id="stream" src="">
            </div>
        </figure>        
        <section class="main">
            <div id="logo">
                <label for="nav-toggle-cb" id="nav-toggle">&#9776;&nbsp;&nbsp;Toggle OV3660 settings</label>
            </div>
            <div id="content">
                <div id="sidebar">
                    <input type="checkbox" id="nav-toggle-cb" checked="checked">
                    <nav id="menu">
                        <section id="buttons">
                            <button id="get-still">Get Still</button>
                            <button id="toggle-stream">Start Stream</button>
                        </section>                    
                        <div class="input-group" id="framesize-group">
                            <label for="framesize">Resolution</label>
                            <select id="framesize" class="default-action">
                                <option value="11">QXGA(2048x1564)</option>
                                <option value="10">UXGA(1600x1200)</option>
                                <option value="9">SXGA(1280x1024)</option>
                                <option value="8">XGA(1024x768)</option>
                                <option value="7">SVGA(800x600)</option>
                                <option value="6">VGA(640x480)</option>
                                <option value="5" selected="selected">CIF(400x296)</option>
                                <option value="4">QVGA(320x240)</option>
                                <option value="3">HQVGA(240x176)</option>
                                <option value="0">QQVGA(160x120)</option>
                            </select>
                        </div>
                        <div class="input-group" id="quality-group">
                            <label for="quality">Quality</label>
                            <div class="range-min">4</div>
                            <input type="range" id="quality" min="4" max="63" value="10" class="default-action">
                            <div class="range-max">63</div>
                        </div>
                        <div class="input-group" id="brightness-group">
                            <label for="brightness">Brightness</label>
                            <div class="range-min">-3</div>
                            <input type="range" id="brightness" min="-3" max="3" value="0" class="default-action">
                            <div class="range-max">3</div>
                        </div>
                        <div class="input-group" id="contrast-group">
                            <label for="contrast">Contrast</label>
                            <div class="range-min">-3</div>
                            <input type="range" id="contrast" min="-3" max="3" value="0" class="default-action">
                            <div class="range-max">3</div>
                        </div>
                        <div class="input-group" id="saturation-group">
                            <label for="saturation">Saturation</label>
                            <div class="range-min">-4</div>
                            <input type="range" id="saturation" min="-4" max="4" value="0" class="default-action">
                            <div class="range-max">4</div>
                        </div>
                        <div class="input-group" id="sharpness-group">
                            <label for="sharpness">Sharpness</label>
                            <div class="range-min">-3</div>
                            <input type="range" id="sharpness" min="-3" max="3" value="0" class="default-action">
                            <div class="range-max">3</div>
                        </div>
                        <div class="input-group" id="denoise-group">
                            <label for="denoise">De-Noise</label>
                            <div class="range-min">Auto</div>
                            <input type="range" id="denoise" min="0" max="8" value="0" class="default-action">
                            <div class="range-max">8</div>
                        </div>
                        <div class="input-group" id="ae_level-group">
                            <label for="ae_level">Exposure Level</label>
                            <div class="range-min">-5</div>
                            <input type="range" id="ae_level" min="-5" max="5" value="0" class="default-action">
                            <div class="range-max">5</div>
                        </div>
                        <div class="input-group" id="gainceiling-group">
                            <label for="gainceiling">Gainceiling</label>
                            <div class="range-min">0</div>
                            <input type="range" id="gainceiling" min="0" max="511" value="0" class="default-action">
                            <div class="range-max">511</div>
                        </div>
                        <div class="input-group" id="special_effect-group">
                            <label for="special_effect">Special Effect</label>
                            <select id="special_effect" class="default-action">
                                <option value="0" selected="selected">No Effect</option>
                                <option value="1">Negative</option>
                                <option value="2">Grayscale</option>
                                <option value="3">Red Tint</option>
                                <option value="4">Green Tint</option>
                                <option value="5">Blue Tint</option>
                                <option value="6">Sepia</option>
                            </select>
                        </div>
                        <div class="input-group" id="awb-group">
                            <label for="awb">AWB Enable</label>
                            <div class="switch">
                                <input id="awb" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="awb"></label>
                            </div>
                        </div>
                        <div class="input-group" id="dcw-group">
                            <label for="dcw">Advanced AWB</label>
                            <div class="switch">
                                <input id="dcw" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="dcw"></label>
                            </div>
                        </div>
                        <div class="input-group" id="awb_gain-group">
                            <label for="awb_gain">Manual AWB</label>
                            <div class="switch">
                                <input id="awb_gain" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="awb_gain"></label>
                            </div>
                        </div>
                        <div class="input-group" id="wb_mode-group">
                            <label for="wb_mode">AWB Mode</label>
                            <select id="wb_mode" class="default-action">
                                <option value="0" selected="selected">Auto</option>
                                <option value="1">Sunny</option>
                                <option value="2">Cloudy</option>
                                <option value="3">Office</option>
                                <option value="4">Home</option>
                            </select>
                        </div>
                        <div class="input-group" id="aec-group">
                            <label for="aec">AEC Enable</label>
                            <div class="switch">
                                <input id="aec" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="aec"></label>
                            </div>
                        </div>
                        <div class="input-group" id="aec_value-group">
                            <label for="aec_value">Manual Exposure</label>
                            <div class="range-min">0</div>
                            <input type="range" id="aec_value" min="0" max="1536" value="320" class="default-action">
                            <div class="range-max">1536</div>
                        </div>
                        <div class="input-group" id="aec2-group">
                            <label for="aec2">Night Mode</label>
                            <div class="switch">
                                <input id="aec2" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="aec2"></label>
                            </div>
                        </div>
                        <div class="input-group" id="agc-group">
                            <label for="agc">AGC</label>
                            <div class="switch">
                                <input id="agc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="agc"></label>
                            </div>
                        </div>
                        <div class="input-group hidden" id="agc_gain-group">
                            <label for="agc_gain">Gain</label>
                            <div class="range-min">1x</div>
                            <input type="range" id="agc_gain" min="0" max="64" value="5" class="default-action">
                            <div class="range-max">64x</div>
                        </div>
                        <div class="input-group" id="raw_gma-group">
                            <label for="raw_gma">GMA Enable</label>
                            <div class="switch">
                                <input id="raw_gma" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="raw_gma"></label>
                            </div>
                        </div>
                        <div class="input-group" id="lenc-group">
                            <label for="lenc">Lens Correction</label>
                            <div class="switch">
                                <input id="lenc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="lenc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="hmirror-group">
                            <label for="hmirror">H-Mirror</label>
                            <div class="switch">
                                <input id="hmirror" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="hmirror"></label>
                            </div>
                        </div>
                        <div class="input-group" id="vflip-group">
                            <label for="vflip">V-Flip</label>
                            <div class="switch">
                                <input id="vflip" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="vflip"></label>
                            </div>
                        </div>
                        <div class="input-group" id="bpc-group">
                            <label for="bpc">BPC</label>
                            <div class="switch">
                                <input id="bpc" type="checkbox" class="default-action">
                                <label class="slider" for="bpc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="wpc-group">
                            <label for="wpc">WPC</label>
                            <div class="switch">
                                <input id="wpc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="wpc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="colorbar-group">
                            <label for="colorbar">Color Bar</label>
                            <div class="switch">
                                <input id="colorbar" type="checkbox" class="default-action">
                                <label class="slider" for="colorbar"></label>
                            </div>
                        </div>
                    </nav>
                </div>
            </div>
        </section>
        <script>
document.addEventListener('DOMContentLoaded', function (event) {
  var baseHost = document.location.origin
  var streamUrl = baseHost + ':81'

  const hide = el => {
    el.classList.add('hidden')
  }
  const show = el => {
    el.classList.remove('hidden')
  }

  const disable = el => {
    el.classList.add('disabled')
    el.disabled = true
  }

  const enable = el => {
    el.classList.remove('disabled')
    el.disabled = false
  }

  const updateValue = (el, value, updateRemote) => {
    updateRemote = updateRemote == null ? true : updateRemote
    let initialValue
    if (el.type === 'checkbox') {
      initialValue = el.checked
      value = !!value
      el.checked = value
    } else {
      initialValue = el.value
      el.value = value
    }

    if (updateRemote && initialValue !== value) {
      updateConfig(el);
    } else if(!updateRemote){
      if(el.id === "aec"){
        value ? hide(exposure) : show(exposure)
      } else if(el.id === "agc"){
        if (value) {
          hide(agcGain)
        } else {
          show(agcGain)
        }
      } else if(el.id === "awb_gain"){
        value ? show(wb) : hide(wb)
      }
    }
  }

  function updateConfig (el) {
    let value
    switch (el.type) {
      case 'checkbox':
        value = el.checked ? 1 : 0
        break
      case 'range':
      case 'select-one':
        value = el.value
        break
      case 'button':
      case 'submit':
        value = '1'
        break
      default:
        return
    }

    const query = `${baseHost}/control?var=${el.id}&val=${value}`

    fetch(query)
      .then(response => {
        console.log(`request to ${query} finished, status: ${response.status}`)
      })
  }

  document
    .querySelectorAll('.close')
    .forEach(el => {
      el.onclick = () => {
        hide(el.parentNode)
      }
    })

  // read initial values
  fetch(`${baseHost}/status`)
    .then(function (response) {
      return response.json()
    })
    .then(function (state) {
      document
        .querySelectorAll('.default-action')
        .forEach(el => {
            updateValue(el, state[el.id], false)
        })
    })

  const view = document.getElementById('stream')
  const viewContainer = document.getElementById('stream-container')
  const stillButton = document.getElementById('get-still')
  const streamButton = document.getElementById('toggle-stream')
  const closeButton = document.getElementById('close-stream')

  const stopStream = () => {
    window.stop();
    streamButton.innerHTML = 'Start Stream'
  }

  const startStream = () => {
    view.src = `${streamUrl}/stream`
    show(viewContainer)
    streamButton.innerHTML = 'Stop Stream'
  }

  // Attach actions to buttons
  stillButton.onclick = () => {
    stopStream()
    view.src = `${baseHost}/capture?_cb=${Date.now()}`
    show(viewContainer)
  }

  closeButton.onclick = () => {
    stopStream()
    hide(viewContainer)
  }

  streamButton.onclick = () => {
    const streamEnabled = streamButton.innerHTML === 'Stop Stream'
    if (streamEnabled) {
      stopStream()
    } else {
      startStream()
    }
  }

  // Attach default on change action
  document
    .querySelectorAll('.default-action')
    .forEach(el => {
      el.onchange = () => updateConfig(el)
    })

  // Custom actions
  // Gain
  const agc = document.getElementById('agc')
  const agcGain = document.getElementById('agc_gain-group')
  agc.onchange = () => {
    updateConfig(agc)
    if (agc.checked) {
      hide(agcGain)
    } else {
      show(agcGain)
    }
  }

  // Exposure
  const aec = document.getElementById('aec')
  const exposure = document.getElementById('aec_value-group')
  aec.onchange = () => {
    updateConfig(aec)
    aec.checked ? hide(exposure) : show(exposure)
  }

  // AWB
  const awb = document.getElementById('awb_gain')
  const wb = document.getElementById('wb_mode-group')
  awb.onchange = () => {
    updateConfig(awb)
    awb.checked ? show(wb) : hide(wb)
  }

  // framesize
  const framesize = document.getElementById('framesize')

  framesize.onchange = () => {
    updateConfig(framesize)
  }
})
        </script>
    </body>
</html>
)rawliteral";
        
/*
 * 
 * 
 * 
 * CONSTANTE index_ov2640_html[]
 * 
 * 
 * 
 */
//Variables de código de la página de inicio (ov2640)
static const char PROGMEM index_ov2640_html[] = R"rawliteral(<!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width,initial-scale=1">
        <title>ESP32 OV2460</title>
        <style>
            body {
                font-family: Arial,Helvetica,sans-serif;
                background: #181818;
                color: #EFEFEF;
                font-size: 16px
            }

            h2 {
                font-size: 18px
            }

            section.main {
                display: flex
            }

            #menu,section.main {
                flex-direction: column
            }

            #menu {
                display: none;
                flex-wrap: nowrap;
                min-width: 340px;
                background: #363636;
                padding: 8px;
                border-radius: 4px;
                margin-top: -10px;
                margin-right: 10px;
            }

            #content {
                display: flex;
                flex-wrap: wrap;
                align-items: stretch
            }

            figure {
                padding: 0px;
                margin: 0;
                -webkit-margin-before: 0;
                margin-block-start: 0;
                -webkit-margin-after: 0;
                margin-block-end: 0;
                -webkit-margin-start: 0;
                margin-inline-start: 0;
                -webkit-margin-end: 0;
                margin-inline-end: 0
            }

            figure img {
                display: block;
                width: 100%;
                height: auto;
                border-radius: 4px;
                margin-top: 8px;
            }

            @media (min-width: 800px) and (orientation:landscape) {
                #content {
                    display:flex;
                    flex-wrap: nowrap;
                    align-items: stretch
                }

                figure img {
                    display: block;
                    max-width: 100%;
                    max-height: calc(100vh - 40px);
                    width: auto;
                    height: auto
                }

                figure {
                    padding: 0 0 0 0px;
                    margin: 0;
                    -webkit-margin-before: 0;
                    margin-block-start: 0;
                    -webkit-margin-after: 0;
                    margin-block-end: 0;
                    -webkit-margin-start: 0;
                    margin-inline-start: 0;
                    -webkit-margin-end: 0;
                    margin-inline-end: 0
                }
            }

            section#buttons {
                display: flex;
                flex-wrap: nowrap;
                justify-content: space-between
            }

            #nav-toggle {
                cursor: pointer;
                display: block
            }

            #nav-toggle-cb {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            #nav-toggle-cb:checked+#menu {
                display: flex
            }

            .input-group {
                display: flex;
                flex-wrap: nowrap;
                line-height: 22px;
                margin: 5px 0
            }

            .input-group>label {
                display: inline-block;
                padding-right: 10px;
                min-width: 47%
            }

            .input-group input,.input-group select {
                flex-grow: 1
            }

            .range-max,.range-min {
                display: inline-block;
                padding: 0 5px
            }

            button {
                display: block;
                margin: 5px;
                padding: 0 12px;
                border: 0;
                line-height: 28px;
                cursor: pointer;
                color: #fff;
                background: #ff3034;
                border-radius: 5px;
                font-size: 16px;
                outline: 0
            }

            button:hover {
                background: #ff494d
            }

            button:active {
                background: #f21c21
            }

            button.disabled {
                cursor: default;
                background: #a0a0a0
            }

            input[type=range] {
                -webkit-appearance: none;
                width: 100%;
                height: 22px;
                background: #363636;
                cursor: pointer;
                margin: 0
            }

            input[type=range]:focus {
                outline: 0
            }

            input[type=range]::-webkit-slider-runnable-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-webkit-slider-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                -webkit-appearance: none;
                margin-top: -11.5px
            }

            input[type=range]:focus::-webkit-slider-runnable-track {
                background: #EFEFEF
            }

            input[type=range]::-moz-range-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: #EFEFEF;
                border-radius: 0;
                border: 0 solid #EFEFEF
            }

            input[type=range]::-moz-range-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer
            }

            input[type=range]::-ms-track {
                width: 100%;
                height: 2px;
                cursor: pointer;
                background: 0 0;
                border-color: transparent;
                color: transparent
            }

            input[type=range]::-ms-fill-lower {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-fill-upper {
                background: #EFEFEF;
                border: 0 solid #EFEFEF;
                border-radius: 0
            }

            input[type=range]::-ms-thumb {
                border: 1px solid rgba(0,0,30,0);
                height: 22px;
                width: 22px;
                border-radius: 50px;
                background: #ff3034;
                cursor: pointer;
                height: 2px
            }

            input[type=range]:focus::-ms-fill-lower {
                background: #EFEFEF
            }

            input[type=range]:focus::-ms-fill-upper {
                background: #363636
            }

            .switch {
                display: block;
                position: relative;
                line-height: 22px;
                font-size: 16px;
                height: 22px
            }

            .switch input {
                outline: 0;
                opacity: 0;
                width: 0;
                height: 0
            }

            .slider {
                width: 50px;
                height: 22px;
                border-radius: 22px;
                cursor: pointer;
                background-color: grey
            }

            .slider,.slider:before {
                display: inline-block;
                transition: .4s
            }

            .slider:before {
                position: relative;
                content: "";
                border-radius: 50%;
                height: 16px;
                width: 16px;
                left: 4px;
                top: 3px;
                background-color: #fff
            }

            input:checked+.slider {
                background-color: #ff3034
            }

            input:checked+.slider:before {
                -webkit-transform: translateX(26px);
                transform: translateX(26px)
            }

            select {
                border: 1px solid #363636;
                font-size: 14px;
                height: 22px;
                outline: 0;
                border-radius: 5px
            }

            .image-container {
                position: relative;
                min-width: 160px
            }

            .close {
                position: absolute;
                right: 5px;
                top: 5px;
                background: #ff3034;
                width: 16px;
                height: 16px;
                border-radius: 100px;
                color: #fff;
                text-align: center;
                line-height: 18px;
                cursor: pointer
            }

            .hidden {
                display: none
            }
        </style>
    </head>
    <body>
        <figure>
            <div id="stream-container" class="image-container hidden">
                <div class="close" id="close-stream">×</div>
                <img id="stream" src="">
            </div>
        </figure>     
        <section class="main">
            <div id="logo">
                <label for="nav-toggle-cb" id="nav-toggle">&#9776;&nbsp;&nbsp;Toggle OV2640 settings</label>
            </div>
            <div id="content">
                <div id="sidebar">
                    <input type="checkbox" id="nav-toggle-cb" checked="checked">
                    <nav id="menu">
                        <section id="buttons">
                            <button id="get-still">Get Still</button>
                            <button id="toggle-stream">Start Stream</button>
                        </section>                    
                        <div class="input-group" id="framesize-group">
                            <label for="framesize">Resolution</label>
                            <select id="framesize" class="default-action">
                                <option value="10">UXGA(1600x1200)</option>
                                <option value="9">SXGA(1280x1024)</option>
                                <option value="8">XGA(1024x768)</option>
                                <option value="7">SVGA(800x600)</option>
                                <option value="6">VGA(640x480)</option>
                                <option value="5" selected="selected">CIF(400x296)</option>
                                <option value="4">QVGA(320x240)</option>
                                <option value="3">HQVGA(240x176)</option>
                                <option value="0">QQVGA(160x120)</option>
                            </select>
                        </div>
                        <div class="input-group" id="quality-group">
                            <label for="quality">Quality</label>
                            <div class="range-min">10</div>
                            <input type="range" id="quality" min="10" max="63" value="10" class="default-action">
                            <div class="range-max">63</div>
                        </div>
                        <div class="input-group" id="brightness-group">
                            <label for="brightness">Brightness</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="brightness" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="contrast-group">
                            <label for="contrast">Contrast</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="contrast" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="saturation-group">
                            <label for="saturation">Saturation</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="saturation" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="special_effect-group">
                            <label for="special_effect">Special Effect</label>
                            <select id="special_effect" class="default-action">
                                <option value="0" selected="selected">No Effect</option>
                                <option value="1">Negative</option>
                                <option value="2">Grayscale</option>
                                <option value="3">Red Tint</option>
                                <option value="4">Green Tint</option>
                                <option value="5">Blue Tint</option>
                                <option value="6">Sepia</option>
                            </select>
                        </div>
                        <div class="input-group" id="awb-group">
                            <label for="awb">AWB</label>
                            <div class="switch">
                                <input id="awb" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="awb"></label>
                            </div>
                        </div>
                        <div class="input-group" id="awb_gain-group">
                            <label for="awb_gain">AWB Gain</label>
                            <div class="switch">
                                <input id="awb_gain" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="awb_gain"></label>
                            </div>
                        </div>
                        <div class="input-group" id="wb_mode-group">
                            <label for="wb_mode">WB Mode</label>
                            <select id="wb_mode" class="default-action">
                                <option value="0" selected="selected">Auto</option>
                                <option value="1">Sunny</option>
                                <option value="2">Cloudy</option>
                                <option value="3">Office</option>
                                <option value="4">Home</option>
                            </select>
                        </div>
                        <div class="input-group" id="aec-group">
                            <label for="aec">AEC SENSOR</label>
                            <div class="switch">
                                <input id="aec" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="aec"></label>
                            </div>
                        </div>
                        <div class="input-group" id="aec2-group">
                            <label for="aec2">AEC DSP</label>
                            <div class="switch">
                                <input id="aec2" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="aec2"></label>
                            </div>
                        </div>
                        <div class="input-group" id="ae_level-group">
                            <label for="ae_level">AE Level</label>
                            <div class="range-min">-2</div>
                            <input type="range" id="ae_level" min="-2" max="2" value="0" class="default-action">
                            <div class="range-max">2</div>
                        </div>
                        <div class="input-group" id="aec_value-group">
                            <label for="aec_value">Exposure</label>
                            <div class="range-min">0</div>
                            <input type="range" id="aec_value" min="0" max="1200" value="204" class="default-action">
                            <div class="range-max">1200</div>
                        </div>
                        <div class="input-group" id="agc-group">
                            <label for="agc">AGC</label>
                            <div class="switch">
                                <input id="agc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="agc"></label>
                            </div>
                        </div>
                        <div class="input-group hidden" id="agc_gain-group">
                            <label for="agc_gain">Gain</label>
                            <div class="range-min">1x</div>
                            <input type="range" id="agc_gain" min="0" max="30" value="5" class="default-action">
                            <div class="range-max">31x</div>
                        </div>
                        <div class="input-group" id="gainceiling-group">
                            <label for="gainceiling">Gain Ceiling</label>
                            <div class="range-min">2x</div>
                            <input type="range" id="gainceiling" min="0" max="6" value="0" class="default-action">
                            <div class="range-max">128x</div>
                        </div>
                        <div class="input-group" id="bpc-group">
                            <label for="bpc">BPC</label>
                            <div class="switch">
                                <input id="bpc" type="checkbox" class="default-action">
                                <label class="slider" for="bpc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="wpc-group">
                            <label for="wpc">WPC</label>
                            <div class="switch">
                                <input id="wpc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="wpc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="raw_gma-group">
                            <label for="raw_gma">Raw GMA</label>
                            <div class="switch">
                                <input id="raw_gma" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="raw_gma"></label>
                            </div>
                        </div>
                        <div class="input-group" id="lenc-group">
                            <label for="lenc">Lens Correction</label>
                            <div class="switch">
                                <input id="lenc" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="lenc"></label>
                            </div>
                        </div>
                        <div class="input-group" id="hmirror-group">
                            <label for="hmirror">H-Mirror</label>
                            <div class="switch">
                                <input id="hmirror" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="hmirror"></label>
                            </div>
                        </div>
                        <div class="input-group" id="vflip-group">
                            <label for="vflip">V-Flip</label>
                            <div class="switch">
                                <input id="vflip" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="vflip"></label>
                            </div>
                        </div>
                        <div class="input-group" id="dcw-group">
                            <label for="dcw">DCW (Downsize EN)</label>
                            <div class="switch">
                                <input id="dcw" type="checkbox" class="default-action" checked="checked">
                                <label class="slider" for="dcw"></label>
                            </div>
                        </div>
                        <div class="input-group" id="colorbar-group">
                            <label for="colorbar">Color Bar</label>
                            <div class="switch">
                                <input id="colorbar" type="checkbox" class="default-action">
                                <label class="slider" for="colorbar"></label>
                            </div>
                        </div>
                    </nav>
                </div>
            </div>
        </section>
        <script>
document.addEventListener('DOMContentLoaded', function (event) {
  var baseHost = document.location.origin
  var streamUrl = baseHost + ':81'

  const hide = el => {
    el.classList.add('hidden')
  }
  const show = el => {
    el.classList.remove('hidden')
  }

  const disable = el => {
    el.classList.add('disabled')
    el.disabled = true
  }

  const enable = el => {
    el.classList.remove('disabled')
    el.disabled = false
  }

  const updateValue = (el, value, updateRemote) => {
    updateRemote = updateRemote == null ? true : updateRemote
    let initialValue
    if (el.type === 'checkbox') {
      initialValue = el.checked
      value = !!value
      el.checked = value
    } else {
      initialValue = el.value
      el.value = value
    }

    if (updateRemote && initialValue !== value) {
      updateConfig(el);
    } else if(!updateRemote){
      if(el.id === "aec"){
        value ? hide(exposure) : show(exposure)
      } else if(el.id === "agc"){
        if (value) {
          show(gainCeiling)
          hide(agcGain)
        } else {
          hide(gainCeiling)
          show(agcGain)
        }
      } else if(el.id === "awb_gain"){
        value ? show(wb) : hide(wb)
      }
    }
  }

  function updateConfig (el) {
    let value
    switch (el.type) {
      case 'checkbox':
        value = el.checked ? 1 : 0
        break
      case 'range':
      case 'select-one':
        value = el.value
        break
      case 'button':
      case 'submit':
        value = '1'
        break
      default:
        return
    }

    const query = `${baseHost}/control?var=${el.id}&val=${value}`

    fetch(query)
      .then(response => {
        console.log(`request to ${query} finished, status: ${response.status}`)
      })
  }

  document
    .querySelectorAll('.close')
    .forEach(el => {
      el.onclick = () => {
        hide(el.parentNode)
      }
    })

  // read initial values
  fetch(`${baseHost}/status`)
    .then(function (response) {
      return response.json()
    })
    .then(function (state) {
      document
        .querySelectorAll('.default-action')
        .forEach(el => {
          updateValue(el, state[el.id], false)
        })
    })

  const view = document.getElementById('stream')
  const viewContainer = document.getElementById('stream-container')
  const stillButton = document.getElementById('get-still')
  const streamButton = document.getElementById('toggle-stream')
  const closeButton = document.getElementById('close-stream')

  const stopStream = () => {
    window.stop();
    streamButton.innerHTML = 'Start Stream'
  }

  const startStream = () => {
    view.src = `${streamUrl}/stream`
    show(viewContainer)
    streamButton.innerHTML = 'Stop Stream'
  }

  // Attach actions to buttons
  stillButton.onclick = () => {
    stopStream()
    view.src = `${baseHost}/capture?_cb=${Date.now()}`
    show(viewContainer)
  }

  closeButton.onclick = () => {
    stopStream()
    hide(viewContainer)
  }

  streamButton.onclick = () => {
    const streamEnabled = streamButton.innerHTML === 'Stop Stream'
    if (streamEnabled) {
      stopStream()
    } else {
      startStream()
    }
  }

  // Attach default on change action
  document
    .querySelectorAll('.default-action')
    .forEach(el => {
      el.onchange = () => updateConfig(el)
    })

  // Custom actions
  // Gain
  const agc = document.getElementById('agc')
  const agcGain = document.getElementById('agc_gain-group')
  const gainCeiling = document.getElementById('gainceiling-group')
  agc.onchange = () => {
    updateConfig(agc)
    if (agc.checked) {
      show(gainCeiling)
      hide(agcGain)
    } else {
      hide(gainCeiling)
      show(agcGain)
    }
  }

  // Exposure
  const aec = document.getElementById('aec')
  const exposure = document.getElementById('aec_value-group')
  aec.onchange = () => {
    updateConfig(aec)
    aec.checked ? hide(exposure) : show(exposure)
  }

  // AWB
  const awb = document.getElementById('awb_gain')
  const wb = document.getElementById('wb_mode-group')
  awb.onchange = () => {
    updateConfig(awb)
    awb.checked ? show(wb) : hide(wb)
  }

  // framesize
  const framesize = document.getElementById('framesize')

  framesize.onchange = () => {
    updateConfig(framesize)
  }
})

        </script>
    </body>
</html>)rawliteral";
/*
 * 
 * 
 * 
 * FUNCION esp_err_t index_handler
 * 
 * 
 * 
 */
//Controlador de página de inicio  http://192.168.xxx.xxx
static esp_err_t index_handler(httpd_req_t *req){
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    sensor_t * s = esp_camera_sensor_get();

    if (s->id.PID == OV3660_PID) {
        return httpd_resp_send(req, (const char *)index_ov3660_html, strlen(index_ov3660_html));
    } else {
        return httpd_resp_send(req, (const char *)index_ov2640_html, strlen(index_ov2640_html));
    }
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
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();  //可在HTTPD_DEFAULT_CONFIG()中設定Server Port

    //Ruta de URL personalizable correspondiente a la función ejecutada
    httpd_uri_t index_uri = {
        .uri       = "/",             //http://192.168.xxx.xxx/
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

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

    httpd_uri_t capture_uri = {
        .uri       = "/capture",      //http://192.168.xxx.xxx/capture
        .method    = HTTP_GET,
        .handler   = capture_handler,
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
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
    }

    config.server_port += 1;  //Stream Port
    config.ctrl_port += 1;  //UDP Port
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

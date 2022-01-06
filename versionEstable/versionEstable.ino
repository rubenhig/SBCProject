#include <svm30.h> //LIBRERIA MODIFICADA POR ERROR DE LA TRAMA CRC I2C 

//librerias:
#include <WiFi.h>
#include <ESPmDNS.h> 
#include <ArduinoOTA.h>

//librerias: 
#include <LiquidCrystal_I2C.h>
#include <Wire.h>


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////DECLARACION DE VARIABLES////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//I2C//
#define I2C_SDA 16
#define I2C_SCL 0

//WIFI//
const char* ssid = "TekiTeki";
const char* password = "officeconnect123";
IPAddress ip(192,168,1,200);     
IPAddress gateway(192,168,1,1);   
IPAddress subnet(255,255,255,0);

//DISPLAY//
LiquidCrystal_I2C lcd(0x27, 16, 2);

//CO2 SENSOR//
TwoWire i2c(0);
SVM30 svm;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////DECLARACION DE FUNCIONES////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setLCDI2C();
void setWiFiOTA();
void co2sensorSetup();
void scanI2C();
void readCO2();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////COMIENZO DEL PROGRAMA///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  i2c.begin(I2C_SDA, I2C_SCL);
  setLCDI2C();
  setWiFiOTA();   
  co2sensorSetup();
  Serial.print(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("RUBEN HIGUERA");
  delay(1000);
  
}


void loop() {
  ArduinoOTA.handle();
  //scanI2C();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("leyendo datos...");
  readCO2();
  delay(2000);
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////CUERPO DE LAS FUNCIONES/////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setLCDI2C() { 
  // Inicializar el LCD
  lcd.init();
  
  //Encender la luz de fondo.
  lcd.backlight();
}

//setup:
void setWiFiOTA(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Booting");
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Conn Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.begin();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP:");
  lcd.println(WiFi.localIP());
  delay(2000);
}

void co2sensorSetup() {
  svm.EnableDebugging(false);
  svm.begin(&i2c);
}

void scanI2C() {
  byte error, address;
  int nDevices;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scanning...");
  delay(1000);

  nDevices = 0;
  lcd.setCursor(0,1);
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      lcd.print("0x");
      if (address<16)
        lcd.print("0");
      lcd.print(address,HEX);
      lcd.print(" ");
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // wait 5 seconds for next scan
}

void readCO2() {
  struct svm_values v;
  lcd.setCursor(0,1);
  if(!svm.GetValues(&v)) lcd.print("error");
  lcd.print("T:");
  lcd.print(String((float) v.temperature/1000));
  lcd.print("CO2:");
  lcd.print(String(v.CO2eq));
}

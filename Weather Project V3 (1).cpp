#include <M5Core2.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "EGR425_Phase1_weather_bitmap_images.h"
#include "WiFi.h"
#include <string>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_VCNL4040.h>
#include <Adafruit_SHT4x.h>
#include "../include/I2C_RW.h"

////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////
// TODO 3: Register for openweather account and get API key
String urlOpenWeather = "https://api.openweathermap.org/data/2.5/weather?";
String apiKey = "29a54537e840e47cdb0839988a0a94b2";

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_VCNL4040 vcnl4040 = Adafruit_VCNL4040();

int const PIN_SDA = 32;
int const PIN_SCL = 33;

///////////////////////////////////////////////////////////////
// Register defines
///////////////////////////////////////////////////////////////
// #define VCNL_I2C_ADDRESS 0x60
// #define VCNL_REG_PROX_DATA 0x08
// #define VCNL_REG_ALS_DATA 0x09
// #define VCNL_REG_WHITE_DATA 0x0A

// #define VCNL_REG_PS_CONFIG 0x03
// #define VCNL_REG_ALS_CONFIG 0x00
// #define VCNL_REG_WHITE_CONFIG 0x04

// demo for zipcode
// TODOz use the following to help...
// something is wrong with the zipcode version of whether, it seems to have really high temperature values
// https://api.openweathermap.org/data/2.5/weather?zip=92879,&appid=29a54537e840e47cdb0839988a0a94b2

// TODO 1: WiFi variables
String wifiNetworkName = "SteelersNation-5G";
String wifiPassword = "9b299822";

// Time variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000; // 5000; 5 minutes (300,000ms) or 5 seconds (5,000ms)

// clock for TODO 2
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "us.pool.ntp.org", -28800);
String timeStamp;
// String timeClient.getFormattedTime();

// LCD variables
int sWidth;
int sHeight;

// Weather/zip variables
String strWeatherIcon;
String strWeatherDesc;
String cityName;
double tempNow;
double tempMin;
double tempMax;

/////////////////////////////////////////
// TODOz make variables for celsius based on the original temperatures
// they will be updated at the same time with the original temps
double tempNowC;
double tempMinC;
double tempMaxC;

//////////////////////////////////////////////
// booleans to record whether each button was pushed
boolean btnPressed = true;
boolean btnBPressed = false;
boolean btnCPressed = false;
boolean isF = false;
boolean isChanged = false;

////////////////////////////////////////////
// variable for the zip code
int zip[5] = {9, 2, 5, 0, 4};
String zipStr;

////////////////////////////////////////////
// touch button conf
// Button z1Inc(10 * 5, M5.lcd.height() / 2 - 30, 20, 20, "left-top");
Button dc1Inc(20, 60, 35, 40, "1up");
Button dc2Inc(80, 60, 35, 40, "2up");
Button dc3Inc(140, 60, 35, 40, "3up");
Button dc4Inc(200, 60, 35, 40, "4up");
Button dc5Inc(260, 60, 35, 40, "5up");
Button dc1Dec(20, 160, 35, 40, "1down");
Button dc2Dec(80, 160, 35, 40, "2down");
Button dc3Dec(140, 160, 35, 40, "3down");
Button dc4Dec(200, 160, 35, 40, "4down");
Button dc5Dec(260, 160, 35, 40, "5down");
Button tempd(260, 160, 35, 40, "temp");

// F and C modes
enum State
{
    Temp_F,
    Temp_C
};
enum Mode
{
    F,
    C
};
static State state = Temp_F;
static Mode mode = F;
float lastTemp = 0;
float lastHum = 0;

////////////////////////////////////////////////////////////////////
// Method header declarations
////////////////////////////////////////////////////////////////////
String httpGETRequest(const char *serverName);
void drawWeatherImage(String iconId, int resizeMult);
void fetchWeatherDetails();
void drawWeatherDisplay();

void newZip();
void redesign();
void localTemp();

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    // Initialize the device
    M5.begin();
    // I2C_RW::initI2C(VCNL_I2C_ADDRESS, 400000, PIN_SDA, PIN_SCL);
    // //I2C_RW::scanI2cLinesForAddresses(false);

    // I2C_RW::writeReg8Addr16DataWithProof(VCNL_REG_PS_CONFIG, 2, 0x0800, " to enable proximity sensor", true);
    // I2C_RW::writeReg8Addr16DataWithProof(VCNL_REG_ALS_CONFIG, 2, 0x0000, " to enable ambient light sensor", true);
    // I2C_RW::writeReg8Addr16DataWithProof(VCNL_REG_WHITE_CONFIG, 2, 0x0000, " to enable raw white light sensor", true);

    //Use the Adafruit library to initialize the sensor over I2C
    Serial.println("Adafruit SHT40 Config demo");
    if (!sht4.begin())
    {
        Serial.println("Couldn't find SHT40 chip");
        while (1) ; // Program ends in infinite loop...
    }
    Serial.println("Found SHT40 chip\n");

    Serial.println("Adafruit VCNL4040 Config demo");
    if (!vcnl4040.begin())
    {
        Serial.println("Couldn't find VCNL4040 chip");
        while (1) ; // Program ends in infinite loop...
    }
    Serial.println("Found VCNL4040 chip\n");

    // Set screen orientation and get height/width
    sWidth = M5.Lcd.width();
    sHeight = M5.Lcd.height();

    // TODO 2: Connect to WiFi
    // WiFi.begin(wifiNetworkNameHome.c_str(), wifiPasswordHome.c_str());
    WiFi.begin(wifiNetworkName.c_str(), wifiPassword.c_str());
    Serial.printf("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n\nConnected to WiFi network with IP address: ");
    Serial.println(WiFi.localIP());

    timeClient.begin();
    //M5.Lcd.setBrightness(0);
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
void loop()
{
    M5.update();
    // int prox = I2C_RW::readReg8Addr16Data(VCNL_REG_PROX_DATA, 2, "to read proximity data", false);
    // Serial.printf("Proximity: %d\n", prox);
    // int als = I2C_RW::readReg8Addr16Data(VCNL_REG_ALS_DATA, 2, "to read ambient light data", false);
    // als = als * 0.1; 
    
    if(vcnl4040.getProximity()>400){
        M5.Lcd.sleep();
    }else{
        M5.Lcd.wakeup();
    }
    if(vcnl4040.getLux()>250 && vcnl4040.getProximity()<400){
        M5.Axp.SetLcdVoltage(3300);
    }else{
         M5.Axp.SetLcdVoltage(2700);
    }
    // if(prox>400){
    //     M5.Lcd.sleep();
    // }else{
    //     M5.Lcd.wakeup();
    // }
    // if(als>250 && prox<400){
    //     M5.Axp.SetLcdVoltage(3300);
    // }else{
    //      M5.Axp.SetLcdVoltage(2700);
    // }

    // if other buttons are pressed, reset the other booleans to be false
    if (M5.BtnA.wasPressed())
    {
        if (mode == F)
        {
            mode = C;
            state = Temp_C;
        }
        else
        {
            mode = F;
            state = Temp_F;
        }
        // if stmts with no bracket only contain the one line after it
        // use the brackets more often!!!
        Serial.printf("A is  %lu. \n", btnPressed = !btnPressed);
        drawWeatherDisplay();
        btnBPressed = false;
        btnCPressed = false;
    }
    if (M5.BtnB.wasPressed())
    {
        Serial.printf("B is %lu. \n", btnBPressed = !btnBPressed);
        newZip();
        btnCPressed = false;
        btnPressed = false;
        //lastTime = 0;
    }
    if (M5.BtnC.wasPressed())
    {
        Serial.printf("C is %lu. \n", btnCPressed = !btnCPressed);
        btnBPressed = false;
        btnPressed = false;
        localTemp();
    }

    if (btnBPressed)
    {
        // draw your buttons

        // update values
        if (dc1Inc.isPressed())
        {
            zip[0] += 1;
            if (zip[0] > 9)
            {
                zip[0] = 0;
            }
            Serial.println(zip[0]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }
        if (dc2Inc.isPressed())
        {
            zip[1] += 1;
            if (zip[1] > 9)
            {
                zip[1] = 0;
            }
            Serial.println(zip[1]);
            Serial.println("2 pressed");
            delay(100);
            newZip();
        }
        if (dc3Inc.isPressed())
        {
            zip[2] += 1;
            if (zip[2] > 9)
            {
                zip[2] = 0;
            }
            Serial.println(zip[2]);
            Serial.println("3 pressed");
            delay(100);
            newZip();
        }
        if (dc4Inc.isPressed())
        {
            zip[3] += 1;
            if (zip[3] > 9)
            {
                zip[3] = 0;
            }
            Serial.println(zip[3]);
            delay(100);
            newZip();
        }
        if (dc5Inc.isPressed())
        {
            zip[4] += 1;
            if (zip[4] > 9)
            {
                zip[4] = 0;
            }
            Serial.println(zip[4]);
            delay(100);
            newZip();
        }
        M5.Lcd.fillTriangle(dc1Inc.x, dc1Inc.y + dc1Inc.h, dc1Inc.x + dc1Inc.w / 2, dc1Inc.y, dc1Inc.x + dc1Inc.w, dc1Inc.y + dc1Inc.h, dc1Inc.isPressed() ? BLACK : GREEN);
        M5.Lcd.fillTriangle(dc2Inc.x, dc2Inc.y + dc2Inc.h, dc2Inc.x + dc2Inc.w / 2, dc2Inc.y, dc2Inc.x + dc2Inc.w, dc2Inc.y + dc2Inc.h, dc2Inc.isPressed() ? BLACK : GREEN);
        M5.Lcd.fillTriangle(dc3Inc.x, dc3Inc.y + dc3Inc.h, dc3Inc.x + dc3Inc.w / 2, dc3Inc.y, dc3Inc.x + dc3Inc.w, dc3Inc.y + dc3Inc.h, dc3Inc.isPressed() ? BLACK : GREEN);
        M5.Lcd.fillTriangle(dc4Inc.x, dc4Inc.y + dc4Inc.h, dc4Inc.x + dc4Inc.w / 2, dc4Inc.y, dc4Inc.x + dc4Inc.w, dc4Inc.y + dc4Inc.h, dc4Inc.isPressed() ? BLACK : GREEN);
        M5.Lcd.fillTriangle(dc5Inc.x, dc5Inc.y + dc5Inc.h, dc5Inc.x + dc5Inc.w / 2, dc5Inc.y, dc5Inc.x + dc5Inc.w, dc5Inc.y + dc5Inc.h, dc5Inc.isPressed() ? BLACK : GREEN);

        if (dc1Dec.isPressed())
        {
            zip[0] -= 1;
            if (zip[0] < 0)
            {
                zip[0] = 9;
            }
            Serial.println(zip[0]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }
        if (dc2Dec.isPressed())
        {
            zip[1] -= 1;
            if (zip[1] < 0)
            {
                zip[1] = 9;
            }
            Serial.println(zip[1]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }
        if (dc3Dec.isPressed())
        {
            zip[2] -= 1;
            if (zip[2] < 0)
            {
                zip[2] = 9;
            }
            Serial.println(zip[2]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }
        if (dc4Dec.isPressed())
        {
            zip[3] -= 1;
            if (zip[3] < 0)
            {
                zip[3] = 9;
            }
            Serial.println(zip[3]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }
        if (dc5Dec.isPressed())
        {
            zip[4] -= 1;
            if (zip[4] < 0)
            {
                zip[4] = 9;
            }
            Serial.println(zip[4]);
            Serial.println("1 pressed");
            delay(100);
            newZip();
        }

        if ((millis() - lastTime) > timerDelay)
        {
            newZip();
            lastTime = millis();
        }
    }
    if (btnCPressed){
        if (tempd.isPressed())
        {
            Serial.println("BUTTON TEMP pressed");
            isF = !isF;
            isChanged = true;
            localTemp();
            delay(100);
        }
    }
    // timerDelay = 50;

    // TODOz write code that checks if button b was pressed
    // the new screen only works outside of the loop for some reason!!!

    // Only execute every so often
    if ((millis() - lastTime) > timerDelay)
    {

        if (btnCPressed)
        {
            localTemp();
            
        }
        else if (btnBPressed)
        {

        }
        else
        {

            if (WiFi.status() == WL_CONNECTED)
            {

                fetchWeatherDetails();
                drawWeatherDisplay();
                // timeClient.update();
                // M5.Lcd.setCursor(150, 0);
                // M5.Lcd.print(timeClient.getFormattedTime());
            }
            else
            {
                Serial.println("WiFi Disconnected");
            }

            // Update the last time to NOW
            lastTime = millis();
        }

        // Update the last time to NOW
        lastTime = millis();
    }
}

///////////////////////////////////////////////////////////////
// make a method that redesigns the main screen copy some code from the drawWhether method

/////////////////////////////////////////////////////////////////
// This method is to change the zip code for pulling whether
void newZip()
{
    int pad = 10;
    M5.lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(pad, pad);
    M5.Lcd.fillTriangle(dc1Inc.x, dc1Inc.y + dc1Inc.h, dc1Inc.x + dc1Inc.w / 2, dc1Inc.y, dc1Inc.x + dc1Inc.w, dc1Inc.y + dc1Inc.h, dc1Inc.isPressed() ? BLACK : GREEN);
    M5.Lcd.fillTriangle(dc2Inc.x, dc2Inc.y + dc2Inc.h, dc2Inc.x + dc2Inc.w / 2, dc2Inc.y, dc2Inc.x + dc2Inc.w, dc2Inc.y + dc2Inc.h, dc2Inc.isPressed() ? BLACK : GREEN);
    M5.Lcd.fillTriangle(dc3Inc.x, dc3Inc.y + dc3Inc.h, dc3Inc.x + dc3Inc.w / 2, dc3Inc.y, dc3Inc.x + dc3Inc.w, dc3Inc.y + dc3Inc.h, dc3Inc.isPressed() ? BLACK : GREEN);
    M5.Lcd.fillTriangle(dc4Inc.x, dc4Inc.y + dc4Inc.h, dc4Inc.x + dc4Inc.w / 2, dc4Inc.y, dc4Inc.x + dc4Inc.w, dc4Inc.y + dc4Inc.h, dc4Inc.isPressed() ? BLACK : GREEN);
    M5.Lcd.fillTriangle(dc5Inc.x, dc5Inc.y + dc5Inc.h, dc5Inc.x + dc5Inc.w / 2, dc5Inc.y, dc5Inc.x + dc5Inc.w, dc5Inc.y + dc5Inc.h, dc5Inc.isPressed() ? BLACK : GREEN);

    M5.Lcd.print("Enter Zipcode:");

    // the sizeof fxn returns the size of the array in bytes,
    // divide by the data type used to get the proper array size
    for (int i = 0; i < sizeof(zip) / sizeof(int); i++)
    {
        int btnWid = pad * (3 + (i * 6));
        M5.Lcd.setCursor(btnWid, sHeight / 2);
        M5.Lcd.print(zip[i]);
        M5.Lcd.fillRect(btnWid - 15, sHeight / 2 - 60, 45, 50, TFT_LIGHTGREY);
        M5.Lcd.fillRect(btnWid - 15, sHeight / 2 + 30, 45, 50, TFT_LIGHTGREY);

        // TODOz draw the up/down triangles
    }
}

void localTemp() {
   
    //Library calls to get the sensor readings over I2C
     sensors_event_t humidity, temp;
  
  uint32_t timestamp = millis();
  sht4.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  timestamp = millis() - timestamp;

  if(round(temp.temperature) != round(lastTemp) || round(humidity.relative_humidity) != round(lastHum) || isChanged == true){
    
 M5.Lcd.fillScreen(BLACK);
   M5.Lcd.fillRect(260, 160, 35, 40, TFT_LIGHTGREY);
  M5.Lcd.setCursor(270,170);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.printf(isF ? "F" : "C");
  M5.Lcd.setCursor(10,10);
M5.Lcd.setTextSize(2);
M5.Lcd.setTextColor(RED);
  M5.Lcd.printf("This is the LOCAL temp  \n and humidity(basically  \n the room you are stading in)");


M5.Lcd.setTextSize(3);
   M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(10,80);
    if(isF){
        M5.Lcd.printf("Temperature:%0.fF\n",(temp.temperature * 1.8 ) + 32);
    }else {
        M5.Lcd.printf("Temperature:%0.fC\n",temp.temperature);
    }
        
        M5.Lcd.setCursor(10,120);
        M5.Lcd.printf("Humidity:%0.f%%rH",humidity.relative_humidity);


  Serial.print("Humidity: "); 
Serial.print(humidity.relative_humidity); 
Serial.println("% rH");

M5.Lcd.setCursor(10,200);
  M5.Lcd.print(timeClient.getFormattedTime());

    Serial.printf("Proximity: %d\n", vcnl4040.getProximity());
    Serial.printf("Ambient light: %d\n", vcnl4040.getLux());
    Serial.printf("Raw white light: %d\n\n", vcnl4040.getWhiteLight());
    Serial.printf(timeClient.getFormattedTime().c_str());
    lastTemp = temp.temperature;
    lastHum = humidity.relative_humidity;
    isChanged = false;
  }
  delay(1000);

}

/////////////////////////////////////////////////////////////////
// This method fetches the weather details from the OpenWeather
// API and saves them into the fields defined above
/////////////////////////////////////////////////////////////////
void fetchWeatherDetails()
{
    //////////////////////////////////////////////////////////////////
    // Hardcode the specific city,state,country into the query
    // Examples: https://api.openweathermap.org/data/2.5/weather?q=riverside,ca,usa&units=imperial&appid=YOUR_API_KEY
    //////////////////////////////////////////////////////////////////
    String serverURL = urlOpenWeather + "q=riverside,ca,usa&units=imperial&appid=" + apiKey;
    // String serverURL = urlOpenWeather + "q=des+moines,ia,usa&units=imperial&appid=" + apiKey;
    // Serial.println(serverURL); // Debug print

    // TODOz make the whether call with the zipcode API instead of cityname
    // https://api.openweathermap.org/data/2.5/weather?zip=92879,us&units=imperial&appid=29a54537e840e47cdb0839988a0a94b2
    // https://api.openweathermap.org/data/2.5/weather?q=corona,ca,usa&units=imperial&appid=29a54537e840e47cdb0839988a0a94b2
    zipStr = "";
    for (int i = 0; i < sizeof(zip) / sizeof(int); i++)
    {
        zipStr += (zip[i]);
    }
    Serial.println(zipStr);
    String zipURL = urlOpenWeather + "zip=" + zipStr + ",us&units=imperial&appid=" + apiKey;

    //////////////////////////////////////////////////////////////////
    // Make GET request and store reponse
    //////////////////////////////////////////////////////////////////
    // String response = httpGETRequest(serverURL.c_str());
    String response = httpGETRequest(zipURL.c_str());
    // Serial.print(response); // Debug print

    //////////////////////////////////////////////////////////////////
    // Import ArduinoJSON Library and then use arduinojson.org/v6/assistant to
    // compute the proper capacity (this is a weird library thing) and initialize
    // the json object
    //////////////////////////////////////////////////////////////////
    const size_t jsonCapacity = 768 + 250;
    DynamicJsonDocument objResponse(jsonCapacity);

    //////////////////////////////////////////////////////////////////
    // Deserialize the JSON document and test if parsing succeeded
    //////////////////////////////////////////////////////////////////
    DeserializationError error = deserializeJson(objResponse, response);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    // serializeJsonPretty(objResponse, Serial); // Debug print

    //////////////////////////////////////////////////////////////////
    // Parse Response to get the weather description and icon
    //////////////////////////////////////////////////////////////////
    JsonArray arrWeather = objResponse["weather"];
    JsonObject objWeather0 = arrWeather[0];
    String desc = objWeather0["main"];
    String icon = objWeather0["icon"];
    String city = objResponse["name"];

    // ArduionJson library will not let us save directly to these
    // variables in the 3 lines above for unknown reason
    strWeatherDesc = desc;
    strWeatherIcon = icon;
    cityName = city;

    // Parse response to get the temperatures
    JsonObject objMain = objResponse["main"];

    tempNow = objMain["temp"];
    tempMin = objMain["temp_min"];
    tempMax = objMain["temp_max"];

    Serial.printf("NOW: %.1f F and %s\tMIN: %.1f F\tMax: %.1f F\n", tempNow, strWeatherDesc, tempMin, tempMax);
    Serial.printf("NOW: %.1f C and %s\tMIN: %.1f C\tMax: %.1f C\n", tempNowC, strWeatherDesc, tempMinC, tempMaxC);
}

/////////////////////////////////////////////////////////////////
// Update the display based on the weather variables defined
// at the top of the screen.
/////////////////////////////////////////////////////////////////
void drawWeatherDisplay()
{
    //////////////////////////////////////////////////////////////////
    // Draw background - light blue if day time and navy blue of night
    //////////////////////////////////////////////////////////////////
    uint16_t primaryTextColor;
    if (strWeatherIcon.indexOf("d") >= 0)
    {
        M5.Lcd.fillScreen(TFT_CYAN);
        primaryTextColor = TFT_DARKGREY;
    }
    else
    {
        M5.Lcd.fillScreen(TFT_BLACK);
        primaryTextColor = TFT_WHITE;
    }

    timeClient.update();
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(150, 0);
    M5.Lcd.print(timeClient.getFormattedTime());

    //////////////////////////////////////////////////////////////////
    // Draw the icon on the right side of the screen - the built in
    // drawBitmap method works, but we cannot scale up the image
    // size well, so we'll call our own method
    //////////////////////////////////////////////////////////////////
    // M5.Lcd.drawBitmap(0, 0, 100, 100, myBitmap, TFT_BLACK);
    drawWeatherImage(strWeatherIcon, 3);

    //////////////////////////////////////////////////////////////////
    // Draw the temperatures and city name
    //////////////////////////////////////////////////////////////////

    int pad = 10;
    if (state == Temp_F)
    {
        M5.Lcd.setCursor(pad, pad);
        M5.Lcd.setTextColor(TFT_BLUE);
        M5.Lcd.setTextSize(3);
        M5.Lcd.printf("LO:%0.fF\n", tempMin);

        M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
        M5.Lcd.setTextColor(primaryTextColor);
        M5.Lcd.setTextSize(10);
        M5.Lcd.printf("%0.fF\n", tempNow);

        M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.setTextSize(3);
        M5.Lcd.printf("HI:%0.fF\n", tempMax);

        M5.Lcd.setCursor(pad, 200);
        M5.Lcd.setTextColor(primaryTextColor);
        M5.Lcd.printf("%s\n", cityName.c_str());
    }
    else
    {
        M5.Lcd.setCursor(pad, pad);
        M5.Lcd.setTextColor(TFT_BLUE);
        M5.Lcd.setTextSize(3);
        M5.Lcd.printf("LO:%0.fC\n", (tempMin - 32) * 0.556);

        M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
        M5.Lcd.setTextColor(primaryTextColor);
        M5.Lcd.setTextSize(10);
        M5.Lcd.printf("%0.fC\n", (tempNow - 32) * 0.556);

        M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
        M5.Lcd.setTextColor(TFT_RED);
        M5.Lcd.setTextSize(3);
        M5.Lcd.printf("HI:%0.fC\n", (tempMax - 32) * 0.556);

        M5.Lcd.setCursor(pad, 200);
        M5.Lcd.setTextColor(primaryTextColor);
        M5.Lcd.printf("%s\n", cityName.c_str());
    }
}

/////////////////////////////////////////////////////////////////
// This method takes in a URL and makes a GET request to the
// URL, returning the response.
/////////////////////////////////////////////////////////////////
String httpGETRequest(const char *serverURL)
{

    // Initialize client
    HTTPClient http;
    http.begin(serverURL);

    // Send HTTP GET request and obtain response
    int httpResponseCode = http.GET();
    String response = http.getString();

    // Check if got an error
    if (httpResponseCode > 0)
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    else
    {
        Serial.printf("HTTP Response ERROR code: %d\n", httpResponseCode);
        Serial.printf("Server Response: %s\n", response);
    }

    // Free resources and return response
    http.end();
    return response;
}

/////////////////////////////////////////////////////////////////
// This method takes in an image icon string (from API) and a
// resize multiple and draws the corresponding image (bitmap byte
// arrays found in EGR425_Phase1_weather_bitmap_images.h) to scale (for
// example, if resizeMult==2, will draw the image as 200x200 instead
// of the native 100x100 pixels) on the right-hand side of the
// screen (centered vertically).
/////////////////////////////////////////////////////////////////
void drawWeatherImage(String iconId, int resizeMult)
{

    // Get the corresponding byte array
    const uint16_t *weatherBitmap = getWeatherBitmap(iconId);

    // Compute offsets so that the image is centered vertically and is
    // right-aligned
    int yOffset = -(resizeMult * imgSqDim - M5.Lcd.height()) / 2;
    int xOffset = sWidth - (imgSqDim * resizeMult * .8); // Right align (image doesn't take up entire array)
    // int xOffset = (M5.Lcd.width() / 2) - (imgSqDim * resizeMult / 2); // center horizontally

    // Iterate through each pixel of the imgSqDim x imgSqDim (100 x 100) array
    for (int y = 0; y < imgSqDim; y++)
    {
        for (int x = 0; x < imgSqDim; x++)
        {
            // Compute the linear index in the array and get pixel value
            int pixNum = (y * imgSqDim) + x;
            uint16_t pixel = weatherBitmap[pixNum];

            // If the pixel is black, do NOT draw (treat it as transparent);
            // otherwise, draw the value
            if (pixel != 0)
            {
                // 16-bit RBG565 values give the high 5 pixels to red, the middle
                // 6 pixels to green and the low 5 pixels to blue as described
                // here: http://www.barth-dev.de/online/rgb565-color-picker/
                byte red = (pixel >> 11) & 0b0000000000011111;
                red = red << 3;
                byte green = (pixel >> 5) & 0b0000000000111111;
                green = green << 2;
                byte blue = pixel & 0b0000000000011111;
                blue = blue << 3;

                // Scale image; for example, if resizeMult == 2, draw a 2x2
                // filled square for each original pixel
                for (int i = 0; i < resizeMult; i++)
                {
                    for (int j = 0; j < resizeMult; j++)
                    {
                        int xDraw = x * resizeMult + i + xOffset;
                        int yDraw = y * resizeMult + j + yOffset;
                        M5.Lcd.drawPixel(xDraw, yDraw, M5.Lcd.color565(red, green, blue));
                    }
                }
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////
// For more documentation see the following links:
// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/
// https://docs.m5stack.com/en/api/core2/lcd_api
//////////////////////////////////////////////////////////////////////////////////
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Please use an Arduino IDE 1.6.8 or greater

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
//#include <DHT.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include "config.h"

static bool messagePending = false;
static bool messageSending = true;

static char *connectionString;
static char *ssid;
static char *pass;

static int interval = INTERVAL;

//Added variables for calibration and testing

const int back = 700;   // background soundValue

//const int amp = 1000; // ampitify soundValue

//const float freq = 0.5;   // frequency soundValue

int start_t,end_t,door_t,click_t,notif_t;

bool timer_f,click_f,door_h,lock_f;     // flags for checking

int max_amp, soundValue;

int progstart_t,count_r;

float total_r,ratio,ratio_t;

const int doorclosed_LED = 12;

bool door_closed, send_f, door_locked, door_lock_ready, reset_f;

const int door_sensor = 4;

int state , check_time;

void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}
//static DHT dht1(DHT_PIN, DHT_TYPE);

void getParams()
{
  if(soundValue > back && timer_f == false)
  {
    timer_f = true;
    start_t = millis();
  }
  if(timer_f == true && soundValue < back)
  {
    
    end_t = millis();
    timer_f = false;
    //frequency = max_amp/(end_t-start_t);
  }
  if(timer_f == true)
  {
    max_amp = max(soundValue, max_amp);
  }
  delay(10);
}

void resetParams()
{
  end_t = millis();
  start_t = millis();
  timer_f = false;
  max_amp = -2000;
}

void updateRatio()
{
  float ratio_d = float(max_amp)/(float)(end_t-start_t);
  if(ratio_t == 0)
  {
    ratio_t = ratio_d;
    total_r += ratio_d;
    count_r++;
    Serial.print("First Ratio: ");
    Serial.println(ratio_d);
  }
  else if((ratio_d >= ratio_t - 10.0) && (ratio_d <= ratio_t + 10.0))
  {
    count_r++;
    total_r += ratio_d;
    ratio_t = total_r/(float)count_r;
    Serial.print("Ratio: ");
    Serial.println(ratio_d);
    Serial.print("True ratio: ");
    Serial.println(ratio_t);
  }
       
  else
  {
    Serial.print("Anomaly data with ratio: ");
    Serial.println(ratio_d);
  }
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
void setup()
{
    pinMode(door_sensor, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    pinMode(PIN_GATE_IN, INPUT);
    initSerial();
    delay(2000);
    readCredentials();

    initWifi();
    initTime();
    initSensor();

    
  //Initializing variables to be used in calibrating and testing sounds
  end_t = millis();
  start_t = millis();
  door_t = millis();
  notif_t = millis();
  click_t = millis();
  
  timer_f = false;
  click_f = false;
  lock_f = true;
  door_h = false;
  send_f = false;
  
  max_amp = -2000;
  
  progstart_t = millis();
  count_r = 0;
  
  total_r = 0.0;
  ratio = 0.0;
  ratio_t = 0.0;
  check_time = millis();
  door_closed = false;
  door_locked = false;

 send_f = false;

 digitalWrite(doorclosed_LED, LOW);
    /*
     * AzureIotHub library remove AzureIoTHubClient class in 1.0.34, so we remove the code below to avoid
     *    compile error
    */

    // initIoThubClient();
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "HappyPath_AdafruitFeatherHuzzah-C");
    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);
    //start_t = millis();
}

static int messageCount = 1;
//Functions used for calibrating and testing


void loop()
{
  /*
  int t_d = millis() - progstart_t;
  if(t_d > 15000 && t_d < 17000)
  { 
    delay(17000 - t_d);
    Serial.println("calibrating for click");
  }
  else if(t_d < 30000)
  {
    soundValue = analogRead(PIN_ANALOG_IN);
    getParams();
    if(end_t > start_t && timer_f == false)
    {
      //Serial.println(soundValue);
      // calculate the frequency
      updateRatio();
      resetParams();
    }
  }
  else if(t_d < 33000)
  {
    Serial.print("Done calibrating click with ratio:");
    Serial.println(ratio_t);
    delay(33000-t_d);
  }*/
  
    state = digitalRead(door_sensor);
    if(state == LOW && !door_closed) 
    {
      digitalWrite(doorclosed_LED, HIGH);
      Serial.println("Door closed");
      notif_t = millis();
      door_closed = true;
      delay(1000);
    }
    if(door_closed && !door_locked && (millis() - notif_t)> 10000)
    {
      notif_t = millis();
      send_f = true;
    }
    if(state == HIGH && door_closed)
    {
      door_lock_ready = true;
      delay(2000);
    }
    if(state == LOW && door_lock_ready)
    {
      Serial.println("Door locked");
      door_locked = true;
      door_lock_ready = false;
      reset_f = true;
      check_time = millis();
    }
    if(reset_f && (millis() - check_time) > 20000)
    {
      Serial.println("Door opened");
      door_closed = false;
      door_locked = false;
      door_lock_ready = false;
      reset_f = false;
    }
    if(send_f == true && !messagePending && messageSending)
    {
        Serial.println("Sending data to the cloud");
        char messagePayload[MESSAGE_MAX_LEN];
        bool temperatureAlert = readMessage(messageCount, messagePayload);

        sendMessage(iotHubClientHandle, messagePayload, temperatureAlert);
        messageCount++;
        send_f = false;
        delay(200);
    }
    IoTHubClient_LL_DoWork(iotHubClientHandle);


  //Added lines to calibrate and test sounds
  /*soundValue = analogRead(PIN_ANALOG_IN);
  int t_d = millis()-progstart_t;
  if(t_d < 3000)
  {
    delay(3000-t_d);
    Serial.println("Calibrating for door");
  }
  else if(t_d < 20000) //Calibrating for door
  {
    getParams();
    if(end_t > start_t && timer_f == false)
    {
      // calculate the frequency
      updateRatio();
      resetParams();
    }
  }
  else if(t_d < 21000 && ratio == 0)
  {
     delay(21000-t_d);
     ratio = ratio_t;
     total_r = 0.0;
     count_r = 0;
     ratio_t = 0.0;
     
     Serial.print("Done calibrating door, with ratio :");
     Serial.println(ratio);

  }
  else if(t_d < 21200)
  {
    delay(21200 - t_d);
    Serial.println("Calibrating for lock"); 
  }
  else if(t_d < 37000)
  {
    getParams();
    if(end_t > start_t && timer_f == false)
    {
      // calculate the frequency
      if(lock_f == true)
      {
        updateRatio();
        lock_f == false;
      }
      else
      {
        lock_f = true;
      }
      resetParams();
    }
  }
  else if(t_d < 37100 && total_r != 0)
  {
    //int ratio_click = ratio_t;
    if(ratio > ratio_t)
      door_h = true;
    ratio = (ratio + ratio_t)/2;
    total_r = 0.0;
    count_r = 0;
    ratio_t = 0.0;
  }

  else
  {
    getParams();
    if(end_t > start_t && timer_f == false){
    // calculate the frequency
    
      float ratio_sensor = (float)max_amp/(float)(end_t-start_t);
      if(door_h)
      {
        if(ratio_sensor > ratio)    // door noise
        {
          Serial.println("door closed");
          door_t = millis();
          notif_t = millis();
          click_f = true;  
        }
        else
        {
          Serial.println("Locked");
          if(click_f == true)
          {
            Serial.println("Door locked");
            send_f = false;
            click_f = false;
            door_t = millis();
            click_t = millis();
          }
        }
      }
      else
      {
        if(ratio_sensor < ratio) //door noise
        {
          Serial.println("door closed");
          door_t = millis();
          notif_t = millis();
          click_f = true;
        }
        else                    // click(lock) noise
        {
          Serial.println("Locked");
          if(click_f == true)
          {
            Serial.println("Door locked");
            send_f = false;
            click_f = false;
            door_t = millis();
            click_t = millis();
          }
        }
      }
      resetParams();
      //delay(10);
    }
    if(click_f && millis()-notif_t > 15000)
    {
      send_f = true;
      notif_t = millis();
     // initWifi();
     // initTime();
    }
*/  

    //delay(100);
  
}

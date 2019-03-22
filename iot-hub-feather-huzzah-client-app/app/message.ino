#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
//#include <DHT.h>

bool readMessage(int messageId, char *payload)
{
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = DEVICE_ID;

    root["messageId"] = messageId;
    root["sendmail"] = "YES";
    //bool temperatureAlert = false;

    //else
    //{
        /*if (temperature > TEMPERATURE_ALERT)
        {
            temperatureAlert = true;
        }*/
    //}

    /*if (std::isnan(humidity))
    {
        root["humidity"] = NULL;
    }
    else
    {
        root["humidity"] = humidity;
    }*/
    root.printTo(payload, MESSAGE_MAX_LEN);
    return temperatureAlert;
}

void parseTwinMessage(char *message)
{
    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(message);
    if (!root.success())
    {
        Serial.printf("Parse %s failed.\r\n", message);
        return;
    }

    if (root["desired"]["interval"].success())
    {
        interval = root["desired"]["interval"];
    }
    else if (root.containsKey("interval"))
    {
        interval = root["interval"];
    }
}

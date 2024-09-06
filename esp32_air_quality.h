#ifndef wc

  #define esp32_air_quality
//  #define DBUG                              // Uncomment this for serial print
  #define WIFI_LED_PIN    4                 // Wifi connection state led pin
  #define MQTT_LED_PIN    0                 // MQTT connection state led pin

  #define NSENSOR         2                 // No. sensors attached to ESP
  #define SERIALBAUD      9600              // Serial baudrate
  #define DEF_INTER_TIME  20000000           // interrupt time in micro seconds
  #define MIN_INTERRUPT   2                // Minimum interrupt time in seconds

  #define NET_ACCESS      (String[][2]){{"ssid-wifi1", "passwordnet1"}, {"ssid-wifi2", "passwordnet2"}}

  #define MQTTADDR        (const char *[]){"192.168.1.73"}    // MQTT broker IP address
  #define MQTTUSERNAME    (char*)"myname"          // MQTT authentification username
  #define MQTTPASSWORD    (char*)"mysecretpassword"       // MQTT authentification passord
  #define MQTT_PAYLOAD_S  512

  #define SPIN            (int[]){39,35}                          // ESP32 Pin No.
  
  #define SNAME           (String[]){"NH3","CO"}         // SENSOR NAME/DESCRIPTION
  #define MEASURE         (String[]){"/100ppmNH3","/100ppmCO"}     // SENSOR MEASURE UNITS


#endif //esp32_air_quality

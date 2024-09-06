#include <WiFi.h>
#include <MQTT.h>
#include "esp32_air_quality.h"
#include <ESP32Time.h>

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;	//
volatile unsigned long totalInterruptCounter = 0;	// interrupt increment only variable
volatile unsigned int interruptCounter = 0;		// interrupt increment and decrement variable
volatile uint16_t sensorval[NSENSOR] = {0};		// sensors storage vector
volatile bool iflag = false;				// interrupt as ocorred flag
unsigned long TimestampMS;				// variable for microseconds passed
hw_timer_t * timer = NULL;				// timer object to further be contructed
uint64_t interrupt_time;              			// can be altered from MQTT subscription
unsigned long t0 = 0;					// timestamp of first sensor read (miliseconds) after connection


WiFiClient net_client;					// Wifi object from <WiFi.h>
MQTTClient mqtt_client(MQTT_PAYLOAD_S);			// mqtt object from <MQTT.h>
String mqttclientid;					// string to store this arduino identification
ESP32Time rtc(0);					// start real time clock

			// PROTOTYPING
void messageReceived(String &topic, String &payload);
void IRAM_ATTR onTimer();
void connect();


void setup() {					// STARTUP ARDUINO CODE
  #ifdef DBUG
    Serial.begin(SERIALBAUD);                   // start arduino serial connection
  #endif
  connect();                                    // call wifi and mqtt connection procedure
  mqtt_client.setOptions(60 , true , 99999);
  mqtt_client.onMessage(messageReceived);
  interrupt_time = DEF_INTER_TIME;
  timer = timerBegin(0, 80, true);              // add interrupt=0, divider (hz related?),
  timerAttachInterrupt(timer, &onTimer, true);  // attach interrupt execution function to >
  timerAlarmWrite(timer, interrupt_time, true); // set interrupt period
  timerAlarmEnable(timer);                      // enable interrupt
  bool result = mqtt_client.subscribe("/"+mqttclientid+"/rate");    // get messages for this device
  mqtt_client.subscribe("/:::::/rate");              // get broadcast messages also
  #ifdef DBUG
    Serial.print("subscription ");
    result ? Serial.println("success") : Serial.println("failed");
  #endif
}

void loop() {			        // ARDUINO CODE TO BE EXECUTED AFTER SETUP COMPLETED
  mqtt_client.loop();			// this activates our event
  if (!mqtt_client.connected()) {	// check if connection to mosquitto is not established, and put arduino to rest
    timerAlarmDisable(timer);		// unregister interruption # TO-DO ENABLE
    portENTER_CRITICAL(&timerMux);	// enter critical section by putting a lock (we don't need this, while not using concurrency)
    interruptCounter = 0;		// reset interrupt counter
    portEXIT_CRITICAL(&timerMux);  	// exit critical section (unlock)
    totalInterruptCounter = 0;    	// reset total interrupt counter
    connect();				// connect to broker procedure
    t0 = 0;				// clean start time
    timerAlarmEnable(timer);        	// enable interrupt (again)
  }
  if (iflag) {				// CODE TO BE EXECUTED ONLY IF INTERRUPT OCCURED
    iflag=false;			// clear interruption flag
    portENTER_CRITICAL(&timerMux);	// enter critical section by putting a lock (we don't need this, while not using concurrency)
    interruptCounter--;			// decrement interrupt counter again (??) NEED TO CHECK WHY THIS IS NEEDED
    portEXIT_CRITICAL(&timerMux); 	// exit critical section (unlock)
    totalInterruptCounter++;		// increment total interrupt counter
    if (t0 == 0) {			// CODE TO BE EXECUTED ONLY AFTER CONNECTION TO MOSQUITO ESTABLISHED
      t0 = millis();			// correct time so we don't enter here again
    } else {				// CODE TO BE EXECUTED EVERY OTHER TIME
      TimestampMS = millis() - t0;	// update timestamp
    }
    #ifdef DBUG
      String serialOut = (String) TimestampMS;		// initialize serial msg String
    #endif
    String topic = "/" + mqttclientid + "/";
    String debug_msg = (String) mqttclientid + "-" + TimestampMS + "-";	//Initialize mosquitto publication msg string
    for ( int i=0; i < NSENSOR; i++) {				         // GET SENSOR VALUES, CONTRUCT PUBLICATION STRING AND SIGNAL LEDs
      sensorval[i] = analogRead(SPIN[i]);			         // read gas value from analog
      #ifdef DBUG
        serialOut += (String)"  " + SNAME[i] + ":" + sensorval[i] + MEASURE[i] + "   ";		// construct serial msg
      #endif
      int nindex = SNAME[i].lastIndexOf(' ');	    		// get measure units split index from string located at header
      int nlength = SNAME[i].length();		        	// get measure units string length
      debug_msg += (String) SNAME[i].substring(nindex+1, nlength) + ";" + sensorval[i] + " ";	// contruct publish msg
      mqtt_client.publish(topic + SNAME[i],(String) sensorval[i]);            // there is another aproach more complicated memory wise performance 

    }
    debug_msg = debug_msg.substring(0, debug_msg.length());	   // remove last character(space) added previously
    #ifdef DBUG
      serialOut = debug_msg.substring(0, serialOut.length());      // remove last character (space) added on last loop
      Serial.println(serialOut);				   // print message sent to mosquitto on serial
      Serial.println((String) "Interrupt counter: " + totalInterruptCounter);
    #endif
  }
}

void messageReceived(String &topic, String &payload) {
  #ifdef DBUG
    Serial.println("incoming: " + topic + "   " + payload);
  #endif
  if (topic.endsWith("rate")) {         // should be good enougth
    int temp = payload.toInt();
    uint8_t new_time=0;
    new_time = (4 < temp < 100) ? temp : 0;
    if (!new_time) {
      #ifdef DBUG
        Serial.println("Unexpected time payload. Not within range.");
      #endif
      return;
    }
    timerAlarmWrite(timer, new_time*1000000, true); // set interrupt period
  }
}

// THIS CODE HANDLES CONNECTIVITY AND ONLY LEAVES IF IS ESTABLISHED
void connect() {
  while (WiFi.status() != WL_CONNECTED) {	        // only leaves when Wifi connection is established
    for (int i=0; i<sizeof(NET_ACCESS); i++) {
      if (WiFi.status() == WL_CONNECTED) break; 	// only leaves when Wifi connection is established
      WiFi.begin(NET_ACCESS[i][0], NET_ACCESS[i][1]);	// start Wifi
      #ifdef DBUG
        Serial.println((String)"connecting to WiFi: " + NET_ACCESS[i][0] + NET_ACCESS[i][1]);
      #endif
      delay(2000);			        	// wait 1 second and retry
    }
    delay(1000);
  }
  char *mqtt_client_id = new char[18];			// alocate memory for id/mac address
  mqttclientid = WiFi.macAddress(); 		  	// get mac address
  strcpy(mqtt_client_id, WiFi.macAddress().c_str());	// store on char* since mqtt doesn't support String for this

  #ifdef DBUG		// log to serial
    Serial.println((String)"WiFi connected. Received IP: ");
    Serial.print(WiFi.localIP()); Serial.println((String)" MAC:" + mqttclientid );
    Serial.println((String)"connecting to Mosquitto");
  #endif
  while (!mqtt_client.connected()) {
    for (int i=0; i<sizeof(MQTTADDR); i++) {
      mqtt_client.begin(MQTTADDR[i], 1883, net_client);		// start mqtt client session
      if (mqtt_client.connect(mqtt_client_id, MQTTUSERNAME, MQTTPASSWORD))	// only leaves when connection to broker is established
        break;
      delay(1200);	                            // wait 1.2 second and retry
    }
  }

  #ifdef DBUG
    Serial.println((String)"connected to Mosquitto");
  #endif
}


// CODE TO BE EXECUTED IF HARDWARE INTERRUPT OCCURED
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);	// aquire lock (not needed since theres no concurrency)
  interruptCounter++;			// increment interrupt counter
  portEXIT_CRITICAL_ISR(&timerMux);	// unlock
  iflag = true;				// set interrupt flag to signal interruption on loop()
}


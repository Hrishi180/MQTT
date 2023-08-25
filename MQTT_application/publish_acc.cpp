#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include "MQTTClient.h"
#include "ADXL345.h"
#include <unistd.h>
#include <time.h>
#include <iomanip>
#include <pthread.h>

using namespace std;
using namespace exploringRPi;

// Please replace the following address with the address of your server
#define ADDRESS     "tcp://192.168.77.253:1883"
#define CLIENTID    "rpi1"
#define AUTHMETHOD  "hrishi"
#define AUTHTOKEN   "hello"
#define TOPIC1       "ee513/SensorData"
#define TOPIC2       "ee513/TempData"
#define QOS         2
#define TIMEOUT     10000L


char* get_formatted_time() {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char* formatted_time = (char*)malloc(20 * sizeof(char));
    strftime(formatted_time, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
    return formatted_time;
}


float get_cpu_temperature() {
    std::ifstream temp("/sys/class/thermal/thermal_zone0/temp");
    float temp_val;
    temp >> temp_val;
    float temp_celsius = temp_val / 1000.0;
    return temp_celsius;
}

float get_cpu_load() {
    std::ifstream loadavg("/proc/loadavg");
    float load;
    loadavg >> load;
    return load;
}

int main(int argc, char* argv[]) {
   
   char* time_str = get_formatted_time();
   float cpu_temp = get_cpu_temperature();
   float cpu_load = get_cpu_load();

   MQTTClient client;
   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
   MQTTClient_message pubmsg1 = MQTTClient_message_initializer;
   MQTTClient_message pubmsg2 = MQTTClient_message_initializer;
   MQTTClient_deliveryToken token;
   MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;
   
   MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
   will_opts.topicName = TOPIC1;
   will_opts.message = "Lost connection to client";
   opts.will = &will_opts; 
   opts.keepAliveInterval = 20;
   opts.cleansession = 1;
   opts.username = AUTHMETHOD;
   opts.password = AUTHTOKEN;


   ADXL345 sensor(1,0x53);
   sensor.setResolution(ADXL345::NORMAL);
   sensor.setRange(ADXL345::PLUSMINUS_4_G);

   while(true) {

   // Get sensor data
   sensor.displayPitchAndRoll(100); // Call the displayPitchAndRoll function to print pitch and roll values to the console
   float pitch = sensor.getPitch();
   float roll = sensor.getRoll();

   //declaring two strings to store the payload 
	char str_payload1[256];
   char str_payload2[256];


   //Create payload for client on topic 1
	sprintf(str_payload1, "{\"d\":{\"Time\": \"%s\", \"Pitch\": %f, \"Roll\": %f }}", time_str, pitch, roll);
	
       pubmsg1.payload = str_payload1;
       pubmsg1.payloadlen = strlen(str_payload1);
       pubmsg1.qos = QOS;
       pubmsg1.retained = 0;

   //Create payload for client on topic 2
   sprintf(str_payload2, "{\"d\":{\"Time\": \"%s\", \"CPU Load\": %f, \"CPU Temperature\": %f }}", time_str, cpu_load, cpu_temp);

       pubmsg2.payload = str_payload2;
       pubmsg2.payloadlen = strlen(str_payload2);
       pubmsg2.qos = QOS;
       pubmsg2.retained = 0;

   usleep(100000); // Add a delay of 100 milliseconds before publishing the message

       int rc;
       if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
          cout << "Failed to connect" << endl;

 }
       
       //Publish message on topic1 and topic2
       MQTTClient_publishMessage(client, TOPIC1, &pubmsg1, &token);
       MQTTClient_publishMessage(client, TOPIC2, &pubmsg2, &token);

       cout <<  cpu_load << cpu_temp << endl; // Print laod and temp values to console
       cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
            " seconds for publication of " << str_payload2 <<
            " \non topic " << TOPIC2 << " for ClientID: " << CLIENTID << endl;


       cout << "Pitch: " << sensor.getPitch() << ", Roll: " << sensor.getRoll() << endl; // Print pitch and roll values to console
       cout << "Waiting for up to " << (int)(TIMEOUT/1000) <<
            " seconds for publication of " << str_payload1 <<
            " \non topic " << TOPIC1 << " for ClientID: " << CLIENTID << endl;

       rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
       cout << "Message with token " << (int)token << " delivered." << endl;
       MQTTClient_disconnect(client, 10000);


       usleep(1000000); // Add a delay of 1 second before the next iteration
   }

   MQTTClient_destroy(&client);
   return 0;
}

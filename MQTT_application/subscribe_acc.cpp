#include <iostream>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <wiringPi.h> // Added for GPIO control

#define ADDRESS     "tcp://192.168.77.253:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "hrishi"
#define AUTHTOKEN   "hello"
#define TOPIC       "ee513/SensorData"
#define QOS         2
#define TIMEOUT     10000L
#define LED         17

using namespace std;

volatile MQTTClient_deliveryToken deliveredtoken;

void trigger_led(float pitchValue, float rollValue) {
    if ((pitchValue > 20.0 || pitchValue < -10) || (rollValue < -10.0 || rollValue > 20)) {
        std::cout << "Condition met" << std::endl;
        // Trigger LED on GPIO pin
        digitalWrite(LED, HIGH);
    } else {
        std::cout << "Condition not met" << std::endl;
        digitalWrite(LED, LOW);
    }
}

void parse_pitch_roll(const char* payload, double& pitchValue, double& rollValue) {
    const char* pitchKey = "\"Pitch\":";
    const char* rollKey = "\"Roll\":";
    const char* pitchStart = strstr(payload, pitchKey);
    const char* rollStart = strstr(payload, rollKey);
    if (pitchStart && rollStart) {
        pitchValue = atof(pitchStart + strlen(pitchKey));
        rollValue = atof(rollStart + strlen(rollKey));
        printf("Pitch: %f, Roll: %f\n", pitchValue, rollValue);
    } else {
        printf("Failed to parse Pitch and Roll values from JSON payload\n");
    }
}


void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {


	 // Initialize WiringPi library for GPIO control
    wiringPiSetupGpio();
    pinMode(LED, OUTPUT); 


    int i;
    char* payloadptr;
    double pitchValue;
    double rollValue;  
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = (char*) message->payload;
    for(i=0; i<message->payloadlen; i++) {
        putchar(*payloadptr++);
    }
    putchar('\n');

    // Get Pitch and Roll values from message payload
    const char* payload = (const char*)message->payload;
    parse_pitch_roll(payload, pitchValue, rollValue);

    
    //function call to the function which triggers the LED if the values cross the defined threshold
    trigger_led(pitchValue, rollValue);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;



    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

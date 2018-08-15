#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "application.h"

// Set the SORACOM APN
STARTUP(cellular_credentials_set("soracom.io", "sora", "sora", NULL));
//
// Configuration
//
#define PARTICLE_KEEPALIVE 3000

// device name
#define DEVICE_NAME "ElectronDHT"

// sensor type: [DHT11, DHT22, DHT21, AM2301]
#define DHTTYPE DHT11

// which digital pin for the DHT
#define DHTPIN D3

// which digital pin for the Photon/Spark Core/Electron LED
#define LEDPIN D7

// whether to use Farenheit instead of Celsius
#define USE_FARENHEIT 1

// min/max values (sanity checks)
#define MIN_TEMPERATURE -30
#define MAX_TEMPERATURE 120

#define MIN_HUMIDITY 0
#define MAX_HUMIDITY 100

// sensor sending interval (seconds)
#define SEND_INTERVAL 60

// Particle event
#define PARTICLE_EVENT 1
#define PARTICLE_EVENT_NAME "electron-dht-logger-log"

//TCP Client Instantiation
TCPClient client;

DHT dht(DHTPIN, DHTTYPE);

float humidity;
float temperature;
float heatIndex;

char humidityString[10];
char temperatureString[10];
char heatIndexString[10];

int failed = 0;

// last time since we sent sensor readings
int lastUpdate = 0;

// for HTTP POST and Particle.publish payloads
char payload[1024];

/**
 * Setup
 */
void setup() {
    /** strip.begin();
    strip.setBrightness(70);
    strip.show();**/
    Serial.begin(9600);
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, HIGH);

    // configure Particle variables - float isn't accepted, so we have to use string versions
    Particle.variable("temperature", &temperatureString[0], STRING);
    Particle.variable("humidity", &humidityString[0], STRING);
    Particle.variable("heatIndex", &heatIndexString[0], STRING);
    Particle.variable("status", &failed, INT);

    // start the DHT sensor
    dht.begin();

#if PARTICLE_EVENT
    // startup event
    sprintf(payload,
            "{\"device\":\"%s\",\"state\":\"starting\"}",
            DEVICE_NAME);

    Spark.publish(PARTICLE_EVENT_NAME, payload, 60, PRIVATE);
#endif

    // run the first measurement
    loop();
}

/**
 * Event loop
 */
void loop() {
    int now = Time.now();

    // only run every SEND_INTERVAL seconds
    if (now - lastUpdate < SEND_INTERVAL) {
        return;
    }

    // turn on LED when updating
    digitalWrite(LEDPIN, HIGH);

    lastUpdate = now;

    failed = 0;

    // read humidity and temperature values
    humidity = dht.readHumidity();
    temperature = dht.readTemperature(USE_FARENHEIT);

    if (temperature == NAN
        || humidity == NAN
        || temperature > MAX_TEMPERATURE
        || temperature < MIN_TEMPERATURE
        || humidity > MAX_HUMIDITY
        || humidity < MIN_HUMIDITY) {
        // if any sensor failed, bail on updates
        failed = 1;
    } else {
        failed = 0;

        // calculate the heat index
        heatIndex = dht.computeHeatIndex(temperature, humidity, USE_FARENHEIT);

        // convert floats to strings for Particle variables
        sprintf(temperatureString, "%.2f", temperature);
        sprintf(humidityString, "%.2f", humidity);
        sprintf(heatIndexString, "%.2f", heatIndex);


        sprintf(payload,
            "{\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"heatIndex\":%.2f}",
            DEVICE_NAME,
            temperature,
            humidity,
            heatIndex);
        if (client.connect("funnel.soracom.io", 23080))
        {
          Serial.println("connected");
        }
        else
        {
          Serial.println("connection failed");
        }
        client.write(payload);
        digitalWrite(LEDPIN, LOW);
        Serial.println("disconnecting.");
        client.stop();
    }
}

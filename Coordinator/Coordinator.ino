#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "FS.h"

#include "src/LittleFS.hpp"

WiFiServer server(80);

void init_wifi() {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(global_config_data.device_custom_hostname.c_str()); //define hostname

    Serial.print("Hostname: ");
    Serial.println(global_config_data.device_custom_hostname);

    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(global_config_data.wifi_ssid, global_config_data.wifi_password);

    Serial.print("Connecting to WiFi '");
    Serial.print(global_config_data.wifi_ssid);
    Serial.print("' ..");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println(WiFi.localIP());
}

void setup() {
	Serial.begin(115200);
	Serial.println(F("Starting..."));

    if (littlefs_read_config()) {
        init_wifi();
        server.begin();
    }

    if (global_config_data.interval < 1) {
        global_config_data.interval = 1;
    }
}

void send_status() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(global_config_data.destination_address);
        http.setAuthorization(global_config_data.auth_user.c_str(), global_config_data.auth_password.c_str());
        http.setTimeout(5);

        int co2_ppm = 0;
        float rel_hum_perc = 0.0f;

        int httpResponseCode = http.POST(
            "deviceId=" + WiFi.macAddress() + 
            "&co2=" + co2_ppm +
            "&rh=" + rel_hum_perc
        );

        if (Serial) {
            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = http.getString();
                Serial.println(payload);
            }
            else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
        }

        http.end();
    }
}

const unsigned long max_wifi_timeout_until_reconnect = 15000;
unsigned long require_wifi_reconnect_time = 0;

void check_wifi() {
    unsigned long now = millis();

    bool connected = (WiFi.status() == WL_CONNECTED);
    bool wifi_reconnect_elapsed = (now > require_wifi_reconnect_time);
    if (connected || wifi_reconnect_elapsed) {
        require_wifi_reconnect_time = now + max_wifi_timeout_until_reconnect;

        if (!connected && wifi_reconnect_elapsed) {
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

unsigned long next_measurements_send_time = 0;

void check_measurements() {

    unsigned long now = millis();
    
    if (now > next_measurements_send_time) {
        next_measurements_send_time = now + (global_config_data.interval * 1000);

        send_status();
    }
}

void process_incoming_measurements() {
    // Current time
    unsigned long current_time = millis();
    // Previous time
    unsigned long previous_time = 0;
    // Define timeout time in milliseconds (example: 2000ms = 2s)
    const long timeout_ms = 2000;

    WiFiClient client = server.available();   // Listen for incoming clients
    String header = "";

    if (client) {                             // If a new client connects,
        current_time = millis();
        previous_time = current_time;
        Serial.println("New Client.");          // print a message out in the serial port
        String currentLine = "";                // make a String to hold incoming data from the client
        while (client.connected() && (current_time - previous_time) <= timeout_ms) {  // loop while the client's connected
            current_time = millis();
            if (client.available()) {             // if there's bytes to read from the client,
                char c = client.read();             // read a byte, then
                Serial.write(c);                    // print it out the serial monitor
                header += c;
                if (c == '\n') {                    // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        Serial.println(header);

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html><body>ESP32 Web Server</body></html>");

                        // The HTTP response ends with another blank line
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r') {  // if you got anything else but a carriage return character,
                    currentLine += c;      // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}

void loop() {
    check_wifi(); 
    check_measurements();
    process_incoming_measurements();
}

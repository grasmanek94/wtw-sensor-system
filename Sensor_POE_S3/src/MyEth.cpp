#include "MyEth.hpp"

#include <ESPmDNS.h>
#include <ETH.h>
#include <esp_mac.h>
#include <SPI.h>
#include <WiFi.h>
#include <lwip/dns.h>

static const unsigned long max_ethernet_timeout_until_reconnect = 15000;
static unsigned long require_ethernet_reconnect_time = 0;

#define W5500_PIN_MISO GPIO_NUM_12
#define W5500_PIN_MOSI GPIO_NUM_11
#define W5500_PIN_SCLK GPIO_NUM_13
#define W5500_PIN_CS   GPIO_NUM_14
#define W5500_PIN_RST  GPIO_NUM_9
#define W5500_PIN_INT  GPIO_NUM_10 

static byte ethernet_mac[6];
static bool ethernet_connected = false;
static SPIClass ethernet_spi = SPIClass(FSPI);

void on_ethernet_event(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //ETH.setHostname("esp32-eth0");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH Got IP: ");
      Serial.println(ETH.localIP());
      ethernet_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      ethernet_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ethernet_connected = false;
      break;
    default:
      break;
  }
}

void init_ethernet() {

    esp_read_mac(ethernet_mac, ESP_MAC_WIFI_STA);

    // make it a valid locally administered MAC
    //ethernet_mac[0] |= 0x02;
    //ethernet_mac[0] &= 0xFE;

    Serial.printf("Using MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        ethernet_mac[0], ethernet_mac[1], ethernet_mac[2],
        ethernet_mac[3], ethernet_mac[4], ethernet_mac[5]);

    WiFi.mode(WIFI_OFF);
    btStop();

    pinMode(W5500_PIN_RST, OUTPUT);
    digitalWrite(W5500_PIN_RST, LOW);
    delay(100);
    digitalWrite(W5500_PIN_RST, HIGH); 
    delay(200);

    // Start SPI with custom pins

    ethernet_spi.begin(W5500_PIN_SCLK, W5500_PIN_MISO, W5500_PIN_MOSI, W5500_PIN_CS);

    Serial.println(__LINE__);

    // Initialize Ethernet (DHCP)
    pinMode(W5500_PIN_CS, OUTPUT);
    digitalWrite(W5500_PIN_CS, HIGH);

    Network.onEvent(on_ethernet_event);
    ETH.begin(
        ETH_PHY_W5500,
        0,
        W5500_PIN_CS,
        W5500_PIN_INT,
        W5500_PIN_RST,
        ethernet_spi
    );
}

bool is_ethernet_connected()
{
    return ethernet_connected;
}

void ethernet_get_mac(uint8_t* const mac)
{
    if(mac != nullptr)
    {
        for(size_t i = 0; i < sizeof(ethernet_mac); ++i)
        {
            mac[i] = ethernet_mac[i];
        }
    }
}

void check_ethernet() {

    /*unsigned long now = millis();

    bool connected = is_ethernet_connected();
    bool ethernet_reconnect_elapsed = (now > require_ethernet_reconnect_time);
    if (connected || ethernet_reconnect_elapsed) {
        require_ethernet_reconnect_time = now + max_ethernet_timeout_until_reconnect;

        if (!connected && ethernet_reconnect_elapsed) {
            Ethernet.begin(ethernet_mac);
        }
    }*/
}

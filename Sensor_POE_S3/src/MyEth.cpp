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
static String ethernet_mac_str;
static bool ethernet_connected = false;
static bool ethernet_ipv6_link_local_ready = false;
static SPIClass ethernet_spi = SPIClass(FSPI);
static char ethernet_link_local_ipv6[40] = {0};

/*
 * Build a deterministic IPv6 link-local address from a 48-bit MAC address
 * using the standard modified EUI-64 method.
 *
 * MAC: aa:bb:cc:dd:ee:ff
 * IID: (aa^0x02):bb:cc:ff:fe:dd:ee:ff
 * IPv6 link-local: fe80::[IID]
 *
 * Link-local prefix is fixed by IPv6; it is not arbitrary.
 */
void ethernet_mac_to_link_local_ipv6(const uint8_t* mac, char* out, size_t out_size)
{
    if (mac == nullptr || out == nullptr || out_size == 0)
    {
        return;
    }

    const uint8_t eui64[8] = {
        static_cast<uint8_t>(mac[0] ^ 0x02),
        mac[1],
        mac[2],
        0xFF,
        0xFE,
        mac[3],
        mac[4],
        mac[5]
    };

    snprintf(
        out,
        out_size,
        "fe80::%02x%02x:%02xff:fe%02x:%02x%02x",
        eui64[0], eui64[1], eui64[2],
        eui64[5], eui64[6], eui64[7]
    );
}

const char* ethernet_get_link_local_ipv6()
{
    return ethernet_link_local_ipv6;
}

bool ethernet_has_link_local_ipv6()
{
    return ethernet_ipv6_link_local_ready;
}

void on_ethernet_event(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //ETH.setHostname("esp32-eth0");

      ethernet_mac_to_link_local_ipv6(ethernet_mac, ethernet_link_local_ipv6, sizeof(ethernet_link_local_ipv6));
      Serial.print("ETH expected link-local IPv6 from MAC: ");
      Serial.println(ethernet_link_local_ipv6);

      if (ETH.enableIPv6()) {
        Serial.println("ETH IPv6 enabled");
      } else {
        Serial.println("ETH failed to enable IPv6");
      }
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH Got IPv4: ");
      Serial.println(ETH.localIP());
      ethernet_connected = true;
      break;

    case ARDUINO_EVENT_ETH_GOT_IP6:
      Serial.print("ETH Got IPv6 link-local: ");
      Serial.println(ETH.linkLocalIPv6());
      Serial.print("ETH Expected MAC-based IPv6: ");
      Serial.println(ethernet_link_local_ipv6);
      ethernet_ipv6_link_local_ready = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      ethernet_connected = false;
      ethernet_ipv6_link_local_ready = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      ethernet_connected = false;
      ethernet_ipv6_link_local_ready = false;
      break;

    default:
      break;
  }
}

static bool parse_mac_address_string()
{
    int colon_count = 0;
    for(int i = 0; i < ethernet_mac_str.length(); ++i)
    {
        colon_count += (ethernet_mac_str[i] == ':');
    }

    if(colon_count != 5)
    {
        return false;
    }

    if (sscanf(ethernet_mac_str.c_str(), "%x:%x:%x:%x:%x:%x",
           &ethernet_mac[0], &ethernet_mac[1], &ethernet_mac[2], 
           &ethernet_mac[3], &ethernet_mac[4], &ethernet_mac[5]) != 6) {
        return false;
    }

    return true;
}

void init_ethernet() {
    WiFi.mode(WIFI_OFF);
    btStop();

    pinMode(W5500_PIN_RST, OUTPUT);
    digitalWrite(W5500_PIN_RST, LOW);
    delay(100);
    digitalWrite(W5500_PIN_RST, HIGH); 
    delay(200);

    // Start SPI with custom pins
    ethernet_spi.begin(W5500_PIN_SCLK, W5500_PIN_MISO, W5500_PIN_MOSI, W5500_PIN_CS);

    // Initialize Ethernet
    pinMode(W5500_PIN_CS, OUTPUT);
    digitalWrite(W5500_PIN_CS, HIGH);

    Network.onEvent(on_ethernet_event);
    if(ETH.begin(
        ETH_PHY_W5500,
        0,
        W5500_PIN_CS,
        W5500_PIN_INT,
        W5500_PIN_RST,
        ethernet_spi
    ))
    {
        ethernet_mac_str = ETH.macAddress();
        parse_mac_address_string();

        static char tmp_hostname[32];
        static String tmp_hostname_str;

        Serial.printf("Using MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
            ethernet_mac[0], ethernet_mac[1], ethernet_mac[2],
            ethernet_mac[3], ethernet_mac[4], ethernet_mac[5]);

        ethernet_mac_to_link_local_ipv6(ethernet_mac, ethernet_link_local_ipv6, sizeof(ethernet_link_local_ipv6));
        Serial.print("Computed fallback link-local IPv6: ");
        Serial.println(ethernet_link_local_ipv6);

        sprintf(tmp_hostname, "%02x%02x%02x%02x%02x%02x",
          ethernet_mac[0], ethernet_mac[1], ethernet_mac[2],
          ethernet_mac[3], ethernet_mac[4], ethernet_mac[5]);
        tmp_hostname_str = tmp_hostname;

        ETH.setHostname(tmp_hostname);

        if (!MDNS.begin("sr-" + tmp_hostname_str + ".local")) {
            Serial.println("Error setting up MDNS responder!");
        }
    }
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

String ethernet_get_mac_str()
{
    return ethernet_mac_str;
}

String ethernet_get_ipv6_address_str()
{
    return ETH.linkLocalIPv6().toString(true);
}

String ethernet_get_local_address_str()
{
    return ETH.localIP().toString(true);
}

void check_ethernet() {

}

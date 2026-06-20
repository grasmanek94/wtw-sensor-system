#pragma once

#include <Arduino.h>
#include <stdint.h>

void init_ethernet();
void check_ethernet();
bool is_ethernet_connected();
void ethernet_get_mac(uint8_t* const mac);
String ethernet_get_mac_str();
String ethernet_get_ipv6_address_str();
String ethernet_get_local_address_str();
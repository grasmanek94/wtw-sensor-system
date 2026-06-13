#pragma once

#include <stdint.h>

void init_ethernet();
void check_ethernet();
bool is_ethernet_connected();
void ethernet_get_mac(uint8_t* const mac);

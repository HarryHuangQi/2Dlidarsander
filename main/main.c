#include <stdio.h>

#include "WIFI.h"
#include "UART.h"
#include "UDP_TCP.h"
#include "LD14.h"
#include "Data_declaration.h"

void app_main(void)
{
	wifi_init_sta();
	UART_init();
	UDP_init();
	init_ok = true;
	LD14_init();
}

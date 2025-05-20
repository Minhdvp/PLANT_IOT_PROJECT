#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void mosfet_init();     // Khởi tạo chân GPIO điều khiển MOSFET
void pump_on();         // Bật máy bơm
void pump_off();        // Tắt máy bơm

#ifdef __cplusplus
}
#endif

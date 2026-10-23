#pragma once
#include <cstdint>
#include <string>
using uint = unsigned int;
inline uint32_t fake_now = 0;
inline std::string fake_uart;
inline int uart0 = 0;
inline constexpr int GPIO_OUT=1, GPIO_FUNC_PWM=2, GPIO_FUNC_UART=3, clk_sys=0;
inline uint32_t get_absolute_time(){return fake_now;}
inline uint32_t to_ms_since_boot(uint32_t t){return t;}
inline bool uart_is_readable(int){return !fake_uart.empty();}
inline char uart_getc(int){char c=fake_uart.front();fake_uart.erase(0,1);return c;}
inline void gpio_put(uint,int){}
inline void gpio_init(uint){}
inline void gpio_set_dir(uint,int){}
inline void gpio_set_function(uint,int){}
inline void pwm_set_gpio_level(uint,uint16_t){}
inline uint pwm_gpio_to_slice_num(uint p){return p/2;}
inline void pwm_set_wrap(uint,uint16_t){}
inline void pwm_set_enabled(uint,bool){}
inline void stdio_init_all(){}
inline uint32_t clock_get_hz(int){return 125000000;}
inline void uart_init(int,int){}

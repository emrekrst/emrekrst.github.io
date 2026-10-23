#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "debug.h" 

#include "protocol.h"

float Kp = 0.5;
float Ki = 0.05;
float Kd = 0.001;

int pid_active = 0; 
int monitor_active = 0; 
int target_adc = 0;       
float integral_sum = 0;   
float prev_error = 0;     
long loop_counter = 0; 

void init_adc() {
    ADCSRA |= (1 << ADEN); 
    ADMUX |= (1 << REFS0); 
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 
}

uint16_t read_adc(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC; 
}

void init_pwm() {
    DDRB |= (1 << PB3); 
    TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1);
    TCCR0B |= (1 << CS00);
}

void set_pwm(uint8_t value) {
    OCR0A = value; 
}

void update_pid() {
    if (!pid_active) return;

    int current_val = read_adc(0);
    float error = target_adc - current_val;
    float derivative = error - prev_error;
    integral_sum += error;

    if (integral_sum > 5000) integral_sum = 5000;
    if (integral_sum < -5000) integral_sum = -5000;

    float output = (Kp * error) + (Ki * integral_sum) + (Kd * derivative);

    prev_error = error;
    
    if (output > 255) output = 255;
    if (output < 0) output = 0;

    set_pwm((uint8_t)output);
}

void process_command(char* input_buffer) {
    command cmd;
    if (!parse_command(input_buffer, &cmd)) {
        printf("ERR invalid command\n"); return;
    }
    switch (cmd.kind) {
        case CMD_SET:
            pid_active = 0; monitor_active = 0;
            set_pwm((uint8_t)cmd.value);
            printf("Manual PWM: %d\n", cmd.value); break;
        case CMD_AUTO:
            pid_active = 1; target_adc = cmd.value;
            integral_sum = 0; prev_error = 0;
            printf("PID ON. Target: %d\n", target_adc); break;
        case CMD_MONITOR:
            monitor_active = 1; loop_counter = 0;
            printf("Monitoring ADC... Press ENTER key to stop.\n"); break;
        case CMD_READ:
            printf("ADC: %u | PWM: %u\n", read_adc(0), (unsigned)OCR0A); break;
        case CMD_STOP:
            monitor_active = 0;
            printf("Monitor stopped\n"); break;
        case CMD_KP: Kp = cmd.value / 1000.0f; printf("KP updated\n"); break;
        case CMD_KI: Ki = cmd.value / 1000.0f; printf("KI updated\n"); break;
        case CMD_KD: Kd = cmd.value / 1000.0f; printf("KD updated\n"); break;
    }
}

int main(void) {
    command_buffer buffer = {{0}, 0, 0};
    
    init_debug_uart0(); 
    init_adc();         
    init_pwm();         
    
    printf("SYSTEM READY\n");

    while(1) {
        if (UCSR0A & (1 << RXC0)) { //STATE REG
            char c = UDR0; 
            
            if (monitor_active) {
                monitor_active = 0;
                printf("\n[Monitor Stopped]\n");
            }
            
            int result = command_feed(&buffer, c);
            if (result == 1) process_command(buffer.text);
            else if (result == -1) printf("ERR command too long\n");
        }

        update_pid();

        if (monitor_active) {
            loop_counter++;
            // 0.1ms * 500 = 50ms
            if (loop_counter >= 500) { 
                int val = read_adc(0);
                printf("ADC: %d | PWM: %d\n", val, OCR0A);
                loop_counter = 0;
            }
        }

        _delay_us(100); 
    }
}
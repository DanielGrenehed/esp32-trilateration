#ifndef TRILAT_LED_H
#define TRILAT_LED_h

void led_init();
void led_toggle();
void led_set_on();
void led_set_off();
void on_led_command(const char*);

#endif /* TRILAT_LED_H */
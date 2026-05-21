#include "encoder.hpp"

#include "config.hpp"
#include "defs.hpp"

#include "driver/pcnt.h"

void encoder_init() {
    gpio_set_pull_mode((gpio_num_t)ENC1_A, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)ENC1_B, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)ENC2_A, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode((gpio_num_t)ENC2_B, GPIO_PULLUP_ONLY);

    pcnt_config_t pcnt_config1 = {};
    pcnt_config1.pulse_gpio_num = ENC1_A;
    pcnt_config1.ctrl_gpio_num = ENC1_B;
    pcnt_config1.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config1.hctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config1.pos_mode = PCNT_COUNT_INC;
    pcnt_config1.neg_mode = PCNT_COUNT_DEC;
    pcnt_config1.counter_h_lim = 32767;
    pcnt_config1.counter_l_lim = -32768;
    pcnt_config1.unit = PCNT_UNIT_0;
    pcnt_config1.channel = PCNT_CHANNEL_0;

    pcnt_config_t pcnt_config2 = {};
    pcnt_config2.pulse_gpio_num = ENC1_B;
    pcnt_config2.ctrl_gpio_num = ENC1_A;
    pcnt_config2.lctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config2.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_config2.pos_mode = PCNT_COUNT_INC;
    pcnt_config2.neg_mode = PCNT_COUNT_DEC;
    pcnt_config2.counter_h_lim = 32767;
    pcnt_config2.counter_l_lim = -32768;
    pcnt_config2.unit = PCNT_UNIT_0;
    pcnt_config2.channel = PCNT_CHANNEL_1;

    pcnt_config_t pcnt_config3 = {};
    pcnt_config3.pulse_gpio_num = ENC2_A;
    pcnt_config3.ctrl_gpio_num = ENC2_B;
    pcnt_config3.lctrl_mode = PCNT_MODE_KEEP;
    pcnt_config3.hctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config3.pos_mode = PCNT_COUNT_INC;
    pcnt_config3.neg_mode = PCNT_COUNT_DEC;
    pcnt_config3.counter_h_lim = 32767;
    pcnt_config3.counter_l_lim = -32768;
    pcnt_config3.unit = PCNT_UNIT_1;
    pcnt_config3.channel = PCNT_CHANNEL_0;

    pcnt_config_t pcnt_config4 = {};
    pcnt_config4.pulse_gpio_num = ENC2_B;
    pcnt_config4.ctrl_gpio_num = ENC2_A;
    pcnt_config4.lctrl_mode = PCNT_MODE_REVERSE;
    pcnt_config4.hctrl_mode = PCNT_MODE_KEEP;
    pcnt_config4.pos_mode = PCNT_COUNT_INC;
    pcnt_config4.neg_mode = PCNT_COUNT_DEC;
    pcnt_config4.counter_h_lim = 32767;
    pcnt_config4.counter_l_lim = -32768;
    pcnt_config4.unit = PCNT_UNIT_1;
    pcnt_config4.channel = PCNT_CHANNEL_1;

    pcnt_unit_config(&pcnt_config1);
    pcnt_unit_config(&pcnt_config2);
    pcnt_unit_config(&pcnt_config3);
    pcnt_unit_config(&pcnt_config4);

    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_pause(PCNT_UNIT_1);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_1);
    pcnt_counter_resume(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_1);

    pcnt_filter_enable(PCNT_UNIT_0);
    pcnt_filter_enable(PCNT_UNIT_1);
    pcnt_set_filter_value(PCNT_UNIT_0, PCNT_FILTER_VALUE);
    pcnt_set_filter_value(PCNT_UNIT_1, PCNT_FILTER_VALUE);
}

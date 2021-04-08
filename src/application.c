// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include <application.h>

#define TMP112_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define TMP112_PUB_VALUE_CHANGE 0.2f
#define TMP112_UPDATE_INTERVAL (2 * 1000)

#define CO2_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define CO2_PUB_VALUE_CHANGE 50.0f
#define CO2_UPDATE_INTERVAL (1 * 60 * 1000)


// LED instance
twr_led_t led;

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event != TWR_TMP112_EVENT_UPDATE)
    {
        return;
    }

    if (twr_tmp112_get_temperature_celsius(self, &value))
    {
        if ((fabs(value - param->value) >= TMP112_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
        {
            twr_radio_pub_temperature(param->channel, &value);
            twr_log_debug("APP: temperature: %.2f °C", value);

            param->value = value;
            param->next_pub = twr_scheduler_get_spin_tick() + TMP112_PUB_NO_CHANGE_INTERVAL;

            //values.temperature = value;
            twr_scheduler_plan_now(0);
        }
    }
}

void co2_event_handler(twr_module_co2_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *) event_param;
    float value;

    if (event == TWR_MODULE_CO2_EVENT_UPDATE)
    {
        if (twr_module_co2_get_concentration_ppm(&value))
        {
            if ((fabs(value - param->value) >= CO2_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
            {
                twr_radio_pub_co2(&value);
                twr_log_debug("APP: CO2: %.0f ppm", value);
                param->value = value;
                param->next_pub = twr_scheduler_get_spin_tick() + CO2_PUB_NO_CHANGE_INTERVAL;

                //values.co2_concentation = value;
                twr_scheduler_plan_now(0);
            }
        }
    }
}





// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, 0);
    twr_led_pulse(&led, 2000);

    // Temperature
    static twr_tmp112_t temperature;
    static event_param_t temperature_event_param = { .next_pub = 0, .channel = TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE };
    twr_tmp112_init(&temperature, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&temperature, tmp112_event_handler, &temperature_event_param);
    twr_tmp112_set_update_interval(&temperature, TMP112_UPDATE_INTERVAL);

    // CO2
    static event_param_t co2_event_param = { .next_pub = 0 };
    twr_module_co2_init();
    twr_module_co2_set_update_interval(CO2_UPDATE_INTERVAL);
    twr_module_co2_set_event_handler(co2_event_handler, &co2_event_param);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    // Send radio pairing request
    twr_radio_pairing_request("room", VERSION);
}
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

#define VOC_LP_TAG_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define VOC_LP_TAG_PUB_VALUE_CHANGE 1.0f
#define VOC_LP_TAG_UPDATE_INTERVAL (5 * 1000)

#define HUMIDITY_TAG_PUB_NO_CHANGE_INTERVAL (15 * 60 * 1000)
#define HUMIDITY_TAG_PUB_VALUE_CHANGE 5.0f
#define HUMIDITY_TAG_UPDATE_INTERVAL (2 * 1000)

static struct
{
    float_t temperature;
    float_t humidity;
    float_t tvoc;
    float_t pressure;
    float_t altitude;
    float_t co2_concentation;
    float_t battery_voltage;
    float_t battery_pct;

} values;


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

            values.temperature = value;
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

                values.co2_concentation = value;
                twr_scheduler_plan_now(0);
            }
        }
    }
}

void voc_lp_tag_event_handler(twr_tag_voc_lp_t *self, twr_tag_voc_lp_event_t event, void *event_param)
{
    event_param_t *param = (event_param_t *)event_param;

    if (event == TWR_TAG_VOC_LP_EVENT_UPDATE)
    {
        uint16_t value;

        if (twr_tag_voc_lp_get_tvoc_ppb(self, &value))
        {
            if ((fabsf(value - param->value) >= VOC_LP_TAG_PUB_VALUE_CHANGE) || (param->next_pub < twr_scheduler_get_spin_tick()))
            {
                param->value = value;
                param->next_pub = twr_scheduler_get_spin_tick() + VOC_LP_TAG_PUB_NO_CHANGE_INTERVAL;

                int radio_tvoc = value;

                values.tvoc = radio_tvoc;

                twr_radio_pub_int("voc-lp-sensor/0:0/tvoc", &radio_tvoc);
                twr_log_debug("APP: VOC: %d ppb", radio_tvoc);
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

    // VOC-LP
    static twr_tag_voc_lp_t voc_lp;
    static event_param_t voc_lp_event_param = { .next_pub = 0 };
    twr_tag_voc_lp_init(&voc_lp, TWR_I2C_I2C0);
    twr_tag_voc_lp_set_event_handler(&voc_lp, voc_lp_tag_event_handler, &voc_lp_event_param);
    twr_tag_voc_lp_set_update_interval(&voc_lp, VOC_LP_TAG_UPDATE_INTERVAL);


    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    // Send radio pairing request
    twr_radio_pairing_request("room", VERSION);
}
void application_task(void){
    bool limit = false;
    if (values.temperature>25.0){
        limit = true;
    }
    if (values.co2_concentation>1000.0){
        limit = true;
    }
    if (values.tvoc>60.0){
        limit = true;
    }

    if (limit)
        twr_led_set_mode(&led,TWR_LED_MODE_ON);
    else
        twr_led_set_mode(&led,TWR_LED_MODE_OFF);


    twr_scheduler_plan_current_relative(1000);
}
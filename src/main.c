#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/drivers/adc.h>
#include <dk_buttons_and_leds.h>
#include "bthome.h"

#define COMPANY_ID_CODE 0xfcd2
#define ADV_TIMEOUT_MS	5000
#define BATT_ALKALINE_VOLTAGE_MV_FULL	(2400u)		// [mv]
#define BATT_ALKALINE_VOLTAGE_MV_EMPTY	(2000u)		// [mV]


static void adv_timer_handler(struct k_timer *timer);
static void my_work_handler(struct k_work *work);
static int update_battery(void);

K_TIMER_DEFINE(adv_timer, adv_timer_handler, NULL);
K_WORK_DEFINE(my_work, my_work_handler);

typedef struct blitch_packet {
	uint8_t bthome_id;			// BTHome Device Information (0x40)
	uint8_t packet_id_type;		// (0x00)
	uint8_t packet_id;			// packet counter to filter duplicate events
	uint8_t battery_id_type;	// battery state of charge type
	uint8_t battery;			// [%]
	uint8_t button1_id_type;	// event type - button (0x3a)
	uint8_t button1_event;		// 0 - no event, 1 - press
	uint8_t button2_id_type;	// event type - button (0x3a)
	uint8_t button2_event;		// 0 - no event, 1 - press
	uint8_t button3_id_type;	// event type - button (0x3a)
	uint8_t button3_event;		// 0 - no event, 1 - press
	uint8_t button4_id_type;	// event type - button (0x3a)
	uint8_t button4_event;		// 0 - no event, 1 - press

} blitch_packet_t;

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
static int16_t adc_buf;
static struct adc_sequence adc_seq = {
	.buffer = &adc_buf,
	.buffer_size = sizeof(adc_buf),
	// optional .calibrate = true,
};

/* Declare the structure for your custom data  */
typedef struct adv_mfg_data {
	uint16_t company_code; /* Company Identifier Code. */
	blitch_packet_t blitch_packet;
} adv_mfg_data_type;

#define USER_BUTTONS (DK_BTN1_MSK | DK_BTN2_MSK | DK_BTN3_MSK | DK_BTN4_MSK)

/* Create an LE Advertising Parameters variable */
static struct bt_le_adv_param *adv_param =
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY  | 		// static address
					BT_LE_ADV_OPT_CODED , 				// coded PHY (long range)
			100, /* Min Advertising Interval 100ms (100*0.625ms) */
			101, /* Max Advertising Interval 100.625ms (101*0.625ms) */
			NULL); /* Set to NULL for undirected advertising */

/* Define and initialize a variable of type adv_mfg_data_type */
static adv_mfg_data_type adv_mfg_data = { COMPANY_ID_CODE, {
											.bthome_id = BTHOME_DEVICE_PACKET_V2 | BTHOME_DEVICE_ADV_IRREGULAR | BTHOME_DEVICE_ENCRYPTION_OFF,
											.packet_id_type = BTHOME_PACKET_ID,
											.packet_id = 0,
											.battery_id_type = BTHOME_SENSOR_BATTERY,
											.battery = 0,
											.button1_id_type = BTHOME_EVENT_TYPE_BUTTON,
											.button1_event = 0,
											.button2_id_type = BTHOME_EVENT_TYPE_BUTTON,
											.button2_event = 0,
											.button3_id_type = BTHOME_EVENT_TYPE_BUTTON,
											.button3_event = 0,
											.button4_id_type = BTHOME_EVENT_TYPE_BUTTON,
											.button4_event = 0,
											}};

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	/* Include the Manufacturer Specific Data in the advertising packet. */
	// BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
	BT_DATA(BT_DATA_SVC_DATA16, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

/**
 * @brief Start advertising and timer for disabling it after defined time
 * 
 * @return int 0 - OK
 */
static int start_adv_timeout_timer(void)
{
	int err;

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) 
	{
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return err;
	}

	k_timer_start(&adv_timer, K_MSEC(ADV_TIMEOUT_MS), K_NO_WAIT);
	LOG_INF("Advertising successfully started");

	return 0;
}

static void adv_timer_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired, calling work queue");
	k_work_submit(&my_work);
}

static void my_work_handler(struct k_work *work)
{
	LOG_INF("Disabling BLE advertising");
	bt_le_adv_stop();
}

static void print_adv(void)
{
	LOG_INF("ID: 0x%02x", adv_mfg_data.blitch_packet.bthome_id);
	LOG_INF("Packet counter: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.packet_id_type, adv_mfg_data.blitch_packet.packet_id);
	LOG_INF("Battery: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.battery_id_type, adv_mfg_data.blitch_packet.battery);
	LOG_INF("Button 1: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.button1_id_type, adv_mfg_data.blitch_packet.button1_event);
	LOG_INF("Button 2: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.button2_id_type, adv_mfg_data.blitch_packet.button2_event);
	LOG_INF("Button 3: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.button3_id_type, adv_mfg_data.blitch_packet.button3_event);
	LOG_INF("Button 4: 0x%02x, 0x%02x", adv_mfg_data.blitch_packet.button4_id_type, adv_mfg_data.blitch_packet.button4_event);
}

/* Add the definition of callback function and update the advertising data dynamically */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTONS) 
	{
		if (button_state)		// if any button is pressed
		{
			bt_le_adv_stop();
			update_battery();

			LOG_INF("Button pressed - state: 0x%04x", button_state);

			adv_mfg_data.blitch_packet.button1_event = button_state & DK_BTN1_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button2_event = button_state & DK_BTN2_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button3_event = button_state & DK_BTN3_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button4_event = button_state & DK_BTN4_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;

			start_adv_timeout_timer();

			// adv_mfg_data.blitch_packet.button1_event = 0x01;			// always send button press event but with every press increment packet id
			++adv_mfg_data.blitch_packet.packet_id;

			print_adv();
			bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
		}
	}
}
/* Define the initialization function of the buttons and setup interrupt.  */
static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}

static int init_adc(void)
{
	int err;

	if (!adc_is_ready_dt(&adc_channel)) {
		LOG_ERR("ADC controller devivce %s not ready", adc_channel.dev->name);
		return -EIO;
	}

	err = adc_channel_setup_dt(&adc_channel);
	if (err < 0)
	{
		LOG_ERR("Could not setup ADC channel #0 (err: %d)", err);
		return err;
	}

	err = adc_sequence_init_dt(&adc_channel, &adc_seq);
	if (err)
	{
		LOG_ERR("Could not initialize ADC sequence (err: %d)", err);
		return err;
	}

	return 0;
}

static int get_battery_voltage(uint32_t *voltage_mv)
{
	int err;
	int battery_voltage_mv;

	err = adc_read(adc_channel.dev, &adc_seq);
	if (err)
	{
		LOG_ERR("Could not read from ADC (err: %d)", err);
		return err;
	}

	// to prevent the battery voltage from becoming negative number
	battery_voltage_mv = adc_buf > 0 ? adc_buf : 0;

	err = adc_raw_to_millivolts_dt(&adc_channel, &battery_voltage_mv);
	if (err < 0)
	{
		LOG_WRN("Cannot convert ADC value to milivolts");
		*voltage_mv = 0;
		return -EIO;
	}
	
	LOG_INF("Battery voltage: %d mV", battery_voltage_mv);
	*voltage_mv = battery_voltage_mv;
	return 0;
}

/**
 * @brief Update battery state in BLE adverising data
 * 
 * @return int 0 - OK
 */
static int update_battery(void)
{
	uint32_t battery_voltage_mv;
	uint8_t percentage = 0;
	int err;

	err = get_battery_voltage(&battery_voltage_mv);
	if (err)
	{
		return err;
	}

	// for 2xAAA batteries
	if (battery_voltage_mv >= BATT_ALKALINE_VOLTAGE_MV_FULL)
	{
		percentage = 100;
	}
	else if (battery_voltage_mv <= BATT_ALKALINE_VOLTAGE_MV_EMPTY)
	{
		percentage = 0;
	}
	else
	{
		percentage = ( 100 * (battery_voltage_mv - BATT_ALKALINE_VOLTAGE_MV_EMPTY) ) / (BATT_ALKALINE_VOLTAGE_MV_FULL - BATT_ALKALINE_VOLTAGE_MV_EMPTY);
	}

	LOG_INF("Battery: %d %%", percentage);
	adv_mfg_data.blitch_packet.battery = percentage;

	return 0;
}

int main(void)
{
	int err;

	LOG_INF("BThome Buttons\n");

	err = dk_leds_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return err;
	}

	/* Setup buttons on your board  */
	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return err;
	}

	// Setup ADC
	err = init_adc();
	if (err)
	{
		LOG_ERR("ADC initialization failed!");
		return err;
	}

	err = update_battery();
	if (err)
	{
		LOG_ERR("Failed to update battery %%");
		return err;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	LOG_INF("Bluetooth initialized\n");

	print_adv();
	err = start_adv_timeout_timer();
	if (err)
	{
		LOG_ERR("Failed to start advertising!");
		return 0;
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

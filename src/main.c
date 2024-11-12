#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <dk_buttons_and_leds.h>
#include "bthome.h"

#define COMPANY_ID_CODE 0xfcd2
#define ADV_TIMEOUT_MS	5000

void adv_timer_handler(struct k_timer *timer);
void my_work_handler(struct k_work *work);

K_TIMER_DEFINE(adv_timer, adv_timer_handler, NULL);
K_WORK_DEFINE(my_work, my_work_handler);

typedef struct blitch_packet {
	uint8_t bthome_id;			// BTHome Device Information (0x40)
	uint8_t packet_id_type;		// (0x00)
	uint8_t packet_id;			// packet counter to filter duplicate events
	uint8_t button1_id_type;	// event type - button (0x3a)
	uint8_t button1_event;		// 0 - no event, 1 - press
	uint8_t button2_id_type;	// event type - button (0x3a)
	uint8_t button2_event;		// 0 - no event, 1 - press
	uint8_t button3_id_type;	// event type - button (0x3a)
	uint8_t button3_event;		// 0 - no event, 1 - press
	uint8_t button4_id_type;	// event type - button (0x3a)
	uint8_t button4_event;		// 0 - no event, 1 - press

} blitch_packet_t;

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
											.bthome_id = 0x40,
											.packet_id_type = BTHOME_PACKET_ID,
											.packet_id = 0,
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

static void start_adv_timeout_timer(void)
{
	k_timer_start(&adv_timer, K_MSEC(ADV_TIMEOUT_MS), K_NO_WAIT);
}

void adv_timer_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired, calling work queue");
	k_work_submit(&my_work);
}

void my_work_handler(struct k_work *work)
{
	LOG_INF("Disabling BLE advertising");
	bt_le_adv_stop();
}

/* Add the definition of callback function and update the advertising data dynamically */
static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTONS) 
	{
		if (button_state)		// if any button is pressed
		{
			bt_le_adv_stop();

			LOG_INF("Button pressed - state: 0x%04x", button_state);

			adv_mfg_data.blitch_packet.button1_event = button_state & DK_BTN1_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button2_event = button_state & DK_BTN2_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button3_event = button_state & DK_BTN3_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;
			adv_mfg_data.blitch_packet.button4_event = button_state & DK_BTN4_MSK ? BTHOME_EVENT_BUTTON_PRESS : BTHOME_EVENT_BUTTON_NONE;

			start_adv_timeout_timer();

			// adv_mfg_data.blitch_packet.button1_event = 0x01;			// always send button press event but with every press increment packet id
			++adv_mfg_data.blitch_packet.packet_id;

			// bt_le_adv_update_data(ad, ARRAY_SIZE(ad), NULL, 0);
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

int main(void)
{
	int err;

	LOG_INF("Starting Lesson 2 - Exercise 2 \n");

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

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return err;
	}

	LOG_INF("Bluetooth initialized\n");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return err;
	}

	LOG_INF("Advertising successfully started\n");

	start_adv_timeout_timer();

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

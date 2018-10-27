/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2018 Bryan Cole <bryan.cole@acsp.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "protocol.h"

#define SERIALCOMM "115200/8n1"

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
	SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
	SR_CONF_CAPTURE_RATIO | SR_CONF_GET | SR_CONF_SET,
	SR_CONF_EXTERNAL_CLOCK | SR_CONF_SET,
	SR_CONF_PATTERN_MODE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_SWAP | SR_CONF_SET,
	SR_CONF_GET | SR_CONF_SET,
};

static const int32_t trigger_matches[] = {
	SR_TRIGGER_ZERO,
	SR_TRIGGER_ONE,
};

#define STR_PATTERN_NONE     "None"
#define STR_PATTERN_EXTERNAL "External"
#define STR_PATTERN_INTERNAL "Internal"

/* Supported methods of test pattern outputs */
enum {
	/**
	 * Capture pins 31:16 (unbuffered wing) output a test pattern
	 * that can captured on pins 0:15.
	 */
	PATTERN_EXTERNAL,

	/** Route test pattern internally to capture buffer. */
	PATTERN_INTERNAL,
};

static const char *patterns[] = {
	STR_PATTERN_NONE,
	STR_PATTERN_EXTERNAL,
	STR_PATTERN_INTERNAL,
};

/* Channels are numbered 0-31 (on the PCB silkscreen). */
SR_PRIV const char *acsp_channel_names[] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
	"13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23",
	"24", "25", "26", "27", "28", "29", "30", "31",
};

/* Default supported samplerates, can be overridden by device metadata. */
static const uint64_t samplerates[] = {
	SR_HZ(10),
	SR_MHZ(200),
	SR_HZ(1),
};

#define RESPONSE_DELAY_US (10 * 1000)

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	// struct drv_context *drvc;
	// GSList *devices;

	// (void)options;

	// devices = NULL;
	// drvc = di->context;
	// drvc->instances = NULL;

	/* TODO: scan for devices, either based on a SR_CONF_CONN option
	 * or on a USB scan. */


	// return devices;

	/*----------above is the given code, below is from OLS----------*/

	sr_dbg("Entering ACSP Scan");
	struct sr_config *src;
	struct sr_dev_inst *sdi;
	struct sr_serial_dev_inst *serial;
	GSList *l;
	int ret;
	unsigned int i;
	const char *conn, *serialcomm;
	char buf[8];

	conn = serialcomm = NULL;
	for (l = options; l; l = l->next) {
		src = l->data;
		switch (src->key) {
		case SR_CONF_CONN:
			conn = g_variant_get_string(src->data, NULL);
			break;
		case SR_CONF_SERIALCOMM:
			serialcomm = g_variant_get_string(src->data, NULL);
			break;
		}
	}
	if (!conn)
		return NULL;

	if (!serialcomm)
		serialcomm = SERIALCOMM;

	serial = sr_serial_dev_inst_new(conn, serialcomm);

	/* The discovery procedure is like this: first send the Reset
	 * command (0x00) 5 times, since the device could be anywhere
	 * in a 5-byte command. Then send the ID command (0x02).
	 * If the device responds with 4 bytes ("OLS1" or "SLA1"), we
	 * have a match.
	 */
	sr_info("Probing %s.", conn);
	if (serial_open(serial, SERIAL_RDWR) != SR_OK)
		return NULL;

	// if (ols_send_reset(serial) != SR_OK) {
	// 	serial_close(serial);
	// 	sr_err("Could not use port %s. Quitting.", conn);
	// 	return NULL;
	// }
	// send_shortcommand(serial, CMD_ID);

	// g_usleep(RESPONSE_DELAY_US);

	// if (sp_input_waiting(serial->data) == 0) {
	// 	sr_dbg("Didn't get any reply.");
	// 	return NULL;
	// }

	// ret = serial_read_blocking(serial, buf, 4, serial_timeout(serial, 4));
	// if (ret != 4) {
	// 	sr_err("Invalid reply (expected 4 bytes, got %d).", ret);
	// 	return NULL;
	// }

	// if (strncmp(buf, "1SLO", 4) && strncmp(buf, "1ALS", 4)) {
	// 	sr_err("Invalid reply (expected '1SLO' or '1ALS', got "
	// 	       "'%c%c%c%c').", buf[0], buf[1], buf[2], buf[3]);
	// 	return NULL;
	// }

	// /* Definitely using the OLS protocol, check if it supports
	//  * the metadata command.
	//  */
	// send_shortcommand(serial, CMD_METADATA);

	// g_usleep(RESPONSE_DELAY_US);

	// if (sp_input_waiting(serial->data) != 0) {
	// 	/* Got metadata. */
	// 	sdi = get_metadata(serial);
	// } else {
	// 	/* Not an OLS -- some other board that uses the sump protocol. */
	// 	sr_info("Device does not support metadata.");
	// 	sdi = g_malloc0(sizeof(struct sr_dev_inst));
	// 	sdi->status = SR_ST_INACTIVE;
	// 	sdi->vendor = g_strdup("Sump");
	// 	sdi->model = g_strdup("Logic Analyzer");
	// 	sdi->version = g_strdup("v1.0");
	// 	for (i = 0; i < ARRAY_SIZE(ols_channel_names); i++)
	// 		sr_channel_new(sdi, i, SR_CHANNEL_LOGIC, TRUE,
	// 				ols_channel_names[i]);
	// 	sdi->priv = ols_dev_new();
	// }
	// /* Configure samplerate and divider. */
	// if (ols_set_samplerate(sdi, DEFAULT_SAMPLERATE) != SR_OK)
	// 	sr_dbg("Failed to set default samplerate (%"PRIu64").",
	// 			DEFAULT_SAMPLERATE);
	sdi->inst_type = SR_INST_SERIAL;
	sdi->conn = serial;

	serial_close(serial);

	return std_scan_complete(di, g_slist_append(NULL, sdi));

}


static int dev_open(struct sr_dev_inst *sdi)
{
	(void)sdi;

	/* TODO: get handle from sdi->conn and open it. */

	return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
	(void)sdi;

	/* TODO: get handle from sdi->conn and close it. */

	return SR_OK;
}

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	/* TODO: configure hardware, reset acquisition state, set up
	 * callbacks and send header packet. */

	(void)sdi;

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	/* TODO: stop acquisition. */

	(void)sdi;

	return SR_OK;
}

SR_PRIV struct sr_dev_driver acsp_driver_info = {
	.name = "acsp",
	.longname = "acsp",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = dev_open,
	.dev_close = dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.context = NULL,
};

SR_REGISTER_DEV_DRIVER(acsp_driver_info);

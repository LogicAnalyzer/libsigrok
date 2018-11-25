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
};

static const int32_t trigger_matches[] = {
	SR_TRIGGER_RISING,
	SR_TRIGGER_FALLING,
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
	"0", "1", "2", "3", "4", "5", "6", "7",
};

/* Default supported samplerates, can be overridden by device metadata. */
static const uint64_t samplerates[] = {
	SR_HZ(10),
	SR_MHZ(200),
	SR_HZ(1),
};

#define RESPONSE_DELAY_US (1000 * 10)

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	// struct drv_context *drvc;
	// GSList *devices;

	// (void)options;

	// devices = NULL;
	// drvc = di->context;
	// drvc->instances = NULL;

	/* TODO: scan for devices, either based on a SR_CONF_CONN optionff
	 * or on a USB scan. */


	// return devices;

	/*----------above is the given code, below is from O L S----------*/

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
			sr_dbg("entered SR_CONF_CONN");
			conn = g_variant_get_string(src->data, NULL);
			break;
		case SR_CONF_SERIALCOMM:
			sr_dbg("entered SR_CONF_SERIALCONN");
			serialcomm = g_variant_get_string(src->data, NULL);
			break;
		}
	}
	if (!conn)
		return NULL;

	if (!serialcomm)
		serialcomm = SERIALCOMM;

	sr_dbg("Serial communication: %s", serialcomm);

	serial = sr_serial_dev_inst_new(conn, serialcomm);

	/* The discovery procedure is like this: first send the Reset
	 * command (0x00) 5 times, since the device could be anywhere
	 * in a 5-byte command. Then send the ID command (0x02).
	 * If the device responds with 4 bytes ("acsp1" or "SLA1"), we
	 * have a match.
	 */
	sr_info("Probing %s.", conn);
	if (serial_open(serial, SERIAL_RDWR) != SR_OK)
		return NULL;

	if (acsp_send_reset(serial) != SR_OK) {
		serial_close(serial);
		sr_err("Could not use port %s. Quitting.", conn);
		return NULL;
	}
	if (acsp_send_id_request(serial) != SR_OK){
		serial_close(serial);
		sr_err("Request not recieved");
	}
	
	g_usleep(RESPONSE_DELAY_US * 2);

	if (sp_input_waiting(serial->data) == 0) {
		sr_dbg("TEMP: Didn't get any reply.");
	}

	ret = serial_read_blocking(serial, buf, 4, serial_timeout(serial, 4));
	if (ret != 4) {
		sr_err("Invalid reply (expected 4 bytes, got %d).", ret);
		return NULL;
	}

    sr_dbg("Returned value: = %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
	if (strncmp(buf, "ACSP", 4) && strncmp(buf, "ACSP", 4)) {
		sr_err("Invalid reply (expected 'ACSP' or 'ACSP', got "
		       "'%c%c%c%c').", buf[0], buf[1], buf[2], buf[3]);
		return NULL;
	}

	/* Definitely using the acsp protocol, check if it supports
	 * the metadata command.
	 */
    if (acsp_send_metadata_request(serial) != SR_OK){
    	sr_dbg("The metadata command didn't send");
    }

	g_usleep(RESPONSE_DELAY_US);

	if (sp_input_waiting(serial->data) != 0) {
		/* Got metadata. */
		sdi = acsp_get_metadata(serial);
	} else {
		/* Not an acsp -- some other board that uses the sump protocol. */
		sr_info("Device does not support metadata.");
		sdi = g_malloc0(sizeof(struct sr_dev_inst));
		sdi->status = SR_ST_INACTIVE;
		sdi->vendor = g_strdup("Sump");
		sdi->model = g_strdup("Logic Analyzer");
		sdi->version = g_strdup("v1.0");
		for (i = 0; i < ARRAY_SIZE(acsp_channel_names); i++)
			sr_channel_new(sdi, i, SR_CHANNEL_LOGIC, TRUE,
					acsp_channel_names[i]);
		sdi->priv = acsp_dev_new();
	}
	/* Configure samplerate and divider. */
	if (acsp_set_samplerate(sdi, DEFAULT_SAMPLERATE) != SR_OK)
		sr_dbg("Failed to set default samplerate (%"PRIu64").",
				DEFAULT_SAMPLERATE);
	sdi->inst_type = SR_INST_SERIAL;
	sdi->conn = serial;

	serial_close(serial);

	sr_dbg("Exiting ACSP Scan after running \"std_scan_complete\"\n ");

	return std_scan_complete(di, g_slist_append(NULL, sdi));

}


// static int dev_open(struct sr_dev_inst *sdi)
// {
// 	// (void)sdi;

// 	// /* TODO: get handle from sdi->conn and open it. */

// 	// return SR_OK;

// 	/*----------above is the given code, below is from O L S----------*/
// 	// not implemented
// }

// static int dev_close(struct sr_dev_inst *sdi)
// {
// 	// (void)sdi;

// 	// /* TODO: get handle from sdi->conn and close it. */

// 	// return SR_OK;

// 	/*----------above is the given code, below is from O L S----------*/
// 	// not implemented
// }

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_dbg("Entering config get");
	// int ret;

	// (void)sdi;
	// (void)data;
	// (void)cg;

	// ret = SR_OK;
	// switch (key) {
	// /* TODO */
	// default:
	// 	return SR_ERR_NA;
	// }

	// return ret;

	/*----------above is the given code, below is from O L S----------*/

	struct dev_context *devc;

	(void)cg;

	if (!sdi)
		return SR_ERR_ARG;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_SAMPLERATE:
		*data = g_variant_new_uint64(devc->cur_samplerate);
		break;
	case SR_CONF_CAPTURE_RATIO:
		*data = g_variant_new_uint64(devc->capture_ratio);
		break;
	case SR_CONF_LIMIT_SAMPLES:
		*data = g_variant_new_uint64(devc->limit_samples);
		break;
	case SR_CONF_PATTERN_MODE:
		if (devc->flag_reg & FLAG_EXTERNAL_TEST_MODE)
			*data = g_variant_new_string(STR_PATTERN_EXTERNAL);
		else if (devc->flag_reg & FLAG_INTERNAL_TEST_MODE)
			*data = g_variant_new_string(STR_PATTERN_INTERNAL);
		else
			*data = g_variant_new_string(STR_PATTERN_NONE);
		break;
	case SR_CONF_RLE:
		*data = g_variant_new_boolean(devc->flag_reg & FLAG_RLE ? TRUE : FALSE);
		break;
	default:
		return SR_ERR_NA;
	}
	sr_dbg("Exiting config get gracefully");
	return SR_OK;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_dbg("Entering config set");
	// int ret;

	// (void)sdi;
	// (void)data;
	// (void)cg;

	// ret = SR_OK;
	// switch (key) {
	// /* TODO */
	// default:
	// 	ret = SR_ERR_NA;
	// }

	// return ret;

	/*----------above is the given code, below is from O L S----------*/
	struct dev_context *devc;
	uint16_t flag;
	uint64_t tmp_u64;
	const char *stropt;

	(void)cg;

	devc = sdi->priv;

	switch (key) {
	case SR_CONF_SAMPLERATE:
		tmp_u64 = g_variant_get_uint64(data);
		if (tmp_u64 < samplerates[0] || tmp_u64 > samplerates[1])
			return SR_ERR_SAMPLERATE;
		return acsp_set_samplerate(sdi, g_variant_get_uint64(data));
	case SR_CONF_LIMIT_SAMPLES:
		tmp_u64 = g_variant_get_uint64(data);
		if (tmp_u64 < MIN_NUM_SAMPLES)
			return SR_ERR;
		devc->limit_samples = tmp_u64;
		break;
	case SR_CONF_CAPTURE_RATIO:
		devc->capture_ratio = g_variant_get_uint64(data);
		break;
	case SR_CONF_EXTERNAL_CLOCK:
		if (g_variant_get_boolean(data)) {
			sr_info("Enabling external clock.");
			devc->flag_reg |= FLAG_CLOCK_EXTERNAL;
		} else {
			sr_info("Disabled external clock.");
			devc->flag_reg &= ~FLAG_CLOCK_EXTERNAL;
		}
		break;
	case SR_CONF_PATTERN_MODE:
		stropt = g_variant_get_string(data, NULL);
		if (!strcmp(stropt, STR_PATTERN_NONE)) {
			sr_info("Disabling test modes.");
			flag = 0x0000;
		} else if (!strcmp(stropt, STR_PATTERN_INTERNAL)) {
			sr_info("Enabling internal test mode.");
			flag = FLAG_INTERNAL_TEST_MODE;
		} else if (!strcmp(stropt, STR_PATTERN_EXTERNAL)) {
			sr_info("Enabling external test mode.");
			flag = FLAG_EXTERNAL_TEST_MODE;
		} else {
			return SR_ERR;
		}
		devc->flag_reg &= ~FLAG_INTERNAL_TEST_MODE;
		devc->flag_reg &= ~FLAG_EXTERNAL_TEST_MODE;
		devc->flag_reg |= flag;
		break;
	case SR_CONF_SWAP:
		if (g_variant_get_boolean(data)) {
			sr_info("Enabling channel swapping.");
			devc->flag_reg |= FLAG_SWAP_CHANNELS;
		} else {
			sr_info("Disabling channel swapping.");
			devc->flag_reg &= ~FLAG_SWAP_CHANNELS;
		}
		break;
	case SR_CONF_RLE:
		if (g_variant_get_boolean(data)) {
			sr_info("Enabling RLE.");
			devc->flag_reg |= FLAG_RLE;
		} else {
			sr_info("Disabling RLE.");
			devc->flag_reg &= ~FLAG_RLE;
		}
		break;
	default:
		return SR_ERR_NA;
	}
	sr_dbg("Exiting config set gracefully");
	return SR_OK;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	sr_dbg("Entering config list");
	// int ret;

	// (void)sdi;
	// (void)data;
	// (void)cg;

	// ret = SR_OK;
	// switch (key) {
	// /* TODO */
	// default:
	// 	return SR_ERR_NA;
	// }

	// return ret;

	/*----------above is the given code, below is from O L S----------*/

	struct dev_context *devc;
	int num_acsp_changrp, i;

	switch (key) {
	case SR_CONF_SCAN_OPTIONS:
	case SR_CONF_DEVICE_OPTIONS:
		sr_dbg("SR_CONF_DEVICE_OPTIONS");
		return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
	case SR_CONF_SAMPLERATE:
		sr_dbg("SR_CONF_SAMPLERATE");
		*data = std_gvar_samplerates_steps(ARRAY_AND_SIZE(samplerates));
		break;
	case SR_CONF_TRIGGER_MATCH:
		sr_dbg("SR_CONF_TRIGGER_MATCH");
		*data = std_gvar_array_i32(ARRAY_AND_SIZE(trigger_matches));
		break;
	case SR_CONF_PATTERN_MODE:
		sr_dbg("SR_CONF_PATTERN_MODE");
		*data = g_variant_new_strv(ARRAY_AND_SIZE(patterns));
		break;
	case SR_CONF_LIMIT_SAMPLES:
		sr_dbg("SR_CONF_LIMIT_SAMPLES");
		if (!sdi)
			sr_dbg("Not sdi");
			return SR_ERR_ARG;
		devc = sdi->priv;
		if (devc->flag_reg & FLAG_RLE)
			sr_dbg("devc->flag_reg & FLAG_RLE");
			return SR_ERR_NA;
		if (devc->max_samples == 0)
			/* Device didn't specify sample memory size in metadata. */
			sr_dbg("devc->max_samples == 0");
			return SR_ERR_NA;
		/*
		 * Channel groups are turned off if no channels in that group are
		 * enabled, making more room for samples for the enabled group.
		*/
		sr_dbg("Attempting channel mask");
		acsp_channel_mask(sdi);
		num_acsp_changrp = 0;
		for (i = 0; i < 4; i++) {
			if (devc->channel_mask & (0xff << (i * 8)))
				sr_dbg("devc->channel_mask & (0xff << (i * 8))");
				num_acsp_changrp++;
		}

		*data = std_gvar_tuple_u64(MIN_NUM_SAMPLES,
			(num_acsp_changrp) ? devc->max_samples / num_acsp_changrp : MIN_NUM_SAMPLES);
		break;
	default:
		return SR_ERR_NA;
	}
	sr_dbg("Exiting config list gracefully");
	return SR_OK;
}


/**** This whole function was not given, it is O L S only*******/
static int set_trigger(const struct sr_dev_inst *sdi, int stage)
{
	sr_dbg("Entering set trigger");
	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;
	uint8_t cmd, arg[4];

	devc = sdi->priv;
	serial = sdi->conn;

	cmd = CMD_SET_TRIGGER_VALUE;
	arg[0] = 0x00;
	arg[1] = 0x00;
	arg[2] = devc->trigger_falling[0];
	arg[3] = devc->trigger_rising[0];
	if (acsp_send_longcommand(serial, cmd, arg) != SR_OK)
		return SR_ERR;


	sr_dbg("Exiting set trigger gracefully");
	return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	sr_dbg("Entering acquisition start");
	// /* TODO: configure hardware, reset acquisition state, set up
	//  * callbacks and send header packet. */

	// (void)sdi;

	// return SR_OK;

	/*----------above is the given code, below is from O L S----------*/

	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;
	uint16_t samplecount, readcount, delaycount;
	uint8_t acsp_changrp_mask, arg[4];
	int num_acsp_changrp;
	int ret, i;

	devc = sdi->priv;
	serial = sdi->conn;

	acsp_channel_mask(sdi);

	num_acsp_changrp = 0;
	acsp_changrp_mask = 0;
	for (i = 0; i < 4; i++) {
		if (devc->channel_mask & (0xff << (i * 8))) {
			acsp_changrp_mask |= (1 << i);
			num_acsp_changrp++;
		}
	}

	/*
	 * Limit readcount to prevent reading past the end of the hardware
	 * buffer.
	 */
	samplecount = MIN(devc->max_samples / num_acsp_changrp, devc->limit_samples);
	readcount = samplecount;

	/* Rather read too many samples than too few. */
	if (samplecount % 4 != 0)
		readcount++;

	/* Basic triggers. */
	if (acsp_convert_trigger(sdi) != SR_OK) {
		sr_err("Failed to configure channels.");
		return SR_ERR;
	}
	if (devc->num_stages > 0) {
		/*
		 * According to http://mygizmos.org/acsp/Logic-Sniffer-FPGA-Spec.pdf
		 * reset command must be send prior each arm command
		 */
		sr_dbg("Send reset command before trigger configure");
		if (acsp_send_reset(serial) != SR_OK)
			return SR_ERR;

		delaycount = readcount * (1 - devc->capture_ratio / 100.0);
		devc->trigger_at = (readcount - delaycount) * 4 - devc->num_stages;
		for (i = 0; i <= devc->num_stages; i++) {
			sr_dbg("Setting acsp stage %d trigger.", i);
			if ((ret = set_trigger(sdi, i)) != SR_OK)
				return ret;
		}
	} else {
		/* No triggers configured, force trigger on first stage. */
		sr_dbg("Forcing trigger at stage 0.");
		if ((ret = set_trigger(sdi, 0)) != SR_OK)
			return ret;
		delaycount = readcount;
	}

	/* Samplerate. */
	sr_dbg("Setting samplerate to %" PRIu64 "Hz (divider %u)",
			devc->cur_samplerate, devc->cur_samplerate_divider);
	arg[3] = devc->cur_samplerate_divider & 0xff;
	arg[2] = (devc->cur_samplerate_divider & 0xff00) >> 8;
	arg[1] = (devc->cur_samplerate_divider & 0xff0000) >> 16;
	arg[0] = 0x00;
	if (acsp_send_longcommand(serial, CMD_SET_DIVIDER, arg) != SR_OK)
		return SR_ERR;

	/* Send sample limit and pre/post-trigger capture ratio. */
	sr_dbg("Setting sample limit %d, trigger point at %d",
			(readcount - 1), (delaycount - 1));
	arg[3] = ((readcount - 1) & 0xff);
	arg[2] = ((readcount - 1) & 0xff00) >> 8;
	arg[1] = ((delaycount - 1) & 0xff);
	arg[0] = ((delaycount - 1) & 0xff00) >> 8;
	if (acsp_send_longcommand(serial, CMD_CAPTURE_SIZE, arg) != SR_OK)
		return SR_ERR;

	/* Flag register. */
	sr_dbg("Setting intpat %s, extpat %s, RLE %s, noise_filter %s, demux %s",
			devc->flag_reg & FLAG_INTERNAL_TEST_MODE ? "on": "off",
			devc->flag_reg & FLAG_EXTERNAL_TEST_MODE ? "on": "off",
			devc->flag_reg & FLAG_RLE ? "on" : "off",
			devc->flag_reg & FLAG_FILTER ? "on": "off",
			devc->flag_reg & FLAG_DEMUX ? "on" : "off");
	/*
	 * Enable/disable acsp channel groups in the flag register according
	 * to the channel mask. 1 means "disable channel".
	 *///TODO: flags need to be adjusted to ACSP
	devc->flag_reg |= ~(acsp_changrp_mask << 2) & 0x3c;
	arg[0] = devc->flag_reg & 0xff;
	arg[1] = devc->flag_reg >> 8;
	arg[2] = arg[3] = 0x00;
	if (acsp_send_longcommand(serial, CMD_SET_FLAGS, arg) != SR_OK)
		return SR_ERR;

	/* Start acquisition on the device. */
	if (acsp_send_shortcommand(serial, CMD_RUN) != SR_OK)
		return SR_ERR;

	/* Reset all operational states. */
	devc->rle_count = devc->num_transfers = 0;
	devc->num_samples = devc->num_bytes = 0;
	devc->cnt_bytes = devc->cnt_samples = devc->cnt_samples_rle = 0;
	memset(devc->sample, 0, 4);

	std_session_send_df_header(sdi);

	/* If the device stops sending for longer than it takes to send a byte,
	 * that means it's finished. But wait at least 100 ms to be safe.
	 */
	serial_source_add(sdi->session, serial, G_IO_IN, 100,
			acsp_receive_data, (struct sr_dev_inst *)sdi);
	sr_dbg("Exiting dev acquisition gracefully");
	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	sr_dbg("Entering dev acquisition stop");
	acsp_abort_acquisition(sdi);
	sr_dbg("Exiting dev acquisition stop gracefully");
	return SR_OK;
}

SR_PRIV struct sr_dev_driver acsp_driver_info = {
	.name = "ACSP",
	.longname = "Actually Challenging Senior Project",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = std_serial_dev_open,
	.dev_close = std_serial_dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.context = NULL,
};

SR_REGISTER_DEV_DRIVER(acsp_driver_info);

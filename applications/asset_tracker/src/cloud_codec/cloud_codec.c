/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <net/cloud.h>

#include "cJSON.h"
#include "cJSON_os.h"
#include "cloud_codec.h"
#include "env_sensor_api.h"

static const char * const sensor_type_str[] = {
	[CLOUD_SENSOR_GPS] = "GPS",
	[CLOUD_SENSOR_FLIP] = "FLIP",
	[CLOUD_SENSOR_BUTTON] = "BUTTON",
	[CLOUD_SENSOR_TEMP] = "TEMP",
	[CLOUD_SENSOR_HUMID] = "HUMID",
	[CLOUD_SENSOR_AIR_PRESS] = "AIR_PRESS",
	[CLOUD_SENSOR_AIR_QUAL] = "AIR_QUAL",
	[CLOUD_LTE_LINK_RSRP] = "RSRP",
	[CLOUD_DEVICE_INFO] = "DEVICE",
};

/* --- A few wrappers for cJSON APIs --- */

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_num(cJSON *parent, const char *str, double num)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(num);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_num);
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

static int json_add_null(cJSON *parent, const char *str)
{
	cJSON *json_null;

	json_null = cJSON_CreateNull();
	if (json_null == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_null);
}

static cJSON *json_object_decode(cJSON *obj, const char *str)
{
	return obj ? cJSON_GetObjectItem(obj, str) : NULL;
}

int cloud_encode_sensor_data(const struct cloud_sensor_data *sensor,
				 struct cloud_msg *output)
{
	int ret;

	__ASSERT_NO_MSG(sensor != NULL);
	__ASSERT_NO_MSG(sensor->data.buf != NULL);
	__ASSERT_NO_MSG(sensor->data.len != 0);
	__ASSERT_NO_MSG(output != NULL);

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		return -ENOMEM;
	}

	ret = json_add_str(root_obj, "appId", sensor_type_str[sensor->type]);
	ret += json_add_str(root_obj, "data", sensor->data.buf);
	ret += json_add_str(root_obj, "messageType", "DATA");

	if (ret != 0) {
		cJSON_Delete(root_obj);
		return -ENOMEM;
	}

	char *buffer;

	buffer = cJSON_PrintUnformatted(root_obj);
	cJSON_Delete(root_obj);
	output->payload = buffer;
	output->len = strlen(buffer);

	return 0;
}

int cloud_encode_env_sensors_data(const env_sensor_data_t *sensor_data,
				 struct cloud_msg *output)
{
	__ASSERT_NO_MSG(sensor_data != NULL);
	__ASSERT_NO_MSG(output != NULL);

	char buf[6];
	u8_t len;
	struct cloud_sensor_data cloud_sensor;

	switch(sensor_data->type) {
		case ENV_SENSOR_TEMPERATURE:
			cloud_sensor.type = CLOUD_SENSOR_TEMP;
			break;
		
		case ENV_SENSOR_HUMIDITY:
			cloud_sensor.type = CLOUD_SENSOR_HUMID;
			break;

		case ENV_SENSOR_AIR_PRESSURE:
			cloud_sensor.type = CLOUD_SENSOR_AIR_PRESS;
			break;
		
		default:
			return -1;
	}

	len = snprintf(buf, sizeof(buf), "%.1f",
		sensor_data->value);
	cloud_sensor.data.buf = buf;
	cloud_sensor.data.len = len;

	return cloud_encode_sensor_data(&cloud_sensor, output);
}
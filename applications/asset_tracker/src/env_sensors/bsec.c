/*bsec.c*/
 
#include "bsec_integration.h"
#include <i2c.h>
#include <string.h>
#include <zephyr.h>
#include <atomic.h>
#include <spinlock.h>
#include <misc/byteorder.h>
#include "env_sensor_api.h"

struct device *i2c_master;

int64_t s_timestamp;
float s_iaq; 
uint8_t s_iaq_accuracy; 
float s_temperature; 
float s_humidity;
float s_pressure; 

struct env_sensor {
	env_sensor_data_t sensor;
	struct k_spinlock lock;
};

static struct env_sensor temp_sensor = {
	.sensor =  {
		.id = 1,
		.type = ENV_SENSOR_TEMPERATURE,
		.value = 0,
		.timestamp = 0
	},
};

static struct env_sensor humid_sensor = {
	.sensor =  {
		.id = 2,
		.type = ENV_SENSOR_HUMIDITY,
		.value = 0,
		.timestamp = 0
	},
};

static struct env_sensor pressure_sensor = {
	.sensor =  {
		.id = 3,
		.type = ENV_SENSOR_AIR_PRESSURE,
		.value = 0,
		.timestamp = 0
	},
};

static struct env_sensor air_quality_sensor = {
	.sensor =  {
		.id = 4,
		.type = ENV_SENSOR_AIR_QUALITY,
		.value = 0,
		.timestamp = 0
	},
};


/* size of stack area used by bsec thread */
#define STACKSIZE 2048

/* scheduling priority used by bsec thread */
#define PRIORITY CONFIG_SYSTEM_WORKQUEUE_PRIORITY

static K_THREAD_STACK_DEFINE(thread_stack, STACKSIZE);
static struct k_thread thread;

static int8_t bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
	uint8_t buf[data_len+1];
	buf[0]=reg_addr;
	memcpy(&buf[1],reg_data_ptr,data_len);				
	return i2c_write(i2c_master, buf, data_len+1, dev_addr);
}

static int8_t bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
	return i2c_write_read(i2c_master,dev_addr, &reg_addr ,1, reg_data_ptr, data_len);
}

static void sleep(uint32_t t_ms)
{
	k_sleep(t_ms);
}

static int64_t get_timestamp_us()
{
	return k_uptime_get()*1000;
}

static void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
	float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status,
	float static_iaq, float co2_equivalent, float breath_voc_equivalent)
{
	k_spinlock_key_t key = k_spin_lock(&(temp_sensor.lock));
	temp_sensor.sensor.value=temperature;
	k_spin_unlock(&(temp_sensor.lock), key);
	key = k_spin_lock(&(humid_sensor.lock));
	humid_sensor.sensor.value=humidity;
	k_spin_unlock(&(humid_sensor.lock), key);
	key = k_spin_lock(&(pressure_sensor.lock));
	pressure_sensor.sensor.value=pressure/1000;
	k_spin_unlock(&(pressure_sensor.lock), key);
	key = k_spin_lock(&(air_quality_sensor.lock));
	air_quality_sensor.sensor.value=iaq;
	k_spin_unlock(&(air_quality_sensor.lock), key);
}

static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
	// ...
	// Load a previous library state from non-volatile memory, if available.
	//
	// Return zero if loading was unsuccessful or no state was available, 
	// otherwise return length of loaded state string.
	// ...
	return 0;
}

static void state_save(const uint8_t *state_buffer, uint32_t length)
{
	// ...
	// Save the string to non-volatile memory
	// ...
}
 
static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
	// ...
	// Load a library config from non-volatile memory, if available.
	//
	// Return zero if loading was unsuccessful or no config was available, 
	// otherwise return length of loaded config string.
	// ...
	return 0;
}

static void bsec_thread(void)
{
	bsec_iot_loop(sleep, get_timestamp_us, output_ready, state_save, 10000);
}



int env_sensors_get_temperature(env_sensor_data_t *sensor_data, u64_t *sensor_id)
{
	if(sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&temp_sensor.lock);
	memcpy(sensor_data, &(temp_sensor.sensor),sizeof(temp_sensor.sensor));
	k_spin_unlock(&temp_sensor.lock, key);
	return 0;
}

int env_sensors_get_humidity(env_sensor_data_t *sensor_data, u64_t *sensor_id)
{
	if(sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&humid_sensor.lock);
	memcpy(sensor_data, &(humid_sensor.sensor),sizeof(humid_sensor.sensor));
	k_spin_unlock(&humid_sensor.lock, key);
	return 0;
}

int env_sensors_get_pressure(env_sensor_data_t *sensor_data, u64_t *sensor_id)
{	
	if(sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&pressure_sensor.lock);
	memcpy(sensor_data, &(pressure_sensor.sensor),sizeof(pressure_sensor.sensor));
	k_spin_unlock(&pressure_sensor.lock, key);
	return 0;	
}

int env_sensors_get_air_quality(env_sensor_data_t *sensor_data, u64_t *sensor_id)
{
	if(sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&air_quality_sensor.lock);
	memcpy(sensor_data, &(air_quality_sensor.sensor),sizeof(air_quality_sensor.sensor));
	k_spin_unlock(&air_quality_sensor.lock, key);
	return 0;	
}

int env_sensors_init(void)
{
	i2c_master = device_get_binding("I2C_2");
	if (!i2c_master) {
		printk("cannot bind to BME680\n");
		return -EINVAL;
	}
	return 0;
}

int env_sensors_start_polling(void)
{
	return_values_init ret;

	ret = bsec_iot_init(BSEC_SAMPLE_RATE_ULP, 3.0f, bus_write, bus_read, sleep, state_load, config_load);
	if (ret.bme680_status)
	{
		/* Could not intialize BME680 */
		return (int)ret.bme680_status;
	}
	else if (ret.bsec_status)
	{
		/* Could not intialize BSEC library */
		return (int)ret.bsec_status;
	}

	k_thread_create(&thread, thread_stack,
					STACKSIZE,
					(k_thread_entry_t)bsec_thread,
					NULL, NULL, NULL,
					PRIORITY, 0, K_NO_WAIT);

	return 0;
}

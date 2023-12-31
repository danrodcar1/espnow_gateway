#include "AUTOpairing.h"
#include "ADConeshot.h"
#include "cJSON.h"

extern "C"
{
	void app_main(void);
}

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE

#define LOG_TAG "main"

static AUTOpairing_t clienteAP;
static ADConeshot_t ADConeshot;

const char *ESP_WIFI_SSID = "HortSost";
const char *ESP_WIFI_PASS = "9b11c2671e5b";
const char *FIRMWARE_UPGRADE_URL = "https://huertociencias.uma.es/ESP32OTA/espnow_project.bin";

UpdateStatus updateStatus = NO_UPDATE_FOUND;

// int adc_channel[1] = {ADC_CHANNEL_0};
// static adc_channel_t adc_channel[4] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_4};
static adc_channel_t adc_channel[1] = {ADC_CHANNEL_0};
uint8_t lengthADC1_CHAN = sizeof(adc_channel) / sizeof(adc_channel_t);

typedef struct
{
	uint8_t tsleep;
	uint8_t pan;
	uint16_t timeout;
	uint16_t time;
} struct_config;
struct_config strConfig;

struct struct_data
{
	uint32_t adc_filtered[ESP_NOW_MAX_MSR]; // Establecer un número máximo de lecturas para almacenar
	uint32_t adc_voltage[ESP_NOW_MAX_MSR];
	uint8_t macAddr[6];
	unsigned long ms_old[ESP_NOW_MAX_MSR];
};

RTC_DATA_ATTR struct_data *strData;

void pan_process_msg(struct_espnow_rcv_msg *my_msg)
{
	ESP_LOGI("* PAN process", "Mensaje PAN recibido");
	ESP_LOGI("* PAN process", "Antiguedad mensaje (ms): %lu", my_msg->ms_old);
	ESP_LOGI("* PAN process", "MAC: %02X:%02X:%02X:%02X:%02X:%02X", my_msg->macAddr[0], my_msg->macAddr[1], my_msg->macAddr[2], my_msg->macAddr[3], my_msg->macAddr[4], my_msg->macAddr[5]);
	ESP_LOGI("* PAN process", "Payload: %s", my_msg->payload);
	ESP_LOGI("* PAN process", "Mi PAN: %d", clienteAP.get_pan());
	for (int i = 0; i < ESP_NOW_MAX_TOTAL_PEER_NUM; i++)
	{
		if (memcmp(my_msg->macAddr, strData[i].macAddr, ESP_NOW_ETH_ALEN) != 0)
		{
			memcpy(strData[i].macAddr, my_msg->macAddr, ESP_NOW_ETH_ALEN);
		}
	}

	cJSON *root = cJSON_Parse(my_msg->payload);
	char tag[25];
	int adc_filtered[lengthADC1_CHAN];
	int adc_voltage[lengthADC1_CHAN];
	for (int i = 0; i < lengthADC1_CHAN; i++)
	{
		ESP_LOGI(TAG, "Deserialize readings of channel %d", i);
		sprintf(tag, "Sensor%d", i);
		cJSON *fmt = cJSON_GetObjectItem(root, tag);
		/* En principio no sé cuantos sensores traerá la lectura. Si están en el mensaje, lo guardo. Si no, termino de deserializar.*/
		if (fmt)
		{
			adc_filtered[i] = cJSON_GetObjectItem(fmt, "adc_filtered")->valueint;
			adc_voltage[i] = cJSON_GetObjectItem(fmt, "adc_voltage")->valueint;
		}
		else
			break;
	}
	cJSON_Delete(root);
	free(my_msg->payload);
}

void mqtt_process_msg(struct_espnow_rcv_msg *my_msg)
{
	ESP_LOGI("* mqtt process", "topic: %s", my_msg->topic);
	cJSON *root = cJSON_Parse(my_msg->payload);
	if (strcmp(my_msg->topic, "config") == 0)
	{
		ESP_LOGI("* mqtt process", "payload: %s", my_msg->payload);
		ESP_LOGI("* mqtt process", "Deserialize payload.....");
		cJSON *sleep = cJSON_GetObjectItem(root, "sleep");
		cJSON *timeout = cJSON_GetObjectItem(root, "timeout");
		cJSON *pan = cJSON_GetObjectItem(root, "pan");
		uint16_t config[MAX_CONFIG_SIZE]; // max config size
		if (timeout)
			config[1] = timeout->valueint;
		if (sleep)
			config[2] = sleep->valueint;
		if (pan)
			config[3] = pan->valueint;
		// ESP_LOGI("* mqtt process", "tsleep = %d", config[1]);
		// ESP_LOGI("* mqtt process", "timeout = %d", config[2]);
		// ESP_LOGI("* mqtt process", "PAN_ID = %d", config[3]);
		clienteAP.set_config(config);
	}
	if (strcmp(my_msg->topic, "update") == 0)
	{
		updateStatus = THERE_IS_AN_UPDATE_AVAILABLE;
		// clienteAP.init_update(); ¿Porque funciona en app_main y aqui no? :(
	}
	free(my_msg->topic);
	free(my_msg->payload);
	cJSON_Delete(root);
}



void app_main(void)
{
	unsigned long start_time = esp_timer_get_time();
	strData = (struct_data *)malloc(ESP_NOW_MAX_TOTAL_PEER_NUM * sizeof(struct_data));
	clienteAP.init_config_size(sizeof(strConfig));
	if (!clienteAP.get_config((uint16_t *)&strConfig))
	{
		strConfig.timeout = 3000;
		strConfig.tsleep = 10;
	}

	struct_adclist *my_reads = ADConeshot.set_adc_channel(adc_channel, lengthADC1_CHAN);
	
	clienteAP.esp_set_https_update(FIRMWARE_UPGRADE_URL, ESP_WIFI_SSID, ESP_WIFI_PASS);
	clienteAP.set_timeOut(strConfig.timeout, true); // tiempo máximo
	clienteAP.set_deepSleep(strConfig.tsleep);		// tiempo dormido en segundos
	ESP_LOGI("Config debug", "Timeout = %d", strConfig.timeout);
	ESP_LOGI("Config debug", "Timeout = %d", strConfig.tsleep);
	clienteAP.set_channel(1); // canal donde empieza el scaneo
	clienteAP.set_pan(2);
	clienteAP.set_mqtt_msg_callback(mqtt_process_msg); // por defecto a NULL -> no se llama a ninguna función
	clienteAP.set_pan_msg_callback(pan_process_msg);
	clienteAP.begin();
	if (clienteAP.envio_disponible() == true)
	{
		cJSON *root, *fmt;
		const esp_partition_t *running = esp_ota_get_running_partition();
		esp_app_desc_t running_app_info;
		char tag[25];
		ADConeshot.adc_init(my_reads);
		root = cJSON_CreateObject();
		if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
		{
			cJSON_AddStringToObject(root, "fw_version", running_app_info.version);
		}

		for (int i = 0; i < my_reads->length; i++)
		{
			ESP_LOGI(TAG, "Serialize readings of channel %d", i);
			sprintf(tag, "Sensor%d", i);
			cJSON_AddItemToObject(root, tag, fmt = cJSON_CreateObject());
			cJSON_AddNumberToObject(fmt, "adc_filtered", ADConeshot.get_adc_filtered_read(my_reads, i));
			cJSON_AddNumberToObject(fmt, "adc_voltage", ADConeshot.get_adc_voltage_read(my_reads, i));
		}

		char *my_json_string = cJSON_Print(root);
		size_t msg_size = strlen(my_json_string);
		ESP_LOGI(TAG, "my_json_string\n%s", my_json_string);
		ESP_LOGI("* Tamaño paquete", "El mensaje ocupa %i bytes", msg_size);
		if (msg_size > 249)
		{
			// Empieza el proceso de envío. Desactivo el deep sleep
			int n = 1;
			for (size_t i = 1; i < msg_size; i++)
			{
				if (msg_size % i == 0)
				{
					ESP_LOGI("* Divisores paquete", "El mensaje se puede dividir en %i paquetes de %i bytes", n, msg_size / n);
					n++;
				}
				if (n * i < 249)
					break;
			}
			ESP_LOGI("* Divisores paquete", "El mensaje se dividirá en %i paquetes de %i bytes", n, msg_size / n);
			int parts = msg_size / n;
			int start = 0;
			int index_part = n;

			// MIRAR ESTA WEB: https://www.appsloveworld.com/cplus/100/584/how-can-i-split-a-string-into-chunks-of-1024-with-iterator-and-string-view
			while (start < msg_size)
			{
				char filename[249];
				char substr[parts + 1]; // Cadena de datos de longitud parts + 1 index_part + 1 caracter fin cadena
				strncpy(substr, my_json_string + start, parts);
				sprintf(filename, "%d%s", index_part, substr);
				printf("Filename is: %s\n", filename);
				start += parts;
				index_part--;
				clienteAP.espnow_send_check(filename, false, DATA); // no hará deepsleep despues del envio
			}
			clienteAP.gotoSleep();
		}
		else
		{
			clienteAP.espnow_send_check(my_json_string); // hará deepsleep por defecto
		}

		free(my_reads);
		cJSON_Delete(root);
		cJSON_free(my_json_string);

		switch (updateStatus)
		{
		case THERE_IS_AN_UPDATE_AVAILABLE:
			clienteAP.init_update();
			break;
		case NO_UPDATE_FOUND:
			// get deep sleep enter time
			//						gotoSleep();
			break;
		}
	}
}

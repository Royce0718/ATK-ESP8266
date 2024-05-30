#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx_hal.h"

typedef struct // 结构体。
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} CalendarInfo;

typedef struct
{
    uint8_t text[23];
    int8_t temperature;
} WeatherInfo;

typedef enum
{
    AP_Connected = 2,     // 已连接 AP，获得 IP 地址
    TCP_Connected = 3,    // 已建⽴立 TCP 或 UDP 传输
    Net_Disconnected = 4, // 断开⽹络连接
    AP_Disconnected = 5   // 未连接 AP
} WiFi_Status;

typedef enum ESPMode
{
    Esp_Server = 0,
    Esp_Client = 1
} ESP_Mode;

// Basic AT command functions
uint8_t *Esp8266_CheckACK(uint8_t *ACK);
uint8_t Esp8266_SendAT(uint8_t *command, uint8_t *ACK, uint16_t waittime);

// WiFi control functions
uint8_t WiFi_Connect_CUR(char *ssid, char *password);
uint8_t WiFi_Connect_DEF(char *ssid, char *password);
void WiFi_Disconnect(void);
void WiFi_Reconnect(uint16_t interval_second, uint16_t repeat_count);
void WiFi_SetAT_CWLAP(uint16_t print_mask, int8_t rssi_filter, uint16_t authmode_mask);
void WiFi_GetIP(uint8_t *ipbuf);
WiFi_Status WiFi_GetStatus(void);

// ESP8266 server/client functions
void Esp_ServerSend(uint8_t ID, uint8_t length, uint8_t *data);
void Esp_ClientSend(uint8_t *data);
void Esp_Reset(void);
void Esp_Uart_Config(uint8_t mode, uint32_t baudrate, uint8_t databits, uint8_t stopbits, uint8_t parity, uint8_t flow_control);
void Esp_Config(void);
void ESP_Init(void);
void Esp_ClientToServer(void);
void Esp_ServerToClient(void);

// API specific functions
void TimeProcess(CalendarInfo *t);
void WeatherProcess(WeatherInfo *w);
void Esp_GetTime(void);
void Esp_GetWeather(void);

#endif

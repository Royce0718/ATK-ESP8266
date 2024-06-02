#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx_hal.h"
#include <stdio.h>

// WiFi强度补偿后的最大值 140
#define WiFi_Rssi_Max 140

// 时间结构体
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} CalendarInfo;

// 成都常见天气代码
typedef enum
{
    SUNNY_DAY = 0,      // 晴（白天）
    CLEAR_NIGHT = 1,    // 晴（夜晚）
    CLOUDY = 4,         // 多云
    OVERCAST = 9,       // 阴
    SHOWER = 10,        // 阵雨
    THUNDERSHOWER = 11, // 雷阵雨
    LIGHT_RAIN = 13,    // 小雨
    MODERATE_RAIN = 14, // 中雨
    HEAVY_RAIN = 15,    // 大雨
    STORM = 16,         // 暴雨
    HEAVY_STORM = 17,   // 大暴雨
    FOGGY = 30,         // 雾
    HAZE = 31           // 霾
} WeatherCode;

// 天气信息结构体
typedef struct
{
    WeatherCode code;
    uint8_t text[23];
    int8_t temperature;
} WeatherInfo;

typedef enum
{
    AP_Connected = 2,     // 已连接 AP，获得 IP 地址
    TCP_Connected = 3,    // 已建⽴立 TCP 或 UDP 传输
    Net_Disconnected = 4, // 断开⽹络连接
    AP_Disconnected = 5,  // 未连接 AP
} WiFi_Status;

typedef enum
{
    ESP_Error = 0,
    ESP_OK = 1,
    ESP_TimeOut = 2,
    ESP_Reserved = 3 // 基于代码完整性设置‘保留项’
} ESP_State;

typedef enum ESPMode
{
    Esp_Server = 0,   // TCP Server
    Esp_Client = 1,   // TCP Client
    Esp_YuanZiYun = 3 // 连接原子云服务器
} ESP_Mode;

// WiFi 存储结构体，用于实现选择信号最强的已保存WiFi，并连接
// WiFi名称最大32byte

typedef struct WiFiInfo
{
    const char *ssid;
    const char *password;
    uint8_t rssi; // 信号强度
} WiFi_Info;

typedef struct WiFiInfoNode
{
    WiFi_Info WiFi_Item;
    struct WiFiInfoNode *next;
    struct WiFiInfoNode *before;
} WiFi_Info_Node;

typedef struct WiFiInfoList
{
    uint8_t WiFi_NumCNT;        // WiFi数量
    WiFi_Info_Node *WiFi_Index; // 节点索引指针,指向WiFiListEnd
    WiFi_Info_Node WiFiListEnd; // 链表最后一个节点
} WiFi_List;

// User 总初始化
void WiFi_Init(void);

// 链表相关
void WiFiInfo_Config(WiFi_List *const pList);
void WiFi_GetRssi_of(WiFi_Info_Node *const Node);
void WiFiList_Init(WiFi_List *const pList);
void WiFiList_InsertEnd(WiFi_List *const pList, WiFi_Info_Node *const NewNode);
void WiFiList_Insert(WiFi_List *const pList, WiFi_Info_Node *const NewNode);
void WiFiListRemove(WiFi_List *const pList, WiFi_Info_Node *const RemoveNode);

// Basic AT command functions
uint8_t *Esp8266_CheckACK(uint8_t *ACK);
ESP_State Esp8266_SendAT(char *command, uint8_t *ACK, uint16_t waittime);

// WiFi control functions
uint8_t WiFi_Connect_CUR(const char *ssid, const char *password);
uint8_t WiFi_Connect_DEF(const char *ssid, const char *password);
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
ESP_State Esp_Disconnect_YuanZiYun(void);
ESP_State Esp_Connect_YuanZiYun(const char *id, const char *pwd);

// API specific functions
void TimeProcess(CalendarInfo *t);
void WeatherProcess(WeatherInfo *w);
void Esp_GetTime(void);
void Esp_GetWeather(void);

#endif

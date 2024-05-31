/***********************************************************************
 * @author 温一壶月光如酒(Github Royce0718)
 * @brief realize the basic function of ATK Esp8266
 *        1. get RealTimeInfo from API specific
 *        2. get RealWeather local from API specific
 *        3. you can Use Fx below to design your own function
 *
 * @attention
 *       - support TCP Server and TCP Client,but not UPD
 *       - usually work as Server for dealing with Requst from Client
 *       but you can change whatever you want
 *       - STM32 Uart1 works in UART IDLE(Interrupt) + DMA_Rx
 *       details in uasrt.c and  stm32f1xx_it.c
 ************************************************************************/

#include "stm32f4xx_hal.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>

#define RXBUFFERSIZE 512

extern UART_HandleTypeDef huart1;
extern uint8_t Uart1_RxBuffer[RXBUFFERSIZE];
extern uint8_t Uart1_RXFlag;
extern uint8_t Uart1_RXLen;

CalendarInfo RealTime = {0, 0, 0, 0, 0, 0};

WeatherInfo RealWeather = {0, 0};

ESP_Mode Esp_Mode = Esp_Server;

uint8_t IP[15];

// WiFi链表
WiFi_List WiFiList;

// 上电是否自动连接 AP,1为上电自动连接，0则否
// 如果你调用了AT_CWAUTOCONN_1或AT_CWAUTOCONN_0，必须根据修改对应数值
#define WiFi_Connect_auto 1

// 如果需要要在keil5使用中文在 C/C++ Misc controls 中添加“--no-multibyte-chars”

// 获取WiFi强度
void WiFi_GetRssi_of(WiFi_Info_Node *const Node)
{
    char p[80];
    sprintf(p, "AT+CWLAP=\"%s\"\r\n", Node->WiFi_Item.ssid);
    if (Esp8266_SendAT(p, "+CWLAP:", 300) != ESP_OK)
    {
        Node->WiFi_Item.rssi = 0;
        printf("WiFi:\"%s\"不在区域内\r\n", Node->WiFi_Item.ssid);
    }
    else
    {
        char *p;
        char *p_end;
        uint8_t width = 0;
        p = strstr((const char *)Uart1_RxBuffer, ",");
        p = p + 1;
        p = strstr((const char *)p, ",");
        p = p + 1;
        p_end = strstr((const char *)p, ",");
        width = p_end - p;
        if ((*p) == '-')
        {
            if (width == 3)
            {
                Node->WiFi_Item.rssi = 100 - ((*(p + 1) - '0') * 10 + (*(p + 2) - '0'));
            }
            else if (width == 2)
            {
                Node->WiFi_Item.rssi = 100 - (*(p + 1) - '0');
            }
        }
        else
        {
            if (width == 2)
            {
                Node->WiFi_Item.rssi = ((*p - '0') * 10) + (*(p + 1) - '0');
            }
            else if (width == 1)
            {
                Node->WiFi_Item.rssi = *p - '0';
            }
        }
        printf("WiFi:\"%s\"强度为:%d\r\n", Node->WiFi_Item.ssid, Node->WiFi_Item.rssi);
    }
}

// 自动选择WiFi强度更高的WiFi进行连接
void WiFi_AutoConnect(WiFi_List *const pList)
{
    for (WiFi_Info_Node *pIterator = pList->WiFi_Index->before; pIterator != pList->WiFi_Index; pIterator = pIterator->before)
    {
        if (WiFi_Connect_CUR(pIterator->WiFi_Item.ssid, pIterator->WiFi_Item.password))
        {
            printf("WiFi:\"%s\"连接成功，强度为:%d\r\n", pIterator->WiFi_Item.ssid, pIterator->WiFi_Item.rssi);
            break;
        }
        else
        {
            printf("WiFi:\"%s\"连接失败\r\n", pIterator->WiFi_Item.ssid);
        }
    }
}

// 初始化链表
void WiFiList_Init(WiFi_List *const pList)
{
    pList->WiFi_Index = &(pList->WiFiListEnd);

    /* WiFi信号强度为[-100:40],得到rssi会加上补偿使其成为正数这里设为尾节点最强的信号 */
    pList->WiFiListEnd.WiFi_Item.rssi = WiFi_Rssi_Max;

    /* 将最后一个节点的pxNext和pxPrevious指针均指向节点自身，表示链表为空 */
    pList->WiFiListEnd.before = &(pList->WiFiListEnd);
    pList->WiFiListEnd.next = &(pList->WiFiListEnd);

    /* 初始化链表节点计数器的值为0，表示链表为空 */
    pList->WiFi_NumCNT = 0;
}

// 将新的节点插入链表的尾部
void WiFiList_InsertEnd(WiFi_List *const pList, WiFi_Info_Node *const NewNode)
{
    WiFi_Info_Node *const pIndex = pList->WiFi_Index;

    NewNode->next = pIndex;
    NewNode->before = pIndex->before;
    pIndex->before->next = NewNode;
    pIndex->before = NewNode;

    pList->WiFi_NumCNT++;
}

// 将新的节点按照WiFi强度升序插入
void WiFiList_Insert(WiFi_List *const pList, WiFi_Info_Node *const NewNode)
{
    WiFi_Info_Node *pIterator;

    /* 获取节点的排序辅助值 */
    const uint8_t ValueOfInsertion = NewNode->WiFi_Item.rssi;

    if (ValueOfInsertion == WiFi_Rssi_Max)
    {
        pIterator = pList->WiFiListEnd.before;
    }
    else
    {
        for (pIterator = &(pList->WiFiListEnd); pIterator->next->WiFi_Item.rssi <= ValueOfInsertion;
             pIterator = pIterator->next)
        {
            // 找到位置即可
        }
    }

    NewNode->next = pIterator->next;
    NewNode->before = pIterator;
    pIterator->next = NewNode;
    NewNode->next->before = NewNode;

    pList->WiFi_NumCNT++;
}

// 介于STM32内存分配不太会用，建议不用该删除函数，实际运用中也不怎么用得到
void WiFiListRemove(WiFi_List *const pList, WiFi_Info_Node *const RemoveNode)
{
    RemoveNode->next->before = RemoveNode->before;
    RemoveNode->before->next = RemoveNode->next;

    if (RemoveNode == pList->WiFi_Index)
    {
        pList->WiFi_Index = RemoveNode->before;
    }

    RemoveNode->before = NULL;
    RemoveNode->next = NULL;

    pList->WiFi_NumCNT--;
}

/*********************************************************
 * @brief Check ACK correct or not in received buffer
 * @param 'ACK'      : the response expected from atk_esp.
 * @retval 0: Check Failed , other: Checked
 */
uint8_t *Esp8266_CheckACK(uint8_t *ACK)
{
    char *str = strstr((const char *)Uart1_RxBuffer, (const char *)ACK);
    return (uint8_t *)str;
}

/************************************************************************************************
 * @brief send command to atk_esp
 * @param 'command'  : the command specific
 * @param 'ACK'      : the response expected from atk_esp.While ACK=NULL,ACK not expected
 * @param 'waittime' : the time( per 10ms) waitting for ACK.While waittime=0,ACK not expected
 *
 * @retval 0: send failed , 1: tha ACK expected received
 *
 * @attention : The Datas received is valid until the next Esp8266_SendAT()
 */
ESP_State Esp8266_SendAT(char *command, uint8_t *ACK, uint16_t waittime)
{
    Uart1_RXFlag = 0;
    memset(Uart1_RxBuffer, 0, Uart1_RXLen);
    if (HAL_UART_Transmit(&huart1, (uint8_t *)command, strlen((const char *)command), 0xFFFF) != HAL_OK)
    {
        return ESP_Error;
    }
    if (ACK && waittime)
    {
        while (--waittime)
        {
            HAL_Delay(10);
            if (Uart1_RXFlag)
            {
                // ESP的数据在一些情况下不是同时同批次的返回，这个时候我需要确定最后的回复，所以
                // 如果不是对应的ACK就清零Uart1_RXFlag，继续接收
                if (Esp8266_CheckACK(ACK))
                {
                    return ESP_OK;
                }
                else
                    Uart1_RXFlag = 0;
            }
        }
        if (waittime == 0)
        {
            return ESP_TimeOut;
        }
    }
    return ESP_Reserved;
}

#define AT Esp8266_SendAT("AT\r\n", "OK", 100)                            // 测试指令
#define AT_RST Esp8266_SendAT("AT+RST\r\n", "OK", 100)                    // 重启模块
#define AT_GMR Esp8266_SendAT("AT+GMR\r\n", "OK", 100)                    // 查看版本信息
#define AT_RESTORE Esp8266_SendAT("AT+RESTORE\r\n", "OK", 100)            // 恢复出厂设置
#define AT_CWLAP Esp8266_SendAT("AT+CWLAP\r\n", "+OK", 100)               // 列出当前可用AP(WiFi)
#define AT_CWQAP Esp8266_SendAT("AT+CWQAP\r\n", "OK", 100)                // 断开与当前AP连接
#define AT_CIFSR Esp8266_SendAT("AT+CIFSR\r\n", "OK", 100)                // 获取ESP的IP
#define AT_CIPCLOSE Esp8266_SendAT("AT+CIPCLOSE\r\n", "OK", 100)          // 断开单连接下的TCP连接
#define AT_CIPCLOSE_5 Esp8266_SendAT("AT+CIPCLOSE=5\r\n", "OK", 100)      // 断开多链接下的所有TCP连接
#define AT_CIPMODE_1 Esp8266_SendAT("AT+CIPMODE=1\r\n", "OK", 100)        // 透传模式
#define AT_CIPMODE_0 Esp8266_SendAT("AT+CIPMODE=0\r\n", "OK", 100)        // 普通传输模式
#define AT_CIPMUX_0 Esp8266_SendAT("AT+CIPMUX=0\r\n", "OK", 100)          // 单连接模式
#define AT_CIPMUX_1 Esp8266_SendAT("AT+CIPMUX=1\r\n", "OK", 100)          // 多连接模式
#define AT_CIPQUIT Esp8266_SendAT("+++", "", 0)                           // 退出透传
#define AT_CIPSTATUS Esp8266_SendAT("AT+CIPSTATUS\r\n", "OK", 100)        // 获得连接状态
#define ATE0 Esp8266_SendAT("ATE0\r\n", "OK", 100);                       // 关闭回显
#define ATE1 Esp8266_SendAT("ATE1\r\n", "OK", 100);                       // 开启回显
#define AT_CWAUTOCONN_1 Esp8266_SendAT("AT+CWAUTOCONN=1\r\n", "OK", 100); // 设置上电自动连接 AP
#define AT_CWAUTOCONN_0 Esp8266_SendAT("AT+CWAUTOCONN=0\r\n", "OK", 100); // 关闭上电自动连接 AP
#define AT_CIPSERVER_1 Esp8266_SendAT("AT+CIPSERVER=1\r\n", "OK", 100)    // 建立TCP服务器(333)
#define AT_CIPSERVER_0 Esp8266_SendAT("AT+CIPSERVER=0\r\n", "OK", 100)    // 关闭TCP服务器
#define ESP_STA Esp8266_SendAT("AT+CWMODE=1\r\n", "OK", 100)              // 设置ESP为STA模式
#define ESP_AP Esp8266_SendAT("AT+CWMODE=2\r\n", "OK", 100)               // 设置ESP为AP模式
#define ESP_STA_AP Esp8266_SendAT("AT+CWMODE=3\r\n", "OK", 100)           // 设置ESP为AP+STA模式

// link to WiFi temporarily
uint8_t WiFi_Connect_CUR(const char *ssid, const char *password)
{
    char p[80]; // 数组大于连WiFi的指令长度
    sprintf(p, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", ssid, password);
    // 连接WiFi时，等待的时间要久一点
    if (Esp8266_SendAT(p, (uint8_t *)"OK", 1000) == ESP_OK)
    {
        return 1; // 连接成功
    }
    else
        return 0; // 连接失败
}

// link to WiFi,while this WiFi is saved to Flash
uint8_t WiFi_Connect_DEF(const char *ssid, const char *password)
{
    char p[80]; // 数组大于连WiFi的指令长度
    sprintf(p, "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", ssid, password);
    // 连接WiFi时，等待的时间要久一点
    if (Esp8266_SendAT(p, (uint8_t *)"OK", 1000) == ESP_OK)
    {
        return 1; // 连接成功
    }
    else
        return 0; // 连接失败
}

// Break the WiFi
void WiFi_Disconnect(void)
{
    if (Esp_Mode == Esp_Server)
    {
        AT_CIPSERVER_0;
        AT_CWQAP;
    }
    else if (Esp_Mode == Esp_Client)
    {
        AT_CIPQUIT;
        AT_CWQAP;
    }
}

/********************************************************************************
 * @brief 设置 Wi-Fi 重连配置
 *
 * @param interval_second：Wi-Fi 重连间隔，单位：秒，默认值：0，最大值 7200
 *        0: 断开连接后，ESP32 station 不重连 AP
 *        [1,7200]: 断开连接后，ESP32 station 每隔指定的时间与 AP 重连
 *
 * @param repeat_count：ESP32 设备尝试重连 AP 的次数,默认值：0，最大值：1000
 *        0: ESP32 station 始终尝试连接 AP
 *        [1,1000]: ESP32 station 按照本参数指定的次数重连 AP
 **********************************************************************************/
void WiFi_Reconnect(uint16_t interval_second, uint16_t repeat_count)
{
    char p[80];
    sprintf(p, "AT+CWRECONNCFG=%d,%d", interval_second, repeat_count);
    Esp8266_SendAT(p, "OK", 100);
}

/***********************************************************
 * @brief  AT+CWLAP 命令扫描结果的属性,详情见ESP官网.
 *         这里给出最简单的用法，只显示可用WiFi名与其强度
 *
 * @param print_mask 建议值：6(只显示ssid 与 rssi)
 * @param rssi_filter
 *          是否过滤掉信号强度低于 rssi filter 参数值的 AP
 *          单位：dBm，默认值：–100，范围：[–100,40]
 * @param authmode_mask 建议值：0
 *
 * 建议 ： WiFi_SetAT_CWLAP(6,-60,0);
 */
void WiFi_SetAT_CWLAP(uint16_t print_mask, int8_t rssi_filter, uint16_t authmode_mask)
{
    char p[80];
    sprintf(p, "AT+CWLAPOPT=1,%d,%d,%d", print_mask, rssi_filter, authmode_mask);
    Esp8266_SendAT(p, "OK", 1000);
}

void WiFi_GetIP(uint8_t *ipbuf)
{
    uint8_t *p, *p1;
    if (Esp8266_SendAT("AT+CIFSR\r\n", "OK", 300) == ESP_Error)
    {
        ipbuf[0] = 0;
        return;
    }
    p = Esp8266_CheckACK("STAIP");
    p1 = (uint8_t *)strstr((const char *)(p + 7), "\"");
    *p1 = 0;
    sprintf((char *)ipbuf, "%s", (char *)(p + 7));
    printf("%s\r\n", (char *)IP);
}

WiFi_Status WiFi_GetStatus(void)
{
    uint8_t *p;
    AT_CIPSTATUS;
    p = Esp8266_CheckACK(":");
    return (WiFi_Status)(*(p + 1) - '0');
}

// TCP Server(Esp) Send Info to TCP Client
void Esp_ServerSend(uint8_t ID, uint8_t length, uint8_t *data)
{
    char p[80];
    sprintf(p, "AT+CIPSEND=%d,%d\r\n", ID, length);
    Esp8266_SendAT(p, "OK", 100);
    HAL_Delay(500);
    HAL_UART_Transmit(&huart1, data, (uint16_t)length, 0xFFFF);
}

// TCP Client(Esp) Send Info to TCP Server
void Esp_ClientSend(uint8_t *data)
{
    uint16_t length = strlen((const char *)data);
    Esp8266_SendAT("AT+CIPSEND\r\n", ">", 300);
    Uart1_RXFlag = 0;
    memset(Uart1_RxBuffer, 0, Uart1_RXLen);
    HAL_UART_Receive_DMA(&huart1, Uart1_RxBuffer, RXBUFFERSIZE);
    while (!Uart1_RXFlag)
    {
        HAL_UART_Transmit(&huart1, data, length, 0xFFFF);
        HAL_Delay(100);
    }
}

void Esp_Reset(void)
{
    // 复位后需要等待3秒
    AT_RST;
    HAL_Delay(1000);
    HAL_Delay(1000);
    HAL_Delay(1000);

    // 如果有WiFi保存到了Flash,并且配置了上电自动连接，则等待2秒自动连接
#if WiFi_Connect_auto

    HAL_Delay(1000);
    HAL_Delay(1000);

#endif
}

/*********************************************************************
 * @brief AT+UART_CUR/AT+UART_DEF 设置UART默认配置
 *        AT+UART_DEF 保存UART设置到Flash
 *        AT+UART_CUR 不保存UART设置到flash，为临时设置
 *
 * @param mode 1，为AT+UART_DEF; 0，为AT+UART_CUR
 * @param baudrate 波特率 ：[80 : 5000000]
 * @param databits 数据位宽 [5 : 8]
 * @param stopbits 停止位 1：1 bit; 2：1.5 bit; 3：2 bit
 * @param parity 校验位 0：None; 1：Odd; 2：Even
 * @param flow_control 流控
 *                     0：不使能流控  2：使能 CTS
 *                     1：使能 RTS   3：同时使能 RTS 和 CTS
 * @attention 默认:Esp_Uart_Config(1,115200,8,1,0,0);
 ************************************************************/
void Esp_Uart_Config(uint8_t mode, uint32_t baudrate, uint8_t databits, uint8_t stopbits, uint8_t parity, uint8_t flow_control)
{
    char p[80];
    if (mode == 1)
    {
        sprintf(p, "AT+UART_DEF=%d,%d,%d,%d,%d", baudrate, databits, stopbits, parity, flow_control);
        Esp8266_SendAT(p, "OK", 100);
    }
    else if (mode == 0)
    {
        sprintf(p, "AT+UART_CUR=%d,%d,%d,%d,%d", baudrate, databits, stopbits, parity, flow_control);
        Esp8266_SendAT(p, "OK", 100);
    }
}

// 该函数用于配置ATK-ESP8266的基础设置，调用一次即可，避免多次配置
// 建议使用串口助手进行配置
void Esp_Config(void)
{
    AT;
    // Esp_Uart_Config(1,115200,8,1,0,0); //默认值，若有改动
    // ATE1;                              // 开启回显(默认值)
    // ATE0;                              // 关闭回显
    // AT_CWAUTOCONN_1;                   // 设置上电自动连接 AP(默认值)
	  // AT_CWAUTOCONN_0;                   // 取消上电自动连接 AP(默认值)
    // WiFi_Reconnect(0, 0);              // 关闭断开AP后自动连接(默认值)
}

// Initailizing into TCP Server
void ESP_Init(void)
{
    Esp_Mode = Esp_Server;
    Esp_Reset();
    ESP_STA_AP;

#if WiFi_Connect_auto

    WiFi_Status state = WiFi_GetStatus();
    if (state == AP_Connected)
    {
        printf("WiFi连接成功\r\n");
    }
    else if (state == AP_Disconnected)
    {
        // 如果未连接WiFi,则临时连接备用AP
        printf("开始自动连接信号最好的WiFi...\r\n");
        WiFi_AutoConnect(&WiFiList);
    }

#else

    printf("开始自动连接信号最好的WiFi...\r\n");
    WiFi_AutoConnect(&WiFiList);

#endif
    AT;
    WiFi_GetIP(IP);
    printf("IP=\"%s\"\r\n", IP);
    AT_CIPMODE_0;
    AT_CIPMUX_1;
    AT_CIPSERVER_1;
		printf("ESP初始化结束，目前工作在TCP Server\r\n");
}

// TCP Client changes into TCP Server
void Esp_ClientToServer(void)
{
    Esp_Mode = Esp_Server;
    AT_CIPQUIT;
    HAL_Delay(1000);
    AT_CIPCLOSE;
    AT_CIPMODE_0;
    AT_CIPMUX_1;
    AT_CIPSERVER_1;
}

// TCP Server changes into TCP CLient
void Esp_ServerToClient(void)
{
    Esp_Mode = Esp_Client;
    AT_CIPCLOSE_5;
    AT_CIPSERVER_0;
    AT_CIPMUX_0;
}

void TimeProcess(CalendarInfo *t)
{
    uint8_t *resp = "GMT";
    uint8_t *p_end = (uint8_t *)strstr((const char *)Uart1_RxBuffer, (const char *)resp);
    uint8_t *p = p_end - 9;

    t->hour = ((*p - 0x30) * 10 + (*(p + 1) - 0x30) + 8) % 24; // GMT0-->GMT8

    t->min = ((*(p + 3) - 0x30) * 10 + (*(p + 4) - 0x30)) % 60;

    t->sec = ((*(p + 6) - 0x30) * 10 + (*(p + 7) - 0x30)) % 60;

    t->year = ((*(p - 5) - 0x30) * 1000 + (*(p - 4) - 0x30) * 100 + (*(p - 3) - 0x30) * 10 + (*(p - 2) - 0x30));

    t->date = ((*(p - 12) - 0x30) * 10 + (*(p - 11) - 0x30));

    if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Jan"))
        t->month = 1;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Feb"))
        t->month = 2;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Mar"))
        t->month = 3;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Apr"))
        t->month = 4;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "May"))
        t->month = 5;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Jun"))
        t->month = 6;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Jul"))
        t->month = 7;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Aug"))
        t->month = 8;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Sep"))
        t->month = 9;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Oct"))
        t->month = 10;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Nov"))
        t->month = 11;
    else if ((uint8_t *)strstr((const char *)Uart1_RxBuffer, "Dec"))
        t->month = 12;

    printf("time=%d年 %d月 %d日 %d时 %d分 %d秒\r\n", t->year, t->month, t->date, t->hour, t->min, t->sec);
}

void WeatherProcess(WeatherInfo *w)
{
    uint8_t *p = (uint8_t *)strstr((const char *)Uart1_RxBuffer, "text");
    uint8_t *p_end = (uint8_t *)strstr((const char *)(p + 7), "\"");
    *p_end = 0;
    sprintf((char *)&(w->text), "%s", (char *)(p + 7));
    p = (uint8_t *)strstr((const char *)Uart1_RxBuffer, "temperature");
    p = p + 14;
    p_end = (uint8_t *)strstr((const char *)p, "\"");
    uint8_t width = p_end - p;
    if ((*p) == '-')
    {
        if (width == 3)
        {
            w->temperature = 0 - ((*(p + 1) - '0') * 10 + (*(p + 2) - '0'));
        }
        else if (width == 2)
        {
            w->temperature = 0 - (*(p + 1) - '0');
        }
    }
    else
    {
        if (width == 2)
        {
            w->temperature = (*(p) - '0') * 10 + (*(p + 1) - '0');
        }
        else if (width == 1)
        {
            w->temperature = *p;
        }
    }
    printf("climat:%s\r\n", w->text);
    printf("temperature:%d°C\r\n", w->temperature);
}

// Get Time From API Specific
void Esp_GetTime(void)
{
    if (Esp_Mode == Esp_Server)
        Esp_ServerToClient();
    else if (Esp_Mode == Esp_Client)
    {
        AT_CIPQUIT;
        HAL_Delay(1000);
        AT_CIPCLOSE;
    }
    Esp8266_SendAT("AT+CIPSTART=\"TCP\",\"www.beijing-time.org\",80\r\n", "OK", 300);
    HAL_Delay(500);
    AT_CIPMODE_1;
    Esp_ClientSend((uint8_t *)("GET /time15.asp HTTP/1.1Host:www.beijing-time.org\r\n"));
    TimeProcess(&RealTime);
    Esp_ClientToServer();
}

// Get Weather From API Specific
void Esp_GetWeather(void)
{
    if (Esp_Mode == Esp_Server)
        Esp_ServerToClient();
    else if (Esp_Mode == Esp_Client)
    {
        AT_CIPQUIT;
        HAL_Delay(1000);
        AT_CIPCLOSE;
    }
    Esp8266_SendAT("AT+CIPSTART=\"TCP\",\"api.seniverse.com\",80\r\n", "OK", 300);
    HAL_Delay(500);
    AT_CIPMODE_1;
    Esp_ClientSend((uint8_t *)("GET https://api.seniverse.com/v3/weather/now.json?key=SkOzA0UGuEReItDhZ&location=beijing&language=zh-Hans&unit=c\n\n"));
    WeatherProcess(&RealWeather);
    Esp_ClientToServer();
}

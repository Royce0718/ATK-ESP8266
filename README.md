# Simple Usage

---

Esp_Readme.pdf为彩色，阅读起来更舒服

### 测试结果

>   以在STM32F411RET6上得到验证

### 使用说明

*   关于WiFi链表的使用，见main.c
*   请认真阅读源码
*   文章参考以及其他资源请见[正点原子_WIFI模块ATK-ESP8266](http://47.111.11.73/docs/modules/iot/atk-esp.html) 及 [ESP官网](https://docs.espressif.com/projects/esp-at/zh_CN/latest/esp32/AT_Command_Set/index.html)
    *   [CSDN](http://t.csdnimg.cn/JFc19)关于一些API的获取

*   对于多链接 TCP Server，请在同一局域网下
*   基本通讯思路：
    *   用户(Info)请求发到 ESP TCP Server
    *   ESP TCP Server转到 ESP TCP Client 
    *   ESP TCP Client Gets from API specific
    *   ESP TCP Client 转到 ESP TCP Server
    *   waiting for the next request
*   如有问题，尽情提出，代码完全开源，若有修改优化之处也请开源

### Attention

*   每条指令后记住紧跟 `\r\n`
*   透传模式，仅⽀支持 TCP 单连接和 UDP 固定通信对端的情况
*   多连接情况下 (AT+CIPMUX=1)，才能开启 TCP 服务器器。
*   必须在没有连接建立` AT+CIPMUX`的情况下，设置连接模式；
*   如果建立了 TCP 服务器，想切换为单连接，必须关闭服务器 `AT+CIPSERVER=0`，服务器仅支持多连接
*   注意Esp在TCP Server 与TCP Client 之间的来回切换，该操作回断开客户端与EspTCP Server之间的连接



### 单连接 TCP Client

1. **简单的指令测试**
   1. `AT `、`AT+RST` .etc
2. **连接WiFi**
   1. 设置ESP模式`AT+CWMODE=3`
       *   1：Station 、2：SoftAP、 3：SoftAP+Station
   2. 查看可用的WiFi` AT+CWLAP`
   3. 连接到WiFi` AT+CWJAP="ssid","password"`
   4. 查询本地 IP 地址` AT+CIFSR`
3. **连接TCP**
   1. 选择连接模式 `AT+CIPMUX=0`(单路0，多路1)
   2. 连接到TCP`AT+CIPSTART=<type>,<remote	IP>,<remote	
      port>`
      *   AT+CIPSTART="TCP","www.beijing-time.org",80
      *   AT+CIPSTART="TCP","api.seniverse.com",80
   3. 查询网络连接信息`AT+CIPSTATUS`
   4. 开启透传模式 `AT+CIPMODE=1`
   4. 发送数据`AT+CIPSEND`
4. **更换TCP连接**
   1. 退出透传模式`printf("+++")`而非`printf("+++\r\n")`断开端口的连接
       *   当输入 +++ 时，返回普通 AT 指令模式，请至少间隔 1 秒再发下一 条 AT 指令。
       *   进入透传模式发送数据，每包最大 2048 字节，或者每包数据以 20 ms 间隔区分。 
   2. 断开TCP链接`AT+CIPCLOSE`
   3. 重复第三步中的2，3，4
5. **转到多链接 TCP Server**
   1. 退出透传模式`printf("+++")`
   2. 断开TCP链接`AT+CIPCLOSE`
   3. 设置传输模式 `AT+CIPMODE=0`(普通传输模式：0，透传：1)
   4. 设置为多连接`AT+CIPMUX=1`
   5. 转到多链接 TCP Server`AT+CIPSERVER=1`
   6. 查询本地 IP 地址` AT+CIFSR`




* 若需更換WiFi，按以下步骤操作
    * 退出透传模式`printf("+++")`
    * 断开与当前AP(WiFi)的连接`AT+CWQAP`
        * 注意断开AP后，建立的TCP也会断开

    * 重复第二步中的2，3，4




### 多链接 TCP Server

1. **简单的指令测试**
   1. `AT `、`AT+RST` .etc
2. **连接WiFi**
   1. 设置ESP模式`AT+CWMODE=3`
       *   1：Station 、2：SoftAP、 3：SoftAP+Station
   2. 查看可用的WiFi` AT+CWLAP`
   3. 连接到WiFi` AT+CWJAP="ssid","password"`
   4. 查询本地 IP 地址` AT+CIFSR`
3. **建立TCP Server**
   1. 设置传输模式`AT+CIPMODE=0`(普通传输模式：0，透传：1)
   2. 选择连接模式 `AT+CIPMUX=1`(单路0，多路1)
   3. 建立 TCP 服务器器 `AT+CIPSERVER=1,80`
       *   创建 TCP 服务器器后，⾃自动建⽴立 TCP 服务器器监听。
       *   当有 TCP 客户端接⼊入，会⾃自动占⽤用⼀一个连接 ID
   4. 给指定ID Client发送数据
       *   先发`AT+CIPSEND=<linkID>,<length>`
       *   再发lenght长度的数据
   5. 自动接收数据`+IPD, 0, n: xxxxxxxxxx    // received n bytes,  data = xxxxxxxxxx`
   6. 断开指定的TCP Client`AT+CIPCLOSE=<linkID>`
4. **转到单连接 TCP Client**
   1. 断开多链接下的所有TCP连接`AT+CIPCLOSE=5`
   2. 关闭服务器 `AT+CIPSERVER=0`
   3. 设置单连接模式 `AT+CIPMUX=0`
   4. 见单连接步骤3==连接TCP==




*   设置 TCP 服务器器超时时间 ` AT+CIPSTO=<time>`(time:0-7200s)

    *   ESP8266 作为 TCP 服务器器，会断开⼀一直不不通信直⾄至超时了了的 TCP 客户端连接
    *   如果设置 AT+CIPSTO=0，则永远不不会超时，不不建议这样设置。

    

* 若需更換WiFi，按以下步骤操作
  
    *   关闭服务器 `AT+CIPSERVER=0`
    *   断开与当前AP(WiFi)的连接`AT+CWQAP`
    *   查看可用的WiFi` AT+CWLAP`
    *   连接到WiFi` AT+CWJAP="ssid","password"`





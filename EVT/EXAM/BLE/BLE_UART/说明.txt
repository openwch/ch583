
ch583 简单的串口透传:

特性:
1, 使用两个128bit uuid,
2, 两个uuid 分别是write without respone,和 notify 方式,分别对应串口收和发,可以在工程文件ble_uart_service/ble_uart_service.c中修改
3, 可以兼容 N* 家的 ble uart 的工程,
4, 支持MTU在20-247 中任意设置,自适适应当前的mtu
5, 默认在CH583上调试,串口使用的UART3,TXD3@PA5,RXD3@PA4,其他的串口需要修改代码
6, ble 名称为"ch583_ble_uart"
7, 默认开启串口notify 成功回写,不需要需要可以去掉代码,在ble service 的回掉函数,BLE_UART_EVT_BLE_DATA_RECIEVED 事件中 屏蔽即可
8, 默认开启串口调试,使用串口1,PA9_TXD 115200. 


一些参数修改:

见工程的config.h文件

1 修改mtu 长度,最大为251此时对应mtu是247,但是实际mtu是多少,要看central端连接时候协商的值
2 修改每个连接 最多传输多少个包数量
3,全局宏定义建议在mounriver stdio工程的properties>C/C++ General> Path and Symbols 的Symbols 标签下设置


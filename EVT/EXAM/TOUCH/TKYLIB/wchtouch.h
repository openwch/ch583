#ifndef __WCHTOUCH_H__
#define __WCHTOUCH_H__

/*********************************************************************
 * GLOBAL MACROS
 */
/*****************filter*****************/
#define FILTER_NONE    		      0       //--无滤波器--
#define FILTER_MODE_1    	      1       //--滤波器模式1, 可输出多个按键--
#define FILTER_MODE_3    	      3       //--滤波器模式3, 可输出多个按键--
#define FILTER_MODE_5             5       //--滤波器模式5, 可输出多个按键--
#define FILTER_MODE_7             7       //--滤波器模式7, 可输出多个按键--
#define FILTER_MODE_9             9       //--密集模式专用滤波器--
#define FILTER_MODE_CS10          2       //--CS10V专用滤波器模式--
/**************single key mode***********/
#define TKY_SINGLE_KEY_MULTI      0       //--多按键输出，即超过阈值的按键都会触发
#define TKY_SINGLE_KEY_MAX        1       //--最大值单按键输出，即超过阈值的按键中只上报变化量最大的按键
#define TKY_SINGLE_KEY_MUTU       2       //--互斥单按键输出，即当前按键释放后才会上报下一个变化量最大的按键，
                                          //  否则其他按键无论变化量多大都不上报
/***************lib param****************/
#define TKY_BUFLEN  			  24
/*----------------------------------------*/
typedef struct
{
	uint8_t maxQueueNum;				    //--测试队列数量--
	uint8_t singlePressMod;                	//---单按键模式---
	uint8_t shieldEn;                       //---屏蔽使能---
	uint8_t filterMode;				        //--滤波器模式--
	uint8_t filterGrade;				    //--滤波器等级--
	uint8_t peakQueueNum;                	//---按键最大偏移队列---
	uint8_t peakQueueOffset;             	//---按键最大偏移队列的偏移值---
	uint8_t baseRefreshOnPress;			    //--基线在按键按下时是否进行--
	uint8_t baseUpRefreshDouble;        	//---基线向上刷新倍速参数---
	uint8_t baseDownRefreshSlow;       		//---基线向下更新降速参数---
	uint32_t baseRefreshSampleNum;     		//--基线刷新采样次数--
	uint8_t *tkyBufP;					 	//--测试通道数据缓冲区指针--
}TKY_BaseInitTypeDef;

typedef struct
{
	uint8_t queueNum;                 		//--该通道在测试队列的序号--
	uint8_t channelNum;               		//--该通道对应的ADC通道标号--
	uint8_t chargeTime;		            	//--该通道充电时间--
	uint8_t disChargeTime;            		//--该通道放电时间--
	uint8_t sleepStatus;		          	//--休眠--
	uint16_t baseLine;  	   	        	//--基线--
	uint16_t threshold;		            	//--阈值--
	uint16_t threshold2;              		//--阈值2--
}TKY_ChannelInitTypeDef;


/*******************************************************************************
 * @fn              TKY_BaseInit
 * 
 * @brief           TouchKey总体参数初始化
 * 
 * @param           TKY_BaseInitStruct  初识化的参数
 * 
 * @return          初始化结果 - 0x00：初始化成功
 *                             - 0x01：滤波器模式参数错误
 *                             - 0x02：滤波器等级参数错误
 */
extern uint8_t TKY_BaseInit(TKY_BaseInitTypeDef TKY_BaseInitStruct);

/*******************************************************************************
 * @fn              TKY_CHInit
 * 
 * @brief           TouchKey通道参数初始化
 * 
 * @param           TKY_CHInitStruct - 初始化的参数
 * 
 * @return          初始化结果 - 0x00：初始化成功
 *                             - 0x01：触摸通道参数有错
 *                             - 0x02：通道转换队列位置错误(超过最大转换通道数量)
 *                             - 0x04：基线值设置错误
 *                             - 0x08：阈值设置错误
 */
extern uint8_t TKY_CHInit(TKY_ChannelInitTypeDef TKY_CHInitStruct);

/*******************************************************************************
 * @fn              TKY_GetCurChannelMean
 * @brief           获取当前通道的平均值，主要用于设置baseline和门槛值用途。
 * 
 * @param           curChNum      - 当前转换通道
 * @param           chargeTime    - 是当前通道的充电时间
 * @param           disChargeTime - 是当前通道的放电时间
 * @param           averageNum    - 是平均求和总数
 * 
 * @return          多次测量的平均值
 */
extern uint16_t TKY_GetCurChannelMean(uint8_t curChNum, uint8_t chargeTime, uint8_t disChargeTime, uint16_t averageNum);

/*******************************************************************************
 * @fn              TKY_CaliCrowedModBaseLine
 * @brief           校准密集模式下的基线值，在所有通道初识化完成之后进行,包括驱动屏蔽引脚
 * 
 * @param           curQueueNum - 当前转换通道序号
 * @param           averageNum  - 平均求和总数
 * 
 * @return          当前测试的基线值。
 */
extern uint16_t TKY_CaliCrowedModBaseLine( uint8_t curQueueNum,uint16_t averageNum);

/*******************************************************************************
 * @fn              TKY_GetCurQueueValue
 * 
 * @brief           获取指定通道处理后的值
 * 
 * @param           curQueueNum - 当前需要取值的通道
 * 
 * @return          滤波模式 1 时为所选择的通道滤波后的测量值(和基线值取差值，再和阈值比
 *                  较)，滤波模式 3、5、7、9、CS10 和 CS10BLE 时为所选择的通道滤波后的
 *                  变化值(可直接和阈值比较)
 */
extern int16_t TKY_GetCurQueueValue(uint8_t curQueueNum);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_1
 * 
 * @brief           TouchKey主循环轮询模式，适合滤波器1滤波
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有
 *                  按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_1(void);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_3
 * 
 * @brief           TouchKey主循环轮询模式，适合滤波器3滤波，执行过程阻塞
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有按键,
 *  				对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_3(void);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_5
 * 
 * @brief           TouchKey主循环轮询模式，与FilerMode3效果一致，执行过程非阻塞
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有
 * 					 按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_5(void);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_7
 * 
 * @brief           TouchKey主循环轮询模式，适合滤波器7滤波，执行过程阻塞
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有
 * 					按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_7(void);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_9
 * 
 * @brief           TouchKey主循环轮询模式，适合密集模式滤波器,执行过程阻塞
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有
 * 					 按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_9(void);

/*******************************************************************************
 * @fn              TKY_PollForFilterMode_CS10
 * 
 * @brief           TouchKey主循环轮询模式，适合用于CS10V测试
 * 
 * @return          各通道按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸通道有
 * 					 按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_PollForFilterMode_CS10(void);


/*******************************************************************************
 * @fn              TKY_ScanForWakeUp
 * 
 * @brief           TouchKey休眠检测，主循环定时轮询
 * 
 * @param           scanBitValue - 该值的置 1 的位，为指定扫描位
 *   				 例 0x0013，则对序列 0、1、4 通道进行扫描
 * 
 * @return          扫描通道疑似按键值，返回值各个位对应各个队列的按键，例如队列 0 的触摸
 *  				 通道有按键，对应最低位置 1。注意并非 ADC 通道编号
 */
extern uint16_t TKY_ScanForWakeUp(uint16_t scanBitValue);

/*******************************************************************************
 * @fn              TKY_SetCurQueueSleepStatus
 * 
 * @brief           设置指定队列通道的休眠模式，休眠后函数 TKY_PollForFilterMode
 *  				将不再对该通道进行扫描
 * 
 * @param           curQueueNum - 当前需要设置的通道
 * @param           sleepStatus - 0：不休眠
 *                              - 1：休眠
 * 
 * @return         设置状态  -  0：设置成功
 *                       - 1：超出队列最大长度
 */
extern uint8_t TKY_SetCurQueueSleepStatus(uint8_t curQueueNum, uint8_t sleepStatus);

/*******************************************************************************
 * @fn              TKY_SetSleepStatusValue
 * 
 * @brief           设置所有队列通道的休眠模式，与函数 TKY_SetCurQueueSleepStatus
 * 					 相比，本函数是直接配置全部通道休眠状态
 * 
 * @param           setValue - 以检测队列顺序按位设置睡眠状态，0：不休眠，1：休眠。
 * 
 * @return          None
 */
extern void TKY_SetSleepStatusValue(uint16_t setValue);

/*******************************************************************************
 * @fn              TKY_ReadSleepStatusValue
 * 
 * @brief           获取所有队列通道的休眠模式
 * 
 * @param           None
 * 
 * @return          返回所有通道的休眠状态，对应位为 0 则不休眠，为 1 则休眠
 */
extern uint16_t TKY_ReadSleepStatusValue(void);

/*******************************************************************************
 * @fn              TKY_SetCurQueueChargeTime
 * 
 * @brief           设置指定队列通道的充放电时间。对于滤波模式1&5,为确保安全更新设置,
 *                  请查询空闲状态(TKY_GetCurIdleStatus),在空闲时进行更新
 * 
 * @param           curQueueNum   - 当前需要设置的测量通道，注意并非ADC通道编号
 * @param           chargeTime    - 所设置的充电时间参数，参数值含义请查阅芯片手册
 * @param           disChargeTime - 所设置的放电时间参数，参数值含义请查阅芯片手册
 * 
 * @return          0 - 设置成功
 *                  1 - 超出队列最大长度。
 *******************************************************************************/
extern uint8_t TKY_SetCurQueueChargeTime( uint8_t curQueueNum,
										uint8_t chargeTime,
										uint8_t disChargeTime );

/*******************************************************************************
 * @fn              TKY_SetCurQueueThreshold
 * @brief           设置指定队列通道的阈值。对于滤波模式1&5,为确保安全更新设置，
 *                  请查询空闲状态(TKY_GetCurIdleStatus)，在空闲时进行更新
 *
 * @param           curQueueNum - 当前需要设置的通道。
 * @param           Threshold   - 该通道的上限阈值，即按键检测"按下"状态的判决值
 * @param           Threshold2  - 该通道的下限阈值，即按键检测"释放"状态的判决值
 *
 * @return          0 - 则设置成功
 *                  1 - 为超出队列最大长度
 *******************************************************************************/
extern uint8_t TKY_SetCurQueueThreshold(uint8_t curQueueNum,
									  uint16_t threshold,
									  uint16_t threshold2);

/*******************************************************************************
 * @fn              TKY_GetCurIdleStatus
 * 
 * @brief           获取空闲状态
 * 
 * @return          返回是否空闲
 *******************************************************************************/
extern uint8_t TKY_GetCurIdleStatus(void);

/*******************************************************************************
 * @fn              TKY_GetCurVersion
 * 
 * @brief           获取当前版本号
 * 
 * @return          当前版本号
 */
extern uint16_t TKY_GetCurVersion(void);

/*******************************************************************************
 * @fn              TKY_GetCurQueueBaseLine
 * 
 * @brief           获取指定通道基线值
 * 
 * @param           curQueueNum - 所选择的测量触摸通道队列编号，注意并非 ADC 通道编号
 * 
 * @return          所选择队列通道的基线值
 */
extern uint16_t TKY_GetCurQueueBaseLine(uint8_t curQueueNum);

/*******************************************************************************
 * @fn              TKY_SetCurQueueBaseLine
 * 
 * @brief           获取指定通道基线值。
 * 
 * @param           curQueueNum - 当前需要设置值的通道
 * @param           baseLineValue - 当前通道的设置值。
 * 
 * @return          当前通道的处理值。
 */
extern void TKY_SetCurQueueBaseLine(uint8_t curQueueNum, uint16_t baseLineValue);

/*******************************************************************************
 * @fn              TKY_SetBaseRefreshSampleNum
 * 
 * @brief           设置基线刷新采样次数，每隔多少次采样刷新一次基线。对于滤波模式1&5,
 *                  为确保安全更新设置，请查询空闲状态(TKY_GetCurIdleStatus)，
 *                  在空闲时进行更新
 * 
 * @param           newValue - 新采样次数
 * 
 * @return          无
 */
extern void TKY_SetBaseRefreshSampleNum(uint32_t newValue);

/*******************************************************************************
 * @fn              TKY_SetBaseDownRefreshSlow
 * 
 * @brief           设置基线向上更新的倍数参数。对于滤波模式1&5,为确保安全更新设置，
 *                  请查询空闲状态(TKY_GetCurIdleStatus)，在空闲时进行更新
 * 
 * @param           newValue - 新参数
 * 
 * @return          无
 */
extern void TKY_SetBaseUpRefreshDouble(uint8_t newValue);

/*******************************************************************************
 * @fn              TKY_SetBaseDownRefreshSlow
 * 
 * @brief           设置基线向下更新的减速参数。对于滤波模式1&5,为确保安全更新设置，
 *                  请查询空闲状态(TKY_GetCurIdleStatus)，在空闲时进行更新
 * 
 * @param           newValue - 新参数
 * 
 * @return          无
 */
extern void TKY_SetBaseDownRefreshSlow(uint8_t newValue);

/*******************************************************************************
 * @fn              TKY_SetFilterMode
 * 
 * @brief           设置新滤波模式。一般场景不建议使用，仅使用在休眠场景下特殊场景切换
 * 
 * @param           newValue - 滤波器模式
 * 
 * @return          无
 */
extern void TKY_SetFilterMode(uint8_t newValue);

/*******************************************************************************
 * @fn              TKY_ClearHistoryData
 * 
 * @brief           清除当前滤波器的历史数据。应用在触摸转换被较长时间打断，
 *                  历史数据无意义的场景。
 * 
 * @param           curFilterMode - 当前滤波模式
 * 
 * @return          无
 */
extern void TKY_ClearHistoryData(uint8_t curFilterMode);

/*******************************************************************************
 * @fn              TKY_SaveAndStop
 * 
 * @brief           保存触摸相关寄存器值，并且在判断触摸扫描空闲时暂停触摸功能，
 *                  以腾出 ADC 模块用于 ADC 转换
 * 
 * @param           无
 * 
 * @return          无
 */
extern void TKY_SaveAndStop(void);

/*******************************************************************************
 * @fn              TKY_LoadAndRun
 * 
 * @brief           载入触摸相关寄存器值，并重新启动被暂停的触摸按键功能
 * 
 * @param           无
 * 
 * @return          无
 */
extern void TKY_LoadAndRun(void);

/*******************************************************************************
 * @fn              TKY_GetCurQueueRealVal
 * 
 * @brief           获取指定通道原始测量值,未滤波的值。
 * 
 * @param           curQueueNum - 当前需要取值的通道。
 * 
 * @return          当前通道的原始测量值。
 */
extern uint16_t TKY_GetCurQueueRealVal(uint8_t curQueueNum);

#endif

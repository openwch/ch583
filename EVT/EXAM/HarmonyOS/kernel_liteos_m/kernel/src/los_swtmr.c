/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "los_swtmr.h"
#include "securec.h"
#include "los_interrupt.h"
#include "los_task.h"
#include "los_memory.h"
#include "los_queue.h"
#include "los_debug.h"
#include "los_hook.h"
#include "los_sched.h"


#if (LOSCFG_BASE_CORE_SWTMR == 1)

LITE_OS_SEC_BSS UINT32            g_swtmrHandlerQueue;           /* Software Timer timeout queue ID */
LITE_OS_SEC_BSS SWTMR_CTRL_S      *g_swtmrCBArray = NULL;        /* first address in Timer memory space */
LITE_OS_SEC_BSS SWTMR_CTRL_S      *g_swtmrFreeList = NULL;       /* Free list of Software Timer */
LITE_OS_SEC_BSS SortLinkAttribute *g_swtmrSortLinkList = NULL;       /* The software timer count list */

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
typedef struct SwtmrAlignDataStr {
    UINT32 times : 24;
    UINT32 : 5;
    UINT32 canMultiple : 1;
    UINT32 canAlign : 1;
    UINT32 isAligned : 1;
} SwtmrAlignData;
LITE_OS_SEC_BSS SwtmrAlignData      g_swtmrAlignID[LOSCFG_BASE_CORE_SWTMR_LIMIT] = {0};   /* store swtmr align */
#endif

#define SWTMR_MAX_RUNNING_TICKS 2
#define OS_SWTMR_MAX_TIMERID    ((0xFFFFFFFF / LOSCFG_BASE_CORE_SWTMR_LIMIT) * LOSCFG_BASE_CORE_SWTMR_LIMIT)

STATIC VOID OsSwtmrDelete(SWTMR_CTRL_S *swtmr);

/*****************************************************************************
Function    : OsSwtmrTask
Description : Swtmr task main loop, handle time-out timer.
Input       : None
Output      : None
Return      : None
*****************************************************************************/
LITE_OS_SEC_TEXT VOID OsSwtmrTask(VOID)
{
    SwtmrHandlerItem swtmrHandle;
    SWTMR_CTRL_S *swtmr = NULL;
    UINT32 intSave;
    UINT32 readSize;
    UINT32 ret;
    UINT64 tick;

    for (;;) {
        readSize = sizeof(SwtmrHandlerItem);
        ret = LOS_QueueReadCopy(g_swtmrHandlerQueue, &swtmrHandle, &readSize, LOS_WAIT_FOREVER);
        if ((ret == LOS_OK) && (readSize == sizeof(SwtmrHandlerItem))) {
            if ((swtmrHandle.handler == NULL) || (swtmrHandle.swtmrID >= OS_SWTMR_MAX_TIMERID)) {
                continue;
            }

            intSave = LOS_IntLock();
            swtmr = g_swtmrCBArray + swtmrHandle.swtmrID % LOSCFG_BASE_CORE_SWTMR_LIMIT;
            if (swtmr->usTimerID != swtmrHandle.swtmrID) {
                LOS_IntRestore(intSave);
                continue;
            }
            if (swtmr->ucMode == LOS_SWTMR_MODE_ONCE) {
                OsSwtmrDelete(swtmr);
            }
            LOS_IntRestore(intSave);

            tick = LOS_TickCountGet();
            swtmrHandle.handler(swtmrHandle.arg);
            tick = LOS_TickCountGet() - tick;
            if (tick >= SWTMR_MAX_RUNNING_TICKS) {
                PRINT_WARN("timer_handler(%p) cost too many ms(%d)\n",
                           swtmrHandle.handler,
                           (UINT32)((tick * OS_SYS_MS_PER_SECOND) / LOSCFG_BASE_CORE_TICK_PER_SECOND));
            }
        }
    }
}

/*****************************************************************************
Function    : OsSwtmrTaskCreate
Description : Create Software Timer
Input       : None
Output      : None
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT UINT32 OsSwtmrTaskCreate(VOID)
{
    UINT32 ret;
    TSK_INIT_PARAM_S swtmrTask;

    // Ignore the return code when matching CSEC rule 6.6(4).
    (VOID)memset_s(&swtmrTask, sizeof(TSK_INIT_PARAM_S), 0, sizeof(TSK_INIT_PARAM_S));

    swtmrTask.pfnTaskEntry    = (TSK_ENTRY_FUNC)OsSwtmrTask;
    swtmrTask.uwStackSize     = LOSCFG_BASE_CORE_TSK_SWTMR_STACK_SIZE;
    swtmrTask.pcName          = "Swt_Task";
    swtmrTask.usTaskPrio      = 0;
    ret = LOS_TaskCreate(&g_swtmrTaskID, &swtmrTask);
    if (ret == LOS_OK) {
        OS_TCB_FROM_TID(g_swtmrTaskID)->taskStatus |= OS_TASK_FLAG_SYSTEM_TASK;
    }
    return ret;
}

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
STATIC UINT64 OsSwtmrCalcStartTime(UINT64 currTime, SWTMR_CTRL_S *swtmr, const SWTMR_CTRL_S *alignSwtmr)
{
    UINT64 usedTime, startTime;
    UINT64 alignEnd = OS_SYS_TICK_TO_CYCLE(alignSwtmr->uwInterval);
    UINT64 swtmrTime = OS_SYS_TICK_TO_CYCLE(swtmr->uwInterval);
    UINT64 remainTime = OsSortLinkGetRemainTime(currTime, &alignSwtmr->stSortList);
    if (remainTime == 0) {
        startTime = GET_SORTLIST_VALUE(&alignSwtmr->stSortList);
    } else {
        usedTime = alignEnd - remainTime;
        startTime = alignSwtmr->startTime + (usedTime / swtmrTime) * swtmrTime;
    }

    return startTime;
}

UINT64 OsSwtmrFindAlignPos(UINT64 currTime, SWTMR_CTRL_S *swtmr)
{
    SWTMR_CTRL_S *minInLarge = (SWTMR_CTRL_S *)NULL;
    SWTMR_CTRL_S *maxInLittle = (SWTMR_CTRL_S *)NULL;
    UINT32 minInLargeVal = OS_NULL_INT;
    UINT32 maxInLittleVal = OS_NULL_INT;
    LOS_DL_LIST *listHead = &g_swtmrSortLinkList->sortLink;
    SwtmrAlignData swtmrAlgInfo = g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT];
    LOS_DL_LIST *listObject = listHead->pstNext;

    if (LOS_ListEmpty(listHead)) {
        goto RETURN_PERIOD;
    }

    do {
        SortLinkList *sortList = LOS_DL_LIST_ENTRY(listObject, SortLinkList, sortLinkNode);
        SWTMR_CTRL_S *swtmrListNode = LOS_DL_LIST_ENTRY(sortList, SWTMR_CTRL_S, stSortList);
        SwtmrAlignData alignListNode = g_swtmrAlignID[swtmrListNode->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT];

        /* swtmr not start */
        if ((alignListNode.isAligned == 0) || (alignListNode.canAlign == 0)) {
            goto CONTINUE_NEXT_NODE;
        }

        /* find same interval timer, directly return */
        if (swtmrListNode->uwInterval == swtmr->uwInterval) {
            return OsSwtmrCalcStartTime(currTime, swtmr, swtmrListNode);
        }

        if ((swtmrAlgInfo.canMultiple != 1) || (alignListNode.times == 0)) {
            goto CONTINUE_NEXT_NODE;
        }

        if (swtmrAlgInfo.times == 0) {
            goto RETURN_PERIOD;
        }

        if ((alignListNode.times >= swtmrAlgInfo.times) && ((alignListNode.times % swtmrAlgInfo.times) == 0)) {
            if (minInLargeVal > (alignListNode.times / swtmrAlgInfo.times)) {
                minInLargeVal = alignListNode.times / swtmrAlgInfo.times;
                minInLarge = swtmrListNode;
            }
        } else if ((alignListNode.times < swtmrAlgInfo.times) && ((swtmrAlgInfo.times % alignListNode.times) == 0)) {
            if (maxInLittleVal > (swtmrAlgInfo.times / alignListNode.times)) {
                maxInLittleVal = swtmrAlgInfo.times / alignListNode.times;
                maxInLittle = swtmrListNode;
            }
        }

CONTINUE_NEXT_NODE:
        listObject = listObject->pstNext;
    } while (listObject != listHead);

    if (minInLarge != NULL) {
        return OsSwtmrCalcStartTime(currTime, swtmr, minInLarge);
    } else if (maxInLittle != NULL) {
        return OsSwtmrCalcStartTime(currTime, swtmr, maxInLittle);
    }

RETURN_PERIOD:
    return currTime;
}
#endif

/*****************************************************************************
Function    : OsSwtmrStart
Description : Start Software Timer
Input       : currTime ------- Current system time
Input       : swtmr ---------- Need to start Software Timer
Output      : None
Return      : None
*****************************************************************************/
LITE_OS_SEC_TEXT VOID OsSwtmrStart(UINT64 currTime, SWTMR_CTRL_S *swtmr)
{
    swtmr->ucState = OS_SWTMR_STATUS_TICKING;

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    if ((g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT].canAlign == 1) &&
        (g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT].isAligned == 0)) {
        g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT].isAligned = 1;
        swtmr->startTime = OsSwtmrFindAlignPos(currTime, swtmr);
    }
#endif
    OsAdd2SortLink(&swtmr->stSortList, swtmr->startTime, swtmr->uwInterval, OS_SORT_LINK_SWTMR);
    OsSchedUpdateExpireTime();
}

/*****************************************************************************
Function    : OsSwtmrDelete
Description : Delete Software Timer
Input       : swtmr --- Need to delete Software Timer, When using, Ensure that it can't be NULL.
Output      : None
Return      : None
*****************************************************************************/
STATIC VOID OsSwtmrDelete(SWTMR_CTRL_S *swtmr)
{
    if (swtmr->usTimerID < (OS_SWTMR_MAX_TIMERID - LOSCFG_BASE_CORE_SWTMR_LIMIT)) {
        swtmr->usTimerID += LOSCFG_BASE_CORE_SWTMR_LIMIT;
    } else {
        swtmr->usTimerID %= LOSCFG_BASE_CORE_SWTMR_LIMIT;
    }

    /* insert to free list */
    swtmr->pstNext = g_swtmrFreeList;
    g_swtmrFreeList = swtmr;
    swtmr->ucState = OS_SWTMR_STATUS_UNUSED;

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    (VOID)memset_s((VOID *)&g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT],
                   sizeof(SwtmrAlignData), 0, sizeof(SwtmrAlignData));
#endif
}


LITE_OS_SEC_TEXT VOID OsSwtmrStop(SWTMR_CTRL_S *swtmr)
{
    OsDeleteSortLink(&swtmr->stSortList);
    swtmr->ucState = OS_SWTMR_STATUS_CREATED;

    swtmr->ucOverrun = 0;
    OsSchedUpdateExpireTime();
#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT].isAligned = 0;
#endif
}

STATIC VOID OsSwtmrTimeoutHandle(UINT64 currTime, SWTMR_CTRL_S *swtmr)
{
    SwtmrHandlerItem swtmrHandler;

    swtmrHandler.handler = swtmr->pfnHandler;
    swtmrHandler.arg = swtmr->uwArg;
    swtmrHandler.swtmrID = swtmr->usTimerID;

    (VOID)LOS_QueueWriteCopy(g_swtmrHandlerQueue, &swtmrHandler, sizeof(SwtmrHandlerItem), LOS_NO_WAIT);
    if (swtmr->ucMode == LOS_SWTMR_MODE_PERIOD) {
        swtmr->ucOverrun++;
        OsSwtmrStart(currTime, swtmr);
    } else if (swtmr->ucMode == LOS_SWTMR_MODE_NO_SELFDELETE) {
        swtmr->ucState = OS_SWTMR_STATUS_CREATED;
    }
}

STATIC BOOL OsSwtmrScan(VOID)
{
    BOOL needSchedule = FALSE;
    LOS_DL_LIST *listObject = &g_swtmrSortLinkList->sortLink;

    if (LOS_ListEmpty(listObject)) {
        return needSchedule;
    }

    SortLinkList *sortList = LOS_DL_LIST_ENTRY(listObject->pstNext, SortLinkList, sortLinkNode);
    UINT64 currTime = OsGetCurrSchedTimeCycle();
    while (sortList->responseTime <= currTime) {
        SWTMR_CTRL_S *swtmr = LOS_DL_LIST_ENTRY(sortList, SWTMR_CTRL_S, stSortList);
        swtmr->startTime = GET_SORTLIST_VALUE(sortList);

        OsDeleteNodeSortLink(sortList);
        OsHookCall(LOS_HOOK_TYPE_SWTMR_EXPIRED, swtmr);
        OsSwtmrTimeoutHandle(currTime, swtmr);

        needSchedule = TRUE;
        if (LOS_ListEmpty(listObject)) {
            break;
        }

        sortList = LOS_DL_LIST_ENTRY(listObject->pstNext, SortLinkList, sortLinkNode);
    }

    return needSchedule;
}

LITE_OS_SEC_TEXT VOID OsSwtmrResponseTimeReset(UINT64 startTime)
{
    LOS_DL_LIST *listHead = &g_swtmrSortLinkList->sortLink;
    LOS_DL_LIST *listNext = listHead->pstNext;

    while (listNext != listHead) {
        SortLinkList *sortList = LOS_DL_LIST_ENTRY(listNext, SortLinkList, sortLinkNode);
        SWTMR_CTRL_S *swtmr = LOS_DL_LIST_ENTRY(sortList, SWTMR_CTRL_S, stSortList);
        OsDeleteNodeSortLink(sortList);
#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
        g_swtmrAlignID[swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT].isAligned = 0;
#endif
        swtmr->startTime = startTime;
        OsSwtmrStart(startTime, swtmr);
        listNext = listNext->pstNext;
    }
}

/*****************************************************************************
Function    : OsSwtmrGetNextTimeout
Description : Get next timeout
Input       : None
Output      : None
Return      : Count of the Timer list
*****************************************************************************/
LITE_OS_SEC_TEXT UINT32 OsSwtmrGetNextTimeout(VOID)
{
    UINT32 intSave = LOS_IntLock();
    UINT64 time = OsSortLinkGetNextExpireTime(g_swtmrSortLinkList);
    LOS_IntRestore(intSave);
    time = OS_SYS_CYCLE_TO_TICK(time);
    if (time > OS_NULL_INT) {
        time = OS_NULL_INT;
    }
    return time;
}

LITE_OS_SEC_TEXT UINT32 OsSwtmrTimeGet(const SWTMR_CTRL_S *swtmr)
{
    UINT64 time = OsSortLinkGetTargetExpireTime(OsGetCurrSchedTimeCycle(), &swtmr->stSortList);
    time = OS_SYS_CYCLE_TO_TICK(time);
    if (time > OS_NULL_INT) {
        time = OS_NULL_INT;
    }
    return (UINT32)time;
}

/*****************************************************************************
Function    : OsSwtmrInit
Description : Initializes Software Timer
Input       : None
Output      : None
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT UINT32 OsSwtmrInit(VOID)
{
    UINT32 size;
    UINT16 index;
    UINT32 ret;

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    // Ignore the return code when matching CSEC rule 6.6(1).
    (VOID)memset_s((VOID *)g_swtmrAlignID, sizeof(SwtmrAlignData) * LOSCFG_BASE_CORE_SWTMR_LIMIT,
                   0, sizeof(SwtmrAlignData) * LOSCFG_BASE_CORE_SWTMR_LIMIT);
#endif

    size = sizeof(SWTMR_CTRL_S) * LOSCFG_BASE_CORE_SWTMR_LIMIT;
    SWTMR_CTRL_S *swtmr = (SWTMR_CTRL_S *)LOS_MemAlloc(m_aucSysMem0, size);
    if (swtmr == NULL) {
        return LOS_ERRNO_SWTMR_NO_MEMORY;
    }
    // Ignore the return code when matching CSEC rule 6.6(3).
    (VOID)memset_s((VOID *)swtmr, size, 0, size);
    g_swtmrCBArray = swtmr;
    g_swtmrFreeList = swtmr;
    swtmr->usTimerID = 0;
    SWTMR_CTRL_S *temp = swtmr;
    swtmr++;
    for (index = 1; index < LOSCFG_BASE_CORE_SWTMR_LIMIT; index++, swtmr++) {
        swtmr->usTimerID = index;
        temp->pstNext = swtmr;
        temp = swtmr;
    }

    ret = LOS_QueueCreate((CHAR *)NULL, OS_SWTMR_HANDLE_QUEUE_SIZE,
                          &g_swtmrHandlerQueue, 0, sizeof(SwtmrHandlerItem));
    if (ret != LOS_OK) {
        (VOID)LOS_MemFree(m_aucSysMem0, swtmr);
        return LOS_ERRNO_SWTMR_QUEUE_CREATE_FAILED;
    }

    ret = OsSwtmrTaskCreate();
    if (ret != LOS_OK) {
        (VOID)LOS_MemFree(m_aucSysMem0, swtmr);
        return LOS_ERRNO_SWTMR_TASK_CREATE_FAILED;
    }

    g_swtmrSortLinkList = OsGetSortLinkAttribute(OS_SORT_LINK_SWTMR);
    if (g_swtmrSortLinkList == NULL) {
        (VOID)LOS_MemFree(m_aucSysMem0, swtmr);
        return LOS_NOK;
    }

    ret = OsSortLinkInit(g_swtmrSortLinkList);
    if (ret != LOS_OK) {
        (VOID)LOS_MemFree(m_aucSysMem0, swtmr);
        return LOS_NOK;
    }

    ret = OsSchedSwtmrScanRegister((SchedScan)OsSwtmrScan);
    if (ret != LOS_OK) {
        (VOID)LOS_MemFree(m_aucSysMem0, swtmr);
        return LOS_NOK;
    }

    return LOS_OK;
}

/*****************************************************************************
Function    : LOS_SwtmrCreate
Description : Create software timer
Input       : interval
              mode
              handler
              arg
Output      : swtmrId
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
LITE_OS_SEC_TEXT_INIT UINT32 LOS_SwtmrCreate(UINT32 interval,
                                             UINT8 mode,
                                             SWTMR_PROC_FUNC handler,
                                             UINT32 *swtmrId,
                                             UINT32 arg,
                                             UINT8 rouses,
                                             UINT8 sensitive)
#else
LITE_OS_SEC_TEXT_INIT UINT32 LOS_SwtmrCreate(UINT32 interval,
                                             UINT8 mode,
                                             SWTMR_PROC_FUNC handler,
                                             UINT32 *swtmrId,
                                             UINT32 arg)
#endif
{
    SWTMR_CTRL_S  *swtmr = NULL;
    UINT32 intSave;

    if (interval == 0) {
        return LOS_ERRNO_SWTMR_INTERVAL_NOT_SUITED;
    }

    if ((mode != LOS_SWTMR_MODE_ONCE) &&
        (mode != LOS_SWTMR_MODE_PERIOD) &&
        (mode != LOS_SWTMR_MODE_NO_SELFDELETE)) {
        return LOS_ERRNO_SWTMR_MODE_INVALID;
    }

    if (handler == NULL) {
        return LOS_ERRNO_SWTMR_PTR_NULL;
    }

    if (swtmrId == NULL) {
        return LOS_ERRNO_SWTMR_RET_PTR_NULL;
    }

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    if ((rouses != OS_SWTMR_ROUSES_IGNORE) && (rouses != OS_SWTMR_ROUSES_ALLOW)) {
        return OS_ERRNO_SWTMR_ROUSES_INVALID;
    }

    if ((sensitive != OS_SWTMR_ALIGN_INSENSITIVE) && (sensitive != OS_SWTMR_ALIGN_SENSITIVE)) {
        return OS_ERRNO_SWTMR_ALIGN_INVALID;
    }
#endif

    intSave = LOS_IntLock();
    if (g_swtmrFreeList == NULL) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SWTMR_MAXSIZE;
    }

    swtmr = g_swtmrFreeList;
    g_swtmrFreeList = swtmr->pstNext;
    LOS_IntRestore(intSave);
    swtmr->pfnHandler    = handler;
    swtmr->ucMode        = mode;
    swtmr->uwInterval    = interval;
    swtmr->pstNext       = (SWTMR_CTRL_S *)NULL;
    swtmr->uwArg         = arg;
#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    swtmr->ucRouses      = rouses;
    swtmr->ucSensitive   = sensitive;
#endif
    swtmr->ucState       = OS_SWTMR_STATUS_CREATED;
    swtmr->ucOverrun     = 0;
    *swtmrId = swtmr->usTimerID;
    SET_SORTLIST_VALUE(&swtmr->stSortList, OS_SORT_LINK_INVALID_TIME);
    OsHookCall(LOS_HOOK_TYPE_SWTMR_CREATE, swtmr);
    return LOS_OK;
}

/*****************************************************************************
Function    : LOS_SwtmrStart
Description : Start software timer
Input       : swtmrId ------- Software timer ID
Output      : None
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT UINT32 LOS_SwtmrStart(UINT32 swtmrId)
{
    UINT32 intSave;
    UINT32 ret = LOS_OK;

    if (swtmrId >= OS_SWTMR_MAX_TIMERID) {
        return LOS_ERRNO_SWTMR_ID_INVALID;
    }

    intSave = LOS_IntLock();
    SWTMR_CTRL_S *swtmr = g_swtmrCBArray + swtmrId % LOSCFG_BASE_CORE_SWTMR_LIMIT;
    if (swtmr->usTimerID != swtmrId) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SWTMR_NOT_CREATED;
    }

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    if ((swtmr->ucSensitive == OS_SWTMR_ALIGN_INSENSITIVE) && (swtmr->ucMode == LOS_SWTMR_MODE_PERIOD)) {
        UINT32 swtmrAlignIdIndex = swtmr->usTimerID % LOSCFG_BASE_CORE_SWTMR_LIMIT;
        g_swtmrAlignID[swtmrAlignIdIndex].canAlign = 1;
        if ((swtmr->uwInterval % LOS_COMMON_DIVISOR) == 0) {
            g_swtmrAlignID[swtmrAlignIdIndex].canMultiple = 1;
            g_swtmrAlignID[swtmrAlignIdIndex].times = swtmr->uwInterval / LOS_COMMON_DIVISOR;
        }
    }
#endif

    switch (swtmr->ucState) {
        case OS_SWTMR_STATUS_UNUSED:
            ret = LOS_ERRNO_SWTMR_NOT_CREATED;
            break;
        case OS_SWTMR_STATUS_TICKING:
            OsSwtmrStop(swtmr);
            /* fall through */
        case OS_SWTMR_STATUS_CREATED:
            swtmr->startTime = OsGetCurrSchedTimeCycle();
            OsSwtmrStart(swtmr->startTime, swtmr);
            break;
        default:
            ret = LOS_ERRNO_SWTMR_STATUS_INVALID;
            break;
    }

    LOS_IntRestore(intSave);
    OsHookCall(LOS_HOOK_TYPE_SWTMR_START, swtmr);
    return ret;
}

/*****************************************************************************
Function    : LOS_SwtmrStop
Description : Stop software timer
Input       : swtmrId ------- Software timer ID
Output      : None
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT UINT32 LOS_SwtmrStop(UINT32 swtmrId)
{
    SWTMR_CTRL_S *swtmr = NULL;
    UINT32 intSave;
    UINT16 swtmrCbId;
    UINT32 ret = LOS_OK;

    if (swtmrId >= OS_SWTMR_MAX_TIMERID) {
        return LOS_ERRNO_SWTMR_ID_INVALID;
    }
    intSave = LOS_IntLock();
    swtmrCbId = swtmrId % LOSCFG_BASE_CORE_SWTMR_LIMIT;
    swtmr = g_swtmrCBArray + swtmrCbId;
    if (swtmr->usTimerID != swtmrId) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SWTMR_NOT_CREATED;
    }

    switch (swtmr->ucState) {
        case OS_SWTMR_STATUS_UNUSED:
            ret = LOS_ERRNO_SWTMR_NOT_CREATED;
            break;
        case OS_SWTMR_STATUS_CREATED:
            ret = LOS_ERRNO_SWTMR_NOT_STARTED;
            break;
        case OS_SWTMR_STATUS_TICKING:
            OsSwtmrStop(swtmr);
            break;
        default:
            ret = LOS_ERRNO_SWTMR_STATUS_INVALID;
            break;
    }

    LOS_IntRestore(intSave);
    OsHookCall(LOS_HOOK_TYPE_SWTMR_STOP, swtmr);
    return ret;
}

LITE_OS_SEC_TEXT UINT32 LOS_SwtmrTimeGet(UINT32 swtmrId, UINT32 *tick)
{
    SWTMR_CTRL_S *swtmr = NULL;
    UINT32 intSave;
    UINT32 ret = LOS_OK;
    UINT16 swtmrCbId;

    if (swtmrId >= OS_SWTMR_MAX_TIMERID) {
        return LOS_ERRNO_SWTMR_ID_INVALID;
    }

    if (tick == NULL) {
        return LOS_ERRNO_SWTMR_TICK_PTR_NULL;
    }

    intSave = LOS_IntLock();
    swtmrCbId = swtmrId % LOSCFG_BASE_CORE_SWTMR_LIMIT;
    swtmr = g_swtmrCBArray + swtmrCbId;
    if (swtmr->usTimerID != swtmrId) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SWTMR_NOT_CREATED;
    }
    switch (swtmr->ucState) {
        case OS_SWTMR_STATUS_UNUSED:
            ret = LOS_ERRNO_SWTMR_NOT_CREATED;
            break;
        case OS_SWTMR_STATUS_CREATED:
            ret = LOS_ERRNO_SWTMR_NOT_STARTED;
            break;
        case OS_SWTMR_STATUS_TICKING:
            *tick = OsSwtmrTimeGet(swtmr);
            break;
        default:
            ret = LOS_ERRNO_SWTMR_STATUS_INVALID;
            break;
    }
    LOS_IntRestore(intSave);
    return ret;
}

/*****************************************************************************
Function    : LOS_SwtmrDelete
Description : Delete software timer
Input       : swtmrId ------- Software timer ID
Output      : None
Return      : LOS_OK on success or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT UINT32 LOS_SwtmrDelete(UINT32 swtmrId)
{
    SWTMR_CTRL_S *swtmr = NULL;
    UINT32 intSave;
    UINT32 ret = LOS_OK;
    UINT16 swtmrCbId;

    if (swtmrId >= OS_SWTMR_MAX_TIMERID) {
        return LOS_ERRNO_SWTMR_ID_INVALID;
    }
    intSave = LOS_IntLock();
    swtmrCbId = swtmrId % LOSCFG_BASE_CORE_SWTMR_LIMIT;
    swtmr = g_swtmrCBArray + swtmrCbId;
    if (swtmr->usTimerID != swtmrId) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SWTMR_NOT_CREATED;
    }

    switch (swtmr->ucState) {
        case OS_SWTMR_STATUS_UNUSED:
            ret = LOS_ERRNO_SWTMR_NOT_CREATED;
            break;
        case OS_SWTMR_STATUS_TICKING:
            OsSwtmrStop(swtmr);
            /* fall through */
        case OS_SWTMR_STATUS_CREATED:
            OsSwtmrDelete(swtmr);
            break;
        default:
            ret = LOS_ERRNO_SWTMR_STATUS_INVALID;
            break;
    }

    LOS_IntRestore(intSave);
    OsHookCall(LOS_HOOK_TYPE_SWTMR_DELETE, swtmr);
    return ret;
}

#endif /* (LOSCFG_BASE_CORE_SWTMR == 1) */

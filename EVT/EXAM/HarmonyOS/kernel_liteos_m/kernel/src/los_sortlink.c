/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
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

#include "los_sortlink.h"
#include "los_sched.h"
#include "los_debug.h"

SortLinkAttribute g_taskSortLink;

#if (LOSCFG_BASE_CORE_SWTMR == 1)
SortLinkAttribute g_swtmrSortLink;
#endif

UINT32 OsSortLinkInit(SortLinkAttribute *sortLinkHead)
{
    LOS_ListInit(&sortLinkHead->sortLink);
    return LOS_OK;
}

STATIC INLINE VOID OsAddNode2SortLink(SortLinkAttribute *sortLinkHead, SortLinkList *sortList)
{
    LOS_DL_LIST *head = (LOS_DL_LIST *)&sortLinkHead->sortLink;

    if (LOS_ListEmpty(head)) {
        LOS_ListAdd(head, &sortList->sortLinkNode);
        return;
    }

    SortLinkList *listSorted = LOS_DL_LIST_ENTRY(head->pstNext, SortLinkList, sortLinkNode);
    if (listSorted->responseTime > sortList->responseTime) {
        LOS_ListAdd(head, &sortList->sortLinkNode);
        return;
    } else if (listSorted->responseTime == sortList->responseTime) {
        LOS_ListAdd(head->pstNext, &sortList->sortLinkNode);
        return;
    }

    LOS_DL_LIST *prevNode = head->pstPrev;
    do {
        listSorted = LOS_DL_LIST_ENTRY(prevNode, SortLinkList, sortLinkNode);
        if (listSorted->responseTime <= sortList->responseTime) {
            LOS_ListAdd(prevNode, &sortList->sortLinkNode);
            break;
        }

        prevNode = prevNode->pstPrev;
    } while (1);
}

VOID OsAdd2SortLink(SortLinkList *node, UINT64 startTime, UINT32 waitTicks, SortLinkType type)
{
    UINT32 intSave;
    SortLinkAttribute *sortLinkHead = NULL;

    if (type == OS_SORT_LINK_TASK) {
        sortLinkHead = &g_taskSortLink;
#if (LOSCFG_BASE_CORE_SWTMR == 1)
    } else if (type == OS_SORT_LINK_SWTMR) {
        sortLinkHead = &g_swtmrSortLink;
#endif
    } else {
        LOS_Panic("Sort link type error : %u\n", type);
    }

    intSave = LOS_IntLock();
    SET_SORTLIST_VALUE(node, startTime + OS_SYS_TICK_TO_CYCLE(waitTicks));
    OsAddNode2SortLink(sortLinkHead, node);
    LOS_IntRestore(intSave);
}

VOID OsDeleteSortLink(SortLinkList *node)
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    if (node->responseTime != OS_SORT_LINK_INVALID_TIME) {
        OsSchedResetSchedResponseTime(node->responseTime);
        OsDeleteNodeSortLink(node);
    }
    LOS_IntRestore(intSave);
}

STATIC INLINE VOID SortLinkNodeTimeUpdate(SortLinkAttribute *sortLinkHead, UINT32 oldFreq)
{
    LOS_DL_LIST *head = (LOS_DL_LIST *)&sortLinkHead->sortLink;

    if (LOS_ListEmpty(head)) {
        return;
    }

    LOS_DL_LIST *nextNode = head->pstNext;
    do {
        SortLinkList *listSorted = LOS_DL_LIST_ENTRY(nextNode, SortLinkList, sortLinkNode);
        listSorted->responseTime = OsTimeConvertFreq(listSorted->responseTime, oldFreq, g_sysClock);
        nextNode = nextNode->pstNext;
    } while (nextNode != head);
}

VOID OsSortLinkResponseTimeConvertFreq(UINT32 oldFreq)
{
    SortLinkAttribute *taskHead = &g_taskSortLink;
    SortLinkNodeTimeUpdate(taskHead, oldFreq);

#if (LOSCFG_BASE_CORE_SWTMR == 1)
    SortLinkAttribute *swtmrHead = &g_swtmrSortLink;
    SortLinkNodeTimeUpdate(swtmrHead, oldFreq);
#endif
}

SortLinkAttribute *OsGetSortLinkAttribute(SortLinkType type)
{
    if (type == OS_SORT_LINK_TASK) {
        return &g_taskSortLink;
#if (LOSCFG_BASE_CORE_SWTMR == 1)
    } else if (type == OS_SORT_LINK_SWTMR) {
        return &g_swtmrSortLink;
#endif
    }

    PRINT_ERR("Invalid sort link type!\n");
    return NULL;
}

UINT64 OsSortLinkGetTargetExpireTime(UINT64 currTime, const SortLinkList *targetSortList)
{
    if (currTime >= targetSortList->responseTime) {
        return 0;
    }

    return (targetSortList->responseTime - currTime);
}

UINT64 OsSortLinkGetNextExpireTime(const SortLinkAttribute *sortLinkHead)
{
    LOS_DL_LIST *head = (LOS_DL_LIST *)&sortLinkHead->sortLink;

    if (LOS_ListEmpty(head)) {
        return 0;
    }

    SortLinkList *listSorted = LOS_DL_LIST_ENTRY(head->pstNext, SortLinkList, sortLinkNode);
    return OsSortLinkGetTargetExpireTime(OsGetCurrSchedTimeCycle(), listSorted);
}

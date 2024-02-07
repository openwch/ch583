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

#ifndef _LOS_SORTLINK_H
#define _LOS_SORTLINK_H

#include "los_compiler.h"
#include "los_list.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef enum {
    OS_SORT_LINK_TASK = 1,
    OS_SORT_LINK_SWTMR = 2,
} SortLinkType;

typedef struct {
    LOS_DL_LIST sortLinkNode;
    UINT64      responseTime;
} SortLinkList;

typedef struct {
    LOS_DL_LIST sortLink;
} SortLinkAttribute;

extern SortLinkAttribute g_taskSortLink;

#if (LOSCFG_BASE_CORE_SWTMR == 1)
extern SortLinkAttribute g_swtmrSortLink;
#endif

#define OS_SORT_LINK_INVALID_TIME ((UINT64)-1)
#define SET_SORTLIST_VALUE(sortList, value) (((SortLinkList *)(sortList))->responseTime = (value))
#define GET_SORTLIST_VALUE(sortList) (((SortLinkList *)(sortList))->responseTime)

#define OS_SORT_LINK_UINT64_MAX ((UINT64)-1)

STATIC INLINE UINT64 OsSortLinkGetRemainTime(UINT64 currTime, const SortLinkList *targetSortList)
{
    if (currTime >= targetSortList->responseTime) {
        return 0;
    }
    return (targetSortList->responseTime - currTime);
}

STATIC INLINE VOID OsDeleteNodeSortLink(SortLinkList *sortList)
{
    LOS_ListDelete(&sortList->sortLinkNode);
    SET_SORTLIST_VALUE(sortList, OS_SORT_LINK_INVALID_TIME);
}

STATIC INLINE UINT64 GetSortLinkNextExpireTime(SortLinkAttribute *sortHead, UINT64 startTime, UINT32 tickPrecision)
{
    LOS_DL_LIST *head = &sortHead->sortLink;
    LOS_DL_LIST *list = head->pstNext;

    if (LOS_ListEmpty(head)) {
        return OS_SORT_LINK_UINT64_MAX - tickPrecision;
    }

    SortLinkList *listSorted = LOS_DL_LIST_ENTRY(list, SortLinkList, sortLinkNode);
    if (listSorted->responseTime <= (startTime + tickPrecision)) {
        return (startTime + tickPrecision);
    }

    return listSorted->responseTime;
}

STATIC INLINE UINT64 OsGetNextExpireTime(UINT64 startTime, UINT32 tickPrecision)
{
    UINT64 taskExpireTime = GetSortLinkNextExpireTime(&g_taskSortLink, startTime, tickPrecision);
#if (LOSCFG_BASE_CORE_SWTMR == 1)
    UINT64 swtmrExpireTime = GetSortLinkNextExpireTime(&g_swtmrSortLink, startTime, tickPrecision);
#else
    UINT64 swtmrExpireTime = taskExpireTime;
#endif
    return (taskExpireTime < swtmrExpireTime) ? taskExpireTime : swtmrExpireTime;
}

SortLinkAttribute *OsGetSortLinkAttribute(SortLinkType type);
UINT32 OsSortLinkInit(SortLinkAttribute *sortLinkHead);
VOID OsAdd2SortLink(SortLinkList *node, UINT64 startTime, UINT32 waitTicks, SortLinkType type);
VOID OsDeleteSortLink(SortLinkList *node);
UINT64 OsSortLinkGetTargetExpireTime(UINT64 currTime, const SortLinkList *targetSortList);
UINT64 OsSortLinkGetNextExpireTime(const SortLinkAttribute *sortLinkHead);
VOID OsSortLinkResponseTimeConvertFreq(UINT32 oldFreq);


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_SORTLINK_H */

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

#include "los_memory.h"
#include "securec.h"
#include "los_arch.h"
#include "los_config.h"
#include "los_debug.h"
#include "los_hook.h"
#include "los_interrupt.h"
#include "los_task.h"
#ifdef LOSCFG_KERNEL_LMS
#include "los_lms_pri.h"
#endif
#if (LOSCFG_KERNEL_LMK == 1)
#include "los_lmk.h"
#endif

/* Used to cut non-essential functions. */
#define OS_MEM_EXPAND_ENABLE    0

UINT8 *m_aucSysMem0 = NULL;

#if (LOSCFG_SYS_EXTERNAL_HEAP == 0)
STATIC UINT8 g_memStart[LOSCFG_SYS_HEAP_SIZE];
#endif

#if (LOSCFG_MEM_MUL_POOL == 1)
VOID *g_poolHead = NULL;
#endif

/* The following is the macro definition and interface implementation related to the TLSF. */

#define OS_MEM_BITMAP_MASK 0x1FU

/* Used to find the first bit of 1 in bitmap. */
STATIC INLINE UINT16 OsMemFFS(UINT32 bitmap)
{
    bitmap &= ~bitmap + 1;
    return (OS_MEM_BITMAP_MASK - CLZ(bitmap));
}

/* Used to find the last bit of 1 in bitmap. */
STATIC INLINE UINT16 OsMemFLS(UINT32 bitmap)
{
    return (OS_MEM_BITMAP_MASK - CLZ(bitmap));
}

STATIC INLINE UINT32 OsMemLog2(UINT32 size)
{
    return (size > 0) ? OsMemFLS(size) : 0;
}

/* Get the first level: f = log2(size). */
STATIC INLINE UINT32 OsMemFlGet(UINT32 size)
{
    if (size < OS_MEM_SMALL_BUCKET_MAX_SIZE) {
        return ((size >> 2) - 1); /* 2: The small bucket setup is 4. */
    }
    return (OsMemLog2(size) - OS_MEM_LARGE_START_BUCKET + OS_MEM_SMALL_BUCKET_COUNT);
}

/* Get the second level: s = (size - 2^f) * 2^SLI / 2^f. */
STATIC INLINE UINT32 OsMemSlGet(UINT32 size, UINT32 fl)
{
    if ((fl < OS_MEM_SMALL_BUCKET_COUNT) || (size < OS_MEM_SMALL_BUCKET_MAX_SIZE)) {
        PRINT_ERR("fl or size is too small, fl = %u, size = %u\n", fl, size);
        return 0;
    }

    UINT32 sl = (size << OS_MEM_SLI) >> (fl - OS_MEM_SMALL_BUCKET_COUNT + OS_MEM_LARGE_START_BUCKET);
    return (sl - (1 << OS_MEM_SLI));
}

/* The following is the memory algorithm related macro definition and interface implementation. */
#if (LOSCFG_TASK_MEM_USED != 1 && LOSCFG_MEM_FREE_BY_TASKID == 1 && (LOSCFG_BASE_CORE_TSK_LIMIT + 1) > 64)
#error "When enter here, LOSCFG_BASE_CORE_TSK_LIMIT larger than 63 is not support"
#endif

struct OsMemUsedNodeHead {
    struct OsMemNodeHead header;
};

/* The memory pool support expand. */
#define OS_MEM_POOL_EXPAND_ENABLE   0x01
/* The memory pool support no lock. */
#define OS_MEM_POOL_UNLOCK_ENABLE   0x02

#define MEM_LOCK(pool, state)       do {                    \
    if (!((pool)->info.attr & OS_MEM_POOL_UNLOCK_ENABLE)) { \
        (state) = LOS_IntLock();                            \
    }                                                       \
} while (0);
#define MEM_UNLOCK(pool, state)     do {                    \
    if (!((pool)->info.attr & OS_MEM_POOL_UNLOCK_ENABLE)) { \
        LOS_IntRestore(state);                              \
    }                                                       \
} while (0);

#define OS_MEM_NODE_MAGIC          0xABCDDCBA
#if (LOSCFG_TASK_MEM_USED != 1 && LOSCFG_MEM_FREE_BY_TASKID == 1)
#define OS_MEM_NODE_USED_FLAG      (1U << 25)
#define OS_MEM_NODE_ALIGNED_FLAG   (1U << 24)
#if (LOSCFG_MEM_LEAKCHECK == 1)
#define OS_MEM_NODE_LEAK_FLAG      (1U << 23)
#else
#define OS_MEM_NODE_LEAK_FLAG      0
#endif
#if (OS_MEM_EXPAND_ENABLE == 1)
#define OS_MEM_NODE_LAST_FLAG      (1U << 22)  /* Sentinel Node */
#else
#define OS_MEM_NODE_LAST_FLAG      0
#endif
#else
#define OS_MEM_NODE_USED_FLAG      (1U << 31)
#define OS_MEM_NODE_ALIGNED_FLAG   (1U << 30)
#if (LOSCFG_MEM_LEAKCHECK == 1)
#define OS_MEM_NODE_LEAK_FLAG      (1U << 29)
#else
#define OS_MEM_NODE_LEAK_FLAG      0
#endif
#if (OS_MEM_EXPAND_ENABLE == 1)
#define OS_MEM_NODE_LAST_FLAG      (1U << 28)  /* Sentinel Node */
#else
#define OS_MEM_NODE_LAST_FLAG      0
#endif
#endif

#define OS_MEM_NODE_ALIGNED_AND_USED_FLAG \
    (OS_MEM_NODE_USED_FLAG | OS_MEM_NODE_ALIGNED_FLAG | OS_MEM_NODE_LEAK_FLAG | OS_MEM_NODE_LAST_FLAG)

#define OS_MEM_NODE_GET_ALIGNED_FLAG(sizeAndFlag) \
            ((sizeAndFlag) & OS_MEM_NODE_ALIGNED_FLAG)
#define OS_MEM_NODE_SET_ALIGNED_FLAG(sizeAndFlag) \
            (sizeAndFlag) = ((sizeAndFlag) | OS_MEM_NODE_ALIGNED_FLAG)
#define OS_MEM_NODE_GET_USED_FLAG(sizeAndFlag) \
            ((sizeAndFlag) & OS_MEM_NODE_USED_FLAG)
#define OS_MEM_NODE_SET_USED_FLAG(sizeAndFlag) \
            (sizeAndFlag) = ((sizeAndFlag) | OS_MEM_NODE_USED_FLAG)
#define OS_MEM_NODE_GET_SIZE(sizeAndFlag) \
            ((sizeAndFlag) & ~OS_MEM_NODE_ALIGNED_AND_USED_FLAG)

#define OS_MEM_GAPSIZE_USED_FLAG      0x80000000U
#define OS_MEM_GAPSIZE_ALIGNED_FLAG   0x40000000U
#define OS_MEM_GET_ALIGNED_GAPSIZE(gapsize) \
            ((gapsize) & ~OS_MEM_GAPSIZE_ALIGNED_FLAG)
#define OS_MEM_GET_GAPSIZE_ALIGNED_FLAG(gapsize) \
                ((gapsize) & OS_MEM_GAPSIZE_ALIGNED_FLAG)
#define OS_MEM_SET_GAPSIZE_ALIGNED_FLAG(gapsize) \
                (gapsize) = ((gapsize) | OS_MEM_GAPSIZE_ALIGNED_FLAG)
#define OS_MEM_GET_GAPSIZE_USED_FLAG(gapsize) \
                ((gapsize) & OS_MEM_GAPSIZE_USED_FLAG)
#define OS_MEM_GAPSIZE_CHECK(gapsize) \
                (OS_MEM_GET_GAPSIZE_ALIGNED_FLAG(gapsize) && \
                 OS_MEM_GET_GAPSIZE_USED_FLAG(gapsize))

#define OS_MEM_NODE_SET_LAST_FLAG(sizeAndFlag) \
            (sizeAndFlag) = ((sizeAndFlag) | OS_MEM_NODE_LAST_FLAG)
#define OS_MEM_NODE_GET_LAST_FLAG(sizeAndFlag) \
            ((sizeAndFlag) & OS_MEM_NODE_LAST_FLAG)
#define OS_MEM_NODE_GET_LEAK_FLAG(sizeAndFlag) \
            ((sizeAndFlag) & OS_MEM_NODE_LEAK_FLAG)
#define OS_MEM_NODE_SET_LEAK_FLAG(sizeAndFlag) \
            (sizeAndFlag) = ((sizeAndFlag) | OS_MEM_NODE_LEAK_FLAG)

#define OS_MEM_ALIGN_SIZE           sizeof(UINTPTR)
#define OS_MEM_IS_POW_TWO(value)    ((((UINTPTR)(value)) & ((UINTPTR)(value) - 1)) == 0)
#define OS_MEM_ALIGN(p, alignSize)  (((UINTPTR)(p) + (alignSize) - 1) & ~((UINTPTR)((alignSize) - 1)))
#define OS_MEM_IS_ALIGNED(a, b)     (!(((UINTPTR)(a)) & (((UINTPTR)(b)) - 1)))
#define OS_MEM_NODE_HEAD_SIZE       sizeof(struct OsMemUsedNodeHead)
#define OS_MEM_MIN_POOL_SIZE        (OS_MEM_NODE_HEAD_SIZE + sizeof(struct OsMemPoolHead))
#define OS_MEM_MIN_LEFT_SIZE        sizeof(struct OsMemFreeNodeHead)
#define OS_MEM_MIN_ALLOC_SIZE       8
#define OS_MEM_NEXT_NODE(node) \
    ((struct OsMemNodeHead *)(VOID *)((UINT8 *)(node) + OS_MEM_NODE_GET_SIZE((node)->sizeAndFlag)))
#define OS_MEM_FIRST_NODE(pool) \
    (struct OsMemNodeHead *)((UINT8 *)(pool) + sizeof(struct OsMemPoolHead))
#define OS_MEM_END_NODE(pool, size) \
    (struct OsMemNodeHead *)((UINT8 *)(pool) + (size) - OS_MEM_NODE_HEAD_SIZE)
#define OS_MEM_MIDDLE_ADDR_OPEN_END(startAddr, middleAddr, endAddr) \
    (((UINT8 *)(startAddr) <= (UINT8 *)(middleAddr)) && ((UINT8 *)(middleAddr) < (UINT8 *)(endAddr)))
#define OS_MEM_MIDDLE_ADDR(startAddr, middleAddr, endAddr) \
    (((UINT8 *)(startAddr) <= (UINT8 *)(middleAddr)) && ((UINT8 *)(middleAddr) <= (UINT8 *)(endAddr)))
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
STATIC INLINE UINT32 OsMemAllocCheck(struct OsMemPoolHead *pool, UINT32 intSave);
#define OS_MEM_SET_MAGIC(node)      ((node)->magic = OS_MEM_NODE_MAGIC)
#define OS_MEM_MAGIC_VALID(node)    ((node)->magic == OS_MEM_NODE_MAGIC)
#else
#define OS_MEM_SET_MAGIC(node)
#define OS_MEM_MAGIC_VALID(node)    TRUE
#endif

#if (LOSCFG_MEM_MUL_REGIONS == 1)
/**
 *  When LOSCFG_MEM_MUL_REGIONS is enabled to support multiple non-continuous memory regions,
 *  the gap between two memory regions is marked as a used OsMemNodeHead node. The gap node
 *  couldn't be freed, and would also be skipped in some DFX functions. The 'ptr.prev' pointer
 *  of this node is set to OS_MEM_GAP_NODE_MAGIC to identify that this is a gap node.
*/
#define OS_MEM_GAP_NODE_MAGIC       0xDCBAABCD
#define OS_MEM_MARK_GAP_NODE(node)  \
    (((struct OsMemNodeHead *)(node))->ptr.prev = (struct OsMemNodeHead *)OS_MEM_GAP_NODE_MAGIC)
#define OS_MEM_IS_GAP_NODE(node)    \
    (((struct OsMemNodeHead *)(node))->ptr.prev == (struct OsMemNodeHead *)OS_MEM_GAP_NODE_MAGIC)
#else
#define OS_MEM_MARK_GAP_NODE(node)
#define OS_MEM_IS_GAP_NODE(node)    FALSE
#endif

STATIC INLINE VOID OsMemFreeNodeAdd(VOID *pool, struct OsMemFreeNodeHead *node);
STATIC INLINE UINT32 OsMemFree(struct OsMemPoolHead *pool, struct OsMemNodeHead *node);
STATIC VOID OsMemInfoPrint(VOID *pool);

#if (LOSCFG_MEM_FREE_BY_TASKID == 1 || LOSCFG_TASK_MEM_USED == 1)
STATIC INLINE VOID OsMemNodeSetTaskID(struct OsMemUsedNodeHead *node)
{
    node->header.taskID = LOS_CurTaskIDGet();
}
#endif
STATIC VOID OsAllMemNodeDoHandle(VOID *pool, VOID (*handle)(struct OsMemNodeHead *curNode, VOID *arg), VOID *arg)
{
    struct OsMemPoolHead *poolInfo = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *tmpNode = NULL;
    struct OsMemNodeHead *endNode = NULL;
    UINT32 intSave = 0;

    if (pool == NULL) {
        PRINTK("input param is NULL\n");
        return;
    }
    if (LOS_MemIntegrityCheck(pool)) {
        PRINTK("LOS_MemIntegrityCheck error\n");
        return;
    }

    MEM_LOCK(poolInfo, intSave);
    endNode = OS_MEM_END_NODE(pool, poolInfo->info.totalSize);
    for (tmpNode = OS_MEM_FIRST_NODE(pool); tmpNode <= endNode; tmpNode = OS_MEM_NEXT_NODE(tmpNode)) {
        if (tmpNode == endNode) {
#if OS_MEM_EXPAND_ENABLE
            UINT32 size;
            if (OsMemIsLastSentinelNode(endNode) == FALSE) {
                size = OS_MEM_NODE_GET_SIZE(endNode->sizeAndFlag);
                tmpNode = OsMemSentinelNodeGet(endNode);
                endNode = OS_MEM_END_NODE(tmpNode, size);
                continue;
            }
#endif
            break;
        }
        handle(tmpNode, arg);
    }
    MEM_UNLOCK(poolInfo, intSave);
}

#if (LOSCFG_TASK_MEM_USED == 1)
STATIC VOID GetTaskMemUsedHandle(struct OsMemNodeHead *curNode, VOID *arg)
{
    UINT32 *args = (UINT32 *)arg;
    UINT32 *tskMemInfoBuf = (UINT32 *)(UINTPTR)*args;
    UINT32 tskMemInfoCnt = *(args + 1);
#ifndef LOSCFG_MEM_MUL_REGIONS
    if (OS_MEM_NODE_GET_USED_FLAG(curNode->sizeAndFlag)) {
#else
    if (OS_MEM_NODE_GET_USED_FLAG(curNode->sizeAndFlag) && !OS_MEM_IS_GAP_NODE(curNode)) {
#endif
        if (curNode->taskID < tskMemInfoCnt) {
            tskMemInfoBuf[curNode->taskID] += OS_MEM_NODE_GET_SIZE(curNode->sizeAndFlag);
        }
    }
    return;
}

VOID OsTaskMemUsed(VOID *pool, UINT32 *tskMemInfoBuf, UINT32 tskMemInfoCnt)
{
    UINT32 args[2] = {(UINT32)(UINTPTR)tskMemInfoBuf, tskMemInfoCnt};
    OsAllMemNodeDoHandle(pool, GetTaskMemUsedHandle, (VOID *)args);
    return;
}
#endif

#if (LOSCFG_MEM_WATERLINE == 1)
STATIC INLINE VOID OsMemWaterUsedRecord(struct OsMemPoolHead *pool, UINT32 size)
{
    pool->info.curUsedSize += size;
    if (pool->info.curUsedSize > pool->info.waterLine) {
        pool->info.waterLine = pool->info.curUsedSize;
    }
}
#else
STATIC INLINE VOID OsMemWaterUsedRecord(struct OsMemPoolHead *pool, UINT32 size)
{
    (VOID)pool;
    (VOID)size;
}
#endif

#if OS_MEM_EXPAND_ENABLE
STATIC INLINE struct OsMemNodeHead *OsMemLastSentinelNodeGet(const struct OsMemNodeHead *sentinelNode)
{
    struct OsMemNodeHead *node = NULL;
    VOID *ptr = sentinelNode->ptr.next;
    UINT32 size = OS_MEM_NODE_GET_SIZE(sentinelNode->sizeAndFlag);

    while ((ptr != NULL) && (size != 0)) {
        node = OS_MEM_END_NODE(ptr, size);
        ptr = node->ptr.next;
        size = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
    }

    return node;
}

STATIC INLINE BOOL OsMemSentinelNodeCheck(struct OsMemNodeHead *sentinelNode)
{
    if (!OS_MEM_NODE_GET_USED_FLAG(sentinelNode->sizeAndFlag)) {
        return FALSE;
    }

    if (!OS_MEM_MAGIC_VALID(sentinelNode)) {
        return FALSE;
    }

    return TRUE;
}

STATIC INLINE BOOL OsMemIsLastSentinelNode(struct OsMemNodeHead *sentinelNode)
{
    if (OsMemSentinelNodeCheck(sentinelNode) == FALSE) {
        PRINT_ERR("%s %d, The current sentinel node is invalid\n", __FUNCTION__, __LINE__);
        return TRUE;
    }

    if ((OS_MEM_NODE_GET_SIZE(sentinelNode->sizeAndFlag) == 0) ||
        (sentinelNode->ptr.next == NULL)) {
        return TRUE;
    }

    return FALSE;
}

STATIC INLINE VOID OsMemSentinelNodeSet(struct OsMemNodeHead *sentinelNode, VOID *newNode, UINT32 size)
{
    if (sentinelNode->ptr.next != NULL) {
        sentinelNode = OsMemLastSentinelNodeGet(sentinelNode);
    }

    sentinelNode->sizeAndFlag = size;
    sentinelNode->ptr.next = newNode;
    OS_MEM_NODE_SET_USED_FLAG(sentinelNode->sizeAndFlag);
    OS_MEM_NODE_SET_LAST_FLAG(sentinelNode->sizeAndFlag);
}

STATIC INLINE VOID *OsMemSentinelNodeGet(struct OsMemNodeHead *node)
{
    if (OsMemSentinelNodeCheck(node) == FALSE) {
        return NULL;
    }

    return node->ptr.next;
}

STATIC INLINE struct OsMemNodeHead *PreSentinelNodeGet(const VOID *pool, const struct OsMemNodeHead *node)
{
    UINT32 nextSize;
    struct OsMemNodeHead *nextNode = NULL;
    struct OsMemNodeHead *sentinelNode = NULL;

    sentinelNode = OS_MEM_END_NODE(pool, ((struct OsMemPoolHead *)pool)->info.totalSize);
    while (sentinelNode != NULL) {
        if (OsMemIsLastSentinelNode(sentinelNode)) {
            PRINT_ERR("PreSentinelNodeGet can not find node 0x%x\n", node);
            return NULL;
        }
        nextNode = OsMemSentinelNodeGet(sentinelNode);
        if (nextNode == node) {
            return sentinelNode;
        }
        nextSize = OS_MEM_NODE_GET_SIZE(sentinelNode->sizeAndFlag);
        sentinelNode = OS_MEM_END_NODE(nextNode, nextSize);
    }

    return NULL;
}

STATIC INLINE BOOL TryShrinkPool(const VOID *pool, const struct OsMemNodeHead *node)
{
    struct OsMemNodeHead *mySentinel = NULL;
    struct OsMemNodeHead *preSentinel = NULL;
    size_t totalSize = (UINTPTR)node->ptr.prev - (UINTPTR)node;
    size_t nodeSize = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);

    if (nodeSize != totalSize) {
        return FALSE;
    }

    preSentinel = PreSentinelNodeGet(pool, node);
    if (preSentinel == NULL) {
        return FALSE;
    }

    mySentinel = node->ptr.prev;
    if (OsMemIsLastSentinelNode(mySentinel)) { /* prev node becomes sentinel node */
        preSentinel->ptr.next = NULL;
        OsMemSentinelNodeSet(preSentinel, NULL, 0);
    } else {
        preSentinel->sizeAndFlag = mySentinel->sizeAndFlag;
        preSentinel->ptr.next = mySentinel->ptr.next;
    }

    if (OsMemLargeNodeFree(node) != LOS_OK) {
        PRINT_ERR("TryShrinkPool free 0x%x failed!\n", node);
        return FALSE;
    }

    return TRUE;
}

STATIC INLINE INT32 OsMemPoolExpand(VOID *pool, UINT32 size, UINT32 intSave)
{
    UINT32 tryCount = MAX_SHRINK_PAGECACHE_TRY;
    struct OsMemPoolHead *poolInfo = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *newNode = NULL;
    struct OsMemNodeHead *endNode = NULL;

    size = ROUNDUP(size + OS_MEM_NODE_HEAD_SIZE, PAGE_SIZE);
    endNode = OS_MEM_END_NODE(pool, poolInfo->info.totalSize);

RETRY:
    newNode = (struct OsMemNodeHead *)LOS_PhysPagesAllocContiguous(size >> PAGE_SHIFT);
    if (newNode == NULL) {
        if (tryCount > 0) {
            tryCount--;
            MEM_UNLOCK(poolInfo, intSave);
            OsTryShrinkMemory(size >> PAGE_SHIFT);
            MEM_LOCK(poolInfo, intSave);
            goto RETRY;
        }

        PRINT_ERR("OsMemPoolExpand alloc failed size = %u\n", size);
        return -1;
    }
    newNode->sizeAndFlag = (size - OS_MEM_NODE_HEAD_SIZE);
    newNode->ptr.prev = OS_MEM_END_NODE(newNode, size);
    OsMemSentinelNodeSet(endNode, newNode, size);
    OsMemFreeNodeAdd(pool, (struct OsMemFreeNodeHead *)newNode);

    endNode = OS_MEM_END_NODE(newNode, size);
    (VOID)memset(endNode, 0, sizeof(*endNode));
    endNode->ptr.next = NULL;
    OS_MEM_SET_MAGIC(endNode);
    OsMemSentinelNodeSet(endNode, NULL, 0);
    OsMemWaterUsedRecord(poolInfo, OS_MEM_NODE_HEAD_SIZE);

    return 0;
}

VOID LOS_MemExpandEnable(VOID *pool)
{
    if (pool == NULL) {
        return;
    }

    ((struct OsMemPoolHead *)pool)->info.attr |= OS_MEM_POOL_EXPAND_ENABLE;
}
#endif

#ifdef LOSCFG_KERNEL_LMS
STATIC INLINE VOID OsLmsFirstNodeMark(VOID *pool, struct OsMemNodeHead *node)
{
    if (g_lms == NULL) {
        return;
    }

    g_lms->simpleMark((UINTPTR)pool, (UINTPTR)node, LMS_SHADOW_PAINT_U8);
    g_lms->simpleMark((UINTPTR)node, (UINTPTR)node + OS_MEM_NODE_HEAD_SIZE, LMS_SHADOW_REDZONE_U8);
    g_lms->simpleMark((UINTPTR)OS_MEM_NEXT_NODE(node), (UINTPTR)OS_MEM_NEXT_NODE(node) + OS_MEM_NODE_HEAD_SIZE,
        LMS_SHADOW_REDZONE_U8);
    g_lms->simpleMark((UINTPTR)node + OS_MEM_NODE_HEAD_SIZE, (UINTPTR)OS_MEM_NEXT_NODE(node),
        LMS_SHADOW_AFTERFREE_U8);
}

STATIC INLINE VOID OsLmsAllocAlignMark(VOID *ptr, VOID *alignedPtr, UINT32 size)
{
    struct OsMemNodeHead *allocNode = NULL;

    if ((g_lms == NULL) || (ptr == NULL)) {
        return;
    }
    allocNode = (struct OsMemNodeHead *)((struct OsMemUsedNodeHead *)ptr - 1);
    if (ptr != alignedPtr) {
        g_lms->simpleMark((UINTPTR)ptr, (UINTPTR)ptr + sizeof(UINT32), LMS_SHADOW_PAINT_U8);
        g_lms->simpleMark((UINTPTR)ptr + sizeof(UINT32), (UINTPTR)alignedPtr, LMS_SHADOW_REDZONE_U8);
    }

    /* mark remining as redzone */
    g_lms->simpleMark(LMS_ADDR_ALIGN((UINTPTR)alignedPtr + size), (UINTPTR)OS_MEM_NEXT_NODE(allocNode),
        LMS_SHADOW_REDZONE_U8);
}

STATIC INLINE VOID OsLmsReallocMergeNodeMark(struct OsMemNodeHead *node)
{
    if (g_lms == NULL) {
        return;
    }

    g_lms->simpleMark((UINTPTR)node + OS_MEM_NODE_HEAD_SIZE, (UINTPTR)OS_MEM_NEXT_NODE(node),
        LMS_SHADOW_ACCESSIBLE_U8);
}

STATIC INLINE VOID OsLmsReallocSplitNodeMark(struct OsMemNodeHead *node)
{
    if (g_lms == NULL) {
        return;
    }
    /* mark next node */
    g_lms->simpleMark((UINTPTR)OS_MEM_NEXT_NODE(node),
        (UINTPTR)OS_MEM_NEXT_NODE(node) + OS_MEM_NODE_HEAD_SIZE, LMS_SHADOW_REDZONE_U8);
    g_lms->simpleMark((UINTPTR)OS_MEM_NEXT_NODE(node) + OS_MEM_NODE_HEAD_SIZE,
        (UINTPTR)OS_MEM_NEXT_NODE(OS_MEM_NEXT_NODE(node)), LMS_SHADOW_AFTERFREE_U8);
}

STATIC INLINE VOID OsLmsReallocResizeMark(struct OsMemNodeHead *node, UINT32 resize)
{
    if (g_lms == NULL) {
        return;
    }
    /* mark remaining as redzone */
    g_lms->simpleMark((UINTPTR)node + resize, (UINTPTR)OS_MEM_NEXT_NODE(node), LMS_SHADOW_REDZONE_U8);
}
#endif

#if (LOSCFG_MEM_LEAKCHECK == 1)
struct OsMemLeakCheckInfo {
    struct OsMemNodeHead *node;
    UINTPTR linkReg[LOSCFG_MEM_RECORD_LR_CNT];
};

struct OsMemLeakCheckInfo g_leakCheckRecord[LOSCFG_MEM_LEAKCHECK_RECORD_MAX_NUM] = {0};
STATIC UINT32 g_leakCheckRecordCnt = 0;

STATIC INLINE VOID OsMemLeakCheckInfoRecord(struct OsMemNodeHead *node)
{
    struct OsMemLeakCheckInfo *info = &g_leakCheckRecord[g_leakCheckRecordCnt];

    if (!OS_MEM_NODE_GET_LEAK_FLAG(node->sizeAndFlag)) {
        info->node = node;
        (VOID)memcpy(info->linkReg, node->linkReg, sizeof(node->linkReg));
        OS_MEM_NODE_SET_LEAK_FLAG(node->sizeAndFlag);
        g_leakCheckRecordCnt++;
        if (g_leakCheckRecordCnt >= LOSCFG_MEM_LEAKCHECK_RECORD_MAX_NUM) {
            g_leakCheckRecordCnt = 0;
        }
    }
}

STATIC INLINE VOID OsMemLeakCheckInit(VOID)
{
    (VOID)memset(g_leakCheckRecord, 0, sizeof(struct OsMemLeakCheckInfo) * LOSCFG_MEM_LEAKCHECK_RECORD_MAX_NUM);
    g_leakCheckRecordCnt = 0;
}

STATIC INLINE VOID OsMemLinkRegisterRecord(struct OsMemNodeHead *node)
{
    (VOID)memset(node->linkReg, 0, sizeof(node->linkReg));
    OsBackTraceHookCall(node->linkReg, LOSCFG_MEM_RECORD_LR_CNT, LOSCFG_MEM_OMIT_LR_CNT, 0);
}

STATIC INLINE VOID OsMemUsedNodePrint(struct OsMemNodeHead *node)
{
    UINT32 count;

    if (OS_MEM_NODE_GET_USED_FLAG(node->sizeAndFlag) && !OS_MEM_IS_GAP_NODE(node)) {
        PRINTK("0x%x: 0x%x ", (UINTPTR)node, OS_MEM_NODE_GET_SIZE(node->sizeAndFlag));
        for (count = 0; count < LOSCFG_MEM_RECORD_LR_CNT; count++) {
            PRINTK(" 0x%x ", node->linkReg[count]);
        }
        PRINTK("\n");

        OsMemLeakCheckInfoRecord(node);
    }
}

STATIC VOID OsMemUsedNodePrintHandle(struct OsMemNodeHead *node, VOID *arg)
{
    UNUSED(arg);
    OsMemUsedNodePrint(node);
    return;
}

VOID LOS_MemUsedNodeShow(VOID *pool)
{
    UINT32 count;

    PRINTK("\n\rnode          size    ");
    for (count = 0; count < LOSCFG_MEM_RECORD_LR_CNT; count++) {
        PRINTK("    LR[%u]   ", count);
    }
    PRINTK("\n");

    OsMemLeakCheckInit();
    OsAllMemNodeDoHandle(pool, OsMemUsedNodePrintHandle, NULL);
    return;
}

#if (LOSCFG_KERNEL_PRINTF != 0)
STATIC VOID OsMemNodeBacktraceInfo(const struct OsMemNodeHead *tmpNode,
                                   const struct OsMemNodeHead *preNode)
{
    int i;
    PRINTK("\n broken node head LR info: \n");
    for (i = 0; i < LOSCFG_MEM_RECORD_LR_CNT; i++) {
        PRINTK(" LR[%d]:0x%x\n", i, tmpNode->linkReg[i]);
    }

    PRINTK("\n pre node head LR info: \n");
    for (i = 0; i < LOSCFG_MEM_RECORD_LR_CNT; i++) {
        PRINTK(" LR[%d]:0x%x\n", i, preNode->linkReg[i]);
    }
}
#endif
#endif

STATIC INLINE UINT32 OsMemFreeListIndexGet(UINT32 size)
{
    UINT32 fl = OsMemFlGet(size);
    if (fl < OS_MEM_SMALL_BUCKET_COUNT) {
        return fl;
    }

    UINT32 sl = OsMemSlGet(size, fl);
    return (OS_MEM_SMALL_BUCKET_COUNT + ((fl - OS_MEM_SMALL_BUCKET_COUNT) << OS_MEM_SLI) + sl);
}

STATIC INLINE struct OsMemFreeNodeHead *OsMemFindCurSuitableBlock(struct OsMemPoolHead *poolHead,
                                        UINT32 index, UINT32 size)
{
    struct OsMemFreeNodeHead *node = NULL;

    for (node = poolHead->freeList[index]; node != NULL; node = node->next) {
        if (node->header.sizeAndFlag >= size) {
            return node;
        }
    }

    return NULL;
}

STATIC INLINE UINT32 OsMemNotEmptyIndexGet(struct OsMemPoolHead *poolHead, UINT32 index)
{
    /* 5: Divide by 32 to calculate the index of the bitmap array. */
    UINT32 mask = poolHead->freeListBitmap[index >> 5];
    mask &= ~((1 << (index & OS_MEM_BITMAP_MASK)) - 1);
    if (mask != 0) {
        index = OsMemFFS(mask) + (index & ~OS_MEM_BITMAP_MASK);
        return index;
    }

    return OS_MEM_FREE_LIST_COUNT;
}

STATIC INLINE struct OsMemFreeNodeHead *OsMemFindNextSuitableBlock(VOID *pool, UINT32 size, UINT32 *outIndex)
{
    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    UINT32 fl = OsMemFlGet(size);
    UINT32 sl;
    UINT32 index, tmp;
    UINT32 curIndex = OS_MEM_FREE_LIST_COUNT;
    UINT32 mask;

    do {
        if (fl < OS_MEM_SMALL_BUCKET_COUNT) {
            index = fl;
        } else {
            sl = OsMemSlGet(size, fl);
            curIndex = ((fl - OS_MEM_SMALL_BUCKET_COUNT) << OS_MEM_SLI) + sl + OS_MEM_SMALL_BUCKET_COUNT;
            index = curIndex + 1;
        }

        tmp = OsMemNotEmptyIndexGet(poolHead, index);
        if (tmp != OS_MEM_FREE_LIST_COUNT) {
            index = tmp;
            goto DONE;
        }

        for (index = LOS_Align(index + 1, 32); index < OS_MEM_FREE_LIST_COUNT; index += 32) {
            /* 5: Divide by 32 to calculate the index of the bitmap array. */
            mask = poolHead->freeListBitmap[index >> 5];
            if (mask != 0) {
                index = OsMemFFS(mask) + index;
                goto DONE;
            }
        }
    } while (0);

    if (curIndex == OS_MEM_FREE_LIST_COUNT) {
        return NULL;
    }

    *outIndex = curIndex;
    return OsMemFindCurSuitableBlock(poolHead, curIndex, size);
DONE:
    *outIndex = index;
    return poolHead->freeList[index];
}

STATIC INLINE VOID OsMemSetFreeListBit(struct OsMemPoolHead *head, UINT32 index)
{
    /* 5: Divide by 32 to calculate the index of the bitmap array. */
    head->freeListBitmap[index >> 5] |= 1U << (index & 0x1f);
}

STATIC INLINE VOID OsMemClearFreeListBit(struct OsMemPoolHead *head, UINT32 index)
{
    /* 5: Divide by 32 to calculate the index of the bitmap array. */
    head->freeListBitmap[index >> 5] &= ~(1U << (index & 0x1f));
}

STATIC INLINE VOID OsMemListAdd(struct OsMemPoolHead *pool, UINT32 listIndex, struct OsMemFreeNodeHead *node)
{
    struct OsMemFreeNodeHead *firstNode = pool->freeList[listIndex];
    if (firstNode != NULL) {
        firstNode->prev = node;
    }
    node->prev = NULL;
    node->next = firstNode;
    pool->freeList[listIndex] = node;
    OsMemSetFreeListBit(pool, listIndex);
    OS_MEM_SET_MAGIC(&node->header);
}

STATIC INLINE VOID OsMemListDelete(struct OsMemPoolHead *pool, UINT32 listIndex, struct OsMemFreeNodeHead *node)
{
    if (node == pool->freeList[listIndex]) {
        pool->freeList[listIndex] = node->next;
        if (node->next == NULL) {
            OsMemClearFreeListBit(pool, listIndex);
        } else {
            node->next->prev = NULL;
        }
    } else {
        node->prev->next = node->next;
        if (node->next != NULL) {
            node->next->prev = node->prev;
        }
    }
    OS_MEM_SET_MAGIC(&node->header);
}

STATIC INLINE VOID OsMemFreeNodeAdd(VOID *pool, struct OsMemFreeNodeHead *node)
{
    UINT32 index = OsMemFreeListIndexGet(node->header.sizeAndFlag);
    if (index >= OS_MEM_FREE_LIST_COUNT) {
        LOS_Panic("The index of free lists is error, index = %u\n", index);
    }
    OsMemListAdd(pool, index, node);
}

STATIC INLINE VOID OsMemFreeNodeDelete(VOID *pool, struct OsMemFreeNodeHead *node)
{
    UINT32 index = OsMemFreeListIndexGet(node->header.sizeAndFlag);
    OsMemListDelete(pool, index, node);
}

STATIC INLINE struct OsMemNodeHead *OsMemFreeNodeGet(VOID *pool, UINT32 size)
{
    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    UINT32 index;
    struct OsMemFreeNodeHead *firstNode = OsMemFindNextSuitableBlock(pool, size, &index);
    if (firstNode == NULL) {
        return NULL;
    }

    OsMemListDelete(poolHead, index, firstNode);

    return &firstNode->header;
}

STATIC INLINE VOID OsMemMergeNode(struct OsMemNodeHead *node)
{
    struct OsMemNodeHead *nextNode = NULL;

    node->ptr.prev->sizeAndFlag += node->sizeAndFlag;
    nextNode = (struct OsMemNodeHead *)((UINTPTR)node + node->sizeAndFlag);
    if (!OS_MEM_NODE_GET_LAST_FLAG(nextNode->sizeAndFlag) && !OS_MEM_IS_GAP_NODE(nextNode)) {
        nextNode->ptr.prev = node->ptr.prev;
    }
}

STATIC INLINE VOID OsMemSplitNode(VOID *pool, struct OsMemNodeHead *allocNode, UINT32 allocSize)
{
    struct OsMemFreeNodeHead *newFreeNode = NULL;
    struct OsMemNodeHead *nextNode = NULL;

    newFreeNode = (struct OsMemFreeNodeHead *)(VOID *)((UINT8 *)allocNode + allocSize);
    newFreeNode->header.ptr.prev = allocNode;
    newFreeNode->header.sizeAndFlag = allocNode->sizeAndFlag - allocSize;
    allocNode->sizeAndFlag = allocSize;
    nextNode = OS_MEM_NEXT_NODE(&newFreeNode->header);
    if (!OS_MEM_NODE_GET_LAST_FLAG(nextNode->sizeAndFlag) && !OS_MEM_IS_GAP_NODE(nextNode)) {
        nextNode->ptr.prev = &newFreeNode->header;
        if (!OS_MEM_NODE_GET_USED_FLAG(nextNode->sizeAndFlag)) {
            OsMemFreeNodeDelete(pool, (struct OsMemFreeNodeHead *)nextNode);
            OsMemMergeNode(nextNode);
        }
    }

    OsMemFreeNodeAdd(pool, newFreeNode);
}

STATIC INLINE VOID *OsMemCreateUsedNode(VOID *addr)
{
    struct OsMemUsedNodeHead *node = (struct OsMemUsedNodeHead *)addr;

#if (LOSCFG_MEM_FREE_BY_TASKID == 1 || LOSCFG_TASK_MEM_USED == 1)
    OsMemNodeSetTaskID(node);
#endif

#ifdef LOSCFG_KERNEL_LMS
    struct OsMemNodeHead *newNode = (struct OsMemNodeHead *)node;
    if (g_lms != NULL) {
        g_lms->mallocMark(newNode, OS_MEM_NEXT_NODE(newNode), OS_MEM_NODE_HEAD_SIZE);
    }
#endif
    return node + 1;
}

STATIC UINT32 OsMemPoolInit(VOID *pool, UINT32 size)
{
    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *newNode = NULL;
    struct OsMemNodeHead *endNode = NULL;
#ifdef LOSCFG_KERNEL_LMS
    UINT32 resize = 0;
    if (g_lms != NULL) {
        /*
         * resize == 0, shadow memory init failed, no shadow memory for this pool, set poolSize as original size.
         * resize != 0, shadow memory init successful, set poolSize as resize.
         */
        resize = g_lms->init(pool, size);
        size = (resize == 0) ? size : resize;
    }
#endif
    (VOID)memset(poolHead, 0, sizeof(struct OsMemPoolHead));

    poolHead->info.pool = pool;
    poolHead->info.totalSize = size;
    /* default attr: lock, not expand. */
    poolHead->info.attr &= ~(OS_MEM_POOL_UNLOCK_ENABLE | OS_MEM_POOL_EXPAND_ENABLE);

    newNode = OS_MEM_FIRST_NODE(pool);
    newNode->sizeAndFlag = (size - sizeof(struct OsMemPoolHead) - OS_MEM_NODE_HEAD_SIZE);
    newNode->ptr.prev = OS_MEM_END_NODE(pool, size);
    OS_MEM_SET_MAGIC(newNode);
    OsMemFreeNodeAdd(pool, (struct OsMemFreeNodeHead *)newNode);

    /* The last mem node */
    endNode = OS_MEM_END_NODE(pool, size);
    OS_MEM_SET_MAGIC(endNode);
#if OS_MEM_EXPAND_ENABLE
    endNode->ptr.next = NULL;
    OsMemSentinelNodeSet(endNode, NULL, 0);
#else
    endNode->sizeAndFlag = 0;
    endNode->ptr.prev = newNode;
    OS_MEM_NODE_SET_USED_FLAG(endNode->sizeAndFlag);
#endif
#if (LOSCFG_MEM_WATERLINE == 1)
    poolHead->info.curUsedSize = sizeof(struct OsMemPoolHead) + OS_MEM_NODE_HEAD_SIZE;
    poolHead->info.waterLine = poolHead->info.curUsedSize;
#endif

#ifdef LOSCFG_KERNEL_LMS
    if (resize != 0) {
        OsLmsFirstNodeMark(pool, newNode);
    }
#endif
    return LOS_OK;
}

#if (LOSCFG_MEM_MUL_POOL == 1)
STATIC VOID OsMemPoolDeinit(VOID *pool)
{
    (VOID)memset(pool, 0, sizeof(struct OsMemPoolHead));
}

STATIC UINT32 OsMemPoolAdd(VOID *pool, UINT32 size)
{
    VOID *nextPool = g_poolHead;
    VOID *curPool = g_poolHead;
    UINTPTR poolEnd;
    while (nextPool != NULL) {
        poolEnd = (UINTPTR)nextPool + LOS_MemPoolSizeGet(nextPool);
        if (((pool <= nextPool) && (((UINTPTR)pool + size) > (UINTPTR)nextPool)) ||
            (((UINTPTR)pool < poolEnd) && (((UINTPTR)pool + size) >= poolEnd))) {
            PRINT_ERR("pool [0x%x, 0x%x) conflict with pool [0x%x, 0x%x)\n", (UINTPTR)pool,
                      (UINTPTR)pool + size, (UINTPTR)nextPool, (UINTPTR)nextPool + LOS_MemPoolSizeGet(nextPool));
            return LOS_NOK;
        }
        curPool = nextPool;
        nextPool = ((struct OsMemPoolHead *)nextPool)->nextPool;
    }

    if (g_poolHead == NULL) {
        g_poolHead = pool;
    } else {
        ((struct OsMemPoolHead *)curPool)->nextPool = pool;
    }

    ((struct OsMemPoolHead *)pool)->nextPool = NULL;
    return LOS_OK;
}

STATIC UINT32 OsMemPoolDelete(VOID *pool)
{
    UINT32 ret = LOS_NOK;
    VOID *nextPool = NULL;
    VOID *curPool = NULL;

    do {
        if (pool == g_poolHead) {
            g_poolHead = ((struct OsMemPoolHead *)g_poolHead)->nextPool;
            ret = LOS_OK;
            break;
        }

        curPool = g_poolHead;
        nextPool = g_poolHead;
        while (nextPool != NULL) {
            if (pool == nextPool) {
                ((struct OsMemPoolHead *)curPool)->nextPool = ((struct OsMemPoolHead *)nextPool)->nextPool;
                ret = LOS_OK;
                break;
            }
            curPool = nextPool;
            nextPool = ((struct OsMemPoolHead *)nextPool)->nextPool;
        }
    } while (0);

    return ret;
}
#endif

UINT32 LOS_MemInit(VOID *pool, UINT32 size)
{
    if ((pool == NULL) || (size <= OS_MEM_MIN_POOL_SIZE)) {
        return LOS_NOK;
    }

    if (((UINTPTR)pool & (OS_MEM_ALIGN_SIZE - 1)) || \
        (size & (OS_MEM_ALIGN_SIZE - 1))) {
        PRINT_ERR("LiteOS heap memory address or size configured not aligned:address:0x%x,size:0x%x, alignsize:%d\n", \
                  (UINTPTR)pool, size, OS_MEM_ALIGN_SIZE);
        return LOS_NOK;
    }

    if (OsMemPoolInit(pool, size)) {
        return LOS_NOK;
    }

#if (LOSCFG_MEM_MUL_POOL == 1)
    if (OsMemPoolAdd(pool, size)) {
        (VOID)OsMemPoolDeinit(pool);
        return LOS_NOK;
    }
#endif

    OsHookCall(LOS_HOOK_TYPE_MEM_INIT, pool, size);

    return LOS_OK;
}

#if (LOSCFG_MEM_MUL_POOL == 1)
UINT32 LOS_MemDeInit(VOID *pool)
{
    if (pool == NULL) {
        return LOS_NOK;
    }

    if (OsMemPoolDelete(pool)) {
        return LOS_NOK;
    }

    OsMemPoolDeinit(pool);

    OsHookCall(LOS_HOOK_TYPE_MEM_DEINIT, pool);

    return LOS_OK;
}

UINT32 LOS_MemPoolList(VOID)
{
    VOID *nextPool = g_poolHead;
    UINT32 index = 0;
    while (nextPool != NULL) {
        PRINTK("pool%u :\n", index);
        index++;
        OsMemInfoPrint(nextPool);
        nextPool = ((struct OsMemPoolHead *)nextPool)->nextPool;
    }
    return index;
}
#endif

STATIC INLINE VOID *OsMemAlloc(struct OsMemPoolHead *pool, UINT32 size, UINT32 intSave)
{
    struct OsMemNodeHead *allocNode = NULL;

#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
    if (OsMemAllocCheck(pool, intSave) == LOS_NOK) {
        return NULL;
    }
#endif

    UINT32 allocSize = OS_MEM_ALIGN(size + OS_MEM_NODE_HEAD_SIZE, OS_MEM_ALIGN_SIZE);
#if OS_MEM_EXPAND_ENABLE || (LOSCFG_KERNEL_LMK == 1)
retry:
#endif
    allocNode = OsMemFreeNodeGet(pool, allocSize);
    if (allocNode == NULL) {
#if OS_MEM_EXPAND_ENABLE
        if (pool->info.attr & OS_MEM_POOL_EXPAND_ENABLE) {
            INT32 ret = OsMemPoolExpand(pool, allocSize, intSave);
            if (ret == 0) {
                goto retry;
            }
        }
#endif

#if (LOSCFG_KERNEL_LMK == 1)
        UINT32 killRet = LOS_LmkTasksKill();
        if (killRet == LOS_OK) {
            goto retry;
        }
#endif
        PRINT_ERR("---------------------------------------------------"
                  "--------------------------------------------------------\n");
        MEM_UNLOCK(pool, intSave);
        OsMemInfoPrint(pool);
        MEM_LOCK(pool, intSave);
        PRINT_ERR("[%s] No suitable free block, require free node size: 0x%x\n", __FUNCTION__, allocSize);
        PRINT_ERR("----------------------------------------------------"
                  "-------------------------------------------------------\n");
        return NULL;
    }

    if ((allocSize + OS_MEM_MIN_LEFT_SIZE) <= allocNode->sizeAndFlag) {
        OsMemSplitNode(pool, allocNode, allocSize);
    }

    OS_MEM_NODE_SET_USED_FLAG(allocNode->sizeAndFlag);
    OsMemWaterUsedRecord(pool, OS_MEM_NODE_GET_SIZE(allocNode->sizeAndFlag));

#if (LOSCFG_MEM_LEAKCHECK == 1)
    OsMemLinkRegisterRecord(allocNode);
#endif
    return OsMemCreateUsedNode((VOID *)allocNode);
}

VOID *LOS_MemAlloc(VOID *pool, UINT32 size)
{
    if ((pool == NULL) || (size == 0)) {
        return NULL;
    }

    if (size < OS_MEM_MIN_ALLOC_SIZE) {
        size = OS_MEM_MIN_ALLOC_SIZE;
    }

    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    VOID *ptr = NULL;
    UINT32 intSave = 0;

    MEM_LOCK(poolHead, intSave);
    do {
        if (OS_MEM_NODE_GET_USED_FLAG(size) || OS_MEM_NODE_GET_ALIGNED_FLAG(size)) {
            break;
        }
        ptr = OsMemAlloc(poolHead, size, intSave);
    } while (0);
    MEM_UNLOCK(poolHead, intSave);

    OsHookCall(LOS_HOOK_TYPE_MEM_ALLOC, pool, ptr, size);

    return ptr;
}

VOID *LOS_MemAllocAlign(VOID *pool, UINT32 size, UINT32 boundary)
{
    UINT32 gapSize;

    if ((pool == NULL) || (size == 0) || (boundary == 0) || !OS_MEM_IS_POW_TWO(boundary) ||
        !OS_MEM_IS_ALIGNED(boundary, sizeof(VOID *))) {
        return NULL;
    }

    if (size < OS_MEM_MIN_ALLOC_SIZE) {
        size = OS_MEM_MIN_ALLOC_SIZE;
    }

    /*
     * sizeof(gapSize) bytes stores offset between alignedPtr and ptr,
     * the ptr has been OS_MEM_ALIGN_SIZE(4 or 8) aligned, so maximum
     * offset between alignedPtr and ptr is boundary - OS_MEM_ALIGN_SIZE
     */
    if ((boundary - sizeof(gapSize)) > ((UINT32)(-1) - size)) {
        return NULL;
    }

    UINT32 useSize = (size + boundary) - sizeof(gapSize);
    if (OS_MEM_NODE_GET_USED_FLAG(useSize) || OS_MEM_NODE_GET_ALIGNED_FLAG(useSize)) {
        return NULL;
    }

    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    UINT32 intSave = 0;
    VOID *ptr = NULL;
    VOID *alignedPtr = NULL;

    MEM_LOCK(poolHead, intSave);
    do {
        ptr = OsMemAlloc(pool, useSize, intSave);
        alignedPtr = (VOID *)OS_MEM_ALIGN(ptr, boundary);
        if (ptr == alignedPtr) {
#ifdef LOSCFG_KERNEL_LMS
            OsLmsAllocAlignMark(ptr, alignedPtr, size);
#endif
            break;
        }

        /* store gapSize in address (ptr - 4), it will be checked while free */
        gapSize = (UINT32)((UINTPTR)alignedPtr - (UINTPTR)ptr);
        struct OsMemUsedNodeHead *allocNode = (struct OsMemUsedNodeHead *)ptr - 1;
        OS_MEM_NODE_SET_ALIGNED_FLAG(allocNode->header.sizeAndFlag);
        OS_MEM_SET_GAPSIZE_ALIGNED_FLAG(gapSize);
        *(UINT32 *)((UINTPTR)alignedPtr - sizeof(gapSize)) = gapSize;
#ifdef LOSCFG_KERNEL_LMS
        OsLmsAllocAlignMark(ptr, alignedPtr, size);
#endif
        ptr = alignedPtr;
    } while (0);
    MEM_UNLOCK(poolHead, intSave);

    OsHookCall(LOS_HOOK_TYPE_MEM_ALLOCALIGN, pool, ptr, size, boundary);

    return ptr;
}

STATIC INLINE BOOL OsMemAddrValidCheck(const struct OsMemPoolHead *pool, const VOID *addr)
{
    UINT32 size;

    size = pool->info.totalSize;
    if (OS_MEM_MIDDLE_ADDR_OPEN_END(pool + 1, addr, (UINTPTR)pool + size)) {
        return TRUE;
    }
#if OS_MEM_EXPAND_ENABLE
    struct OsMemNodeHead *node = NULL;
    struct OsMemNodeHead *sentinel = OS_MEM_END_NODE(pool, size);
    while (OsMemIsLastSentinelNode(sentinel) == FALSE) {
        size = OS_MEM_NODE_GET_SIZE(sentinel->sizeAndFlag);
        node = OsMemSentinelNodeGet(sentinel);
        sentinel = OS_MEM_END_NODE(node, size);
        if (OS_MEM_MIDDLE_ADDR_OPEN_END(node, addr, (UINTPTR)node + size)) {
            return TRUE;
        }
    }
#endif
    return FALSE;
}

STATIC INLINE BOOL OsMemIsNodeValid(const struct OsMemNodeHead *node, const struct OsMemNodeHead *startNode,
                                    const struct OsMemNodeHead *endNode,
                                    const struct OsMemPoolHead *poolInfo)
{
    if (!OS_MEM_MIDDLE_ADDR(startNode, node, endNode)) {
        return FALSE;
    }

    if (OS_MEM_NODE_GET_USED_FLAG(node->sizeAndFlag)) {
        if (!OS_MEM_MAGIC_VALID(node)) {
            return FALSE;
        }
        return TRUE;
    }

    if (!OsMemAddrValidCheck(poolInfo, node->ptr.prev)) {
        return FALSE;
    }

    return TRUE;
}

STATIC UINT32 OsMemCheckUsedNode(const struct OsMemPoolHead *pool, const struct OsMemNodeHead *node)
{
    struct OsMemNodeHead *startNode = (struct OsMemNodeHead *)OS_MEM_FIRST_NODE(pool);
    struct OsMemNodeHead *endNode = (struct OsMemNodeHead *)OS_MEM_END_NODE(pool, pool->info.totalSize);
    struct OsMemNodeHead *nextNode = NULL;
    BOOL doneFlag = FALSE;

    do {
        do {
            if (OS_MEM_IS_GAP_NODE(node)) {
                break;
            }

            if (!OsMemIsNodeValid(node, startNode, endNode, pool)) {
                break;
            }

            if (!OS_MEM_NODE_GET_USED_FLAG(node->sizeAndFlag)) {
                break;
            }

            nextNode = OS_MEM_NEXT_NODE(node);
            if (!OsMemIsNodeValid(nextNode, startNode, endNode, pool)) {
                break;
            }

            if (!OS_MEM_NODE_GET_LAST_FLAG(nextNode->sizeAndFlag) && !OS_MEM_IS_GAP_NODE(nextNode)) {
                if (nextNode->ptr.prev != node) {
                    break;
                }
            }

            if ((node != startNode) &&
                ((!OsMemIsNodeValid(node->ptr.prev, startNode, endNode, pool)) ||
                (OS_MEM_NEXT_NODE(node->ptr.prev) != node))) {
                break;
            }
            doneFlag = TRUE;
        } while (0);

        if (!doneFlag) {
#if OS_MEM_EXPAND_ENABLE
            if (OsMemIsLastSentinelNode(endNode) == FALSE) {
                startNode = OsMemSentinelNodeGet(endNode);
                endNode = OS_MEM_END_NODE(startNode, OS_MEM_NODE_GET_SIZE(endNode->sizeAndFlag));
                continue;
            }
#endif
            return LOS_NOK;
        }
    } while (!doneFlag);

    return LOS_OK;
}

STATIC INLINE UINT32 OsMemFree(struct OsMemPoolHead *pool, struct OsMemNodeHead *node)
{
    UINT32 ret = OsMemCheckUsedNode(pool, node);
    if (ret != LOS_OK) {
        PRINT_ERR("OsMemFree check error!\n");
        return ret;
    }

#if (LOSCFG_MEM_WATERLINE == 1)
    pool->info.curUsedSize -= OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
#endif

    node->sizeAndFlag = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
#if (LOSCFG_MEM_LEAKCHECK == 1)
    OsMemLinkRegisterRecord(node);
#endif
#ifdef LOSCFG_KERNEL_LMS
    struct OsMemNodeHead *nextNodeBackup = OS_MEM_NEXT_NODE(node);
    struct OsMemNodeHead *curNodeBackup = node;
    if (g_lms != NULL) {
        g_lms->check((UINTPTR)node + OS_MEM_NODE_HEAD_SIZE, TRUE);
    }
#endif
    struct OsMemNodeHead *preNode = node->ptr.prev; /* merage preNode */
    if ((preNode != NULL) && !OS_MEM_NODE_GET_USED_FLAG(preNode->sizeAndFlag)) {
        OsMemFreeNodeDelete(pool, (struct OsMemFreeNodeHead *)preNode);
        OsMemMergeNode(node);
        node = preNode;
    }

    struct OsMemNodeHead *nextNode = OS_MEM_NEXT_NODE(node); /* merage nextNode */
    if ((nextNode != NULL) && !OS_MEM_NODE_GET_USED_FLAG(nextNode->sizeAndFlag)) {
        OsMemFreeNodeDelete(pool, (struct OsMemFreeNodeHead *)nextNode);
        OsMemMergeNode(nextNode);
    }

#if OS_MEM_EXPAND_ENABLE
    if (pool->info.attr & OS_MEM_POOL_EXPAND_ENABLE) {
        struct OsMemNodeHead *firstNode = OS_MEM_FIRST_NODE(pool);
        /* if this is a expand head node, and all unused, free it to pmm */
        if ((node->prev > node) && (node != firstNode)) {
            if (TryShrinkPool(pool, node)) {
                return LOS_OK;
            }
        }
    }
#endif

    OsMemFreeNodeAdd(pool, (struct OsMemFreeNodeHead *)node);
#ifdef LOSCFG_KERNEL_LMS
    if (g_lms != NULL) {
        g_lms->freeMark(curNodeBackup, nextNodeBackup, OS_MEM_NODE_HEAD_SIZE);
    }
#endif
    return ret;
}

STATIC INLINE VOID *OsGetRealPtr(const VOID *pool, VOID *ptr)
{
    VOID *realPtr = ptr;
    UINT32 gapSize = *((UINT32 *)((UINTPTR)ptr - sizeof(UINT32)));

    if (OS_MEM_GAPSIZE_CHECK(gapSize)) {
        PRINT_ERR("[%s:%d]gapSize:0x%x error\n", __FUNCTION__, __LINE__, gapSize);
        return NULL;
    }

    if (OS_MEM_GET_GAPSIZE_ALIGNED_FLAG(gapSize)) {
        gapSize = OS_MEM_GET_ALIGNED_GAPSIZE(gapSize);
        if ((gapSize & (OS_MEM_ALIGN_SIZE - 1)) ||
            (gapSize > ((UINTPTR)ptr - OS_MEM_NODE_HEAD_SIZE - (UINTPTR)pool))) {
            PRINT_ERR("[%s:%d]gapSize:0x%x error\n", __FUNCTION__, __LINE__, gapSize);
            return NULL;
        }
        realPtr = (VOID *)((UINTPTR)ptr - (UINTPTR)gapSize);
    }
    return realPtr;
}

UINT32 LOS_MemFree(VOID *pool, VOID *ptr)
{
    if ((pool == NULL) || (ptr == NULL) || !OS_MEM_IS_ALIGNED(pool, sizeof(VOID *)) ||
        !OS_MEM_IS_ALIGNED(ptr, sizeof(VOID *))) {
        return LOS_NOK;
    }

    OsHookCall(LOS_HOOK_TYPE_MEM_FREE, pool, ptr);

    UINT32 ret = LOS_NOK;
    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *node = NULL;
    UINT32 intSave = 0;

    MEM_LOCK(poolHead, intSave);
    do {
        ptr = OsGetRealPtr(pool, ptr);
        if (ptr == NULL) {
            break;
        }
        node = (struct OsMemNodeHead *)((UINTPTR)ptr - OS_MEM_NODE_HEAD_SIZE);
        ret = OsMemFree(poolHead, node);
    } while (0);
    MEM_UNLOCK(poolHead, intSave);

    return ret;
}

STATIC INLINE VOID OsMemReAllocSmaller(VOID *pool, UINT32 allocSize, struct OsMemNodeHead *node, UINT32 nodeSize)
{
#if (LOSCFG_MEM_WATERLINE == 1)
    struct OsMemPoolHead *poolInfo = (struct OsMemPoolHead *)pool;
#endif
    node->sizeAndFlag = nodeSize;
    if ((allocSize + OS_MEM_MIN_LEFT_SIZE) <= nodeSize) {
        OsMemSplitNode(pool, node, allocSize);
#if (LOSCFG_MEM_WATERLINE == 1)
        poolInfo->info.curUsedSize -= nodeSize - allocSize;
#endif
#ifdef LOSCFG_KERNEL_LMS
        OsLmsReallocSplitNodeMark(node);
    } else {
        OsLmsReallocResizeMark(node, allocSize);
#endif
    }
    OS_MEM_NODE_SET_USED_FLAG(node->sizeAndFlag);
#if (LOSCFG_MEM_LEAKCHECK == 1)
    OsMemLinkRegisterRecord(node);
#endif
}

STATIC INLINE VOID OsMemMergeNodeForReAllocBigger(VOID *pool, UINT32 allocSize, struct OsMemNodeHead *node,
                                                  UINT32 nodeSize, struct OsMemNodeHead *nextNode)
{
    node->sizeAndFlag = nodeSize;
    OsMemFreeNodeDelete(pool, (struct OsMemFreeNodeHead *)nextNode);
    OsMemMergeNode(nextNode);
#ifdef LOSCFG_KERNEL_LMS
    OsLmsReallocMergeNodeMark(node);
#endif
    if ((allocSize + OS_MEM_MIN_LEFT_SIZE) <= node->sizeAndFlag) {
        OsMemSplitNode(pool, node, allocSize);
#ifdef LOSCFG_KERNEL_LMS
        OsLmsReallocSplitNodeMark(node);
    } else {
        OsLmsReallocResizeMark(node, allocSize);
#endif
    }
    OS_MEM_NODE_SET_USED_FLAG(node->sizeAndFlag);
    OsMemWaterUsedRecord((struct OsMemPoolHead *)pool, OS_MEM_NODE_GET_SIZE(node->sizeAndFlag) - nodeSize);
#if (LOSCFG_MEM_LEAKCHECK == 1)
    OsMemLinkRegisterRecord(node);
#endif
}

STATIC INLINE VOID *OsMemRealloc(struct OsMemPoolHead *pool, const VOID *ptr,
                struct OsMemNodeHead *node, UINT32 size, UINT32 intSave)
{
    struct OsMemNodeHead *nextNode = NULL;
    UINT32 allocSize = OS_MEM_ALIGN(size + OS_MEM_NODE_HEAD_SIZE, OS_MEM_ALIGN_SIZE);
    UINT32 nodeSize = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
    VOID *tmpPtr = NULL;

    if (nodeSize >= allocSize) {
        OsMemReAllocSmaller(pool, allocSize, node, nodeSize);
        return (VOID *)ptr;
    }

    nextNode = OS_MEM_NEXT_NODE(node);
    if (!OS_MEM_NODE_GET_USED_FLAG(nextNode->sizeAndFlag) &&
        ((nextNode->sizeAndFlag + nodeSize) >= allocSize)) {
        OsMemMergeNodeForReAllocBigger(pool, allocSize, node, nodeSize, nextNode);
        return (VOID *)ptr;
    }

    tmpPtr = OsMemAlloc(pool, size, intSave);
    if (tmpPtr != NULL) {
        if (memcpy_s(tmpPtr, size, ptr, (nodeSize - OS_MEM_NODE_HEAD_SIZE)) != EOK) {
            MEM_UNLOCK(pool, intSave);
            (VOID)LOS_MemFree((VOID *)pool, (VOID *)tmpPtr);
            MEM_LOCK(pool, intSave);
            return NULL;
        }
        (VOID)OsMemFree(pool, node);
    }
    return tmpPtr;
}

VOID *LOS_MemRealloc(VOID *pool, VOID *ptr, UINT32 size)
{
    if ((pool == NULL) || OS_MEM_NODE_GET_USED_FLAG(size) || OS_MEM_NODE_GET_ALIGNED_FLAG(size)) {
        return NULL;
    }

    OsHookCall(LOS_HOOK_TYPE_MEM_REALLOC, pool, ptr, size);

    if (ptr == NULL) {
        return LOS_MemAlloc(pool, size);
    }

    if (size == 0) {
        (VOID)LOS_MemFree(pool, ptr);
        return NULL;
    }

    if (size < OS_MEM_MIN_ALLOC_SIZE) {
        size = OS_MEM_MIN_ALLOC_SIZE;
    }

    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *node = NULL;
    VOID *newPtr = NULL;
    UINT32 intSave = 0;

    MEM_LOCK(poolHead, intSave);
    do {
        ptr = OsGetRealPtr(pool, ptr);
        if (ptr == NULL) {
            break;
        }

        node = (struct OsMemNodeHead *)((UINTPTR)ptr - OS_MEM_NODE_HEAD_SIZE);
        if (OsMemCheckUsedNode(pool, node) != LOS_OK) {
            break;
        }

        newPtr = OsMemRealloc(pool, ptr, node, size, intSave);
    } while (0);
    MEM_UNLOCK(poolHead, intSave);

    return newPtr;
}

#if (LOSCFG_MEM_FREE_BY_TASKID == 1)
STATIC VOID MemNodeFreeByTaskIDHandle(struct OsMemNodeHead *curNode, VOID *arg)
{
    UINT32 *args = (UINT32 *)arg;
    UINT32 taskID = *args;
    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)(UINTPTR)(*(args + 1));
    struct OsMemUsedNodeHead *node = NULL;
    if (!OS_MEM_NODE_GET_USED_FLAG(curNode->sizeAndFlag)) {
        return;
    }

    node = (struct OsMemUsedNodeHead *)curNode;
    if (node->header.taskID == taskID) {
        OsMemFree(poolHead, &node->header);
    }
    return;
}

UINT32 LOS_MemFreeByTaskID(VOID *pool, UINT32 taskID)
{
    UINT32 args[2] = { taskID, (UINT32)(UINTPTR)pool };
    if (pool == NULL) {
        return LOS_NOK;
    }

    if (taskID >= LOSCFG_BASE_CORE_TSK_LIMIT) {
        return LOS_NOK;
    }

    OsAllMemNodeDoHandle(pool, MemNodeFreeByTaskIDHandle, (VOID *)args);

    return LOS_OK;
}
#endif

UINT32 LOS_MemPoolSizeGet(const VOID *pool)
{
    UINT32 count = 0;

    if (pool == NULL) {
        return LOS_NOK;
    }

    count += ((struct OsMemPoolHead *)pool)->info.totalSize;
#if (LOSCFG_MEM_MUL_REGIONS == 1)
    count -= ((struct OsMemPoolHead *)pool)->info.totalGapSize;
#endif

#if OS_MEM_EXPAND_ENABLE
    UINT32 size;
    struct OsMemNodeHead *node = NULL;
    struct OsMemNodeHead *sentinel = OS_MEM_END_NODE(pool, count);

    while (OsMemIsLastSentinelNode(sentinel) == FALSE) {
        size = OS_MEM_NODE_GET_SIZE(sentinel->sizeAndFlag);
        node = OsMemSentinelNodeGet(sentinel);
        sentinel = OS_MEM_END_NODE(node, size);
        count += size;
    }
#endif
    return count;
}

STATIC VOID MemUsedGetHandle(struct OsMemNodeHead *curNode, VOID *arg)
{
    UINT32 *memUsed = (UINT32 *)arg;
    if (OS_MEM_IS_GAP_NODE(curNode)) {
        *memUsed += OS_MEM_NODE_HEAD_SIZE;
    } else if (OS_MEM_NODE_GET_USED_FLAG(curNode->sizeAndFlag)) {
        *memUsed += OS_MEM_NODE_GET_SIZE(curNode->sizeAndFlag);
    }
    return;
}

UINT32 LOS_MemTotalUsedGet(VOID *pool)
{
    UINT32 memUsed = 0;

    if (pool == NULL) {
        return LOS_NOK;
    }

    OsAllMemNodeDoHandle(pool, MemUsedGetHandle, (VOID *)&memUsed);

    return memUsed;
}

STATIC INLINE VOID OsMemMagicCheckPrint(struct OsMemNodeHead **tmpNode)
{
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
    PRINT_ERR("[%s], %d, memory check error!\n"
              "memory used but magic num wrong, magic num = 0x%x\n",
              __FUNCTION__, __LINE__, (*tmpNode)->magic);
#else
    (VOID)tmpNode;
#endif
}

STATIC UINT32 OsMemAddrValidCheckPrint(const VOID *pool, struct OsMemFreeNodeHead **tmpNode)
{
    if (((*tmpNode)->prev != NULL) && !OsMemAddrValidCheck(pool, (*tmpNode)->prev)) {
        PRINT_ERR("[%s], %d, memory check error!\n"
                  " freeNode.prev: %p is out of legal mem range\n",
                  __FUNCTION__, __LINE__, (*tmpNode)->prev);
        return LOS_NOK;
    }
    if (((*tmpNode)->next != NULL) && !OsMemAddrValidCheck(pool, (*tmpNode)->next)) {
        PRINT_ERR("[%s], %d, memory check error!\n"
                  " freeNode.next: %p is out of legal mem range\n",
                  __FUNCTION__, __LINE__, (*tmpNode)->next);
        return LOS_NOK;
    }
    return LOS_OK;
}

STATIC UINT32 OsMemIntegrityCheckSub(struct OsMemNodeHead **tmpNode, const VOID *pool)
{
    if (!OS_MEM_MAGIC_VALID(*tmpNode)) {
        OsMemMagicCheckPrint(tmpNode);
        return LOS_NOK;
    }

    if (!OsMemAddrValidCheck(pool, (*tmpNode)->ptr.prev)) {
        PRINT_ERR("[%s], %d, memory check error!\n"
                  " node prev: %p is out of legal mem range\n",
                  __FUNCTION__, __LINE__, (*tmpNode)->ptr.next);
        return LOS_NOK;
    }

    if (!OS_MEM_NODE_GET_USED_FLAG((*tmpNode)->sizeAndFlag)) { /* is free node, check free node range */
        if (OsMemAddrValidCheckPrint(pool, (struct OsMemFreeNodeHead **)tmpNode)) {
            return LOS_NOK;
        }
    }

    return LOS_OK;
}

STATIC UINT32 OsMemFreeListNodeCheck(const struct OsMemPoolHead *pool,
                const struct OsMemFreeNodeHead *node)
{
    if (!OsMemAddrValidCheck(pool, node) ||
        ((node->prev != NULL) && !OsMemAddrValidCheck(pool, node->prev)) ||
        ((node->next != NULL) && !OsMemAddrValidCheck(pool, node->next)) ||
        !OsMemAddrValidCheck(pool, node->header.ptr.prev)) {
        return LOS_NOK;
    }

    if (!OS_MEM_IS_ALIGNED(node, sizeof(VOID *)) ||
        !OS_MEM_IS_ALIGNED(node->prev, sizeof(VOID *)) ||
        !OS_MEM_IS_ALIGNED(node->next, sizeof(VOID *)) ||
        !OS_MEM_IS_ALIGNED(node->header.ptr.prev, sizeof(VOID *))) {
        return LOS_NOK;
    }

    return LOS_OK;
}

STATIC VOID OsMemPoolHeadCheck(const struct OsMemPoolHead *pool)
{
    struct OsMemFreeNodeHead *tmpNode = NULL;
    UINT32 index;
    UINT32 flag = 0;

    if ((pool->info.pool != pool) || !OS_MEM_IS_ALIGNED(pool, sizeof(VOID *))) {
        PRINT_ERR("wrong mem pool addr: %p, func: %s, line: %d\n", pool, __FUNCTION__, __LINE__);
        return;
    }

    for (index = 0; index < OS_MEM_FREE_LIST_COUNT; index++) {
        for (tmpNode = pool->freeList[index]; tmpNode != NULL; tmpNode = tmpNode->next) {
            if (OsMemFreeListNodeCheck(pool, tmpNode)) {
                flag = 1;
                PRINT_ERR("FreeListIndex: %u, node: %p, bNode: %p, prev:%p, next: %p\n",
                          index, tmpNode, tmpNode->header.ptr.prev, tmpNode->prev, tmpNode->next);
            }
        }
    }

    if (flag) {
        PRINTK("mem pool info: poolAddr: %p, poolSize: 0x%x\n", pool, pool->info.totalSize);
#if (LOSCFG_MEM_WATERLINE == 1)
        PRINTK("mem pool info: poolWaterLine: 0x%x, poolCurUsedSize: 0x%x\n", pool->info.waterLine,
               pool->info.curUsedSize);
#endif
#if OS_MEM_EXPAND_ENABLE
        UINT32 size;
        struct OsMemNodeHead *node = NULL;
        struct OsMemNodeHead *sentinel = OS_MEM_END_NODE(pool, pool->info.totalSize);
        while (OsMemIsLastSentinelNode(sentinel) == FALSE) {
            size = OS_MEM_NODE_GET_SIZE(sentinel->sizeAndFlag);
            node = OsMemSentinelNodeGet(sentinel);
            sentinel = OS_MEM_END_NODE(node, size);
            PRINTK("expand node info: nodeAddr: 0x%x, nodeSize: 0x%x\n", node, size);
        }
#endif
    }
}

STATIC UINT32 OsMemIntegrityCheck(const struct OsMemPoolHead *pool, struct OsMemNodeHead **tmpNode,
                struct OsMemNodeHead **preNode)
{
    struct OsMemNodeHead *endNode = OS_MEM_END_NODE(pool, pool->info.totalSize);

    OsMemPoolHeadCheck(pool);

    *preNode = OS_MEM_FIRST_NODE(pool);
    do {
        for (*tmpNode = *preNode; *tmpNode < endNode; *tmpNode = OS_MEM_NEXT_NODE(*tmpNode)) {
            if (OS_MEM_IS_GAP_NODE(*tmpNode)) {
                continue;
            }
            if (OsMemIntegrityCheckSub(tmpNode, pool) == LOS_NOK) {
                return LOS_NOK;
            }
            *preNode = *tmpNode;
        }
#if OS_MEM_EXPAND_ENABLE
        if (OsMemIsLastSentinelNode(*tmpNode) == FALSE) {
            *preNode = OsMemSentinelNodeGet(*tmpNode);
            endNode = OS_MEM_END_NODE(*preNode, OS_MEM_NODE_GET_SIZE((*tmpNode)->sizeAndFlag));
        } else
#endif
        {
            break;
        }
    } while (1);
    return LOS_OK;
}

#if (LOSCFG_KERNEL_PRINTF != 0)
STATIC VOID OsMemNodeInfo(const struct OsMemNodeHead *tmpNode,
                          const struct OsMemNodeHead *preNode)
{
    struct OsMemUsedNodeHead *usedNode = NULL;
    struct OsMemFreeNodeHead *freeNode = NULL;

    if (tmpNode == preNode) {
        PRINTK("\n the broken node is the first node\n");
    }

    if (OS_MEM_NODE_GET_USED_FLAG(tmpNode->sizeAndFlag)) {
        usedNode = (struct OsMemUsedNodeHead *)tmpNode;
        PRINTK("\n broken node head: %p  "
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            "0x%x  "
#endif
            "0x%x, ",
            usedNode->header.ptr.prev,
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            usedNode->header.magic,
#endif
            usedNode->header.sizeAndFlag);
    } else {
        freeNode = (struct OsMemFreeNodeHead *)tmpNode;
        PRINTK("\n broken node head: %p  %p  %p  "
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            "0x%x  "
#endif
            "0x%x, ",
            freeNode->header.ptr.prev, freeNode->next, freeNode->prev,
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            freeNode->header.magic,
#endif
            freeNode->header.sizeAndFlag);
    }

    if (OS_MEM_NODE_GET_USED_FLAG(preNode->sizeAndFlag)) {
        usedNode = (struct OsMemUsedNodeHead *)preNode;
        PRINTK("prev node head: %p  "
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            "0x%x  "
#endif
            "0x%x\n",
            usedNode->header.ptr.prev,
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            usedNode->header.magic,
#endif
            usedNode->header.sizeAndFlag);
    } else {
        freeNode = (struct OsMemFreeNodeHead *)preNode;
        PRINTK("prev node head: %p  %p  %p  "
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            "0x%x  "
#endif
            "0x%x, ",
            freeNode->header.ptr.prev, freeNode->next, freeNode->prev,
#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
            freeNode->header.magic,
#endif
            freeNode->header.sizeAndFlag);
    }

#if (LOSCFG_MEM_LEAKCHECK == 1)
    OsMemNodeBacktraceInfo(tmpNode, preNode);
#endif
}
#endif

struct OsMemIntegrityCheckInfo {
    struct OsMemNodeHead preNode;
    struct OsMemNodeHead errNode;
};

struct OsMemIntegrityCheckInfo g_integrityCheckRecord = {0};

STATIC INLINE VOID OsMemCheckInfoRecord(const struct OsMemNodeHead *errNode,
                                     const struct OsMemNodeHead *preNode)
{
    (VOID)memcpy(&g_integrityCheckRecord.preNode, preNode, sizeof(struct OsMemNodeHead));
    (VOID)memcpy(&g_integrityCheckRecord.errNode, errNode, sizeof(struct OsMemNodeHead));
}

STATIC VOID OsMemIntegrityCheckError(struct OsMemPoolHead *pool,
                                     const struct OsMemNodeHead *tmpNode,
                                     const struct OsMemNodeHead *preNode,
                                     UINT32 intSave)
{
#if (LOSCFG_KERNEL_PRINTF != 0)
    OsMemNodeInfo(tmpNode, preNode);
#endif
    OsMemCheckInfoRecord(tmpNode, preNode);
#if (LOSCFG_MEM_FREE_BY_TASKID == 1 || LOSCFG_TASK_MEM_USED == 1)
    LosTaskCB *taskCB = NULL;
    if (OS_MEM_NODE_GET_USED_FLAG(preNode->sizeAndFlag)) {
        struct OsMemUsedNodeHead *usedNode = (struct OsMemUsedNodeHead *)preNode;
        UINT32 taskID = usedNode->header.taskID;
        if (taskID >= LOSCFG_BASE_CORE_TSK_LIMIT) {
            MEM_UNLOCK(pool, intSave);
            LOS_Panic("Task ID %u in pre node is invalid!\n", taskID);
        }

        taskCB = OS_TCB_FROM_TID(taskID);
        if ((taskCB->taskStatus & OS_TASK_STATUS_UNUSED) || (taskCB->taskEntry == NULL)) {
            MEM_UNLOCK(pool, intSave);
            LOS_Panic("\r\nTask ID %u in pre node is not created!\n", taskID);
        }
    } else {
        PRINTK("The prev node is free\n");
    }
    MEM_UNLOCK(pool, intSave);
    PRINT_ERR("cur node: 0x%x, pre node: 0x%x, pre node was allocated by task: %d, %s\n",
              (unsigned int)tmpNode, (unsigned int)preNode, taskCB->taskID, taskCB->taskName);
    LOS_Panic("Memory integrity check error!\n");
#else
    MEM_UNLOCK(pool, intSave);
    LOS_Panic("Memory integrity check error, cur node: 0x%x, pre node: 0x%x\n", tmpNode, preNode);
#endif
}

#if (LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK == 1)
STATIC INLINE UINT32 OsMemAllocCheck(struct OsMemPoolHead *pool, UINT32 intSave)
{
    struct OsMemNodeHead *tmpNode = NULL;
    struct OsMemNodeHead *preNode = NULL;

    if (OsMemIntegrityCheck(pool, &tmpNode, &preNode)) {
        OsMemIntegrityCheckError(pool, tmpNode, preNode, intSave);
        return LOS_NOK;
    }
    return LOS_OK;
}
#endif

UINT32 LOS_MemIntegrityCheck(const VOID *pool)
{
    if (pool == NULL) {
        return LOS_NOK;
    }

    struct OsMemPoolHead *poolHead = (struct OsMemPoolHead *)pool;
    struct OsMemNodeHead *tmpNode = NULL;
    struct OsMemNodeHead *preNode = NULL;
    UINT32 intSave = 0;

    MEM_LOCK(poolHead, intSave);
    if (OsMemIntegrityCheck(poolHead, &tmpNode, &preNode)) {
        goto ERROR_OUT;
    }
    MEM_UNLOCK(poolHead, intSave);
    return LOS_OK;

ERROR_OUT:
    OsMemIntegrityCheckError(poolHead, tmpNode, preNode, intSave);
    return LOS_NOK;
}

STATIC INLINE VOID OsMemInfoGet(struct OsMemNodeHead *node,
                LOS_MEM_POOL_STATUS *poolStatus)
{
    UINT32 totalUsedSize = 0;
    UINT32 totalFreeSize = 0;
    UINT32 usedNodeNum = 0;
    UINT32 freeNodeNum = 0;
    UINT32 maxFreeSize = 0;
    UINT32 size;

    if (!OS_MEM_NODE_GET_USED_FLAG(node->sizeAndFlag)) {
        size = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
        ++freeNodeNum;
        totalFreeSize += size;
        if (maxFreeSize < size) {
            maxFreeSize = size;
        }
    } else {
        if (OS_MEM_IS_GAP_NODE(node)) {
            size = OS_MEM_NODE_HEAD_SIZE;
        } else {
            size = OS_MEM_NODE_GET_SIZE(node->sizeAndFlag);
        }
        ++usedNodeNum;
        totalUsedSize += size;
    }

    poolStatus->totalUsedSize += totalUsedSize;
    poolStatus->totalFreeSize += totalFreeSize;
    poolStatus->maxFreeNodeSize = poolStatus->maxFreeNodeSize > maxFreeSize ?
                                  poolStatus->maxFreeNodeSize : maxFreeSize;
    poolStatus->usedNodeNum += usedNodeNum;
    poolStatus->freeNodeNum += freeNodeNum;
}

STATIC VOID OsMemNodeInfoGetHandle(struct OsMemNodeHead *curNode, VOID *arg)
{
    LOS_MEM_POOL_STATUS *poolStatus = (LOS_MEM_POOL_STATUS *)arg;
    OsMemInfoGet(curNode, poolStatus);
    return;
}

UINT32 LOS_MemInfoGet(VOID *pool, LOS_MEM_POOL_STATUS *poolStatus)
{
    struct OsMemPoolHead *poolInfo = pool;
    UINT32 intSave = 0;

    if (poolStatus == NULL) {
        PRINT_ERR("can't use NULL addr to save info\n");
        return LOS_NOK;
    }

    if ((pool == NULL) || (poolInfo->info.pool != pool)) {
        PRINT_ERR("wrong mem pool addr: 0x%x, line:%d\n", (UINTPTR)poolInfo, __LINE__);
        return LOS_NOK;
    }

    (VOID)memset(poolStatus, 0, sizeof(LOS_MEM_POOL_STATUS));

    OsAllMemNodeDoHandle(pool, OsMemNodeInfoGetHandle, (VOID *)poolStatus);

    MEM_LOCK(poolInfo, intSave);
#if (LOSCFG_MEM_WATERLINE == 1)
    poolStatus->usageWaterLine = poolInfo->info.waterLine;
#endif
    MEM_UNLOCK(poolInfo, intSave);

    return LOS_OK;
}

STATIC VOID OsMemInfoPrint(VOID *pool)
{
#if (LOSCFG_KERNEL_PRINTF != 0)
    struct OsMemPoolHead *poolInfo = (struct OsMemPoolHead *)pool;
    LOS_MEM_POOL_STATUS status = {0};

    if (LOS_MemInfoGet(pool, &status) == LOS_NOK) {
        return;
    }

#if (LOSCFG_MEM_WATERLINE == 1)
    PRINTK("pool addr          pool size    used size     free size    "
           "max free node size   used node num     free node num      UsageWaterLine\n");
    PRINTK("---------------    --------     -------       --------     "
           "--------------       -------------      ------------      ------------\n");
    PRINTK("%-16p   0x%-8x   0x%-8x    0x%-8x   0x%-16x   0x%-13x    0x%-13x    0x%-13x\n",
           poolInfo->info.pool, LOS_MemPoolSizeGet(pool), status.totalUsedSize,
           status.totalFreeSize, status.maxFreeNodeSize, status.usedNodeNum,
           status.freeNodeNum, status.usageWaterLine);
#else
    PRINTK("pool addr          pool size    used size     free size    "
           "max free node size   used node num     free node num\n");
    PRINTK("---------------    --------     -------       --------     "
           "--------------       -------------      ------------\n");
    PRINTK("%-16p  0x%-8x   0x%-8x    0x%-8x   0x%-16x   0x%-13x    0x%-13x\n",
           poolInfo->info.pool, LOS_MemPoolSizeGet(pool), status.totalUsedSize,
           status.totalFreeSize, status.maxFreeNodeSize, status.usedNodeNum,
           status.freeNodeNum);
#endif
#endif
}

UINT32 LOS_MemFreeNodeShow(VOID *pool)
{
#if (LOSCFG_KERNEL_PRINTF != 0)
    struct OsMemPoolHead *poolInfo = (struct OsMemPoolHead *)pool;

    if ((poolInfo == NULL) || ((UINTPTR)pool != (UINTPTR)poolInfo->info.pool)) {
        PRINT_ERR("wrong mem pool addr: 0x%x, line: %d\n", (UINTPTR)poolInfo, __LINE__);
        return LOS_NOK;
    }

    struct OsMemFreeNodeHead *node = NULL;
    UINT32 countNum[OS_MEM_FREE_LIST_COUNT] = {0};
    UINT32 index;
    UINT32 intSave = 0;

    MEM_LOCK(poolInfo, intSave);
    for (index = 0; index < OS_MEM_FREE_LIST_COUNT; index++) {
        node = poolInfo->freeList[index];
        while (node) {
            node = node->next;
            countNum[index]++;
        }
    }
    MEM_UNLOCK(poolInfo, intSave);

    PRINTK("\n   ************************ left free node number**********************\n");
    for (index = 0; index < OS_MEM_FREE_LIST_COUNT; index++) {
        if (countNum[index] == 0) {
            continue;
        }

        PRINTK("free index: %03u, ", index);
        if (index < OS_MEM_SMALL_BUCKET_COUNT) {
            PRINTK("size: [0x%x], num: %u\n", (index + 1) << 2, countNum[index]); /* 2: setup is 4. */
        } else {
            UINT32 val = 1 << (((index - OS_MEM_SMALL_BUCKET_COUNT) >> OS_MEM_SLI) + OS_MEM_LARGE_START_BUCKET);
            UINT32 offset = val >> OS_MEM_SLI;
            PRINTK("size: [0x%x, 0x%x], num: %u\n",
                   (offset * ((index - OS_MEM_SMALL_BUCKET_COUNT) % (1 << OS_MEM_SLI))) + val,
                   ((offset * (((index - OS_MEM_SMALL_BUCKET_COUNT) % (1 << OS_MEM_SLI)) + 1)) + val - 1),
                   countNum[index]);
        }
    }
    PRINTK("\n   ********************************************************************\n\n");
#endif
    return LOS_OK;
}

VOID LOS_MemUnlockEnable(VOID *pool)
{
    if (pool == NULL) {
        return;
    }

    ((struct OsMemPoolHead *)pool)->info.attr |= OS_MEM_POOL_UNLOCK_ENABLE;
}

#if (LOSCFG_MEM_MUL_REGIONS == 1)
STATIC INLINE UINT32 OsMemMulRegionsParamCheck(VOID *pool, const LosMemRegion * const memRegions,
                                                UINT32 memRegionCount)
{
    const LosMemRegion *memRegion = NULL;
    VOID *lastStartAddress = NULL;
    VOID *curStartAddress = NULL;
    UINT32 lastLength;
    UINT32 curLength;
    UINT32 regionCount;

    if ((pool != NULL) && (((struct OsMemPoolHead *)pool)->info.pool != pool)) {
        PRINT_ERR("wrong mem pool addr: %p, func: %s, line: %d\n", pool, __FUNCTION__, __LINE__);
        return LOS_NOK;
    }

    if (pool != NULL) {
        lastStartAddress = pool;
        lastLength = ((struct OsMemPoolHead *)pool)->info.totalSize;
    }

    memRegion = memRegions;
    regionCount = 0;
    while (regionCount < memRegionCount) {
        curStartAddress = memRegion->startAddress;
        curLength = memRegion->length;
        if ((curStartAddress == NULL) || (curLength == 0)) {
            PRINT_ERR("Memory address or length configured wrongly:address:0x%x, the length:0x%x\n",
                      (UINTPTR)curStartAddress, curLength);
            return LOS_NOK;
        }
        if (((UINTPTR)curStartAddress & (OS_MEM_ALIGN_SIZE - 1)) || (curLength & (OS_MEM_ALIGN_SIZE - 1))) {
            PRINT_ERR("Memory address or length configured not aligned:address:0x%x, the length:0x%x, alignsize:%d\n",
                      (UINTPTR)curStartAddress, curLength, OS_MEM_ALIGN_SIZE);
            return LOS_NOK;
        }
        if ((lastStartAddress != NULL) && (((UINT8 *)lastStartAddress + lastLength) >= (UINT8 *)curStartAddress)) {
            PRINT_ERR("Memory regions overlapped, the last start address:0x%x, "
                      "the length:0x%x, the current start address:0x%x\n",
                      (UINTPTR)lastStartAddress, lastLength, (UINTPTR)curStartAddress);
            return LOS_NOK;
        }
        memRegion++;
        regionCount++;
        lastStartAddress = curStartAddress;
        lastLength = curLength;
    }
    return LOS_OK;
}

STATIC INLINE VOID OsMemMulRegionsLink(struct OsMemPoolHead *poolHead, VOID *lastStartAddress, UINT32 lastLength,
                                       struct OsMemNodeHead *lastEndNode, const LosMemRegion *memRegion)
{
    UINT32 curLength;
    UINT32 gapSize;
    struct OsMemNodeHead *curEndNode = NULL;
    struct OsMemNodeHead *curFreeNode = NULL;
    VOID *curStartAddress = NULL;

    curStartAddress = memRegion->startAddress;
    curLength = memRegion->length;
#ifdef LOSCFG_KERNEL_LMS
    UINT32 resize = 0;
    if (g_lms != NULL) {
        /*
         * resize == 0, shadow memory init failed, no shadow memory for this pool, set poolSize as original size.
         * resize != 0, shadow memory init successful, set poolSize as resize.
         */
        resize = g_lms->init(curStartAddress, curLength);
        curLength = (resize == 0) ? curLength : resize;
    }
#endif
    // mark the gap between two regions as one used node
    gapSize = (UINT8 *)(curStartAddress) - ((UINT8 *)(poolHead) + poolHead->info.totalSize);
    lastEndNode->sizeAndFlag = gapSize + OS_MEM_NODE_HEAD_SIZE;
    OS_MEM_SET_MAGIC(lastEndNode);
    OS_MEM_NODE_SET_USED_FLAG(lastEndNode->sizeAndFlag);

    // mark the gap node with magic number
    OS_MEM_MARK_GAP_NODE(lastEndNode);

    poolHead->info.totalSize += (curLength + gapSize);
    poolHead->info.totalGapSize += gapSize;

    curFreeNode = (struct OsMemNodeHead *)curStartAddress;
    curFreeNode->sizeAndFlag = curLength - OS_MEM_NODE_HEAD_SIZE;
    curFreeNode->ptr.prev = lastEndNode;
    OS_MEM_SET_MAGIC(curFreeNode);
    OsMemFreeNodeAdd(poolHead, (struct OsMemFreeNodeHead *)curFreeNode);

    curEndNode = OS_MEM_END_NODE(curStartAddress, curLength);
    curEndNode->sizeAndFlag = 0;
    curEndNode->ptr.prev = curFreeNode;
    OS_MEM_SET_MAGIC(curEndNode);
    OS_MEM_NODE_SET_USED_FLAG(curEndNode->sizeAndFlag);

#if (LOSCFG_MEM_WATERLINE == 1)
    poolHead->info.curUsedSize += OS_MEM_NODE_HEAD_SIZE;
    poolHead->info.waterLine = poolHead->info.curUsedSize;
#endif
}

UINT32 LOS_MemRegionsAdd(VOID *pool, const LosMemRegion *const memRegions, UINT32 memRegionCount)
{
    UINT32 ret;
    UINT32 lastLength;
    UINT32 curLength;
    UINT32 regionCount;
    struct OsMemPoolHead *poolHead = NULL;
    struct OsMemNodeHead *lastEndNode = NULL;
    struct OsMemNodeHead *firstFreeNode = NULL;
    const LosMemRegion *memRegion = NULL;
    VOID *lastStartAddress = NULL;
    VOID *curStartAddress = NULL;

    ret = OsMemMulRegionsParamCheck(pool, memRegions, memRegionCount);
    if (ret != LOS_OK) {
        return ret;
    }

    memRegion = memRegions;
    regionCount = 0;
    if (pool != NULL) { // add the memory regions to the specified memory pool
        poolHead = (struct OsMemPoolHead *)pool;
        lastStartAddress = pool;
        lastLength = poolHead->info.totalSize;
    } else { // initialize the memory pool with the first memory region
        lastStartAddress = memRegion->startAddress;
        lastLength = memRegion->length;
        poolHead = (struct OsMemPoolHead *)lastStartAddress;
        ret = LOS_MemInit(lastStartAddress, lastLength);
        if (ret != LOS_OK) {
            return ret;
        }
        memRegion++;
        regionCount++;
    }

    firstFreeNode = OS_MEM_FIRST_NODE(lastStartAddress);
    lastEndNode = OS_MEM_END_NODE(lastStartAddress, poolHead->info.totalSize);
    /* traverse the rest memory regions, and initialize them as free nodes and link together */
    while (regionCount < memRegionCount) {
        curStartAddress = memRegion->startAddress;
        curLength = memRegion->length;

        OsMemMulRegionsLink(poolHead, lastStartAddress, lastLength, lastEndNode, memRegion);
        lastStartAddress = curStartAddress;
        lastLength = curLength;
        lastEndNode = OS_MEM_END_NODE(poolHead, poolHead->info.totalSize);
        memRegion++;
        regionCount++;
    }

    firstFreeNode->ptr.prev = lastEndNode;
    return ret;
}
#endif

UINT32 OsMemSystemInit(VOID)
{
    UINT32 ret;

#if (LOSCFG_SYS_EXTERNAL_HEAP == 0)
    m_aucSysMem0 = g_memStart;
#else
    m_aucSysMem0 = LOSCFG_SYS_HEAP_ADDR;
#endif

    ret = LOS_MemInit(m_aucSysMem0, LOSCFG_SYS_HEAP_SIZE);
    PRINT_INFO("LiteOS heap memory address:%p, size:0x%lx\n", m_aucSysMem0, (unsigned long int)LOSCFG_SYS_HEAP_SIZE);
    return ret;
}

#if (LOSCFG_PLATFORM_EXC == 1)
STATIC VOID OsMemExcInfoGetSub(struct OsMemPoolHead *pool, MemInfoCB *memExcInfo)
{
    struct OsMemNodeHead *tmpNode = NULL;
    UINT32 taskID = OS_TASK_ERRORID;
    UINT32 intSave = 0;

    (VOID)memset(memExcInfo, 0, sizeof(MemInfoCB));

    MEM_LOCK(pool, intSave);
    memExcInfo->type = MEM_MANG_MEMORY;
    memExcInfo->startAddr = (UINTPTR)pool->info.pool;
    memExcInfo->size = pool->info.totalSize;
    memExcInfo->free = pool->info.totalSize - pool->info.curUsedSize;

    struct OsMemNodeHead *firstNode = OS_MEM_FIRST_NODE(pool);
    struct OsMemNodeHead *endNode = OS_MEM_END_NODE(pool, pool->info.totalSize);

    for (tmpNode = firstNode; tmpNode < endNode; tmpNode = OS_MEM_NEXT_NODE(tmpNode)) {
        memExcInfo->blockSize++;
        if (OS_MEM_NODE_GET_USED_FLAG(tmpNode->sizeAndFlag)) {
            if (!OS_MEM_MAGIC_VALID(tmpNode) ||
                !OsMemAddrValidCheck(pool, tmpNode->ptr.prev)) {
#if (LOSCFG_MEM_FREE_BY_TASKID == 1 || LOSCFG_TASK_MEM_USED == 1)
                taskID = ((struct OsMemUsedNodeHead *)tmpNode)->header.taskID;
#endif
                goto ERROUT;
            }
        } else { /* is free node, check free node range */
            struct OsMemFreeNodeHead *freeNode = (struct OsMemFreeNodeHead *)tmpNode;
            if (OsMemAddrValidCheckPrint(pool, &freeNode)) {
                goto ERROUT;
            }
        }
    }
    MEM_UNLOCK(pool, intSave);
    return;

ERROUT:
    memExcInfo->errorAddr = (UINTPTR)((CHAR *)tmpNode + OS_MEM_NODE_HEAD_SIZE);
    memExcInfo->errorLen = OS_MEM_NODE_GET_SIZE(tmpNode->sizeAndFlag) - OS_MEM_NODE_HEAD_SIZE;
    memExcInfo->errorOwner = taskID;
    MEM_UNLOCK(pool, intSave);
    return;
}

UINT32 OsMemExcInfoGet(UINT32 memNumMax, MemInfoCB *memExcInfo)
{
    UINT8 *buffer = (UINT8 *)memExcInfo;
    UINT32 count = 0;

#if (LOSCFG_MEM_MUL_POOL == 1)
    struct OsMemPoolHead *memPool = g_poolHead;
    while (memPool != NULL) {
        OsMemExcInfoGetSub(memPool, (MemInfoCB *)buffer);
        count++;
        buffer += sizeof(MemInfoCB);
        if (count >= memNumMax) {
            break;
        }
        memPool = memPool->nextPool;
    }
#else
    OsMemExcInfoGetSub(m_aucSysMem0, buffer);
    count++;
#endif

    return count;
}
#endif

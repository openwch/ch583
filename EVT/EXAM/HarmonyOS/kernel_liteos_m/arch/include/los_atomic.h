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

#ifndef _LOS_ATOMIC_H
#define _LOS_ATOMIC_H

#include "los_compiler.h"
//#include "los_arch_atomic.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define LOS_AtomicRead ArchAtomicRead
#define LOS_AtomicSet ArchAtomicSet
#define LOS_AtomicAdd ArchAtomicAdd
#define LOS_AtomicSub ArchAtomicSub
#define LOS_AtomicInc ArchAtomicInc
#define LOS_AtomicIncRet ArchAtomicIncRet
#define LOS_AtomicDec ArchAtomicDec
#define LOS_AtomicDecRet ArchAtomicDecRet
#define LOS_Atomic64Read ArchAtomic64Read
#define LOS_Atomic64Set ArchAtomic64Set
#define LOS_Atomic64Add ArchAtomic64Add
#define LOS_Atomic64Sub ArchAtomic64Sub
#define LOS_Atomic64Inc ArchAtomic64Inc
#define LOS_Atomic64IncRet ArchAtomic64IncRet
#define LOS_Atomic64Dec ArchAtomic64Dec
#define LOS_Atomic64DecRet ArchAtomic64DecRet
#define LOS_AtomicXchg32bits ArchAtomicXchg32bits
#define LOS_AtomicXchg64bits ArchAtomicXchg64bits
#define LOS_AtomicCmpXchg32bits ArchAtomicCmpXchg32bits
#define LOS_AtomicCmpXchg64bits ArchAtomicCmpXchg64bits
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_ATOMIC_H */


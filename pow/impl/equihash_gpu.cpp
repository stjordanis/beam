// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "equihash_gpu.h"
#include "compat/endian.h"

#include <string.h>

typedef uint32_t u32;
typedef unsigned char uchar;

#ifndef WN
#define WN	144
#endif

#ifndef WK
#define WK	5
#endif

#define NDIGITS		(WK+1)
#define DIGITBITS	(WN/(NDIGITS))

static const u32 PROOFSIZE = 1 << WK;
static const u32 BASE = 1 << DIGITBITS;
static const u32 NHASHES = 2 * BASE;
static const u32 HASHESPERBLAKE = 512 / WN;
static const u32 HASHOUT = HASHESPERBLAKE * WN / 8;

#define COMPRESSED_SOL_SIZE (PROOFSIZE * (DIGITBITS + 1) / 8)

typedef u32 proof[PROOFSIZE];

void EquihashGpu::initState(blake2b_state& state)
{
    uint32_t le_N = htole32(WN);
    uint32_t le_K = htole32(WK);

    unsigned char personalization[BLAKE2B_PERSONALBYTES] = {};
    memcpy(personalization, "ZcashPoW", 8);
    memcpy(personalization + 8, &le_N, 4);
    memcpy(personalization + 12, &le_K, 4);

    const uint8_t outlen = (512 / WN)*WN / 8;

    static_assert(!((!outlen) || (outlen > BLAKE2B_OUTBYTES)));

    blake2b_param param = { 0 };
    param.digest_length = outlen;
    param.fanout = 1;
    param.depth = 1;

    memcpy(&param.personal, personalization, BLAKE2B_PERSONALBYTES);
}

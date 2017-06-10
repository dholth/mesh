/***********************************************************************************
Copyright (c) Nordic Semiconductor ASA
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  3. Neither the name of Nordic Semiconductor ASA nor the names of other
  contributors to this software may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************************************************/
#include "rand.h"

#include <fcntl.h>
#include <unistd.h>

/*****************************************************************************
* Local defines
*****************************************************************************/
#define ROT(x,k) (((x)<<(k))|((x)>>(32-(k)))) /** PRNG cyclic leftshift */
#define SMALL_PRNG_BASE_SEED    (0xf1ea5eed)  /** Base seed for PRNG, defined by the author of the generator. */
/*****************************************************************************
* Interface functions
*****************************************************************************/
uint32_t rand_prng_seed(prng_t* p_prng)
{
    uint32_t seed = 0;
    /* Get true random seed from HW. */
    uint32_t error_code = rand_hw_rng_get((uint8_t*) &seed, 4);
    if (error_code != 0)
    {
        return error_code;
    }

    /* establish base magic numbers */
    p_prng->a = SMALL_PRNG_BASE_SEED;
    p_prng->b = seed;
    p_prng->c = seed;
    p_prng->d = seed;

    /* run the prng a couple of times to flush out the seeds */
    for (uint32_t i = 0; i < 20; ++i)
    {
        (void)rand_prng_get(p_prng);
    }

    return 0;
}

uint32_t rand_prng_get(prng_t* p_prng)
{
    /* Bob Jenkins' small PRNG
        http://burtleburtle.net/bob/rand/smallprng.html */
    uint32_t e = p_prng->a - ROT(p_prng->b, 27);
    p_prng->a = p_prng->b ^ ROT(p_prng->c, 17);
    p_prng->b = p_prng->c + p_prng->d;
    p_prng->c = p_prng->d + e;
    p_prng->d = e + p_prng->a;
    return p_prng->d;
}

uint32_t rand_hw_rng_get(uint8_t * p_result, uint16_t len)
{
    int random_file = open("/dev/random", O_RDONLY);

    if(random_file == -1)
    {
        return 1;
    }

    /* Reading from /dev/random will always give the number of bytes asked for,
     * so there is no need for a read() loop here.
     */
    if(read(random_file, p_result, len) == -1)
    {
        return 1;
    }

    close(random_file);
    return 0;
}


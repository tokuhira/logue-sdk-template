/*
    BSD 3-Clause License

    Copyright (c) 2019, tokuhira
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//*/

/*
 * File: saw.cpp
 *
 * Naive saw oscillator test
 *
 */

#include "userosc.h"

typedef struct State {
  float w0;
  float phase;
  float duty;
  float drive;
  float lfo, lfoz;
  uint8_t flags;
} State;

enum {
  k_flags_none = 0,
  k_flag_reset = 1<<0,
};

static State s_state;

void OSC_INIT(uint32_t platform, uint32_t api)
{
  s_state.w0    = 0.f;
  s_state.phase = 0.f;
  s_state.duty  = 0.f;
  s_state.drive = 1.f;
  s_state.lfo = s_state.lfoz = 0.f;
  s_state.flags = k_flags_none;
}

void OSC_CYCLE(const user_osc_param_t * const params,
               int32_t *yn,
               const uint32_t frames)
{
  const uint8_t flags = s_state.flags;
  s_state.flags = k_flags_none;
    
  const float w0 = s_state.w0 = osc_w0f_for_note((params->pitch)>>8, params->pitch & 0xFF);
  float phase = (flags & k_flag_reset) ? 0.f : s_state.phase;
  
  const float duty = s_state.duty;
  const float drive = s_state.drive;

  const float lfo = s_state.lfo = q31_to_f32(params->shape_lfo);
  float lfoz = (flags & k_flag_reset) ? lfo : s_state.lfoz;
  const float lfo_inc = (lfo - lfoz) / frames;
  
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  
  for (; y != y_e; ) {
    float sig;

    // Main signal, not certain...
    if (phase < (duty * 0.5f) || (1.f - duty * 0.5f) < phase) {
      sig = 1.f - phase * 2.f;
    } else {
      sig = phase * 2.f - (phase < 0.5 ? 0.f : 2.f);
    }
    sig *= drive;
    sig -= (uint32_t)sig;

    *(y++) = f32_to_q31(sig);

    phase += w0;
    phase -= (uint32_t)phase;

    lfoz += lfo_inc;
  }
  
  s_state.phase = phase;
  s_state.lfoz = lfoz;
}

void OSC_NOTEON(const user_osc_param_t * const params)
{
  s_state.flags |= k_flag_reset;
}

void OSC_NOTEOFF(const user_osc_param_t * const params)
{
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value)
{
  const float valf = param_val_to_f32(value);
  
  switch (index) {
  case k_user_osc_param_id1:
  case k_user_osc_param_id2:
  case k_user_osc_param_id3:
  case k_user_osc_param_id4:
  case k_user_osc_param_id5:
  case k_user_osc_param_id6:
    break;
  case k_user_osc_param_shape:
    s_state.duty = 0.f + valf;
    break;
  case k_user_osc_param_shiftshape:
    s_state.drive = 1.f + valf;
    break;
  default:
    break;
  }
}


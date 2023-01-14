/*
 * [2-Clause BSD License]
 *
 * Copyright 2022 Victor Zappi
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LIN_INTERPOLATOR_H_
#define LIN_INTERPOLATOR_H_


class LinInterpolator {
    public:
    LinInterpolator();
    LinInterpolator(float startVal, float endVal, int steps);
    void setup(float startVal, float endVal, int steps);
    float operator[](int index);

    private:
    float start_val;
    float delta;
};


//-------------------------------------------------------------------
LinInterpolator::LinInterpolator()
{
    start_val = -1;
    delta = 0;
}

inline LinInterpolator::LinInterpolator(float startVal, float endVal, int steps)
{
    setup(startVal, endVal, steps);
}

inline void LinInterpolator::setup(float startVal, float endVal, int steps)
{
    start_val = startVal;
    delta = (endVal-startVal) / float(steps);
}

inline float LinInterpolator::operator[](int index)
{
    return start_val + delta*index;
}

#endif /* LIN_INTERPOLATOR_H_ */

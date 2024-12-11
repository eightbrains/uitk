//-----------------------------------------------------------------------------
// Copyright 2021 - 2024 Eight Brains Studios, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef UITK_SOUND_H
#define UITK_SOUND_H

#include <stdint.h>

namespace uitk {

class Sound
{
public:
    enum class Loop { kNo = 0, kYes = 1 };
    /// Plays the given raw audio. The samples can be one or two channels
    /// (interleaved). `count` is the length of the passed array,
    /// not the number of bytes. If `loop` is Loop::kYes, the only way to
    /// stop looping is to call stop(). (This is a limitation from the
    /// Win32 PlaySound() function.)
    virtual void play(int16_t *samples, uint32_t count, int rateHz, int nChannels, Loop loop = Loop::kNo) = 0;

    /// Stops all sound playing.
    virtual void stop() = 0;
};

} // namespace uitk
#endif // UITK_SOUND_H

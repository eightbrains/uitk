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

#include "Win32Sound.h"

#include <string.h> // for memcpy

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

namespace uitk {

void Win32Sound::play(int16_t *samples, uint32_t count, int rateHz, int nChannels, Sound::Loop loop /*= kNo*/)
{    
    const int kWAVHeaderBytes = 44;

    const int nSoundBytes = count * sizeof(int16_t);
    const int nBytes = nSoundBytes + kWAVHeaderBytes;
    const int bytesPerSec = rateHz * nChannels * sizeof(int16_t);

    char *wavBytes = new char[nBytes]; // use new[] so it doesn't initialize, unlike vector
    memcpy(wavBytes + kWAVHeaderBytes, (char*)samples, nSoundBytes);
    char *wav = wavBytes;
    // 'RIFF' chunk
    *wav++ = 'R';
    *wav++ = 'I';
    *wav++ = 'F';
    *wav++ = 'F';
    *(uint32_t*)wav = nBytes - 8;
    wav += 4;
    *wav++ = 'W';
    *wav++ = 'A';
    *wav++ = 'V';
    *wav++ = 'E';
    // 'fmt ' chunk
    *wav++ = 'f';
    *wav++ = 'm';
    *wav++ = 't';
    *wav++ = ' ';
    *(uint32_t*)wav = 16;
    wav += 4;
    *wav++ = 0x01;   *wav++ = 0x00;  // PCM integer (3 is float)
    *wav++ = nChannels; *wav++ = 0x00;
    *(uint32_t*)wav = rateHz;
    wav += 4;
    *(uint32_t*)wav = bytesPerSec;
    wav += 4;
    *(uint16_t*)wav = nChannels * uint16_t(sizeof(uint16_t));
    wav += 2;
    *(uint16_t*)wav = 8 * sizeof(uint16_t);
    wav += 2;
    // 'data' chunk
    *wav++ = 'd';
    *wav++ = 'a';
    *wav++ = 't';
    *wav++ = 'a';
    *(uint32_t*)wav = nSoundBytes;

    DWORD flags = SND_ASYNC | SND_MEMORY;
    if (loop == Sound::Loop::kYes) {
        flags |= SND_LOOP;
    }
    PlaySoundA(wavBytes, NULL, flags);

    delete [] wavBytes;
}

void Win32Sound::stop()
{
    PlaySound(NULL, NULL, SND_ASYNC);
}

} // namespace uitk

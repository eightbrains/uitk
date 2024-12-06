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

#include "WASMSound.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <string.h> // for memcpy

#include <assert.h>
#include <iostream>

static_assert(sizeof(uint32_t) == sizeof(ALuint),
              "ALuint is not 32 bits; update WASMSound::mActiveSources template parameter");

namespace uitk {

namespace {

// Checking for errors is mostly only useful for development.
// Pass a char* so that there is no construction cost for std::string.
bool checkError(const char *info) {
    ALenum error;
    if ((error = alGetError()) != AL_NO_ERROR) {
        std::cout << "[uitk.audio] Error at " << info << ": " << (int)error << std::endl;
        return true;
    }
    return false;
};

} // namespace

WASMSound::~WASMSound()
{
    if (mContext) {
        closeSound();
    }
}

void WASMSound::openSound()
{
    // For some reason, the calls to alcOpenDevice() and alcCreateContext()
    // fail with AL_INVALID_OPERATION, but they return non-null and workable
    // pointers.
    if (!mContext) {
        alGetError(); // clear error code
        auto *device = alcOpenDevice(NULL); // default device
        if (device) {
            mContext = alcCreateContext(device, NULL);
            if (!mContext) {
                checkError("alcCreateContext");
            }
        } else {
            std::cout << "[uitk.audio] Could not open sound device" << std::endl;
            checkError("alcOpenDevice");
        }
    }
    alcMakeContextCurrent((ALCcontext*)mContext);
}

void WASMSound::closeSound()
{
    // This probably is not actually going to get called, because the application
    // almost surely lasts as long as the webpage, and the user will just close
    // the tab/window. But we should do it properly just in case.
    if (mContext) {
        auto *device = alcGetContextsDevice((ALCcontext*)mContext);
        alcMakeContextCurrent(NULL);
        alcDestroyContext((ALCcontext*)mContext);
        mContext = nullptr;
        alcCloseDevice(device);
    }
}

void WASMSound::purgeCompletedSources()
{
    // OpenAL appears to have no way of telling when something is completed.
    // To avoid having a separate thread polling (which may not be possible
    // in WASM), just poll the next time play() is called.

    if (mContext && !mActiveSources.empty()) {
        alcMakeContextCurrent((ALCcontext*)mContext);
        for (auto it = mActiveSources.begin();  it != mActiveSources.end();  ) {
            auto source = *it;
            ALint state = AL_PLAYING;  // in case the call fails
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            checkError("alGetSourcei");
            if (state == AL_STOPPED) {
                ALint buffer;
                alGetSourcei(source, AL_BUFFER, &buffer);
                alGetError(); // clear error code
                alDeleteSources(1, (ALuint*)&source);
                checkError("alDeleteSources");
                alDeleteBuffers(1, (ALuint*)&buffer);
                checkError("alDeleteBuffers");
                it = mActiveSources.erase(it);  // it is already next, so don't increment
            } else {
                ++it;
            }
        }
    }
}

void WASMSound::play(int16_t *samples, uint32_t count, int rateHz, int nChannels)
{
    const int nBytes = count * sizeof(int16_t);

    // Don't initialize sound until we know the user actually wants to use
    // it (which is unlikely).
    openSound();

    alGetError(); // clear error code

    ALuint buffer;
    alGenBuffers(1, &buffer);
    checkError("alGenBuffers");

    auto format = (nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16);
    alBufferData(buffer, format, samples, nBytes, rateHz);
    checkError("alBufferData");

    ALuint source;
    alGenSources(1, &source);
    checkError("alGenSources");

    alSourcei(source, AL_BUFFER, buffer);
    checkError("alSourcei");

    alSourcePlay(source);
    checkError("alSourcePlay");

    mActiveSources.push_back(source);
    purgeCompletedSources();
}

} // namespace uitk

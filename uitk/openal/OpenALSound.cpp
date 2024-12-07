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

#include "OpenALSound.h"

#include <string.h> // for memcpy

#include <assert.h>
#include <iostream>

// ------------------- dynamically load OpenAL on Linux -----------------------
// Will also be true for non-Apple unixen: FreeBSD, etc.
#if (defined(__unix__) || defined(unix)) && !defined(__APPLE__)
#define IsLinux 1
#include <dlfcn.h>

// These are what the Linux headers use, although uint32_t, etc. would be better
using ALvoid = void;
using ALint = int;
using ALuint = unsigned int;
using ALsizei = int;
using ALenum = int;
using ALCboolean = char;
using ALCchar = char;
using ALCint = int;
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;

#define AL_NO_ERROR                              0
#define AL_INVALID_NAME                          0xA001
#define AL_INVALID_ENUM                          0xA002
#define AL_INVALID_VALUE                         0xA003
#define AL_INVALID_OPERATION                     0xA004
#define AL_OUT_OF_MEMORY                         0xA005

#define AL_FORMAT_MONO8                          0x1100
#define AL_FORMAT_MONO16                         0x1101
#define AL_FORMAT_STEREO8                        0x1102
#define AL_FORMAT_STEREO16                       0x1103

#define AL_LOOPING                               0x1007

#define AL_BUFFER                                0x1009
#define AL_SOURCE_STATE                          0x1010

#define AL_INITIAL                               0x1011
#define AL_PLAYING                               0x1012
#define AL_PAUSED                                0x1013
#define AL_STOPPED                               0x1014

#define OPENAL_LIST \
AL(ALCdevice*,  alcOpenDevice, const ALCchar *devicename) \
AL(ALCboolean,  alcCloseDevice, ALCdevice *device) \
AL(ALCcontext*, alcCreateContext, ALCdevice *device, const ALCint* attrlist) \
AL(ALCboolean,  alcMakeContextCurrent, ALCcontext *context) \
AL(void,        alcDestroyContext, ALCcontext *context) \
AL(ALCcontext*, alcGetCurrentContext, void) \
AL(ALCdevice*,  alcGetContextsDevice, ALCcontext *context) \
AL(ALenum,      alGetError, void) \
AL(void,        alGenSources, ALsizei n, ALuint *sources) \
AL(void,        alDeleteSources, ALsizei n, const ALuint *sources) \
AL(void,        alSourcei, ALuint source, ALenum param, ALint value) \
AL(void,        alGetSourcei, ALuint source,  ALenum param, ALint *value) \
AL(void,        alSourcePlay, ALuint source) \
AL(void,        alSourceStop, ALuint source) \
AL(void,        alGenBuffers, ALsizei n, ALuint *buffers) \
AL(void,        alDeleteBuffers, ALsizei n, const ALuint *buffers) \
AL(void,        alBufferData, ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq) \

#define AL(ret, name, ...) typedef ret name##proc(__VA_ARGS__); static name##proc * name;
OPENAL_LIST
#undef AL

void *gOpenAL = nullptr;

void unloadOpenAL()
{
    if (gOpenAL) {
        dlclose(gOpenAL);
        gOpenAL = nullptr;
    }
}

bool loadOpenAL()
{
    if (gOpenAL) {
        return true;
    }

    gOpenAL = dlopen("libopenal.so", RTLD_LAZY);
    if (gOpenAL) {
#define AL(ret, name, ...) name = (name##proc *)dlsym(gOpenAL, #name);
        OPENAL_LIST
#undef AL
        if (!alcOpenDevice) {
            std::cout << "[uitk.audio] Could not read symbols from libopenal.so (OpenAL)" << std::endl;
            unloadOpenAL();
            return false;
        }

        return true;
    }
    return false;
}
#endif // IsLinux
//-----------------------------------------------------------------------------
#ifndef IsLinux
#include <AL/al.h>
#include <AL/alc.h>
#endif // !IsLinux

static_assert(sizeof(uint32_t) == sizeof(ALuint),
              "ALuint is not 32 bits; update OpenALSound::mActiveSources template parameter");

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

OpenALSound::~OpenALSound()
{
    if (mContext) {
        closeSound();
    }
}

void OpenALSound::openSound()
{
    // For some reason, the calls to alcOpenDevice() and alcCreateContext()
    // fail with AL_INVALID_OPERATION, but they return non-null and workable
    // pointers.
    if (!mContext) {
#if IsLinux
        if (!loadOpenAL()) {
            std::cout << "[uitk.audio] Could open libopenal.so (OpenAL)" << std::endl;
            return;
        }
#endif // IsLinux
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

void OpenALSound::closeSound()
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
#if IsLinux
        unloadOpenAL();
#endif // IsLinux
    }
}

void OpenALSound::purgeSources(Purge purge)
{
    // OpenAL appears to have no way of telling when something is completed.
    // To avoid having a separate thread polling (which may not be possible
    // in OpenAL), just poll the next time play() is called.

    if (mContext && !mActiveSources.empty()) {
        alcMakeContextCurrent((ALCcontext*)mContext);
        for (auto it = mActiveSources.begin();  it != mActiveSources.end();  ) {
            auto source = *it;
            ALint state = AL_PLAYING;  // in case the call fails
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            checkError("alGetSourcei");
            if (state == AL_STOPPED || purge == Purge::kAll) {
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

void OpenALSound::play(int16_t *samples, uint32_t count, int rateHz,
                       int nChannels, Sound::Loop loop /*= kNo*/)
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

    if (loop == Sound::Loop::kYes) {
        alSourcei(source, AL_LOOPING, 1);
        checkError("alSourcei [loop]");
    }

    alSourcePlay(source);
    checkError("alSourcePlay");

    mActiveSources.push_back(source);
    purgeSources(Purge::kCompleted);
}

void OpenALSound::stop()
{
    purgeSources(Purge::kAll);
}

} // namespace uitk

//-----------------------------------------------------------------------------
// Copyright 2021 - 2023 Eight Brains Studios, LLC
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

#include "../OSApplication.h"

#include <chrono>
#include <list>
#include <memory>
#include <mutex>

namespace uitk {

template <typename W>  // W must by (efficiently) copyable
class DeferredFunctions // has it's own lock
{
public:
    OSApplication::SchedulingId add(W win, float delaySecs, bool repeats,
                                    std::function<void(OSApplication::SchedulingId)> f)
    {
        std::lock_guard<std::mutex> locker(mLock);

        auto id = ++mNextId;
        auto now = std::chrono::steady_clock::now();
        mFunctions.push_back(std::make_shared<Func>(id, f, win, delaySecs,
                                                    repeats, now, now));
        updateNextTime(mFunctions.back().get());
        reSort_locked();

        return id;
    }

    void remove(OSApplication::SchedulingId id)
    {
        std::lock_guard<std::mutex> locker(mLock);

        for (auto it = mFunctions.begin();  it != mFunctions.end();  ++it) {
            if ((*it)->id == id) {
                it = mFunctions.erase(it);  // mFunctions is still sorted afterwards
                break;
            }
        }
    }

    void removeForWindow(W win)
    {
        std::lock_guard<std::mutex> locker(mLock);

        for (auto it = mFunctions.begin();  it != mFunctions.end();  ++it) {
            if ((*it)->win == win) {
                it = mFunctions.erase(it);  // mFunctions is still sorted afterwards
            }
        }
    }

    void executeTick()
    {
        if (mFunctions.empty()) {
            return;
        }

        auto now = std::chrono::steady_clock::now();
        int nCallbacksRun = 0;

        // A callback may close the window that other callbacks are using,
        // or it may remove itself or other callbacks. In both cases we should
        // not execute any of those other callbacks. There seem to be two
        // options for this:
        // a) the (current) simple algorithm that starts over from the beginning
        //    after running a callback. It is potentially O(n^2), although
        //    large N is unlikely.
        // b) store all the callbacks that need to be executed in an instance
        //    variable, and removing a callback should also remove it from the
        //    mToExecute queue.

        while (true) {
            std::shared_ptr<Func> f;
            for (auto it = mFunctions.begin();  it != mFunctions.end();  ++it) {
                if (now >= (*it)->nextTime) {
                    f = (*it);
                    if ((*it)->repeats) {
                        updateNextTime(it->get());
                    } else {
                        it = mFunctions.erase(it);
                    }
                    break;
                }
            }
            if (f) {
                f->f(f->id);
                nCallbacksRun += 1;
            } else {
                break;  // didn't execute anything; break: nothing else is ready
            }
        }

        if (nCallbacksRun > 0) {
            std::lock_guard<std::mutex> locker(mLock);
            reSort_locked();
        }
    }

private:
    struct Func
    {
        OSApplication::SchedulingId id;
        std::function<void(OSApplication::SchedulingId)> f;
        W win;
        float delaySec;
        bool repeats;
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        std::chrono::time_point<std::chrono::steady_clock> nextTime;

        Func(OSApplication::SchedulingId id_,
             std::function<void(OSApplication::SchedulingId)> f_,
             W win_, float d, bool r, 
             std::chrono::time_point<std::chrono::steady_clock> st,
             std::chrono::time_point<std::chrono::steady_clock> nt)
            : id(id_), f(f_), win(win_), delaySec(d), repeats(r)
            , startTime(st), nextTime(nt)
        {}
    };

    OSApplication::SchedulingId mNextId = OSApplication::kInvalidSchedulingId;
    std::mutex mLock;
    // Should always be sorted. Uses shared_ptr<> so that an executing callback
    // can safely unschedule itself.
    std::list<std::shared_ptr<Func>> mFunctions;

    void reSort_locked()
    {
        mFunctions.sort([](const std::shared_ptr<Func>& x,
                           const std::shared_ptr<Func>& y) -> bool {
            return (x->nextTime < y->nextTime);
        });
    }

    void updateNextTime(Func *func)
    {
        // Try to avoid drift from accumulated floating point error from
        // just doing 'nextTime += delaySec'.

        // std::chrono::duration stores values as int not double, so using
        // seconds is no good. Note that std::chrono::microseconds specs
        // at least 55 bits, which is 1141 years' of microseconds.
        double totalDT = std::chrono::duration_cast<std::chrono::microseconds>(func->nextTime - func->startTime).count() / 1e6;
        double n = std::round(totalDT / double(func->delaySec)); // fix (n-1).9999999 or n.0000001
        double dt = (n + 1.0) * double(func->delaySec);
        uint64_t dt_usec = (uint64_t)(std::round(dt * 1e6));
        func->nextTime = func->startTime + std::chrono::microseconds(dt_usec);
    }
};

} // namespace uitk

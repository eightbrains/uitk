//-----------------------------------------------------------------------------
// Copyright 2021 Eight Brains Studios, LLC
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

#include "X11Clipboard.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <chrono>
#include <thread>

namespace uitk {

std::vector<unsigned char> getClipboardData(Display *display, ::Window w,
                                            Atom which,
                                            Atom target,
                                            Atom dataName) {
    XConvertSelection(display, which, target, dataName, w, CurrentTime);

    using ms = std::chrono::milliseconds;
    const int kMaxWaitMSec = 200;
    std::vector<unsigned char> data;
    auto startTime = std::chrono::steady_clock::now();
    auto now = startTime;
    XEvent event;
    while (std::chrono::duration_cast<ms>(now - startTime).count() < kMaxWaitMSec) {
        if (XCheckTypedWindowEvent(display, w, SelectionNotify, &event)) {
            if (event.xselection.property != None) {
                Atom receivedTarget;
                int format;
                unsigned long nItems;
                unsigned long nBytesLeft;
                char* bytes;
                XGetWindowProperty(event.xselection.display,
                                   event.xselection.requestor,
                                   event.xselection.property,
                                   0L,(~0L), 0, AnyPropertyType,
                                   &receivedTarget, &format, &nItems,
                                   &nBytesLeft, (unsigned char**)&bytes);
                if (receivedTarget == target) {
                    auto chunk = std::vector<unsigned char>(bytes, bytes + nItems);
                    data.insert(data.end(), chunk.begin(), chunk.end());
                }
                XFree(bytes);
                XDeleteProperty(event.xselection.display,
                                event.xselection.requestor,
                                event.xselection.property);
                if (nBytesLeft == 0) {
                    break;
                }
            }
        } else {
            std::this_thread::yield();  // avoid high CPU usage in spin-wait
        }
        now = std::chrono::steady_clock::now();
    }
    return data;
}

struct X11Clipboard::Impl
{
    Display *display = nullptr;
	Atom clipboardAtom;
    Atom primaryAtom;
    Atom secondaryAtom;
    Atom dataAtom;
    Atom targetsAtom;
    Atom textAtom;
    Atom utf8Atom;
    
    ::Window activeWindow = 0;

    bool weAreOwner = false;
    std::vector<Atom> supportedTypes;
    std::string utf8Data;

    bool weAreSelectionOwner = false;
    std::string utf8Selection;

    std::vector<unsigned char> getClipboardText(X11Clipboard::Selection sel) const {
        auto which = (sel == Selection::kTextSelection ? this->primaryAtom
                                                       : this->clipboardAtom);
        // Try UTF8_STRING
        auto utf8 = getClipboardData(this->display, this->activeWindow,
                                     which, this->utf8Atom, this->dataAtom);
        if (!utf8.empty()) {
            return utf8;
        }
        // If not, try TEXT
        utf8 = getClipboardData(this->display, this->activeWindow,
                                which, this->textAtom, this->dataAtom);
        if (!utf8.empty()) {
            return utf8;
        }
        // Fallback to XA_STRING
        return getClipboardData(this->display, this->activeWindow,
                                which, XA_STRING, this->dataAtom);
    }
};

X11Clipboard::X11Clipboard(void *display)
    : mImpl(new Impl())
{
    mImpl->display = (Display*)display;
    mImpl->clipboardAtom = XInternAtom(mImpl->display, "CLIPBOARD", False);
    mImpl->primaryAtom = XInternAtom(mImpl->display, "PRIMARY", False);
    mImpl->secondaryAtom = XInternAtom(mImpl->display, "SECONDARY", False);
    mImpl->dataAtom = XInternAtom(mImpl->display, "XSEL_DATA", 0);
    mImpl->targetsAtom = XInternAtom(mImpl->display, "TARGETS", False);
    mImpl->textAtom = XInternAtom(mImpl->display, "TEXT", False);
    mImpl->utf8Atom = XInternAtom(mImpl->display, "UTF8_STRING", True);
}

X11Clipboard::~X11Clipboard()
{
}

void X11Clipboard::setActiveWindow(unsigned long w)
{
    mImpl->activeWindow = (Window)w;
}

void X11Clipboard::weAreNoLongerOwner(Selection sel)
{
    if (sel == Selection::kTextSelection) {
        mImpl->weAreSelectionOwner = false;
    } else {
        mImpl->weAreOwner = false;
        mImpl->supportedTypes.clear();
    }
}

bool X11Clipboard::doWeHaveDataForTarget(Selection sel, unsigned int targetAtom)
{
    return (targetAtom == XA_STRING || targetAtom == mImpl->textAtom ||
            targetAtom == mImpl->utf8Atom);
}

std::vector<unsigned char> X11Clipboard::supportedTypes(Selection sel) const
{
    auto *start = (unsigned char*)mImpl->supportedTypes.data();
    auto nBytes = mImpl->supportedTypes.size() * sizeof(Atom);
    return std::vector<unsigned char>(start, start + nBytes);
}

std::vector<unsigned char> X11Clipboard::dataForTarget(Selection sel, unsigned int targetAtom) const
{
    unsigned char *start;
    unsigned int nBytes;
    if (sel == Selection::kTextSelection) {
        start = (unsigned char*)mImpl->utf8Selection.data();
        nBytes = mImpl->utf8Selection.size() * sizeof(char);
    } else {
        start = (unsigned char*)mImpl->utf8Data.data();
        nBytes = mImpl->utf8Data.size() * sizeof(char);
    }
    return std::vector<unsigned char>(start, start + nBytes);
}

bool X11Clipboard::hasString() const
{
    if (mImpl->weAreOwner) {
        return !mImpl->utf8Data.empty();
    } else {
        // TODO: is there a way we can ask if our targets convert withou
        //       needing to copy all the data? Is it even worth it, since
        //       we aren't really memory/cpu bound if we are pasting (we
        //       probably spent a lot longer waiting between Ctrl-down and
        //       V-down than in copying data).
        // Note that this function has the potential to take a while
        return !mImpl->getClipboardText(Selection::kClipboard).empty();
    }
}

std::string X11Clipboard::string() const
{
    if (mImpl->weAreOwner) {
        return mImpl->utf8Data;
    } else {
        // Note that this function has the potential to take a while
        auto data = mImpl->getClipboardText(Selection::kClipboard);
        return std::string(data.begin(), data.end());
    }
}

void X11Clipboard::setString(const std::string& utf8)
{
    mImpl->weAreOwner = true;
    mImpl->utf8Data = utf8;
    mImpl->supportedTypes = { mImpl->targetsAtom, XA_STRING, mImpl->utf8Atom };
    XSetSelectionOwner(mImpl->display, mImpl->clipboardAtom,
                       mImpl->activeWindow, CurrentTime);
}

bool X11Clipboard::supportsX11SelectionString() const { return true; }

void X11Clipboard::setX11SelectionString(const std::string& utf8)
{
    if (!utf8.empty()) {
        mImpl->weAreSelectionOwner = true;
        mImpl->utf8Selection = utf8;
        XSetSelectionOwner(mImpl->display, mImpl->primaryAtom,
                           mImpl->activeWindow, CurrentTime);
    }
}

std::string X11Clipboard::x11SelectionString() const
{
    if (mImpl->weAreSelectionOwner) {
        return mImpl->utf8Selection;
    } else {
        // Note that this function has the potential to take a while
        auto data = mImpl->getClipboardText(Selection::kTextSelection);
        return std::string(data.begin(), data.end());
    }
}

} // namespace uitk

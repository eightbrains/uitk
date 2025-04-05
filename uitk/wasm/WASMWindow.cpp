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

#include "WASMWindow.h"

#include "WASMApplication.h"
#include "WASMCursor.h"
#include "../Application.h"
#include "../TextEditorLogic.h"

#include <iostream>

namespace uitk {

namespace {
static float kCSSDPI = 96.0f;
static int kNoOrigin = -100000;

WASMApplication& getWASMApplication()
{
    return *(WASMApplication*)&Application::instance().osApplication();
}

}  // namespace

struct WASMWindow::Impl
{
    Window::Flags::Value flags;
    float dpi;  // for convenience and speed
    Rect frame;
    DrawContext *dc;  // we do not own; this is a ref
    PicaPt borderWidth;
    std::string title;
    IWindowCallbacks &callbacks;
    TextEditorLogic *textEditor = nullptr;
    Rect textRect;
    bool isVisible = false;
    Point popupMousePos;
    bool ignoreNextLeftMouseUp = false;

    Impl(IWindowCallbacks& callbacks) : callbacks(callbacks) {}

    OSScreen refreshDC(WASMWindow *w)
    {
        auto &app = getWASMApplication();
        this->dc = app.getDrawContext(w);
        auto &osscreen = app.screenOfWindow(w);
        this->dpi = osscreen.dpi;
        return osscreen;
    }
};

WASMWindow::WASMWindow(IWindowCallbacks& callbacks,
                       const std::string& title, int width, int height,
                       Window::Flags::Value flags)
    : WASMWindow(callbacks, title, kNoOrigin, kNoOrigin, width, height, flags)
{
}

WASMWindow::WASMWindow(IWindowCallbacks& callbacks,
                       const std::string& title, int x, int y, int width, int height,
                       Window::Flags::Value flags)
    : mImpl(new Impl(callbacks))
{
    mImpl->flags = flags;
    mImpl->title = title;

    auto &app = getWASMApplication();
    app.registerWindow(this);

    auto osscreen = mImpl->refreshDC(this);

    // Since normal windows take up the full canvas, they do not get a border:
    // this way the web page can decide to put a border around the canvas or
    // not, depending on whether they want the app to stand out or integrate
    // with the page (or, if it is the entire page, then a border is unhelpful).
    int borderPx = 0;
    if (flags == Window::Flags::kNormal) {
        borderPx = 0;
        mImpl->borderWidth = PicaPt::kZero;
    } else {
        borderPx = int(std::round(PicaPt::fromStandardPixels(1).toPixels(mImpl->dpi)));
        mImpl->borderWidth = PicaPt::fromPixels(borderPx, mImpl->dpi);
    }

    const int maxWidth = int(osscreen.desktopFrame.width);
    const int maxHeight = int(osscreen.desktopFrame.height);
    width = std::min(maxWidth, width + 2 * borderPx);
    height = std::min(maxHeight, height + 2 * borderPx);
    if (x == kNoOrigin) {
        x = (maxWidth - width) / 2;
    }
    if (y == kNoOrigin) {
        y = (maxHeight - height) / 2;
    }
    // Note: for kNormal windows, these value will be ignored, and the window
    //       will be set to the full desktop window.
    setOSFrame(x, y, width, height);
}

WASMWindow::~WASMWindow()
{
    mImpl->dc = nullptr;
    getWASMApplication().unregisterWindow(this);
}

Window::Flags::Value WASMWindow::flags() const { return mImpl->flags; }

bool WASMWindow::isShowing() const { return mImpl->isVisible; }

void WASMWindow::show(bool show,
                      std::function<void(const DrawContext&)> onWillShow)
{
    auto &app = getWASMApplication();
    if (show && !mImpl->isVisible) {
        onWillShow(*mImpl->dc);

        // If this is a popup window and we popped up under the mouse
        // (for example, a ComboBox menu), we want to ignore the next left mouse
        // up, so that the user does not need to hold the mouse down until
        // they are finished selecting.
        auto osMousePos = app.currentMouseLocation();
        if ((mImpl->flags & Window::Flags::kPopup) && mImpl->frame.contains(osMousePos)) {
            mImpl->ignoreNextLeftMouseUp = true;
            mImpl->popupMousePos = currentMouseLocation();  // in window coords
        }
    }
    mImpl->isVisible = show;
    app.showWindow(this, show);
}

void WASMWindow::toggleMinimize()
{
    // No-op; cannot minimize in a web page
}

void WASMWindow::toggleMaximize()
{
    // No-op; cannot maximize in a web page
}

void WASMWindow::close()
{
    if (mImpl->flags == Window::Flags::kNormal && getWASMApplication().nOpenNormalWindows() == 1) {
        std::cerr << "[uitk] Cannot close last normal window on this platform" << std::endl;
    } else {
        if (onWindowShouldClose()) {
            onWindowWillClose();
        }
    }
}

void WASMWindow::raiseToTop() const
{
    getWASMApplication().raiseWindow(this);
}

void WASMWindow::setTitle(const std::string& title)
{
    mImpl->title = title;
}

void WASMWindow::setCursor(const Cursor& cursor)
{
    if (auto *osCursor = cursor.osCursor()) {
        osCursor->set(this);
    }
}

Rect WASMWindow::contentRect() const
{
    return Rect(PicaPt::kZero, PicaPt::kZero,
                mImpl->frame.width, mImpl->frame.height);
}


void WASMWindow::setContentSize(const Size& size)
{
    if (mImpl->flags != Window::Flags::kNormal) {
        mImpl->frame.width = size.width + 2.0f * mImpl->borderWidth;
        mImpl->frame.height = size.height + 2.0f * mImpl->borderWidth;
        getWASMApplication().setWindowFrame(this, mImpl->frame);
    }
}

OSRect WASMWindow::osContentRect() const
{
    // no title bar, so content rect is just less the border (if any)
    auto r = contentRect();
    auto os = osFrame();
    return { os.x + r.x.toPixels(mImpl->dpi), os.y + r.y.toPixels(mImpl->dpi),
             r.width.toPixels(mImpl->dpi), r.height.toPixels(mImpl->dpi) };
}

float WASMWindow::dpi() const { return mImpl->dpi; }

OSRect WASMWindow::osFrame() const
{
    OSRect r;
    r.x = mImpl->frame.x.toPixels(mImpl->dpi);
    r.y = mImpl->frame.y.toPixels(mImpl->dpi);
    r.width = mImpl->frame.width.toPixels(mImpl->dpi);
    r.height = mImpl->frame.height.toPixels(mImpl->dpi);
    return r;
}

void WASMWindow::setOSFrame(float x, float y, float width, float height)
{
    // If this is a normal window, make it the size of the display (which in
    // this case is the HTML canvas). Since there is not a title bar, and no
    // way to move the window, the only sensible size is the entire canvas.
    // However, dialog and menu windows obviously have good reason to specify
    // their size.
    if (mImpl->flags == Window::Flags::kNormal) {
        auto &osscreen = getWASMApplication().screenOfWindow(this);
        x = 0;
        y = 0;
        width = int(osscreen.desktopFrame.width);
        height = int(osscreen.desktopFrame.height);
    }
    mImpl->frame = Rect::fromPixels(x, y, width, height, mImpl->dpi);
    getWASMApplication().setWindowFrame(this, mImpl->frame);
}

OSScreen WASMWindow::osScreen() const
{
    return getWASMApplication().screenOfWindow(this);
}

PicaPt WASMWindow::borderWidth() const
{
    return PicaPt::kZero;
}

void WASMWindow::postRedraw() const
{
    getWASMApplication().postRedraw(this);
}

void WASMWindow::beginModalDialog(OSWindow *w)
{
    auto thisOSF = osFrame();
    auto dlgOSF = w->osFrame();
    dlgOSF.x = 0.5f * (thisOSF.width - dlgOSF.width);
    dlgOSF.y = 0.5f * (thisOSF.height - dlgOSF.height);
    w->setOSFrame(dlgOSF.x, dlgOSF.y, dlgOSF.width, dlgOSF.height);
    w->show(true, [](const DrawContext&){});
}

void WASMWindow::endModalDialog(OSWindow *w)
{
    w->show(false, [](const DrawContext&){});
}

Point WASMWindow::currentMouseLocation() const
{
    auto osPos = getWASMApplication().currentMouseLocation();
    return osPos - mImpl->frame.upperLeft();
}

void* WASMWindow::nativeHandle() { return this; }

IWindowCallbacks& WASMWindow::callbacks() { return mImpl->callbacks; }

void WASMWindow::callWithLayoutContext(std::function<void(const DrawContext&)> f)
{
    f(*mImpl->dc);
}

void WASMWindow::setTextEditing(TextEditorLogic *te, const Rect& frame)
{
    mImpl->textEditor = te;
    mImpl->textRect = frame;
    getWASMApplication().setTextEditing(te, frame.translated(mImpl->frame.upperLeft()));
}

void WASMWindow::setNeedsAccessibilityUpdate()
{
    // No-op; WebAssembly does not support accessibility
}

void WASMWindow::setAccessibleElements(const std::vector<AccessibilityInfo>& elements)
{
    std::cerr << "[error] WASMWindow::setAccessibleElements() not implemented" << std::endl;
}

void WASMWindow::onResize()
{
    mImpl->refreshDC(this);
    mImpl->callbacks.onResize(*mImpl->dc);
}

void WASMWindow::onLayout()
{
    mImpl->callbacks.onLayout(*mImpl->dc);
}

void WASMWindow::onDraw()
{
    bool drawsBorder = (mImpl->borderWidth > PicaPt::kZero);

    if (!drawsBorder) {
        mImpl->callbacks.onDraw(*mImpl->dc);
    } else {
        auto &params = Application::instance().theme()->params();
        auto dx = 0.5f * mImpl->borderWidth;
        auto r = mImpl->frame.insetted(dx, dx);
        auto roundedBorder = (params.borderRadius > PicaPt::kZero);
        // Window::draw() will clip to a rect, so if we are a rounded window
        // we need to pre-clip to the rounded rect so that any fill does not
        // bleed into the corners. (We need to do this here because Window
        // should not know how the OS [which is us in this case] is drawing
        // the window.)
        // Design Question: should we always do this beginDraw() and remove
        //   Window's drawContextMightBeShared property and always do the
        //   frame offset and clipping (if necessary) here? Seems cleaner for
        //   Window but dirtier here; now we can at least pretend that it's
        //   "clean" because the "dirty" path is only for dialogs.
        if (roundedBorder) {
            // This is somewhat hacky: to clip, we need to be in a beginDraw,
            // but Window::onDraw() will also--and correctly--call beginDraw.
            // We rely on the WASM version of libnativedraw not asserting
            // if beginDraw() is called twice. There should be a comment to
            // this effect in the WASM implementation of beginDraw().
            mImpl->dc->beginDraw();
            mImpl->dc->save();
            auto path = mImpl->dc->createBezierPath();
            path->addRoundedRect(r, params.borderRadius);
            mImpl->dc->clipToPath(path);
        }
        mImpl->callbacks.onDraw(*mImpl->dc);
        // We do NOT call restore() or endDraw(), because onDraw() already
        // called endDraw(), which also restores anything unrestored (as is
        // necessary to make sure the canvas context state is properly
        // restored.

        // Draw the border last, so it is on top
        auto borderColor = params.nonNativeMenuSeparatorColor;
        mImpl->dc->setStrokeColor(borderColor);
        mImpl->dc->setStrokeWidth(mImpl->borderWidth);
        if (!roundedBorder) {
            mImpl->dc->drawRect(r, kPaintStroke);
        } else {
            mImpl->dc->drawRoundedRect(r, params.borderRadius, kPaintStroke);
        }
    }
}

void WASMWindow::onMouse(const MouseEvent& e)
{
    // If we requested to ignore the next left mouse up (so that a popup, like
    // a ComboBox menu, does not close on the release of the click that
    // initiated it), handle that.
    if (mImpl->ignoreNextLeftMouseUp) {
        if (e.type == MouseEvent::Type::kButtonUp && e.button.button == MouseButton::kLeft) {
            mImpl->ignoreNextLeftMouseUp = false;
            return;
        } else if (e.type == MouseEvent::Type::kDrag) {
            // If we've dragged past a threshold, do not ignore the next mouse
            // up, user has decide to keep holding and drag.
            auto dXY = (e.pos - mImpl->popupMousePos);
            auto dist = dXY.x * dXY.x + dXY.y * dXY.y;
            if (std::abs(dist.asFloat()) > PicaPt::fromStandardPixels(3).asFloat()) {
                mImpl->ignoreNextLeftMouseUp = false;
            }
        }
    }

    mImpl->callbacks.onMouse(e);
}

void WASMWindow::onKey(const KeyEvent& e)
{
    mImpl->callbacks.onKey(e);
}

void WASMWindow::onText(const TextEvent& e)
{
    if (mImpl->textEditor) {
        mImpl->textEditor->setIMEConversion(TextEditorLogic::IMEConversion());
    }
    mImpl->callbacks.onText(e);
}

void WASMWindow::onActivated()
{
    mImpl->callbacks.onActivated(currentMouseLocation());
}

void WASMWindow::onDeactivated()
{
    mImpl->callbacks.onDeactivated();
}

bool WASMWindow::onWindowShouldClose()
{
    return mImpl->callbacks.onWindowShouldClose();
}

void WASMWindow::onWindowWillClose()
{
    mImpl->callbacks.onWindowWillClose();
}

}  // namespace uitk

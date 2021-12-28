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

// No includes, this is included directly as a .cpp file, which avoids the
// hassle of a bunch of mostly identical header files.

namespace cursor {

class CursorObj : public Widget
{
    using Super = Widget;
public:
    explicit CursorObj(const Cursor& cursor, const std::string& info = "")
        : mCursor(cursor), mInfo(info)
    {
    }

    void mouseEntered() override
    {
        Super::mouseEntered();
        window()->setCursor(mCursor);
    }

    void mouseExited() override
    {
        Super::mouseExited();
        window()->setCursor(Cursor::arrow());
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);
        if (!mInfo.empty()) {
            context.dc.setFillColor(borderColor());
            context.dc.drawText(mInfo.c_str(), bounds(), Alignment::kCenter,
                                context.theme.params().labelFont, kPaintFill);
        }
    }

protected:
    std::string mInfo;
    Cursor mCursor;
};

class DraggableObj : public CursorObj
{
    using Super = CursorObj;
public:
    explicit DraggableObj(const Cursor& cursor, const Cursor& dragCursor, const std::string& info = "")
        : CursorObj(cursor, info), mDragCursor(dragCursor)
    {
        auto addGrab = [this](const Cursor& cursor) -> CursorObj* {
            auto *grab = new CursorObj(cursor);
            grab->setBorderWidth(PicaPt(1));
            grab->setBorderColor(Color(0.5f, 0.5f, 0.5f));
            addChild(grab);
            return grab;
        };
        mULGrab = addGrab(Cursor::resizeNWSE());
        mUpperGrab = addGrab(Cursor::resizeUpDown());
        mURGrab = addGrab(Cursor::resizeNESW());
        mRightGrab = addGrab(Cursor::resizeLeftRight());
        mLRGrab = addGrab(Cursor::resizeNWSE());
        mLowerGrab = addGrab(Cursor::resizeUpDown());
        mLLGrab = addGrab(Cursor::resizeNESW());
        mLeftGrab = addGrab(Cursor::resizeLeftRight());
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        auto &r = bounds();
        mUpperGrab->setFrame(Rect(r.midX() - 0.5f * em, r.y, em, em));
        mLowerGrab->setFrame(Rect(r.midX() - 0.5f * em, r.maxY() - em, em, em));
        mLeftGrab->setFrame(Rect(r.minX(), r.midY() - 0.5f * em, em, em));
        mRightGrab->setFrame(Rect(r.maxX() - em, r.midY() - 0.5f * em, em, em));
        mULGrab->setFrame(Rect(r.x, r.y, em, em));
        mURGrab->setFrame(Rect(r.maxX() - em, r.y, em, em));
        mLRGrab->setFrame(Rect(r.maxX() - em, r.maxY() - em, em, em));
        mLLGrab->setFrame(Rect(r.x, r.maxY() - em, em, em));
        Super::layout(context);
    }

    Widget::EventResult mouse(const MouseEvent& e) override
    {
        auto retval = Super::mouse(e);

        if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft
            && e.button.nClicks == 1) {
            window()->setCursor(mDragCursor);
            mOriginAtClick = frame().upperLeft();
            mClickPos = e.pos;
            return EventResult::kConsumed;
        } else if (e.type == MouseEvent::Type::kButtonUp) {
            window()->setCursor(mCursor);
            return EventResult::kConsumed;
        } else if (e.type == MouseEvent::Type::kDrag && e.button.button == MouseButton::kLeft) {
            auto dxy = e.pos - mClickPos;
            setFrame(Rect(mOriginAtClick.x + dxy.x, mOriginAtClick.y + dxy.y,
                          frame().width, frame().height));
            setNeedsDraw();
            return EventResult::kConsumed;
        } else {
            return retval;
        }
    }

protected:
    CursorObj *mULGrab;
    CursorObj *mUpperGrab;
    CursorObj *mURGrab;
    CursorObj *mRightGrab;
    CursorObj *mLRGrab;
    CursorObj *mLowerGrab;
    CursorObj *mLLGrab;
    CursorObj *mLeftGrab;
    Cursor mDragCursor;
    Point mOriginAtClick;
    Point mClickPos;
};

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        mEdit = new StringEdit();
        mEdit->setPlaceholderText("StringEdit for I-beam");
        addChild(mEdit);

        mForbidden = new CursorObj(Cursor::forbidden(), "forbidden");
        mForbidden->setBorderColor(Color(1.0f, 0.0f, 0.0f));
        mForbidden->setBorderWidth(PicaPt(1));
        addChild(mForbidden);

        mCrosshair = new CursorObj(Cursor::crosshair(), "crosshair");
        mCrosshair->setBorderColor(Color(0.0f, 1.0f, 0.25f));
        mCrosshair->setBorderWidth(PicaPt(1));
        addChild(mCrosshair);

        mPointingHand = new CursorObj(Cursor::pointingHand(), "pointing hand");
        mPointingHand->setBorderColor(Color(0.3f, 0.3f, 1.0f));
        mPointingHand->setBorderWidth(PicaPt(1));
        addChild(mPointingHand);

        mMainObj = new DraggableObj(Cursor::openHand(), Cursor::closedHand(),
                                    "hand\n(click to drag)");
        addChild(mMainObj);
    }

    void layout(const LayoutContext& context) override
    {
        auto em = context.theme.params().labelFont.pointSize();
        mEdit->setFrame(Rect(em, em, 12.0f * em, mEdit->preferredSize(context).height));
        mForbidden->setFrame(Rect(em, 3.0f * em, 7.0f * em, 3.0f * em));
        mCrosshair->setFrame(Rect(mForbidden->frame().maxX() + em, 3.0f * em, 7.0f * em, 3.0f * em));
        mPointingHand->setFrame(Rect(mCrosshair->frame().maxX() + em, 3.0f * em, 7.0f * em, 3.0f * em));
        mMainObj->setFrame(Rect(10.f * em, 10.f * em, 10.0f * em, 7.5 * em));
        mMainObj->setBorderColor(context.theme.params().textColor);
        mMainObj->setBorderWidth(PicaPt(1));
        Super::layout(context);
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);
    }

private:
    StringEdit *mEdit;
    CursorObj *mForbidden;
    CursorObj *mCrosshair;
    CursorObj *mPointingHand;
    CursorObj *mMainObj;
};

}  // namespace cursor

//-----------------------------------------------------------------------------
// Copyright 2021 - 2022 Eight Brains Studios, LLC
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

namespace gradients {

enum GradientType { kGradientLinear, kGradientRadial };

struct Direction
{
    float startRX;
    float startRY;
    float endRX;
    float endRY;
};

struct GradientInfo
{
    GradientType type;
    std::vector<Gradient::Stop> stops;
    Direction dir;
};

PicaPt calcDistance(const Point& p1, const Point& p2)
{
    auto v = p2 - p1;
    return PicaPt(std::sqrt((v.x * v.x + v.y * v.y).asFloat()));
}

class Canvas : public Widget
{
    using Super = Widget;
public:
    Canvas()
    {
        setBorderWidth(PicaPt::fromStandardPixels(1.0f));
        setBorderColor(Color(0.5f, 0.5f, 0.5f));
    }

    void setGradient(const GradientInfo& info)
    {
        mGradient = info;
        setNeedsDraw();
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);

        PicaPt size = context.dc.roundToNearestPixel(std::min(bounds().width, bounds().height));
        PicaPt x = context.dc.roundToNearestPixel(0.5f * (bounds().width - size));
        PicaPt y = context.dc.roundToNearestPixel(0.5f * (bounds().height - size));

        auto &gradient = context.dc.getGradient(mGradient.stops);

        PicaPt rectSize = context.dc.roundToNearestPixel(0.2f * size);
        Point start(x + rectSize * mGradient.dir.startRX, y + rectSize * mGradient.dir.startRY);
        Point end(x + rectSize * mGradient.dir.endRX, y + rectSize * mGradient.dir.endRY);
        auto path = context.dc.createBezierPath();
        path->addRect(Rect(x, y, rectSize, rectSize));
        if (mGradient.type == kGradientLinear) {
            context.dc.drawLinearGradientPath(path, gradient, start, end);
        } else {
            Point center(x + 0.5f * rectSize, y + 0.5f * rectSize);
            context.dc.drawRadialGradientPath(path, gradient, center,
                                              calcDistance(center, start), calcDistance(center, end));
        }

        auto radius = 0.5f * rectSize;
        x = context.dc.roundToNearestPixel(bounds().maxX() - 1.2f * 2.0f * radius);
        y = context.dc.roundToNearestPixel(bounds().maxY() - 1.2f * 2.0f * radius);
        start = Point(x + 2.0f * radius * mGradient.dir.startRX, y + 2.0f * radius * mGradient.dir.startRY);
        end = Point(x + 2.0f * radius * mGradient.dir.endRX, y + 2.0f * radius * mGradient.dir.endRY);
        path = createStar(context.dc, 10, radius, Point(x + radius, y + radius));
        if (mGradient.type == kGradientLinear) {
            context.dc.drawLinearGradientPath(path, gradient, start, end);
        } else {
            Point center(x + radius, y + radius);
            context.dc.drawRadialGradientPath(path, gradient, center,
                                              calcDistance(center, start), calcDistance(center, end));
        }
    }

protected:
    std::shared_ptr<BezierPath> createStar(DrawContext& dc, int nPts, const PicaPt& radius, const Point& center)
    {
        auto star = dc.createBezierPath();
        PicaPt outer = radius;
        PicaPt inner = 0.666f * radius;
        PicaPt tanLen = 0.2f * radius;
        float dtheta = 2.0f * 3.141592f / float(nPts);
        star->moveTo(Point(outer + center.x, center.y));
        for (int i = 0;  i < nPts;  ++i) {
            float thetaOuter = float(i) * dtheta;
            float thetaInner = thetaOuter + 0.5f * dtheta;
            float thetaOuter2 = float(i + 1) * dtheta;
            float outCosT = std::cos(thetaOuter);
            float outSinT = std::sin(thetaOuter);
            float inCosT = std::cos(thetaInner);
            float inSinT = std::sin(thetaInner);
            float out2CosT = std::cos(thetaOuter2);
            float out2SinT = std::sin(thetaOuter2);
            auto outX = outer * outCosT + center.x;
            auto outY = outer * outSinT + center.y;
            auto inX = inner * inCosT + center.x;
            auto inY = inner * inSinT + center.y;
            auto outX2 = outer * out2CosT + center.x;
            auto outY2 = outer * out2SinT + center.y;
            auto tangent_x = tanLen * inSinT;
            auto tangent_y = tanLen * -inCosT;
            star->cubicTo(Point(outX, outY),
                          Point(inX + tangent_x, inY + tangent_y),
                          Point(inX, inY));
            star->cubicTo(Point(inX - tangent_x, inY - tangent_y),
                          Point(outX2, outY2),
                          Point(outX2, outY2));
        }
        star->close();
        return star;
    }

private:
    GradientInfo mGradient;
};

class LinearDirectionEditor : public Widget
{
    using Super = Widget;
public:
    LinearDirectionEditor()
    {
        mDirection.startRX = 0.0f;
        mDirection.startRY = 0.0f;
        mDirection.endRX = 1.0f;
        mDirection.endRY = 1.0f;
    }

    void setGradientType(GradientType type)
    {
        mGradientType = type;
        setNeedsDraw();
    }

    const Direction& direction() const { return mDirection; }
    LinearDirectionEditor* setDirection(const Direction& d)
    {
        mDirection = d;
        return this;
    }

    LinearDirectionEditor* setOnChanged(std::function<void(LinearDirectionEditor*)> onChanged)
    {
        mOnChanged = onChanged;
        return this;
    }

    Widget::EventResult mouse(const MouseEvent& e) override
    {
        auto oldHighlight = mHighlightId;
        auto oldGrab = mGrabId;

        auto d1 = e.pos - mDrawInfo.start;
        auto d2 = e.pos - mDrawInfo.end;
        if (d1.x * d1.x + d1.y * d1.y <= kRadius * kRadius) {
            mHighlightId = kStartId;
            if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
                mGrabId = kStartId;
            } else if (e.type != MouseEvent::Type::kDrag) {
                mGrabId = kNoneId;
            }
        } else if (d2.x * d2.x + d2.y * d2.y <= kRadius * kRadius) {
            mHighlightId = kEndId;
            if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
                mGrabId = kEndId;
            } else if (e.type != MouseEvent::Type::kDrag) {
                mGrabId = kNoneId;
            }
        } else if (e.type != MouseEvent::Type::kDrag) {
            mHighlightId = kNoneId;
            mGrabId = kNoneId;
        }

        if (e.type == MouseEvent::Type::kDrag) {
            if (mGrabId == kStartId) {
                mDirection.startRX = (e.pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width;
                mDirection.startRY = (e.pos.y - mDrawInfo.rect.x) / mDrawInfo.rect.height;
            } else if (mGrabId == kEndId) {
                mDirection.endRX = (e.pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width;
                mDirection.endRY = (e.pos.y - mDrawInfo.rect.x) / mDrawInfo.rect.height;
            }
            if (mOnChanged) { mOnChanged(this); }
            setNeedsDraw();
        }

        if (oldHighlight != mHighlightId || oldGrab != mGrabId) {
            setNeedsDraw();
        }

        return Widget::EventResult::kIgnored;
    }

    Size preferredSize(const LayoutContext &context) const override
    {
        auto em = context.theme.params().labelFont.pointSize();
        return Size(10.0f * em, 10.0f * em);
    }

    void draw(UIContext& context) override
    {
        const float kArrowHalfWidth = 3.5f;

        Super::draw(context);

        auto borderWidth = PicaPt::fromStandardPixels(1);
        mDrawInfo = calcDrawInfo(context.dc);
        auto &info = mDrawInfo;

        auto fg = Color(context.theme.params().textColor, 1.0f);

        context.dc.setStrokeColor(Color(0.5f, 0.5f, 0.5f));
        context.dc.setStrokeWidth(borderWidth);
        context.dc.drawRect(bounds().insetted(0.5f * borderWidth, 0.5f * borderWidth), kPaintStroke);

        context.dc.setStrokeColor(fg);
        context.dc.setStrokeDashes({ PicaPt(2), PicaPt(2) }, PicaPt::kZero);
        context.dc.drawRect(info.rect, kPaintStroke);

        if (mGradientType) {
            context.dc.setStrokeDashes({ PicaPt(1), PicaPt(1) }, PicaPt::kZero);

            auto center = info.rect.center();
            auto v = info.start - center;
            PicaPt radius1(std::sqrt((v.x * v.x + v.y * v.y).asFloat()));
            v = info.end - center;
            PicaPt radius2(std::sqrt((v.x * v.x + v.y * v.y).asFloat()));
            context.dc.drawEllipse(Rect(center.x - radius1, center.y - radius1,
                                        2.0f * radius1, 2.0f * radius1), kPaintStroke);
            context.dc.drawEllipse(Rect(center.x - radius2, center.y - radius2,
                                        2.0f * radius2, 2.0f * radius2), kPaintStroke);
        }

        context.dc.setStrokeDashes({}, PicaPt::kZero);

        context.dc.drawLines({ info.start, info.end });
        auto arrowhead = context.dc.createBezierPath();
        arrowhead->moveTo(info.end - kRadius.asFloat() * info.toEndUnit);
        arrowhead->lineTo(info.end - 3.0f * kArrowHalfWidth * info.toEndUnit - kArrowHalfWidth * Point(info.toEndUnit.y, -info.toEndUnit.x));
        arrowhead->lineTo(info.end - 3.0f * kArrowHalfWidth * info.toEndUnit - kArrowHalfWidth * Point(-info.toEndUnit.y, info.toEndUnit.x));
        arrowhead->close();
        context.dc.setFillColor(fg);
        context.dc.drawPath(arrowhead, kPaintFill);

        auto setColor = [this, &context](GrabId id) {
            if (mGrabId == id) {
                context.dc.setStrokeColor(context.theme.params().textColor);
                context.dc.setFillColor(context.theme.params().accentColor);
            } else if (mHighlightId == id) {
                context.dc.setStrokeColor(context.theme.params().accentColor);
                context.dc.setFillColor(Color::kWhite);
            } else {
                context.dc.setStrokeColor(context.theme.params().textColor);
                context.dc.setFillColor(Color::kWhite);
            }
        };

        setColor(kStartId);
        context.dc.drawEllipse(Rect(info.start.x - kRadius, info.start.y - kRadius,
                                    2.0f * kRadius, 2.0f * kRadius), kPaintStrokeAndFill);
        setColor(kEndId);
        context.dc.drawEllipse(Rect(info.end.x - kRadius, info.end.y - kRadius,
                                    2.0f * kRadius, 2.0f * kRadius), kPaintStrokeAndFill);
    }

protected:
    const PicaPt kRadius = PicaPt(3.0f);

    struct DrawInfo {
        Rect rect;
        Point start;
        Point end;
        Point toEndUnit;
    };

    DrawInfo calcDrawInfo(const DrawContext& dc)
    {
        auto size = std::min(bounds().width, bounds().height);
        auto inset = dc.roundToNearestPixel(0.2f * size);
        auto r = bounds().insetted(inset, inset);
        Point start(r.x + mDirection.startRX * r.width, r.y + mDirection.startRY * r.height);
        Point end(r.x + mDirection.endRX * r.width, r.y + mDirection.endRY * r.height);

        auto toEnd = end - start;
        float distPica = (toEnd.x * toEnd.x + toEnd.y * toEnd.y).asFloat();
        distPica = std::sqrt(distPica);
        Point dir(toEnd.x / distPica, toEnd.y / distPica);

        return { r, start, end, dir };
    }

private:
    GradientType mGradientType = kGradientLinear;
    Direction mDirection;
    std::function<void(LinearDirectionEditor*)> mOnChanged;
    DrawInfo mDrawInfo;

    enum GrabId { kNoneId = 0, kStartId = 1, kEndId = 2 };
    GrabId mHighlightId = kNoneId;
    GrabId mGrabId = kNoneId;
};

class OneStopEdit : public HLayout
{
    using Super = HLayout;
public:
    OneStopEdit(const Color& c, float loc)
    {
        mColor = (new ColorEdit())->setColor(c);
        mLocation = (new NumberEdit())->setValue(double(loc))->setLimits(0.0, 1.0, 0.01);
        mRemove = (new Button(Theme::StandardIcon::kRemoveCircle))
                        ->setDrawStyle(Button::DrawStyle::kNoDecoration);
        addChild(mColor);
        addChild(mLocation);
        addChild(mRemove);

        mColor->setOnColorChanged([this](ColorEdit *ce) {
            if (mOnChanged) { mOnChanged(this); }
        });
        mLocation->setOnValueChanged([this](NumberEdit *ne) {
            if (mOnChanged) { mOnChanged(this); }
        });
        mRemove->setOnClicked([this](Button*) {
            if (mOnRemove) { mOnRemove(this); }
        });
    }

    const Color& color() const { return mColor->color(); }
    OneStopEdit* setColor(const Color& c) { mColor->setColor(c); return this; }

    float location() const { return mLocation->doubleValue(); }
    OneStopEdit* setLocation(float v) { mLocation->setValue(double(v)); return this; }

    OneStopEdit* setCanRemove(bool can)
    {
        mRemove->setEnabled(can);
        return this;
    }

    OneStopEdit* setOnChanged(std::function<void(OneStopEdit*)> onChanged)
    {
        mOnChanged = onChanged;
        return this;
    }

    OneStopEdit* setOnRequestRemove(std::function<void(OneStopEdit*)> onRemove)
    {
        mOnRemove = onRemove;
        return this;
    }

private:
    ColorEdit *mColor;
    NumberEdit *mLocation;
    Button *mRemove;
    std::function<void(OneStopEdit*)> mOnChanged;
    std::function<void(OneStopEdit*)> mOnRemove;
};

class StopEditor : public VLayout
{
    using Super = VLayout;
public:
    StopEditor()
    {
        setStops({});  // will get the default
    }

    const std::vector<Gradient::Stop>& stops() const { return mStops; }
    StopEditor* setStops(const std::vector<Gradient::Stop>& stops)
    {
        mStops = stops;
        if (mStops.empty()) {
            mStops = { { Color::kRed, 0.0f}, { Color::kYellow, 1.0f} };
        } else if (mStops.size() == 1) {
            if (mStops[0].location > 0.0f) {
                mStops.insert(mStops.begin(), { Color::kRed, 0.0f });
            } else {
                mStops.push_back({ Color::kRed, 1.0f });
            }
        }
        updateUI();
        return this;
    }

    void removeStop(int idx)
    {
        mStops.erase(mStops.begin() + idx);
        removeChild(children()[idx]);
        updateModel();
        updateUI();
    }

    void addStop()
    {
        float adjust = float(mStops.size() - 1) / float(mStops.size());
        for (auto &stop : mStops) {
            stop.location *= adjust;
        }
        mStops.push_back({ Color::kRed, 1.0f });
        updateUI();
    }

    StopEditor* setOnChanged(std::function<void(StopEditor*)> onChanged)
    {
        mOnChanged = onChanged;
        return this;
    }

protected:
    void updateModel()
    {
        auto &childs = children();
        if (mStops.size() != childs.size()) {
            mStops.resize(childs.size());
        }
        for (size_t i = 0;  i < childs.size();  ++i) {
            mStops[i].color = ((OneStopEdit*)childs[i])->color();
            mStops[i].location = ((OneStopEdit*)childs[i])->location();
        }
    }

    void updateUI()
    {
        assert(mStops.size() >= 2);
        auto nAdd = mStops.size() - children().size();
        for (size_t i = 0;  i < nAdd;  ++i) {
            auto *edit = new OneStopEdit(Color::kRed, 0.0f);
            edit->setOnChanged([this](OneStopEdit *e) {
                updateModel();
                if (mOnChanged) { mOnChanged(this); }
            });
            auto idx = children().size();
            edit->setOnRequestRemove([this, idx /* copy */](OneStopEdit *e) {
                removeStop(idx);
            });
            addChild(edit);
        }
        while (children().size() > mStops.size()) {
            removeChild(children()[mStops.size()]);
        }

        for (size_t i = 0;  i < mStops.size();  ++i) {
            auto *stop = (OneStopEdit*)(children()[i]);
            stop->setColor(mStops[i].color);
            stop->setLocation(mStops[i].location);
        }

        for (auto *child : children()) {
            ((OneStopEdit*)child)->setCanRemove(mStops.size() > 2);
        }
    }

private:
    std::vector<Gradient::Stop> mStops;
    std::function<void(StopEditor*)> mOnChanged;
};

class Panel : public Widget
{
    using Super = Widget;
public:
    Panel()
    {
        mModel = {
            kGradientLinear,
            { { Color::kRed, 0.0f },
              { Color::kYellow, 1.0f }
            },
            {
                0.0f, 0.0f,
                1.0f, 1.0f
            }
        };

        auto *layout = new VLayout({
            new HLayout({
                mCanvas = new Canvas(),
                new VLayout({
                    new HLayout({
                        new HLayout::Stretch(),
                        mGradientType = (new SegmentedControl())
                                            ->addItem("Linear")
                                            ->addItem("Radial")
                                            ->setAction(SegmentedControl::Action::kSelectOne),
                        new HLayout::Stretch()
                    }),
                    new HLayout({
                        (new VLayout({
                            mStops = new StopEditor(),
                            // Functionally this would be better off in StopEditor,
                            // but that means more hassle because StopEditor could not just
                            // iterate over its children and assume that they are all stops.
                            mAddStop = (new Button(Theme::StandardIcon::kAddCircle))
                                            ->setDrawStyle(Button::DrawStyle::kNoDecoration),
                            new VLayout::Stretch()
                        }))->setSpacingEm(0.5f),
                        new VLayout({
                            mDirEdit = new LinearDirectionEditor(),
                            new VLayout::Stretch()
                        }),
                    }),
                })
            }),
            new VLayout::Stretch()
        });
        layout->setMarginsEm(1.0f);
        addChild(layout);

        mGradientType->setSegmentOn(0, true);
        mGradientType->setOnClicked([this](int idx) {
            updateModel();
            updateDraw();
        });

        mAddStop->setOnClicked([this](Button*) {
            mStops->addStop();
            updateModel();
            updateDraw();
        });

        mStops->setOnChanged([this](StopEditor *se) {
            updateModel();
            updateDraw();
        });

        mDirEdit->setOnChanged([this](LinearDirectionEditor *lde) {
            updateModel();
            updateDraw();
        });

        updateDraw();
    }

    void updateModel()
    {
        mModel.type = (mGradientType->isSegmentOn(0) ? kGradientLinear : kGradientRadial);
        mModel.stops = mStops->stops();
        mModel.dir = mDirEdit->direction();
    }

    void updateDraw()
    {
        mCanvas->setGradient(mModel);
    }

    void layout(const LayoutContext& context) override
    {
        children()[0]->setFrame(bounds());
        Super::layout(context);
    }

private:
    GradientInfo mModel;
    Canvas *mCanvas;  // ref, superclass owns
    SegmentedControl *mGradientType;
    StopEditor *mStops;  // ref, superclass owns
    Button *mAddStop;  // ref, superclass owns
    LinearDirectionEditor *mDirEdit;
};

} // namespace gradients

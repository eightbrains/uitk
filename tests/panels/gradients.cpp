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

static const float kGradientEditSizeEm = 15.0f;
static const PicaPt kEditHandleRadius = PicaPt(3.0f);

enum GradientType { kGradientLinear, kGradientRadial };

struct Direction
{
    float startRX;
    float startRY;
    float endRX;
    float endRY;
    float centerRX = 0.0f;  // only used for radial gradients
    float centerRY = 0.0f;
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

        auto &gradient = context.dc.getGradient(mGradient.stops);

        auto drawPath = [&context, &gradient](std::shared_ptr<BezierPath> path,
                                              const GradientInfo& config,
                                              const PicaPt& x, const PicaPt& y, const PicaPt& size) {
            Point start(x + size * config.dir.startRX, y + size * config.dir.startRY);
            Point end(x + size * config.dir.endRX, y + size * config.dir.endRY);
            if (config.type == kGradientLinear) {
                context.dc.drawLinearGradientPath(path, gradient, start, end);
            } else {
                Point center(x + size * config.dir.centerRX, y + size * config.dir.centerRY);
                context.dc.drawRadialGradientPath(path, gradient, center,
                                                  calcDistance(center, start), calcDistance(center, end));
            }
        };

        const PicaPt size = context.dc.roundToNearestPixel(std::min(bounds().width, bounds().height));
        const PicaPt squareSize = context.dc.roundToNearestPixel(0.3f * size);
        const PicaPt rectWidth = context.dc.roundToNearestPixel(0.5f * size);
        const PicaPt rectHeight = context.dc.roundToNearestPixel(0.3f * size);

        PicaPt x = context.dc.roundToNearestPixel(0.05f * size + 0.5f * (rectWidth - squareSize));
        PicaPt y = context.dc.roundToNearestPixel(0.05f * size);
        auto path = context.dc.createBezierPath();
        path->addRect(Rect(x, y, squareSize, squareSize));
        drawPath(path, mGradient, x, y, squareSize);

        x = context.dc.roundToNearestPixel(0.05f * size);
        y = context.dc.roundToNearestPixel(0.65f * size);
        path = context.dc.createBezierPath();
        path->addRect(Rect(x, y, rectWidth, rectHeight));
        PicaPt gradientSize = std::min(rectWidth, rectHeight);
        drawPath(path, mGradient,
                 x + 0.5f * (rectWidth - gradientSize),
                 y + 0.5f * (rectHeight - gradientSize),
                 gradientSize);

        x = context.dc.roundToNearestPixel(bounds().maxX() - 0.05f * size - squareSize);
        y = context.dc.roundToNearestPixel(0.05f * size);
        path = context.dc.createBezierPath();
        path->addEllipse(Rect(x, y, squareSize, squareSize));
        drawPath(path, mGradient, x, y, squareSize);

        auto radius = 0.5f * squareSize;
        x = context.dc.roundToNearestPixel(bounds().maxX() - 0.05f * size - squareSize);
        y = context.dc.roundToNearestPixel(bounds().maxY() - 0.05f * size - squareSize);
        path = createStar(context.dc, 10, radius, Point(x + radius, y + radius));
        drawPath(path, mGradient, x, y, squareSize);
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

class DirectionEditor : public Widget
{
    using Super = Widget;
public:
    explicit DirectionEditor(GradientType t, const Direction& d)
        : mDirection(d)
    {
        mGradientType = t;
    }

    const Direction& direction() const { return mDirection; }
    DirectionEditor* setDirection(const Direction& d)
    {
        mDirection = d;
        return this;
    }

    DirectionEditor* setOnChanged(std::function<void(DirectionEditor*)> onChanged)
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
        auto d3 = e.pos - mDrawInfo.center;
        auto radius2 = kEditHandleRadius * kEditHandleRadius;
        if (d1.x * d1.x + d1.y * d1.y <= radius2) {
            mHighlightId = kStartId;
            if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
                mGrabId = kStartId;
            } else if (e.type != MouseEvent::Type::kDrag) {
                mGrabId = kNoneId;
            }
        } else if (d2.x * d2.x + d2.y * d2.y <= radius2) {
            mHighlightId = kEndId;
            if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
                mGrabId = kEndId;
            } else if (e.type != MouseEvent::Type::kDrag) {
                mGrabId = kNoneId;
            }
        } else if (mGradientType == kGradientRadial && d3.x * d3.x + d3.y * d3.y <= radius2) {
            mHighlightId = kCenterId;
            if (e.type == MouseEvent::Type::kButtonDown && e.button.button == MouseButton::kLeft) {
                mGrabId = kCenterId;
            } else if (e.type != MouseEvent::Type::kDrag) {
                mGrabId = kNoneId;
            }
        } else if (e.type != MouseEvent::Type::kDrag) {
            mHighlightId = kNoneId;
            mGrabId = kNoneId;
        }

        if (e.type == MouseEvent::Type::kDrag) {
            onMouseMoved(mGrabId, e.pos);
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
        return Size(kGradientEditSizeEm * em, kGradientEditSizeEm * em);
    }

protected:
    enum GrabId { kNoneId = 0, kStartId = 1, kEndId = 2, kCenterId = 3 };

    struct DrawInfo {
        Rect rect;
        Point start;
        Point end;
        Point toEndUnit;
        Point center;  // only used in radial mode
    };
    DrawInfo mDrawInfo;
    Direction mDirection;

    virtual void onMouseMoved(GrabId id, const Point& pos) = 0;

    void drawCommon(UIContext& context, const DrawInfo& info)
    {
        const float kArrowHalfWidth = 3.5f;

        auto borderWidth = PicaPt::fromStandardPixels(1);
        auto fg = Color(context.theme.params().textColor, 1.0f);

        context.dc.setStrokeColor(Color(0.5f, 0.5f, 0.5f));
        context.dc.setStrokeWidth(borderWidth);
        context.dc.drawRect(bounds().insetted(0.5f * borderWidth, 0.5f * borderWidth), kPaintStroke);

        context.dc.setStrokeColor(fg);
        context.dc.setStrokeDashes({ PicaPt(2), PicaPt(2) }, PicaPt::kZero);
        context.dc.drawRect(info.rect, kPaintStroke);

        context.dc.setStrokeDashes({}, PicaPt::kZero);

        context.dc.drawLines({ info.start, info.end });
        auto arrowhead = context.dc.createBezierPath();
        arrowhead->moveTo(info.end - kEditHandleRadius.asFloat() * info.toEndUnit);
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

        if (mGradientType == kGradientRadial) {
            setColor(kCenterId);
            context.dc.drawEllipse(Rect(info.center.x - kEditHandleRadius, info.center.y - kEditHandleRadius,
                                        2.0f * kEditHandleRadius, 2.0f * kEditHandleRadius),
                                   kPaintStrokeAndFill);
        }
        setColor(kStartId);
        context.dc.drawEllipse(Rect(info.start.x - kEditHandleRadius, info.start.y - kEditHandleRadius,
                                    2.0f * kEditHandleRadius, 2.0f * kEditHandleRadius), kPaintStrokeAndFill);
        setColor(kEndId);
        context.dc.drawEllipse(Rect(info.end.x - kEditHandleRadius, info.end.y - kEditHandleRadius,
                                    2.0f * kEditHandleRadius, 2.0f * kEditHandleRadius), kPaintStrokeAndFill);
    }

    DrawInfo calcDrawInfo(const DrawContext& dc)
    {
        auto size = std::min(bounds().width, bounds().height);
        auto inset = dc.roundToNearestPixel(0.2f * size);
        auto r = bounds().insetted(inset, inset);
        Point start(r.x + mDirection.startRX * r.width, r.y + mDirection.startRY * r.height);
        Point end(r.x + mDirection.endRX * r.width, r.y + mDirection.endRY * r.height);
        Point center(r.x + mDirection.centerRX * r.width, r.y + mDirection.centerRY * r.height);

        Point dir;
        auto toEnd = end - start;
        float distPica = (toEnd.x * toEnd.x + toEnd.y * toEnd.y).asFloat();
        if (distPica > 1e-3f) {
            distPica = std::sqrt(distPica);
            dir = Point(toEnd.x / distPica, toEnd.y / distPica);
        } else {
            dir = Point::kZero;
        }

        return { r, start, end, dir, center };
    }

private:
    GradientType mGradientType = kGradientLinear;
    std::function<void(DirectionEditor*)> mOnChanged;

    GrabId mHighlightId = kNoneId;
    GrabId mGrabId = kNoneId;
};

class LinearEditor : public DirectionEditor
{
    using Super = Widget;
public:
    LinearEditor() : DirectionEditor(kGradientLinear,
                                     { 0.0f, 0.0f,
                                       1.0f, 1.0f,
                                       0.0f, 0.0f }) // center; unused
    {
    }

    void onMouseMoved(GrabId id, const Point& pos) override
    {
        if (id == kStartId) {
            mDirection.startRX = (pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width;
            mDirection.startRY = (pos.y - mDrawInfo.rect.y) / mDrawInfo.rect.height;
        } else if (id == kEndId) {
            mDirection.endRX = (pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width;
            mDirection.endRY = (pos.y - mDrawInfo.rect.y) / mDrawInfo.rect.height;
        }
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);

        mDrawInfo = calcDrawInfo(context.dc);
        auto &info = mDrawInfo;

        drawCommon(context, info);
    }
};

class RadialEditor : public DirectionEditor
{
    using Super = Widget;
public:
    RadialEditor() : DirectionEditor(kGradientRadial,
                                     { 0.55f, 0.55f,  // start
                                       1.0f, 1.0f,    // end
                                       0.5f, 0.5f })  // center
    {}

    void onMouseMoved(GrabId id, const Point& pos) override
    {
        if (id == kCenterId) {
            auto centerRX = (pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width;
            auto centerRY = (pos.y - mDrawInfo.rect.y) / mDrawInfo.rect.height;
            auto dx = centerRX - mDirection.centerRX;
            auto dy = centerRY - mDirection.centerRY;
            mDirection.centerRX = centerRX;
            mDirection.centerRY = centerRY;
            mDirection.startRX += dx;
            mDirection.startRY += dy;
            mDirection.endRX += dx;
            mDirection.endRY += dy;
        } else {
            auto dx = std::max(mDirection.centerRX, (pos.x - mDrawInfo.rect.x) / mDrawInfo.rect.width) -
                      mDirection.centerRX;
            auto dy = std::max(mDirection.centerRY, (pos.y - mDrawInfo.rect.y) / mDrawInfo.rect.height) -
                      mDirection.centerRY;
            // Take the max distance, it feels weird to have only one direction change the position
            auto delta = std::max(dx, dy);
            if (id == kStartId) {
                auto endDelta = (mDirection.endRX - mDirection.centerRX);
                delta = std::min(delta, endDelta);
                mDirection.startRX = mDirection.centerRX + delta;
                mDirection.startRY = mDirection.centerRY + delta;
            } else {
                auto deltaFromStart = delta - (mDirection.startRX - mDirection.centerRX);
                deltaFromStart = std::max(0.0f, deltaFromStart);
                mDirection.endRX = mDirection.startRX + deltaFromStart;
                mDirection.endRY = mDirection.startRY + deltaFromStart;
            }
        }
    }

    void draw(UIContext& context) override
    {
        Super::draw(context);

        mDrawInfo = calcDrawInfo(context.dc);
        auto &info = mDrawInfo;

        auto lineWidth = PicaPt::fromStandardPixels(1);
        auto fg = Color(context.theme.params().textColor, 1.0f);

        context.dc.save();
        context.dc.clipToRect(bounds());
        context.dc.setStrokeColor(fg);
        context.dc.setStrokeWidth(lineWidth);
        context.dc.setStrokeDashes({ PicaPt(4), PicaPt(2), PicaPt(2), PicaPt(2) }, PicaPt::kZero);
        context.dc.drawLines({ Point(bounds().x, info.center.y), Point(bounds().maxX(), info.center.y) });
        context.dc.drawLines({ Point(info.center.x, bounds().y), Point(info.center.x, bounds().maxY()) });

        context.dc.setStrokeDashes({ PicaPt(1), PicaPt(1) }, PicaPt::kZero);

        auto v = info.start - info.center;
        PicaPt radius1(std::sqrt((v.x * v.x + v.y * v.y).asFloat()));
        v = info.end - info.center;
        PicaPt radius2(std::sqrt((v.x * v.x + v.y * v.y).asFloat()));
        context.dc.drawEllipse(Rect(info.center.x - radius1, info.center.y - radius1,
                                    2.0f * radius1, 2.0f * radius1), kPaintStroke);
        context.dc.drawEllipse(Rect(info.center.x - radius2, info.center.y - radius2,
                                    2.0f * radius2, 2.0f * radius2), kPaintStroke);

        context.dc.setStrokeDashes({}, PicaPt::kZero);
        context.dc.restore();

        drawCommon(context, info);
    }
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

    float location() const { return float(mLocation->doubleValue()); }
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
            { { Color(0.6f, 0.0f, 0.4f), 0.00f },
              { Color::kRed,             0.01f },
              { Color::kYellow,          0.99f },
              { Color(0.5f, 1.0f, 0.0f), 1.00f }
            },
            {
                0.0f, 0.0f,
                1.0f, 1.0f
            }
        };

        Button *addStop;
        LinearEditor *linear;
        RadialEditor *radial;

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
                        new VLayout({
                            mStops = (new StopEditor())->setStops(mModel.stops),
                            // Functionally this would be better off in StopEditor,
                            // but that means more hassle because StopEditor could not just
                            // iterate over its children and assume that they are all stops.
                            new HLayout({
                                new HLayout::Stretch(),
                                addStop = new Button("+ Stop"),
                                new HLayout::Stretch()
                            }),
                            new VLayout::Stretch()
                        }),
                        new VLayout({
                            mDirEditStack = (new StackedWidget())
                                                ->addPanel(linear = new LinearEditor())
                                                ->addPanel(radial = new RadialEditor()),
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
            mDirEditStack->setIndexShowing(idx);
            updateModel();
            updateDraw();
        });

        addStop->setOnClicked([this](Button*) {
            mStops->addStop();
            updateModel();
            updateDraw();
        });

        mStops->setOnChanged([this](StopEditor *se) {
            updateModel();
            updateDraw();
        });

        linear->setOnChanged([this](DirectionEditor *lde) {
            updateModel();
            updateDraw();
        });

        radial->setOnChanged([this](DirectionEditor *lde) {
            updateModel();
            updateDraw();
        });

        updateDraw();
    }

    void updateModel()
    {
        mModel.type = (mGradientType->isSegmentOn(0) ? kGradientLinear : kGradientRadial);
        mModel.stops = mStops->stops();
        mModel.dir = ((DirectionEditor*)mDirEditStack->currentPanel())->direction();
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
    SegmentedControl *mGradientType;  // ref, superclass owns
    StopEditor *mStops;  // ref, superclass owns
    StackedWidget *mDirEditStack;  // ref, superclass owns
};

} // namespace gradients

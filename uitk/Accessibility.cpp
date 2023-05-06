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

#include "Accessibility.h"

#include "Widget.h"

#include <sstream>

namespace uitk
{

AccessibilityInfo::UID AccessibilityInfo::uniqueId() const
{
    return std::make_pair(this->widget, this->indexInParent);
}

std::string AccessibilityInfo::debugDescription(const std::string& indent /*= ""*/) const
{
    auto type2str = [](Type t) {
        switch (t) {
            case Type::kNone:
                return "kNone";
            case Type::kContainer:
                return "kContainer";
            case Type::kRadioGroup:
                return "kRadioGroup";
            case Type::kList:
                return "kList";
            case Type::kLabel:
                return "kLabel";
            case Type::kMenuItem:
                return "kMenuItem";
            case Type::kButton:
                return "kButton";
            case Type::kCheckbox:
                return "kCheckbox";
            case Type::kRadioButton:
                return "kRadioButton";
            case Type::kIncDec:
                return "kIncDec";
            case Type::kSlider:
                return "kSlider";
            case Type::kCombobox:
                return "kCombobox";
            case Type::kTextEdit:
                return "kTextEdit";
            case Type::kPassword:
                return "kPassword";
        }
    };

    std::stringstream s;
    s << indent << type2str(this->type) << ", "
      << (this->isVisibleToUser ? "" : "!visible")
      << "(" << this->frameWinCoord.x.asFloat() << ", " << this->frameWinCoord.y.asFloat() << ") "
      << this->frameWinCoord.width.asFloat() << " x " << this->frameWinCoord.height.asFloat()
      << " ";
    if (std::holds_alternative<bool>(this->value)) {
        s << "val=" << (std::get<bool>(this->value) ? "true" : "false");
    } else if (std::holds_alternative<int>(this->value)) {
        s << "val=" << std::get<int>(this->value);
    } else if (std::holds_alternative<double>(this->value)) {
        s << "val=" << std::get<double>(this->value);
    } else if (std::holds_alternative<std::string>(this->value)) {
        s << "val=\"" << std::get<std::string>(this->value) << "\"";
    } else {
        s << "val=none";
    }
    s << ", f={"
      << (this->performLeftClick ? "click " : "")
      << (this->performDecrementNumeric ? "dec" : "")
      << (this->performIncrementNumeric ? "inc" : "")
      << "} [" << this->text << "]";
    for (auto &child : this->children) {
        s << std::endl;
        s << child.debugDescription(indent + "  ");
    }
    return s.str();
}

}  // namespace uitk

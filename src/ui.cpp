#include "DistrhoUI.hpp"
#include "Color.hpp"

#include <algorithm>
#include <cmath>

USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Circle;
using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::GraphicsContext;
using DGL_NAMESPACE::Line;
using DGL_NAMESPACE::Rectangle;

namespace
{

    constexpr float kDefaultValues[kParameterCount] = {
        -27.0f,
        10.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
    };

} // namespace

class MuteUI : public UI
{
public:
    MuteUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fDraggingParam(kParameterCount),
          fDragStartY(0.0),
          fDragStartValue(0.0f)
    {
        for (uint32_t i = 0; i < kParameterCount; ++i)
            fValues[i] = kDefaultValues[i];

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true, true);
    }

protected:
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index >= kParameterCount)
            return;

        fValues[index] = clampParameter(index, value);
        repaint();
    }

    void onDisplay() override
    {
        const GraphicsContext &context = getGraphicsContext();
        const double scale = std::min(getWidth() / 320.0, getHeight() / 180.0);
        const double offsetX = (getWidth() - 320.0 * scale) * 0.5;
        const double offsetY = (getHeight() - 180.0 * scale) * 0.5;

        for (uint32_t index = 0; index < 4; ++index)
            drawSlider(context, index, offsetX, offsetY, scale);

        drawToggle(context, kParameterFlip, offsetX, offsetY, scale);
        drawToggle(context, kParameterRect, offsetX, offsetY, scale);
    }

    bool onMouse(const MouseEvent &event) override
    {
        if (event.button != 1)
            return false;

        if (event.press)
        {
            const uint32_t index = parameterAt(event.pos.getX(), event.pos.getY());

            if (index >= kParameterCount)
                return false;

            if (index == kParameterFlip || index == kParameterRect)
            {
                editParameter(index, true);
                setParameter(index, fValues[index] < 0.5f ? 1.0f : 0.0f, true);
                editParameter(index, false);
                return true;
            }

            fDraggingParam = index;
            fDragStartY = event.pos.getY();
            fDragStartValue = fValues[index];
            editParameter(index, true);
            setParameterFromY(index, event.pos.getY());
            return true;
        }

        if (fDraggingParam < kParameterCount)
        {
            editParameter(fDraggingParam, false);
            fDraggingParam = kParameterCount;
            return true;
        }

        return false;
    }

    bool onMotion(const MotionEvent &event) override
    {
        if (fDraggingParam >= kParameterCount)
            return false;

        const float span = parameterMax(fDraggingParam) - parameterMin(fDraggingParam);
        const float delta = static_cast<float>((fDragStartY - event.pos.getY()) / 90.0) * span;
        setParameter(fDraggingParam, fDragStartValue + delta, true);
        return true;
    }

    bool onScroll(const ScrollEvent &event) override
    {
        const uint32_t index = parameterAt(event.pos.getX(), event.pos.getY());

        if (index >= kParameterCount)
            return false;

        if (index == kParameterFlip || index == kParameterRect)
        {
            editParameter(index, true);
            setParameter(index, fValues[index] < 0.5f ? 1.0f : 0.0f, true);
            editParameter(index, false);
            return true;
        }

        const float span = parameterMax(index) - parameterMin(index);
        editParameter(index, true);
        setParameter(index, fValues[index] + static_cast<float>(event.delta.getY()) * span * 0.01f, true);
        editParameter(index, false);
        return true;
    }

private:
    void setParameter(const uint32_t index, const float value, const bool sendToPlugin)
    {
        const float newValue = clampParameter(index, value);

        if (fValues[index] == newValue)
            return;

        fValues[index] = newValue;

        if (sendToPlugin)
            setParameterValue(index, fValues[index]);

        repaint();
    }

    void setParameterFromY(const uint32_t index, const double y)
    {
        const double scale = std::min(getWidth() / 320.0, getHeight() / 180.0);
        const double offsetY = (getHeight() - 180.0 * scale) * 0.5;
        const double top = offsetY + 28.0 * scale;
        const double height = 108.0 * scale;
        const float minValue = parameterMin(index);
        const float maxValue = parameterMax(index);
        const double normalized = std::max(0.0, std::min(1.0, 1.0 - (y - top) / height));

        setParameter(index, minValue + static_cast<float>(normalized) * (maxValue - minValue), true);
    }

    uint32_t parameterAt(const double x, const double y) const
    {
        const double scale = std::min(getWidth() / 320.0, getHeight() / 180.0);
        const double offsetX = (getWidth() - 320.0 * scale) * 0.5;
        const double offsetY = (getHeight() - 180.0 * scale) * 0.5;

        for (uint32_t index = 0; index < 4; ++index)
        {
            const double centerX = offsetX + (44.0 + index * 56.0) * scale;
            const double left = centerX - 18.0 * scale;
            const double top = offsetY + 20.0 * scale;
            const double width = 36.0 * scale;
            const double height = 128.0 * scale;

            if (x >= left && x <= left + width && y >= top && y <= top + height)
                return index;
        }

        if (containsToggle(kParameterFlip, x, y, offsetX, offsetY, scale))
            return kParameterFlip;

        if (containsToggle(kParameterRect, x, y, offsetX, offsetY, scale))
            return kParameterRect;

        return kParameterCount;
    }

    void drawSlider(const GraphicsContext &context,
                    const uint32_t index,
                    const double offsetX,
                    const double offsetY,
                    const double scale) const
    {
        const double centerX = offsetX + (44.0 + index * 56.0) * scale;
        const double top = offsetY + 28.0 * scale;
        const double height = 108.0 * scale;
        const double normalized = normalizedValue(index);
        const double fillHeight = height * normalized;
        const double thumbY = top + height - fillHeight;

        Color(35, 37, 38).setFor(context);
        Rectangle<double>(centerX - 5.0 * scale, top, 10.0 * scale, height).draw(context);

        parameterColor(index).setFor(context);
        Rectangle<double>(centerX - 5.0 * scale, thumbY, 10.0 * scale, fillHeight).draw(context);

        Color(236, 239, 239).setFor(context);
        Rectangle<double>(centerX - 15.0 * scale,
                          thumbY - 4.0 * scale,
                          30.0 * scale,
                          8.0 * scale)
            .draw(context);

        Color(73, 78, 80).setFor(context);
        Rectangle<double>(centerX - 18.0 * scale,
                          top - 8.0 * scale,
                          36.0 * scale,
                          height + 16.0 * scale)
            .drawOutline(context, 1.0 * scale);
    }

    void drawToggle(const GraphicsContext &context,
                    const uint32_t index,
                    const double offsetX,
                    const double offsetY,
                    const double scale) const
    {
        const double left = offsetX + 258.0 * scale;
        const double top = offsetY + (index == kParameterFlip ? 46.0 : 96.0) * scale;
        const bool enabled = fValues[index] >= 0.5f;

        (enabled ? Color(245, 158, 42) : Color(35, 37, 38)).setFor(context);
        Rectangle<double>(left, top, 38.0 * scale, 28.0 * scale).draw(context);

        Color(236, 239, 239).setFor(context);
        Circle<double>(left + (enabled ? 24.0 : 8.0) * scale,
                       top + 8.0 * scale,
                       static_cast<float>(12.0 * scale),
                       24)
            .draw(context);

        Color(73, 78, 80).setFor(context);
        Rectangle<double>(left, top, 38.0 * scale, 28.0 * scale).drawOutline(context, 1.0 * scale);
    }

    static bool containsToggle(const uint32_t index,
                               const double x,
                               const double y,
                               const double offsetX,
                               const double offsetY,
                               const double scale)
    {
        const double left = offsetX + 258.0 * scale;
        const double top = offsetY + (index == kParameterFlip ? 46.0 : 96.0) * scale;

        return x >= left && x <= left + 38.0 * scale && y >= top && y <= top + 28.0 * scale;
    }

    double normalizedValue(const uint32_t index) const
    {
        const float minValue = parameterMin(index);
        const float maxValue = parameterMax(index);

        return (fValues[index] - minValue) / (maxValue - minValue);
    }

    static Color parameterColor(const uint32_t index)
    {
        switch (index)
        {
        case kParameterThreshold:
            return Color(245, 158, 42);
        case kParameterRatio:
            return Color(66, 184, 131);
        case kParameterInputGain:
            return Color(84, 160, 255);
        case kParameterOutputGain:
            return Color(220, 95, 133);
        default:
            return Color(245, 158, 42);
        }
    }

    static float clampParameter(const uint32_t index, const float value)
    {
        return std::max(parameterMin(index), std::min(parameterMax(index), value));
    }

    static float parameterMin(const uint32_t index)
    {
        switch (index)
        {
        case kParameterThreshold:
            return -100.0f;
        case kParameterRatio:
            return 1.0f;
        case kParameterInputGain:
        case kParameterOutputGain:
            return -30.0f;
        case kParameterFlip:
        case kParameterRect:
            return 0.0f;
        default:
            return 0.0f;
        }
    }

    static float parameterMax(const uint32_t index)
    {
        switch (index)
        {
        case kParameterThreshold:
            return 0.0f;
        case kParameterRatio:
            return 50.0f;
        case kParameterInputGain:
        case kParameterOutputGain:
            return 6.0f;
        case kParameterFlip:
        case kParameterRect:
            return 1.0f;
        default:
            return 1.0f;
        }
    }

    float fValues[kParameterCount];
    uint32_t fDraggingParam;
    double fDragStartY;
    float fDragStartValue;
};

UI *createUI()
{
    return new MuteUI();
}

END_NAMESPACE_DISTRHO

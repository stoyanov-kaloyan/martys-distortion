#include "DistrhoUI.hpp"
#include "ArrowArtwork.hpp"
#include "BackgroundArtwork.hpp"
#include "Color.hpp"
#include "Image.hpp"
#include "ImageWidgets.hpp"

#include <algorithm>

USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::GraphicsContext;
using DGL_NAMESPACE::Image;
using DGL_NAMESPACE::ImageKnob;

namespace
{

    constexpr float kDefaultDoom = 0.5f;

} // namespace

class MuteUI : public UI, public ImageKnob::Callback
{
public:
    MuteUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fDoom(kDefaultDoom)
    {
        fBackground.loadFromMemory(BackgroundArtwork::bgData,
                                   BackgroundArtwork::bgWidth,
                                   BackgroundArtwork::bgHeight);

        Image knobSkin(ArrowArtwork::arrowData,
                       ArrowArtwork::arrowWidth,
                       ArrowArtwork::arrowHeight);

        fKnob = new ImageKnob(this, knobSkin);
        fKnob->setAbsolutePos(144, 416);
        fKnob->setCallback(this);
        fKnob->setId(kParameterDoom);
        fKnob->setDefault(kDefaultDoom);
        fKnob->setValue(fDoom);

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true, true);
    }

protected:
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index != kParameterDoom)
            return;

        fDoom = clamp01(value);
        fKnob->setValue(fDoom);
        repaint();
    }

    void onDisplay() override
    {
        const GraphicsContext &context = getGraphicsContext();
        const int offsetX = static_cast<int>((getWidth() - BackgroundArtwork::bgWidth) / 2);
        const int offsetY = static_cast<int>((getHeight() - BackgroundArtwork::bgHeight) / 2);

        drawBackground(context, offsetX, offsetY);
    }

private:
    void imageKnobDragStarted(ImageKnob *knob) override
    {
        if (knob->getId() == kParameterDoom)
            editParameter(kParameterDoom, true);
    }

    void imageKnobDragFinished(ImageKnob *knob) override
    {
        if (knob->getId() == kParameterDoom)
            editParameter(kParameterDoom, false);
    }

    void imageKnobValueChanged(ImageKnob *knob, const float value) override
    {
        if (knob->getId() != kParameterDoom)
            return;

        fDoom = clamp01(value);
        setParameterValue(kParameterDoom, fDoom);
        repaint();
    }

    void imageKnobDoubleClicked(ImageKnob *knob) override
    {
        if (knob->getId() != kParameterDoom)
            return;

        fDoom = kDefaultDoom;
        fKnob->setValue(fDoom);
        setParameterValue(kParameterDoom, fDoom);
        repaint();
    }

    void drawBackground(const GraphicsContext &context, const int offsetX, const int offsetY)
    {
        Color(0, 0, 0).setFor(context);
        DGL_NAMESPACE::Rectangle<double>(0.0, 0.0, getWidth(), getHeight()).draw(context);

        if (fBackground.isValid())
            fBackground.drawAt(context, offsetX, offsetY);
    }

    static float clamp01(const float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    Image fBackground;
    float fDoom;
    ScopedPointer<ImageKnob> fKnob;
};

UI *createUI()
{
    return new MuteUI();
}

END_NAMESPACE_DISTRHO

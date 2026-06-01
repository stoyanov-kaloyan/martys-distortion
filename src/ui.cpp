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
    constexpr float kDefaultTone = 0.75f;

} // namespace

class MuteUI : public UI, public ImageKnob::Callback
{
public:
    MuteUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fDoom(kDefaultDoom),
          fTone(kDefaultTone)
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

        fToneKnob = new ImageKnob(this, knobSkin);
        fToneKnob->setAbsolutePos(288, 416);
        fToneKnob->setCallback(this);
        fToneKnob->setId(kParameterTone);
        fToneKnob->setDefault(kDefaultTone);
        fToneKnob->setValue(fTone);

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true, true);
    }

protected:
    void parameterChanged(const uint32_t index, const float value) override
    {
        switch (index)
        {
        case kParameterDoom:
            fDoom = clamp01(value);
            fKnob->setValue(fDoom);
            repaint();
            break;
        case kParameterTone:
            fTone = clamp01(value);
            fToneKnob->setValue(fTone);
            repaint();
            break;
        }
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
        editParameter(knob->getId(), true);
    }

    void imageKnobDragFinished(ImageKnob *knob) override
    {
        editParameter(knob->getId(), false);
    }

    void imageKnobValueChanged(ImageKnob *knob, const float value) override
    {
        const uint32_t index = knob->getId();
        const float clampedValue = clamp01(value);

        switch (index)
        {
        case kParameterDoom:
            fDoom = clampedValue;
            setParameterValue(kParameterDoom, fDoom);
            repaint();
            break;
        case kParameterTone:
            fTone = clampedValue;
            setParameterValue(kParameterTone, fTone);
            repaint();
            break;
        }
    }

    void imageKnobDoubleClicked(ImageKnob *knob) override
    {
        switch (knob->getId())
        {
        case kParameterDoom:
            fDoom = kDefaultDoom;
            fKnob->setValue(fDoom);
            setParameterValue(kParameterDoom, fDoom);
            repaint();
            break;
        case kParameterTone:
            fTone = kDefaultTone;
            fToneKnob->setValue(fTone);
            setParameterValue(kParameterTone, fTone);
            repaint();
            break;
        }
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
    float fTone;
    ScopedPointer<ImageKnob> fKnob;
    ScopedPointer<ImageKnob> fToneKnob;
};

UI *createUI()
{
    return new MuteUI();
}

END_NAMESPACE_DISTRHO

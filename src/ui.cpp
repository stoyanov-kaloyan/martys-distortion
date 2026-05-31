#include "DistrhoUI.hpp"
#include "Color.hpp"
#include "Image.hpp"
#include "ImageWidgets.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
# pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
# pragma GCC diagnostic ignored "-Wmisleading-indentation"
# pragma GCC diagnostic ignored "-Wshift-negative-value"
# pragma GCC diagnostic ignored "-Wstringop-overflow"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "src/nanovg/stb_image.h"
#if defined(__GNUC__)
# pragma GCC diagnostic pop
#endif

#include "dpf/examples/CairoUI/Artwork.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

USE_NAMESPACE_DISTRHO;

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Circle;
using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::GraphicsContext;
using DGL_NAMESPACE::Image;
using DGL_NAMESPACE::ImageKnob;
using DGL_NAMESPACE::Line;
using DGL_NAMESPACE::Rectangle;

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
        Image knobSkin;
        loadKnobSkin(knobSkin);

        fKnob = new ImageKnob(this, knobSkin);
        fKnob->setAbsolutePos(88, 56);
        fKnob->setSize(64, 64);
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
        const double scale = std::min(getWidth() / 240.0, getHeight() / 180.0);
        const double offsetX = (getWidth() - 240.0 * scale) * 0.5;
        const double offsetY = (getHeight() - 180.0 * scale) * 0.5;

        drawBackground(context, offsetX, offsetY, scale);
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

    void drawBackground(const GraphicsContext &context,
                        const double offsetX,
                        const double offsetY,
                        const double scale) const
    {

        const double needle = -2.35 + clamp01(fDoom) * 4.7;
        const double x = offsetX + (120.0 + std::sin(needle) * 48.0) * scale;
        const double y = offsetY + (88.0 - std::cos(needle) * 48.0) * scale;
        Color(240, 219, 181).setFor(context);
        Line<double>(offsetX + 120.0 * scale, offsetY + 88.0 * scale, x, y).draw(context, 2.0 * scale);
    }

    static void loadKnobSkin(Image &image)
    {
        int width = 0;
        int height = 0;
        unsigned char *const pixels = stbi_load_from_memory(
            reinterpret_cast<const unsigned char *>(Artwork::knobData),
            static_cast<int>(Artwork::knobDataSize),
            &width,
            &height,
            nullptr,
            4);

        if (pixels == nullptr)
            return;

        fDecodedKnob.assign(pixels, pixels + static_cast<size_t>(width * height * 4));
        stbi_image_free(pixels);

        for (size_t i = 0; i + 3 < fDecodedKnob.size(); i += 4)
        {
            const unsigned char red = fDecodedKnob[i];
            fDecodedKnob[i] = fDecodedKnob[i + 2];
            fDecodedKnob[i + 2] = red;
        }

        image.loadFromMemory(reinterpret_cast<const char *>(fDecodedKnob.data()),
                             static_cast<uint>(width),
                             static_cast<uint>(height));
    }

    static float clamp01(const float value)
    {
        return std::max(0.0f, std::min(1.0f, value));
    }

    static std::vector<unsigned char> fDecodedKnob;

    float fDoom;
    ScopedPointer<ImageKnob> fKnob;
};

std::vector<unsigned char> MuteUI::fDecodedKnob;

UI *createUI()
{
    return new MuteUI();
}

END_NAMESPACE_DISTRHO

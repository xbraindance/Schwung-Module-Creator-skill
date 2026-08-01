#include "Saturator.h"

namespace bdvhs
{

namespace
{
    constexpr float kEmphasisHz    = 3000.0f;
    constexpr float kEmphasisDb    = 6.0f;
    constexpr float kMaxDriveDb    = 24.0f;
    constexpr float kAsymmetry     = 0.12f;

    /** -18 dBFS. The level the gain compensation is referenced to, chosen as a
        realistic average programme level rather than full scale. */
    constexpr float kReferenceLevel = 0.1259f;

    /** Cosmetic trim so that turning the knob up reads as character rather than
        as volume. */
    constexpr float kLoudnessTrimDb = -3.0f;
}

void Saturator::prepare (double sampleRate)
{
    sr = sampleRate;

    const float osRate = static_cast<float> (sr) * 2.0f;
    preEmphasis.setHighShelf (kEmphasisHz, 0.707f, +kEmphasisDb, osRate);
    deEmphasis.setHighShelf  (kEmphasisHz, 0.707f, -kEmphasisDb, osRate);
    dcBlock.setCutoff (12.0f, osRate);

    preGain.setTimeMs (30.0f, sr);
    postGain.setTimeMs (30.0f, sr);
    preGain.reset (1.0f);
    postGain.reset (1.0f);

    reset();
}

void Saturator::reset()
{
    halfBand.reset();
    preEmphasis.reset();
    deEmphasis.reset();
    dcBlock.reset();
}

void Saturator::setDrive (float saturate01, float profileBias) noexcept
{
    const float amount = clampf (saturate01, 0.0f, 1.0f);
    const float pre = dbToGain (kMaxDriveDb * amount) * profileBias;

    // Compensate for the compression the nonlinearity applies at the reference
    // level, so the knob does not double as a volume control.
    const float compressionAtRef = std::tanh (pre * kReferenceLevel) / kReferenceLevel;
    const float post = clampf (1.0f / std::fmax (compressionAtRef, 1.0e-6f), 0.0f, 4.0f)
                       * dbToGain (kLoudnessTrimDb * amount);

    preGain.setTarget (pre);
    postGain.setTarget (post);
}

float Saturator::shape (float x) const noexcept
{
    // Asymmetry gives the even-order content tape is known for; tanh supplies
    // the soft knee.
    return std::tanh (x + kAsymmetry * x * x);
}

float Saturator::process (float x) noexcept
{
    const float pre  = preGain.next();
    const float post = postGain.next();

    float up0 = 0.0f, up1 = 0.0f;
    halfBand.upsample (x, up0, up1);

    up0 = deEmphasis.process (shape (pre * preEmphasis.process (up0)));
    up1 = deEmphasis.process (shape (pre * preEmphasis.process (up1)));

    up0 = dcBlock.process (up0);
    up1 = dcBlock.process (up1);

    return halfBand.downsample (up0, up1) * post;
}

} // namespace bdvhs

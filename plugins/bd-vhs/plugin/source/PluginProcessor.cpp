#include "PluginProcessor.h"

namespace bdvhs
{

namespace
{
    using APVTS = juce::AudioProcessorValueTreeState;

    /** Bumped only if a future version needs to migrate old saved state. */
    constexpr int kStateVersion = 1;

    juce::NormalisableRange<float> percentRange()
    {
        return { 0.0f, 100.0f, 0.01f };
    }

    std::unique_ptr<juce::AudioParameterFloat> percentParam (const char* id,
                                                             const juce::String& name,
                                                             float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, percentRange(), defaultValue,
            juce::AudioParameterFloatAttributes().withLabel ("%"));
    }

    /** Hidden dip switches: they configure the machine rather than perform with
        it, so keep them out of the host's automation lanes. */
    juce::AudioParameterChoiceAttributes hiddenChoice()
    {
        return juce::AudioParameterChoiceAttributes().withAutomatable (false);
    }

    juce::AudioParameterBoolAttributes hiddenBool()
    {
        return juce::AudioParameterBoolAttributes().withAutomatable (false);
    }
}

APVTS::ParameterLayout createParameterLayout()
{
    APVTS::ParameterLayout layout;

    layout.add (percentParam (ids::wow,      "Wow",      25.0f));
    layout.add (percentParam (ids::flutter,  "Flutter",  20.0f));
    layout.add (percentParam (ids::model,    "Model",    30.0f));
    layout.add (percentParam (ids::saturate, "Saturate", 30.0f));
    layout.add (percentParam (ids::failure,  "Failure",  15.0f));
    layout.add (percentParam (ids::mix,      "Mix",     100.0f));

    auto volumeRange = juce::NormalisableRange<float> { -60.0f, 6.0f, 0.01f };
    volumeRange.setSkewForCentre (-12.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::volume, 1 }, "Volume", volumeRange, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::inputGain, 1 }, "Input Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::auxMode, 1 }, "Aux Mode",
        juce::StringArray { "Stop", "Filter", "Fail" }, 0));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::aux, 1 }, "Aux Footswitch", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::dryLevel, 1 }, "Dry",
        juce::StringArray { "None", "Small", "Unity" }, 1));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::noise, 1 }, "Noise",
        juce::StringArray { "Off", "Low", "High" }, 1));

    auto stopRange = juce::NormalisableRange<float> { 0.05f, 4.0f, 0.001f };
    stopRange.setSkewForCentre (0.6f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::stopTime, 1 }, "Tape Stop Time", stopRange, 0.6f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    auto rampRange = juce::NormalisableRange<float> { 0.05f, 20.0f, 0.001f };
    rampRange.setSkewForCentre (2.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ids::rampTime, 1 }, "Ramp Time", rampRange, 2.0f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::bypass, 1 }, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::noiseResponse, 1 }, "Noise Response",
        juce::StringArray { "Static", "Gated", "Ducked" }, 1, hiddenChoice()));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::dryType, 1 }, "Dry Type",
        juce::StringArray { "Clean", "Processed" }, 0, hiddenChoice()));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::rampMode, 1 }, "Ramp Mode",
        juce::StringArray { "Off", "Ramp", "Bounce" }, 0, hiddenChoice()));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::spread, 1 }, "Spread", false, hiddenBool()));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::modelSnap, 1 }, "Model Snap", false, hiddenBool()));

    return layout;
}

// ============================================================================

BdVhsProcessor::BdVhsProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BDVHS", createParameterLayout())
{
    pWow           = apvts.getRawParameterValue (ids::wow);
    pFlutter       = apvts.getRawParameterValue (ids::flutter);
    pModel         = apvts.getRawParameterValue (ids::model);
    pSaturate      = apvts.getRawParameterValue (ids::saturate);
    pFailure       = apvts.getRawParameterValue (ids::failure);
    pVolume        = apvts.getRawParameterValue (ids::volume);
    pMix           = apvts.getRawParameterValue (ids::mix);
    pInputGain     = apvts.getRawParameterValue (ids::inputGain);
    pAuxMode       = apvts.getRawParameterValue (ids::auxMode);
    pAux           = apvts.getRawParameterValue (ids::aux);
    pDryLevel      = apvts.getRawParameterValue (ids::dryLevel);
    pNoise         = apvts.getRawParameterValue (ids::noise);
    pStopTime      = apvts.getRawParameterValue (ids::stopTime);
    pRampTime      = apvts.getRawParameterValue (ids::rampTime);
    pNoiseResponse = apvts.getRawParameterValue (ids::noiseResponse);
    pDryType       = apvts.getRawParameterValue (ids::dryType);
    pSpread        = apvts.getRawParameterValue (ids::spread);
    pModelSnap     = apvts.getRawParameterValue (ids::modelSnap);
    pRampMode      = apvts.getRawParameterValue (ids::rampMode);

    bypassParam = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (ids::bypass));

    setLatencySamples (Core::latencySamples());
}

void BdVhsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int channels = juce::jmax (1, juce::jmin (2, getTotalNumOutputChannels()));

    core.setParams (gatherParams());
    core.prepare (sampleRate, samplesPerBlock, channels);

    // Constant by construction at every sample rate, so this never changes at
    // runtime -- which is the only way hosts reliably cope with it.
    setLatencySamples (Core::latencySamples());
}

void BdVhsProcessor::releaseResources()
{
    core.reset();
}

bool BdVhsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    // Mono in / stereo out is supported and genuinely useful here: the
    // decorrelated transport wobble does the widening.
    return in.size() <= out.size();
}

Params BdVhsProcessor::gatherParams() const
{
    Params p;

    p.wow          = pWow->load();
    p.flutter      = pFlutter->load();
    p.model        = pModel->load();
    p.saturate     = pSaturate->load();
    p.failure      = pFailure->load();
    p.volumeDb     = pVolume->load();
    p.mix          = pMix->load();
    p.inputGainDb  = pInputGain->load();
    p.stopTimeSec  = pStopTime->load();
    p.rampTimeSec  = pRampTime->load();

    p.auxMode       = static_cast<AuxMode>       (static_cast<int> (pAuxMode->load()));
    p.dryLevel      = static_cast<DryLevel>      (static_cast<int> (pDryLevel->load()));
    p.noise         = static_cast<NoiseLevel>    (static_cast<int> (pNoise->load()));
    p.noiseResponse = static_cast<NoiseResponse> (static_cast<int> (pNoiseResponse->load()));
    p.dryType       = static_cast<DryType>       (static_cast<int> (pDryType->load()));
    p.rampMode      = static_cast<RampMode>      (static_cast<int> (pRampMode->load()));

    p.auxHeld   = pAux->load()       > 0.5f;
    p.spread    = pSpread->load()    > 0.5f;
    p.modelSnap = pModelSnap->load() > 0.5f;
    p.bypass    = bypassParam != nullptr && bypassParam->get();

    return p;
}

void BdVhsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Mono into stereo: duplicate before processing so both channels get their
    // own transport wobble rather than one silent side.
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.copyFrom (ch, 0, buffer, juce::jmin (ch, juce::jmax (0, numIn - 1)), 0, numSamples);

    const int channels = juce::jmin (2, numOut);
    if (channels <= 0 || numSamples <= 0)
        return;

    float* ptrs[2] = { buffer.getWritePointer (0),
                       channels > 1 ? buffer.getWritePointer (1) : nullptr };

    core.setParams (gatherParams());
    core.process (ptrs, channels, numSamples);

    // Anything above the second channel (not reachable through the supported
    // layouts, but cheap insurance) gets cleared rather than left stale.
    for (int ch = channels; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);
}

void BdVhsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("stateVersion", kStateVersion, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BdVhsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace bdvhs

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new bdvhs::BdVhsProcessor();
}

#pragma once

#include <bdvhs/BdVhsCore.h>

#include "ParameterIds.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bdvhs
{

/**
    Thin wrapper around bdvhs::Core.

    Deliberately thin: everything with an opinion about sound lives in the
    framework-free core, and this class only marshals parameters, reports
    latency, and manages state. That split is what lets the DSP be built and
    tested on machines where the JUCE plugin targets cannot even configure.

    No custom editor yet -- hosts draw generic controls. Adding one later is a
    pure addition: create the editor class, flip hasEditor(), return it from
    createEditor(). Nothing here has to change.
*/
class BdVhsProcessor : public juce::AudioProcessor
{
public:
    BdVhsProcessor();
    ~BdVhsProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override                     { return false; }

    const juce::String getName() const override         { return JucePlugin_Name; }

    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0.5; }

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Lets the host drive real bypass, with correct latency compensation,
        rather than each host inventing its own. */
    juce::AudioParameterBool* getBypassParameter() const override { return bypassParam; }

    juce::AudioProcessorValueTreeState apvts;

private:
    Params gatherParams() const;

    Core core;

    std::atomic<float>* pWow           = nullptr;
    std::atomic<float>* pFlutter       = nullptr;
    std::atomic<float>* pModel         = nullptr;
    std::atomic<float>* pSaturate      = nullptr;
    std::atomic<float>* pFailure       = nullptr;
    std::atomic<float>* pVolume        = nullptr;
    std::atomic<float>* pMix           = nullptr;
    std::atomic<float>* pInputGain     = nullptr;
    std::atomic<float>* pAuxMode       = nullptr;
    std::atomic<float>* pAux           = nullptr;
    std::atomic<float>* pDryLevel      = nullptr;
    std::atomic<float>* pNoise         = nullptr;
    std::atomic<float>* pStopTime      = nullptr;
    std::atomic<float>* pRampTime      = nullptr;
    std::atomic<float>* pNoiseResponse = nullptr;
    std::atomic<float>* pDryType       = nullptr;
    std::atomic<float>* pSpread        = nullptr;
    std::atomic<float>* pModelSnap     = nullptr;
    std::atomic<float>* pRampMode      = nullptr;

    juce::AudioParameterBool* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BdVhsProcessor)
};

} // namespace bdvhs

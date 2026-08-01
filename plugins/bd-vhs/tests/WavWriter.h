#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace bdvhs::test
{

/** Minimal 32-bit float RIFF writer, just enough to dump renders for listening. */
inline bool writeWav (const std::string& path,
                      const std::vector<std::vector<float>>& channels,
                      double sampleRate)
{
    if (channels.empty())
        return false;

    const uint16_t numChannels = static_cast<uint16_t> (channels.size());
    const uint32_t numFrames   = static_cast<uint32_t> (channels[0].size());
    const uint16_t bitsPerSample = 32;
    const uint16_t blockAlign  = static_cast<uint16_t> (numChannels * bitsPerSample / 8);
    const uint32_t byteRate    = static_cast<uint32_t> (sampleRate) * blockAlign;
    const uint32_t dataBytes   = numFrames * blockAlign;

    FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr)
        return false;

    auto u32 = [f] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto u16 = [f] (uint16_t v) { std::fwrite (&v, 2, 1, f); };

    std::fwrite ("RIFF", 1, 4, f);
    u32 (36 + dataBytes);
    std::fwrite ("WAVE", 1, 4, f);

    std::fwrite ("fmt ", 1, 4, f);
    u32 (16);
    u16 (3);                                       // IEEE float
    u16 (numChannels);
    u32 (static_cast<uint32_t> (sampleRate));
    u32 (byteRate);
    u16 (blockAlign);
    u16 (bitsPerSample);

    std::fwrite ("data", 1, 4, f);
    u32 (dataBytes);

    for (uint32_t i = 0; i < numFrames; ++i)
        for (uint16_t c = 0; c < numChannels; ++c)
            std::fwrite (&channels[c][i], 4, 1, f);

    std::fclose (f);
    return true;
}

} // namespace bdvhs::test

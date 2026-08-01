#pragma once

#include "Common.h"

#include <algorithm>
#include <vector>

namespace bdvhs
{

/**
    Power-of-two circular delay line with 4-point Catmull-Rom fractional reads.

    Catmull-Rom rather than an allpass interpolator: an allpass carries state,
    and under the fast delay changes this effect produces -- 8 Hz flutter, and
    failure "snag" events that jump the read rate outright -- that state smears
    into audible transients. Catmull-Rom is stateless and C1-continuous, so a
    swept read pointer produces no derivative discontinuity and therefore no
    zipper.
*/
class DelayLine
{
public:
    /** Allocates. Call from prepare() only. */
    void prepare (int maxDelaySamples)
    {
        const int size = nextPowerOfTwo (maxDelaySamples + 4);
        buffer.assign (static_cast<size_t> (size), 0.0f);
        mask = size - 1;
        writeIndex = 0;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void write (float x) noexcept
    {
        writeIndex = (writeIndex + 1) & mask;
        buffer[static_cast<size_t> (writeIndex)] = x;
    }

    /** Reads `delaySamples` in the past. Must be >= 2 and <= the prepared size
        minus 2, so the four-point kernel always has valid neighbours. */
    float read (float delaySamples) const noexcept
    {
        const float clamped = clampf (delaySamples, 2.0f, static_cast<float> (mask - 2));
        const int   i       = static_cast<int> (clamped);
        const float t       = clamped - static_cast<float> (i);

        // y1 sits at delay i, y2 one sample older, y0 one newer, y3 two older.
        const float y0 = buffer[static_cast<size_t> ((writeIndex - (i - 1)) & mask)];
        const float y1 = buffer[static_cast<size_t> ((writeIndex -  i     ) & mask)];
        const float y2 = buffer[static_cast<size_t> ((writeIndex - (i + 1)) & mask)];
        const float y3 = buffer[static_cast<size_t> ((writeIndex - (i + 2)) & mask)];

        return catmullRom (y0, y1, y2, y3, t);
    }

    /** Integer-delay read, for the dry alignment tap where no interpolation is
        wanted or needed. */
    float readInt (int delaySamples) const noexcept
    {
        return buffer[static_cast<size_t> ((writeIndex - delaySamples) & mask)];
    }

    int capacity() const noexcept { return mask + 1; }

private:
    static float catmullRom (float y0, float y1, float y2, float y3, float t) noexcept
    {
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    std::vector<float> buffer;
    int mask = 0;
    int writeIndex = 0;
};

} // namespace bdvhs

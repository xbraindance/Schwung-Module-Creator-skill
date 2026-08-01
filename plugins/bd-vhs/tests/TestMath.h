#pragma once

namespace bdvhs::test
{

// Not M_PI. That is a POSIX extension: MSVC only defines it when
// _USE_MATH_DEFINES is set before every <cmath> in the translation unit, which
// is not something a header can rely on.
inline constexpr double kPiD = 3.14159265358979323846;

} // namespace bdvhs::test

#ifndef GLOBAL_CONSTANTS_H
#define GLOBAL_CONSTANTS_H

constexpr bool buildInReleaseMode =
#ifdef BUILD_IN_RELEASE_MODE
    true;
#else
    false;
#endif

#endif // GLOBAL_CONSTANTS_H

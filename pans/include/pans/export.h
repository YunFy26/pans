#ifndef PANS_INCLUDE_PANSEXPORT_H
#define PANS_INCLUDE_PANSEXPORT_H

#if defined(_WIN32) && defined(PANS_SHARED_LIBIARY)
#if define(PANS_BUILDING_LIBRARY)
#define PANS_API __declspec(dllexport)
#else
#define PANS_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && defined(PANS_SHARED_LIBRARY)
#define PANS_API __attribute__((visibility("default")))
#else
#define PANS_API
#endif

#endif
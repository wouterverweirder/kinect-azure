
#ifndef K4A_EXPORT_H
#define K4A_EXPORT_H

#ifdef K4A_STATIC_DEFINE
#  define K4A_EXPORT
#  define K4A_NO_EXPORT
#else
#  ifndef K4A_EXPORT
#    ifdef k4a_EXPORTS
        /* We are building this library */
#      define K4A_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define K4A_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef K4A_NO_EXPORT
#    define K4A_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef K4A_DEPRECATED
#  define K4A_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef K4A_DEPRECATED_EXPORT
#  define K4A_DEPRECATED_EXPORT K4A_EXPORT K4A_DEPRECATED
#endif

#ifndef K4A_DEPRECATED_NO_EXPORT
#  define K4A_DEPRECATED_NO_EXPORT K4A_NO_EXPORT K4A_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef K4A_NO_DEPRECATED
#    define K4A_NO_DEPRECATED
#  endif
#endif

#endif

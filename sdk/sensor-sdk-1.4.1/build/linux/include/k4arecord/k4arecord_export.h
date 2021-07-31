
#ifndef K4ARECORD_EXPORT_H
#define K4ARECORD_EXPORT_H

#ifdef K4ARECORD_STATIC_DEFINE
#  define K4ARECORD_EXPORT
#  define K4ARECORD_NO_EXPORT
#else
#  ifndef K4ARECORD_EXPORT
#    ifdef k4arecord_EXPORTS
        /* We are building this library */
#      define K4ARECORD_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define K4ARECORD_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef K4ARECORD_NO_EXPORT
#    define K4ARECORD_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef K4ARECORD_DEPRECATED
#  define K4ARECORD_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef K4ARECORD_DEPRECATED_EXPORT
#  define K4ARECORD_DEPRECATED_EXPORT K4ARECORD_EXPORT K4ARECORD_DEPRECATED
#endif

#ifndef K4ARECORD_DEPRECATED_NO_EXPORT
#  define K4ARECORD_DEPRECATED_NO_EXPORT K4ARECORD_NO_EXPORT K4ARECORD_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef K4ARECORD_NO_DEPRECATED
#    define K4ARECORD_NO_DEPRECATED
#  endif
#endif

#endif

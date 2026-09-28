#include "kinoko/audio_math.h"
#include <fenv.h>
#include <float.h>

namespace {
class AudioEnvironmentScope {
    fenv_t previous{};
public:
    AudioEnvironmentScope() {
        fegetenv(&previous);
#if defined(_MSC_VER)
        _fpreset(); // Preserve the verified Windows decoder environment exactly.
#else
        fesetenv(FE_DFL_ENV);
#endif
    }
    ~AudioEnvironmentScope() { fesetenv(&previous); }
    AudioEnvironmentScope(const AudioEnvironmentScope &) = delete;
    AudioEnvironmentScope &operator=(const AudioEnvironmentScope &) = delete;
};
}

extern "C" int kinoko_prepare_audio(kinoko_prepare_audio_fn prepare,
    uint32_t handle, const char *path, int looping, float volume) noexcept(false) {
    AudioEnvironmentScope environment;
    return prepare(handle, path, looping, volume);
}

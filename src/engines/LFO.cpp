/*
 * Copyright (c) 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "LFO.h"
#include "common/LFOAll.h"
#include <type_traits> // for std::is_same
#include <assert.h>

namespace LinuxSampler {

enum lfo_class_t {
    lfo_class_sine_signed,
    lfo_class_sine_unsigned,
    lfo_class_triangle_signed,
    lfo_class_triangle_unsigned,
    lfo_class_saw_signed,
    lfo_class_saw_unsigned,
    lfo_class_square_signed,
    lfo_class_square_unsigned,
};

struct LFOPriv {
    LFOPOD* lfo = NULL;
    lfo_class_t lfoClass = (lfo_class_t) -1; // some invalid value

    virtual ~LFOPriv() {
        if (lfo) delete lfo;
    }
};

LFO::LFO() {
    SELF = new LFOPriv;
}

LFO::~LFO() {
    if (SELF) delete SELF;
}

template<class T>
static T* createLFO(LFOPriv* SELF, const LFO::SetupOpt& opt) {
    if (SELF->lfo) {
        delete SELF->lfo;
        SELF->lfo = NULL;
    }

    const bool flipPolarity = (opt.flipPolarity) ? *opt.flipPolarity : false;
    const float maxValue = (opt.maxValue) ? *opt.maxValue : 1.0;
    const float frequency = (opt.frequency) ? *opt.frequency : 1.0;
    const LFO::start_level_t startLevel = (opt.startLevel) ? *opt.startLevel : LFO::start_level_mid;
    const uint16_t internalDepth = (opt.internalDepth) ? *opt.internalDepth : 0;
    const uint16_t midiCtrlDepth = (opt.midiControllerDepth) ? *opt.midiControllerDepth : 0;
    const float samplerate = (opt.samplerate) ? *opt.samplerate : 44100;

    T* lfo = new T(maxValue);
    SELF->lfo = lfo;

    lfo->trigger(frequency, startLevel, internalDepth, midiCtrlDepth, flipPolarity, samplerate);
    if (opt.phase)
        lfo->setPhase( *opt.phase );
    lfo->updateByMIDICtrlValue(0);

    if (std::is_same<T,LFOSineSigned>::value)
        SELF->lfoClass = lfo_class_sine_signed;
    else if (std::is_same<T,LFOSineUnsigned>::value)
        SELF->lfoClass = lfo_class_sine_unsigned;
    else if (std::is_same<T,LFOTriangleSigned>::value)
        SELF->lfoClass = lfo_class_triangle_signed;
    else if (std::is_same<T,LFOTriangleUnsigned>::value)
        SELF->lfoClass = lfo_class_triangle_unsigned;
    else if (std::is_same<T,LFOSawSigned>::value)
        SELF->lfoClass = lfo_class_saw_signed;
    else if (std::is_same<T,LFOSawUnsigned>::value)
        SELF->lfoClass = lfo_class_saw_unsigned;
    else if (std::is_same<T,LFOSquareSigned>::value)
        SELF->lfoClass = lfo_class_square_signed;
    else if (std::is_same<T,LFOSquareUnsigned>::value)
        SELF->lfoClass = lfo_class_square_unsigned;
    else
        assert(false);

    return lfo;
}

void LFO::setup(const SetupOpt& opt) {
    const wave_t wave = (opt.waveType) ? *opt.waveType : wave_sine;
    const bool isSigned = (opt.rangeType) ? (*opt.rangeType == range_signed) : false;

    switch (wave) {
        case wave_sine:
            if (isSigned)
                createLFO<LFOSineSigned>(SELF, opt);
            else
                createLFO<LFOSineUnsigned>(SELF, opt);
            break;
        case wave_triangle:
            if (isSigned)
                createLFO<LFOTriangleSigned>(SELF, opt);
            else
                createLFO<LFOTriangleUnsigned>(SELF, opt);
            break;
        case wave_saw:
            if (isSigned)
                createLFO<LFOSawSigned>(SELF, opt);
            else
                createLFO<LFOSawUnsigned>(SELF, opt);
            break;
        case wave_square:
            if (isSigned)
                createLFO<LFOSquareSigned>(SELF, opt);
            else
                createLFO<LFOSquareUnsigned>(SELF, opt);
            break;
        default:
            assert(false);
    }
}

template<class T>
inline float renderLFO(LFOPriv* SELF) {
    return static_cast<T*>(SELF->lfo)->render();
}

float LFO::render() {
    switch (SELF->lfoClass) {
        case lfo_class_sine_signed:
            return renderLFO<LFOSineSigned>(SELF);
        case lfo_class_sine_unsigned:
            return renderLFO<LFOSineUnsigned>(SELF);
        case lfo_class_triangle_signed:
            return renderLFO<LFOTriangleSigned>(SELF);
        case lfo_class_triangle_unsigned:
            return renderLFO<LFOTriangleUnsigned>(SELF);
        case lfo_class_saw_signed:
            return renderLFO<LFOSawSigned>(SELF);
        case lfo_class_saw_unsigned:
            return renderLFO<LFOSawUnsigned>(SELF);
        case lfo_class_square_signed:
            return renderLFO<LFOSquareSigned>(SELF);
        case lfo_class_square_unsigned:
            return renderLFO<LFOSquareUnsigned>(SELF);
    }
    return 0;
}

template<class T>
inline void setLFOMidiCtrlValue(LFOPriv* SELF, uint16_t value) {
    return static_cast<T*>(SELF->lfo)->updateByMIDICtrlValue(value);
}

void LFO::setMIDICtrlValue(uint8_t midiCCValue) {
    switch (SELF->lfoClass) {
        case lfo_class_sine_signed:
            return setLFOMidiCtrlValue<LFOSineSigned>(SELF, midiCCValue);
        case lfo_class_sine_unsigned:
            return setLFOMidiCtrlValue<LFOSineUnsigned>(SELF, midiCCValue);
        case lfo_class_triangle_signed:
            return setLFOMidiCtrlValue<LFOTriangleSigned>(SELF, midiCCValue);
        case lfo_class_triangle_unsigned:
            return setLFOMidiCtrlValue<LFOTriangleUnsigned>(SELF, midiCCValue);
        case lfo_class_saw_signed:
            return setLFOMidiCtrlValue<LFOSawSigned>(SELF, midiCCValue);
        case lfo_class_saw_unsigned:
            return setLFOMidiCtrlValue<LFOSawUnsigned>(SELF, midiCCValue);
        case lfo_class_square_signed:
            return setLFOMidiCtrlValue<LFOSquareSigned>(SELF, midiCCValue);
        case lfo_class_square_unsigned:
            return setLFOMidiCtrlValue<LFOSquareUnsigned>(SELF, midiCCValue);
    }
}

} // namespace LinuxSampler

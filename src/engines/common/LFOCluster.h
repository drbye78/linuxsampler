/*
 * Copyright (c) 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_LFO_CLUSTER_H
#define LS_LFO_CLUSTER_H

#include "LFOAll.h"
#include <type_traits> // for std::conditional

namespace LinuxSampler {

/** @brief Low Frequency Oscillator (sampler internal template class).
 *
 * This is a generalized cluster class providing all our LFO implementations
 * encapsulated as one single class. This is thus a C++ template variant of the
 * similar public API C++ class LFO. Even though LFOCluster and LFO serve a
 * similar purpose, you should however always use this template variant instead
 * of the public API LFO class for sampler internal code. LFOCluster has a
 * higher potential for the compiler to optimize the finally emitted
 * instructions, however it requires direct access to our individual LFO
 * implementations' code (which are subject to change at any time for
 * performance reasons). Hence this template class is not suitable for public
 * API purposes (that is for third party apps), hence the reason for the
 + existence of the separate public API LFO class. The latter has the priority
 * for API & ABI stability for the price of slightly reduced runtime efficiency
 * though.
 */
template<LFO::range_type_t RANGE>
class LFOCluster {
public:
    uint8_t& ExtController = sine.ExtController; /// redirect to union

    /**
     * Constructor
     *
     * @param Max - maximum value of the output levels
     */
    LFOCluster(float Max) :
        wave(LFO::wave_sine),
        sine(Max) // union-like class: use any union member's constructor (one for all, all for one)
    {
    }

    /**
     * Calculates exactly one sample point of the LFO wave.
     *
     * @returns next LFO level
     */
    inline float render() {
        switch (wave) {
            case LFO::wave_sine:      return sine.render();
            case LFO::wave_triangle:  return triangle.render();
            case LFO::wave_saw:       return saw.render();
            case LFO::wave_square:    return square.render();
        }
        return 0.f;
    }

    /**
     * Will be called by the voice when the key / voice was triggered.
     *
     * @param Wave            - wave form to be used (e.g. sine, saw, square)
     * @param Frequency       - frequency of the oscillator in Hz
     * @param Phase           - phase displacement of wave form's start level
     *                          (0°..360°)
     * @param StartLevel      - on which level the wave should start
     * @param InternalDepth   - firm, internal oscillator amplitude
     * @param ExtControlDepth - defines how strong the external MIDI
     *                          controller has influence on the
     *                          oscillator amplitude
     * @param FlipPhase       - inverts the oscillator wave against
     *                          a horizontal axis
     * @param SampleRate      - current sample rate of the engines
     *                          audio output signal
     */
    void trigger(LFO::wave_t Wave, float Frequency, float Phase, LFO::start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) {
        wave = Wave;
        switch (Wave) {
            case LFO::wave_sine:
                sine.trigger(Frequency, StartLevel, InternalDepth, ExtControlDepth, FlipPhase, SampleRate);
                sine.setPhase(Phase);
                break;
            case LFO::wave_triangle:
                triangle.trigger(Frequency, StartLevel, InternalDepth, ExtControlDepth, FlipPhase, SampleRate);
                triangle.setPhase(Phase);
                break;
            case LFO::wave_saw:
                saw.trigger(Frequency, StartLevel, InternalDepth, ExtControlDepth, FlipPhase, SampleRate);
                saw.setPhase(Phase);
                break;
            case LFO::wave_square:
                square.trigger(Frequency, StartLevel, InternalDepth, ExtControlDepth, FlipPhase, SampleRate);
                square.setPhase(Phase);
                break;
        }
    }

    /**
     * Update LFO depth with a new external controller value.
     *
     * @param ExtControlValue - new external controller value
     */
    void updateByMIDICtrlValue(const uint16_t& ExtControlValue) {
        switch (wave) {
            case LFO::wave_sine:
                sine.updateByMIDICtrlValue(ExtControlValue);
                break;
            case LFO::wave_triangle:
                triangle.updateByMIDICtrlValue(ExtControlValue);
                break;
            case LFO::wave_saw:
                saw.updateByMIDICtrlValue(ExtControlValue);
                break;
            case LFO::wave_square:
                square.updateByMIDICtrlValue(ExtControlValue);
                break;
        }
    }

    /**
     * Should be invoked after the LFO is triggered.
     * @param phase From 0 to 360 degrees.
     */
    void setPhase(float phase) {
        switch (wave) {
            case LFO::wave_sine:
                sine.setPhase(phase);
                break;
            case LFO::wave_triangle:
                triangle.setPhase(phase);
                break;
            case LFO::wave_saw:
                saw.setPhase(phase);
                break;
            case LFO::wave_square:
                square.setPhase(phase);
                break;
        }
    }

    void setFrequency(float Frequency, unsigned int SampleRate) {
        switch (wave) {
            case LFO::wave_sine:
                sine.setFrequency(Frequency, SampleRate);
                break;
            case LFO::wave_triangle:
                triangle.setFrequency(Frequency, SampleRate);
                break;
            case LFO::wave_saw:
                saw.setFrequency(Frequency, SampleRate);
                break;
            case LFO::wave_square:
                square.setFrequency(Frequency, SampleRate);
                break;
        }
    }

    void setScriptDepthFactor(float factor, bool isFinal) {
        switch (wave) {
            case LFO::wave_sine:
                sine.setScriptDepthFactor(factor, isFinal);
                break;
            case LFO::wave_triangle:
                triangle.setScriptDepthFactor(factor, isFinal);
                break;
            case LFO::wave_saw:
                saw.setScriptDepthFactor(factor, isFinal);
                break;
            case LFO::wave_square:
                square.setScriptDepthFactor(factor, isFinal);
                break;
        }
    }

    void setScriptFrequencyFactor(float factor, unsigned int samplerate) {
        switch (wave) {
            case LFO::wave_sine:
                sine.setScriptFrequencyFactor(factor, samplerate);
                break;
            case LFO::wave_triangle:
                triangle.setScriptFrequencyFactor(factor, samplerate);
                break;
            case LFO::wave_saw:
                saw.setScriptFrequencyFactor(factor, samplerate);
                break;
            case LFO::wave_square:
                square.setScriptFrequencyFactor(factor, samplerate);
                break;
        }
    }

    void setScriptFrequencyFinal(float hz, unsigned int samplerate) {
        switch (wave) {
            case LFO::wave_sine:
                sine.setScriptFrequencyFinal(hz, samplerate);
                break;
            case LFO::wave_triangle:
                triangle.setScriptFrequencyFinal(hz, samplerate);
                break;
            case LFO::wave_saw:
                saw.setScriptFrequencyFinal(hz, samplerate);
                break;
            case LFO::wave_square:
                square.setScriptFrequencyFinal(hz, samplerate);
                break;
        }
    }

protected:
    typedef LFOSineNumericComplexNr<RANGE> Sine;
    typedef typename std::conditional<RANGE == LFO::range_signed, LFOTriangleSigned, LFOTriangleUnsigned>::type Triangle;
    typedef LFOSawIntMathNew<RANGE> Saw;
    typedef LFOSquareIntMath<RANGE> Square;

private:
    LFO::wave_t wave;
    union {
        Sine      sine;
        Triangle  triangle;
        Saw       saw;
        Square    square;
    };
};

typedef LFOCluster<LFO::range_signed> LFOClusterSigned;
typedef LFOCluster<LFO::range_unsigned> LFOClusterUnsigned;

} // namespace LinuxSampler

#endif // LS_LFO_CLUSTER_H

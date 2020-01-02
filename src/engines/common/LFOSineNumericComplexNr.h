/*
 * Copyright (c) 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_LFOSINE_NUMERIC_COMPLEX_NR_H
#define LS_LFOSINE_NUMERIC_COMPLEX_NR_H

#include "LFOBase.h"

namespace LinuxSampler {

    /** @brief Sine LFO (numeric, complex nr implementation)
     *
     * This is a Sinus Low Frequency Oscillator implementation using a complex
     * number with numeric (descrete) math as basis for its oscillation.
     */
    template<LFO::range_type_t RANGE>
    class LFOSineNumericComplexNr : public LFOBase<RANGE> {
        public:

            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOSineNumericComplexNr(float Max) : LFOBase<RANGE>::LFOBase(Max) {
                //NOTE: DO NOT add any custom initialization here, since it would break LFOCluster construction !
            }

            /**
             * Calculates exactly one sample point of the LFO wave.
             *
             * @returns next LFO level
             */
            inline float render() {
                real -= c * imag;
                imag += c * real;
                if (RANGE == LFO::range_unsigned)
                    return real * normalizer + offset;
                else /* signed range */
                    return real * normalizer;
            }

            /**
             * Update LFO depth with a new external controller value.
             *
             * @param ExtControlValue - new external controller value
             */
            inline void updateByMIDICtrlValue(const uint16_t& ExtControlValue) {
                this->ExtControlValue = ExtControlValue;

                const float max = (this->InternalDepth + ExtControlValue * this->ExtControlDepthCoeff) * this->ScriptDepthFactor;
                if (RANGE == LFO::range_unsigned) {
                    normalizer = max * 0.5f;
                    offset     = normalizer;
                } else { // signed range
                    normalizer = max;
                }
            }

            /**
             * Will be called by the voice when the key / voice was triggered.
             *
             * @param Frequency       - frequency of the oscillator in Hz
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
            void trigger(float Frequency, LFO::start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) {
                this->Frequency = Frequency;
                this->ScriptFrequencyFactor = this->ScriptDepthFactor = 1.f; // reset for new voice
                this->InternalDepth = (InternalDepth / 1200.0f) * this->Max;
                this->ExtControlDepthCoeff = (((float) ExtControlDepth / 1200.0f) / 127.0f) * this->Max;
                this->pFinalDepth = NULL;
                this->pFinalFrequency = NULL;

                const float freq = Frequency * this->ScriptFrequencyFactor;
                c = 2.0f * M_PI * freq / (float) SampleRate;

                switch (StartLevel) {
                    case LFO::start_level_mid:
                        this->startPhase = (FlipPhase) ? 0.5 * M_PI : 1.5 * M_PI; // 90° or 270°
                        break;
                    case LFO::start_level_max:
                        this->startPhase = (FlipPhase) ? M_PI : 0.0; // 180° or 0°
                        break;
                    case LFO::start_level_min:
                        this->startPhase = (FlipPhase) ? 0.0 : M_PI; // 0° or 180°
                        break;
                }
                real = cos(this->startPhase);
                imag = sin(this->startPhase);
            }

            /**
             * Should be invoked after the LFO is triggered with StartLevel
             * start_level_min.
             * @param phase From 0 to 360 degrees.
             */
            void setPhase(float phase) {
                if (phase < 0) phase = 0;
                if (phase > 360) phase = 360;
                phase = phase / 360.0f * 2 * M_PI;
                real = cos(this->startPhase + phase);
                imag = sin(this->startPhase + phase);
            }

            void setFrequency(float Frequency, unsigned int SampleRate) {
                this->Frequency = Frequency;
                const float freq = Frequency * this->ScriptFrequencyFactor;
                c = 2.0f * M_PI * freq / (float) SampleRate;
            }

            void setScriptDepthFactor(float factor, bool isFinal) {
                this->ScriptDepthFactor = factor;
                // set or reset this script depth parameter to be the sole
                // source for the LFO depth
                if (isFinal && !this->pFinalDepth)
                    this->pFinalDepth = &this->ScriptDepthFactor;
                else if (!isFinal && this->pFinalDepth == &this->ScriptDepthFactor)
                    this->pFinalDepth = NULL;
                // recalculate upon new depth
                updateByMIDICtrlValue(this->ExtControlValue);
            }

            void setScriptFrequencyFactor(float factor, unsigned int SampleRate) {
                this->ScriptFrequencyFactor = factor;
                // in case script frequency was set as "final" value before,
                // reset it so that all sources are processed from now on
                if (this->pFinalFrequency == &this->ScriptFrequencyFactor)
                    this->pFinalFrequency = NULL;
                // recalculate upon new frequency
                setFrequency(this->Frequency, SampleRate);
            }

            void setScriptFrequencyFinal(float hz, unsigned int SampleRate) {
                this->ScriptFrequencyFactor = hz;
                // assign script's given frequency as sole source for the LFO
                // frequency, thus ignore all other sources
                if (!this->pFinalFrequency)
                    this->pFinalFrequency = &this->ScriptFrequencyFactor;
                // recalculate upon new frequency
                setFrequency(this->Frequency, SampleRate);
            }

        private:
            float c;
            float real;
            float imag;
            float normalizer;
            float offset;
            double startPhase;
    };

} // namespace LinuxSampler

#endif // LS_LFOSINE_NUMERIC_COMPLEX_NR_H

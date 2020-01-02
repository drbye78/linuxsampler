/*
 * Copyright (c) 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_LFOSAWINTMATHNEW_H
#define LS_LFOSAWINTMATHNEW_H

#include <stdlib.h>
#include "LFOBase.h"

namespace LinuxSampler {

    /** @brief Saw LFO (int math implementation)
     *
     * This is a Saw Low Frequency Oscillator which uses pure integer
     * math (without branches) to synthesize the saw wave.
     */
    template<LFO::range_type_t RANGE>
    class LFOSawIntMathNew : public LFOBase<RANGE> {
        public:
            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOSawIntMathNew(float Max) : LFOBase<RANGE>::LFOBase(Max) {
                //NOTE: DO NOT add any custom initialization here, since it would break LFOCluster construction !
            }

            /**
             * Calculates exactly one sample point of the LFO wave.
             *
             * @returns next LFO level
             */
            inline float render() {
                this->slope += this->c;
                if (RANGE == LFO::range_unsigned)
                    return this->denormalizer * float(this->slope);
                else // signed range
                    return this->denormalizer * float(int(this->slope) + this->offset);
            }

            /**
             * Update LFO depth with a new external controller value.
             *
             * @param ExtControlValue - new external controller value
             */
            inline void updateByMIDICtrlValue(const uint16_t& ExtControlValue) {
                this->ExtControlValue = ExtControlValue;

                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                const float max = (this->InternalDepth + ExtControlValue * this->ExtControlDepthCoeff) * this->ScriptDepthFactor;
                if (RANGE == LFO::range_unsigned) {
                    denormalizer = max / (float) intLimit / 2.0;
                } else { // signed range
                    denormalizer = max / (float) intLimit * 2.0;
                    offset       = -max;
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
                this->Frequency            = Frequency;
                this->InternalDepth        = (InternalDepth / 1200.0f) * this->Max;
                this->ExtControlDepthCoeff = (((float) ExtControlDepth / 1200.0f) / 127.0f) * this->Max;
                this->ScriptFrequencyFactor = this->ScriptDepthFactor = 1.f; // reset for new voice
                if (RANGE == LFO::range_unsigned) {
                    this->InternalDepth        *= 2.0f;
                    this->ExtControlDepthCoeff *= 2.0f;
                }
                this->pFinalDepth = NULL;
                this->pFinalFrequency = NULL;

                this->flipPhaseFactor = FlipPhase ? -1 : 1;
                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                const float freq = Frequency * this->ScriptFrequencyFactor;
                const float r = freq / (float) SampleRate; // frequency alteration quotient
                c = int(intLimit * r * this->flipPhaseFactor);

                const int slopeAtMid = (RANGE == LFO::range_unsigned) ? intLimit / 2 : intLimit;
                const int slopeAtMin = (RANGE == LFO::range_unsigned) ? intLimit : intLimit / 2;

                switch (StartLevel) {
                    case LFO::start_level_mid:
                        //slope = (FlipPhase) ? slopeAtMin : slopeAtMid;
                        slope = slopeAtMid;
                        break;

                    // with saw function, min and max are actually always the same thing
                    // (and no matter if FlipPhase or not)
                    case LFO::start_level_max:
                    case LFO::start_level_min:
                        slope = slopeAtMin;
                        break;
                }
            }

            /**
             * Should be invoked after the LFO is triggered.
             * @param phase From 0 to 360 degrees.
             */
            void setPhase(float phase) {
                if (phase < 0) phase = 0;
                if (phase > 360) phase = 360;
                phase /= 360.0f;
                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                slope += intLimit * phase * flipPhaseFactor;
            }

            void setFrequency(float Frequency, unsigned int SampleRate) {
                this->Frequency = Frequency;
                const float freq = Frequency * this->ScriptFrequencyFactor;
                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                float r = freq / (float) SampleRate; // frequency alteration quotient
                c = int(intLimit * r * this->flipPhaseFactor);
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

        protected:
            unsigned int slope;
            unsigned int c;
            float offset; ///< only needed for signed range
            float denormalizer;
            float flipPhaseFactor; ///< Factor instead of boolean to avoid branches.
    };

} // namespace LinuxSampler

#endif // LS_LFOSAWINTMATHNEW_H

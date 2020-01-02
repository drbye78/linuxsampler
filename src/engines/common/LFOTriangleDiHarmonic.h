/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 - 2019 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_LFOTRIANGLEDIHARMONIC_H__
#define __LS_LFOTRIANGLEDIHARMONIC_H__

#include "LFOBase.h"

// amplitue of 2nd harmonic (to approximate the triangular wave)
#define AMP2	-0.11425509f

namespace LinuxSampler {

    /** @brief Triangle LFO (di-harmonic implementation)
     *
     * This is a triangle Low Frequency Oscillator implementation which uses
     * a di-harmonic solution. This means it sums up two harmonics
     * (sinusoids) to approximate a triangular wave.
     *
     * @deprecated This class will probably be removed in future. Reason: The
     * resulting wave form is not similar enough to a triangular wave. to
     * achieve a more appropriate triangular wave form, this class would need
     * to use more harmonics, but that in turn would make runtime performance of
     * this class even worse. And since it currently seems to perform worst
     * already among all triangular wave implementations on all known
     * architectures, doing that required harmonics change currently does not
     * make sense. Furthermore the detailed behaviour of the other triangular
     * LFO implementations had been fixed in the meantime; this one not.
     */
    template<LFO::range_type_t RANGE>
    class DEPRECATED_API LFOTriangleDiHarmonic : public LFOBase<RANGE> {
        public:

            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOTriangleDiHarmonic(float Max) : LFOBase<RANGE>::LFOBase(Max) {
                //NOTE: DO NOT add any custom initialization here, since it would break LFOCluster construction !
            }

            /**
             * Calculates exactly one sample point of the LFO wave.
             *
             * @returns next LFO level
             */
            inline float render() {
                real1 -= c1 * imag1;
                imag1 += c1 * real1;
                real2 -= c2 * imag2;
                imag2 += c2 * real2;
                if (RANGE == LFO::range_unsigned)
                    return (real1 + real2 * AMP2) * normalizer + offset;
                else /* signed range */
                    return (real1 + real2 * AMP2) * normalizer;
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
                    const float harmonicCompensation = 1.0f + fabsf(AMP2); // to compensate the compensation ;) (see trigger())
                    normalizer = max * 0.5f;
                    offset     = normalizer * harmonicCompensation;
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
                const float harmonicCompensation = 1.0f + fabsf(AMP2); // to compensate the 2nd harmonic's amplitude overhead
                this->InternalDepth        = (InternalDepth / 1200.0f) * this->Max / harmonicCompensation;
                this->ExtControlDepthCoeff = (((float) ExtControlDepth / 1200.0f) / 127.0f) * this->Max / harmonicCompensation;
                this->pFinalDepth = NULL;
                this->pFinalFrequency = NULL;

                const float freq = Frequency * this->ScriptFrequencyFactor;
                c1 = 2.0f * M_PI * freq / (float) SampleRate;
                c2 = 2.0f * M_PI * freq / (float) SampleRate * 3.0f;

                double phi; // phase displacement
                switch (StartLevel) {
                    case LFO::start_level_mid:
                        //FIXME: direct jumping to 90° and 270° doesn't work out due to numeric accuracy problems (causes wave deformation)
                        //phi = (FlipPhase) ? 0.5 * M_PI : 1.5 * M_PI; // 90° or 270°
                        //break;
                    case LFO::start_level_max:
                        phi = (FlipPhase) ? M_PI : 0.0; // 180° or 0°
                        break;
                    case LFO::start_level_min:
                        phi = (FlipPhase) ? 0.0 : M_PI; // 0° or 180°
                        break;
                }
                real1 = real2 = cos(phi);
                imag1 = imag2 = sin(phi);
            }
            
            /**
             * Should be invoked after the LFO is triggered with StartLevel
             * start_level_min.
             * @param phase From 0 to 360 degrees.
             */
            void setPhase(float phase) {
                if (phase < 0) phase = 0;
                if (phase > 360) phase = 360;
                phase /= 360.0f;
                
                // FIXME: too heavy?
                float steps = 1.0f / (c1 / (2.0f * M_PI)); // number of steps for one cycle
                steps *= phase + 0.25f;
                for (int i = 0; i < steps; i++) render();
            }
            
            void setFrequency(float Frequency, unsigned int SampleRate) {
                this->Frequency = Frequency;
                const float freq = Frequency * this->ScriptFrequencyFactor;
                c1 = 2.0f * M_PI * freq / (float) SampleRate;
                c2 = 2.0f * M_PI * freq / (float) SampleRate * 3.0f;
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
            float c1;
            float c2;
            float real1;
            float imag1;
            float real2;
            float imag2;
            float normalizer;
            float offset;
    };

} // namespace LinuxSampler

#endif // __LS_LFOTRIANGLEDIHARMONIC_H__

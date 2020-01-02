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

#ifndef __LS_LFOBASE_H__
#define __LS_LFOBASE_H__

#include "../LFO.h"
#include "../../common/RTMath.h"

#include <math.h>
#include <stdint.h>

namespace LinuxSampler {

    /** @brief POD base for all LFO implementations.
     *
     * This structure acts as POD (Plain Old Data structure) base for all LFO
     * implementations. For efficiency reasons all our LFOs are C++ template
     * classes. By deriving all those LFO C++ templase classes from this
     * POD structure, it allows us to at least do a unified @code delete for
     * any LFO object, which would not work on @code void* pointers (correctly).
     *
     * ATM this is actually only used in LFO.cpp.
     */
    struct LFOPOD {
    };

    /** @brief LFO (abstract base class)
     * 
     * Abstract base class for all Low Frequency Oscillator implementations.
     */
    template<LFO::range_type_t RANGE>
    class LFOBase : public LFOPOD {
        public:

            // *************** attributes ***************
            // *

            uint8_t ExtController; ///< MIDI control change controller number if the LFO is controlled by an external controller, 0 otherwise.


            // *************** methods ***************
            // *

            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOBase(float Max) {
                this->ExtController = 0;
                this->Max           = Max;
                this->InternalDepth = 0;
                this->Frequency = 20.f;
                this->ExtControlValue = 0;
                this->ExtControlDepthCoeff = 0;
                this->ScriptDepthFactor = 1.f;
                this->ScriptFrequencyFactor = 1.f;
                this->pFinalDepth = NULL;
                this->pFinalFrequency = NULL;
            }

            /**
             * Calculates exactly one sample point of the LFO wave. This
             * inline method has to be implemented by the descendant.
             *
             * @returns next LFO level
             */
            //abstract inline float render(); //< what a pity that abstract inliners are not supported by C++98 (probably by upcoming C++0x?)

            /**
             * Update LFO depth with a new external controller value. This
             * inline method has to be implemented by the descendant.
             *
             * @param ExtControlValue - new external controller value
             */
            //abstract inline void updateByMIDICtrlValue(const uint16_t& ExtControlValue); //< what a pity that abstract inliners are not supported by C++98 (probably by upcoming C++0x?)

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
            virtual void trigger(float Frequency, LFO::start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) = 0;

        protected:
            float Max;
            float InternalDepth;
            float Frequency; ///< Internal base frequency for this LFO.
            float ExtControlValue; ///< The current external MIDI controller value (0-127).
            float ExtControlDepthCoeff; ///< A usually constant factor used to convert a new MIDI controller value from range 0-127 to the required internal implementation dependent value range.
            float ScriptDepthFactor; ///< Usually neutral (1.0), only altered by external RT instrument script functions.
            float ScriptFrequencyFactor; ///< Usually neutral (1.0), only altered by external RT instrument script functions.
            float* pFinalDepth; ///< Usually NULL; it may be set to one of above's member variables in order to process that and ignore all other sources for LFO depth.
            float* pFinalFrequency; ///< Usually NULL; it may be set to one of above's member variables in order to process that and ignore all other sources for LFO frequency.
    };

} // namespace LinuxSampler

#endif // __LS_LFOBASE_H__

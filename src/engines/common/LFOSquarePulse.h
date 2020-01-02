/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2011 Grigor Iliev                                       *
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

#ifndef LS_LFO_SQUARE_PULSE_H
#define LS_LFO_SQUARE_PULSE_H

#include "LFOPulse.h"

namespace LinuxSampler {

    /** @brief Square LFO (using specialized Pulse LFO as implementation).
     */
    template<LFO::range_type_t RANGE>
    class LFOSquarePulse : public LFOPulse<RANGE, 500> {
    public:
        LFOSquarePulse(float Max) : LFOPulse<RANGE, 500>::LFOPulse(Max) {
            //NOTE: DO NOT add any custom initialization here, since it would break LFOCluster construction !
        }
    };

} // namespace LinuxSampler

#endif // LS_LFO_SQUARE_PULSE_H

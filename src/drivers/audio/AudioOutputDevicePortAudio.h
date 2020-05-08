/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2013 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef _LS_AUDIOOUTPUTDEVICEPORTAUDIO_H_
#define _LS_AUDIOOUTPUTDEVICEPORTAUDIO_H_

#include <codecvt>
#include <string>
#include <string.h>

#include <math.h>
#include <locale>
#include "portaudio.h"
#include <exception>

#include "../../common/global_private.h"
#include "AudioOutputDevice.h"
#include "AudioChannel.h"
#include "../DeviceParameter.h"

namespace LinuxSampler { //
	
    class AudioOutputDevicePortAudio : public AudioOutputDevice {
    public:
        AudioOutputDevicePortAudio(std::map<String, DeviceCreationParameter*> Parameters) throw (Exception);
        ~AudioOutputDevicePortAudio();

        // derived abstract methods from class 'AudioOutputDevice'
        virtual void Play() OVERRIDE;
        virtual bool IsPlaying() OVERRIDE;
        virtual void Stop() OVERRIDE;
        virtual uint MaxSamplesPerCycle() OVERRIDE;
        virtual uint SampleRate() OVERRIDE;
        virtual AudioChannel* CreateChannel(uint ChannelNr) OVERRIDE;
        virtual String Driver() OVERRIDE;
        virtual float latency() OVERRIDE;


        static String Name();
        static String Description();
        static String Version();

        template<typename _Index, typename _Result>
        class PaTraits {
        public:
            typedef std::pair<_Index, _Result> Pair;
            typedef _Result ValueType;
        	
            static int GetCount();
            static _Result GetItem(_Index index);
            static _Index GetDefault();
            static const char* ParamName();
            static const char* Prefix();

            static String SafeValue(_Index index, _Result info)
            {
                return SafeStr(info->name, Prefix(), index);
            }

            static Pair GetValue(String value)
            {
                auto len = value.length();
                if (len > 1) {
                    if ((value[0] == '\'') && (value[len - 1] == '\'')) {
                        value = value.substr(1, len - 2);
                    }
                }

                const auto count = GetCount();
                for (auto i = 0; i < count; i++)
                {
                    const auto info = GetItem(i);
                    if (info != nullptr && value.compare(SafeStr(info->name, Prefix(), i)) == 0)
                        return { i, info };
                }
                return { paNoDevice, nullptr };
            }

            static Pair FromParameters(const std::map<String, String>& Parameters)
            {
                const auto it = Parameters.find(ParamName());
                if (it != Parameters.end())
                    return GetValue(it->second);
                else {
                    auto index = GetDefault();
                    if (index >= 0) {
                        auto value = GetItem(index);
                        if (value != nullptr)
                            return { index, value };
                    }
                }
                return { paNoDevice, nullptr };
            }
        };

        using TraitsInterface = PaTraits<PaHostApiIndex, const PaHostApiInfo*>;
        using TraitsDevice = PaTraits<PaDeviceIndex, const PaDeviceInfo*>;

        template<typename _Traits>
        class IndexedParameter : public DeviceCreationParameterString {
        public:
            typedef _Traits Traits;

            IndexedParameter() : DeviceCreationParameterString()
            {
                InitWithDefault();
            }

            IndexedParameter(String s) throw (Exception) : DeviceCreationParameterString(s)
            {
            }

            virtual bool Fix() OVERRIDE
            {
                return true;
            }

            virtual bool Mandatory() OVERRIDE
            {
                return true;
            }
            
            static String Name()
            {
                return Traits::ParamName();
            }

            static inline auto FromParams(const std::map<String, String>& Parameters)
            {
                return Traits::FromParameters(Parameters);
            }

            optional<String> DefaultAsString(std::map<String, String> Parameters)
            {
                ApiLock lock;
                optional<String> result;
                const auto index = Traits::GetDefault();
                if (index >= 0)
                {
                    const auto info = Traits::GetItem(index);
                    if (info != nullptr)
                    {
                        result = Traits::SafeValue(index, info);
                    }
                }
                return result;
            }

            std::vector<String> PossibilitiesAsString(std::map<String, String> Parameters)
            {
                ApiLock lock;
                std::vector<String> list;
                const auto count = Traits::GetCount();
                for (auto i = 0; i < count; i++)
                {
                    const auto info = Traits::GetItem(i);
                    if (info != nullptr)
                    {
                        list.push_back(Traits::SafeValue(i, info));
                    }
                }
                return list;
            }
        };

        /** Device Parameter 'Interface'
            *
            * Used to select the desired Asio sound card.
            */
        class ParameterInterface : public IndexedParameter<TraitsInterface> {
        public:
            ParameterInterface() : IndexedParameter() {}
            ParameterInterface(String s) : IndexedParameter(s) {}

            virtual String Description() OVERRIDE;
            virtual std::map<String, DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
            virtual void OnSetValue(String s) throw (Exception)OVERRIDE;
        };

        /** Device Parameter 'CARD'
            *
            * Used to select the desired Asio sound card.
            */
        class ParameterCard : public IndexedParameter <TraitsDevice> {
        public:
            ParameterCard() : IndexedParameter() {}
            ParameterCard(String s) throw (Exception) : IndexedParameter(s) {}

            virtual String Description() OVERRIDE;
            virtual std::map<String, DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
            virtual optional<String>    DefaultAsString(std::map<String, String> Parameters) OVERRIDE;
            virtual std::vector<String> PossibilitiesAsString(std::map<String, String> Parameters) OVERRIDE;
            virtual void                OnSetValue(String s) throw (Exception)OVERRIDE;
        };

    	template <typename _Base>
        class CardParameter : public _Base
        {
        public:
            CardParameter() : _Base() 
            {
            }

            CardParameter(String s) : _Base(s) {}
    		
            virtual std::map<String, DeviceCreationParameter*> DependsAsParameters() OVERRIDE
    		{
            //    return { {ParameterInterface::Name(), nullptr}, {ParameterCard::Name(), nullptr} };
                return {  {ParameterCard::Name(), nullptr} };
            }
    		
            virtual bool Fix() OVERRIDE
    		{
                return true;
            }
    		
            virtual bool Mandatory() OVERRIDE
    		{
                return true;
    		}

    		TraitsDevice::Pair FromParams(const std::map<String, String>& Parameters)
            {
                return TraitsDevice::FromParameters(Parameters);
            }
    	};
    	
        /** Device Parameter 'SAMPLERATE'
            *
            * Used to set the sample rate of the audio output device.
            */
        class ParameterSampleRate : public CardParameter<AudioOutputDevice::ParameterSampleRate> {
        public:
            ParameterSampleRate() : CardParameter() {}
            ParameterSampleRate(String s) : CardParameter(s) {}
            virtual optional<int> DefaultAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual std::vector<int> PossibilitiesAsInt(std::map<String, String> Parameters) OVERRIDE;
        };

        /** Device Parameters 'CHANNELS'
            *
            * Used to increase / decrease the number of audio channels of
            * audio output device.
            */
        class ParameterChannels : public CardParameter<AudioOutputDevice::ParameterChannels> {
        public:
            ParameterChannels() : CardParameter() {}
            ParameterChannels(String s) : CardParameter(s) {}
            virtual optional<int> DefaultAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual optional<int> RangeMinAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual optional<int> RangeMaxAsInt(std::map<String, String> Parameters) OVERRIDE;
        };


        /** Device Parameter 'FRAGMENTSIZE'
            *
            * Used to set the audio fragment size / period size.
            */
        class ParameterFragmentSize : public CardParameter<DeviceCreationParameterInt> {
        public:
            ParameterFragmentSize() : CardParameter() {}
            ParameterFragmentSize(String s) : CardParameter(s) {}
            virtual String Description() OVERRIDE;
            virtual optional<int>    DefaultAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual optional<int>    RangeMinAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual optional<int>    RangeMaxAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual std::vector<int> PossibilitiesAsInt(std::map<String, String> Parameters) OVERRIDE;
            virtual void             OnSetValue(int value) throw (Exception)OVERRIDE;
            static String Name();
        };

        // make protected methods public
        using AudioOutputDevice::RenderAudio;
        using AudioOutputDevice::Channels;

    public:
        class PaException : public Exception
        {
        private:
            PaError _error;
            String _reason;

        public:
            PaException(const String& cause, PaError errorCode) : Exception(getMessage()), _error(errorCode), _reason(cause)
            {
                dmsg(1, ("PortAudio: %s\n", getMessage().c_str()));
            }

            String getMessage() const
            {
                return Pa_GetErrorText(_error);
            }

            const String& getReason() const
            {
                return _reason;
            }
        };

    private:
        static Mutex apiMutex;
        static atomic<int> apiLocks;

        static inline void apiCheck(PaError errorCode, const char* reason)
        {
            if (errorCode < 0)
                throw PaException(String(reason) + " - " + Pa_GetErrorText(errorCode), errorCode);
        }

        static inline bool apiWarn(PaError errorCode, const char* reason)
        {
            if (errorCode < 0) {
                dmsg(1, ("%s - %s\n", reason, Pa_GetErrorText(errorCode)));
                return false;
            }
            else
                return true;
        }


        class ApiLock
        {
        public:
            ApiLock()
            {
                AudioOutputDevicePortAudio::acquireApi();
            }

            ~ApiLock()
            {
                AudioOutputDevicePortAudio::releaseApi();
            }
        };

        static void acquireApi();
        static void releaseApi();

        static inline std::wstring FromUtf8(const String& source)
        {
            return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(source);
        }

    private:
       static inline String SafeStr(const char* lpSz, const char* prefix, int ident)
        {
       		if (lpSz == nullptr)
                return String(prefix) + ToString(ident);

            return lpSz;
        }

        PaDeviceIndex _device;
        PaStream* _stream;
        int _channels;
        uint _sampleRate;
        float _fragment;
        atomic<bool> _shouldStop;
        uint _fragmentSize;

        template<typename _Fmt> void Render(const void* input, _Fmt* output, unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlag);

        template<typename _Fmt> static int StreamCallback(const void* input, void* output, unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
        {
            auto device = static_cast<AudioOutputDevicePortAudio*>(userData);
            if (device->_shouldStop)
                return paComplete;

            auto res = device->RenderAudio(frameCount);
            if (res != 0)
                printf("RENDER: %.5f %.5f, %p, %ld, %d --> %d\n", timeInfo->currentTime, timeInfo->outputBufferDacTime, output,
                    frameCount, static_cast<int>(statusFlags), res);

            device->Render(input, static_cast<_Fmt*>(output), frameCount, timeInfo, statusFlags);
            return paContinue;
        }
    };
}

#endif // _LS_AudioOutputDevicePortAudio_H_

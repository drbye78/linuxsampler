#include "AudioOutputDevicePortAudio.h"

#include "AudioOutputDeviceFactory.h"
#include "portaudio.h"
#include "../portaudio/src/common/pa_debugprint.h"


namespace LinuxSampler
{
	Mutex AudioOutputDevicePortAudio::apiMutex;
	atomic<int> AudioOutputDevicePortAudio::apiLocks;

	REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDevicePortAudio);
	/* Common parameters for now they'll have to be registered here. */
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterActive);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterSampleRate);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterChannels);
	/* Driver specific parameters */
	//REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterInterface);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterCard);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterFragmentSize);

	#define DEFAULT_RATE 44100
	#define DEFAULT_LATENCY 50.0
	#define DEFAULT_CHANNELS 2
	#define LATENCY_SCALE 1000.0f

	static std::vector PossibleRates = { 22500, 44100, 48000, 96000 };

	static bool bootstrap = true;

	void PaLogCallback(const char* logmsg)
	{
		dmsg(6, ("%s\n", logmsg));
	}

	void AudioOutputDevicePortAudio::acquireApi()
	{
		LockGuard lock(apiMutex);

		if (apiLocks == 0)
		{
			if (bootstrap)
			{
				PaUtil_SetDebugPrintFunction(&PaLogCallback);
				apiLocks++;
				bootstrap = false;
			}
			const auto res = Pa_Initialize();
			apiCheck(res, "PortAudio failed to initialize");
		}
		apiLocks++;
	}

	void AudioOutputDevicePortAudio::releaseApi()
	{
		LockGuard lock(apiMutex);
		if (--apiLocks == 0)
		{
			Pa_Terminate();
		}
	}


	AudioOutputDevicePortAudio::AudioOutputDevicePortAudio(std::map<String, DeviceCreationParameter*> Parameters) :
		AudioOutputDevice(Parameters), _device(paNoDevice), _stream(nullptr)
	{
		ApiLock lock;

		const auto sampleRate = static_cast<ParameterSampleRate*>(Parameters.at(ParameterSampleRate::Name()));
		const auto channels = static_cast<ParameterChannels*>(Parameters.at(ParameterChannels::Name()));
		const auto fragmentSize = static_cast<ParameterFragmentSize*>(Parameters.at(ParameterFragmentSize::Name()));

		_device = paNoDevice;
		const auto card = static_cast<ParameterCard*>(Parameters.at(ParameterCard::Name()));
		if (card != nullptr)
		{
			const auto cardInfo = TraitsDevice::GetValue(card->ValueAsString());
			if (cardInfo.second != nullptr)
				_device = cardInfo.first;
			else
				_device = Pa_GetDefaultOutputDevice();
		}
		else
		{
			const auto host = static_cast<ParameterInterface*>(Parameters.at(ParameterInterface::Name()));
			if (host != nullptr)
			{
				auto hostInfo = TraitsInterface::GetValue(host->ValueAsString());
				if (hostInfo.second != nullptr)
					_device = hostInfo.second->defaultOutputDevice;
			}
			else
			{
				_device = Pa_GetDefaultOutputDevice();
			}
		}

		if (_device < 0)
			throw PaException("Failed to detect output device", _device);

		_sampleRate = sampleRate->ValueAsInt();
		_channels = channels->ValueAsInt();
		_fragment = fragmentSize->ValueAsInt() / LATENCY_SCALE;
		_shouldStop = true;

		_fragmentSize = 1 << static_cast<int>(log2(_sampleRate * _fragment));
		
		PaStreamParameters streamParams = {_device, _channels, paFloat32, _fragment, nullptr};
		auto res = Pa_IsFormatSupported(nullptr, &streamParams, _sampleRate);
		auto handler = &StreamCallback<float>;
		if (res < 0) {
			streamParams.sampleFormat = paInt16;
			res = Pa_IsFormatSupported(nullptr, &streamParams, _sampleRate);
			if (res < 0)
				throw PaException("output audio format is unsupported", res);
			handler = &StreamCallback<int16_t>;
		}

		res = Pa_OpenStream(&_stream, nullptr, &streamParams, _sampleRate, _fragmentSize,
		                    paDitherOff, handler, this);
		if (res < 0)
			throw PaException("Failed to create output stream", res);

		const auto size = MaxSamplesPerCycle();
		for (auto i = 0; i < _channels; i++)
		{
			Channels.push_back(new AudioChannel(i, size));
		}

		auto deviceInfo = Pa_GetDeviceInfo(_device);
		auto deviceName = TraitsDevice::SafeValue(_device, deviceInfo);
		dmsg(2, ("PortAudio: successfully opened device '%s', format = %dHz, %d channels, %d samples\n", deviceName.c_str(), _sampleRate, _channels, _fragmentSize));

		if (static_cast<DeviceCreationParameterBool*>(Parameters["ACTIVE"])->ValueAsBool())
		{
			Play();
		}

		apiLocks++;
	}

	AudioOutputDevicePortAudio::~AudioOutputDevicePortAudio()
	{
		ApiLock lock;

		if (_stream != nullptr)
		{
			Stop();
			Pa_AbortStream(_stream);
			Pa_CloseStream(_stream);
			_stream = nullptr;
		}
		apiLocks--;
	}

	void AudioOutputDevicePortAudio::Play()
	{
		if (_stream != nullptr)
		{
			_shouldStop = false;
			if (Pa_IsStreamStopped(_stream) == 1)
				apiWarn(Pa_StartStream(_stream), "Failed to start stream");
		}
	}

	template<> void AudioOutputDevicePortAudio::Render(const void* input, float* output, unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlag)
	{
		float mm = -5;
		const auto channels = ChannelCount();
		auto work = output;
		for (auto i = 0; i < channels; i++)
		{
			auto dest = work++;
			auto src = Channels[i]->Buffer();
			for (auto n = 0; n < frameCount; n++, dest += channels) {
				*dest = *src++;
				if (*dest > mm) mm = *dest;
			}
		}
		//printf("%.3f %d %.3f\n", timeInfo->outputBufferDacTime, (int)frameCount, mm);
	}

	template<> void AudioOutputDevicePortAudio::Render(const void* input, int16_t* output, unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlag)
	{
		const auto channels = ChannelCount();
		auto work = output;
		for (auto i = 0; i < channels; i++)
		{
			auto dest = work++;
			auto src = Channels[i]->Buffer();
			for (auto n = 0; n < frameCount; n++, dest += channels)
				*dest = (int16_t)std::clamp((int)(*src++ * 32768.0f), -32767, 32768);
		}
	}

	bool AudioOutputDevicePortAudio::IsPlaying()
	{
		return (_stream != nullptr) && (Pa_IsStreamActive(_stream) == 1);
	}

	void AudioOutputDevicePortAudio::Stop()
	{
		if (_stream != nullptr)
		{
			_shouldStop = true;
			if (Pa_IsStreamActive(_stream) == 1)
				apiWarn(Pa_StopStream(_stream), "Failed to stop stream");
		}
	}

	float AudioOutputDevicePortAudio::latency()
	{
		return _fragment;
	}

	uint AudioOutputDevicePortAudio::MaxSamplesPerCycle()
	{
		return _fragmentSize;
	}

	uint AudioOutputDevicePortAudio::SampleRate()
	{
		return _sampleRate;
	}

	AudioChannel* AudioOutputDevicePortAudio::CreateChannel(uint ChannelNr)
	{
		return new AudioChannel(ChannelNr, Channel(ChannelNr % _channels));
	}

	String AudioOutputDevicePortAudio::Driver()
	{
		return Name();
	}

	String AudioOutputDevicePortAudio::Name()
	{
		return "PortAudio";
	}

	String AudioOutputDevicePortAudio::Description()
	{
		return "PortAudio driver";
	}

	String AudioOutputDevicePortAudio::Version()
	{
		return Pa_GetVersionText();
	}

	/////////////////////
	// 'INTERFACE'
	/////////////////////

	int AudioOutputDevicePortAudio::TraitsInterface::GetCount()
	{
		return Pa_GetHostApiCount();
	}

	const PaHostApiInfo* AudioOutputDevicePortAudio::TraitsInterface::GetItem(PaHostApiIndex index)
	{
		return Pa_GetHostApiInfo(index);
	}

	PaHostApiIndex AudioOutputDevicePortAudio::TraitsInterface::GetDefault()
	{
		return Pa_GetDefaultHostApi();
	}

	const char* AudioOutputDevicePortAudio::TraitsInterface::Prefix()
	{
		return "Host_";
	}

	const char* AudioOutputDevicePortAudio::TraitsInterface::ParamName()
	{
		return "Host";
	}

	String AudioOutputDevicePortAudio::ParameterInterface::Description()
	{
		return "Host interface to be used";
	}

	std::map<String, DeviceCreationParameter*> AudioOutputDevicePortAudio::ParameterInterface::DependsAsParameters()
	{
		return {};
	}

	void AudioOutputDevicePortAudio::ParameterInterface::OnSetValue(String s) throw (Exception)
	{
	}

	/////////////////////
	// 'CARD'
	/////////////////////

	int AudioOutputDevicePortAudio::TraitsDevice::GetCount()
	{
		return Pa_GetDeviceCount();
	}

	const PaDeviceInfo* AudioOutputDevicePortAudio::TraitsDevice::GetItem(PaDeviceIndex index)
	{
		return Pa_GetDeviceInfo(index);
	}

	PaDeviceIndex AudioOutputDevicePortAudio::TraitsDevice::GetDefault()
	{
		return Pa_GetDefaultOutputDevice();
	}

	const char* AudioOutputDevicePortAudio::TraitsDevice::Prefix()
	{
		return "Device_";
	}

	const char* AudioOutputDevicePortAudio::TraitsDevice::ParamName()
	{
		return "Card";
	}

	String AudioOutputDevicePortAudio::ParameterCard::Description()
	{
		return "Sound card to play audio";
	}

	std::map<String, DeviceCreationParameter*> AudioOutputDevicePortAudio::ParameterCard::DependsAsParameters()
	{
#ifdef USE_HOST
		return { {ParameterInterface::Name(),  nullptr} };
#else
		return {};
#endif
	}

	optional<String> AudioOutputDevicePortAudio::ParameterCard::DefaultAsString(std::map<String, String> Parameters)
	{
		ApiLock lock;
#if USE_HOST
		optional<String> result;
		const auto host = TraitsInterface::FromParameters(Parameters);
		if (host.second != nullptr && host.second->defaultOutputDevice >= 0) {
			const auto dev = TraitsDevice::GetItem(host.second->defaultOutputDevice);
			if (dev != nullptr)
			{
				result = TraitsDevice::SafeValue(host.second->defaultOutputDevice, dev);
			}
		}
		return result;
#else
		return IndexedParameter::DefaultAsString(Parameters);
#endif
	}

	std::vector<String> AudioOutputDevicePortAudio::ParameterCard::PossibilitiesAsString(
		std::map<String, String> Parameters)
	{
		ApiLock lock;
		std::vector<String> result;
#if USE_HOST
		const auto host = TraitsInterface::FromParameters(Parameters);
		PaHostApiIndex hostId;
		TraitsInterface::ValueType hostInfo;

		if (host.second != nullptr)
		{
			hostId = host.first;
			hostInfo = host.second;
		}
		else
		{
			hostId = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
			if (hostId < 0)
				hostId = Pa_GetDefaultOutputDevice();

			hostInfo = TraitsInterface::GetItem(hostId);
			if (hostInfo == nullptr)
				return result;
		}

		for (auto i = 0; i < hostInfo->deviceCount; i++)
		{
			const auto devIndex = Pa_HostApiDeviceIndexToDeviceIndex(hostId, i);
			const auto devInfo = TraitsDevice::GetItem(devIndex);
			if (devInfo != nullptr && devInfo->maxOutputChannels > 0)
				result.push_back(TraitsDevice::SafeValue(devIndex, devInfo));
		}
#else
		for(auto i = 0; i < TraitsDevice::GetCount(); i++)
		{
			const auto devInfo = TraitsDevice::GetItem(i);
			if (devInfo != nullptr && devInfo->maxOutputChannels > 1) {
				dmsg(1, ("PA ENUM: %d. %s\n", i, devInfo->name));
				result.push_back(TraitsDevice::SafeValue(i, devInfo));
			}
		}
#endif
		return result;
	}

	void AudioOutputDevicePortAudio::ParameterCard::OnSetValue(String s) throw (Exception)
	{
	}


	/////////////////////
	// 'SAMPLE RATE'
	/////////////////////

	optional<int> AudioOutputDevicePortAudio::ParameterSampleRate::DefaultAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;
		const auto card = FromParams(Parameters);
		if (card.second) 
			return static_cast<int>(card.second->defaultSampleRate); 
		else 
			return optional<int>::nothing;
	}

	std::vector<int> AudioOutputDevicePortAudio::ParameterSampleRate::PossibilitiesAsInt(
		std::map<String, String> Parameters)
	{
		ApiLock lock;
		std::vector<int> rates;
		const auto card = FromParams(Parameters);

		if (card.second != nullptr)
		{
			PaStreamParameters params = {card.first, 2, paFloat32, card.second->defaultLowOutputLatency, nullptr};
			for (auto rate : PossibleRates)
			{
				if (Pa_IsFormatSupported(nullptr, &params, rate) == 0)
					rates.push_back(rate);
			}
		}
		return rates;
	}


	/////////////////////
	// 'CHANNELS'
	/////////////////////

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::DefaultAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;
		const auto card = ParameterCard::FromParams(Parameters);
		if (card.second) 
			return card.second->maxOutputChannels; 
		else 
			return optional<int>::nothing;
	}

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::RangeMinAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;
		const auto card = FromParams(Parameters);
		if (card.second) 
			return 1; 
		else 
			return optional<int>::nothing;
	}

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::RangeMaxAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		if (card.second) 
			return card.second->maxOutputChannels; 
		else 
			return optional<int>::nothing;
	}

	// 'FRAGMENT SIZE'

	String AudioOutputDevicePortAudio::ParameterFragmentSize::Description()
	{
		return "Preferred latency during playback (ms)";
	}

	optional<int> AudioOutputDevicePortAudio::ParameterFragmentSize::DefaultAsInt(
		std::map<String, String> Parameters)
	{
		ApiLock lock;
		const auto card = FromParams(Parameters);
		if (card.second) 
			return card.second->defaultLowOutputLatency * LATENCY_SCALE;
		else
			return optional<int>::nothing;
	}

	optional<int> AudioOutputDevicePortAudio::ParameterFragmentSize::RangeMinAsInt(
		std::map<String, String> Parameters)
	{
		return DefaultAsInt(Parameters);
	}

	optional<int> AudioOutputDevicePortAudio::ParameterFragmentSize::RangeMaxAsInt(
		std::map<String, String> Parameters)
	{
		ApiLock lock;
		const auto card = FromParams(Parameters);
		if (card.second != nullptr) 
			return std::max(card.second->defaultHighOutputLatency * LATENCY_SCALE, 200.0);
		else
			return optional<int>::nothing;
	}

	std::vector<int> AudioOutputDevicePortAudio::ParameterFragmentSize::PossibilitiesAsInt(
		std::map<String, String> Parameters)
	{
		return {};
	}

	void AudioOutputDevicePortAudio::ParameterFragmentSize::OnSetValue(int value) throw (Exception)
	{
	}

	String AudioOutputDevicePortAudio::ParameterFragmentSize::Name()
	{
		return "Latency";
	}
}

#include "AudioOutputDevicePortAudio.h"

#include "AudioOutputDeviceFactory.h"
#include "portaudio.h"
#include "pa_debugprint.h"


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
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterInterface);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterCard);
	REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePortAudio, ParameterFragmentSize);

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
		_fragment = fragmentSize->ValueAsFloat();

		uint fragSize = 1 << int(log2(_sampleRate * _fragment));
		
		PaStreamParameters streamParams = {_device, _channels, paFloat32, _fragment, nullptr};
		auto res = Pa_IsFormatSupported(nullptr, &streamParams, _sampleRate);
		if (res < 0)
			throw PaException("output audio format is unsupported", res);

		res = Pa_OpenStream(&_stream, nullptr, &streamParams, _sampleRate, fragSize,
		                    paDitherOff, &SysStreamCallback, this);
		if (res < 0)
			throw PaException("Failed to create output stream", res);

		const auto size = MaxSamplesPerCycle();
		for (auto i = 0; i < _channels; i++)
		{
			Channels.push_back(new AudioChannel(i, size));
		}

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
			if (Pa_IsStreamStopped(_stream) == 1)
				apiCheck(Pa_StartStream(_stream), "Failed to start stream");
		}
	}

	int AudioOutputDevicePortAudio::StreamCallback(
		const void* input, void* output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags)
	{
		auto res = RenderAudio(frameCount);
		if (res != 0)
			printf("RENDER: %.5f %.5f, %p, %ld, %d --> %d\n", timeInfo->currentTime, timeInfo->outputBufferDacTime, output,
				frameCount, static_cast<int>(statusFlags), res);

		const auto channels = ChannelCount();
		auto work = static_cast<float*>(output);
		for (auto i = 0; i < channels; i++)
		{
			auto dest = work++;
			auto src = Channels[i]->Buffer();
			for (auto n = 0; n < frameCount; n++, dest += channels)
				*dest = *src++;
		}

		return paContinue;
	}


	bool AudioOutputDevicePortAudio::IsPlaying()
	{
		return (_stream != nullptr) && (Pa_IsStreamActive(_stream) == 0);
	}

	void AudioOutputDevicePortAudio::Stop()
	{
		if (_stream != nullptr)
		{
			if (Pa_IsStreamActive(_stream) == 1)
				apiCheck(Pa_StopStream(_stream), "Failed to stop stream");
		}
	}

	float AudioOutputDevicePortAudio::latency()
	{
		return _fragment;
	}

	uint AudioOutputDevicePortAudio::MaxSamplesPerCycle()
	{
		return uint(_fragment * _sampleRate * 4);
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

	// 'INTERFACE'
	int AudioOutputDevicePortAudio::TraitsInterface::GetCount()
	{
		return Pa_GetHostApiCount();
	}

	const PaHostApiInfo* AudioOutputDevicePortAudio::TraitsInterface::GetItem(PaHostApiIndex index)
	{
		return Pa_GetHostApiInfo(index);
	}

	const char* AudioOutputDevicePortAudio::TraitsInterface::Prefix()
	{
		return "Host_";
	}


	String AudioOutputDevicePortAudio::ParameterInterface::Name()
	{
		return "Host";
	}

	AudioOutputDevicePortAudio::ParameterInterface::ParameterInterface() : DeviceCreationParameterString()
	{
		InitWithDefault();
	}

	AudioOutputDevicePortAudio::ParameterInterface::ParameterInterface(String s) : DeviceCreationParameterString(s)
	{
	}

	String AudioOutputDevicePortAudio::ParameterInterface::Description()
	{
		return "Host interface to be used";
	}

	bool AudioOutputDevicePortAudio::ParameterInterface::Fix()
	{
		return false;
	}

	bool AudioOutputDevicePortAudio::ParameterInterface::Mandatory()
	{
		return true;
	}

	std::map<String, DeviceCreationParameter*> AudioOutputDevicePortAudio::ParameterInterface::DependsAsParameters()
	{
		return std::map<String, DeviceCreationParameter*>();
	}

	optional<String> AudioOutputDevicePortAudio::ParameterInterface::DefaultAsString(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto res = Pa_GetDefaultHostApi();
		optional<String> result;
		if (res >= 0)
		{
			const auto info = TraitsInterface::GetItem(res);
			if (info != nullptr)
			{
				result = TraitsInterface::SafeValue(res, info);
			}
		}
		return result;
	}

	std::vector<String> AudioOutputDevicePortAudio::ParameterInterface::PossibilitiesAsString(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto count = TraitsInterface::GetCount();
		std::vector<String> list;
		for (auto i = 0; i < count; i++)
		{
			const auto info = TraitsInterface::GetItem(i);
			if (info != nullptr)
			{
				list.push_back(TraitsInterface::SafeValue(i, info));
			}
		}
		return list;
	}

	void AudioOutputDevicePortAudio::ParameterInterface::OnSetValue(String s) throw (Exception)
	{
	}

	// 'CARD'

	int AudioOutputDevicePortAudio::TraitsDevice::GetCount()
	{
		return Pa_GetDeviceCount();
	}

	const PaDeviceInfo* AudioOutputDevicePortAudio::TraitsDevice::GetItem(PaHostApiIndex index)
	{
		return Pa_GetDeviceInfo(index);
	}

	const char* AudioOutputDevicePortAudio::TraitsDevice::Prefix()
	{
		return "Device_";
	}

	String AudioOutputDevicePortAudio::ParameterCard::Name()
	{
		return "Card";
	}

	AudioOutputDevicePortAudio::ParameterCard::ParameterCard() : DeviceCreationParameterString()
	{
		InitWithDefault();
	}

	AudioOutputDevicePortAudio::ParameterCard::ParameterCard(String s) : DeviceCreationParameterString(s)
	{
	}

	String AudioOutputDevicePortAudio::ParameterCard::Description()
	{
		return "Sound card to play audio";
	}

	bool AudioOutputDevicePortAudio::ParameterCard::Fix()
	{
		return false;
	}

	bool AudioOutputDevicePortAudio::ParameterCard::Mandatory()
	{
		return true;
	}

	std::map<String, DeviceCreationParameter*> AudioOutputDevicePortAudio::ParameterCard::DependsAsParameters()
	{
#if 0
		return { {ParameterInterface::Name(),  nullptr} };
#else
		return {};
#endif
	}

	optional<String> AudioOutputDevicePortAudio::ParameterCard::DefaultAsString(std::map<String, String> Parameters)
	{
		ApiLock lock;
		optional<String> result;
#if 0
		const auto host = TraitsInterface::FromParameters(Parameters);
		if (host.second != nullptr && host.second->defaultOutputDevice >= 0) {
			const auto dev = TraitsDevice::GetItem(host.second->defaultOutputDevice);
			if (dev != nullptr)
			{
				result = TraitsDevice::SafeValue(host.second->defaultOutputDevice, dev);
			}
		}
#elif 1
		auto items = PossibilitiesAsString(Parameters);
		if (items.size() > 0)
			result = items[0];
#else
		const auto idx = Pa_GetDefaultOutputDevice();
		if (idx >= 0) {
			const auto dev = TraitsDevice::GetItem(idx);
			if (dev != nullptr)
			{
				result = TraitsDevice::SafeValue(idx, dev);
			}
		}
#endif
		return result;
	}

	std::vector<String> AudioOutputDevicePortAudio::ParameterCard::PossibilitiesAsString(
		std::map<String, String> Parameters)
	{
		ApiLock lock;
		std::vector<String> result;
#if 1
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
			if (devInfo != nullptr)
				result.push_back(TraitsDevice::SafeValue(i, devInfo));
		}
#endif
		return result;
	}

	void AudioOutputDevicePortAudio::ParameterCard::OnSetValue(String s) throw (Exception)
	{
	}

	// 'SAMPLE RATE'
#define DEFAULT_RATE 44100
#define DEFAULT_LATENCY 0.05
#define DEFAULT_CHANNELS 2

	static std::vector PossibleRates = {22500, 44100, 48000, 96000};


	AudioOutputDevicePortAudio::ParameterSampleRate::ParameterSampleRate() : CardParameter<
		AudioOutputDevice::ParameterSampleRate>()
	{
	}

	AudioOutputDevicePortAudio::ParameterSampleRate::ParameterSampleRate(String s) : CardParameter<
		AudioOutputDevice::ParameterSampleRate>(s)
	{
	}

	optional<int> AudioOutputDevicePortAudio::ParameterSampleRate::DefaultAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		return (card.second != nullptr) ? static_cast<int>(card.second->defaultSampleRate) : DEFAULT_RATE;
	}

	std::vector<int> AudioOutputDevicePortAudio::ParameterSampleRate::PossibilitiesAsInt(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		std::vector<int> rates;

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

	// 'CHANNELS'

	AudioOutputDevicePortAudio::ParameterChannels::ParameterChannels() : CardParameter<
		AudioOutputDevice::ParameterChannels>()
	{
	}

	AudioOutputDevicePortAudio::ParameterChannels::ParameterChannels(String s) : CardParameter<
		AudioOutputDevice::ParameterChannels>(s)
	{
	}

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::DefaultAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		return (card.second != nullptr) ? std::min(card.second->maxOutputChannels, DEFAULT_CHANNELS) : DEFAULT_CHANNELS;
	}

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::RangeMinAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		return (card.second != nullptr) ? std::min(card.second->maxOutputChannels, 1) : 1;
	}

	optional<int> AudioOutputDevicePortAudio::ParameterChannels::RangeMaxAsInt(std::map<String, String> Parameters)
	{
		ApiLock lock;

		const auto card = FromParams(Parameters);
		return (card.second != nullptr) ? card.second->maxOutputChannels : DEFAULT_CHANNELS;
	}

	// 'FRAGMENT SIZE'

	AudioOutputDevicePortAudio::ParameterFragmentSize::ParameterFragmentSize() : CardParameter<
		DeviceCreationParameterFloat>()
	{
	}

	AudioOutputDevicePortAudio::ParameterFragmentSize::ParameterFragmentSize(String s) : CardParameter<
		DeviceCreationParameterFloat>(s)
	{
	}

	String AudioOutputDevicePortAudio::ParameterFragmentSize::Description()
	{
		return "Preferred latency during playback (ms)";
	}

	optional<float> AudioOutputDevicePortAudio::ParameterFragmentSize::DefaultAsFloat(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		optional<float> result;
		const auto card = FromParams(Parameters);
		if (card.second != nullptr) result = card.second->defaultLowInputLatency;
		else result = 0.1f;
		return result;
	}

	optional<float> AudioOutputDevicePortAudio::ParameterFragmentSize::RangeMinAsFloat(
		std::map<String, String> Parameters)
	{
		return DefaultAsFloat(Parameters);
	}

	optional<float> AudioOutputDevicePortAudio::ParameterFragmentSize::RangeMaxAsFloat(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		optional<float> result;
		const auto card = FromParams(Parameters);
		if (card.second != nullptr) result = card.second->defaultHighOutputLatency;
		return result;
	}

	std::vector<float> AudioOutputDevicePortAudio::ParameterFragmentSize::PossibilitiesAsFloat(
		std::map<String, String> Parameters)
	{
		ApiLock lock;

		std::vector<float> values;
		const auto card = FromParams(Parameters);
		if (card.second != nullptr)
		{
			for (auto lat = card.second->defaultLowOutputLatency; lat < card.second->defaultHighOutputLatency; lat *=
			     1.5)
			{
				values.push_back(lat);
			}
			values.push_back(card.second->defaultHighOutputLatency);
		}
		return values;
	}

	void AudioOutputDevicePortAudio::ParameterFragmentSize::OnSetValue(float value) throw (Exception)
	{
	}

	String AudioOutputDevicePortAudio::ParameterFragmentSize::Name()
	{
		return "Latency";
	}
}

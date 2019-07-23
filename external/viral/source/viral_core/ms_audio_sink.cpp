/**
 *************************************************************************
 *
 * @file ms_audio_sink.cpp
 *
 * WASAPI audio sinks on Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_audio_sink.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include "log.hpp"
#include "audio_buffer.hpp"
#include "ms_audio_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_audio_sink
//
//////////////////////////////////////////////////////////////////////////


ms_audio_sink::ms_audio_sink
	(ms_audio_render_format& render_format,
	 ms_audio_buffer_queue& buffer_queue)
	:
	render_format_(render_format),
	buffer_queue_(buffer_queue),
	device_enumerator_(NULL),
	audio_sink_callbacks_(NULL),
	device_(NULL),
	audio_client_(NULL),
	mix_format_(NULL),
	mix_format_extensible_(NULL),
	audio_render_client_(NULL)
{
	LOG_INFO("Creating and running internal thread.");


	// Start internal ms_audio_sink thread.
	// Then wait until initialization has finished,
	// so that any errors (i.e. thread exceptions)
	// cause construction of ms_audio_sink to fail.
	start();

	while (true)
	{
		init_finished_signal_.wait_for(true);

		if (init_finished_signal_.get())
			break;

		if (thread_has_terminated())
		{
			LOG_ERROR("Internal thread failed to create WASAPI.");
			join(); // should throw unhandled_exception

			LOG_CRITICAL("Internal thread terminated without exception.");
			throw assert_failed();
		}
	}
}


ms_audio_sink::~ms_audio_sink() NOTHROW
{
	LOG_INFO("Terminating internal thread.");

	try { join(); }
	catch (...)
	{
		LOG_CRITICAL
			("Internal thread has thrown an exception on joining.");
	}

	// No need to clean up WASAPI objects here,
	// the internal thread does so in task_main().
}


void ms_audio_sink::throw_if_terminated()
{
	// Check whether the internal thread terminated prematurely.
	// Optionally pass on any unhandled_exception through join().
	if (thread_has_terminated())
	{
		join(); 

		LOG_CRITICAL("Internal thread self-terminated without exception.");
		throw assert_failed();
	}
}


void ms_audio_sink::task_main()
{
	// First-time initialization of WASAPI.
	init_com();
	SCOPE_GUARD
		(com_guard, (*this, &ms_audio_sink::shutdown_com));

	init_device_enumerator();
	SCOPE_GUARD
		(device_enumerator_guard,
			(*this, &ms_audio_sink::shutdown_device_enumerator));

	init_audio_sink_callbacks();
	SCOPE_GUARD
		(audio_sink_callbacks_guard,
			(*this, &ms_audio_sink::shutdown_audio_sink_callbacks));

	init_device_and_clients(true);
	SCOPE_GUARD
		(device_guard,
			(*this, &ms_audio_sink::shutdown_device_and_clients));

	init_finished_signal_.set(true);


	// Main WASAPI loop:
	// Feed OS audio buffers, process COM messages,
	// and handle connections to devices.
	while (!join_now())
	{
		shutdown_if_default_device_changed();
		reinit_if_no_device();
		feed_device_buffer();
		message_loop();

		MsgWaitForMultipleObjects
			(0, NULL, false, wasapi_thread_sleep_time_msecs_, QS_ALLINPUT);
	}


	// Guards shut down both COM and WASAPI here.
}


void ms_audio_sink::init_com()
{
	// Strong exception safety, even without scope guard.

	LOG_INFO("Initializing STA COM for internal thread.");

	HRESULT hr =
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (FAILED(hr))
	{
		LOG_ERROR("Failed to initialize STA COM.");
		throw api_call_failed();
	}
}


void ms_audio_sink::shutdown_com() NOTHROW
{
	LOG_INFO("Uninitializing COM for internal thread.")
	CoUninitialize();
}


void ms_audio_sink::init_device_enumerator()
{
	// Strong exception safety, even without scope guard.

	LOG_INFO("Creating device enumerator.");

	HRESULT hr = CoCreateInstance
		(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
		 __uuidof(IMMDeviceEnumerator),
		 reinterpret_cast<void**>(&device_enumerator_));

	if (FAILED(hr))
	{
		LOG_ERROR("Failed to create device enumerator.");
		throw api_call_failed();
	}
}


void ms_audio_sink::shutdown_device_enumerator() NOTHROW
{
	if (device_enumerator_)
	{
		LOG_INFO("Releasing device enumerator.");
		device_enumerator_->Release();
		device_enumerator_ = NULL;
	}
}


void ms_audio_sink::init_audio_sink_callbacks()
{
	// Strong exception safety:
	// There are only two throwing points:
	// - Creating ms_audio_sink_callbacks may throw,
	//		in which case no state has changed yet.
	// - We throw if registering the callbacks fails,
	//		but release audio_sink_callbacks_ afore.

	LOG_INFO("Registering audio sink callbacks.");

	audio_sink_callbacks_ = new ms_audio_sink_callbacks();

	HRESULT hr =
		device_enumerator_->
			RegisterEndpointNotificationCallback
				(audio_sink_callbacks_);

	if (FAILED(hr))
	{
		LOG_ERROR("Failed to register audio sink callbacks.");
		audio_sink_callbacks_->Release();
		audio_sink_callbacks_ = NULL;
		throw api_call_failed();
	}
}


void ms_audio_sink::shutdown_audio_sink_callbacks() NOTHROW
{
	if (audio_sink_callbacks_)
	{
		LOG_INFO("Unregistering audio sink callbacks.");

		HRESULT hr =
			device_enumerator_->
				UnregisterEndpointNotificationCallback
					(audio_sink_callbacks_);

		if (FAILED(hr))
		{
			LOG_ERROR("Error while unregistering callbacks, ignoring.");
			// Exception safety: NOTHROW, thus ignore.
		}

		audio_sink_callbacks_->Release();
		audio_sink_callbacks_ = NULL;
	}
}


void ms_audio_sink::init_device_and_clients(bool first_try)
{
	/**
	 * Initialize the default WASAPI audio device.
	 *
	 * Usage notes:
	 * - Clients should only use init_device_and_clients()
	 *		instead of individual initialization utilities
	 *		to ensure strong exception safety.
	 * - If @bref{first_try} is @false, we suppress logging
	 *		to avoid log spam while no device is connected.
	 *
	 * Error handling:
	 * - We return directly if there is no default audio device,
	 *		or if the device disconnects during initialization.
	 *		Any other errors are critical and throw.
	 * - In either case, shutdown_device_and_clients() executes
	 *		to reset WASAPI for future init_device_and_clients().
	 */

	if (device_)
	{
		LOG_ERROR("There already is a not-NULL audio device.");
		throw assert_failed();
	}

	SCOPE_GUARD
		(device_and_clients_guard,
			(*this, &ms_audio_sink::shutdown_device_and_clients));

	if (!try_init_device(first_try)) return;
	if (!try_activate_audio_client()) return;
	if (!try_get_mix_format()) return;
	if (!try_init_audio_client()) return;
	if (!try_start_audio_client()) return;
	if (!try_init_audio_render_client()) return;

	// Success.
	// Exception safety: NOTHROW from here.
	device_and_clients_guard.dismiss();
}


bool ms_audio_sink::try_init_device(bool first_try)
{
	// Call through init_device_and_clients() for exception safety.

	if (first_try)
		{ LOG_INFO("Fetching default audio endpoint."); }

	HRESULT hr =
		device_enumerator_->GetDefaultAudioEndpoint
			(eRender, eConsole, &device_);

	// No check_hresult() due to custom logging.
	if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
	{
		if (first_try)
			{ LOG_WARN("No audio device found on startup."); }

		return false;
	}

	if (FAILED(hr))
	{
		LOG_ERROR("Failed to fetch default audio endpoint.");
		throw api_call_failed();
	}

	// Log after connection success,
	// mainly to track reconnecting.
	LOG_INFO("Connected to default audio endpoint.");
	return true;
}


bool ms_audio_sink::try_activate_audio_client()
{
	// Call through init_device_and_clients() for exception safety.

	LOG_INFO("Activating audio client.");

	HRESULT hr =
		device_->Activate
			(__uuidof(IAudioClient), CLSCTX_ALL, NULL,
			 reinterpret_cast<void**>(&audio_client_));

	return check_hresult(hr, "Activating audio client.");
}


bool ms_audio_sink::try_get_mix_format()
{
	// Call through init_device_and_clients() for exception safety.

	LOG_INFO("Getting mix format.");

	HRESULT hr =
		audio_client_->GetMixFormat(&mix_format_);

	if (!check_hresult(hr, "Getting mix format."))
		return false;

	LOG_INFO("Current mix format:" +
			 " frames per second: " +
					static_cast<int32>(mix_format_->nSamplesPerSec) +
			 " channels: " +
					static_cast<int32>(mix_format_->nChannels) +
			 " bits per sample: " +
					static_cast<int32>(mix_format_->wBitsPerSample));


	// Check for supported stream format.
	// Note some systems report WAVE_FORMAT_EXTENSIBLE
	// instead of directly listing WAVE_FORMAT_IEEE_FLOAT.
	if (mix_format_->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		mix_format_extensible_ =
			reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mix_format_);

	WORD mix_format_wave_tag =
		mix_format_extensible_ ?
			to_wave_format(mix_format_extensible_->SubFormat) :
			mix_format_->wFormatTag;

	if (mix_format_wave_tag == WAVE_FORMAT_IEEE_FLOAT)
		{ LOG_INFO("Format tag: WAVE_FORMAT_IEEE_FLOAT"); }
	else
	{
		LOG_ERROR("Type of mix format is not float.");
		throw unsupported_feature();
	}

	if (mix_format_->wBitsPerSample != 32)
	{
		LOG_ERROR("Bits per sample on mix format is not 32.");
		throw unsupported_feature();
	}


	// Derive sampling parameters for new rendering format.
	audio_channels_type channels =
		audio_channels::to_channels(mix_format_->nChannels);

	if (channels == audio_channels::channels_unknown)
	{
		LOG_ERROR("Unsupported channel count on mix format.");
		throw unsupported_feature();
	}

	audio_rate_type rate =
		audio_rate::to_rate(mix_format_->nSamplesPerSec);

	if (rate == audio_rate::rate_unknown)
	{
		LOG_ERROR("Unsupported sampling rate on mix format.");
		throw unsupported_feature();
	}


	// Success.
	// Exception safety: NOTHROW from here.
	render_format_.set(channels, rate);
	return true;
}


bool ms_audio_sink::try_init_audio_client()
{
	// Call through init_device_and_clients() for exception safety.

	LOG_INFO("Initializing audio client.");

	HRESULT hr =
		audio_client_->Initialize
			(AUDCLNT_SHAREMODE_SHARED, 0,
			 wasapi_buffer_duration_100nsec_,
			 0, mix_format_, NULL);

	return check_hresult(hr, "Initializing audio client.");
}


bool ms_audio_sink::try_start_audio_client()
{
	// Call through init_device_and_clients() for exception safety.

	LOG_INFO("Starting audio client.");

	HRESULT hr =
		audio_client_->Start();

	return check_hresult(hr, "Starting audio client.");
}


bool ms_audio_sink::try_init_audio_render_client()
{
	// Call through init_device_and_clients() for exception safety.

	LOG_INFO("Fetching audio render client.");

	HRESULT hr =
		audio_client_->GetService
			(__uuidof(IAudioRenderClient),
			 reinterpret_cast<void**>(&audio_render_client_));

	return check_hresult(hr, "Fetching audio render client.");
}


void ms_audio_sink::shutdown_device_and_clients() NOTHROW
{
	if (audio_render_client_)
	{
		LOG_INFO("Releasing audio render client.");
		audio_render_client_->Release();
		audio_render_client_ = NULL;
	}

	if (audio_client_)
	{
		// We must separately stop rendering
		// before releasing the mix format.
		LOG_INFO("Stopping audio client.");

		HRESULT hr = audio_client_->Stop();

		if (FAILED(hr))
		{
			LOG_ERROR("Error while stopping audio client, ignoring.");
			// Exception safety: NOTHROW, thus ignore.
		}
	}

	if (mix_format_)
	{
		LOG_INFO("Releasing mix format.");

		CoTaskMemFree(mix_format_);
		mix_format_ = NULL;
		mix_format_extensible_ = NULL;

		// Reset rendering format as well.
		// Note feed_device_buffer() discards
		// any remaining, mismatching buffers
		// from ms_audio_buffer_queue.
		render_format_.set
			(audio_channels::channels_unknown,
			 audio_rate::rate_unknown);
	}

	if (audio_client_)
	{
		LOG_INFO("Releasing audio client.");
		audio_client_->Release();
		audio_client_ = NULL;
	}

	if (device_)
	{
		LOG_INFO("Releasing audio device.");
		device_->Release();
		device_ = NULL;
	}
}


bool ms_audio_sink::check_hresult
	(HRESULT hr, const char* operation)
{
	/**
	 * Returns @true if @bref{hr}esult implies success,
	 * @false on non-permanent errors (e.g. device loss).
	 * Throws on fatal (i.e. permanent) errors.
	 */

	if (hr == AUDCLNT_E_DEVICE_INVALIDATED)
	{
		LOG_WARN("Audio device lost on: " + operation);
		return false;
	}
	else if (hr == AUDCLNT_E_DEVICE_IN_USE)
	{
		LOG_WARN("Audio device in use on: " + operation);
		return false;
	}
	else if (FAILED(hr))
	{
		LOG_ERROR("WASAPI failure on: " + operation);
		throw api_call_failed();
	}

	return true;
}


WORD ms_audio_sink::to_wave_format(const GUID& guid)
{
	if (guid == KSDATAFORMAT_SUBTYPE_PCM)
		return WAVE_FORMAT_PCM;
	else if (guid == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		return WAVE_FORMAT_IEEE_FLOAT;

	LOG_ERROR("Unknown WAVEFORMATEXTENSIBLE subformat GUID.");
	throw unsupported_feature();
}


void ms_audio_sink::shutdown_if_default_device_changed()
{
	/**
	 * If the callbacks report a change in default device,
	 * shutdown the current audio @bref{device_} to @NULL
	 * for later reinit_if_no_device() to the new default.
	 */

	if (!audio_sink_callbacks_->default_device_changed())
		return;

	LOG_INFO("Default audio device changed.");
	shutdown_device_and_clients();
}


void ms_audio_sink::reinit_if_no_device()
{
	/**
	 * If the current audio @bref{device_} is @NULL
	 * (e.g. due to changed default or device loss),
	 * then try to reinitialize from default device.
	 *
	 * Implementation notes:
	 * - We apply a timer to avoid permanent checking for devices
	 *		while no default audio device is currently connected
	 *		or in case there are many rapid device changes.
	 * - Timerless alternatives (e.g. ms_audio_sink_callbacks)
	 *		are somewhat fragile (e.g. due to threading issues)
	 *		and hard to handle (e.g. spurious property changes).
	 */

	if (device_) return;

	if (no_device_timer_.seconds_passed() <
		no_device_wait_seconds_)
			return;

	no_device_timer_.restart();	
	init_device_and_clients(false);
}


void ms_audio_sink::feed_device_buffer()
{
	// Abort if no audio device is connected.
	// Otherwise, release device on exceptions.
	if (!audio_client_ || !audio_render_client_)
		return;

	SCOPE_GUARD
		(device_and_clients_guard,
			(*this, &ms_audio_sink::shutdown_device_and_clients));


	// Fetch WASAPI buffer.
	UINT32 frame_count_total = 0;
	HRESULT hr =
		audio_client_->GetBufferSize
			(&frame_count_total);

	if (!check_hresult(hr, "Getting WASAPI buffer size."))
		return; // Scope guard disconnects device.

	UINT32 frame_count_used = 0;
	hr = audio_client_->GetCurrentPadding
			(&frame_count_used);

	if (!check_hresult(hr, "Getting WASAPI buffer padding."))
		return; // Scope guard disconnects device.

	BYTE* buffer = NULL;
	UINT32 frame_count_unused =
		frame_count_total - frame_count_used;
	
	hr = audio_render_client_->GetBuffer
			(frame_count_unused, &buffer);

	if (!check_hresult(hr, "Getting WASAPI buffer pointer."))
		return; // Scope guard disconnects device.


	// Fetch current rendering parameters.
	// Thread safety: Only the internal ms_audio_sink thread
	// changes render_format_ and calls feed_device_buffer().
	// Thus, there is no race condition here.
	audio_channels_type channels =
		audio_channels::channels_unknown;
	audio_rate_type rate =
		audio_rate::rate_unknown;

	render_format_.get(channels, rate);

	if (channels == audio_channels::channels_unknown ||
		rate == audio_rate::rate_unknown)
	{
		LOG_ERROR("Valid device has invalid channels or rate.");
		throw assert_failed();
	}


	// Fill WASAPI buffer.
	// Note try_get_mix_format() ensures a float format.
	// Exception safety: Buffer filling must be NOTHROW
	// to ensure later ReleaseBuffer() executes.
	buffer_queue_.fetch_buffers
		(reinterpret_cast<float*>(buffer),
		 frame_count_unused, channels, rate);
	

	// Release WASAPI buffer.
	hr = audio_render_client_->ReleaseBuffer
			(frame_count_unused, 0);

	if (!check_hresult(hr, "Releasing WASAPI buffer pointer."))
		return; // Scope guard disconnects device.


	// Success.
	// Exception safety: NOTHROW from here.
	device_and_clients_guard.dismiss();
}


void ms_audio_sink::message_loop()
{
	/**
	 * Dispatch all messages on the thread message queue.
	 * This may include messages to any auxiliary windows
	 * that WASAPI shows (e.g. audio or error prompts).
	 */

	MSG message;

	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}


/** How long to wait before rechecking for new devices. */
const float ms_audio_sink::no_device_wait_seconds_ = 1.f;


/**
 * WASAPI buffering and sleeping times.
 * Sleeping should be no more than half the buffer time.
 */
//@{
const REFERENCE_TIME ms_audio_sink::wasapi_buffer_duration_100nsec_ = 100;
const DWORD ms_audio_sink::wasapi_thread_sleep_time_msecs_ = 5;
//@}




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_sink_callbacks
//
//////////////////////////////////////////////////////////////////////////


ms_audio_sink_callbacks::ms_audio_sink_callbacks() NOTHROW
	:
	ref_count_(1),
	default_device_changed_(0)
{ }


ms_audio_sink_callbacks::~ms_audio_sink_callbacks() NOTHROW { }


HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::QueryInterface
		(REFIID riid, VOID **ppvInterface)
{
	if (riid == IID_IUnknown ||
		riid == __uuidof(IMMNotificationClient))
	{
		AddRef();
		*ppvInterface = this;
		return S_OK;
	}

	*ppvInterface = NULL;
	return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE
	ms_audio_sink_callbacks::AddRef()
		{ return InterlockedIncrement(&ref_count_); }

ULONG STDMETHODCALLTYPE
	ms_audio_sink_callbacks::Release()
{
	ULONG new_ref_count = InterlockedDecrement(&ref_count_);
	if (new_ref_count == 0) delete this;
	return new_ref_count;
}


HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::OnDefaultDeviceChanged
		(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
	// We only use console audio rendering devices,
	// so we only react to a change in one of these.
	if (flow == eRender && role == eConsole)
		InterlockedCompareExchange
			(&default_device_changed_, 1, 0);

	return S_OK;
}


HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::OnDeviceAdded(LPCWSTR pwstrDeviceId)
		{ return S_OK; }

HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
		{ return S_OK; }

HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::OnDeviceStateChanged
		(LPCWSTR pwstrDeviceId, DWORD dwNewState)
			{ return S_OK; }

HRESULT STDMETHODCALLTYPE
	ms_audio_sink_callbacks::OnPropertyValueChanged
		(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
			{ return S_OK; }


bool ms_audio_sink_callbacks::default_device_changed() NOTHROW
{
	ULONG former_value =
		InterlockedCompareExchange
			(&default_device_changed_, 0, 1);

	return former_value == 1;
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */

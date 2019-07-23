/**
 *************************************************************************
 *
 * @file ms_audio_sink.hpp
 *
 * WASAPI audio sinks on Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_SINK_HPP)
#define INCLUDED__VIRAL_CORE__MS_AUDIO_SINK_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include <mmdeviceapi.h>
#include <audioclient.h>


#include "audio_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "owning_list.hpp"
#include "timer.hpp"
#include "thread.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_audio_sink
 * @ingroup audio_module
 *
 * Wraps all WASAPI objects for use in ms_audio_context.
 *
 * Usage notes:
 * - The ms_audio_sink internally handles lost, occupied, or changed
 *		audio devices without throwing. This enables users to connect,
 *		disconnect, and configure audio devices arbitrarily.
 *
 * Implementation notes:
 * - WASAPI requires a single-threaded COM appartment for IAudioClient.
 *		Otherwise, WASAPI has undefined behaviour on Windows 8 or newer.
 * - Our client threads default to multi-threaded COM (see ms_com_init). 
 *		Thus, we manage WASAPI playback in a separate, internal thread.
 *
 ************************************************************************/
class ms_audio_sink :
	private threaded_task
{

	public:

		ms_audio_sink
			(ms_audio_render_format& render_format,
			 ms_audio_buffer_queue& buffer_queue);


		~ms_audio_sink() NOTHROW;


		/**
		 * Throw an exception if the internal thread
		 * has terminated unexpectedly.
		 */
		void throw_if_terminated();


	private:

		void task_main() override;

		void init_com();
		void shutdown_com() NOTHROW;

		void init_device_enumerator();
		void shutdown_device_enumerator() NOTHROW;

		void init_audio_sink_callbacks();
		void shutdown_audio_sink_callbacks() NOTHROW;

		void init_device_and_clients(bool first_try);
		bool try_init_device(bool first_try);
		bool try_activate_audio_client();
		bool try_get_mix_format();
		bool try_init_audio_client();
		bool try_start_audio_client();
		bool try_init_audio_render_client();
		void shutdown_device_and_clients() NOTHROW;

		static bool check_hresult(HRESULT hr, const char* operation);
		static WORD to_wave_format(const GUID& guid);

		void shutdown_if_default_device_changed();
		void reinit_if_no_device();
		void feed_device_buffer();
		void message_loop();


		static const float no_device_wait_seconds_;

		static const REFERENCE_TIME wasapi_buffer_duration_100nsec_;
		static const DWORD wasapi_thread_sleep_time_msecs_;

		ms_audio_render_format& render_format_;
		ms_audio_buffer_queue& buffer_queue_;

		signal init_finished_signal_;

		IMMDeviceEnumerator* device_enumerator_;
		ms_audio_sink_callbacks* audio_sink_callbacks_;

		IMMDevice* device_;
		IAudioClient* audio_client_;
		WAVEFORMATEX* mix_format_;
		WAVEFORMATEXTENSIBLE* mix_format_extensible_;
		IAudioRenderClient* audio_render_client_;

		free_timer no_device_timer_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_sink_callbacks
 * @ingroup audio_module
 *
 * Callbacks for WASAPI in ms_audio_sink.
 * 
 * Default and disconnected devices:
 * - We employ callbacks to detect a changed default audio device
 *		(e.g. when the user changes from speakers to headphones).
 * - While a notification also triggers for disconnecting devices,
 *		threaded callbacks prohibit a reliable and timely reaction.
 *		Instead, ms_audio_sink separately detects device loss.
 *
 * Device properties and states:
 * - Changed device properties (e.g. sampling rate or exclusive state)
 *		are difficult to reliably identify in ms_audio_sink_callbacks.
 *		Notably, some devices spuriously change internal properties.
 * - Vital property changes (e.g. to sampling rate) cause WASAPI
 *		to invalidate the current audio device without triggering
 *		OnDefaultDeviceChanged() and thus default_device_changed().
 *		A timer in ms_audio_sink::reinit_if_no_device() then still
 *		causes reinitialization of the default audio device.
 *
 * Implementation notes:
 * - Threaded callbacks must not block, according to documentation.
 *		Thus, we use interlocked operations to notify ms_audio_sink.
 * - Lifetime of ms_audio_sink_callbacks depends on COM references,
 *		it may possibly outlive the higher-level ms_audio_sink.
 *
 ************************************************************************/
class ms_audio_sink_callbacks :
	public IMMNotificationClient
{

	public:

		ms_audio_sink_callbacks() NOTHROW;
		virtual ~ms_audio_sink_callbacks() NOTHROW; // Not virtual in base class.


		/** Implementation of IUnknown. */
		//@{
		HRESULT STDMETHODCALLTYPE
			QueryInterface(REFIID riid, VOID **ppvInterface) override;

		ULONG STDMETHODCALLTYPE AddRef() override;
		ULONG STDMETHODCALLTYPE Release() override;
		//@}


		/** Implementation of IMMNotificationClient. */
		//@{
		HRESULT STDMETHODCALLTYPE
			OnDefaultDeviceChanged
				(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) override;
		
		HRESULT STDMETHODCALLTYPE
			OnDeviceAdded(LPCWSTR pwstrDeviceId) override;
		HRESULT STDMETHODCALLTYPE
			OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;

		HRESULT STDMETHODCALLTYPE
			OnDeviceStateChanged
				(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;
		HRESULT STDMETHODCALLTYPE
			OnPropertyValueChanged
				(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;
		//@}


		/**
		 * Returns whether the default audio device changed
		 * since the previous default_device_changed() call.
		 * Queries and lowers @bref{default_device_changed_}
		 * in a single, atomic operation.
		 */
		bool default_device_changed() NOTHROW;


	private:

		ULONG ref_count_;	
		ULONG default_device_changed_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_SINK_HPP) */



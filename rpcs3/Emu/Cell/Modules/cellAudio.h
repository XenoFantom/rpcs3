#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/Thread.h"
#include "Utilities/simple_ringbuf.h"
#include "Emu/Memory/vm.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/AudioDumper.h"
#include "Emu/system_config_types.h"

struct lv2_event_queue;

// Error codes
enum CellAudioError : u32
{
	CELL_AUDIO_ERROR_ALREADY_INIT               = 0x80310701,
	CELL_AUDIO_ERROR_AUDIOSYSTEM                = 0x80310702,
	CELL_AUDIO_ERROR_NOT_INIT                   = 0x80310703,
	CELL_AUDIO_ERROR_PARAM                      = 0x80310704,
	CELL_AUDIO_ERROR_PORT_FULL                  = 0x80310705,
	CELL_AUDIO_ERROR_PORT_ALREADY_RUN           = 0x80310706,
	CELL_AUDIO_ERROR_PORT_NOT_OPEN              = 0x80310707,
	CELL_AUDIO_ERROR_PORT_NOT_RUN               = 0x80310708,
	CELL_AUDIO_ERROR_TRANS_EVENT                = 0x80310709,
	CELL_AUDIO_ERROR_PORT_OPEN                  = 0x8031070a,
	CELL_AUDIO_ERROR_SHAREDMEMORY               = 0x8031070b,
	CELL_AUDIO_ERROR_MUTEX                      = 0x8031070c,
	CELL_AUDIO_ERROR_EVENT_QUEUE                = 0x8031070d,
	CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND      = 0x8031070e,
	CELL_AUDIO_ERROR_TAG_NOT_FOUND              = 0x8031070f,
};

// constants
enum
{
	CELL_AUDIO_BLOCK_8                 = 8,
	CELL_AUDIO_BLOCK_16                = 16,
	CELL_AUDIO_BLOCK_32                = 32,
	CELL_AUDIO_BLOCK_SAMPLES           = 256,

	CELL_AUDIO_CREATEEVENTFLAG_SPU     = 0x00000001,

	CELL_AUDIO_EVENT_MIX               = 0,
	CELL_AUDIO_EVENT_HEADPHONE         = 1,

	CELL_AUDIO_EVENTFLAG_BEFOREMIX     = 0x80000000,
	CELL_AUDIO_EVENTFLAG_DECIMATE_2    = 0x08000000,
	CELL_AUDIO_EVENTFLAG_DECIMATE_4    = 0x10000000,
	CELL_AUDIO_EVENTFLAG_HEADPHONE     = 0x20000000,
	CELL_AUDIO_EVENTFLAG_NOMIX         = 0x40000000,

	CELL_AUDIO_MAX_PORT                = 4,
	CELL_AUDIO_MAX_PORT_2              = 8,

	CELL_AUDIO_MISC_ACCVOL_ALLDEVICE   = 0x0000ffffUL,
	CELL_AUDIO_PERSONAL_DEVICE_PRIMARY = 0x8000,

	CELL_AUDIO_PORT_2CH                = 2,
	CELL_AUDIO_PORT_8CH                = 8,

	CELL_AUDIO_PORTATTR_BGM            = 0x0000000000000010ULL,
	CELL_AUDIO_PORTATTR_INITLEVEL      = 0x0000000000001000ULL,
	CELL_AUDIO_PORTATTR_OUT_NO_ROUTE   = 0x0000000000100000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_0 = 0x0000000001000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_1 = 0x0000000002000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_2 = 0x0000000004000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_3 = 0x0000000008000000ULL,
	CELL_AUDIO_PORTATTR_OUT_SECONDARY  = 0x0000000000000001ULL,
	CELL_AUDIO_PORTATTR_OUT_STREAM1    = 0x0000000000000001ULL,

	CELL_AUDIO_STATUS_CLOSE            = 0x1010,
	CELL_AUDIO_STATUS_READY            = 1,
	CELL_AUDIO_STATUS_RUN              = 2,
};

enum class audio_backend_update : u32
{
	NONE,
	PARAM,
	ALL,
};

//libaudio datatypes
struct CellAudioPortParam
{
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u64> attr;
	be_t<float> level;
};

struct CellAudioPortConfig
{
	vm::bptr<u64> readIndexAddr;
	be_t<u32> status;
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u32> portSize;
	be_t<u32> portAddr;
};

enum : u32
{
	AUDIO_PORT_COUNT = 8,
	AUDIO_MAX_BLOCK_COUNT = 32,
	AUDIO_MAX_CHANNELS_COUNT = 8,

	AUDIO_PORT_OFFSET = AUDIO_BUFFER_SAMPLES * AUDIO_MAX_BLOCK_COUNT * AUDIO_MAX_CHANNELS_COUNT * sizeof(f32),
	EXTRA_AUDIO_BUFFERS = 8,
	MAX_AUDIO_EVENT_QUEUES = 64,

	AUDIO_BLOCK_SIZE_2CH = 2 * AUDIO_BUFFER_SAMPLES,
	AUDIO_BLOCK_SIZE_6CH = 6 * AUDIO_BUFFER_SAMPLES,
	AUDIO_BLOCK_SIZE_8CH = 8 * AUDIO_BUFFER_SAMPLES,

	PORT_BUFFER_TAG_COUNT = 6,

	PORT_BUFFER_TAG_LAST_2CH = AUDIO_BLOCK_SIZE_2CH - 1,
	PORT_BUFFER_TAG_DELTA_2CH = PORT_BUFFER_TAG_LAST_2CH / (PORT_BUFFER_TAG_COUNT - 1),
	PORT_BUFFER_TAG_FIRST_2CH = PORT_BUFFER_TAG_LAST_2CH % (PORT_BUFFER_TAG_COUNT - 1),

	PORT_BUFFER_TAG_LAST_6CH = AUDIO_BLOCK_SIZE_6CH - 1,
	PORT_BUFFER_TAG_DELTA_6CH = PORT_BUFFER_TAG_LAST_6CH / (PORT_BUFFER_TAG_COUNT - 1),
	PORT_BUFFER_TAG_FIRST_6CH = PORT_BUFFER_TAG_LAST_6CH % (PORT_BUFFER_TAG_COUNT - 1),

	PORT_BUFFER_TAG_LAST_8CH = AUDIO_BLOCK_SIZE_8CH - 1,
	PORT_BUFFER_TAG_DELTA_8CH = PORT_BUFFER_TAG_LAST_8CH / (PORT_BUFFER_TAG_COUNT - 1),
	PORT_BUFFER_TAG_FIRST_8CH = PORT_BUFFER_TAG_LAST_8CH % (PORT_BUFFER_TAG_COUNT - 1),
};

enum class audio_port_state : u32
{
	closed,
	opened,
	started,
};

struct audio_port
{
	atomic_t<audio_port_state> state = audio_port_state::closed;

	u32 number;
	vm::ptr<char> addr{};
	vm::ptr<u64> index{};

	u32 num_channels;
	u32 num_blocks;
	u64 attr;
	u64 cur_pos;
	u64 global_counter; // copy of global counter
	u64 active_counter;
	u32 size;
	u64 timestamp; // copy of global timestamp

	struct level_set_t
	{
		float value;
		float inc;
	};

	float level;
	atomic_t<level_set_t> level_set;

	u32 block_size() const
	{
		return num_channels * AUDIO_BUFFER_SAMPLES;
	}

	u32 buf_size() const
	{
		return block_size() * sizeof(float);
	}

	u32 position(s32 offset = 0) const
	{
		s32 ofs = (offset % num_blocks) + num_blocks;
		return (cur_pos + ofs) % num_blocks;
	}

	u32 buf_addr(s32 offset = 0) const
	{
		return addr.addr() + position(offset) * buf_size();
	}

	be_t<f32>* get_vm_ptr(s32 offset = 0) const
	{
		return vm::_ptr<f32>(buf_addr(offset));
	}


	// Tags
	u32 prev_touched_tag_nr;
	f32 last_tag_value[PORT_BUFFER_TAG_COUNT] = { 0 };

	void tag(s32 offset = 0);
};

struct cell_audio_config
{
	struct raw_config
	{
		bool buffering_enabled = false;
		s64 desired_buffer_duration = 0;
		bool enable_time_stretching = false;
		s64 time_stretching_threshold = 0;
		bool convert_to_s16 = false;
		audio_downmix downmix = audio_downmix::downmix_to_stereo;
		audio_renderer renderer = audio_renderer::null;
		audio_provider provider = audio_provider::none;
	} raw;

	std::shared_ptr<AudioBackend> backend = nullptr;

	u32 audio_channels = 0;
	u32 audio_sampling_rate = 0;
	u32 audio_block_period = 0;
	u32 audio_sample_size = 0;
	f64 audio_min_buffer_duration = 0.0;

	u32 audio_buffer_length = 0;
	u32 audio_buffer_size = 0;

	/*
	 * Buffering
	 */

	u64 desired_buffer_duration = 0;

	// We need a non-blocking backend (implementing play/pause/flush) to be able to do buffering correctly
	// We also need to be able to query the current playing state
	bool buffering_enabled = false;

	u64 minimum_block_period = 0; // the block period will not be dynamically lowered below this value (usecs)
	u64 maximum_block_period = 0; // the block period will not be dynamically increased above this value (usecs)

	u32 desired_full_buffers = 0;
	u32 num_allocated_buffers = 0; // number of ringbuffer buffers

	const f32 period_average_alpha = 0.02f; // alpha factor for the m_average_period rolling average

	const s64 period_comparison_margin = 250; // when comparing the current period time with the desired period, if it is below this number of usecs we do not wait any longer

	u64 fully_untouched_timeout = 0; // timeout if the game has not touched any audio buffer yet
	u64 partially_untouched_timeout = 0; // timeout if the game has not touched all audio buffers yet

	/*
	 * Time Stretching
	 */

	// We need to be able to set a dynamic frequency ratio to be able to do time stretching
	bool time_stretching_enabled = false;

	f32 time_stretching_threshold = 0.0f; // we only apply time stretching below this buffer fill rate (adjusted for average period)
	const f32 time_stretching_step = 0.1f; // will only reduce/increase the frequency ratio in steps of at least this value
	const f32 time_stretching_scale = 0.9f;

	/*
	 * Constructor
	 */
	cell_audio_config();

	/*
	 * Config changes
	 */
	void reset(bool backend_changed = true);
};

class audio_ringbuffer
{
private:
	const std::shared_ptr<AudioBackend> backend;

	const cell_audio_config& cfg;

	const u32 buf_sz;

	std::unique_ptr<AudioDumper> m_dump;

	std::unique_ptr<float[]> buffer[MAX_AUDIO_BUFFERS];
	const float silence_buffer[u32{AUDIO_MAX_CHANNELS_COUNT} * u32{AUDIO_BUFFER_SAMPLES}] = { 0 };

	simple_ringbuf cb_ringbuf{};

	atomic_t<bool> backend_active = false;
	bool playing = false;
	bool emu_paused = false;

	u64 update_timestamp = 0;
	u64 play_timestamp = 0;

	u64 last_remainder = 0;
	u64 enqueued_samples = 0;

	f32 frequency_ratio = 1.0f;

	u32 cur_pos = 0;

	bool get_backend_playing() const
	{
		return backend->IsPlaying();
	}

	u32 backend_write_callback(u32 size, void *buf);

public:
	audio_ringbuffer(cell_audio_config &cfg);
	~audio_ringbuffer();

	void play();
	void enqueue(const float* in_buffer = nullptr);
	void flush();
	u64 update();
	void enqueue_silence(u32 buf_count = 1);
	f32 set_frequency_ratio(f32 new_ratio);

	float* get_buffer(u32 num) const
	{
		AUDIT(num < cfg.num_allocated_buffers);
		AUDIT(buffer[num].get() != nullptr);
		return buffer[num].get();
	}

	static u64 get_timestamp();

	float* get_current_buffer() const
	{
		return get_buffer(cur_pos);
	}

	u64 get_enqueued_samples() const
	{
		AUDIT(cfg.buffering_enabled);
		return enqueued_samples;
	}

	u64 get_enqueued_playtime(bool raw = false) const
	{
		AUDIT(cfg.buffering_enabled);
		u64 sampling_rate = raw ? cfg.audio_sampling_rate : static_cast<u64>(cfg.audio_sampling_rate * frequency_ratio);
		return enqueued_samples * 1'000'000 / sampling_rate;
	}

	bool is_playing() const
	{
		return playing;
	}

	f32 get_frequency_ratio() const
	{
		return frequency_ratio;
	}

	u32 has_capability(u32 cap) const
	{
		return backend->has_capability(cap);
	}

	bool get_operational_status() const
	{
		return backend->Operational();
	}

	const char* get_backend_name() const
	{
		return backend->GetName();
	}
};


class cell_audio_thread
{
private:
	std::unique_ptr<audio_ringbuffer> ringbuffer;

	void reset_ports(s32 offset = 0);
	void advance(u64 timestamp, bool reset = true);
	std::tuple<u32, u32, u32, u32> count_port_buffer_tags();
	template <audio_downmix downmix>
	void mix(float *out_buffer, s32 offset = 0);
	void finish_port_volume_stepping();

	constexpr static u64 get_thread_wait_delay(u64 time_left)
	{
		return (time_left > 350) ? time_left - 250 : 100;
	}

	void update_config(bool backend_changed);
	void reset_counters();

public:
	cell_audio_config cfg;
	atomic_t<audio_backend_update> m_update_configuration = audio_backend_update::NONE;

	shared_mutex mutex;
	atomic_t<u32> init = 0;

	u32 key_count = 0;
	u8 event_period = 0;

	struct key_info
	{
		u8 start_period; // Starting event_period
		u32 flags; // iFlags
		u64 source; // Event source
		std::shared_ptr<lv2_event_queue> port; // Underlying event port
	};

	std::vector<key_info> keys;
	std::array<audio_port, AUDIO_PORT_COUNT> ports;

	u64 m_last_period_end = 0;
	u64 m_counter = 0;
	u64 m_start_time = 0;
	u64 m_dynamic_period = 0;
	f32 m_average_playtime = 0.0f;
	bool m_backend_failed = false;

	void operator()();

	cell_audio_thread()
	{
	}

	audio_port* open_port()
	{
		for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
		{
			if (ports[i].state.compare_and_swap_test(audio_port_state::closed, audio_port_state::opened))
			{
				return &ports[i];
			}
		}

		return nullptr;
	}

	bool has_capability(u32 cap) const
	{
		return ringbuffer->has_capability(cap);
	}

	static constexpr auto thread_name = "cellAudio Thread"sv;
};

using cell_audio = named_thread<cell_audio_thread>;

namespace audio
{
	cell_audio_config::raw_config get_raw_config();
	void configure_audio();
}

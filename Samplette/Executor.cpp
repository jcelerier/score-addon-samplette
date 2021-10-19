#include "Executor.hpp"

#include <Process/ExecutionContext.hpp>

#include <ossia/dataflow/port.hpp>
#include <ossia/dataflow/nodes/sound_utils.hpp>

#include <Samplette/Process.hpp>
#include <ossia/dataflow/nodes/sound_ref.hpp>
#include <ossia/detail/logger.hpp>
#include <flat_map.hpp>

namespace Samplette
{
template<typename T>
struct deferred_value
{
  alignas(T) char bytes[sizeof(T)];
  deferred_value() noexcept
  {

  }

  deferred_value(const deferred_value&) = delete;
  deferred_value(deferred_value&&) = delete;
  deferred_value& operator=(const deferred_value&) = delete;
  deferred_value& operator=(deferred_value&&) = delete;

  template<typename... Args>
  T& allocate(Args&&... args)
  {
    return *new (&bytes) T(std::forward<Args>(args)...);
  }

  T& get() noexcept
  {
    return *reinterpret_cast<T*>(&bytes);
  }

  const T& get() const noexcept
  {
    return *reinterpret_cast<T*>(&bytes);
  }

  ~deferred_value()
  {
    get().~T();
  }
};


// Adapted from http://www.martin-finke.de/blog/articles/audio-plugins-011-envelopes/
struct envelope_state
{
  double value{};
  bool finished{};
};

class exponential_adsr {
public:
  static constexpr const double min_level{0.0001};

  enum Stage {
    Off,
    Attack,
    Decay,
    Sustain,
    Release
  };

  [[nodiscard]]
  static auto compute_multiplier(double start, double end, int64_t samples) noexcept
  {
    using namespace std;
    return 1.0 + (std::log(end) - std::log(start)) / (samples);
  }

  void enter_stage(Stage newStage) noexcept
  {
    if (m_stage == newStage)
      return;

    m_stage = newStage;
    m_sampleIndex = 0;

    switch (newStage)
    {
      case Off:
        m_nextStageSample = 0;
        m_level = 0.0;
        m_mult = 1.0;
        break;
      case Attack:
        m_nextStageSample = m_stages[m_stage] * m_rate;
        m_level = min_level;
        m_mult = compute_multiplier(
              m_level,
              1.0,
              m_nextStageSample);
        break;
      case Decay:
        m_nextStageSample = m_stages[m_stage] * m_rate;
        m_level = 1.0;
        m_mult = compute_multiplier(
              m_level,
              fmax(m_stages[Sustain], min_level),
              m_nextStageSample);
        break;
      case Sustain:
        m_nextStageSample = 0;
        m_level = m_stages[Sustain];
        m_mult = 1.0;
        break;
      case Release:
        m_nextStageSample = m_stages[m_stage] * m_rate;
        // We could go from ATTACK/DECAY to RELEASE,
        // so we're not changing currentLevel here.
        m_mult = compute_multiplier(m_level, min_level, m_nextStageSample);
        break;
    }
  }

  envelope_state next_sample() noexcept
  {
    envelope_state ret;

    switch(m_stage)
    {
      case Off:
        ret.value = 0.;
        ret.finished = true;
        break;

      case Sustain:
        ret.value = m_level;
        break;

      case Attack:
      case Decay:
      case Release:
      {
        if (m_sampleIndex == m_nextStageSample)
        {
          const Stage newStage = static_cast<Stage>((m_stage + 1) % 5);
          enter_stage(newStage);
          if(m_stage == Off)
            ret.finished = true;
        }
        m_level *= m_mult;
        m_sampleIndex++;

        ret.value = m_level;
        break;
      }
    }

    return ret;
  }

  void init_stage(Stage stage, double value) noexcept
  {
    m_stages[stage] = value;
  }

  void set_stage(Stage stage, double value) noexcept
  {
    init_stage(stage, value);

    if (stage == m_stage)
    {
      // Re-calculate the multiplier and nextStageSampleIndex
      if(m_stage == Attack ||
         m_stage == Decay ||
         m_stage == Release)
      {
        double nextLevelValue;
        switch (m_stage) {
          case Attack:
            nextLevelValue = 1.0;
            break;
          case Decay:
            nextLevelValue = fmax(m_stages[Sustain], min_level);
            break;
          case Release:
            nextLevelValue = min_level;
            break;
        }

        // How far the generator is into the current stage:
        double currentStageProcess = double(m_sampleIndex) / m_nextStageSample;
        // How much of the current stage is left:
        double remainingStageProcess = 1.0 - currentStageProcess;

        int64_t samplesUntilNextStage = remainingStageProcess * value * m_rate;
        m_nextStageSample = m_sampleIndex + samplesUntilNextStage;
        m_mult = compute_multiplier(m_level, nextLevelValue, samplesUntilNextStage);
      }
      else if(m_stage == Sustain)
      {
        m_level = value;
      }
    }

    if (m_stage == Decay && stage == Sustain)
    {
      // We have to decay to a different sustain value than before.
      // Re-calculate multiplier:
      int64_t samplesUntilNextStage = m_nextStageSample - m_sampleIndex;
      m_mult = compute_multiplier(
            m_level,
            fmax(m_stages[Sustain], min_level),
            samplesUntilNextStage);
    }
  }

  void reset() noexcept
  {
    m_stage = Stage::Off;
    m_level = min_level;
    m_mult = 1.0;
    m_sampleIndex = 0;
    m_nextStageSample = 0;
  }

private:
  double m_rate{44100};
  double m_level{min_level};
  double m_mult{1.0};
  double m_stages[5]{ 0.0, 0.01, 0.5, 0.1, 1.0 };
  int64_t m_sampleIndex{};
  int64_t m_nextStageSample{};
  Stage m_stage{Off};
};



class node final : public ossia::nonowning_graph_node
{
public:
  node() {
    this->root_inputs().push_back(&in);

    this->root_inputs().push_back(&trigger_mode);
    this->root_inputs().push_back(&poly_mode);
    this->root_inputs().push_back(&root);

    this->root_inputs().push_back(&gain);

    this->root_inputs().push_back(&start);
    this->root_inputs().push_back(&length);

    this->root_inputs().push_back(&loops);
    this->root_inputs().push_back(&loop_start);

    this->root_inputs().push_back(&pitch);

    this->root_inputs().push_back(&attack);
    this->root_inputs().push_back(&decay);
    this->root_inputs().push_back(&sustain);
    this->root_inputs().push_back(&release);

    this->root_inputs().push_back(&velocity);
    this->root_inputs().push_back(&fade);

    this->root_outputs().push_back(&out);
  }

  void add_voice(int note)
  {
    auto new_voice = std::make_unique<voice>();
    const auto channels = this->m_handle.get()->data.size();
    new_voice->pitcher.allocate(channels, 1024, 0); // TODO ist not gut
    //new_voice->info.m_resampler.reset(0, ossia::audio_stretch_mode::Repitch, m_handle->data.size(), this->m_dataSampleRate);

    // Playback speed
    if(m_root != 0)
    {
      // Tempo
      // m_handle at pitch m_root
      // we want: to set the resampler to put it at note's pitch
      ossia::frequency src_freq = m_root;
      ossia::frequency dst_freq = ossia::midi_pitch{note};
      new_voice->note_speed_ratio = dst_freq.dataspace_value / src_freq.dataspace_value; // TODO /0
    }
    new_voice->info.tempo = ossia::root_tempo * new_voice->note_speed_ratio;

    // Envelope
    {
      // m_attack / m_decay / m_release are in seconds
      new_voice->envelope.init_stage(exponential_adsr::Attack, this->m_attack);
      new_voice->envelope.init_stage(exponential_adsr::Decay, this->m_decay);
      new_voice->envelope.init_stage(exponential_adsr::Sustain, this->m_sustainGain);
      new_voice->envelope.init_stage(exponential_adsr::Release, this->m_release);

      new_voice->envelope.enter_stage(exponential_adsr::Attack);
    }

    voices[note] = std::move(new_voice);
  }

  void remove_voice(int note)
  {
    voices.erase(note);
  }
  void start_release(int note)
  {
    auto it = voices.find(note);
    if(it != voices.end())
    {
      it->second->envelope.enter_stage(exponential_adsr::Release);
    }
  }

  void set_sound(const ossia::audio_handle& hdl, int channels, int sampleRate)
  {
    m_handle = hdl;
    m_data.clear();
    if (hdl)
    {
      m_dataSampleRate = sampleRate;
      m_data.assign(m_handle->data.begin(), m_handle->data.end());
    }
  }

  void process_midi()
  {
    // First parse the MIDI input
    for(libremidi::message& m : in->messages) {
      switch(m.get_message_type()) {
        case libremidi::message_type::NOTE_ON:
          if(this->m_polyMode == Mono)
          {
            voices.clear();
          }
          add_voice(m.bytes[1]);
          break;
        case libremidi::message_type::NOTE_OFF:
          switch(this->m_triggerMode)
          {
            case Trigger:
              start_release(m.bytes[1]);
              break;
            case Gate:
              remove_voice(m.bytes[1]);
              break;
          }
          break;
        case libremidi::message_type::PITCH_BEND:
          m_midiPitchShift = (m.bytes[2] * 128 + m.bytes[1] - 8192.) / 10.;
          break;
        default:
          break;
      }
    }
  }

  template<typename T>
  static bool read_control(const ossia::value_port& in, auto& out) {
    auto& d = in.get_data();
    if(!d.empty())
    {
      out = ossia::convert<T>(d.back().value);
      return true;
    }
    return false;
  }

  void process_controls()
  {
    std::optional<bool> trigger_mode_v;
    read_control<bool>(*this->trigger_mode, trigger_mode_v);
    if(trigger_mode_v) {
      this->m_triggerMode = *trigger_mode_v ? Trigger : Gate;
    }
    std::optional<std::string> poly_mode_v;
    read_control<std::string>(*this->poly_mode, poly_mode_v);
    if(poly_mode_v) {
      if(*poly_mode_v == "Mono")
        this->m_polyMode = Mono;
      else
        this->m_polyMode = Poly;
    }

    std::optional<int> root_v{};
    read_control<int>(*this->root, root_v);
    if(root_v) {
      this->m_root = *root_v;
    }

    read_control<float>(*this->gain, this->m_gain);
    if(read_control<float>(*this->start, this->m_start))
      this->m_start /= 100.;

    if(read_control<float>(*this->length, this->m_length))
      this->m_length /= 100.;

    read_control<bool>(*this->loops, this->m_loops);
    if(read_control<float>(*this->loop_start, this->m_loopStart))
      this->m_loopStart /= 100.;

    read_control<float>(*this->pitch, this->m_userPitchShift);

    // UI control is in msec, ADSR is in sec
    if(read_control<float>(*this->attack, this->m_attack))
      this->m_attack /= 1000.;
    if(read_control<float>(*this->decay, this->m_decay))
      this->m_decay /= 1000.;
    read_control<float>(*this->sustain, this->m_sustainGain);
    if(read_control<float>(*this->release, this->m_release))
      this->m_release /= 1000.;

    read_control<float>(*this->velocity, this->m_velocity);
    read_control<float>(*this->fade, this->m_fade);
  }

  void
  run(const ossia::token_request& tk,
      ossia::exec_state_facade s) noexcept override
  {
    process_controls();
    process_midi();

    const auto [first_pos, tick_duration] = s.timings(tk);
    if(tick_duration <= 0)
      return;

    const auto channels = this->m_handle.get()->data.size();
    const auto len = this->m_handle.get()->data[0].size();

    // Play all our voices
    for(auto it = voices.begin(); it != voices.end(); )
    {
      auto& [note, voice_ptr] = *it;
      auto& voice = *voice_ptr;

      // Clear prev channel output
      voice.port.samples.resize(channels);
      for(auto& c : voice.port.samples)
      {
        c.clear();
        c.resize(tick_duration);
      }

      // Setup timing
      voice.timing.tempo = ossia::root_tempo * voice.note_speed_ratio + m_midiPitchShift + m_userPitchShift;
      voice.timing.date += tick_duration;
      if(voice.timing.tempo <= 0.000001)
      {
        ++it;
        continue;
      }

      // Execute
      int64_t samples_to_read = s.bufferSize() * (voice.timing.tempo / ossia::root_tempo);
      int64_t samples_to_write = s.bufferSize();
      int64_t samples_offset = 0;

      const int64_t total_samples = this->m_handle->data[0].size();
      const int64_t start_offset = total_samples * this->m_start;
      const int64_t main_length = (total_samples - start_offset) * this->m_length;
      const int64_t loop_start = (total_samples - start_offset) * this->m_loopStart;

      //int loop_duration = this->m_handle->data[0].size();

      ossia::mutable_audio_span<double> output = voice.port;
      struct {
       ossia::audio_span<float>& m_data;
        const int64_t& start_offset;
        const int64_t& loop_duration;
        node& n;
        void fetch_audio (
              const int64_t start,
              const int64_t samples_to_write,
              float** const audio_array)  {
          ossia::read_audio_from_buffer(m_data, start, samples_to_write, start_offset, loop_duration, n.m_loops, audio_array);
        }
      } fetcher{m_data, start_offset, main_length, *this};

      voice.pitcher.get().run(fetcher,
            voice.timing,
            s,
            ossia::root_tempo / voice.timing.tempo,
            channels, len, samples_to_read, samples_to_write, samples_offset, output);
      voice.timing.prev_date = voice.timing.date;

      // Copy samples to output
      this->out->samples.resize(std::max(this->out->samples.size(), voice.port.samples.size()));
      auto& voice_samples = voice.port.samples;
      auto& out_samples = this->out->samples;

      // Make sure we have enough space
      const std::size_t channels = voice_samples.size();
      const std::size_t num_samples = voice_samples[0].size();
      for(std::size_t channel = 0; channel < channels; channel++) {
        out_samples.resize(std::max(out_samples.size(), voice_samples.size()));
        auto& out_channel = out_samples[channel];
        out_samples[channel].resize(std::max(out_channel.size(), num_samples));
      }

      // Apply envelope, gain... while copying the output
      envelope_state env;
      for(std::size_t i = 0; i < num_samples; i++)
      {
        env = voice.envelope.next_sample();

        if(env.finished)
          break;

        env.value *= this->m_gain;
        for(std::size_t channel = 0; channel < channels; channel++) {
          auto& out_channel = out_samples[channel];
          const auto& voice_channel = voice_samples[channel];

          out_channel[i] += voice_channel[i] * env.value;
        }
      }

      if(env.finished)
        it = voices.erase(it);
      else
        ++it;
    }
  }

  std::string label() const noexcept override { return "samplette"; }

  ossia::midi_inlet in;

  ossia::value_inlet trigger_mode;
  ossia::value_inlet poly_mode;
  ossia::value_inlet root;

  ossia::value_inlet gain;

  ossia::value_inlet start;
  ossia::value_inlet length;

  ossia::value_inlet loops;
  ossia::value_inlet loop_start;

  ossia::value_inlet pitch;

  ossia::value_inlet attack;
  ossia::value_inlet decay;
  ossia::value_inlet sustain;
  ossia::value_inlet release;

  ossia::value_inlet velocity;
  ossia::value_inlet fade;

  ossia::audio_outlet out;

  struct voice {
    ossia::sound_processing_info info;
    ossia::audio_port port;
    deferred_value<ossia::repitch_stretcher> pitcher;
    exponential_adsr envelope;
    //ossia::nodes::sound_sampler sampler{&info, &port, {}};
    ossia::token_request timing;
    double note_speed_ratio{1.0};
    bool in_loop{};
  };

  ossia::fast_hash_map<int, std::unique_ptr<voice>> voices;

  ossia::audio_span<float> m_data;

  std::size_t m_dataSampleRate{};
  ossia::audio_handle m_handle{};

  enum { Trigger, Gate } m_triggerMode{};
  enum { Mono, Poly } m_polyMode{};

  ossia::midi_pitch m_root{60};

  double m_gain{1.};

  bool m_loops{false};

  // The three values below in percentages
  double m_start{0.};
  double m_length{1.};
  double m_loopStart{0.};
  //double m_loopEnd{1.};

  double m_userPitchShift{0.};
  double m_midiPitchShift{0.};

  double m_attack{0.};
  double m_decay{1.};
  double m_sustainGain{1.};
  double m_release{0.};

  double m_velocity{};
  double m_fade{};
};

ProcessExecutorComponent::ProcessExecutorComponent(
    Samplette::Model& element,
    const Execution::Context& ctx,
    QObject* parent)
    : ProcessComponent_T{element, ctx, "SampletteExecutorComponent", parent}
{
  auto n = std::make_shared<Samplette::node>();

  SCORE_ASSERT(element.file()->finishedDecoding());
  auto file = element.file()->unsafe_handle().target<std::shared_ptr<Media::AudioFile::LibavReader>>();
  SCORE_ASSERT(file);
  auto& snd = *file;
  SCORE_ASSERT(snd);

  n->set_sound(snd->handle, snd->decoder.channels, snd->decoder.fileSampleRate);
  this->node = n;
  m_ossia_process = std::make_shared<ossia::node_process>(n);

#define map_func(model, exec, T, func) \
  n->exec = func(ossia::convert<T>(element.model->value())); \
  connect(element.model.get(), &Process::ControlInlet::valueChanged, this, [this, n] (const ossia::value& v) { \
    in_exec([val = func(ossia::convert<T>(v)), n] { n->exec = val; }); \
  });

  map_func(trigger_mode, m_triggerMode, bool, [] (bool t) { return t ? node::Gate : node::Trigger; });
  map_func(poly_mode, m_polyMode, std::string, [] (const std::string& t) { return t == "Mono" ? node::Mono : node::Poly; });
  map_func(root, m_root, int, [] (int t) { return t; });

  map_func(gain, m_gain, float, [] (float t) { return t; });

  map_func(start, m_start, float, [] (float t) { return t / 100.; });
  map_func(length, m_length, float, [] (float t) { return t / 100.; });

  map_func(loops, m_loops, bool, [] (bool t) { return t; });
  map_func(loop_start, m_loopStart, float, [] (float t) { return t / 100.; });

  map_func(pitch, m_userPitchShift, float, [] (float t) { return t; });

  map_func(attack, m_attack, float, [] (float t) { return t / 1000.; });
  map_func(decay, m_decay, float, [] (float t) { return t / 1000.; });
  map_func(sustain, m_sustainGain, float, [] (float t) { return t; });
  map_func(release, m_release, float, [] (float t) { return t / 1000.; });

  map_func(velocity, m_velocity, float, [] (float t) { return t / 100.; });

  map_func(fade, m_fade, float, [] (float t) { return t / 100.; });

#undef map_func
  connect(&element, &Samplette::Model::fileChanged, this, [&, n] {\
    auto file = element.file()->unsafe_handle().target<std::shared_ptr<Media::AudioFile::LibavReader>>();
    SCORE_ASSERT(file);

    in_exec([=] {
              auto& snd = *file;
              SCORE_ASSERT(snd);
              n->set_sound(snd->handle, snd->decoder.channels, snd->decoder.fileSampleRate);
    });
  });

}
}

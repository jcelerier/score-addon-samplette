#pragma once
#include <Process/GenericProcessFactory.hpp>
#include <Process/Process.hpp>
#include <Process/Dataflow/Port.hpp>
#include <Media/MediaFileHandle.hpp>

#include <Samplette/Metadata.hpp>

namespace Samplette
{
class Model final : public Process::ProcessModel
{
  SCORE_SERIALIZE_FRIENDS
  PROCESS_METADATA_IMPL(Samplette::Model)
  W_OBJECT(Model)

public:
  Model(
      const TimeVal& duration,
      const Id<Process::ProcessModel>& id,
      QObject* parent);

  template <typename Impl>
  Model(Impl& vis, QObject* parent)
      : Process::ProcessModel{vis, parent}
  {
    vis.writeTo(*this);
    init();
  }

  ~Model() override;

  void setFileForced(const QString& file);
  const std::shared_ptr<Media::AudioFile>& file() const noexcept { return m_file; }

  void fileChanged() W_SIGNAL(fileChanged)

  std::unique_ptr<Process::MidiInlet> inlet;

  std::unique_ptr<Process::ControlInlet> trigger_mode; // trigger / gate
  std::unique_ptr<Process::ControlInlet> poly_mode; // mono / poly
  std::unique_ptr<Process::ControlInlet> root; // root note

  std::unique_ptr<Process::ControlInlet> gain;

  std::unique_ptr<Process::ControlInlet> start;
  std::unique_ptr<Process::ControlInlet> length;

  std::unique_ptr<Process::ControlInlet> loops;
  std::unique_ptr<Process::ControlInlet> loop_start;

  std::unique_ptr<Process::ControlInlet> pitch;

  std::unique_ptr<Process::ControlInlet> attack;
  std::unique_ptr<Process::ControlInlet> decay;
  std::unique_ptr<Process::ControlInlet> sustain;
  std::unique_ptr<Process::ControlInlet> release;

  std::unique_ptr<Process::ControlInlet> velocity;
  std::unique_ptr<Process::ControlInlet> fade;

  std::unique_ptr<Process::AudioOutlet> outlet;

  void for_each_control(auto&& f)
  {
    f(this->trigger_mode);
    f(this->poly_mode);
    f(this->root);

    f(this->gain);

    f(this->start);
    f(this->length);

    f(this->loops);
    f(this->loop_start);

    f(this->pitch);

    f(this->attack);
    f(this->decay);
    f(this->sustain);
    f(this->release);

    f(this->velocity);
    f(this->fade);
  }

private:
  void init();
  void loadFile(const QString& str);
  QString prettyName() const noexcept override;

  std::shared_ptr<Media::AudioFile> m_file;
};

using ProcessFactory = Process::ProcessFactory_T<Samplette::Model>;
}

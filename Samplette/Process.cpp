#include "Process.hpp"

#include <score/tools/File.hpp>
#include <Process/Dataflow/PortFactory.hpp>
#include <Process/Dataflow/Port.hpp>
#include <Process/Dataflow/PortSerialization.hpp>
#include <wobjectimpl.h>
#include <Control/Widgets.hpp>

W_OBJECT_IMPL(Samplette::Model)
namespace Samplette
{

Model::Model(
    const TimeVal& duration,
    const Id<Process::ProcessModel>& id,
    QObject* parent)
    : Process::ProcessModel{duration, id, "SampletteProcess", parent}
    , inlet{Process::make_midi_inlet(Id<Process::Port>(0), this)}
    , trigger_mode{new Process::ChooserToggle({"Trigger", "Gate"}, false, "Trigger", Id<Process::Port>(1), this)}
    , poly_mode{new Process::Enum(QStringList{"Mono", "Poly"}, {}, "Mono", "Polyphony", Id<Process::Port>(2), this)}
    , root{new Process::IntSlider(0, 127, 40, "Root note", Id<Process::Port>(3), this)}

    , gain{new Process::LogFloatSlider(0, 2, 1, "Gain", Id<Process::Port>(4), this)}

    , start{new Process::FloatKnob(0, 100, 0, "Start", Id<Process::Port>(5), this)}
    , length{new Process::FloatKnob(0, 100, 100, "Length", Id<Process::Port>(6), this)}

    , loops{new Process::Toggle(false, "Loops", Id<Process::Port>(7), this)}
    , loop_start{new Process::FloatKnob(0, 100, 0, "Loop start", Id<Process::Port>(8), this)}

    , pitch{new Process::FloatKnob(-24, 24, 0, "Pitch", Id<Process::Port>(9), this)}

    , attack{new Process::LogFloatSlider(0, 20000, 0, "Attack", Id<Process::Port>(10), this)}
    , decay{new Process::LogFloatSlider(1, 60000, 0, "Decay", Id<Process::Port>(11), this)}
    , sustain{new Process::LogFloatSlider(0, 1, 1, "Sustain", Id<Process::Port>(12), this)}
    , release{new Process::LogFloatSlider(1, 60000, 0, "Release", Id<Process::Port>(13), this)}

    , velocity{new Process::FloatKnob(0, 100, 0, "Velocity", Id<Process::Port>(14), this)}
    , fade{new Process::FloatKnob(0, 100, 0, "Fade", Id<Process::Port>(15), this)}

    , outlet{Process::make_audio_outlet(Id<Process::Port>(100), this)}
{
  outlet->setPropagate(true);
  metadata().setInstanceName(*this);
  init();
  //loadFile("/home/jcelerier/Documents/ossia/score/packages/dirt-samples/ade/006_glass.wav");
  loadFile("/home/jcelerier/Documents/ossia/score/packages/dirt-samples/bass3/83253__zgump__bass-0209.wav");
}

Model::~Model() { }

void Model::init()
{
  m_inlets.push_back(inlet.get());
  for_each_control([this] (auto& ctl) { m_inlets.push_back(ctl.get()); });
  m_outlets.push_back(outlet.get());
}

void Model::setFileForced(const QString& file)
{
  SCORE_ASSERT(QFile::exists(file));
  loadFile(file);
}

QString Model::prettyName() const noexcept
{
  return tr("Samplette");
}

void Model::loadFile(const QString& file)
{
  if(m_file)
    m_file->on_mediaChanged.disconnect<&Model::fileChanged>(*this);

  auto& ctx = score::IDocument::documentContext(*this);
  auto r = std::make_shared<Media::AudioFile>();
  auto abspath = score::locateFilePath(file, ctx);
  r->load(file, file, Media::DecodingMethod::Libav);
  m_file = r;

  m_file->on_mediaChanged.connect<&Model::fileChanged>(*this);

  fileChanged();
}

}

template <>
void DataStreamReader::read(const Samplette::Model& proc)
{
  readPorts(*this, proc.m_inlets, proc.m_outlets);
  m_stream << proc.m_file->originalFile();

  insertDelimiter();
}

template <>
void DataStreamWriter::write(Samplette::Model& proc)
{
  writePorts(
      *this,
      components.interfaces<Process::PortFactoryList>(),
      proc.m_inlets,
      proc.m_outlets,
      &proc);

  QString s;
  m_stream >> s;
  proc.loadFile(s);

  checkDelimiter();
}

template <>
void JSONReader::read(const Samplette::Model& proc)
{
  readPorts(*this, proc.m_inlets, proc.m_outlets);
  obj["File"] = proc.m_file->originalFile();
}

template <>
void JSONWriter::write(Samplette::Model& proc)
{
  writePorts(
      *this,
      components.interfaces<Process::PortFactoryList>(),
      proc.m_inlets,
      proc.m_outlets,
      &proc);


  proc.loadFile(obj["File"].toString());
}

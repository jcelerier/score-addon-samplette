#include <Samplette/CommandFactory.hpp>
#include <Samplette/Process.hpp>
#include <score/model/path/PathSerialization.hpp>

namespace Samplette
{

ChangeAudioFile::ChangeAudioFile(
    const Model& model,
    const QString& text)
    : m_model{model}
    , m_new{text}
{
  m_old = model.file()->originalFile();
}

void ChangeAudioFile::undo(const score::DocumentContext& ctx) const
{
  auto& snd = m_model.find(ctx);
  snd.setFileForced(m_old);
}

void ChangeAudioFile::redo(const score::DocumentContext& ctx) const
{
  auto& snd = m_model.find(ctx);
  snd.setFileForced(m_new);
}

void ChangeAudioFile::serializeImpl(DataStreamInput& s) const
{
  s << m_model << m_old << m_new;
}

void ChangeAudioFile::deserializeImpl(DataStreamOutput& s)
{
  s >> m_model >> m_old >> m_new;
}

}

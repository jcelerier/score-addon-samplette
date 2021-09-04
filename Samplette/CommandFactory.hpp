#pragma once

#include <Samplette/Process.hpp>
#include <score/command/Command.hpp>


namespace Samplette
{
class Model;
inline const CommandGroupKey& CommandFactoryName()
{
  static const CommandGroupKey key{"Samplette"};
  return key;
}

class ChangeAudioFile final : public score::Command
{
  SCORE_COMMAND_DECL(
      CommandFactoryName(),
      ChangeAudioFile,
      "Change audio file")
public:
  ChangeAudioFile(
      const Model&,
      const QString& text);

  void undo(const score::DocumentContext& ctx) const override;
  void redo(const score::DocumentContext& ctx) const override;

protected:
  void serializeImpl(DataStreamInput& s) const override;
  void deserializeImpl(DataStreamOutput& s) override;

private:
  Path<Model> m_model;
  QString m_old, m_new;
};

}

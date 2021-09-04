#pragma once
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegateFactory.hpp>

#include <score/command/Dispatchers/CommandDispatcher.hpp>

#include <Samplette/Process.hpp>

namespace Samplette
{
class InspectorWidget final
    : public Process::InspectorWidgetDelegate_T<Samplette::Model>
{
public:
  explicit InspectorWidget(
      const Samplette::Model& object,
      const score::DocumentContext& context,
      QWidget* parent);
  ~InspectorWidget() override;

private:
  CommandDispatcher<> m_dispatcher;
};

class InspectorFactory final
    : public Process::InspectorWidgetDelegateFactory_T<Model, InspectorWidget>
{
  SCORE_CONCRETE("39162fc6-6d17-41b7-802a-aa2c8b0d64e4")
};
}

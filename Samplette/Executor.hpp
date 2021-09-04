#pragma once
#include <Process/Execution/ProcessComponent.hpp>

#include <ossia/dataflow/node_process.hpp>

namespace Samplette
{
class Model;
class ProcessExecutorComponent final
    : public Execution::
          ProcessComponent_T<Samplette::Model, ossia::node_process>
{
  COMPONENT_METADATA("67954c3b-10b7-42a0-9c07-579f2f6e85d6")
public:
  ProcessExecutorComponent(
      Model& element,
      const Execution::Context& ctx,
      QObject* parent);
};

using ProcessExecutorComponentFactory
    = Execution::ProcessComponentFactory_T<ProcessExecutorComponent>;
}

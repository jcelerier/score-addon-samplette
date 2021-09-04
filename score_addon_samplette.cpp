#include "score_addon_samplette.hpp"

#include <score/plugins/FactorySetup.hpp>

#include <Samplette/CommandFactory.hpp>
#include <Samplette/Executor.hpp>
#include <Samplette/Inspector.hpp>
#include <Samplette/Layer.hpp>
#include <Samplette/LocalTree.hpp>
#include <Samplette/Process.hpp>
#include <score_addon_samplette_commands_files.hpp>

score_addon_samplette::score_addon_samplette() { }

score_addon_samplette::~score_addon_samplette() { }

std::vector<std::unique_ptr<score::InterfaceBase>>
score_addon_samplette::factories(
    const score::ApplicationContext& ctx,
    const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext,
      FW<Process::ProcessModelFactory, Samplette::ProcessFactory>,
      FW<Process::LayerFactory, Samplette::LayerFactory>,
      FW<Execution::ProcessComponentFactory,
         Samplette::ProcessExecutorComponentFactory>>(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap>
score_addon_samplette::make_commands()
{
  using namespace Samplette;
  std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
      CommandFactoryName(), CommandGeneratorMap{}};

  ossia::for_each_type<
#include <score_addon_samplette_commands.hpp>
      >(score::commands::FactoryInserter{cmds.second});

  return cmds;
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_addon_samplette)

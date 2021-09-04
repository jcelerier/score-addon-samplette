#include "LocalTree.hpp"

#include <LocalTree/Property.hpp>

#include <Samplette/Process.hpp>

namespace Samplette
{
LocalTreeProcessComponent::LocalTreeProcessComponent(
    ossia::net::node_base& parent,
    Samplette::Model& proc,
    const score::DocumentContext& sys,
    QObject* parent_obj)
    : LocalTree::ProcessComponent_T<Samplette::Model>{
        parent,
        proc,
        sys,
        "SampletteComponent",
        parent_obj}
{
}

LocalTreeProcessComponent::~LocalTreeProcessComponent() { }
}

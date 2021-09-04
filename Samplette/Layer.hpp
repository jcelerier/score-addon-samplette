#pragma once
#include <Process/GenericProcessFactory.hpp>

#include <Samplette/Presenter.hpp>
#include <Samplette/Process.hpp>
#include <Samplette/View.hpp>

namespace Samplette
{
using LayerFactory = Process::
    LayerFactory_T<Samplette::Model, Samplette::Presenter, Samplette::View>;
}

#include <score/command/Dispatchers/CommandDispatcher.hpp>

#include <Samplette/CommandFactory.hpp>
#include <Samplette/Presenter.hpp>
#include <Samplette/Process.hpp>
#include <Samplette/View.hpp>

#include <Media/Sound/Drop/SoundDrop.hpp>
namespace Samplette
{
Presenter::Presenter(
    const Model& layer,
    View* view,
    const Process::Context& ctx,
    QObject* parent)
    : Process::LayerPresenter{layer, view, ctx, parent}
    , m_model{layer}
    , m_view{view}
{
  connect(view, &View::dropReceived, this, &Presenter::on_drop);
}

void Presenter::setWidth(qreal val, qreal defaultWidth)
{
  m_view->setWidth(val);
  m_view->recompute();
}

void Presenter::setHeight(qreal val)
{
  m_view->setHeight(val);
  m_view->recompute();
}

void Presenter::putToFront()
{
  m_view->setOpacity(1);
}

void Presenter::putBehind()
{
  m_view->setOpacity(0.2);
}

void Presenter::on_zoomRatioChanged(ZoomRatio z)
{
}

void Presenter::on_drop(const QMimeData* mime)
{
  Media::Sound::DroppedAudioFiles drops{context().context, *mime};
  if (!drops.valid() || drops.files.size() != 1)
  {
    return;
  }

  CommandDispatcher<> disp{context().context.commandStack};
  disp.submit<ChangeAudioFile>(
      static_cast<const Model&>(m_process),
      std::move(drops.files.front().first));
}

void Presenter::parentGeometryChanged()
{
  m_view->recompute();
}

}

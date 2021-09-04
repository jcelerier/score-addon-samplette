#include "View.hpp"
#include <Samplette/Process.hpp>


#include <Process/Style/ScenarioStyle.hpp>
#include <score/graphics/GraphicsItem.hpp>
#include <Media/Sound/QImagePool.hpp>
#include <score/tools/ThreadPool.hpp>
#include <score/tools/std/Invoke.hpp>

#include <QGraphicsView>
#include <QPainter>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Samplette::View)
namespace Samplette
{

View::View(const Model& m, QGraphicsItem* parent)
    : Process::LayerView{parent}
    , m_cpt{new Media::Sound::WaveformComputer{}}
{
  connect(
      m_cpt,
      &Media::Sound::WaveformComputer::ready,
      this,
      [=](QVector<QImage*> img, Media::Sound::ComputedWaveform wf) {
        {
          Media::Sound::QImagePool::instance().giveBack(m_images);
          m_images = std::move(img);

          // We display the image at the device ratio of the view
          auto view = ::getView(*this);
          if (view)
          {
            for (auto image : m_images)
            {
              image->setDevicePixelRatio(view->devicePixelRatioF());
            }
          }
        }
        m_wf = wf;

        update();
      });

  connect(&m, &Model::fileChanged, this, [this, &m] { setData(m.file()); });
  setData(m.file());
}

void View::setData(const std::shared_ptr<Media::AudioFile>& data)
{
  if (m_data)
  {
    m_data->on_finishedDecoding.disconnect<&View::recompute>(*this);
  }

  SCORE_ASSERT(data);

  m_data = data;
  if (m_data)
  {
    m_data->on_finishedDecoding.connect<&View::recompute>(
        *this);
    recompute();
  }
}

void View::recompute() const
{
  if (Q_UNLIKELY(!m_data))
    return;

  if (auto view = getView(*this))
  {
    const auto samples = m_data->decodedSamples();
    const auto seconds = double(samples) / m_data->sampleRate();
    const auto flicks = seconds * ossia::flicks_per_second<double>;
    double flicks_per_pixel = flicks / width();

    Media::Sound::WaveformRequest req{
        m_data,
        flicks_per_pixel,
        1.0,
        QSizeF{width(), height()},
        view->devicePixelRatioF(),
        mapFromScene(view->mapToScene(0, 0)).x(),
        mapFromScene(view->mapToScene(view->width(), 0)).x(),
        TimeVal::zero(),
        TimeVal::fromMsecs(1000. * seconds),
        false,
        true};

    m_cpt->recompute(std::move(req));
  }
}


View::~View()
{
  ossia::qt::run_async(m_cpt, &QObject::deleteLater);
  m_cpt = nullptr;

  score::ThreadPool::instance().releaseThread();
}

void View::paint_impl(QPainter* painter) const
{
  if (!m_data)
    return;

  int channels = m_images.size();
  if (channels == 0.)
  {
    //if (!m_recomputed)
    //  recompute();
    return;
  }

  const auto samples = m_data->decodedSamples();
  const auto seconds = double(samples) / m_data->sampleRate();
  const auto flicks = seconds * ossia::flicks_per_second<double>;
  double flicks_per_pixel = flicks / width();

  auto ratio = m_wf.zoom / flicks_per_pixel;

  const qreal w = (m_wf.xf - m_wf.x0) * ratio;
  const qreal h = height() / channels;

  const double x0 = m_wf.x0 * ratio;

  painter->setRenderHint(QPainter::SmoothPixmapTransform, 0);
  for (int i = 0; i < channels; i++)
  {
    painter->drawImage(QRectF{x0, h * i, w, h}, *m_images[i]);
  }
  painter->setRenderHint(QPainter::SmoothPixmapTransform, 1);
}

void View::dropEvent(QGraphicsSceneDragDropEvent* event)
{
  dropReceived(event->mimeData());
  event->accept();
}

}

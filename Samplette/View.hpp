#pragma once
#include <Process/LayerView.hpp>
#include <Media/MediaFileHandle.hpp>
#include <Media/Sound/WaveformComputer.hpp>

#include <QMimeData>
namespace Samplette
{
class Model;
class View final : public Process::LayerView
    , public Nano::Observer
{
  W_OBJECT(View)
public:
  explicit View(const Model& m, QGraphicsItem* parent);
  ~View() override;

  void recompute() const;
  void setData(const std::shared_ptr<Media::AudioFile>& data);

  void dropReceived(const QMimeData* mime) W_SIGNAL(dropReceived, mime)
private:

  void paint_impl(QPainter*) const override;


  void dropEvent(QGraphicsSceneDragDropEvent* event) override;

  std::shared_ptr<Media::AudioFile> m_data;

  QVector<QImage*> m_images;
  Media::Sound::WaveformComputer* m_cpt{};

  Media::Sound::ComputedWaveform m_wf{};
};
}

W_REGISTER_ARGTYPE(const QMimeData*)

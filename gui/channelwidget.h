#ifndef D2W_CHANNELWIDGET_H
#define D2W_CHANNELWIDGET_H

#include <QGroupBox>
class QCheckBox;
class Channel;
class SynthContext;
class VUMeter;
class ChannelWidget;

class ChannelCheckBox : public QWidget
{
  Q_OBJECT
public:
  ChannelCheckBox(int index, Channel* channel, ChannelWidget* parent);

  QCheckBox* chk;
  VUMeter* vu;
  Channel* channel;

  void updateMeter(double timestamp);

public slots:
  void setActive(bool muted);
  void setSolo();
};

class ChannelWidget : public QGroupBox
{
  Q_OBJECT
public:
  ChannelWidget(QWidget* parent);

  void setSolo(ChannelCheckBox* channel);

public slots:
  void contextUpdated(SynthContext* context);
  void updateMeters();
  void unmuteAll();

private:
  SynthContext* context;
  QList<ChannelCheckBox*> channels;
  double lastUpdate;
};

#endif

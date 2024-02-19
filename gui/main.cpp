#include <QApplication>
#include "mainwindow.h"
#include "clefcontext.h"
#include "channelwidget.h"
#include "playercontrols.h"
#include "synth/synthcontext.h"
#include "plugin/baseplugin.h"
#include <QtDebug>

class GbampWindow : public MainWindow
{
public:
  GbampWindow(ClefPluginBase* plugin) : MainWindow(plugin)
  {
    resize(400, 200);
  }

  QWidget* createPluginWidget(QWidget* parent)
  {
    ChannelWidget* cw = new ChannelWidget(parent);
    QObject::connect(controls, SIGNAL(bufferUpdated()), cw, SLOT(updateMeters()));
    return cw;
  }
};

int main(int argc, char** argv)
{
  ClefContext ctx;
  ClefPluginBase* plugin = Clef::makePlugin(&ctx);

  QCoreApplication::setApplicationName(QString::fromStdString(plugin->pluginName()));
  QCoreApplication::setApplicationVersion(QString::fromStdString(plugin->version()));
  QCoreApplication::setOrganizationName("libclef");
  QCoreApplication::setOrganizationDomain("libclef" + QString::fromStdString(plugin->pluginShortName()));
  QApplication app(argc, argv);

  GbampWindow mw(plugin);
  mw.show();
  if (app.arguments().length() > 1) {
    mw.openFile(app.arguments()[1], true);
  }

  return app.exec();
}

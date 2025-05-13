// -*- Mode: C++ -*-
#ifndef DISPLAYTEXT_H
#define DISPLAYTEXT_H

#include <QTextEdit>
#include <QFont>
#include <QHash>
#include <QPair>
#include <QString>

#include "decodedtext.h"
#include "Configuration.hpp"

class QAction;
class Configuration;
class LogBook;

class DisplayText
  : public QTextEdit
{
  Q_OBJECT
public:
  explicit DisplayText(QWidget *parent = 0);
  void set_configuration (Configuration const * configuration) {m_config = configuration;}
  void setContentFont (QFont const&);
  void insertLineSpacer(QString const&);
  void displayDecodedText(QString displayMsg, DecodedText const& decodedText, QString const& myCall, QString const& mode,
                          bool displayDXCCEntity, LogBook const& logBook,
                          QString const& currentBand=QString {}, bool ppfx=false, bool bCQonly=false, bool dim=false);      //avt 9/15/22
  void displayTransmittedText(QString text, QString modeTx, qint32 txFreq, bool bFastMode);
  void displayQSY(QString text);
  void displayFoxToBeCalled(QString t, QColor bg = QColor {}, QColor fg = QColor {});

  Q_SIGNAL void selectCallsign (Qt::KeyboardModifiers);
  Q_SIGNAL void erased ();

  Q_SLOT void appendText (QString const& text, QColor bg = QColor {}, QColor fg = QColor {}
                          , QString const& call1 = QString {}, QString const& call2 = QString {});
  Q_SLOT void erase ();
  Q_SLOT void highlight_callsign (QString const& callsign, QColor const& bg, QColor const& fg, bool last_only);

protected:
  void mouseDoubleClickEvent(QMouseEvent *e);

private:
  Configuration const * m_config;
  bool m_bPrincipalPrefix;
  QString appendWorkedB4(QString message, QString callsign
                         , QString const& grid, QColor * bg, QColor * fg
                         , LogBook const& logBook, QString const& currentBand
                         , QString const& currentMode);
  QFont char_font_;
  QAction * erase_action_;
  QHash<QString, QPair<QColor, QColor>> highlighted_calls_;
};

  extern QHash<QString,int> m_LoTW;

#endif // DISPLAYTEXT_H

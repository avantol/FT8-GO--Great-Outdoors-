#include "logqso.h"

#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

#include "logbook/logbook.h"
#include "MessageBox.hpp"
#include "Configuration.hpp"
#include "models/Bands.hpp"
#include "models/CabrilloLog.hpp"
#include "validators/MaidenheadLocatorValidator.hpp"

#include "ui_logqso.h"
#include "moc_logqso.cpp"

LogQSO::LogQSO(QString const& programTitle, QSettings * settings
               , Configuration const * config, QWidget *parent)
  : QDialog {parent, Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint}
  , ui(new Ui::LogQSO)
  , m_settings (settings)
  , m_config {config}
{
  ui->setupUi(this);
  setWindowTitle(programTitle + " - Log QSO");
  loadSettings ();
  ui->grid->setValidator (new MaidenheadLocatorValidator {this});
}

LogQSO::~LogQSO ()
{
}

void LogQSO::loadSettings ()
{
  m_settings->beginGroup ("LogQSO");
  restoreGeometry (m_settings->value ("geometry", saveGeometry ()).toByteArray ());
  ui->cbTxPower->setChecked (m_settings->value ("SaveTxPower", false).toBool ());
  ui->cbComments->setChecked (m_settings->value ("SaveComments", false).toBool ());
  m_txPower = m_settings->value ("TxPower", "").toString ();
  m_comments = m_settings->value ("LogComments", "").toString();
  m_settings->endGroup ();
}

void LogQSO::storeSettings () const
{
  m_settings->beginGroup ("LogQSO");
  m_settings->setValue ("geometry", saveGeometry ());
  m_settings->setValue ("SaveTxPower", ui->cbTxPower->isChecked ());
  m_settings->setValue ("SaveComments", ui->cbComments->isChecked ());
  m_settings->setValue ("TxPower", m_txPower);
  m_settings->setValue ("LogComments", m_comments);
  m_settings->endGroup ();
}

void LogQSO::initLogQSO(QString const& hisCall, QString const& hisGrid, QString mode,
                        QString const& rptSent, QString const& rptRcvd,
                        QDateTime const& dateTimeOn, QDateTime const& dateTimeOff,
                        Radio::Frequency dialFreq, bool noSuffix, QString xSent, QString xRcvd,
                        CabrilloLog * cabrillo_log, bool forceAccept)    //avt 10/23/19
{
  if(!isHidden()) return;
  ui->call->setText(hisCall);
  ui->grid->setText(hisGrid);
  ui->name->setText("");
  ui->txPower->setText("");
  ui->comments->setText("");
  if (ui->cbTxPower->isChecked ()) ui->txPower->setText(m_txPower);
  if (ui->cbComments->isChecked ()) ui->comments->setText(m_comments);
  if (m_config->report_in_comments()) {
    auto t=mode;
    if(rptSent!="") t+="  Sent: " + rptSent;
    if(rptRcvd!="") t+="  Rcvd: " + rptRcvd;
    ui->comments->setText(t);
  }
  if(noSuffix and mode.mid(0,3)=="JT9") mode="JT9";
  if(m_config->log_as_RTTY() and mode.mid(0,3)=="JT9") mode="RTTY";
  ui->mode->setText(mode);
  ui->sent->setText(rptSent);
  ui->rcvd->setText(rptRcvd);
  ui->start_date_time->setDateTime (dateTimeOn);
  ui->end_date_time->setDateTime (dateTimeOff);
  m_dialFreq=dialFreq;
  m_myCall=m_config->my_callsign();
  m_myGrid=m_config->my_grid();
  ui->band->setText (m_config->bands ()->find (dialFreq));
  ui->loggedOperator->setText(m_config->opCall());
  ui->exchSent->setText (xSent);
  ui->exchRcvd->setText (xRcvd);
  m_cabrilloLog = cabrillo_log;

  using SpOp = Configuration::SpecialOperatingActivity;
  auto special_op = m_config->special_op_id ();
  if (forceAccept                           //avt 10/23/19
      || (m_config->autoLog ()
          && special_op > SpOp::NONE))
    {
      // allow auto logging in Fox mode and contests
      accept();
    }
  else
    {
      show();
    }
}

void LogQSO::accept()
{
  auto hisCall = ui->call->text ();
  auto hisGrid = ui->grid->text ();
  auto mode = ui->mode->text ();
  auto rptSent = ui->sent->text ();
  auto rptRcvd = ui->rcvd->text ();
  auto m_dateTimeOn = ui->start_date_time->dateTime ();
  auto m_dateTimeOff = ui->end_date_time->dateTime ();
  auto band = ui->band->text ();
  auto name = ui->name->text ();
  auto m_txPower = ui->txPower->text ();
  auto m_comments = ui->comments->text ();
  auto strDialFreq = QString::number (m_dialFreq / 1.e6,'f',6);
  auto operator_call = ui->loggedOperator->text ();
  auto xsent = ui->exchSent->text ();
  auto xrcvd = ui->exchRcvd->text ();

  // validate
  using SpOp = Configuration::SpecialOperatingActivity;
  auto special_op = m_config->special_op_id ();
  if (special_op > SpOp::NONE)
    {
      if (xsent.isEmpty () || xrcvd.isEmpty ())
        {
          if (special_op == SpOp::POTA)
            {
              xrcvd = xsent = "+00";
              if (rptRcvd.size()) xrcvd = rptRcvd;
              if (rptSent.size()) xsent = rptSent;
            }
            else
            {
              show ();
              MessageBox::warning_message (this, tr ("Invalid QSO Data"),
                                       tr ("Check exchange sent and received"));
              return;               // without accepting
            }
        }

      if (!m_cabrilloLog->add_QSO (m_dialFreq, m_dateTimeOff, hisCall, xsent, xrcvd))
        {
          show ();
          MessageBox::warning_message (this, tr ("Invalid QSO Data"),
                                       tr ("Check all fields"));
          return;               // without accepting
        }
    }

  //Log this QSO to file "wsjtx.log"
  static QFile f {QDir {QStandardPaths::writableLocation (QStandardPaths::DataLocation)}.absoluteFilePath ("wsjtx.log")};
  if(!f.open(QIODevice::Text | QIODevice::Append)) {
    MessageBox::warning_message (this, tr ("Log file error"),
                                 tr ("Cannot open \"%1\" for append").arg (f.fileName ()),
                                 tr ("Error: %1").arg (f.errorString ()));
  } else {
    QString logEntry=m_dateTimeOn.date().toString("yyyy-MM-dd,") +
      m_dateTimeOn.time().toString("hh:mm:ss,") +
      m_dateTimeOff.date().toString("yyyy-MM-dd,") +
      m_dateTimeOff.time().toString("hh:mm:ss,") + hisCall + "," +
      hisGrid + "," + strDialFreq + "," + mode +
      "," + rptSent + "," + rptRcvd + "," + m_txPower +
      "," + m_comments + "," + name;
    QTextStream out(&f);
    out << logEntry << endl;
    f.close();
  }

  //Clean up and finish logging
  Q_EMIT acceptQSO (m_dateTimeOff
                    , hisCall
                    , hisGrid
                    , m_dialFreq
                    , mode
                    , rptSent
                    , rptRcvd
                    , m_txPower
                    , m_comments
                    , name
                    , m_dateTimeOn
                    , operator_call
                    , m_myCall
                    , m_myGrid
                    , xsent
                    , xrcvd
                    , LogBook::QSOToADIF (hisCall
                                          , hisGrid
                                          , mode
                                          , rptSent
                                          , rptRcvd
                                          , m_dateTimeOn
                                          , m_dateTimeOff
                                          , band
                                          , m_comments
                                          , name
                                          , strDialFreq
                                          , m_myCall
                                          , m_myGrid
                                          , m_txPower
                                          , operator_call
                                          , xsent
                                          , xrcvd));
  QDialog::accept();
}

//avt 5/2/25
//bool LogQSO::dupe(QString const& hisCall, Radio::Frequency dialFreq)
//{
// return m_cabrilloLog->dupe(dialFreq, hisCall);
//}

// closeEvent is only called from the system menu close widget for a
// modeless dialog so we use the hideEvent override to store the
// window settings
void LogQSO::hideEvent (QHideEvent * e)
{
  storeSettings ();
  QDialog::hideEvent (e);
}

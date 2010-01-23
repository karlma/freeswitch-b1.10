#include <QSettings>
#include <QtGui>
#include "accountdialog.h"
#include "ui_accountdialog.h"
#include "fshost.h"

AccountDialog::AccountDialog(QString accId, QWidget *parent) :
    QDialog(parent),
    _accId(accId),
    ui(new Ui::AccountDialog)
{
    ui->setupUi(this);
    _settings = new QSettings;
    connect(this, SIGNAL(accepted()), this, SLOT(writeConfig()));
    connect(ui->sofiaExtraParamAddBtn, SIGNAL(clicked()), this, SLOT(addExtraParam()));
    connect(ui->sofiaExtraParamRemBtn, SIGNAL(clicked()), this, SLOT(remExtraParam()));

    ui->sofiaExtraParamTable->horizontalHeader()->setStretchLastSection(true);
}

AccountDialog::~AccountDialog()
{
    delete ui;
}

void AccountDialog::remExtraParam()
{
    QList<QTableWidgetSelectionRange> sel = ui->sofiaExtraParamTable->selectedRanges();

    foreach(QTableWidgetSelectionRange range, sel)
    {
        int offset =0;
        for(int row = range.topRow(); row<=range.bottomRow(); row++)
        {
            ui->sofiaExtraParamTable->removeRow(row-offset);
            offset++;
        }
    }
}

void AccountDialog::addExtraParam()
{
    bool ok;
    QString paramName = QInputDialog::getText(this, tr("Add parameter."),
                                         tr("New parameter name:"), QLineEdit::Normal,
                                         NULL, &ok);
    if (!ok)
        return;
    QString paramVal = QInputDialog::getText(this, tr("Add parameter."),
                                         tr("New parameter value:"), QLineEdit::Normal,
                                         NULL, &ok);
    if (!ok)
        return;

    QTableWidgetItem* paramNameItem = new QTableWidgetItem(paramName);
    QTableWidgetItem* paramValItem = new QTableWidgetItem(paramVal);
    ui->sofiaExtraParamTable->setRowCount(ui->sofiaExtraParamTable->rowCount()+1);
    ui->sofiaExtraParamTable->setItem(ui->sofiaExtraParamTable->rowCount()-1,0,paramNameItem);
    ui->sofiaExtraParamTable->setItem(ui->sofiaExtraParamTable->rowCount()-1,1,paramValItem);
    ui->sofiaExtraParamTable->resizeColumnsToContents();
    ui->sofiaExtraParamTable->resizeRowsToContents();
    ui->sofiaExtraParamTable->horizontalHeader()->setStretchLastSection(true);
}

void AccountDialog::readConfig()
{
    _settings->beginGroup("FreeSWITCH/conf/sofia.conf/profiles/profile/gateways");
    _settings->beginGroup(_accId);

    _settings->beginGroup("gateway/attrs");
    ui->sofiaGwNameEdit->setText(_settings->value("name").toString());
    _settings->endGroup();

    _settings->beginGroup("gateway/params");
    ui->sofiaGwUsernameEdit->setText(_settings->value("username").toString());
    ui->sofiaGwRealmEdit->setText(_settings->value("realm").toString());
    ui->sofiaGwPasswordEdit->setText(_settings->value("password").toString());
    ui->sofiaGwExtensionEdit->setText(_settings->value("extension").toString());
    ui->sofiaGwExpireSecondsSpin->setValue(_settings->value("expire-seconds").toInt());
    ui->sofiaGwRegisterCombo->setCurrentIndex(ui->sofiaGwRegisterCombo->findText(_settings->value("register").toString(),
                                                                                 Qt::MatchExactly));
    ui->sofiaGwRegisterTransportCombo->setCurrentIndex(ui->sofiaGwRegisterTransportCombo->findText(_settings->value("register-transport").toString(),
                                                                                                   Qt::MatchExactly));
    ui->sofiaGwRetrySecondsSpin->setValue(_settings->value("retry-seconds").toInt());
    _settings->endGroup();

    _settings->beginGroup("gateway/customParams");
    int row = 0;
    ui->sofiaExtraParamTable->clearContents();
    foreach(QString k, _settings->childKeys())
    {
        row++;
        ui->sofiaExtraParamTable->setRowCount(row);
        QTableWidgetItem *varName = new QTableWidgetItem(k);
        QTableWidgetItem *varVal = new QTableWidgetItem(_settings->value(k).toString());
        ui->sofiaExtraParamTable->setItem(row-1, 0,varName);
        ui->sofiaExtraParamTable->setItem(row-1, 1,varVal);
    }
    _settings->endGroup();

    _settings->endGroup();
    _settings->endGroup();

    ui->sofiaExtraParamTable->resizeColumnsToContents();
    ui->sofiaExtraParamTable->resizeRowsToContents();
    ui->sofiaExtraParamTable->horizontalHeader()->setStretchLastSection(true);
}

void AccountDialog::writeConfig()
{
    _settings->beginGroup("FreeSWITCH/conf/sofia.conf/profiles/profile/gateways");

    _settings->beginGroup(_accId);

    if (!g_FSHost.getAccountByUUID(_accId).isNull())
    {
        QString res;
        QString arg = QString("profile softphone killgw %1").arg(g_FSHost.getAccountByUUID(_accId).data()->getName());

        if (g_FSHost.sendCmd("sofia", arg.toAscii().data() , &res) != SWITCH_STATUS_SUCCESS)
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not killgw %s from profile softphone.\n",
                              g_FSHost.getAccountByUUID(_accId).data()->getName().toAscii().data());
        }
    }
    
    _settings->beginGroup("gateway/attrs");
    _settings->setValue("name", ui->sofiaGwNameEdit->text());
    _settings->endGroup();


    _settings->beginGroup("gateway/params");
    _settings->setValue("username", ui->sofiaGwUsernameEdit->text());
    _settings->setValue("realm", ui->sofiaGwRealmEdit->text());
    _settings->setValue("password", ui->sofiaGwPasswordEdit->text());
    _settings->setValue("extension", ui->sofiaGwExtensionEdit->text());
    _settings->setValue("expire-seconds", ui->sofiaGwExpireSecondsSpin->value());
    _settings->setValue("register", ui->sofiaGwRegisterCombo->currentText());
    _settings->setValue("register-transport", ui->sofiaGwRegisterTransportCombo->currentText());
    _settings->setValue("retry-seconds", ui->sofiaGwRetrySecondsSpin->value());    
    _settings->endGroup();

    _settings->beginGroup("gateway/customParams");
    for (int i = 0; i< ui->sofiaExtraParamTable->rowCount(); i++)
    {
        _settings->setValue(ui->sofiaExtraParamTable->item(i, 0)->text(),
                            ui->sofiaExtraParamTable->item(i, 1)->text());
    }
    _settings->endGroup();

    _settings->endGroup();

    _settings->endGroup();

    emit gwAdded(_accId);
}

void AccountDialog::clear()
{
    ui->sofiaExtraParamTable->clearContents();
    ui->sofiaExtraParamTable->setRowCount(0);

    ui->sofiaGwNameEdit->clear();
    ui->sofiaGwUsernameEdit->clear();
    ui->sofiaGwRealmEdit->clear();
    ui->sofiaGwPasswordEdit->clear();
    ui->sofiaGwExtensionEdit->clear();
    ui->sofiaGwExpireSecondsSpin->setValue(60);
    ui->sofiaGwRegisterCombo->setCurrentIndex(0);
    ui->sofiaGwRegisterTransportCombo->setCurrentIndex(0);
    ui->sofiaGwRetrySecondsSpin->setValue(30);
}

void AccountDialog::setAccId(QString accId)
{
    _accId = accId;
}

void AccountDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

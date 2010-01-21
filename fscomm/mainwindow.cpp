/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2009, Anthony Minessale II <anthm@freeswitch.org>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthm@freeswitch.org>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Joao Mesquita <jmesquita@freeswitch.org>
 *
 */

#include <QInputDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <switch_version.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    preferences(NULL)
{
    ui->setupUi(this);

    /* Setup the taskbar icon */
    sysTray = new QSystemTrayIcon(QIcon(":/images/taskbar_icon"), this);
    sysTray->setToolTip(tr("FSComm"));

    /* Connect DTMF buttons */
    dialpadMapper = new QSignalMapper(this);
    connect(ui->dtmf0Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf1Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf2Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf3Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf4Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf5Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf6Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf7Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf8Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmf9Btn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfABtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfBBtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfCBtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfDBtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfAstBtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    connect(ui->dtmfPoundBtn, SIGNAL(clicked()), dialpadMapper, SLOT(map()));
    dialpadMapper->setMapping(ui->dtmf0Btn, QString("0"));
    dialpadMapper->setMapping(ui->dtmf1Btn, QString("1"));
    dialpadMapper->setMapping(ui->dtmf2Btn, QString("2"));
    dialpadMapper->setMapping(ui->dtmf3Btn, QString("3"));
    dialpadMapper->setMapping(ui->dtmf4Btn, QString("4"));
    dialpadMapper->setMapping(ui->dtmf5Btn, QString("5"));
    dialpadMapper->setMapping(ui->dtmf6Btn, QString("6"));
    dialpadMapper->setMapping(ui->dtmf7Btn, QString("7"));
    dialpadMapper->setMapping(ui->dtmf8Btn, QString("8"));
    dialpadMapper->setMapping(ui->dtmf9Btn, QString("9"));
    dialpadMapper->setMapping(ui->dtmfABtn, QString("A"));
    dialpadMapper->setMapping(ui->dtmfBBtn, QString("B"));
    dialpadMapper->setMapping(ui->dtmfCBtn, QString("C"));
    dialpadMapper->setMapping(ui->dtmfDBtn, QString("D"));
    dialpadMapper->setMapping(ui->dtmfAstBtn, QString("*"));
    dialpadMapper->setMapping(ui->dtmfPoundBtn, QString("#"));
    connect(dialpadMapper, SIGNAL(mapped(QString)), this, SLOT(sendDTMF(QString)));

    /* Connect events related to FreeSWITCH */
    connect(&g_FSHost, SIGNAL(ready()),this, SLOT(fshostReady()));
    connect(&g_FSHost, SIGNAL(ringing(QSharedPointer<Call>)), this, SLOT(ringing(QSharedPointer<Call>)));
    connect(&g_FSHost, SIGNAL(answered(QSharedPointer<Call>)), this, SLOT(answered(QSharedPointer<Call>)));
    connect(&g_FSHost, SIGNAL(hungup(QSharedPointer<Call>)), this, SLOT(hungup(QSharedPointer<Call>)));
    connect(&g_FSHost, SIGNAL(newOutgoingCall(QSharedPointer<Call>)), this, SLOT(newOutgoingCall(QSharedPointer<Call>)));
    connect(&g_FSHost, SIGNAL(callFailed(QSharedPointer<Call>)), this, SLOT(callFailed(QSharedPointer<Call>)));
    connect(&g_FSHost, SIGNAL(accountStateChange(QSharedPointer<Account>)), this, SLOT(accountStateChanged(QSharedPointer<Account>)));
    connect(&g_FSHost, SIGNAL(newAccount(QSharedPointer<Account>)), this, SLOT(accountAdd(QSharedPointer<Account>)));
    connect(&g_FSHost, SIGNAL(delAccount(QSharedPointer<Account>)), this, SLOT(accountDel(QSharedPointer<Account>)));
    /*connect(&g_FSHost, SIGNAL(coreLoadingError(QString)), this, SLOT(coreLoadingError(QString)));*/

    /* Connect call commands */
    connect(ui->newCallBtn, SIGNAL(clicked()), this, SLOT(makeCall()));
    connect(ui->answerBtn, SIGNAL(clicked()), this, SLOT(paAnswer()));
    connect(ui->hangupBtn, SIGNAL(clicked()), this, SLOT(paHangup()));
    connect(ui->recoredCallBtn, SIGNAL(toggled(bool)), SLOT(recordCall(bool)));
    connect(ui->tableCalls, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(callTableDoubleClick(QTableWidgetItem*)));
    connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(prefTriggered()));
    connect(ui->action_Exit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(ui->actionSetDefaultAccount, SIGNAL(triggered(bool)), this, SLOT(setDefaultAccount()));
    connect(sysTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(sysTrayActivated(QSystemTrayIcon::ActivationReason)));

    /* Set the context menus */
    ui->tableAccounts->addAction(ui->actionSetDefaultAccount);

    /* Set other properties */
    ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);

}

MainWindow::~MainWindow()
{
    delete ui;
    QString res;
    g_FSHost.sendCmd("fsctl", "shutdown", &res);
    g_FSHost.wait();
}

void MainWindow::setDefaultAccount()
{
    QString accName = ui->tableAccounts->item(ui->tableAccounts->selectedRanges()[0].topRow(), 0)->text();

    if (accName.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("FreeSWITCH/conf/globals");
    switch_core_set_variable("default_gateway", accName.toAscii().data());
    settings.setValue("default_gateway", accName);
    settings.endGroup();
}

void MainWindow::prefTriggered()
{
    if (!preferences)
        preferences = new PrefDialog();

    preferences->raise();
    preferences->show();
    preferences->activateWindow();
}

void MainWindow::coreLoadingError(QString err)
{
    QMessageBox::warning(this, "Error Loading Core...", err, QMessageBox::Ok);
    QApplication::exit(255);
}

void MainWindow::accountAdd(QSharedPointer<Account> acc)
{
    ui->tableAccounts->setRowCount(ui->tableAccounts->rowCount()+1);
    QTableWidgetItem *gwField = new QTableWidgetItem(acc.data()->getName());
    QTableWidgetItem *stField = new QTableWidgetItem(acc.data()->getStateName());
    ui->tableAccounts->setItem(ui->tableAccounts->rowCount()-1,0,gwField);
    ui->tableAccounts->setItem(ui->tableAccounts->rowCount()-1,1,stField);
    ui->tableAccounts->resizeColumnsToContents();
    ui->tableAccounts->resizeRowsToContents();
    ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::accountDel(QSharedPointer<Account> acc)
{
    foreach (QTableWidgetItem *i, ui->tableAccounts->findItems(acc.data()->getName(), Qt::MatchExactly))
    {
        if (i->text() == acc.data()->getName())
        {
            ui->tableAccounts->removeRow(i->row());
            ui->tableAccounts->setRowCount(ui->tableAccounts->rowCount()-1);
            ui->tableAccounts->resizeColumnsToContents();
            ui->tableAccounts->resizeRowsToContents();
            ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);
            return;
        }
    }
}

void MainWindow::accountStateChanged(QSharedPointer<Account> acc)
{
    ui->statusBar->showMessage(tr("Account %1 is %2").arg(acc.data()->getName(), acc.data()->getStateName()));
    foreach (QTableWidgetItem *i, ui->tableAccounts->findItems(acc.data()->getName(), Qt::MatchExactly))
    {
        if (i->text() == acc.data()->getName())
        {
            ui->tableAccounts->item(i->row(), 1)->setText(acc.data()->getStateName());
            ui->tableAccounts->resizeColumnsToContents();
            ui->tableAccounts->resizeRowsToContents();
            ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);
            return;
        }
    }
}

void MainWindow::sendDTMF(QString dtmf)
{
    g_FSHost.getCurrentActiveCall().data()->sendDTMF(dtmf);
}

/* TODO: Update the timers and the item text! */
void MainWindow::callTableDoubleClick(QTableWidgetItem *item)
{
    QSharedPointer<Call> lastCall = g_FSHost.getCurrentActiveCall();
    QSharedPointer<Call> call = g_FSHost.getCallByUUID(item->data(Qt::UserRole).toString());
    QString switch_str = QString("switch %1").arg(call.data()->getCallID());
    QString result;
    if (g_FSHost.sendCmd("pa", switch_str.toAscii(), &result) == SWITCH_STATUS_FALSE) {
        ui->textEdit->setText(QString("Error switching to call %1").arg(call.data()->getCallID()));
        return;
    }
    ui->hangupBtn->setEnabled(true);
    /* Last call was hungup and we are switching */
    if (!lastCall.isNull())
        lastCall.data()->setActive(false);
    call.data()->setActive(true);
}

void MainWindow::makeCall()
{
    bool ok;
    QString dialstring = QInputDialog::getText(this, tr("Make new call"),
                                         tr("Number to dial:"), QLineEdit::Normal, NULL,&ok);

    if (ok && !dialstring.isEmpty())
    {
        paCall(dialstring);
    }
}

void MainWindow::fshostReady()
{
    ui->statusBar->showMessage("Ready");
    ui->newCallBtn->setEnabled(true);
    ui->textEdit->setEnabled(true);
    ui->textEdit->setText("Ready to dial and receive calls!");
    sysTray->show();
    sysTray->showMessage(tr("Status"), tr("FSComm has initialized!"), QSystemTrayIcon::Information, 5000);
}

void MainWindow::paAnswer()
{
    QString result;
    if (g_FSHost.sendCmd("pa", "answer", &result) == SWITCH_STATUS_FALSE) {
        ui->textEdit->setText("Error sending that command");
    }

    ui->textEdit->setText("Talking...");
    ui->hangupBtn->setEnabled(true);
    ui->answerBtn->setEnabled(false);
}

void MainWindow::paCall(QString dialstring)
{
    QString result;

    QString callstring = QString("call %1").arg(dialstring);

    if (g_FSHost.sendCmd("pa", callstring.toAscii(), &result) == SWITCH_STATUS_FALSE) {
        ui->textEdit->setText("Error sending that command");
    }

    ui->hangupBtn->setEnabled(true);
}

void MainWindow::paHangup()
{
    QString result;
    if (g_FSHost.sendCmd("pa", "hangup", &result) == SWITCH_STATUS_FALSE) {
        ui->textEdit->setText("Error sending that command");
    }

    ui->textEdit->setText("Click to dial number...");
    ui->statusBar->showMessage("Call hungup");
    ui->hangupBtn->setEnabled(false);
}

void MainWindow::recordCall(bool pressed)
{
    QSharedPointer<Call> call = g_FSHost.getCurrentActiveCall();

    if (call.isNull())
    {
        QMessageBox::warning(this,tr("Record call"),
                             tr("<p>FSComm reports that there are no active calls to be recorded."
                                "<p>Please report this bug."),
                             QMessageBox::Ok);
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not record call because there is not current active call!.\n");
        return;
    }

    if (call.data()->toggleRecord(pressed) != SWITCH_STATUS_SUCCESS)
    {
        QMessageBox::warning(this,tr("Record call"),
                             tr("<p>Could not get active call to start/stop recording."
                                "<p>Please report this bug."),
                             QMessageBox::Ok);
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not record call [%s].\n", call.data()->getUUID().toAscii().data());
        return;
    }
}

void MainWindow::newOutgoingCall(QSharedPointer<Call> call)
{
    ui->textEdit->setText(QString("Calling %1 (%2)").arg(call.data()->getCidName(), call.data()->getCidNumber()));

    ui->tableCalls->setRowCount(ui->tableCalls->rowCount()+1);
    QTableWidgetItem *item0 = new QTableWidgetItem(QString("%1 (%2)").arg(call.data()->getCidName(), call.data()->getCidNumber()));
    item0->setData(Qt::UserRole, call.data()->getUUID());
    ui->tableCalls->setItem(ui->tableCalls->rowCount()-1,0,item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(tr("Dialing..."));
    item1->setData(Qt::UserRole, call.data()->getUUID());
    ui->tableCalls->setItem(ui->tableCalls->rowCount()-1,1,item1);

    ui->tableCalls->resizeColumnsToContents();
    ui->tableCalls->resizeRowsToContents();
    ui->tableCalls->horizontalHeader()->setStretchLastSection(true);

    ui->hangupBtn->setEnabled(true);
    call.data()->setActive(true);
}

void MainWindow::ringing(QSharedPointer<Call> call)
{

    for (int i=0; i<ui->tableCalls->rowCount(); i++)
    {
        QTableWidgetItem *item = ui->tableCalls->item(i, 1);
        if (item->data(Qt::UserRole).toString() == call.data()->getUUID())
        {
            item->setText(tr("Ringing"));
            ui->textEdit->setText(QString("Call from %1 (%2)").arg(call.data()->getCidName(), call.data()->getCidNumber()));
            return;
        }
    }

    ui->textEdit->setText(QString("Call from %1 (%2)").arg(call.data()->getCidName(), call.data()->getCidNumber()));

    ui->tableCalls->setRowCount(ui->tableCalls->rowCount()+1);
    QTableWidgetItem *item0 = new QTableWidgetItem(QString("%1 (%2)").arg(call.data()->getCidName(), call.data()->getCidNumber()));
    item0->setData(Qt::UserRole, call.data()->getUUID());
    ui->tableCalls->setItem(ui->tableCalls->rowCount()-1,0,item0);

    QTableWidgetItem *item1 = new QTableWidgetItem(tr("Ringing"));
    item1->setData(Qt::UserRole, call.data()->getUUID());
    ui->tableCalls->setItem(ui->tableCalls->rowCount()-1,1,item1);

    ui->tableCalls->resizeColumnsToContents();
    ui->tableCalls->resizeRowsToContents();
    ui->tableCalls->horizontalHeader()->setStretchLastSection(true);

    ui->answerBtn->setEnabled(true);
    call.data()->setActive(true);
}

void MainWindow::answered(QSharedPointer<Call> call)
{
    for (int i=0; i<ui->tableCalls->rowCount(); i++)
    {
        QTableWidgetItem *item = ui->tableCalls->item(i, 1);
        if (item->data(Qt::UserRole).toString() == call.data()->getUUID())
        {
            item->setText(tr("Answered"));
            ui->tableCalls->resizeColumnsToContents();
            ui->tableCalls->resizeRowsToContents();
            ui->tableCalls->horizontalHeader()->setStretchLastSection(true);
            break;
        }
    }
    ui->recoredCallBtn->setEnabled(true);
    ui->recoredCallBtn->setChecked(false);
    ui->dtmf0Btn->setEnabled(true);
    ui->dtmf1Btn->setEnabled(true);
    ui->dtmf2Btn->setEnabled(true);
    ui->dtmf3Btn->setEnabled(true);
    ui->dtmf4Btn->setEnabled(true);
    ui->dtmf5Btn->setEnabled(true);
    ui->dtmf6Btn->setEnabled(true);
    ui->dtmf7Btn->setEnabled(true);
    ui->dtmf8Btn->setEnabled(true);
    ui->dtmf9Btn->setEnabled(true);
    ui->dtmfABtn->setEnabled(true);
    ui->dtmfBBtn->setEnabled(true);
    ui->dtmfCBtn->setEnabled(true);
    ui->dtmfDBtn->setEnabled(true);
    ui->dtmfAstBtn->setEnabled(true);
    ui->dtmfPoundBtn->setEnabled(true);
}

void MainWindow::callFailed(QSharedPointer<Call> call)
{
    for (int i=0; i<ui->tableCalls->rowCount(); i++)
    {
        QTableWidgetItem *item = ui->tableCalls->item(i, 1);
        if (item->data(Qt::UserRole).toString() == call.data()->getUUID())
        {
            ui->tableCalls->removeRow(i);
            ui->tableCalls->resizeColumnsToContents();
            ui->tableCalls->resizeRowsToContents();
            ui->tableCalls->horizontalHeader()->setStretchLastSection(true);
            break;
        }
    }
    ui->textEdit->setText(tr("Call with %1 (%2) failed with reason %3.").arg(call.data()->getCidName(),
                                                                             call.data()->getCidNumber(),
                                                                             call.data()->getCause()));
    call.data()->setActive(false);
    /* TODO: Will cause problems if 2 calls are received at the same time */
    ui->recoredCallBtn->setEnabled(false);
    ui->recoredCallBtn->setChecked(false);
    ui->answerBtn->setEnabled(false);
    ui->hangupBtn->setEnabled(false);
    ui->dtmf0Btn->setEnabled(false);
    ui->dtmf1Btn->setEnabled(false);
    ui->dtmf2Btn->setEnabled(false);
    ui->dtmf3Btn->setEnabled(false);
    ui->dtmf4Btn->setEnabled(false);
    ui->dtmf5Btn->setEnabled(false);
    ui->dtmf6Btn->setEnabled(false);
    ui->dtmf7Btn->setEnabled(false);
    ui->dtmf8Btn->setEnabled(false);
    ui->dtmf9Btn->setEnabled(false);
    ui->dtmfABtn->setEnabled(false);
    ui->dtmfBBtn->setEnabled(false);
    ui->dtmfCBtn->setEnabled(false);
    ui->dtmfDBtn->setEnabled(false);
    ui->dtmfAstBtn->setEnabled(false);
    ui->dtmfPoundBtn->setEnabled(false);

}

void MainWindow::hungup(QSharedPointer<Call> call)
{
    for (int i=0; i<ui->tableCalls->rowCount(); i++)
    {
        QTableWidgetItem *item = ui->tableCalls->item(i, 1);
        if (item->data(Qt::UserRole).toString() == call.data()->getUUID())
        {
            ui->tableCalls->removeRow(i);
            ui->tableCalls->resizeColumnsToContents();
            ui->tableCalls->resizeRowsToContents();
            ui->tableCalls->horizontalHeader()->setStretchLastSection(true);
            break;
        }
    }
    call.data()->setActive(false);
    ui->textEdit->setText(tr("Call with %1 (%2) hungup.").arg(call.data()->getCidName(), call.data()->getCidNumber()));
    /* TODO: Will cause problems if 2 calls are received at the same time */
    ui->recoredCallBtn->setEnabled(false);
    ui->recoredCallBtn->setChecked(false);
    ui->answerBtn->setEnabled(false);
    ui->hangupBtn->setEnabled(false);
    ui->dtmf0Btn->setEnabled(false);
    ui->dtmf1Btn->setEnabled(false);
    ui->dtmf2Btn->setEnabled(false);
    ui->dtmf3Btn->setEnabled(false);
    ui->dtmf4Btn->setEnabled(false);
    ui->dtmf5Btn->setEnabled(false);
    ui->dtmf6Btn->setEnabled(false);
    ui->dtmf7Btn->setEnabled(false);
    ui->dtmf8Btn->setEnabled(false);
    ui->dtmf9Btn->setEnabled(false);
    ui->dtmfABtn->setEnabled(false);
    ui->dtmfBBtn->setEnabled(false);
    ui->dtmfCBtn->setEnabled(false);
    ui->dtmfDBtn->setEnabled(false);
    ui->dtmfAstBtn->setEnabled(false);
    ui->dtmfPoundBtn->setEnabled(false);
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::showAbout()
{
    QString result;
    g_FSHost.sendCmd("version", "", &result);

    QMessageBox::about(this, tr("About FSComm"),
                       tr("<h2>FSComm</h2>"
                          "<p>Author: Jo&atilde;o Mesquita &lt;jmesquita@freeswitch.org>"
                          "<p>FsComm is a softphone based on libfreeswitch."
                          "<p>The FreeSWITCH&trade; images and name are trademark of"
                          " Anthony Minessale II, primary author of FreeSWITCH&trade;."
                          "<p>Compiled FSComm version: %1"
                          "<p>%2").arg(SWITCH_VERSION_FULL, result));
}

void MainWindow::sysTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
        {
            if (this->isVisible())
                this->hide();
            else {
                this->show();
                this->activateWindow();
                this->raise();
            }
            break;
        }
    default:
        break;
    }
}

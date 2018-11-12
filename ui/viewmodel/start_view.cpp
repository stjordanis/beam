// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "start_view.h"
#include "wallet/keystore.h"
#include <QMessageBox>
#include <QStringBuilder>
#include <QApplication>
#include <QClipboard>
#include <QVariant>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QPaintEngine>
#include <QPrinterInfo>
#endif
#include "settings_view.h"
#include "model/app_model.h"
#include "wallet/secstring.h"
#include <thread>

using namespace beam;
using namespace ECC;
using namespace std;

namespace
{
    const char* Testnet[] =
    {
#ifdef BEAM_TESTNET
        "139.59.174.80:8100",
        "167.99.221.104:8100",
        "159.65.201.244:8100",
        "142.93.217.246:8100",
        "142.93.221.22:8100",
        "178.62.59.65:8100",
        "139.59.169.77:8100",
        "167.99.166.33:8100",
        "159.65.119.216:8100",
        "142.93.221.66:8100",
        "206.189.10.149:8100",
        "138.68.134.136:8100",
        "159.65.119.226:8100",
        "142.93.221.80:8100",
        "104.248.159.97:8100",
        "159.65.101.94:8100",
        "68.183.103.130:8100",
        "142.93.213.21:8100",
        "104.248.159.154:8100",
        "188.166.165.240:8100",
        "159.65.104.75:8100",
        "142.93.213.106:8100"
 #else
        "172.104.249.212:8101",
        "23.239.23.209:8201",
        "172.105.211.232:8301",
        "96.126.111.61:8401",
        "176.58.98.195:8501"
#endif
    };

    QString chooseRandomNode()
    {
        srand(time(0));
        return QString(Testnet[rand() % (sizeof(Testnet) / sizeof(Testnet[0]))]);
    }

    const QChar PHRASES_SEPARATOR = ';';
}

RecoveryPhraseItem::RecoveryPhraseItem(int index, const QString& phrase)
    : m_index(index)
    , m_phrase(phrase)
{

}

RecoveryPhraseItem::~RecoveryPhraseItem()
{

}

bool RecoveryPhraseItem::isCorrect() const
{
    return m_userInput == m_phrase;
}

const QString& RecoveryPhraseItem::getValue() const
{
    return m_userInput;
}

void RecoveryPhraseItem::setValue(const QString& value)
{
    if (m_userInput != value)
    {
        m_userInput = value;
        emit valueChanged();
        emit isCorrectChanged();
    }
}

const QString& RecoveryPhraseItem::getPhrase() const
{
    return m_phrase;
}

int RecoveryPhraseItem::getIndex() const
{
    return m_index;
}

StartViewModel::StartViewModel()
    : m_isRecoveryMode{false}
{

}

StartViewModel::~StartViewModel()
{

}

bool StartViewModel::walletExists() const
{
    return WalletDB::isInitialized(AppModel::getInstance()->getSettings().getWalletStorage());
}

bool StartViewModel::getIsRecoveryMode() const
{
    return m_isRecoveryMode;
}

void StartViewModel::setIsRecoveryMode(bool value)
{
    if (value != m_isRecoveryMode)
    {
        m_isRecoveryMode = value;
        m_recoveryPhrases.clear();
        AppModel::getInstance()->setRestoreWallet(value);
        emit isRecoveryModeChanged();
    }
}

const QList<QObject*>& StartViewModel::getRecoveryPhrases()
{
    if (m_recoveryPhrases.empty())
    {
        if (!m_isRecoveryMode)
        {
            m_generatedPhrases = beam::createMnemonic(beam::getEntropy(), beam::language::en);
        }
        else
        {
            m_generatedPhrases.resize(12);
        }
        assert(m_generatedPhrases.size() == 12);
        m_recoveryPhrases.reserve(static_cast<int>(m_generatedPhrases.size()));
        int i = 0;
        for (const auto& p : m_generatedPhrases)
        {
            m_recoveryPhrases.push_back(new RecoveryPhraseItem(i++, QString::fromStdString(p)));
        }
    }
    return m_recoveryPhrases;
}

const QList<QObject*>& StartViewModel::getCheckPhrases()
{
    if (m_checkPhrases.empty())
    {
        srand(time(0));
        set<int> indecies;
        while (indecies.size() < 6)
        {
            int index = rand() % m_recoveryPhrases.size();
            auto it = indecies.insert(index);
            if (it.second)
            {
                m_checkPhrases.push_back(new RecoveryPhraseItem(index, QString::fromStdString(m_generatedPhrases[index])));
            }
        }
    }

    return m_checkPhrases;
}

QChar StartViewModel::getPhrasesSeparator()
{
    return PHRASES_SEPARATOR;
}

void StartViewModel::setupLocalNode(int port, int miningThreads, bool generateGenesys)
{
    auto& settings = AppModel::getInstance()->getSettings();
    settings.setLocalNodeMiningThreads(miningThreads);
    auto localAddress = QString::asprintf("127.0.0.1:%d", port);
    settings.setNodeAddress(localAddress);
    settings.setLocalNodePort(port);
    settings.setRunLocalNode(true);
    settings.setGenerateGenesys(generateGenesys);
    QStringList peers;
    peers.push_back(chooseRandomNode());
    settings.setLocalNodePeers(peers);
}

void StartViewModel::setupRemoteNode(const QString& nodeAddress)
{
    AppModel::getInstance()->getSettings().setRunLocalNode(false);
    AppModel::getInstance()->getSettings().setNodeAddress(nodeAddress);
}

void StartViewModel::setupTestnetNode()
{
    AppModel::getInstance()->getSettings().setRunLocalNode(false);
    AppModel::getInstance()->getSettings().setNodeAddress(chooseRandomNode());
}

uint StartViewModel::coreAmount() const
{
    return std::thread::hardware_concurrency();
}

void StartViewModel::copyPhrasesToClipboard()
{
    QString phrases;
    for (const auto& p : m_generatedPhrases)
    {
        phrases = phrases % p.c_str() % PHRASES_SEPARATOR;
    }
    QApplication::clipboard()->setText(phrases);
}

void StartViewModel::printRecoveryPhrases(QVariant viewData )
{
    try
    {
        if (QPrinterInfo::availablePrinters().isEmpty())
        {
            AppModel::getInstance()->getMessages().addMessage(tr("Printer is not found. Please, check your printer preferences."));
            return;
        }
        QImage image = qvariant_cast<QImage>(viewData);
        QPrinter printer;
        printer.setOutputFormat(QPrinter::NativeFormat);
        printer.setColorMode(QPrinter::GrayScale);
        QPrintDialog dialog(&printer);
        if (dialog.exec() == QDialog::Accepted) {

            QPainter painter(&printer);
            
            QRect rect = painter.viewport();
            QFont f;
            f.setPixelSize(16);
            painter.setFont(f);
            int x = 60, y = 30;

            const int n = 4;
            int s = rect.width() / n;

            for (int i = 0; i < m_recoveryPhrases.size(); ++i)
            {
                if (i % n == 0)
                {
                    x = 60;
                    y += 30;
                }
                else
                {
                    x += s;
                }
                QString t = QString::number(i + 1) % " - " % m_generatedPhrases[i].c_str();
                painter.drawText(x, y, t);
                
            }
           
            //QRect rect = painter.viewport();
            //QSize size = image.size();
            //size.scale(rect.size(), Qt::KeepAspectRatio);
            //painter.setViewport(rect.x(), rect.y() + 60, size.width(), size.height());
            //painter.setWindow(image.rect());
            //painter.drawImage(0, 0, image);
            painter.end();
        }
    }
    catch (...)
    {
        AppModel::getInstance()->getMessages().addMessage(tr("Failed to print recovery phrases. Please, check your printer."));
    }
}

void StartViewModel::resetPhrases()
{
    m_recoveryPhrases.clear();
    m_generatedPhrases.clear();
    m_checkPhrases.clear();
    emit recoveryPhrasesChanged();
}

bool StartViewModel::createWallet(const QString& pass)
{
    if (m_isRecoveryMode)
    {
        assert(m_generatedPhrases.size() == static_cast<size_t>(m_recoveryPhrases.size()));
        for (int i = 0; i < m_recoveryPhrases.size(); ++i)
        {
            QString s = static_cast<RecoveryPhraseItem*>(m_recoveryPhrases[i])->getValue();
            m_generatedPhrases[i] = s.toStdString();
        }
    }
    auto buf = beam::decodeMnemonic(m_generatedPhrases);

    SecString secretSeed;
    secretSeed.assign(buf.data(), buf.size());
    SecString sectretPass = pass.toStdString();
    return AppModel::getInstance()->createWallet(secretSeed, sectretPass);
}

bool StartViewModel::openWallet(const QString& pass)
{
    // TODO make this secure
    SecString secretPass = pass.toStdString();
    return AppModel::getInstance()->openWallet(secretPass);
}

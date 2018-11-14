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

#include "restore_view.h"
#include "model/app_model.h"

using namespace beam;
using namespace std;

RestoreViewModel::RestoreViewModel()
    : _progress{0.0}
    , _nodeTotal{0}
    , _nodeDone{0}
    , _total{0}
    , _done{0}
    , _walletConnected{false}
    , _hasLocalNode{ AppModel::getInstance()->getSettings().getRunLocalNode() }
    , _syncStart{getTimestamp()}
{
    connect(AppModel::getInstance()->getWallet().get(), SIGNAL(onSyncProgressUpdated(int, int)),
        SLOT(onSyncProgressUpdated(int, int)));

    //connect(&_model, SIGNAL(onRecoverProgressUpdated(int, int, const QString&)),
    //    SLOT(onRestoreProgressUpdated(int, int, const QString&)));

    if (AppModel::getInstance()->getSettings().getRunLocalNode())
    {
        connect(&AppModel::getInstance()->getNode(), SIGNAL(syncProgressUpdated(int, int)),
            SLOT(onNodeSyncProgressUpdated(int, int)));
    }
    if (!_hasLocalNode && _walletConnected == false)
    {
        syncWithNode();
    }
    connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimer()));
    _updateTimer.start(1000);
}

RestoreViewModel::~RestoreViewModel()
{
}


void RestoreViewModel::restoreFromBlockchain()
{
    WalletModel& wallet = *AppModel::getInstance()->getWallet();
    if (wallet.async)
    {
        wallet.async->restoreFromBlockchain();
    }
}

void RestoreViewModel::onRestoreProgressUpdated(int, int, const QString&)
{

}

void RestoreViewModel::onSyncProgressUpdated(int done, int total)
{
    _done = done;
    _total = total;
    updateProgress();
}

void RestoreViewModel::onNodeSyncProgressUpdated(int done, int total)
{
    _nodeDone = done;
    _nodeTotal = total;
    updateProgress();
}

void RestoreViewModel::updateProgress()
{
    double nodeSyncProgress = 0.0;
    QString progressMessage;
    if (_nodeTotal > 0)
    {
        int blocksDiff = _nodeTotal / 2;
        if (_nodeDone <= blocksDiff)
        {
            progressMessage = QString::asprintf(tr("Downloading block headers %d/%d").toStdString().c_str(), _nodeDone, blocksDiff);
        }
        else
        {
            progressMessage = QString::asprintf(tr("Downloading blocks %d/%d").toStdString().c_str(), _nodeDone - blocksDiff, blocksDiff);
        }
        nodeSyncProgress = static_cast<double>(_nodeDone) / _nodeTotal;
    }

    if (nodeSyncProgress >= 1.0 && _walletConnected == false)
    {
        WalletModel& wallet = *AppModel::getInstance()->getWallet();
        if (wallet.async)
        {
            _walletConnected = true;
            wallet.async->syncWithNode();
        }
    }

    double walletSyncProgress = 0.0;
    if (_total)
    {
        progressMessage = QString::asprintf(tr("Scanning UTXO %d/%d").toStdString().c_str(), _done, _total);
        walletSyncProgress = static_cast<double>(_done) / _total;
    }
    double p = 0.0;
    if (AppModel::getInstance()->getSettings().getRunLocalNode())
    {
        p = nodeSyncProgress * 0.7 + walletSyncProgress * 0.3;
    }
    else
    {
        p = walletSyncProgress;
    }

    if (p > 0)
    {
        progressMessage.append(tr(", estimated time:"));

        auto d = getTimestamp() - _syncStart;
        Timestamp estimateInSec = d * (1.0 / p - 1);

        int hours = estimateInSec / 3600;
        if (hours > 0)
        {
            progressMessage.append(QString::asprintf(tr(" %d h").toStdString().c_str(), hours));
        }
        int minutes = (estimateInSec - 3600 * hours) / 60;
        if (minutes > 0)
        {
            progressMessage.append(QString::asprintf(tr(" %d min").toStdString().c_str(), minutes));
        }
        int seconds = estimateInSec % 60;
        progressMessage.append(QString::asprintf(tr(" %d sec").toStdString().c_str(), seconds));
    }

    setProgressMessage(progressMessage);
    setProgress(p);
    if (p >= 1.0)
    {
        emit syncCompleted();
    }
}

double RestoreViewModel::getProgress() const
{
    return _progress;
}

void RestoreViewModel::setProgress(double value)
{
    if (value > _progress)
    {
        _progress = value;
        emit progressChanged();
    }
}

const QString& RestoreViewModel::getProgressMessage() const
{
    return _progressMessage;
}
void RestoreViewModel::setProgressMessage(const QString& value)
{
    if (_progressMessage != value)
    {
        _progressMessage = value;
        emit progressMessageChanged();
    }
}

void RestoreViewModel::syncWithNode()
{
    WalletModel& wallet = *AppModel::getInstance()->getWallet();
    if (wallet.async)
    {
        _walletConnected = true;
        wallet.async->syncWithNode();
    }
}

void RestoreViewModel::onUpdateTimer()
{
    updateProgress();
}
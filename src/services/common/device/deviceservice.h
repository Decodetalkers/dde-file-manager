﻿/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DEVICESERVICE_H
#define DEVICESERVICE_H

#include "dfm_common_service_global.h"

#include <dfm-framework/service/pluginservicecontext.h>

DSC_BEGIN_NAMESPACE

class DeviceMonitorHandler;
class DeviceService final : public dpf::PluginService, dpf::AutoServiceRegister<DeviceService>
{
    Q_OBJECT
    Q_DISABLE_COPY(DeviceService)
    friend class dpf::QtClassFactory<dpf::PluginService>;

public:
    static QString name()
    {
        return "org.deepin.service.DeviceService";
    }

public: // operations
    void startAutoMount();
    bool startMonitor();
    bool stopMonitor();
    bool stopDefenderScanDrive(const QString &deviceId);
    bool stopDefenderScanAllDrives();

    void detachBlockDevice(const QString &deviceId);
    void detachProtocolDevice(const QString &deviceId);
    void detachAllMountedBlockDevices();
    void detachAllMountedProtocolDevices();
    void mountBlockDeviceAsync(const QString &deviceId, const QVariantMap &opts = {});
    bool mountBlockDevice(const QString &deviceId, const QVariantMap &opts = {});
    void unmountBlockDeviceAsync(const QString &deviceId, const QVariantMap &opts = {});
    bool unmountBlockDevice(const QString &deviceId, const QVariantMap &opts = {});
    void ejectBlockDeviceAsync(const QString &deviceId, const QVariantMap &opts = {});
    bool ejectBlockDevice(const QString &deviceId, const QVariantMap &opts = {});
    void poweroffBlockDeviceAsync(const QString &deviceId, const QVariantMap &opts = {});
    bool poweroffBlockDevice(const QString &deviceId, const QVariantMap &opts = {});

public: // status
    bool isBlockDeviceMonitorWorking() const;
    bool isProtolDeviceMonitorWorking() const;
    bool isInLiveSystem() const;
    bool isAutoMountSetting() const;
    bool isAutoMountAndOpenSetting() const;
    bool isDefenderScanningDrive(const QString &driveName = "") const;
    QStringList blockDevicesIdList(const QVariantMap &opts) const;
    QVariantMap blockDeviceInfo(const QString &deviceId);
    QStringList protocolDevicesIdList() const;
    QVariantMap protocolDeviceInfo(const QString &deviceId);

signals:
    void blockDriveAdded();
    void blockDriveRemoved();
    void blockDeviceAdded(const QString &deviceId);
    void blockDeviceRemoved(const QString &deviceId);
    void blockDeviceFilesystemAdded(const QString &deviceId);
    void blockDeviceFilesystemRemoved(const QString &deviceId);
    void blockDeviceMounted(const QString &deviceId, const QString &mountPoint);
    void blockDeviceUnmounted(const QString &deviceId);

private:
    explicit DeviceService(QObject *parent = nullptr);
    virtual ~DeviceService() override;

private:
    QScopedPointer<DeviceMonitorHandler> monitorHandler;
};

DSC_END_NAMESPACE

#endif // DEVICESERVICE_H

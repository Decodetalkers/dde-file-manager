// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILEMENUREPORTDATA_H
#define FILEMENUREPORTDATA_H

#include "reportdatainterface.h"

#include <QObject>

class FileMenuReportData : public ReportDataInterface
{
public:
    QString type() const override;
    QJsonObject prepareData(const QVariantMap &args) const override;
};

#endif // FILEMENUREPORTDATA_H
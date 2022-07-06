/*
 * Copyright (C) 2021 ~ 2022 Uniontech Software Technology Co., Ltd.
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
#include "workspaceeventcaller.h"

#include "services/common/propertydialog/property_defines.h"
#include "services/filemanager/workspace/workspace_defines.h"

#include "dfm-base/dfm_event_defines.h"
#include "dfm-base/base/schemefactory.h"
#include "dfm_global_defines.h"

#include <dfm-framework/framework.h>

DFMGLOBAL_USE_NAMESPACE
DSB_FM_USE_NAMESPACE
DSC_USE_NAMESPACE
using namespace dfmplugin_workspace;
DFMBASE_USE_NAMESPACE

static constexpr char kEventNS[] { DPF_MACRO_TO_STR(DPWORKSPACE_NAMESPACE) };

void WorkspaceEventCaller::sendOpenWindow(const QList<QUrl> &urls)
{
    bool hooked = dpfHookSequence->run(kEventNS, "hook_SendOpenWindow", urls);
    if (hooked)
        return;

    if (urls.isEmpty()) {
        dpfSignalDispatcher->publish(GlobalEventType::kOpenNewWindow, QUrl());
    } else {
        for (const QUrl &url : urls)
            dpfSignalDispatcher->publish(GlobalEventType::kOpenNewWindow, url);
    }
}

void WorkspaceEventCaller::sendChangeCurrentUrl(const quint64 windowId, const QUrl &url)
{
    bool hooked = dpfHookSequence->run(kEventNS, "hook_SendChangeCurrentUrl", windowId, url);
    if (hooked)
        return;

    if (!url.isEmpty())
        dpfSignalDispatcher->publish(GlobalEventType::kChangeCurrentUrl, windowId, url);
}

void WorkspaceEventCaller::sendOpenAsAdmin(const QUrl &url)
{
    dpfSignalDispatcher->publish(GlobalEventType::kOpenAsAdmin, url);
}

void WorkspaceEventCaller::sendTabAdded(const quint64 windowID)
{
    dpfSignalDispatcher->publish(Workspace::EventType::kTabAdded, windowID);
}

void WorkspaceEventCaller::sendTabChanged(const quint64 windowID, const int index)
{
    dpfSignalDispatcher->publish(Workspace::EventType::kTabChanged, windowID, index);
}

void WorkspaceEventCaller::sendTabMoved(const quint64 windowID, const int from, const int to)
{
    dpfSignalDispatcher->publish(Workspace::EventType::kTabMoved, windowID, from, to);
}

void WorkspaceEventCaller::sendTabRemoved(const quint64 windowID, const int index)
{
    dpfSignalDispatcher->publish(Workspace::EventType::kTabRemoved, windowID, index);
}

void WorkspaceEventCaller::sendShowCustomTopWidget(const quint64 windowID, const QString &scheme, bool visible)
{
    dpfSignalDispatcher->publish(DSB_FM_NAMESPACE::Workspace::EventType::kShowCustomTopWidget, windowID, scheme, visible);
}

void WorkspaceEventCaller::sendSetSelectDetailFileUrl(const quint64 windowId, const QUrl &url)
{
    dpfSlotChannel->push("dfmplugin_detailspace", "slot_DetailView_Select", windowId, url);
}

void WorkspaceEventCaller::sendPaintEmblems(QPainter *painter, const QRectF &paintArea, const QUrl &url)
{
    dpfSlotChannel->push("dfmplugin_emblem", "slot_FileEmblems_Paint", painter, paintArea, url);
}

void WorkspaceEventCaller::sendViewSelectionChanged(const quint64 windowID, const QItemSelection &selected, const QItemSelection &deselected)
{
    dpfSignalDispatcher->publish(Workspace::EventType::kViewSelectionChanged, windowID, selected, deselected);
}

bool WorkspaceEventCaller::sendRenameStartEdit(const quint64 &winId, const QUrl &url)
{
    bool ret = dpfSignalDispatcher->publish("dfmplugin_workspace", "signal_RenameStartEdit", winId, url);
    return ret;
}

bool WorkspaceEventCaller::sendRenameEndEdit(const quint64 &winId, const QUrl &url)
{
    bool ret = dpfSignalDispatcher->publish("dfmplugin_workspace", "signal_RenameEndEdit", winId, url);
    return ret;
}

bool WorkspaceEventCaller::sendEnterFileView(const quint64 &winId, const QUrl &url)
{
    bool ret = dpfSignalDispatcher->publish("dfmplugin_workspace", "signal_EnterFileView", winId, url);
    return ret;
}

/*
    SnoreNotify is a Notification Framework based on Qt
    Copyright (C) 2013-2014  Patrick von Reth <vonreth@kde.org>


    SnoreNotify is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SnoreNotify is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnoreNotify.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SNARL_BACKEND_H
#define SNARL_BACKEND_H
#include "core/plugins/snorebackend.h"
#include "SnarlInterface.h"



class SnarlBackend:public Snore::SnoreBackend
{
    Q_OBJECT
    Q_INTERFACES(Snore::SnoreBackend)
    Q_PLUGIN_METADATA(IID "org.Snore.NotificationBackend/1.0")
public:
    SnarlBackend();
    ~SnarlBackend();
    virtual bool init(Snore::SnoreCore *snore);

private:
    class SnarlWidget;
    SnarlBackend::SnarlWidget* m_eventLoop;
    QHash<QString,Snarl::V42::SnarlInterface*> m_applications;
    Snarl::V42::SnarlInterface* m_defautSnarlinetrface;

public slots:
    void slotRegisterApplication(Snore::Application *application);
    void slotUnregisterApplication(Snore::Application *application);
    void slotNotify(Snore::Notification notification);
    void slotCloseNotification(Snore::Notification notification);

private:
    QHash<uint,LONG32> m_idMap;

};


#endif // SNARL_BACKEND_H

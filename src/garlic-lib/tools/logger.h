/*************************************************************************************
    garlic-player: SMIL Player for Digital Signage
    Copyright (C) 2024 Nikolaos Saghiadinos <ns@smil-control.com>
    This file is part of the garlic-player source code

    This program is free software: you can redistribute it and/or  modify
    it under the terms of the GNU Affero General Public License, version 3,
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************************/
#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include <iostream>
#include <mutex>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QQueue>

#include "log_file.h"
#include "logging_categories.h"
#include "i_main_configuration.hpp"

class ResourceMonitor;

/**
 * @brief The Logger class
 * Thread-Safe C++11 Meyers' Singleton
 */
class Logger : public QObject
{
        Q_OBJECT
    public:
        static   Logger&                  getInstance();
                 void                     setResourceMonitor(ResourceMonitor *rm);
                 void                     dispatchMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg);
                 QString                  createPlayLogEntry(QString start_time, QString content_id);
                 QString                  createTaskExecutionLogEntry(QString task_id, QString type);
                 QString                  createEventLogMetaData(QString event_name, QStringList meta_data);
                 void                     rotateLog(QString log_name);
                 QString                  getCurrentIsoDateTime();
                 void                     setConfiguration(IMainConfiguration *config);
    protected:
                 ResourceMonitor          *MyResourceMonitor = nullptr;
                 QScopedPointer<LogFile>  qtdebug_log, debug_log, play_log, event_log, task_execution_log;
                 QString                  collectDebugLog(QtMsgType type, const QMessageLogContext &context, const QString &msg);
                 QString                  collectEventLog(QtMsgType type, const QMessageLogContext &context, const QString &meta_data);
                 QString                  determineSeverity(QtMsgType type);

    private:
        explicit Logger(QObject *parent = nullptr);
                ~Logger()                          = default;
                 Logger(const Logger&)             = delete;
                 Logger & operator=(const Logger&) = delete;

          static Logger         *instance;
          static std::once_flag  initInstanceFlag;
          static void            initSingleton();

          // Remote Logging
          IMainConfiguration    *m_config = nullptr;
          QQueue<QString>        m_logBuffer;
          QNetworkAccessManager *m_networkManager = nullptr;
          QTimer                *m_uploadTimer = nullptr;
          const int              MAX_BUFFER_SIZE = 100;
          
          void                   addToBuffer(const QString &line);
          void                   triggerUpload();
};

#endif // LOGGER_H

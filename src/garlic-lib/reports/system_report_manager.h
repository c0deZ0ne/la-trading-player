#ifndef SYSTEM_REPORT_MANAGER_H
#define SYSTEM_REPORT_MANAGER_H

#include "base_report_manager.h"
#include "create/system_report.h"
#include "tools/resource_monitor.h"

namespace Reporting
{
    class SystemReportManager : public Reporting::BaseReportManager
    {
            Q_OBJECT
        public:
            explicit SystemReportManager(MainConfiguration *config, SystemInfos::DiscSpace *ds, ResourceMonitor *rm, QObject *parent = nullptr);
            void             handleSend();
        protected:
            QScopedPointer<Reporting::CreateSystemReport> MyCreateSystemReport;
        protected slots:
           void               doSucceed(TNetworkAccess *uploader);
           void               doFailed(TNetworkAccess *uploader);
       };
}
#endif // SYSTEM_REPORT_MANAGER_H

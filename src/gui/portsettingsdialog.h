#ifndef PORTSETTINGSDIALOG_H
#define PORTSETTINGSDIALOG_H

#include <QDialog>
#include "../protocols/serialprotocol.h"

namespace Ui {
class PortSettingsDialog;
}


class PortSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PortSettingsDialog(QString title, PortSettingsEx portSettings, QWidget *parent = nullptr);
    void accept() override;
    PortSettingsEx portSettings() const { return settings; }
    ~PortSettingsDialog();
    
private:
    /**
     * @brief Forward data exchange PortSettings -> GUI
     */
    void setInputValues();
    /**
     * @brief Backward data exchange GUI -> PortSettings
     */
    void getInputValues();

    Ui::PortSettingsDialog *ui;
    PortSettingsEx settings;
};

#endif // PORTSETTINGSDIALOG_H

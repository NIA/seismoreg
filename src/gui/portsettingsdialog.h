#ifndef PORTSETTINGSDIALOG_H
#define PORTSETTINGSDIALOG_H

#include <QDialog>
#include "qextserialport.h"

namespace Ui {
class PortSettingsDialog;
}

class PortSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PortSettingsDialog(QString title, PortSettings portSettings, QWidget *parent = 0);
    virtual void accept();
    PortSettings portSettings() const { return settings; }
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
    PortSettings settings;
};

#endif // PORTSETTINGSDIALOG_H

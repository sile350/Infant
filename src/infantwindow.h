#ifndef INFANTWINDOW_H
#define INFANTWINDOW_H

#include "apiclient.h"
#include "repository.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QDialog>
#include <optional>

class ImageButton final : public QLabel {
    Q_OBJECT
public:
    explicit ImageButton(QWidget *parent = nullptr);
    void setImagePath(const QString &path);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

class InfantWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit InfantWindow(const QString &licenseKey, QWidget *parent = nullptr);

private:
    enum class ScreenMode { Enter, Patients, Admin, Anamnesis };

    void buildUi();
    void applyLegacyStyle();
    void bindSignals();
    void setScreen(ScreenMode mode);

    QString imagePath(const QString &name) const;
    QString resourcePath(const QString &name) const;
    QString htmlPath(const QString &name) const;
    void setImage(ImageButton *button, const QString &name);
    QString defaultAnamnesisHtml() const;
    QString decodeDocument(const QString &raw) const;

    void refreshUsers();
    void refreshPatients();
    void refreshTemplateNames();
    void openPatientFromTable();
    void saveAnamnesisToDb();
    void showInfoPopup();
    void showHelpWindow(const QString &address);
    void showAboutWindow();

    ApiClient m_api;
    Repository m_repository;
    QString m_licenseKey;
    QString m_helpIndex = "окновходавпрограмму.html";
    std::optional<SessionUser> m_session;
    QString m_currentPatientId;

    QWidget *m_root = nullptr;

    ImageButton *m_bClose = nullptr;
    ImageButton *m_bLine = nullptr;
    ImageButton *m_bUp = nullptr;
    ImageButton *m_bBack = nullptr;
    ImageButton *m_bList = nullptr;
    ImageButton *m_bExit = nullptr;
    ImageButton *m_bPicPrint = nullptr;
    ImageButton *m_bUpload = nullptr;
    ImageButton *m_bSave = nullptr;
    ImageButton *m_bPrint = nullptr;
    ImageButton *m_bSettings = nullptr;
    ImageButton *m_bInfo = nullptr;
    ImageButton *m_pAna = nullptr;
    ImageButton *m_pProto = nullptr;
    ImageButton *m_pUpr = nullptr;
    QLabel *m_logo1 = nullptr;
    QLabel *m_logo2 = nullptr;

    QWidget *m_panelLogin = nullptr;
    QLineEdit *m_loginEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QLabel *m_loginManIcon = nullptr;
    QLabel *m_loginKeyIcon = nullptr;
    ImageButton *m_loginEye = nullptr;
    ImageButton *m_loginButton = nullptr;
    ImageButton *m_adminButton = nullptr;

    QWidget *m_panelAdmin = nullptr;
    QTableWidget *m_usersTable = nullptr;
    QLineEdit *m_userFio = nullptr;
    QLineEdit *m_userPass = nullptr;
    QLineEdit *m_userPass2 = nullptr;
    QLabel *m_adminLabel1 = nullptr;
    QLabel *m_adminLabel2 = nullptr;
    QLabel *m_adminManIcon = nullptr;
    QLabel *m_adminKeyIcon1 = nullptr;
    QLabel *m_adminKeyIcon2 = nullptr;
    ImageButton *m_adminEye1 = nullptr;
    ImageButton *m_adminEye2 = nullptr;
    QComboBox *m_userRole = nullptr;
    ImageButton *m_userSaveButton = nullptr;
    ImageButton *m_userOpenPatients = nullptr;
    QString m_editUserId;

    QWidget *m_panelPatients = nullptr;
    QLineEdit *m_patientSearch = nullptr;
    QTableWidget *m_patientsTable = nullptr;
    QCheckBox *m_dateFilter = nullptr;
    QDateEdit *m_dateFrom = nullptr;
    QDateEdit *m_dateTo = nullptr;
    QLabel *m_labelFrom = nullptr;
    QLabel *m_labelTo = nullptr;
    ImageButton *m_addPatient = nullptr;

    QWidget *m_panelWork = nullptr;
    QTextEdit *m_anamnesisEdit = nullptr;
    QLabel *m_patientTitle = nullptr;
    QComboBox *m_templates = nullptr;
    QSpinBox *m_fontSize = nullptr;
    ImageButton *m_underlineButton = nullptr;
    ImageButton *m_boldButton = nullptr;

    QDialog *m_infoPopup = nullptr;
    QDialog *m_helpWindow = nullptr;
    QDialog *m_aboutWindow = nullptr;
    QTextBrowser *m_helpBrowser = nullptr;
};

#endif

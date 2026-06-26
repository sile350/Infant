#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "src/apiclient.h"
#include "src/repository.h"

#include <QCheckBox>
#include <QDateEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QComboBox>
#include <QSpinBox>
#include <optional>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QString &licenseKey, QWidget *parent = nullptr);
    ~MainWindow();

private:
    void buildUi();
    void buildLoginPage();
    void buildPatientsPage();
    void buildAdminPage();
    void buildAnamnesisPage();
    void buildToolbar();

    void refreshUsers();
    void refreshPatients();
    void refreshTemplateNames();
    void loadPatientToEditor(const PatientRecord &patient);
    void setDefaultAnamnesis();
    QString decodeDocument(const QString &raw) const;

private slots:
    void onLoginClicked();
    void onLogoutClicked();
    void onUsersNewClicked();
    void onUsersSaveClicked();
    void onUsersDeleteClicked();
    void onUsersSelectionChanged();
    void onPatientsSearchChanged();
    void onPatientsFilterChanged();
    void onPatientNewClicked();
    void onPatientOpenClicked();
    void onPatientDeleteClicked();
    void onSaveAnamnesisClicked();
    void onSaveAnamnesisToFileClicked();
    void onPrintAnamnesisClicked();
    void onTemplateChanged(const QString &name);
    void onTemplateSaveClicked();
    void onBoldClicked();
    void onUnderlineClicked();
    void onFontSizeChanged(int size);

    void showLoginPage();
    void showPatientsPage();
    void showAdminPage();
    void showAnamnesisPage();

private:
    ApiClient m_api;
    Repository m_repository;
    QString m_licenseKey;

    std::optional<SessionUser> m_session;
    QString m_currentPatientId;
    QString m_currentPatientBirthDate;
    bool m_updatingTemplateList = false;

    QStackedWidget *m_stack = nullptr;
    QWidget *m_loginPage = nullptr;
    QWidget *m_patientsPage = nullptr;
    QWidget *m_adminPage = nullptr;
    QWidget *m_anamnesisPage = nullptr;

    QLineEdit *m_loginEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;

    QTableWidget *m_usersTable = nullptr;
    QLineEdit *m_userFioEdit = nullptr;
    QLineEdit *m_userPasswordEdit = nullptr;
    QLineEdit *m_userPasswordRepeatEdit = nullptr;
    QComboBox *m_userRoleCombo = nullptr;
    QString m_editingUserId;

    QLineEdit *m_patientSearchEdit = nullptr;
    QDateEdit *m_dateFromEdit = nullptr;
    QDateEdit *m_dateToEdit = nullptr;
    QCheckBox *m_dateFilterCheck = nullptr;
    QTableWidget *m_patientsTable = nullptr;

    QLabel *m_patientTitleLabel = nullptr;
    QComboBox *m_templateCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QTextEdit *m_anamnesisEdit = nullptr;
    QToolButton *m_boldButton = nullptr;
    QToolButton *m_underlineButton = nullptr;
};
#endif // MAINWINDOW_H

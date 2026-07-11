#ifndef INFANTWINDOW_H
#define INFANTWINDOW_H

#include "appsettings.h"
#include "exercisehost.h"
#include "imagebutton.h"
#include "repository.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QSlider>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QDialog>
#include <QTimer>
#include <optional>

class QPrinter;

class InfantWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit InfantWindow(const QString &licenseKey, bool openAdminOnStart = false, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class ScreenMode { Enter, Patients, Admin, Anamnesis, Protocols, Exercises };

    void buildUi();
    void buildSlidePanels();
    void applyLegacyStyle();
    void bindSignals();
    void setScreen(ScreenMode mode, bool pushHistory = true);
    void navigateBack();
    void updatePatientTabIcons();
    void toggleWindowMaximize();
    void updateMaximizeButtonIcon();
    void applyNormalWindowGeometry();
    void applyWindowGeometry(const QRect &rect);
    QRect calculateNormalWindowGeometry() const;
    QRect calculateMaximizedWindowGeometry() const;

    QString imagePath(const QString &name) const;
    QString resourcePath(const QString &name) const;
    QString htmlPath(const QString &name) const;
    QString exerciseNamesPath(const QString &name) const;
    void setImage(ImageButton *button, const QString &name);
    void applyEnterLogos();
    void styleInputField(QLineEdit *edit) const;
    void styleAdminInputField(QLineEdit *edit) const;
    void styleAdminScreen();
    void stylePatientsScreen();
    void styleAnamnesisScreen();
    void styleProtocolsView();
    void styleExercisesScreen();
    QString protocolsDocumentHtml(const QString &innerContent) const;
    void refreshProtocolsView();
    void styleUsersTable();
    void fitPatientsTableToContent();
    void loadDefaultAnamnesisTemplate();
    void loadStandardAnamnesisHtml();
    void applyAnamnesisDocument(const QString &raw);
    void loadAnamnesisRtf(const QByteArray &rtf);
    void applyAnamnesisFont(int pointSize);
    void applyAnamnesisFontToEntireDocument(int pointSize);
    void changeDocumentFontSize(int pointSize, bool persistProfile = true);
    void prepareAnamnesisDocumentForOutput();
    void applyAnamnesisDocumentFontDefaults();
    void applyCompactAnamnesisLineSpacing();
    void writeProfileConfig(const QString &profileName, int fontSize);
    QString profileConfigPath() const;
    void fitUsersTableToContent();
    void bindClearableField(QLineEdit *edit, ImageButton *clearBtn);
    QString defaultAnamnesisHtml() const;
    QString decodeDocument(const QString &raw) const;

    void refreshUsers();
    void scheduleRefreshUsers();
    void refreshPatients();
    void refreshTemplateNames();
    void loadAnamnesisTemplateByName(const QString &name);
    void saveCurrentAnamnesisTemplate();
    QString anamnesisTemplatePayload() const;
    void syncTemplateSelectorFromProfile();
    void refreshExercisesTree();
    QString exerciseFavoritesPath() const;
    bool isExerciseInFavorites(const QString &exerciseId) const;
    bool addExerciseToFavorites(const QString &exerciseId, const QString &displayText);
    bool removeExerciseFromFavorites(const QString &exerciseId);
    void showExercisesContextMenu(const QPoint &pos);
    void openExercise(const QString &exerciseId);
    void closeExerciseHost();
    void saveProtocolsEdits();
    void raiseChromeWidgets();
    void resetUserCreateForm();
    void loadUserForEdit(const QString &id);
    void deleteUserById(const QString &userId);
    void saveUser();
    void enterAsManagedUser();
    void rememberManagedUser(const QString &userId, const QString &login, const QString &password);
    void handlePatientsTableClick(int row, int column);
    void handlePatientsHeaderClick(int section);
    void openPatientFromTable();
    void tryAutoSaveAnamnesis(bool forceRefreshPatients = false);
    void setAnamnesisDbControlsEnabled(bool enabled);
    void updatePatientTitleFromDocument();
    void styleTemplateComboBox();
    void saveAnamnesisToDb();
    void exportDocument();
    void printSelectedContent();
    void showInfoPopup();
    void showHelpWindow(const QString &address);
    void showAboutWindow();
    void toggleSlidePanel(QWidget *panel);
    void hideSlidePanels();
    void animateSlidePanel(QWidget *panel, bool showPanel);
    void updateFormatButtonIcons();
    void showSettingsSaveTemplateView(bool show);
    void installToolbarTooltips();
    QString currentPatientBirthDate() const;
    struct ExportSelection {
        bool anamnesis = false;
        bool protocols = false;
        bool forPatient = false;
        bool forSpecialist = false;
    };
    QString protocolsExportHeader() const;
    QString assembleExportHtml(const ExportSelection &selection);
    void renderExportToPrinter(QPrinter &printer, const ExportSelection &selection, const QString &assembledHtml);
    ExportSelection saveExportSelection() const;
    ExportSelection printExportSelection() const;
    bool validateExportSelection(
        const ExportSelection &selection,
        const QString &emptyMessage,
        const QString &protocolsMessage);

    ApiClient m_api;
    Repository m_repository;
    QString m_licenseKey;
    QString m_helpIndex = "окновходавпрограмму.html";
    std::optional<SessionUser> m_session;
    QString m_mainId;
    QString m_currentPatientId;
    QList<ScreenMode> m_navHistory;
    ScreenMode m_currentScreen = ScreenMode::Enter;
    bool m_navigatingBack = false;
    int m_patientSortColumn = 1;
    bool m_patientSortAscending = true;
    int m_hoveredPatientRow = -1;
    QString m_selectedPatientRowId;
    bool m_underlineActive = false;
    bool m_boldActive = false;
    bool m_settingsTemplateMode = false;
    bool m_suppressTemplateLoad = false;
    QWidget *m_activeSlidePanel = nullptr;
    QTimer *m_printTooltipTimer = nullptr;

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
    ImageButton *m_loginClear = nullptr;
    ImageButton *m_passwordClear = nullptr;
    QLabel *m_loginManIcon = nullptr;
    QLabel *m_loginKeyIcon = nullptr;
    ImageButton *m_loginEye = nullptr;
    ImageButton *m_loginButton = nullptr;
    ImageButton *m_adminButton = nullptr;

    QWidget *m_panelAdmin = nullptr;
    QWidget *m_userFioPanel = nullptr;
    QWidget *m_userLoginPanel = nullptr;
    QWidget *m_userPassPanel = nullptr;
    QWidget *m_userPass2Panel = nullptr;
    QTableWidget *m_usersTable = nullptr;
    QLineEdit *m_userFio = nullptr;
    QLineEdit *m_userLogin = nullptr;
    QLineEdit *m_userPass = nullptr;
    QLineEdit *m_userPass2 = nullptr;
    ImageButton *m_userFioClear = nullptr;
    ImageButton *m_userLoginClear = nullptr;
    ImageButton *m_userPassClear = nullptr;
    ImageButton *m_userPass2Clear = nullptr;
    QLabel *m_adminLabel1 = nullptr;
    QLabel *m_adminLabel2 = nullptr;
    QLabel *m_adminManIcon = nullptr;
    QLabel *m_adminLoginIcon = nullptr;
    QLabel *m_adminKeyIcon1 = nullptr;
    QLabel *m_adminKeyIcon2 = nullptr;
    ImageButton *m_adminEye1 = nullptr;
    ImageButton *m_adminEye2 = nullptr;
    QComboBox *m_userRole = nullptr;
    ImageButton *m_userSaveButton = nullptr;
    ImageButton *m_userOpenPatients = nullptr;
    QCheckBox *m_dualScreenCheck = nullptr;
    QString m_editUserId;
    bool m_userSaveInProgress = false;
    bool m_anamnesisSaveInProgress = false;
    QString m_lastManagedUserId;
    QString m_lastManagedUserLogin;
    QString m_lastManagedUserPassword;

    QWidget *m_panelPatients = nullptr;
    QLineEdit *m_patientSearch = nullptr;
    ImageButton *m_patientSearchClear = nullptr;
    QTableWidget *m_patientsTable = nullptr;
    QStyledItemDelegate *m_patientsDelegate = nullptr;
    QCheckBox *m_dateFilter = nullptr;
    QDateEdit *m_dateFrom = nullptr;
    QDateEdit *m_dateTo = nullptr;
    QLabel *m_labelFrom = nullptr;
    QLabel *m_labelTo = nullptr;
    ImageButton *m_addPatient = nullptr;

    QWidget *m_panelWork = nullptr;
    QWidget *m_panelProtocols = nullptr;
    QWidget *m_panelExercises = nullptr;
    QStackedWidget *m_workStack = nullptr;
    QTextEdit *m_anamnesisEdit = nullptr;
    QTextEdit *m_protocolsView = nullptr;
    QLabel *m_protocolsTitle = nullptr;
    QLabel *m_protocolsPatient = nullptr;
    QTreeWidget *m_exercisesTree = nullptr;
    QWidget *m_exercisesTreeHost = nullptr;
    QComboBox *m_authorsFilter = nullptr;
    QWidget *m_authorsFilterHost = nullptr;
    QLabel *m_patientTitle = nullptr;
    QLabel *m_adminTitle = nullptr;
    QComboBox *m_templates = nullptr;
    QSlider *m_fontSlider = nullptr;
    QLabel *m_fontSizeLabel = nullptr;
    ImageButton *m_fontDownButton = nullptr;
    ImageButton *m_fontUpButton = nullptr;
    ImageButton *m_underlineButton = nullptr;
    ImageButton *m_boldButton = nullptr;
    ImageButton *m_templateDeleteButton = nullptr;
    ImageButton *m_templateSaveAsButton = nullptr;
    QLineEdit *m_templateNameEdit = nullptr;
    QWidget *m_settingsMainView = nullptr;
    QWidget *m_settingsSaveView = nullptr;

    QWidget *m_settingsPanel = nullptr;
    QWidget *m_savePanel = nullptr;
    QWidget *m_printPanel = nullptr;
    QCheckBox *m_saveAnamnesisCb = nullptr;
    QCheckBox *m_saveProtocolsCb = nullptr;
    QCheckBox *m_saveForPatientCb = nullptr;
    QCheckBox *m_saveForSpecialistCb = nullptr;
    QWidget *m_saveProtocolsSubPanel = nullptr;
    QCheckBox *m_printAnamnesisCb = nullptr;
    QCheckBox *m_printProtocolsCb = nullptr;
    QCheckBox *m_printForPatientCb = nullptr;
    QCheckBox *m_printForSpecialistCb = nullptr;
    QWidget *m_printProtocolsSubPanel = nullptr;
    QByteArray m_lastAnamnesisRtf;

    static constexpr int kDesignWidth = 1920;
    static constexpr int kDesignHeight = 1080;
    static constexpr int kTitleBarHeight = 57;
    static constexpr int kTaskbarReserve = 44;
    QRect m_savedWindowGeometry;
    QRect m_normalGeometryBeforeMaximize;
    bool m_isCustomMaximized = false;
    bool m_geometryInitialized = false;
    bool m_programmaticGeometryChange = false;
    bool m_screenTransitionGuard = false;

    QDialog *m_infoPopup = nullptr;
    QDialog *m_helpWindow = nullptr;
    QDialog *m_aboutWindow = nullptr;
    QTextBrowser *m_helpBrowser = nullptr;
    QString m_currentHelpFilePath;

    ExerciseHost *m_exerciseHost = nullptr;
    bool m_exerciseOpen = false;
};

#endif

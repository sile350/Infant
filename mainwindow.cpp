#include "mainwindow.h"
#include "src/custommessagebox.h"

#include <QAction>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QPrinter>
#include <QPrintDialog>
#include <QRegularExpression>
#include <QPushButton>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentWriter>
#include <QToolBar>
#include <QVBoxLayout>

MainWindow::MainWindow(const QString &licenseKey, QWidget *parent)
    : QMainWindow(parent),
      m_repository(&m_api, this),
      m_licenseKey(licenseKey) {
    buildUi();
    showLoginPage();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi() {
    resize(1400, 900);
    setWindowTitle("DokitLab Infant");
    buildToolbar();
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);
    buildLoginPage();
    buildPatientsPage();
    buildAdminPage();
    buildAnamnesisPage();
}

void MainWindow::buildToolbar() {
    QToolBar *toolbar = addToolBar("Навигация");
    toolbar->setMovable(false);

    QAction *patientsAction = toolbar->addAction("Пациенты");
    connect(patientsAction, &QAction::triggered, this, &MainWindow::showPatientsPage);

    QAction *anamnesisAction = toolbar->addAction("Анамнез");
    connect(anamnesisAction, &QAction::triggered, this, &MainWindow::showAnamnesisPage);

    QAction *adminAction = toolbar->addAction("Администрирование");
    connect(adminAction, &QAction::triggered, this, &MainWindow::showAdminPage);

    QAction *exerciseAction = toolbar->addAction("Упражнения");
    exerciseAction->setEnabled(false);

    QAction *logoutAction = toolbar->addAction("Выход");
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogoutClicked);
}

void MainWindow::buildLoginPage() {
    m_loginPage = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(m_loginPage);
    root->addStretch();

    QWidget *card = new QWidget(m_loginPage);
    card->setMaximumWidth(420);
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    QLabel *title = new QLabel("Вход в программу", card);
    QFont titleFont = title->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    title->setFont(titleFont);
    cardLayout->addWidget(title);

    m_loginEdit = new QLineEdit(card);
    m_loginEdit->setPlaceholderText("Логин");
    cardLayout->addWidget(m_loginEdit);

    m_passwordEdit = new QLineEdit(card);
    m_passwordEdit->setPlaceholderText("Пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    cardLayout->addWidget(m_passwordEdit);

    QPushButton *loginButton = new QPushButton("Войти", card);
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    cardLayout->addWidget(loginButton);

    root->addWidget(card, 0, Qt::AlignHCenter);
    root->addStretch();

    m_stack->addWidget(m_loginPage);
}

void MainWindow::buildPatientsPage() {
    m_patientsPage = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(m_patientsPage);

    QHBoxLayout *filters = new QHBoxLayout;
    m_patientSearchEdit = new QLineEdit(m_patientsPage);
    m_patientSearchEdit->setPlaceholderText("Поиск по ФИО");
    connect(m_patientSearchEdit, &QLineEdit::textChanged, this, &MainWindow::onPatientsSearchChanged);
    filters->addWidget(m_patientSearchEdit, 1);

    m_dateFilterCheck = new QCheckBox("Фильтр по дате добавления", m_patientsPage);
    connect(m_dateFilterCheck, &QCheckBox::stateChanged, this, &MainWindow::onPatientsFilterChanged);
    filters->addWidget(m_dateFilterCheck);

    m_dateFromEdit = new QDateEdit(QDate::currentDate().addMonths(-1), m_patientsPage);
    m_dateFromEdit->setCalendarPopup(true);
    connect(m_dateFromEdit, &QDateEdit::dateChanged, this, &MainWindow::onPatientsFilterChanged);
    filters->addWidget(m_dateFromEdit);

    m_dateToEdit = new QDateEdit(QDate::currentDate(), m_patientsPage);
    m_dateToEdit->setCalendarPopup(true);
    connect(m_dateToEdit, &QDateEdit::dateChanged, this, &MainWindow::onPatientsFilterChanged);
    filters->addWidget(m_dateToEdit);

    QPushButton *newButton = new QPushButton("Новая карта", m_patientsPage);
    connect(newButton, &QPushButton::clicked, this, &MainWindow::onPatientNewClicked);
    filters->addWidget(newButton);

    QPushButton *openButton = new QPushButton("Открыть", m_patientsPage);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::onPatientOpenClicked);
    filters->addWidget(openButton);

    QPushButton *deleteButton = new QPushButton("Удалить", m_patientsPage);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onPatientDeleteClicked);
    filters->addWidget(deleteButton);

    root->addLayout(filters);

    m_patientsTable = new QTableWidget(m_patientsPage);
    m_patientsTable->setColumnCount(4);
    m_patientsTable->setHorizontalHeaderLabels({"ID", "ФИО", "Дата рождения", "Дата добавления"});
    m_patientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_patientsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_patientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_patientsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    root->addWidget(m_patientsTable, 1);

    m_stack->addWidget(m_patientsPage);
}

void MainWindow::buildAdminPage() {
    m_adminPage = new QWidget(this);
    QHBoxLayout *root = new QHBoxLayout(m_adminPage);

    m_usersTable = new QTableWidget(m_adminPage);
    m_usersTable->setColumnCount(3);
    m_usersTable->setHorizontalHeaderLabels({"ID", "ФИО", "Уровень доступа"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_usersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    connect(m_usersTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onUsersSelectionChanged);
    root->addWidget(m_usersTable, 2);

    QWidget *form = new QWidget(m_adminPage);
    QVBoxLayout *formLayout = new QVBoxLayout(form);
    QFormLayout *fields = new QFormLayout;
    m_userFioEdit = new QLineEdit(form);
    fields->addRow("ФИО", m_userFioEdit);
    m_userPasswordEdit = new QLineEdit(form);
    m_userPasswordEdit->setEchoMode(QLineEdit::Password);
    fields->addRow("Пароль", m_userPasswordEdit);
    m_userPasswordRepeatEdit = new QLineEdit(form);
    m_userPasswordRepeatEdit->setEchoMode(QLineEdit::Password);
    fields->addRow("Подтверждение", m_userPasswordRepeatEdit);
    m_userRoleCombo = new QComboBox(form);
    m_userRoleCombo->addItems({"Специалист", "Администратор"});
    fields->addRow("Роль", m_userRoleCombo);
    formLayout->addLayout(fields);

    QPushButton *newButton = new QPushButton("Новый", form);
    connect(newButton, &QPushButton::clicked, this, &MainWindow::onUsersNewClicked);
    formLayout->addWidget(newButton);

    QPushButton *saveButton = new QPushButton("Сохранить", form);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onUsersSaveClicked);
    formLayout->addWidget(saveButton);

    QPushButton *deleteButton = new QPushButton("Удалить", form);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::onUsersDeleteClicked);
    formLayout->addWidget(deleteButton);

    formLayout->addStretch();
    root->addWidget(form, 1);
    m_stack->addWidget(m_adminPage);
}

void MainWindow::buildAnamnesisPage() {
    m_anamnesisPage = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(m_anamnesisPage);

    QHBoxLayout *topLine = new QHBoxLayout;
    m_patientTitleLabel = new QLabel("Новая карта", m_anamnesisPage);
    QFont patientFont = m_patientTitleLabel->font();
    patientFont.setPointSize(12);
    patientFont.setBold(true);
    m_patientTitleLabel->setFont(patientFont);
    topLine->addWidget(m_patientTitleLabel, 1);

    m_templateCombo = new QComboBox(m_anamnesisPage);
    connect(m_templateCombo, &QComboBox::currentTextChanged, this, &MainWindow::onTemplateChanged);
    topLine->addWidget(m_templateCombo);

    QPushButton *saveTemplateButton = new QPushButton("Сохранить шаблон", m_anamnesisPage);
    connect(saveTemplateButton, &QPushButton::clicked, this, &MainWindow::onTemplateSaveClicked);
    topLine->addWidget(saveTemplateButton);
    root->addLayout(topLine);

    QHBoxLayout *styleLine = new QHBoxLayout;
    m_boldButton = new QToolButton(m_anamnesisPage);
    m_boldButton->setText("B");
    m_boldButton->setCheckable(true);
    connect(m_boldButton, &QToolButton::clicked, this, &MainWindow::onBoldClicked);
    styleLine->addWidget(m_boldButton);

    m_underlineButton = new QToolButton(m_anamnesisPage);
    m_underlineButton->setText("U");
    m_underlineButton->setCheckable(true);
    connect(m_underlineButton, &QToolButton::clicked, this, &MainWindow::onUnderlineClicked);
    styleLine->addWidget(m_underlineButton);

    m_fontSizeSpin = new QSpinBox(m_anamnesisPage);
    m_fontSizeSpin->setRange(8, 36);
    m_fontSizeSpin->setValue(12);
    connect(m_fontSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::onFontSizeChanged);
    styleLine->addWidget(m_fontSizeSpin);

    QPushButton *saveDbButton = new QPushButton("Сохранить в БД", m_anamnesisPage);
    connect(saveDbButton, &QPushButton::clicked, this, &MainWindow::onSaveAnamnesisClicked);
    styleLine->addWidget(saveDbButton);

    QPushButton *saveFileButton = new QPushButton("Сохранить в файл", m_anamnesisPage);
    connect(saveFileButton, &QPushButton::clicked, this, &MainWindow::onSaveAnamnesisToFileClicked);
    styleLine->addWidget(saveFileButton);

    QPushButton *printButton = new QPushButton("Печать", m_anamnesisPage);
    connect(printButton, &QPushButton::clicked, this, &MainWindow::onPrintAnamnesisClicked);
    styleLine->addWidget(printButton);

    QPushButton *backButton = new QPushButton("К списку пациентов", m_anamnesisPage);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showPatientsPage);
    styleLine->addWidget(backButton);
    root->addLayout(styleLine);

    m_anamnesisEdit = new QTextEdit(m_anamnesisPage);
    root->addWidget(m_anamnesisEdit, 1);

    m_stack->addWidget(m_anamnesisPage);
}

void MainWindow::refreshUsers() {
    const QList<UserRecord> users = m_repository.fetchUsers();
    m_usersTable->setRowCount(users.size());
    for (int row = 0; row < users.size(); ++row) {
        const UserRecord &user = users.at(row);
        m_usersTable->setItem(row, 0, new QTableWidgetItem(user.id));
        m_usersTable->setItem(row, 1, new QTableWidgetItem(user.fio));
        m_usersTable->setItem(row, 2, new QTableWidgetItem(user.role));
    }
    m_usersTable->hideColumn(0);
}

void MainWindow::refreshPatients() {
    const QList<PatientRecord> patients = m_repository.fetchPatients(
        m_patientSearchEdit->text(),
        m_dateFilterCheck->isChecked(),
        m_dateFromEdit->date(),
        m_dateToEdit->date()
    );
    m_patientsTable->setRowCount(patients.size());
    for (int row = 0; row < patients.size(); ++row) {
        const PatientRecord &patient = patients.at(row);
        m_patientsTable->setItem(row, 0, new QTableWidgetItem(patient.id));
        m_patientsTable->setItem(row, 1, new QTableWidgetItem(patient.fio));
        m_patientsTable->setItem(row, 2, new QTableWidgetItem(patient.birthDate));
        const QString dateText = patient.createdAt > 0 ? QDateTime::fromSecsSinceEpoch(patient.createdAt).toString("dd.MM.yyyy HH:mm") : "";
        m_patientsTable->setItem(row, 3, new QTableWidgetItem(dateText));
    }
    m_patientsTable->hideColumn(0);
}

void MainWindow::refreshTemplateNames() {
    m_updatingTemplateList = true;
    const QString current = m_templateCombo->currentText();
    const QStringList templates = m_repository.loadTemplateNames();
    m_templateCombo->clear();
    m_templateCombo->addItems(templates);
    int index = m_templateCombo->findText(current);
    if (index < 0) {
        index = m_templateCombo->findText("Стандартный");
    }
    if (index >= 0) {
        m_templateCombo->setCurrentIndex(index);
    }
    m_updatingTemplateList = false;
}

void MainWindow::loadPatientToEditor(const PatientRecord &patient) {
    const QString raw = m_repository.loadPatientAnamnesis(patient.id);
    m_currentPatientId = patient.id;
    m_currentPatientBirthDate = patient.birthDate;
    m_patientTitleLabel->setText("ФИО: " + patient.fio + "   Дата рождения: " + patient.birthDate);
    m_anamnesisEdit->setHtml(decodeDocument(raw));
    showAnamnesisPage();
}

void MainWindow::setDefaultAnamnesis() {
    m_currentPatientId.clear();
    m_currentPatientBirthDate.clear();
    m_patientTitleLabel->setText("Новая карта");
    m_anamnesisEdit->setHtml(m_repository.defaultAnamnesisTemplate());
}

QString MainWindow::decodeDocument(const QString &raw) const {
    if (raw.trimmed().startsWith("{\\rtf")) {
        QString text = raw;
        text.replace(QRegularExpression("\\\\[a-zA-Z]+-?\\d* ?"), "");
        text.replace("{", "");
        text.replace("}", "");
        text.replace("\\'", "");
        return "<pre>" + text.toHtmlEscaped() + "</pre>";
    }
    if (raw.trimmed().isEmpty()) {
        return m_repository.defaultAnamnesisTemplate();
    }
    return raw;
}

void MainWindow::onLoginClicked() {
    const QString fio = m_loginEdit->text().trimmed();
    const QString password = m_passwordEdit->text();
    if (fio.isEmpty() || password.isEmpty()) {
        CustomMessageBox::showWarning(this, "Введите логин и пароль.");
        return;
    }
    const auto user = m_repository.login(fio, password);
    if (!user.has_value()) {
        CustomMessageBox::showError(this, "Неверный логин или пароль.");
        return;
    }
    if (user->role != "Администратор" && user->role != "Специалист") {
        CustomMessageBox::showError(this, "Недостаточный уровень доступа.");
        return;
    }
    m_session = user;
    refreshPatients();
    refreshUsers();
    refreshTemplateNames();
    showPatientsPage();
}

void MainWindow::onLogoutClicked() {
    m_session.reset();
    m_loginEdit->clear();
    m_passwordEdit->clear();
    m_currentPatientId.clear();
    showLoginPage();
}

void MainWindow::onUsersNewClicked() {
    m_editingUserId.clear();
    m_userFioEdit->clear();
    m_userPasswordEdit->clear();
    m_userPasswordRepeatEdit->clear();
    m_userRoleCombo->setCurrentText("Специалист");
}

void MainWindow::onUsersSaveClicked() {
    if (!m_session.has_value()) {
        return;
    }
    const QString fio = m_userFioEdit->text().trimmed();
    const QString password = m_userPasswordEdit->text();
    const QString repeat = m_userPasswordRepeatEdit->text();
    const QString role = m_userRoleCombo->currentText();

    if (fio.isEmpty()) {
        CustomMessageBox::showWarning(this, "Заполните ФИО.");
        return;
    }
    if (!password.isEmpty() && password != repeat) {
        CustomMessageBox::showWarning(this, "Пароль и подтверждение не совпадают.");
        return;
    }

    QString errorText;
    bool ok = false;
    if (m_editingUserId.isEmpty()) {
        if (password.isEmpty()) {
            CustomMessageBox::showWarning(this, "Введите пароль для нового пользователя.");
            return;
        }
        ok = m_repository.createUser(fio, fio, password, role, m_session->id, &errorText);
    } else {
        ok = m_repository.updateUser(m_editingUserId, fio, fio, password, role, &errorText);
    }
    if (!ok) {
        CustomMessageBox::showError(this, errorText.isEmpty() ? "Не удалось сохранить пользователя." : errorText);
        return;
    }
    onUsersNewClicked();
    refreshUsers();
}

void MainWindow::onUsersDeleteClicked() {
    const int row = m_usersTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString id = m_usersTable->item(row, 0)->text();
    if (!CustomMessageBox::askConfirm(this, "Вы собираетесь удалить данные!\nЭто действие необратимо!")) {
        return;
    }
    QString errorText;
    if (!m_repository.deleteUser(id, &errorText)) {
        CustomMessageBox::showError(this, errorText.isEmpty() ? "Не удалось удалить пользователя." : errorText);
        return;
    }
    onUsersNewClicked();
    refreshUsers();
}

void MainWindow::onUsersSelectionChanged() {
    const int row = m_usersTable->currentRow();
    if (row < 0) {
        return;
    }
    m_editingUserId = m_usersTable->item(row, 0)->text();
    m_userFioEdit->setText(m_usersTable->item(row, 1)->text());
    m_userRoleCombo->setCurrentText(m_usersTable->item(row, 2)->text());
    m_userPasswordEdit->clear();
    m_userPasswordRepeatEdit->clear();
}

void MainWindow::onPatientsSearchChanged() {
    refreshPatients();
}

void MainWindow::onPatientsFilterChanged() {
    refreshPatients();
}

void MainWindow::onPatientNewClicked() {
    setDefaultAnamnesis();
    showAnamnesisPage();
}

void MainWindow::onPatientOpenClicked() {
    const int row = m_patientsTable->currentRow();
    if (row < 0) {
        return;
    }
    PatientRecord patient;
    patient.id = m_patientsTable->item(row, 0)->text();
    patient.fio = m_patientsTable->item(row, 1)->text();
    patient.birthDate = m_patientsTable->item(row, 2)->text();
    loadPatientToEditor(patient);
}

void MainWindow::onPatientDeleteClicked() {
    const int row = m_patientsTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString id = m_patientsTable->item(row, 0)->text();
    if (!CustomMessageBox::askConfirm(this, "Вы собираетесь удалить данные!\nЭто действие необратимо!")) {
        return;
    }
    QString errorText;
    if (!m_repository.deletePatient(id, &errorText)) {
        CustomMessageBox::showError(this, errorText.isEmpty() ? "Не удалось удалить карту." : errorText);
        return;
    }
    refreshPatients();
}

void MainWindow::onSaveAnamnesisClicked() {
    QString detectedFio;
    QString detectedBirthDate;
    QString errorText;
    QString patientId = m_currentPatientId;
    if (!m_repository.savePatientAnamnesis(
            &patientId,
            m_licenseKey,
            m_anamnesisEdit->toPlainText(),
            m_anamnesisEdit->toHtml(),
            &detectedFio,
            &detectedBirthDate,
            &errorText)) {
        CustomMessageBox::showError(this, errorText.isEmpty() ? "Не удалось сохранить карту." : errorText);
        return;
    }
    m_currentPatientId = patientId;
    m_currentPatientBirthDate = detectedBirthDate;
    m_patientTitleLabel->setText("ФИО: " + detectedFio + "   Дата рождения: " + detectedBirthDate);
    CustomMessageBox::showInfo(this, "Карта успешно сохранена.");
    refreshPatients();
}

void MainWindow::onSaveAnamnesisToFileClicked() {
    const QString fileName = QFileDialog::getSaveFileName(this, "Сохранить анамнез", "anamnez", "HTML (*.html);;ODF (*.odf)");
    if (fileName.isEmpty()) {
        return;
    }
    QTextDocumentWriter writer(fileName);
    writer.write(m_anamnesisEdit->document());
}

void MainWindow::onPrintAnamnesisClicked() {
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    m_anamnesisEdit->document()->print(&printer);
}

void MainWindow::onTemplateChanged(const QString &name) {
    if (m_updatingTemplateList) {
        return;
    }
    const QString data = m_repository.loadTemplate(name);
    m_anamnesisEdit->setHtml(decodeDocument(data));
}

void MainWindow::onTemplateSaveClicked() {
    bool ok = false;
    QString name = m_templateCombo->currentText().trimmed();
    if (name == "Стандартный" || name.isEmpty()) {
        name = QInputDialog::getText(this, "Шаблон", "Название шаблона:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || name.isEmpty()) {
            return;
        }
    }
    QString errorText;
    if (!m_repository.saveTemplate(name, m_fontSizeSpin->value(), m_anamnesisEdit->toHtml(), &errorText)) {
        CustomMessageBox::showError(this, errorText.isEmpty() ? "Не удалось сохранить шаблон." : errorText);
        return;
    }
    refreshTemplateNames();
    int idx = m_templateCombo->findText(name);
    if (idx >= 0) {
        m_templateCombo->setCurrentIndex(idx);
    }
}

void MainWindow::onBoldClicked() {
    QTextCharFormat format;
    format.setFontWeight(m_boldButton->isChecked() ? QFont::Bold : QFont::Normal);
    m_anamnesisEdit->mergeCurrentCharFormat(format);
}

void MainWindow::onUnderlineClicked() {
    QTextCharFormat format;
    format.setFontUnderline(m_underlineButton->isChecked());
    m_anamnesisEdit->mergeCurrentCharFormat(format);
}

void MainWindow::onFontSizeChanged(int size) {
    QTextCharFormat format;
    format.setFontPointSize(size);
    m_anamnesisEdit->mergeCurrentCharFormat(format);
}

void MainWindow::showLoginPage() {
    m_stack->setCurrentWidget(m_loginPage);
}

void MainWindow::showPatientsPage() {
    if (!m_session.has_value()) {
        showLoginPage();
        return;
    }
    refreshPatients();
    m_stack->setCurrentWidget(m_patientsPage);
}

void MainWindow::showAdminPage() {
    if (!m_session.has_value()) {
        showLoginPage();
        return;
    }
    if (m_session->role != "Администратор") {
        CustomMessageBox::showError(this, "Ваш уровень доступа не позволяет открыть этот раздел.");
        return;
    }
    refreshUsers();
    m_stack->setCurrentWidget(m_adminPage);
}

void MainWindow::showAnamnesisPage() {
    if (!m_session.has_value()) {
        showLoginPage();
        return;
    }
    refreshTemplateNames();
    m_stack->setCurrentWidget(m_anamnesisPage);
}

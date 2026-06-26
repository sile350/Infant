#include "infantwindow.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QTextCharFormat>
#include <QTextDocumentWriter>
#include <QUrl>
#include <QVBoxLayout>
#include <functional>

ImageButton::ImageButton(QWidget *parent) : QLabel(parent) {
    setScaledContents(true);
}

void ImageButton::setImagePath(const QString &path) {
    setPixmap(QPixmap(path));
}

void ImageButton::mousePressEvent(QMouseEvent *event) {
    emit clicked();
    QLabel::mousePressEvent(event);
}

InfantWindow::InfantWindow(const QString &licenseKey, QWidget *parent)
    : QMainWindow(parent), m_repository(&m_api, this), m_licenseKey(licenseKey) {
    buildUi();
    applyLegacyStyle();
    bindSignals();
    setScreen(ScreenMode::Enter);
}

void InfantWindow::buildUi() {
    setWindowFlags(Qt::FramelessWindowHint);
    setFixedSize(1920, 1080);
    setWindowTitle("Инфант");
    const QString iconFile = resourcePath("infant.ico");
    if (!iconFile.isEmpty()) {
        setWindowIcon(QIcon(iconFile));
    }

    m_root = new QWidget(this);
    setCentralWidget(m_root);

    m_bClose = new ImageButton(m_root);
    m_bClose->setGeometry(1878, 10, 36, 34);
    m_bLine = new ImageButton(m_root);
    m_bLine->setGeometry(1794, 10, 36, 34);
    m_bUp = new ImageButton(m_root);
    m_bUp->setGeometry(1836, 10, 36, 34);
    m_bBack = new ImageButton(m_root);
    m_bBack->setGeometry(8, 12, 36, 34);
    m_bList = new ImageButton(m_root);
    m_bList->setGeometry(50, 12, 166, 34);
    m_bExit = new ImageButton(m_root);
    m_bExit->setGeometry(1683, 10, 36, 34);
    m_bPicPrint = new ImageButton(m_root);
    m_bPicPrint->setGeometry(1440, 10, 36, 34);
    m_bUpload = new ImageButton(m_root);
    m_bUpload->setGeometry(1398, 10, 36, 34);
    m_bSave = new ImageButton(m_root);
    m_bSave->setGeometry(1490, 10, 36, 34);
    m_bPrint = new ImageButton(m_root);
    m_bPrint->setGeometry(1532, 10, 36, 34);
    m_bSettings = new ImageButton(m_root);
    m_bSettings->setGeometry(1574, 10, 36, 34);
    m_bInfo = new ImageButton(m_root);
    m_bInfo->setGeometry(1617, 10, 36, 34);

    m_pAna = new ImageButton(m_root);
    m_pAna->setGeometry(745, 24, 143, 34);
    m_pProto = new ImageButton(m_root);
    m_pProto->setGeometry(889, 24, 143, 34);
    m_pUpr = new ImageButton(m_root);
    m_pUpr->setGeometry(1033, 24, 143, 34);

    m_logo1 = new QLabel(m_root);
    m_logo1->setGeometry(931, 81, 100, 50);
    m_logo1->setScaledContents(true);
    m_logo2 = new QLabel(m_root);
    m_logo2->setGeometry(1037, 81, 100, 50);
    m_logo2->setScaledContents(true);

    m_panelLogin = new QWidget(m_root);
    m_panelLogin->setGeometry((1920 - 500) / 2, (1080 - 176) / 2, 500, 176);
    m_panelLogin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelLogin->setStyleSheet("background: transparent;");
    m_loginEdit = new QLineEdit(m_panelLogin);
    m_loginEdit->setGeometry(75, 22, 393, 29);
    m_passwordEdit = new QLineEdit(m_panelLogin);
    m_passwordEdit->setGeometry(75, 75, 258, 29);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_loginManIcon = new QLabel(m_panelLogin);
    m_loginManIcon->setGeometry(47, 23, 24, 26);
    m_loginManIcon->setScaledContents(true);
    m_loginKeyIcon = new QLabel(m_panelLogin);
    m_loginKeyIcon->setGeometry(47, 76, 24, 26);
    m_loginKeyIcon->setScaledContents(true);
    m_loginEye = new ImageButton(m_panelLogin);
    m_loginEye->setGeometry(342, 77, 35, 23);
    m_loginButton = new ImageButton(m_panelLogin);
    m_loginButton->setGeometry(387, 71, 80, 36);
    m_adminButton = new ImageButton(m_panelLogin);
    m_adminButton->setGeometry(125, 129, 247, 37);

    m_panelAdmin = new QWidget(m_root);
    m_panelAdmin->setGeometry((1920 - 1034) / 2, (1080 - 337) / 2, 1034, 337);
    m_panelAdmin->setAttribute(Qt::WA_StyledBackground, true);
    m_panelAdmin->setStyleSheet("background: transparent;");
    m_usersTable = new QTableWidget(m_panelAdmin);
    m_usersTable->setGeometry(3, 31, 506, 260);
    m_usersTable->setColumnCount(3);
    m_usersTable->setHorizontalHeaderLabels({"id", "Пользователи", "Уровень доступа"});
    m_usersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usersTable->hideColumn(0);
    m_adminLabel1 = new QLabel("Создать пользователя", m_panelAdmin);
    m_adminLabel1->setGeometry(636, 20, 208, 20);
    m_adminLabel2 = new QLabel("Уровень доступа", m_panelAdmin);
    m_adminLabel2->setGeometry(674, 216, 154, 20);
    m_adminManIcon = new QLabel(m_panelAdmin);
    m_adminManIcon->setGeometry(515, 56, 32, 33);
    m_adminManIcon->setScaledContents(true);
    m_adminKeyIcon1 = new QLabel(m_panelAdmin);
    m_adminKeyIcon1->setGeometry(640, 111, 32, 33);
    m_adminKeyIcon1->setScaledContents(true);
    m_adminKeyIcon2 = new QLabel(m_panelAdmin);
    m_adminKeyIcon2->setGeometry(640, 165, 32, 33);
    m_adminKeyIcon2->setScaledContents(true);
    m_adminEye1 = new ImageButton(m_panelAdmin);
    m_adminEye1->setGeometry(954, 114, 35, 23);
    m_adminEye2 = new ImageButton(m_panelAdmin);
    m_adminEye2->setGeometry(954, 170, 35, 23);
    m_userFio = new QLineEdit(m_panelAdmin);
    m_userFio->setGeometry(551, 54, 399, 37);
    m_userPass = new QLineEdit(m_panelAdmin);
    m_userPass->setGeometry(676, 109, 274, 37);
    m_userPass->setEchoMode(QLineEdit::Password);
    m_userPass2 = new QLineEdit(m_panelAdmin);
    m_userPass2->setGeometry(676, 163, 274, 37);
    m_userPass2->setEchoMode(QLineEdit::Password);
    m_userRole = new QComboBox(m_panelAdmin);
    m_userRole->setGeometry(834, 215, 154, 28);
    m_userRole->addItems({"Специалист", "Администратор"});
    m_userSaveButton = new ImageButton(m_panelAdmin);
    m_userSaveButton->setGeometry(769, 261, 101, 30);
    m_userOpenPatients = new ImageButton(m_panelAdmin);
    m_userOpenPatients->setGeometry(896, 257, 80, 36);

    m_panelPatients = new QWidget(m_root);
    m_panelPatients->setGeometry((1920 - 749) / 2, 80, 749, 950);
    m_panelPatients->setAttribute(Qt::WA_StyledBackground, true);
    m_panelPatients->setStyleSheet("background: transparent;");
    m_patientSearch = new QLineEdit(m_panelPatients);
    m_patientSearch->setGeometry(14, 21, 319, 29);
    m_addPatient = new ImageButton(m_panelPatients);
    m_addPatient->setGeometry(334, 21, 143, 29);
    m_dateFilter = new QCheckBox("Показать пациентов, добавленных", m_panelPatients);
    m_dateFilter->setGeometry(485, 31, 263, 19);
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1), m_panelPatients);
    m_dateFrom->setGeometry(523, 57, 200, 20);
    m_dateTo = new QDateEdit(QDate::currentDate(), m_panelPatients);
    m_dateTo->setGeometry(523, 83, 200, 20);
    m_labelFrom = new QLabel("С", m_panelPatients);
    m_labelFrom->setGeometry(495, 63, 15, 13);
    m_labelTo = new QLabel("До", m_panelPatients);
    m_labelTo->setGeometry(495, 89, 24, 13);
    m_patientsTable = new QTableWidget(m_panelPatients);
    m_patientsTable->setGeometry(14, 63, 463, 800);
    m_patientsTable->setColumnCount(4);
    m_patientsTable->setHorizontalHeaderLabels({"id", "ФИО", "День рождения", "dt"});
    m_patientsTable->hideColumn(0);
    m_patientsTable->hideColumn(3);
    m_patientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_panelWork = new QWidget(m_root);
    m_panelWork->setGeometry((1920 - 965) / 2, 57, 965, 1000);
    m_panelWork->setStyleSheet("background-color:#f2f0f0;");
    m_underlineButton = new ImageButton(m_panelWork);
    m_underlineButton->setGeometry(38, 15, 36, 34);
    m_boldButton = new ImageButton(m_panelWork);
    m_boldButton->setGeometry(80, 15, 36, 34);
    m_patientTitle = new QLabel(m_root);
    m_patientTitle->setGeometry(478, 24, 237, 34);
    m_templates = new QComboBox(m_root);
    m_templates->setGeometry(1202, 29, 232, 28);
    m_fontSize = new QSpinBox(m_root);
    m_fontSize->setGeometry(1438, 29, 52, 28);
    m_fontSize->setRange(8, 36);
    m_fontSize->setValue(12);
    m_anamnesisEdit = new QTextEdit(m_panelWork);
    m_anamnesisEdit->setGeometry(29, 55, 900, 950);
}

void InfantWindow::applyLegacyStyle() {
    m_root->setStyleSheet("background-image: url('" + imagePath("fone.jpg") + "');");
    QFont formFont("Microsoft Sans Serif", 10);
    setFont(formFont);

    auto setFromCandidates = [](ImageButton *button, const QStringList &candidates) {
        for (const QString &file : candidates) {
            if (!file.isEmpty()) {
                button->setImagePath(file);
                return;
            }
        }
    };

    setFromCandidates(m_bClose, {resourcePath("Закрыть.png"), resourcePath("close.png"), imagePath("close.png")});
    setFromCandidates(m_bLine, {resourcePath("Свернуть.png"), imagePath("build.png")});
    setImage(m_bUp, "up.png");
    setImage(m_bBack, "back.png");
    setImage(m_bList, "plist.png");
    setImage(m_bExit, "exit.png");
    setImage(m_bPicPrint, "saveas.png");
    setImage(m_bUpload, "showp.png");
    setImage(m_bSave, "save.png");
    setImage(m_bPrint, "saveas.png");
    setImage(m_bSettings, "toogl.png");
    setFromCandidates(m_bInfo, {resourcePath("Информация.png"), imagePath("im1.png")});
    setImage(m_loginButton, "enter.png");
    setImage(m_adminButton, "admin.png");
    if (m_loginManIcon) {
        m_loginManIcon->setPixmap(QPixmap(imagePath("man.png")));
    }
    if (m_loginKeyIcon) {
        m_loginKeyIcon->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_loginEye, "pon.png");
    setImage(m_userSaveButton, "save.png");
    setImage(m_userOpenPatients, "enter.png");
    if (m_adminManIcon) {
        m_adminManIcon->setPixmap(QPixmap(imagePath("man.png")));
    }
    if (m_adminKeyIcon1) {
        m_adminKeyIcon1->setPixmap(QPixmap(imagePath("key.png")));
    }
    if (m_adminKeyIcon2) {
        m_adminKeyIcon2->setPixmap(QPixmap(imagePath("key.png")));
    }
    setImage(m_adminEye1, "pon.png");
    setImage(m_adminEye2, "pon.png");
    setImage(m_addPatient, "addp.png");
    setImage(m_underlineButton, "undeline.png");
    setImage(m_boldButton, "to.png");

    m_logo1->setPixmap(QPixmap(imagePath("infant.png")));
    m_logo2->setPixmap(QPixmap(imagePath("infant2.png")));

    m_loginEdit->setPlaceholderText("ФИО");
    m_passwordEdit->setPlaceholderText("Пароль");
    m_userFio->setPlaceholderText("ФИО");
    m_userPass->setPlaceholderText("Придумайте пароль");
    m_userPass2->setPlaceholderText("Подтвердите пароль");
    m_patientSearch->setPlaceholderText("Поиск");

    m_loginEdit->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_passwordEdit->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_userFio->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_userPass->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_userPass2->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_patientSearch->setStyleSheet("font-size:14pt;color:silver;background:white;");
    m_dateFilter->setStyleSheet("color:white;font-weight:bold;");
    m_adminLabel1->setStyleSheet("color:white;font-size:12pt;font-weight:bold;");
    m_adminLabel2->setStyleSheet("color:white;font-size:12pt;font-weight:bold;");
    m_labelFrom->setStyleSheet("color:white;font-weight:bold;");
    m_labelTo->setStyleSheet("color:white;font-weight:bold;");
    m_patientTitle->setStyleSheet("color:black;font-size:11pt;");
    m_anamnesisEdit->setStyleSheet("background:white;");
    m_usersTable->setStyleSheet("background:white;");
    m_patientsTable->setStyleSheet("background:white;");
}

void InfantWindow::bindSignals() {
    connect(m_bClose, &ImageButton::clicked, this, &InfantWindow::close);
    connect(m_bLine, &ImageButton::clicked, this, &InfantWindow::showMinimized);
    connect(m_bUp, &ImageButton::clicked, this, [this]() {
        if (height() == 1080) {
            setFixedHeight(1036);
            setImage(m_bUp, "down.png");
        } else {
            setFixedHeight(1080);
            setImage(m_bUp, "up.png");
        }
    });
    connect(m_bExit, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Enter); });
    connect(m_bBack, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Patients); });
    connect(m_bList, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Patients); });
    connect(m_pAna, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Anamnesis); });
    connect(m_pUpr, &ImageButton::clicked, this, [this]() { QMessageBox::information(this, "Раздел", "Блок упражнений временно отключен."); });
    connect(m_pProto, &ImageButton::clicked, this, [this]() { QMessageBox::information(this, "Раздел", "Блок протоколов временно отключен."); });
    connect(m_bInfo, &ImageButton::clicked, this, [this]() { showInfoPopup(); });

    connect(m_loginButton, &ImageButton::clicked, this, [this]() {
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            QMessageBox::critical(this, "Вход", "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор" && user->role != "Специалист") {
            QMessageBox::critical(this, "Вход", "Недостаточный уровень доступа.");
            return;
        }
        m_session = user;
        refreshPatients();
        setScreen(ScreenMode::Patients);
    });
    connect(m_loginEye, &ImageButton::clicked, this, [this]() {
        if (m_passwordEdit->echoMode() == QLineEdit::Password) {
            m_passwordEdit->setEchoMode(QLineEdit::Normal);
            setImage(m_loginEye, "poff.png");
        } else {
            m_passwordEdit->setEchoMode(QLineEdit::Password);
            setImage(m_loginEye, "pon.png");
        }
    });

    connect(m_adminButton, &ImageButton::clicked, this, [this]() {
        const auto user = m_repository.login(m_loginEdit->text().trimmed(), m_passwordEdit->text());
        if (!user.has_value()) {
            QMessageBox::critical(this, "Вход", "Неверный логин или пароль!");
            return;
        }
        if (user->role != "Администратор") {
            QMessageBox::critical(this, "Вход", "Ваш уровень доступа не позволяет осуществлять администрирование!");
            return;
        }
        m_session = user;
        refreshUsers();
        setScreen(ScreenMode::Admin);
    });

    connect(m_userSaveButton, &ImageButton::clicked, this, [this]() {
        if (!m_session.has_value()) {
            return;
        }
        if (m_userFio->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Пользователи", "Заполните все поля.");
            return;
        }
        if (m_userPass->text() != m_userPass2->text()) {
            QMessageBox::warning(this, "Пользователи", "Пароли должны быть идентичны.");
            return;
        }
        QString err;
        bool ok = false;
        if (m_editUserId.isEmpty()) {
            if (m_userPass->text().isEmpty()) {
                QMessageBox::warning(this, "Пользователи", "Введите пароль.");
                return;
            }
            ok = m_repository.createUser(m_userFio->text().trimmed(), m_userPass->text(), m_userRole->currentText(), m_session->id, &err);
        } else {
            ok = m_repository.updateUser(m_editUserId, m_userFio->text().trimmed(), m_userPass->text(), m_userRole->currentText(), &err);
        }
        if (!ok) {
            QMessageBox::critical(this, "Пользователи", err);
            return;
        }
        m_editUserId.clear();
        m_userFio->clear();
        m_userPass->clear();
        m_userPass2->clear();
        refreshUsers();
    });

    connect(m_userOpenPatients, &ImageButton::clicked, this, [this]() { setScreen(ScreenMode::Patients); });
    connect(m_adminEye1, &ImageButton::clicked, this, [this]() {
        if (m_userPass->echoMode() == QLineEdit::Password) {
            m_userPass->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye1, "poff.png");
        } else {
            m_userPass->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye1, "pon.png");
        }
    });
    connect(m_adminEye2, &ImageButton::clicked, this, [this]() {
        if (m_userPass2->echoMode() == QLineEdit::Password) {
            m_userPass2->setEchoMode(QLineEdit::Normal);
            setImage(m_adminEye2, "poff.png");
        } else {
            m_userPass2->setEchoMode(QLineEdit::Password);
            setImage(m_adminEye2, "pon.png");
        }
    });

    connect(m_usersTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        const int row = m_usersTable->currentRow();
        if (row < 0) {
            return;
        }
        m_editUserId = m_usersTable->item(row, 0)->text();
        m_userFio->setText(m_usersTable->item(row, 1)->text());
        m_userRole->setCurrentText(m_usersTable->item(row, 2)->text());
        m_userPass->clear();
        m_userPass2->clear();
    });

    connect(m_patientSearch, &QLineEdit::textChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFilter, &QCheckBox::stateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateFrom, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });
    connect(m_dateTo, &QDateEdit::dateChanged, this, [this]() { refreshPatients(); });

    connect(m_addPatient, &ImageButton::clicked, this, [this]() {
        m_currentPatientId.clear();
        m_patientTitle->setText("Новая карта");
        m_anamnesisEdit->setHtml(defaultAnamnesisHtml());
        setScreen(ScreenMode::Anamnesis);
    });

    connect(m_patientsTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) { openPatientFromTable(); });
    connect(m_bSave, &ImageButton::clicked, this, [this]() { saveAnamnesisToDb(); });
    connect(m_bPrint, &ImageButton::clicked, this, [this]() {
        QPrinter printer;
        QPrintDialog dialog(&printer, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_anamnesisEdit->document()->print(&printer);
        }
    });
    connect(m_bSettings, &ImageButton::clicked, this, [this]() {
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Шаблон", "Название шаблона:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || name.isEmpty()) {
            return;
        }
        QString err;
        if (!m_repository.saveTemplate(name, m_fontSize->value(), m_anamnesisEdit->toHtml(), &err)) {
            QMessageBox::critical(this, "Шаблон", err);
            return;
        }
        refreshTemplateNames();
    });
    connect(m_templates, &QComboBox::currentTextChanged, this, [this](const QString &name) {
        if (name.isEmpty()) {
            return;
        }
        m_anamnesisEdit->setHtml(decodeDocument(m_repository.loadTemplate(name)));
    });
    connect(m_fontSize, qOverload<int>(&QSpinBox::valueChanged), this, [this](int size) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(size);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
    });
    connect(m_underlineButton, &ImageButton::clicked, this, [this]() {
        QTextCharFormat fmt;
        fmt.setFontUnderline(!m_anamnesisEdit->currentCharFormat().fontUnderline());
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
    });
    connect(m_boldButton, &ImageButton::clicked, this, [this]() {
        QTextCharFormat fmt;
        const bool isBold = m_anamnesisEdit->currentCharFormat().fontWeight() >= QFont::Bold;
        fmt.setFontWeight(isBold ? QFont::Normal : QFont::Bold);
        m_anamnesisEdit->mergeCurrentCharFormat(fmt);
    });
}

void InfantWindow::setScreen(ScreenMode mode) {
    const bool enter = mode == ScreenMode::Enter;
    const bool patients = mode == ScreenMode::Patients;
    const bool admin = mode == ScreenMode::Admin;
    const bool anamnesis = mode == ScreenMode::Anamnesis;

    m_logo1->setVisible(enter);
    m_logo2->setVisible(enter);
    m_panelLogin->setVisible(enter);

    m_panelPatients->setVisible(patients);
    m_panelAdmin->setVisible(admin);
    m_panelWork->setVisible(anamnesis);

    m_bBack->setVisible(patients || admin || anamnesis);
    m_bList->setVisible(anamnesis);
    m_bExit->setVisible(!enter);
    m_bSave->setVisible(anamnesis);
    m_bPrint->setVisible(anamnesis);
    m_bSettings->setVisible(anamnesis);
    m_bPicPrint->setVisible(false);
    m_bUpload->setVisible(false);
    m_bInfo->setVisible(true);
    m_pAna->setVisible(anamnesis);
    m_pProto->setVisible(anamnesis);
    m_pUpr->setVisible(anamnesis);
    m_templates->setVisible(anamnesis);
    m_fontSize->setVisible(false);
    m_templates->setVisible(false);
    m_patientTitle->setVisible(anamnesis);

    if (patients) {
        m_helpIndex = "списокпациентов.html";
        refreshPatients();
    }
    if (admin) {
        m_helpIndex = "администрирование.html";
        refreshUsers();
    }
    if (anamnesis) {
        m_helpIndex = "анамнез.html";
        refreshTemplateNames();
        setImage(m_pAna, "anaon.png");
        setImage(m_pProto, "protoff.png");
        setImage(m_pUpr, "uproff.png");
        if (m_currentPatientId.isEmpty() && m_anamnesisEdit->toPlainText().trimmed().isEmpty()) {
            m_anamnesisEdit->setHtml(defaultAnamnesisHtml());
        }
    }
    if (enter) {
        m_helpIndex = "окновходавпрограмму.html";
    }
}

QString InfantWindow::imagePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::resourcePath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../assets/resources",
        QCoreApplication::applicationDirPath() + "/../../../assets/resources",
        QDir::currentPath() + "/assets/resources"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::htmlPath(const QString &name) const {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../assets/htmls",
        QCoreApplication::applicationDirPath() + "/../../../assets/htmls",
        QDir::currentPath() + "/assets/htmls",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/htmls"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QString InfantWindow::defaultAnamnesisHtml() const {
    const QString path = htmlPath("anamnez.rtf");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QString raw = QString::fromUtf8(file.readAll());
            file.close();
            if (raw.trimmed().isEmpty()) {
                return m_repository.defaultAnamnesisTemplate();
            }
            return decodeDocument(raw);
        }
    }
    return m_repository.defaultAnamnesisTemplate();
}

void InfantWindow::setImage(ImageButton *button, const QString &name) {
    const QString path = imagePath(name);
    if (!path.isEmpty()) {
        button->setImagePath(path);
    }
}

QString InfantWindow::decodeDocument(const QString &raw) const {
    if (raw.trimmed().startsWith("{\\rtf")) {
        QString text = raw;
        text.replace(QRegularExpression("\\\\[a-zA-Z]+-?\\d* ?"), "");
        text.replace("{", "");
        text.replace("}", "");
        text.replace("\\'", "");
        return "<pre>" + text.toHtmlEscaped() + "</pre>";
    }
    if (raw.trimmed().isEmpty()) {
        return defaultAnamnesisHtml();
    }
    return raw;
}

void InfantWindow::refreshUsers() {
    const QList<UserRecord> users = m_repository.fetchUsers();
    m_usersTable->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        m_usersTable->setItem(i, 0, new QTableWidgetItem(users.at(i).id));
        m_usersTable->setItem(i, 1, new QTableWidgetItem(users.at(i).fio));
        m_usersTable->setItem(i, 2, new QTableWidgetItem(users.at(i).role));
    }
}

void InfantWindow::refreshPatients() {
    const QList<PatientRecord> patients = m_repository.fetchPatients(
        m_patientSearch->text(),
        m_dateFilter->isChecked(),
        m_dateFrom->date(),
        m_dateTo->date()
    );
    m_patientsTable->setRowCount(patients.size());
    for (int i = 0; i < patients.size(); ++i) {
        const PatientRecord &p = patients.at(i);
        m_patientsTable->setItem(i, 0, new QTableWidgetItem(p.id));
        m_patientsTable->setItem(i, 1, new QTableWidgetItem(p.fio));
        m_patientsTable->setItem(i, 2, new QTableWidgetItem(p.birthDate));
        m_patientsTable->setItem(i, 3, new QTableWidgetItem(QString::number(p.createdAt)));
    }
}

void InfantWindow::refreshTemplateNames() {
    const QString current = m_templates->currentText();
    m_templates->clear();
    m_templates->addItems(m_repository.loadTemplateNames());
    const int idx = m_templates->findText(current);
    if (idx >= 0) {
        m_templates->setCurrentIndex(idx);
    }
}

void InfantWindow::openPatientFromTable() {
    const int row = m_patientsTable->currentRow();
    if (row < 0) {
        return;
    }
    const QString patientId = m_patientsTable->item(row, 0)->text();
    const QString fio = m_patientsTable->item(row, 1)->text();
    const QString dr = m_patientsTable->item(row, 2)->text();
    m_currentPatientId = patientId;
    m_patientTitle->setText("ФИО: " + fio + "       Дата рождения: " + dr);
    m_anamnesisEdit->setHtml(decodeDocument(m_repository.loadPatientAnamnesis(patientId)));
    setScreen(ScreenMode::Anamnesis);
}

void InfantWindow::saveAnamnesisToDb() {
    QString patientId = m_currentPatientId;
    QString fio;
    QString dr;
    QString err;
    if (!m_repository.savePatientAnamnesis(&patientId, m_licenseKey, m_anamnesisEdit->toHtml(), &fio, &dr, &err)) {
        QMessageBox::critical(this, "Анамнез", err);
        return;
    }
    m_currentPatientId = patientId;
    m_patientTitle->setText("ФИО: " + fio + "       Дата рождения: " + dr);
    refreshPatients();
    QMessageBox::information(this, "Анамнез", "Сохранено.");
}

void InfantWindow::showInfoPopup() {
    if (!m_infoPopup) {
        m_infoPopup = new QDialog(this, Qt::FramelessWindowHint);
        m_infoPopup->setFixedSize(447, 133);
        m_infoPopup->setAttribute(Qt::WA_TranslucentBackground, false);
        const QString bg = imagePath("popup.png");
        m_infoPopup->setStyleSheet("QDialog { background-image: url('" + bg + "'); }");

        auto createLabel = [&](const QString &text, const QRect &rect, auto callback) {
            QLabel *label = new QLabel(text, m_infoPopup);
            label->setGeometry(rect);
            label->setStyleSheet("color:black;background:transparent;font: 11.25pt 'Microsoft Sans Serif';");
            label->setCursor(Qt::PointingHandCursor);
            class ClickLabel final : public QLabel {
            public:
                using QLabel::QLabel;
                std::function<void()> onClick;
            protected:
                void mousePressEvent(QMouseEvent *event) override {
                    if (onClick) {
                        onClick();
                    }
                    QLabel::mousePressEvent(event);
                }
            };
            ClickLabel *clickLabel = new ClickLabel(m_infoPopup);
            clickLabel->setText(text);
            clickLabel->setGeometry(rect);
            clickLabel->setStyleSheet(label->styleSheet());
            clickLabel->setCursor(label->cursor());
            clickLabel->onClick = callback;
            label->deleteLater();
            return clickLabel;
        };

        createLabel("Руководство пользователя к данной странице", QRect(23, 23, 380, 18), [this]() {
            m_infoPopup->hide();
            const QString path = htmlPath("spravka/" + m_helpIndex);
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("Общее руководство пользователя", QRect(23, 56, 300, 18), [this]() {
            m_infoPopup->hide();
            QString path = htmlPath("spravka/руководствопользователя.html");
            if (path.isEmpty()) {
                path = htmlPath("spravka/основныеэлементыуправления.html");
            }
            if (!path.isEmpty()) {
                showHelpWindow(path);
            }
        });
        createLabel("О программе", QRect(23, 89, 140, 18), [this]() {
            m_infoPopup->hide();
            showAboutWindow();
        });
    }

    QPoint p = mapToGlobal(QPoint(1400, 70));
    m_infoPopup->move(p);
    m_infoPopup->show();
    m_infoPopup->raise();
    m_infoPopup->activateWindow();
}

void InfantWindow::showHelpWindow(const QString &address) {
    if (!m_helpWindow) {
        m_helpWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_helpWindow->setFixedSize(873, 900);
        m_helpWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("spravka.png") + "'); }");

        m_helpBrowser = new QTextBrowser(m_helpWindow);
        m_helpBrowser->setGeometry(10, 110, 851, 767);

        ImageButton *closeBtn = new ImageButton(m_helpWindow);
        closeBtn->setGeometry(817, 12, 44, 38);
        const QString closeRes = resourcePath("Закрыть.png");
        if (!closeRes.isEmpty()) {
            closeBtn->setImagePath(closeRes);
        }
        connect(closeBtn, &ImageButton::clicked, m_helpWindow, &QDialog::hide);

        ImageButton *manualBtn = new ImageButton(m_helpWindow);
        manualBtn->setGeometry(733, 74, 100, 30);
        manualBtn->setImagePath(imagePath("toogl.png"));
        connect(manualBtn, &ImageButton::clicked, this, [this]() {
            QString path = htmlPath("spravka/руководствопользователя.html");
            if (path.isEmpty()) {
                path = htmlPath("spravka/основныеэлементыуправления.html");
            }
            if (!path.isEmpty() && m_helpBrowser) {
                m_helpBrowser->setSource(QUrl::fromLocalFile(path));
            }
        });
    }

    if (m_helpBrowser) {
        m_helpBrowser->setSource(QUrl::fromLocalFile(address));
    }
    m_helpWindow->show();
    m_helpWindow->raise();
    m_helpWindow->activateWindow();
}

void InfantWindow::showAboutWindow() {
    if (!m_aboutWindow) {
        m_aboutWindow = new QDialog(this, Qt::FramelessWindowHint);
        m_aboutWindow->setFixedSize(447, 133);
        m_aboutWindow->setStyleSheet("QDialog { background-image: url('" + imagePath("popup.png") + "'); }");

        auto addText = [&](const QString &text, const QRect &r, int size) {
            QLabel *l = new QLabel(text, m_aboutWindow);
            l->setGeometry(r);
            l->setStyleSheet("color:black;background:transparent;font:" + QString::number(size) + "pt 'Microsoft Sans Serif';");
        };
        addText("О программе", QRect(170, 29, 150, 18), 11);
        addText("Программа для ЭВМ «Пособие для оценки, мониторинга и ", QRect(20, 57, 410, 16), 9);
        addText("скрининга психофизического развития ребенка «Инфант» ", QRect(20, 73, 410, 16), 9);
        addText("(интерактивная программа)» по ТУ 58.29.32-001-42446431-2019", QRect(12, 89, 430, 16), 9);
        addText("Версия 1.0 от 12.12.2019.", QRect(143, 105, 180, 16), 9);

        ImageButton *closeBtn = new ImageButton(m_aboutWindow);
        closeBtn->setGeometry(404, 11, 31, 29);
        QString closeRes = resourcePath("Закрыть.png");
        if (!closeRes.isEmpty()) {
            closeBtn->setImagePath(closeRes);
        }
        connect(closeBtn, &ImageButton::clicked, m_aboutWindow, &QDialog::hide);
    }
    m_aboutWindow->show();
    m_aboutWindow->raise();
    m_aboutWindow->activateWindow();
}

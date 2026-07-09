#include "exercisehost.h"

#include "custommessagebox.h"
#include "exerciseassets.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "repository.h"

#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QString loadExerciseHtml(const QString &exerciseId, const QString &fileName) {
    const QString path = ExerciseAssets::exerciseFile(exerciseId, fileName);
    if (path.isEmpty()) {
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QString baseDir = ExerciseAssets::exerciseDir(exerciseId);
    return ExerciseAssets::prepareExerciseHtml(QString::fromUtf8(file.readAll()), baseDir);
}

QCheckBox *makeCheck(const QString &text, QWidget *parent) {
    auto *box = new QCheckBox(text, parent);
    box->setStyleSheet(QStringLiteral("QCheckBox { font-size: 14px; color: #000000; }"));
    return box;
}

} // namespace

ExerciseHost::ExerciseHost(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background-color: #f8f8f8;"));

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setGeometry(0, 0, 1100, 1000);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_scrollContent = new QWidget;
    auto *layout = new QVBoxLayout(m_scrollContent);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    m_orBrowser = new QTextBrowser(m_scrollContent);
    m_orBrowser->setOpenExternalLinks(false);
    m_orBrowser->setFrameShape(QFrame::NoFrame);
    m_orBrowser->setStyleSheet(QStringLiteral("background-color: #f8f8f8; border: none;"));
    m_orBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_checkboxPanel = new QWidget(m_scrollContent);
    auto *checkboxLayout = new QVBoxLayout(m_checkboxPanel);
    checkboxLayout->setContentsMargins(24, 0, 8, 0);
    checkboxLayout->setSpacing(4);

    m_activityChecks << makeCheck(QStringLiteral("Ребенок не понимает инструкцию."), m_checkboxPanel)
                     << makeCheck(QStringLiteral("Ребенок понимает инструкцию, но не может выполнить задание."), m_checkboxPanel)
                     << makeCheck(QStringLiteral("Целенаправленное выполнение задания."), m_checkboxPanel);
    m_helpChecks << makeCheck(
                        QStringLiteral(
                            "Одобрение или неодобрение действий ребенка, стимуляция с помощью слов "
                            "«хорошо», «правильно», «неправильно, подумай еще»."),
                        m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral(
                            "Вопросы к испытуемому о том, почему он сделал то или иное действие, с целью "
                            "повышения уровня осознания смысла задания и ориентировки в задании."),
                        m_checkboxPanel)
                 << makeCheck(QStringLiteral("Подсказка, совет действовать тем или иным образом."), m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral("Показ способа выполнения задания с просьбой повторить это действие."),
                        m_checkboxPanel)
                 << makeCheck(
                        QStringLiteral(
                            "Совместно-раздельная деятельность: специалист начинает выполнять задание, а ребенок "
                            "продолжает."),
                        m_checkboxPanel);

    for (QCheckBox *box : m_activityChecks) {
        checkboxLayout->addWidget(box);
        connect(box, &QCheckBox::toggled, this, [this, box](bool checked) {
            if (!checked) {
                return;
            }
            for (QCheckBox *other : m_activityChecks) {
                if (other != box) {
                    other->setChecked(false);
                }
            }
        });
    }
    checkboxLayout->addSpacing(8);
    for (QCheckBox *box : m_helpChecks) {
        checkboxLayout->addWidget(box);
    }

    m_beginButton = new QPushButton(QStringLiteral("Начать"), m_scrollContent);
    m_beginButton->setFixedHeight(34);
    m_beginButton->setStyleSheet(
        QStringLiteral("QPushButton { font-size: 14px; font-weight: bold; padding: 4px 18px; }"));

    m_templateBrowser = new QTextBrowser(m_scrollContent);
    m_templateBrowser->setOpenExternalLinks(false);
    m_templateBrowser->setFrameShape(QFrame::NoFrame);
    m_templateBrowser->setStyleSheet(QStringLiteral("background-color: #f8f8f8; border: none;"));
    m_templateBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_formProtocolButton = new QPushButton(QStringLiteral("Сформировать протокол"), m_scrollContent);
    m_formProtocolButton->setFixedHeight(34);
    m_formProtocolButton->setStyleSheet(
        QStringLiteral("QPushButton { font-size: 14px; font-weight: bold; padding: 4px 18px; }"));

    layout->addWidget(m_orBrowser);
    layout->addWidget(m_checkboxPanel);
    layout->addWidget(m_beginButton, 0, Qt::AlignLeft);
    layout->addWidget(m_templateBrowser);
    layout->addWidget(m_formProtocolButton, 0, Qt::AlignLeft);
    layout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);

    m_previewImage = new QLabel(this);
    m_previewImage->setGeometry(1100, 75, 400, 400);
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setScaledContents(true);

    m_rightCountLabel = new QLabel(this);
    m_rightCountLabel->setGeometry(1100, 250, 300, 40);
    m_rightCountLabel->setStyleSheet(QStringLiteral("font: bold 17px Arial; color: #000000;"));
    m_rightCountLabel->hide();

    m_wrongCountLabel = new QLabel(this);
    m_wrongCountLabel->setGeometry(1100, 350, 300, 40);
    m_wrongCountLabel->setStyleSheet(QStringLiteral("font: bold 17px Arial; color: #000000;"));
    m_wrongCountLabel->hide();

    m_onlyP = new OnlyPExercise(this);
    m_onlyP->setGeometry(0, 0, 1920, 1080);
    m_onlyP->hide();

    m_patientDisplay = new PatientDisplay;

    connect(m_beginButton, &QPushButton::clicked, this, [this]() {
        if (!m_protocolFormed) {
            CustomMessageBox::showError(this, QStringLiteral("Сначала необходимо сформировать отчет"));
            return;
        }
        runOnlyPExercise();
    });
    connect(m_formProtocolButton, &QPushButton::clicked, this, [this]() { formProtocol(); });
    connect(m_onlyP, &OnlyPExercise::finished, this, [this](const QList<bool> &answers, int elapsedSeconds) {
        m_answers = answers;
        m_elapsedSeconds = elapsedSeconds;
        m_exerciseDone = true;
        m_protocolFormed = false;
        showResultLabels(answers, elapsedSeconds);
        if (m_patientDisplay) {
            m_patientDisplay->hideDisplay();
        }
    });
}

void ExerciseHost::openExercise(
    const QString &exerciseId,
    const QString &patientId,
    const QString &specialistFio,
    const QString &patientFio,
    const QString &patientBirthDate,
    Repository *repository,
    bool dualScreen) {
    m_exerciseId = exerciseId;
    m_patientId = patientId;
    m_specialistFio = specialistFio;
    m_patientFio = patientFio;
    m_patientBirthDate = patientBirthDate;
    m_repository = repository;
    m_dualScreen = dualScreen;
    m_exerciseDone = false;
    m_protocolFormed = true;
    m_partly = false;
    m_answers.clear();
    m_elapsedSeconds = 0;
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();

    for (QCheckBox *box : m_activityChecks) {
        box->setChecked(false);
    }
    for (QCheckBox *box : m_helpChecks) {
        box->setChecked(false);
    }

    if (exerciseId == QStringLiteral("1.2")) {
        loadStaticPictureExercise();
    }
    show();
    raise();
}

void ExerciseHost::loadStaticPictureExercise() {
    m_checkboxPanel->show();
    m_orBrowser->setHtml(loadExerciseHtml(m_exerciseId, QStringLiteral("or.html")));
    m_templateBrowser->setHtml(loadExerciseHtml(m_exerciseId, QStringLiteral("template.html")));

    const QString previewPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("f1.png"));
    if (!previewPath.isEmpty()) {
        m_previewImage->setPixmap(QPixmap(previewPath));
        m_previewImage->show();
    } else {
        m_previewImage->hide();
    }
    layoutContent();
}

void ExerciseHost::layoutContent() {
    QTimer::singleShot(0, this, [this]() { updateContentHeights(); });
}

void ExerciseHost::updateContentHeights() {
    if (!m_orBrowser || !m_templateBrowser) {
        return;
    }
    const int orHeight = m_orBrowser->document()->size().toSize().height() + 16;
    const int templateHeight = m_templateBrowser->document()->size().toSize().height() + 16;
    m_orBrowser->setMinimumHeight(orHeight);
    m_orBrowser->setMaximumHeight(orHeight);
    m_templateBrowser->setMinimumHeight(templateHeight);
    m_templateBrowser->setMaximumHeight(templateHeight);
    m_scrollContent->adjustSize();
}

void ExerciseHost::runOnlyPExercise() {
    m_protocolFormed = false;
    m_rightCountLabel->hide();
    m_wrongCountLabel->hide();
    m_onlyP->setGeometry(0, 0, width(), height());
    m_onlyP->start(m_exerciseId);
    m_onlyP->raise();

    if (m_dualScreen && m_patientDisplay) {
        m_patientDisplay->attachExercise(m_onlyP);
        m_patientDisplay->showOnSecondaryScreen();
    }
}

void ExerciseHost::showResultLabels(const QList<bool> &answers, int elapsedSeconds) {
    Q_UNUSED(elapsedSeconds);
    int right = 0;
    int wrong = 0;
    for (bool answer : answers) {
        if (answer) {
            ++right;
        } else {
            ++wrong;
        }
    }
    m_rightCountLabel->setText(QStringLiteral("Верно: %1").arg(right));
    m_wrongCountLabel->setText(QStringLiteral("Не верно: %1").arg(wrong));
    m_rightCountLabel->show();
    m_wrongCountLabel->show();
}

ExerciseProtocol::CheckboxValues ExerciseHost::checkboxValues() const {
    ExerciseProtocol::CheckboxValues values;
    for (QCheckBox *box : m_activityChecks) {
        if (box->isChecked()) {
            values.activity = box->text();
            break;
        }
    }
    QStringList helpValues;
    for (QCheckBox *box : m_helpChecks) {
        if (box->isChecked()) {
            helpValues << box->text();
        }
    }
    values.help = helpValues.join(QStringLiteral("; "));
    return values;
}

QString ExerciseHost::orHtmlSnapshot() const {
    return m_orBrowser ? m_orBrowser->toHtml() : QString();
}

void ExerciseHost::formProtocol() {
    if (!m_repository) {
        return;
    }
    if (!m_exerciseDone) {
        CustomMessageBox::showError(
            this, QStringLiteral("Формирование протокола невозможно без выполнения упражнения"));
        return;
    }

    const QString protocolBody = ExerciseProtocol::createProtocolHtml(
        m_exerciseId,
        m_specialistFio,
        m_elapsedSeconds,
        m_partly,
        m_templateBrowser->toHtml(),
        m_answers,
        checkboxValues());

    QString error;
    QString protocolId;
    if (!m_repository->saveExerciseProtocol(m_patientId, m_exerciseId, protocolBody, m_partly, &error, &protocolId)) {
        CustomMessageBox::showError(this, error);
        return;
    }

    const QString viewHtml = m_repository->loadProtocolViewHtml(
        m_exerciseId, protocolId, m_patientFio, m_patientBirthDate);
    m_templateBrowser->setHtml(viewHtml);
    layoutContent();

    m_protocolFormed = true;
    m_partly = true;
    emit protocolSaved();
}

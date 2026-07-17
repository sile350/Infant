#include "wolfrunner.h"

#include "exerciseassets.h"
#include "imagebutton.h"

#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QRegularExpression>
#include <QTextBrowser>
#include <QTimer>

namespace {

QString appendToHelpCell(const QString &html, int episode, const QString &text) {
    if (text.isEmpty()) {
        return html;
    }
    const QString id = QStringLiteral("h%1").arg(episode);
    const QRegularExpression re(
        QStringLiteral("(<div\\s+contenteditable=['\"]true['\"]\\s+id=['\"]%1['\"][^>]*>)").arg(id),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(html);
    if (!match.hasMatch()) {
        return html;
    }
    const int insertPos = match.capturedEnd(0);
    const QString addition =
        (episode <= 3 ? QStringLiteral("<br>&nbsp;&nbsp;&nbsp;") : QString()) + text + QLatin1Char(' ');
    return html.left(insertPos) + addition + html.mid(insertPos);
}

} // namespace

WolfRunner::WolfRunner(QWidget *parent) : ExerciseRunnerWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

    auto *stopButton = new ImageButton(this);
    m_stop = stopButton;
    stopButton->setImagePath(ExerciseAssets::sysImage(QStringLiteral("stop.png")));
    connect(stopButton, &ImageButton::clicked, this, [this]() { finishSession(); });

    auto *template1 = new ImageButton(this);
    m_templateBtn1 = template1;
    connect(template1, &ImageButton::clicked, this, [this]() { toggleTemplate1(); });

    auto *template2 = new ImageButton(this);
    m_templateBtn2 = template2;
    connect(template2, &ImageButton::clicked, this, [this]() { toggleTemplate2(); });

    m_taleImage = new QLabel(this);
    m_templateImage = new QLabel(this);
    m_tableBrowser = new QTextBrowser(this);
    m_episodeCombo = new QComboBox(this);
    m_helpCombo = new QComboBox(this);

    for (int i = 1; i <= 7; ++i) {
        m_episodeCombo->addItem(QString::number(i));
    }
    m_helpCombo->addItems({
        QStringLiteral("Наводящие вопросы."),
        QStringLiteral(
            "Карточки с изображением лиц, выражающих те или иные эмоции, с предложением указать "
            "эмоции, которые испытывает тот или иной герой."),
        QStringLiteral("Напоминает сюжета, с указанием на соответствующие сюжетные картинки."),
    });
    connect(m_helpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        appendHelpText();
    });
    connect(m_episodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        m_helpCombo->setCurrentIndex(-1);
    });
}

void WolfRunner::startSession(
    const QString &exerciseId,
    const ExerciseDefinition &definition,
    const QString &stepId) {
    Q_UNUSED(definition);
    Q_UNUSED(stepId);
    m_exerciseId = exerciseId;
    m_elapsed = 0;

    const QString tablePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("table.html"));
    if (!tablePath.isEmpty()) {
        QFile file(tablePath);
        if (file.open(QIODevice::ReadOnly)) {
            m_tableHtml = QString::fromUtf8(file.readAll());
            m_tableBrowser->setHtml(ExerciseAssets::prepareExerciseHtml(
                m_tableHtml, QFileInfo(tablePath).absolutePath()));
        }
    }

    const QString talePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("tale.png"));
    if (!talePath.isEmpty()) {
        m_taleImage->setPixmap(QPixmap(talePath));
    }

    if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
        btn1->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("shows.png")));
    }
    if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
        btn2->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("showe.png")));
    }

    toggleTemplate1();
    layoutUi();
    m_timer->start();
    show();
    raise();
}

void WolfRunner::toggleTemplate1() {
    const QString trafPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf1.png"));
    const QString showsPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png"));
    const QString hidePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png"));
    const QString showePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png"));

    if (!m_template1Visible) {
        if (!trafPath.isEmpty()) {
            m_templateImage->setPixmap(QPixmap(trafPath));
            m_templateImage->show();
        }
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(hidePath);
        }
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(showePath);
        }
        m_template2Visible = false;
        m_template1Visible = true;
    } else {
        m_templateImage->hide();
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(showsPath);
        }
        m_template1Visible = false;
    }
    layoutUi();
}

void WolfRunner::toggleTemplate2() {
    const QString trafPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("traf2.png"));
    const QString showsPath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("shows.png"));
    const QString hidePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("hidep.png"));
    const QString showePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("showe.png"));

    if (!m_template2Visible) {
        if (!trafPath.isEmpty()) {
            m_templateImage->setPixmap(QPixmap(trafPath));
            m_templateImage->show();
        }
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(hidePath);
        }
        if (auto *btn1 = qobject_cast<ImageButton *>(m_templateBtn1)) {
            btn1->setImagePath(showsPath);
        }
        m_template1Visible = false;
        m_template2Visible = true;
    } else {
        m_templateImage->hide();
        if (auto *btn2 = qobject_cast<ImageButton *>(m_templateBtn2)) {
            btn2->setImagePath(showePath);
        }
        m_template2Visible = false;
    }
    layoutUi();
}

void WolfRunner::appendHelpText() {
    const int episode = m_episodeCombo->currentText().toInt();
    const QString text = m_helpCombo->currentText();
    if (episode < 1 || episode > 7 || text.isEmpty()) {
        return;
    }
    m_tableHtml = appendToHelpCell(m_tableHtml, episode, text);
    const QString tablePath = ExerciseAssets::exerciseFile(m_exerciseId, QStringLiteral("table.html"));
    m_tableBrowser->setHtml(ExerciseAssets::prepareExerciseHtml(
        m_tableHtml, QFileInfo(tablePath).absolutePath()));
}

void WolfRunner::layoutUi() {
    m_stop->move(970, 70);
    m_templateBtn1->move(1204, 12);
    m_templateBtn2->move(1567, 12);
    m_taleImage->setGeometry(12, 24, 805, 248);
    m_episodeCombo->setGeometry(74, 278, 52, 24);
    m_helpCombo->setGeometry(313, 278, 467, 24);
    m_tableBrowser->setGeometry(12, 316, 805, 752);
    m_templateImage->setGeometry(964, 243, 616, 399);
    m_stop->raise();
    m_templateBtn1->raise();
    m_templateBtn2->raise();
}

void WolfRunner::finishSession() {
    ExerciseSessionResult result;
    result.elapsedSeconds = m_elapsed;
    // Как в exbegin: h1..h7 | p1..p7
    const QString html = m_tableBrowser ? m_tableBrowser->toHtml() : m_tableHtml;
    auto extract = [](const QString &source, const QString &id) -> QString {
        const QRegularExpression re(
            QStringLiteral("<div[^>]*\\bid=['\"]%1['\"][^>]*>([\\s\\S]*?)</div>")
                .arg(QRegularExpression::escape(id)),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = re.match(source);
        if (!match.hasMatch()) {
            return {};
        }
        QString inner = match.captured(1);
        inner.replace(QRegularExpression(QStringLiteral("<[^>]+>")), QString());
        return inner.trimmed();
    };
    QStringList helps;
    QStringList answers;
    for (int i = 1; i <= 7; ++i) {
        helps << extract(html, QStringLiteral("h%1").arg(i));
        answers << extract(html, QStringLiteral("p%1").arg(i));
    }
    // Fallback: значения из исходного HTML, куда пишутся подсказки.
    if (helps.join(QString()).isEmpty() && !m_tableHtml.isEmpty()) {
        helps.clear();
        answers.clear();
        for (int i = 1; i <= 7; ++i) {
            helps << extract(m_tableHtml, QStringLiteral("h%1").arg(i));
            answers << extract(m_tableHtml, QStringLiteral("p%1").arg(i));
        }
    }
    result.additional = helps.join(QLatin1Char(';')) + QLatin1Char('|') + answers.join(QLatin1Char(';'));
    m_timer->stop();
    hide();
    emitFinished(result);
}

void WolfRunner::stopSession() {
    finishSession();
}

void WolfRunner::resizeEvent(QResizeEvent *event) {
    ExerciseRunnerWidget::resizeEvent(event);
    layoutUi();
}

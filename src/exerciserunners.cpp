#include "exerciserunnerwidget.h"

#include "e126canvas.h"
#include "e15canvas.h"
#include "exerciseassets.h"
#include "onlypexercise.h"
#include "puzzlecanvas.h"
#include "puzzlelayout.h"
#include "remembercanvas.h"
#include "wolfrunner.h"

#include "imagebutton.h"

#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace {

QString resolveSessionImage(const QString &exerciseId, const QString &stepId) {
    const QStringList candidates = stepId.isEmpty()
        ? QStringList{
              QStringLiteral("f1.png"),
              QStringLiteral("traf1.png"),
              QStringLiteral("1.png"),
              QStringLiteral("p1.png"),
          }
        : QStringList{
              stepId + QStringLiteral(".png"),
              QStringLiteral("p") + stepId + QStringLiteral(".png"),
              QStringLiteral("f") + stepId + QStringLiteral(".png"),
              QStringLiteral("traf") + stepId + QStringLiteral(".png"),
              QStringLiteral("ex") + stepId + QStringLiteral(".png"),
          };
    for (const QString &name : candidates) {
        const QString path = ExerciseAssets::exerciseFile(exerciseId, name);
        if (!path.isEmpty()) {
            return path;
        }
    }
    return ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("f1.png"));
}

QString scansDirectory() {
    const QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/data/scans");
    QDir().mkpath(dir);
    return dir;
}

QPixmap pixmapNativeOrDownscale(const QPixmap &source, int maxW, int maxH) {
    if (source.isNull()) {
        return source;
    }
    if (source.width() <= maxW && source.height() <= maxH) {
        return source;
    }
    return source.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

struct CanvasLayout {
    QSize size = QSize(1856, 961);
    QPoint pos = QPoint(0, 140);
    QPoint trafPos;
    QString trafFile;
    QPoint traf2Pos;
    QString traf2File;
};

CanvasLayout paintCanvasLayout(const QString &exerciseId, const QString &stepId) {
    CanvasLayout layout;
    if (exerciseId == QStringLiteral("3.3.1")) {
        layout.size = QSize(1450, 770);
        layout.pos = QPoint(300, 200);
        layout.trafPos = QPoint(810, 0);
        layout.trafFile = QStringLiteral("traf1.png");
        layout.traf2Pos = QPoint(10, 0);
        layout.traf2File = QStringLiteral("traf2.png");
        return layout;
    }
    if (exerciseId == QStringLiteral("3.3.2")) {
        layout.size = QSize(1000, 620);
        layout.pos = QPoint(450, 200);
        layout.trafPos = QPoint(10, 10);
        layout.trafFile = QStringLiteral("traf1.png");
        return layout;
    }
    if (exerciseId == QStringLiteral("3.3.3")) {
        layout.size = QSize(1000, 620);
        layout.pos = QPoint(0, 140);
        layout.trafPos = QPoint(1100, 50);
        layout.trafFile = QStringLiteral("traf1.png");
        return layout;
    }
    if (exerciseId == QStringLiteral("1.7")) {
        layout.size = stepId == QStringLiteral("3") ? QSize(650, 800) : QSize(600, 800);
        layout.pos = QPoint(600, 150);
        layout.trafPos = QPoint(0, 0);
        if (!stepId.isEmpty()) {
            layout.trafFile = stepId + QStringLiteral(".png");
        }
        return layout;
    }
    if (exerciseId == QStringLiteral("1.12")) {
        layout.size = QSize(1900, 961);
        layout.pos = QPoint(0, 0);
        layout.trafPos = QPoint(1000, 50);
        layout.trafFile = QStringLiteral("traf.png");
        layout.traf2Pos = QPoint(200, 50);
        layout.traf2File = QStringLiteral("exampleno.png");
        return layout;
    }
    layout.trafFile = QStringLiteral("traf1.png");
    layout.trafPos = QPoint(0, 0);
    return layout;
}

CanvasLayout findMarkCanvasLayout(const QString &exerciseId, const QString &stepId) {
    CanvasLayout layout;
    layout.trafPos = QPoint(0, 0);
    if (exerciseId == QStringLiteral("2.3")) {
        layout.size = QSize(430, 344);
        layout.pos = QPoint(1000, 330);
        layout.trafFile = QStringLiteral("11.png");
        layout.traf2Pos = QPoint(220, 3);
        layout.traf2File = QStringLiteral("void.png");
        return layout;
    }
    if (exerciseId == QStringLiteral("2.2")) {
        layout.size = QSize(674, 799);
        layout.pos = QPoint(1000, 125);
        layout.trafFile = QStringLiteral("traf1.png");
        return layout;
    }
    if (exerciseId == QStringLiteral("2.1")) {
        if (stepId == QStringLiteral("3")) {
            layout.size = QSize(1290, 764);
            layout.pos = QPoint(300, 150);
            layout.trafFile = QStringLiteral("traf3.png");
        } else if (stepId == QStringLiteral("2")) {
            layout.size = QSize(641, 894);
            layout.pos = QPoint(1000, 130);
            layout.trafFile = QStringLiteral("traf2.png");
        } else {
            layout.size = QSize(641, 894);
            layout.pos = QPoint(1000, 130);
            layout.trafFile = QStringLiteral("traf1.png");
        }
        return layout;
    }
    layout.size = QSize(641, 894);
    layout.pos = QPoint(1000, 130);
    layout.trafFile = QStringLiteral("traf1.png");
    return layout;
}

int findMarkRedIntervalMs(const QString &exerciseId, const QString &stepId) {
    if (exerciseId == QStringLiteral("2.2")) {
        return 120000;
    }
    if (exerciseId == QStringLiteral("2.1")) {
        return stepId == QStringLiteral("1") ? 30000 : 60000;
    }
    return 30000;
}

void drawPixmapOnImage(QImage *image, const QString &exerciseId, const QString &file, const QPoint &pos) {
    if (!image || file.isEmpty()) {
        return;
    }
    const QString path = ExerciseAssets::exerciseFile(exerciseId, file);
    if (path.isEmpty()) {
        return;
    }
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        return;
    }
    QPainter painter(image);
    painter.drawPixmap(pos, pixmap);
}

class ClickableLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> onClick;

protected:
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && onClick) {
            onClick();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

class TimedSessionRunner : public ExerciseRunnerWidget {
public:
    explicit TimedSessionRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finish(); };

        m_picture = new QLabel(this);
        m_picture->setAlignment(Qt::AlignCenter);
        m_picture->setStyleSheet(QStringLiteral("background:white;"));
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_elapsed = 0;
        m_capturePath.clear();

        const QString imagePath = resolveSessionImage(exerciseId, stepId);
        if (!imagePath.isEmpty()) {
            m_pixmap = QPixmap(imagePath);
            m_picture->setPixmap(m_pixmap);
        }
        m_timer->start();
        show();
        raise();
        layoutUi();
    }

    void stopSession() override { finish(); }

protected:
    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        layoutUi();
    }

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        QWidget::paintEvent(event);
    }

    virtual void finish() {
        m_timer->stop();
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.picturesShown = 1;
        result.capturedImagePath = m_capturePath;
        hide();
        emitFinished(result);
    }

    virtual void layoutUi() {
        m_stop->move(80, 72);
        m_stop->raise();
        const int top = 140;
        const int margin = 20;
        const int maxW = qMax(100, width() - 2 * margin);
        const int maxH = qMax(100, height() - top - margin);
        if (!m_pixmap.isNull()) {
            const QPixmap display = pixmapNativeOrDownscale(m_pixmap, maxW, maxH);
            m_picture->setPixmap(display);
            m_picture->setFixedSize(display.size());
            m_picture->move((width() - display.width()) / 2, top);
        }
        m_picture->raise();
    }

    QTimer *m_timer = nullptr;
    ClickableLabel *m_stop = nullptr;
    QLabel *m_picture = nullptr;
    QPixmap m_pixmap;
    int m_elapsed = 0;
    QString m_capturePath;
};

class OnlyPictureRunner final : public ExerciseRunnerWidget {
public:
    explicit OnlyPictureRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_exercise = new OnlyPExercise(this);
        m_exercise->hide();
        connect(m_exercise, &OnlyPExercise::finished, this, [this](const QList<bool> &answers, int elapsed) {
            ExerciseSessionResult result;
            result.answers = answers;
            result.elapsedSeconds = elapsed;
            result.picturesShown = m_exercise ? m_exercise->picturesShown() : 0;
            emitFinished(result);
        });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_exercise->setDisplayRole(OnlyPExercise::DisplayRole::Primary);
        m_exercise->setGeometry(0, 0, width(), height());
        m_exercise->start(exerciseId, definition.onlyPicture, stepId);
        m_exercise->raise();
    }

    void stopSession() override { m_exercise->stopExercise(); }

    OnlyPExercise *exerciseWidget() const { return m_exercise; }

private:
    OnlyPExercise *m_exercise = nullptr;
};

class PaintRunner : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_elapsed = 0;
        m_capturePath.clear();
        m_pixmap = QPixmap();
        m_layout = paintCanvasLayout(exerciseId, stepId);
        m_canvas = QImage(m_layout.size, QImage::Format_RGB32);
        m_canvas.fill(QColor(0xf2, 0xf0, 0xf0));
        drawPixmapOnImage(&m_canvas, exerciseId, m_layout.trafFile, m_layout.trafPos);
        drawPixmapOnImage(&m_canvas, exerciseId, m_layout.traf2File, m_layout.traf2Pos);
        m_drawing = true;
        m_timer->start();
        show();
        raise();
        updateCanvasDisplay();
    }

protected:
    void layoutUi() override {
        if (exerciseId() == QStringLiteral("3.3.1") || exerciseId() == QStringLiteral("3.3.2")
            || exerciseId() == QStringLiteral("3.3.3")) {
            m_stop->move(970, 70);
        } else {
            m_stop->move(80, 72);
        }
        m_stop->raise();
        updateCanvasDisplay();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_drawing = true;
            m_hasLast = false;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_drawing = false;
            m_hasLast = false;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!m_drawing || !event->buttons().testFlag(Qt::LeftButton)) {
            return;
        }
        const QPoint canvasPt = mapToCanvas(event->pos());
        if (canvasPt.x() < 0) {
            return;
        }
        QPainter painter(&m_canvas);
        painter.setPen(QPen(Qt::blue, 20, Qt::SolidLine, Qt::RoundCap));
        if (m_hasLast) {
            painter.drawLine(m_lastPoint, canvasPt);
        }
        m_lastPoint = canvasPt;
        m_hasLast = true;
        updateCanvasDisplay();
    }

    void finish() override {
        const QString path = scansDirectory() + QStringLiteral("/") + m_exerciseId + QStringLiteral("-")
            + QString::number(QDateTime::currentMSecsSinceEpoch()) + QStringLiteral(".png");
        m_canvas.save(path);
        m_capturePath = path;
        TimedSessionRunner::finish();
    }

    QString exerciseId() const { return m_exerciseId; }

    QPoint mapToCanvas(const QPoint &widgetPos) const {
        if (!m_picture) {
            return QPoint(-1, -1);
        }
        const QPoint local = widgetPos - m_picture->pos();
        if (local.x() < 0 || local.y() < 0 || local.x() >= m_picture->width()
            || local.y() >= m_picture->height()) {
            return QPoint(-1, -1);
        }
        const double sx = m_canvas.width() > 0
            ? static_cast<double>(m_canvas.width()) / m_picture->width()
            : 1.0;
        const double sy = m_canvas.height() > 0
            ? static_cast<double>(m_canvas.height()) / m_picture->height()
            : 1.0;
        return QPoint(qRound(local.x() * sx), qRound(local.y() * sy));
    }

    void updateCanvasDisplay() {
        if (m_canvas.isNull() || !m_picture) {
            return;
        }
        m_picture->setPixmap(QPixmap::fromImage(m_canvas));
        m_picture->setFixedSize(m_canvas.size());
        m_picture->move(m_layout.pos);
        m_picture->raise();
    }

    QImage m_canvas;
    CanvasLayout m_layout;
    bool m_drawing = false;
    bool m_hasLast = false;
    QPoint m_lastPoint;
};

class FindMarkRunner final : public PaintRunner {
public:
    explicit FindMarkRunner(QWidget *parent = nullptr) : PaintRunner(parent) {
        m_slideshowTimer = new QTimer(this);
        m_slideshowTimer->setInterval(1000);
        connect(m_slideshowTimer, &QTimer::timeout, this, [this]() {
            if (m_dotime == 0) {
                redrawTemplate();
            }
            ++m_dotime;
        });

        m_redTimer = new QTimer(this);
        connect(m_redTimer, &QTimer::timeout, this, [this]() {
            if (m_redOverlay) {
                m_redOverlay->setGeometry(m_picture->geometry());
                m_redOverlay->show();
                m_redOverlay->raise();
            }
            m_redTimer->stop();
        });

        m_redOverlay = new ClickableLabel(this);
        m_redOverlay->hide();
        m_redOverlay->onClick = [this]() {
            if (m_exerciseId != QStringLiteral("2.1")) {
                return;
            }
            m_redOverlay->hide();
            if (m_continueButton) {
                m_continueButton->show();
                m_continueButton->raise();
            }
        };

        m_continueButton = new ClickableLabel(this);
        m_continueButton->hide();
        m_continueButton->onClick = [this]() {
            ++m_cycles;
            if (m_cycles >= 5) {
                finish();
                return;
            }
            m_redOverlay->hide();
            if (m_continueButton) {
                m_continueButton->hide();
            }
            m_brushColor = QColor(QStringLiteral("#ef47e3"));
            m_redTimer->start();
        };
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_elapsed = 0;
        m_capturePath.clear();
        m_dotime = 0;
        m_cycles = 0;
        m_drawing = false;
        m_hasLast = false;

        m_layout = findMarkCanvasLayout(exerciseId, stepId);
        m_canvas = QImage(m_layout.size, QImage::Format_RGB32);
        m_canvas.fill(Qt::white);

        m_brushColor = QColor(QStringLiteral("#ef47e3"));
        if (exerciseId == QStringLiteral("2.3")) {
            m_brushColor = QColor(QStringLiteral("#0000ff"));
        }

        const QString redPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("red1.png"));
        if (!redPath.isEmpty() && m_redOverlay) {
            m_redOverlay->setPixmap(QPixmap(redPath));
            m_redOverlay->setScaledContents(true);
        }
        const QString continuePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("continue.png"));
        if (!continuePath.isEmpty() && m_continueButton) {
            const QPixmap continuePixmap(continuePath);
            m_continueButton->setPixmap(continuePixmap);
            m_continueButton->setFixedSize(continuePixmap.size());
        }

        if (exerciseId == QStringLiteral("3.3.1") || exerciseId == QStringLiteral("3.3.2")
            || exerciseId == QStringLiteral("3.3.3")) {
            m_stop->move(970, 70);
        } else {
            m_stop->move(970, 70);
        }

        m_timer->start();
        m_slideshowTimer->start();
        if (exerciseId != QStringLiteral("2.3")) {
            m_redTimer->setInterval(findMarkRedIntervalMs(exerciseId, stepId));
            m_redTimer->start();
        }
        updateCanvasDisplay();
        show();
        raise();
        layoutUi();
    }

protected:
    void layoutUi() override {
        m_stop->raise();
        updateCanvasDisplay();
        if (m_continueButton && !m_continueButton->pixmap(Qt::ReturnByValue).isNull()) {
            m_continueButton->move(m_stop->x() + m_stop->width() + 24, m_stop->y());
        }
    }

    void redrawTemplate() {
        m_canvas.fill(Qt::white);
        drawPixmapOnImage(&m_canvas, m_exerciseId, m_layout.trafFile, m_layout.trafPos);
        drawPixmapOnImage(&m_canvas, m_exerciseId, m_layout.traf2File, m_layout.traf2Pos);
        updateCanvasDisplay();
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_drawing = true;
            m_hasLast = false;
            if (m_exerciseId == QStringLiteral("2.3")) {
                const QPoint canvasPt = mapToCanvas(event->pos());
                if (canvasPt.x() >= 0) {
                    QPainter painter(&m_canvas);
                    painter.setPen(QPen(m_brushColor, 20, Qt::SolidLine, Qt::RoundCap));
                    painter.drawPoint(canvasPt);
                    updateCanvasDisplay();
                }
            }
        }
        PaintRunner::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && m_exerciseId != QStringLiteral("2.3")) {
            m_drawing = false;
            m_hasLast = false;
        }
        if (event->button() == Qt::RightButton && m_exerciseId == QStringLiteral("2.3")) {
            const QPoint canvasPt = mapToCanvas(event->pos());
            if (canvasPt.x() >= 0) {
                QPainter painter(&m_canvas);
                painter.setPen(QPen(QColor(QStringLiteral("#f8f8f8")), 23, Qt::SolidLine, Qt::RoundCap));
                painter.drawPoint(canvasPt);
                updateCanvasDisplay();
            }
        }
        QWidget::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!m_drawing || !event->buttons().testFlag(Qt::LeftButton) || m_exerciseId == QStringLiteral("2.3")) {
            return;
        }
        const QPoint canvasPt = mapToCanvas(event->pos());
        if (canvasPt.x() < 0) {
            return;
        }
        QPainter painter(&m_canvas);
        painter.setPen(QPen(m_brushColor, 6, Qt::SolidLine, Qt::RoundCap));
        if (m_hasLast) {
            painter.drawLine(m_lastPoint, canvasPt);
        }
        m_lastPoint = canvasPt;
        m_hasLast = true;
        updateCanvasDisplay();
    }

    void finish() override {
        m_slideshowTimer->stop();
        m_redTimer->stop();
        PaintRunner::finish();
    }

    QTimer *m_slideshowTimer = nullptr;
    QTimer *m_redTimer = nullptr;
    ClickableLabel *m_redOverlay = nullptr;
    ClickableLabel *m_continueButton = nullptr;
    int m_dotime = 0;
    int m_cycles = 0;
    QColor m_brushColor = Qt::magenta;
};

class Remember2Runner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        TimedSessionRunner::startSession(exerciseId, definition, stepId);
        m_showA = new ClickableLabel(this);
        m_showB = new ClickableLabel(this);
        const QString hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
        const QString showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
        if (!hidePath.isEmpty()) {
            m_showA->setPixmap(QPixmap(hidePath));
            m_showB->setPixmap(QPixmap(hidePath));
        }
        m_showA->onClick = [this, showPath, hidePath]() { toggleCard(m_cardA, m_showA, showPath, hidePath); };
        m_showB->onClick = [this, showPath, hidePath]() { toggleCard(m_cardB, m_showB, showPath, hidePath); };
        m_cardA = new QLabel(this);
        m_cardB = new QLabel(this);
        const QString aPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("a.png"));
        const QString bPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("b.png"));
        if (!aPath.isEmpty()) {
            m_cardA->setPixmap(QPixmap(aPath));
        }
        if (!bPath.isEmpty()) {
            m_cardB->setPixmap(QPixmap(bPath));
        }
        m_cardA->hide();
        m_cardB->hide();
        layoutRemember2();
    }

    void layoutRemember2() {
        m_showA->move(200, 500);
        m_showB->move(400, 500);
        m_cardA->move(220, 200);
        m_cardB->move(420, 200);
    }

    void toggleCard(QLabel *card, ClickableLabel *button, const QString &showPath, const QString &hidePath) {
        if (card->isVisible()) {
            card->hide();
            if (!hidePath.isEmpty()) {
                button->setPixmap(QPixmap(hidePath));
            }
        } else {
            card->show();
            if (!showPath.isEmpty()) {
                button->setPixmap(QPixmap(showPath));
            }
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        layoutRemember2();
    }

    ClickableLabel *m_showA = nullptr;
    ClickableLabel *m_showB = nullptr;
    QLabel *m_cardA = nullptr;
    QLabel *m_cardB = nullptr;
};

class E28Runner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        TimedSessionRunner::startSession(exerciseId, definition, stepId);
        m_reference = new QLabel(this);
        m_task = new QLabel(this);
        const QString refPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.png"));
        if (!refPath.isEmpty()) {
            const QPixmap refPixmap(refPath);
            m_reference->setPixmap(refPixmap);
            m_reference->setFixedSize(refPixmap.size());
        }
        m_next = new ClickableLabel(this);
        const QString nextPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("next.png"));
        if (!nextPath.isEmpty()) {
            m_next->setPixmap(QPixmap(nextPath));
        }
        m_next->onClick = [this, exerciseId]() {
            const QString taskPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("2.png"));
            if (!taskPath.isEmpty()) {
                const QPixmap taskPixmap(taskPath);
                m_task->setPixmap(taskPixmap);
                m_task->setFixedSize(taskPixmap.size());
            }
            m_reference->hide();
            m_next->hide();
        };
        m_toggle = new ClickableLabel(this);
        const QString showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
        if (!showPath.isEmpty()) {
            m_toggle->setPixmap(QPixmap(showPath));
        }
        m_toggle->onClick = [this, exerciseId, showPath]() {
            if (m_reference->isVisible()) {
                m_reference->hide();
                const QString hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
                if (!hidePath.isEmpty()) {
                    m_toggle->setPixmap(QPixmap(hidePath));
                }
            } else {
                m_reference->show();
                if (!showPath.isEmpty()) {
                    m_toggle->setPixmap(QPixmap(showPath));
                }
            }
        };
        layoutE28();
    }

    void layoutE28() {
        m_reference->move(900, 200);
        m_task->move(500, 200);
        m_next->move(250, 72);
        m_toggle->move(450, 72);
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        layoutE28();
    }

    QLabel *m_reference = nullptr;
    QLabel *m_task = nullptr;
    ClickableLabel *m_next = nullptr;
    ClickableLabel *m_toggle = nullptr;
};

class DigitsRunner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_elapsed = 0;
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(120, 120, 120, 120);
        auto *title = new QLabel(QStringLiteral("Выберите цифры и нажмите «Стоп»"), this);
        layout->addWidget(title);
        auto *row1 = new QWidget(this);
        auto *row1Layout = new QVBoxLayout(row1);
        for (int i = 1; i <= 8; ++i) {
            auto *radio = new QRadioButton(QString::number(i), row1);
            m_row1 << radio;
            row1Layout->addWidget(radio);
        }
        layout->addWidget(row1);
        auto *row2 = new QWidget(this);
        auto *row2Layout = new QVBoxLayout(row2);
        for (int i = 8; i >= 1; --i) {
            auto *radio = new QRadioButton(QString::number(i), row2);
            m_row2 << radio;
            row2Layout->addWidget(radio);
        }
        layout->addWidget(row2);
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
    }

    void finish() override {
        QString first;
        QString second;
        for (QRadioButton *radio : m_row1) {
            if (radio->isChecked()) {
                first = radio->text();
                break;
            }
        }
        for (QRadioButton *radio : m_row2) {
            if (radio->isChecked()) {
                second = radio->text();
                break;
            }
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        if (!first.isEmpty() && !second.isEmpty()) {
            result.additional = first + QLatin1Char('/') + second;
        }
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    QList<QRadioButton *> m_row1;
    QList<QRadioButton *> m_row2;
};

class HtmlTableRunner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_elapsed = 0;
        m_browser = new QTextBrowser(this);
        const QString tablePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("table.html"));
        if (tablePath.isEmpty()) {
            const QString table2 = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("table2.html"));
            if (!table2.isEmpty()) {
                QFile file(table2);
                if (file.open(QIODevice::ReadOnly)) {
                    m_browser->setHtml(QString::fromUtf8(file.readAll()));
                }
            }
        } else {
            QFile file(tablePath);
            if (file.open(QIODevice::ReadOnly)) {
                m_browser->setHtml(QString::fromUtf8(file.readAll()));
            }
        }
        m_browser->setGeometry(100, 120, width() - 200, height() - 200);
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
    }

    void finish() override {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        if (m_browser) {
            result.additional = m_browser->toPlainText().left(4000);
        }
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        if (m_browser) {
            m_browser->setGeometry(100, 120, qMax(200, width() - 200), qMax(200, height() - 200));
        }
    }

    QTextBrowser *m_browser = nullptr;
};

class PuzzlesRunner : public ExerciseRunnerWidget {
public:
    explicit PuzzlesRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new PuzzleCanvas(this);
        m_canvas->hide();

        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        m_stop->hide();

        connect(m_canvas, &PuzzleCanvas::stopRequested, this, [this]() { finishSession(); });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;

        PuzzleLayout layout;
        if (!loadPuzzleLayout(exerciseId, stepId, &layout)) {
            layout.templateFile = QStringLiteral("f1.png");
            layout.templateX = 500;
            layout.templateY = 70;
        }

        const QStringList rotateExercises = {
            QStringLiteral("1.19"), QStringLiteral("1.20"), QStringLiteral("1.21"),
            QStringLiteral("1.22"), QStringLiteral("3.1.8"), QStringLiteral("3.1.16")};
        if (rotateExercises.contains(exerciseId)) {
            layout.rotateAllowed = true;
        }

        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->loadExercise(exerciseId, stepId, layout);
        m_canvas->applySessionOptions(m_sessionOptions);
        m_canvas->show();
        m_canvas->raise();
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        show();
        raise();
        setFocus();
    }

    void stopSession() override { finishSession(); }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_stop) {
            m_stop->move(80, 72);
            m_stop->raise();
        }
    }

private:
    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        result.additional = m_canvas ? m_canvas->positionsSnapshot() : QString();
        m_canvas->hide();
        m_stop->hide();
        hide();
        emitFinished(result);
    }

    PuzzleCanvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
};

class FlipCardCanvas final : public QWidget {
public:
    explicit FlipCardCanvas(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_timer.setInterval(1000);
        connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });
    }

    struct Card {
        QPixmap front;
        QPixmap back;
        int x = 0;
        int y = 0;
        bool closed = true;
        bool clickable = true;
    };

    int elapsedSeconds() const { return m_elapsed; }

    void loadCards(const QString &exerciseId, const QVector<Card> &cards) {
        m_exerciseId = exerciseId;
        m_cards = cards;
        m_elapsed = 0;
        m_timer.start();
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        const double scale = qMin(
            1.0,
            qMin(width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0,
                 height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0));
        const int offsetX = (width() - qRound(1920 * scale)) / 2;
        const int offsetY = (height() - qRound(1080 * scale)) / 2;
        for (const Card &card : m_cards) {
            const QPixmap &pixmap = card.closed ? card.back : card.front;
            if (pixmap.isNull()) {
                continue;
            }
            painter.drawPixmap(
                offsetX + qRound(card.x * scale),
                offsetY + qRound(card.y * scale),
                qRound(pixmap.width() * scale),
                qRound(pixmap.height() * scale),
                pixmap);
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        const double scale = qMin(
            1.0,
            qMin(width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0,
                 height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0));
        const int offsetX = (width() - qRound(1920 * scale)) / 2;
        const int offsetY = (height() - qRound(1080 * scale)) / 2;
        const int designX = qRound((event->x() - offsetX) / scale);
        const int designY = qRound((event->y() - offsetY) / scale);

        for (Card &card : m_cards) {
            if (!card.clickable) {
                continue;
            }
            const QPixmap &pixmap = card.closed ? card.back : card.front;
            if (designX >= card.x && designY >= card.y && designX < card.x + pixmap.width()
                && designY < card.y + pixmap.height()) {
                card.closed = !card.closed;
                update();
                return;
            }
        }
    }

private:
    QString m_exerciseId;
    QVector<Card> m_cards;
    QTimer m_timer;
    int m_elapsed = 0;
};

class CardsRunner final : public ExerciseRunnerWidget {
public:
    explicit CardsRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new FlipCardCanvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;

        QVector<FlipCardCanvas::Card> cards;
        const QString zeroPath = ExerciseAssets::exerciseFile(
            exerciseId,
            exerciseId == QStringLiteral("4.1.6") ? QStringLiteral("zero.png")
                                                  : QStringLiteral("zero.png"));
        QPixmap backPixmap(zeroPath);

        if (exerciseId == QStringLiteral("4.1.5")) {
            FlipCardCanvas::Card card;
            card.front = QPixmap(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.png")));
            card.back = backPixmap;
            card.x = 50;
            card.y = 81;
            cards.append(card);
        } else if (exerciseId == QStringLiteral("4.1.6")) {
            int count = 1;
            int linex = 910;
            int liney = 100;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 4; ++col) {
                    FlipCardCanvas::Card card;
                    card.front = QPixmap(ExerciseAssets::exerciseFile(
                        exerciseId, QString::number(count) + QStringLiteral(".png")));
                    card.back = backPixmap;
                    card.x = linex;
                    card.y = liney;
                    cards.append(card);
                    ++count;
                    linex += 250;
                }
                linex = 910;
                liney += 250;
            }
        } else if (exerciseId == QStringLiteral("4.1.8")) {
            if (!m_tableBrowser) {
                m_tableBrowser = new QTextBrowser(this);
                m_tableBrowser->hide();
            }
            const QString tablePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("table.html"));
            if (!tablePath.isEmpty()) {
                QFile file(tablePath);
                if (file.open(QIODevice::ReadOnly)) {
                    m_tableBrowser->setHtml(QString::fromUtf8(file.readAll()));
                }
            }
            m_tableBrowser->setGeometry(0, 0, qMax(400, width() / 2), height());
            m_tableBrowser->show();

            if (!m_hideButton) {
                m_hideButton = new ImageButton(this);
                m_hideButton->hide();
                connect(m_hideButton, &ImageButton::clicked, this, [this]() {
                    if (m_canvas) {
                        m_canvas->setVisible(!m_canvas->isVisible());
                    }
                });
            }
            m_hideButton->setImagePath(ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png")));
            m_hideButton->move(900, 72);
            m_hideButton->show();

            int count = 1;
            int linex = 1000;
            int liney = 20;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    if (row == 3 && col == 3) {
                        break;
                    }
                    FlipCardCanvas::Card card;
                    card.front = QPixmap(ExerciseAssets::exerciseFile(
                        exerciseId, QString::number(count) + QStringLiteral(".png")));
                    card.back = backPixmap;
                    card.x = linex;
                    card.y = liney;
                    card.clickable = false;
                    cards.append(card);
                    ++count;
                    linex += 220;
                }
                linex = 1000;
                liney += 210;
            }
        }

        if (exerciseId == QStringLiteral("4.1.8")) {
            m_canvas->setGeometry(width() / 2, 0, width() - width() / 2, height());
        } else {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        m_canvas->loadCards(exerciseId, cards);
        m_canvas->show();
        m_canvas->raise();
        if (m_tableBrowser && exerciseId == QStringLiteral("4.1.8")) {
            m_tableBrowser->raise();
        }
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        show();
        raise();
    }

    void stopSession() override { finishSession(); }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            if (m_exerciseId == QStringLiteral("4.1.8")) {
                m_canvas->setGeometry(width() / 2, 0, width() - width() / 2, height());
                if (m_tableBrowser) {
                    m_tableBrowser->setGeometry(0, 0, width() / 2, height());
                }
            } else {
                m_canvas->setGeometry(0, 0, width(), height());
            }
        }
        if (m_stop) {
            m_stop->move(80, 72);
            m_stop->raise();
        }
    }

private:
    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        if (m_tableBrowser && m_exerciseId == QStringLiteral("4.1.8")) {
            result.additional = m_tableBrowser->toPlainText().left(4000);
        }
        m_canvas->hide();
        if (m_tableBrowser) {
            m_tableBrowser->hide();
        }
        if (m_hideButton) {
            m_hideButton->hide();
        }
        m_stop->hide();
        hide();
        emitFinished(result);
    }

    FlipCardCanvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
    QTextBrowser *m_tableBrowser = nullptr;
    ImageButton *m_hideButton = nullptr;
};

class E15Runner final : public ExerciseRunnerWidget {
public:
    explicit E15Runner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new E15Canvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        connect(m_canvas, &E15Canvas::stopRequested, this, [this]() { finishSession(); });
        connect(m_canvas, &E15Canvas::exerciseCompleted, this, [this]() { finishSession(); });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->startExercise(exerciseId, m_sessionOptions.e15SelectMode);
        m_canvas->show();
        m_canvas->raise();
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        show();
        raise();
    }

    void stopSession() override { finishSession(); }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_stop) {
            m_stop->move(80, 72);
        }
    }

private:
    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        result.doneState = m_canvas ? m_canvas->doneState() : QString();
        m_canvas->hide();
        m_stop->hide();
        hide();
        emitFinished(result);
    }

    E15Canvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
};

class RememberRunner final : public ExerciseRunnerWidget {
public:
    explicit RememberRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new RememberCanvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        m_removek = new ClickableLabel(this);
        m_removek->hide();
        m_removek->onClick = [this]() {
            if (m_canvas) {
                m_canvas->advanceRemovePhase();
                syncRemoveButton();
            }
        };
        connect(m_canvas, &RememberCanvas::stopRequested, this, [this]() { finishSession(); });
        connect(m_canvas, &RememberCanvas::removeButtonChanged, this, [this]() { syncRemoveButton(); });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->startExercise(exerciseId, stepId);
        m_canvas->show();
        m_canvas->raise();
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        syncRemoveButton();
        show();
        raise();
    }

    void stopSession() override { finishSession(); }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_stop) {
            m_stop->move(80, 72);
        }
        if (m_removek && m_removek->isVisible()) {
            m_removek->move(m_stop ? m_stop->x() + m_stop->width() + 20 : 280, m_stop ? m_stop->y() : 72);
        }
    }

private:
    void syncRemoveButton() {
        if (!m_canvas || !m_removek) {
            return;
        }
        if (!m_canvas->removeButtonVisible()) {
            m_removek->hide();
            return;
        }
        const QString imageFile = m_canvas->removeButtonImage();
        const QString path = ExerciseAssets::exerciseFile(QStringLiteral("4.1.7"), imageFile);
        if (!path.isEmpty()) {
            const QPixmap pixmap(path);
            m_removek->setPixmap(pixmap);
            m_removek->setFixedSize(pixmap.size());
        }
        m_removek->move(m_stop ? m_stop->x() + m_stop->width() + 20 : 280, m_stop ? m_stop->y() : 72);
        m_removek->show();
        m_removek->raise();
    }

    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        result.additional = m_canvas ? m_canvas->positionsSnapshot() : QString();
        m_canvas->hide();
        m_stop->hide();
        if (m_removek) {
            m_removek->hide();
        }
        hide();
        emitFinished(result);
    }

    RememberCanvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
    ClickableLabel *m_removek = nullptr;
};

class EmotionsRunner final : public ExerciseRunnerWidget {
public:
    explicit EmotionsRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new E126Canvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        connect(m_canvas, &E126Canvas::stopRequested, this, [this]() { finishSession(); });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->startExercise(exerciseId, stepId);
        m_canvas->show();
        m_canvas->raise();
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        show();
        raise();
    }

    void stopSession() override { finishSession(); }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_stop) {
            m_stop->move(80, 72);
        }
    }

private:
    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        result.additional = m_canvas ? m_canvas->answersSnapshot() : QString();
        m_canvas->hide();
        m_stop->hide();
        hide();
        emitFinished(result);
    }

    E126Canvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
};

class OnlyDemoRunner final : public ExerciseRunnerWidget {
public:
    explicit OnlyDemoRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_demo = new TimedSessionRunner(this);
        m_demo->hide();
        m_puzzles = new PuzzlesRunner(this);
        m_puzzles->hide();
        connect(m_demo, &ExerciseRunnerWidget::sessionFinished, this, &ExerciseRunnerWidget::sessionFinished);
        connect(m_puzzles, &ExerciseRunnerWidget::sessionFinished, this, &ExerciseRunnerWidget::sessionFinished);
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        m_exerciseId = exerciseId;
        m_stepId = stepId;
        if (stepId == QStringLiteral("2")) {
            m_demo->hide();
            m_puzzles->setGeometry(0, 0, width(), height());
            m_puzzles->startSession(exerciseId, definition, stepId);
            m_puzzles->show();
            m_puzzles->raise();
            show();
            raise();
            return;
        }
        m_puzzles->hide();
        m_demo->setGeometry(0, 0, width(), height());
        m_demo->startSession(exerciseId, definition, stepId);
        m_demo->show();
        m_demo->raise();
        show();
        raise();
    }

    void stopSession() override {
        if (m_puzzles->isVisible()) {
            m_puzzles->stopSession();
        } else {
            m_demo->stopSession();
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        const QRect area(0, 0, width(), height());
        m_demo->setGeometry(area);
        m_puzzles->setGeometry(area);
    }

    TimedSessionRunner *m_demo = nullptr;
    PuzzlesRunner *m_puzzles = nullptr;
};

} // namespace

ExerciseRunnerWidget::ExerciseRunnerWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void ExerciseRunnerWidget::emitFinished(const ExerciseSessionResult &result) {
    emit sessionFinished(result);
}

ExerciseRunnerWidget *createExerciseRunner(ExerciseRunnerKind kind, QWidget *parent) {
    switch (kind) {
    case ExerciseRunnerKind::OnlyPicture:
        return new OnlyPictureRunner(parent);
    case ExerciseRunnerKind::Paint:
        return new PaintRunner(parent);
    case ExerciseRunnerKind::FindMark:
        return new FindMarkRunner(parent);
    case ExerciseRunnerKind::Remember2:
        return new Remember2Runner(parent);
    case ExerciseRunnerKind::E28:
        return new E28Runner(parent);
    case ExerciseRunnerKind::Digits:
        return new DigitsRunner(parent);
    case ExerciseRunnerKind::E511:
    case ExerciseRunnerKind::E521:
    case ExerciseRunnerKind::WordsLearning:
        return new HtmlTableRunner(parent);
    case ExerciseRunnerKind::Wolf:
        return new WolfRunner(parent);
    case ExerciseRunnerKind::Puzzles:
        return new PuzzlesRunner(parent);
    case ExerciseRunnerKind::Cards:
        return new CardsRunner(parent);
    case ExerciseRunnerKind::Remember:
        return new RememberRunner(parent);
    case ExerciseRunnerKind::E15:
        return new E15Runner(parent);
    case ExerciseRunnerKind::E126:
    case ExerciseRunnerKind::E1272:
        return new EmotionsRunner(parent);
    case ExerciseRunnerKind::OnlyDemo:
        return new OnlyDemoRunner(parent);
    default:
        return new TimedSessionRunner(parent);
    }
}

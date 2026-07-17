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

#include <QButtonGroup>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
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
        // Как в paint.Designer: pictureBox 1856×961 @ (52,107); traf1 @ (1100,50)
        layout.size = QSize(1856, 961);
        layout.pos = QPoint(52, 107);
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
        if (exerciseId == QStringLiteral("3.3.3")) {
            m_brushColor = QColor(QStringLiteral("#176ee3"));
            m_brushWidth = 5;
        } else {
            m_brushColor = Qt::blue;
            m_brushWidth = 20;
        }
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
        painter.setPen(QPen(m_brushColor, m_brushWidth, Qt::SolidLine, Qt::RoundCap));
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
        // Не вылезаем за экран: при необходимости уменьшаем отображение, сохраняя hit-test через mapToCanvas.
        QPixmap full = QPixmap::fromImage(m_canvas);
        const int maxW = qMax(100, width() - m_layout.pos.x() - 20);
        const int maxH = qMax(100, height() - m_layout.pos.y() - 20);
        QPixmap display = full;
        if (display.width() > maxW || display.height() > maxH) {
            display = full.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_picture->setPixmap(display);
        m_picture->setFixedSize(display.size());
        m_picture->move(m_layout.pos);
        m_picture->raise();
    }

    QImage m_canvas;
    CanvasLayout m_layout;
    bool m_drawing = false;
    bool m_hasLast = false;
    QPoint m_lastPoint;
    QColor m_brushColor = Qt::blue;
    int m_brushWidth = 20;
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
        if (m_picture) {
            m_picture->hide();
        }
        m_showA = new ClickableLabel(this);
        m_showB = new ClickableLabel(this);
        const QString hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
        const QString showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
        if (!hidePath.isEmpty()) {
            m_showA->setPixmap(QPixmap(hidePath));
        }
        if (!showPath.isEmpty()) {
            m_showB->setPixmap(QPixmap(showPath));
        }
        m_showA->onClick = [this, showPath, hidePath]() { toggleCard(m_cardA, m_showA, showPath, hidePath); };
        m_showB->onClick = [this, showPath, hidePath]() { toggleCard(m_cardB, m_showB, showPath, hidePath); };
        m_cardA = new QLabel(this);
        m_cardB = new QLabel(this);
        const QString aPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("a.png"));
        const QString bPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("b.png"));
        if (!aPath.isEmpty()) {
            const QPixmap aPixmap(aPath);
            m_cardA->setPixmap(aPixmap);
            m_cardA->setFixedSize(aPixmap.size());
        }
        if (!bPath.isEmpty()) {
            const QPixmap bPixmap(bPath);
            m_cardB->setPixmap(bPixmap);
            m_cardB->setFixedSize(bPixmap.size());
        }
        // В оригинале картинка А видна сразу, Б скрыта.
        m_cardA->show();
        m_cardB->hide();
        layoutRemember2();
    }

    void layoutRemember2() {
        // remember2.Designer: кнопки 1120/1380@120, карточки 708/1284@174, stop 970@70
        if (m_stop) {
            m_stop->move(970, 70);
            m_stop->show();
            m_stop->raise();
        }
        if (m_showA) {
            m_showA->move(1120, 120);
            m_showA->show();
            m_showA->raise();
        }
        if (m_showB) {
            m_showB->move(1380, 120);
            m_showB->show();
            m_showB->raise();
        }
        if (m_cardA) {
            m_cardA->move(708, 174);
        }
        if (m_cardB) {
            m_cardB->move(1284, 174);
        }
    }

    void toggleCard(QLabel *card, ClickableLabel *button, const QString &showPath, const QString &hidePath) {
        Q_UNUSED(button);
        // Как в remember2.cs: кнопки переключают видимость А/Б взаимно.
        if (card == m_cardA) {
            if (m_cardA->isVisible()) {
                m_cardA->hide();
                if (!showPath.isEmpty()) {
                    m_showA->setPixmap(QPixmap(showPath));
                }
            } else {
                m_cardA->show();
                m_cardB->hide();
                if (!hidePath.isEmpty()) {
                    m_showA->setPixmap(QPixmap(hidePath));
                }
                if (!showPath.isEmpty()) {
                    m_showB->setPixmap(QPixmap(showPath));
                }
            }
            return;
        }
        if (card == m_cardB) {
            if (m_cardB->isVisible()) {
                m_cardB->hide();
                m_cardA->show();
                if (!showPath.isEmpty()) {
                    m_showB->setPixmap(QPixmap(showPath));
                }
                if (!hidePath.isEmpty()) {
                    m_showA->setPixmap(QPixmap(hidePath));
                }
            } else {
                m_cardB->show();
                m_cardA->hide();
                if (!hidePath.isEmpty()) {
                    m_showB->setPixmap(QPixmap(hidePath));
                }
                if (!showPath.isEmpty()) {
                    m_showA->setPixmap(QPixmap(showPath));
                }
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
    explicit E28Runner(QWidget *parent = nullptr) : TimedSessionRunner(parent) {
        m_reference = new QLabel(this);
        m_task = new QLabel(this);
        m_next = new ClickableLabel(this);
        m_toggle = new ClickableLabel(this);
        m_reference->hide();
        m_task->hide();
        m_next->hide();
        m_toggle->hide();
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
        m_pixmap = QPixmap();
        if (m_picture) {
            m_picture->hide();
        }

        QString refPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.png"));
        if (refPath.isEmpty()) {
            refPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.PNG"));
        }
        if (!refPath.isEmpty()) {
            const QPixmap refPixmap(refPath);
            m_reference->setPixmap(refPixmap);
            m_reference->setFixedSize(refPixmap.size());
            m_reference->show();
        } else {
            m_reference->hide();
        }
        m_task->clear();
        m_task->hide();

        const QString nextPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("next.png"));
        if (!nextPath.isEmpty()) {
            const QPixmap nextPixmap(nextPath);
            m_next->setPixmap(nextPixmap);
            m_next->setFixedSize(nextPixmap.size());
            m_next->show();
        }
        m_next->onClick = [this, exerciseId]() {
            QString taskPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("2.png"));
            if (taskPath.isEmpty()) {
                taskPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("2.PNG"));
            }
            if (!taskPath.isEmpty()) {
                const QPixmap taskPixmap(taskPath);
                m_task->setPixmap(taskPixmap);
                m_task->setFixedSize(taskPixmap.size());
                m_task->show();
            }
            m_reference->hide();
            m_next->hide();
            const QString showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
            if (!showPath.isEmpty()) {
                const QPixmap showPixmap(showPath);
                m_toggle->setPixmap(showPixmap);
                m_toggle->setFixedSize(showPixmap.size());
                m_toggle->show();
            }
            layoutE28();
        };

        m_toggle->hide();
        m_toggle->onClick = [this, exerciseId]() {
            // Как в оригинале e28: Image==null → показать подсказку + hide.png; иначе скрыть + show.png.
            if (!m_reference->isVisible() || m_reference->pixmap(Qt::ReturnByValue).isNull()) {
                m_reference->show();
                const QString hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
                if (!hidePath.isEmpty()) {
                    const QPixmap hidePixmap(hidePath);
                    m_toggle->setPixmap(hidePixmap);
                    m_toggle->setFixedSize(hidePixmap.size());
                }
            } else {
                m_reference->hide();
                const QString showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
                if (!showPath.isEmpty()) {
                    const QPixmap showPixmap(showPath);
                    m_toggle->setPixmap(showPixmap);
                    m_toggle->setFixedSize(showPixmap.size());
                }
            }
            layoutE28();
        };

        m_timer->start();
        show();
        raise();
        layoutE28();
        m_stop->move(970, 70);
        m_stop->show();
        m_stop->raise();
    }

    void layoutUi() override {
        m_stop->move(970, 70);
        m_stop->raise();
        layoutE28();
    }

    void layoutE28() {
        // Оригинал e28.Designer: pictureBox1 (задание/2.png) = (228,246),
        // pictureBox2 (подсказка/1.png) = (1043,246) — иначе 2.png перекрывает подсказку.
        if (m_reference) {
            m_reference->move(1043, 246);
            m_reference->raise();
        }
        if (m_task) {
            m_task->move(228, 246);
            m_task->raise();
        }
        if (m_next) {
            m_next->move(m_stop->x() + m_stop->width() + 24, 70);
            m_next->raise();
        }
        if (m_toggle) {
            m_toggle->move(m_stop->x() + m_stop->width() + 24, 70);
            m_toggle->raise();
        }
        // Подсказка поверх задания, когда обе видны.
        if (m_reference && m_reference->isVisible()) {
            m_reference->raise();
        }
        m_stop->raise();
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
        m_row1.clear();
        m_row2.clear();

        // Как в digits.Designer: две группы рядов цифр.
        static const QStringList kLeft = {
            QStringLiteral("9"),
            QStringLiteral("2 4"),
            QStringLiteral("3 8 6"),
            QStringLiteral("1 5 8 5"),
            QStringLiteral("4 6 2 3 9"),
            QStringLiteral("4 8 9 1 7 3"),
            QStringLiteral("5 1 7 4 2 3 8"),
            QStringLiteral("1 4 2 5 9 7 6 3"),
        };
        static const QStringList kRight = {
            QStringLiteral("4 9 1 6 3 2 5 8"),
            QStringLiteral("8 5 9 2 3 4 6"),
            QStringLiteral("1 6 5 2 9 8"),
            QStringLiteral("4 1 3 7 2"),
            QStringLiteral("9 2 6 5"),
            QStringLiteral("4 1 7"),
            QStringLiteral("2 5"),
            QStringLiteral("3"),
        };

        auto *host = new QWidget(this);
        host->setGeometry(200, 140, 1200, 800);
        auto *layout = new QHBoxLayout(host);
        layout->setSpacing(80);

        auto *group1 = new QGroupBox(QStringLiteral("Ряд 1"), host);
        auto *row1Layout = new QVBoxLayout(group1);
        auto *group1Exclusive = new QButtonGroup(this);
        for (int i = 0; i < kLeft.size(); ++i) {
            auto *radio = new QRadioButton(kLeft.at(i), group1);
            radio->setProperty("digitValue", i + 1);
            group1Exclusive->addButton(radio);
            m_row1 << radio;
            row1Layout->addWidget(radio);
        }
        layout->addWidget(group1);

        auto *group2 = new QGroupBox(QStringLiteral("Ряд 2"), host);
        auto *row2Layout = new QVBoxLayout(group2);
        auto *group2Exclusive = new QButtonGroup(this);
        for (int i = 0; i < kRight.size(); ++i) {
            auto *radio = new QRadioButton(kRight.at(i), group2);
            // В оригинале radio16=8 … radio9=1
            radio->setProperty("digitValue", 8 - i);
            group2Exclusive->addButton(radio);
            m_row2 << radio;
            row2Layout->addWidget(radio);
        }
        layout->addWidget(group2);

        m_host = host;
        m_stop->move(970, 70);
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
                first = radio->property("digitValue").toString();
                break;
            }
        }
        for (QRadioButton *radio : m_row2) {
            if (radio->isChecked()) {
                second = radio->property("digitValue").toString();
                break;
            }
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        if (!first.isEmpty() && !second.isEmpty()) {
            result.additional = first + QLatin1Char('/') + second;
        } else if (!first.isEmpty()) {
            result.additional = first;
        }
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    void layoutUi() override {
        m_stop->move(970, 70);
        m_stop->raise();
        if (m_host) {
            m_host->setGeometry(200, 140, qMax(400, width() - 400), qMax(300, height() - 200));
        }
    }

    QList<QRadioButton *> m_row1;
    QList<QRadioButton *> m_row2;
    QWidget *m_host = nullptr;
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
                    m_tableHtml = QString::fromUtf8(file.readAll());
                    m_browser->setHtml(ExerciseAssets::prepareExerciseHtml(
                        m_tableHtml, QFileInfo(table2).absolutePath()));
                }
            }
        } else {
            QFile file(tablePath);
            if (file.open(QIODevice::ReadOnly)) {
                m_tableHtml = QString::fromUtf8(file.readAll());
                m_browser->setHtml(ExerciseAssets::prepareExerciseHtml(
                    m_tableHtml, QFileInfo(tablePath).absolutePath()));
            }
        }
        m_browser->setGeometry(100, 120, width() - 200, height() - 200);
        m_stop->move(970, 70);
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
    }

    static QString extractDivById(const QString &html, const QString &id) {
        const QRegularExpression re(
            QStringLiteral("<div[^>]*\\bid=['\"]%1['\"][^>]*>([\\s\\S]*?)</div>")
                .arg(QRegularExpression::escape(id)),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = re.match(html);
        if (!match.hasMatch()) {
            return {};
        }
        QString inner = match.captured(1);
        inner.replace(QRegularExpression(QStringLiteral("<[^>]+>")), QString());
        return inner.trimmed();
    }

    QString collectAdditional() const {
        // Берём актуальный HTML из браузера (после правок contenteditable).
        const QString html = m_browser ? m_browser->toHtml() : m_tableHtml;
        if (m_exerciseId == QStringLiteral("5.1.1")) {
            QStringList parts;
            for (int i = 1; i <= 8; ++i) {
                parts << extractDivById(html, QStringLiteral("data%1").arg(i));
            }
            return parts.join(QLatin1Char('['));
        }
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            QStringList parts;
            for (int i = 1; i <= 10; ++i) {
                parts << extractDivById(html, QStringLiteral("data%1").arg(i));
            }
            return parts.join(QLatin1Char(';'));
        }
        if (m_exerciseId == QStringLiteral("4.2.2")) {
            QStringList parts;
            for (int i = 1; i <= 6; ++i) {
                parts << extractDivById(html, QStringLiteral("p%1").arg(i));
            }
            return parts.join(QLatin1Char(';'));
        }
        return m_browser ? m_browser->toPlainText().left(4000) : QString();
    }

    void finish() override {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.additional = collectAdditional();
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        if (m_browser) {
            m_browser->setGeometry(100, 120, qMax(200, width() - 200), qMax(200, height() - 200));
        }
        if (m_stop) {
            m_stop->move(970, 70);
            m_stop->raise();
        }
    }

    QTextBrowser *m_browser = nullptr;
    QString m_tableHtml;
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
        // Для NumberedDoneTime хост сам соберёт №;done — не перетираем позициями.
        result.additional.clear();
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
            // cards.cs: 1,5,4,6,3,2 в ряд с шагом 250
            const int order[] = {1, 5, 4, 6, 3, 2};
            for (int i = 0; i < 6; ++i) {
                FlipCardCanvas::Card card;
                card.front = QPixmap(ExerciseAssets::exerciseFile(
                    exerciseId, QString::number(order[i]) + QStringLiteral(".png")));
                card.back = backPixmap;
                card.x = 50 + i * 250;
                card.y = 81;
                cards.append(card);
            }
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
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(true);
        setStyleSheet(QStringLiteral(
            "EmotionsRunner {"
            "  background-color:#ffffff;"
            "  background-image:none;"
            "}"));
        m_canvas = new E126Canvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        m_stop->setStyleSheet(QStringLiteral("background-color:#ffffff; background-image:none;"));
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        connect(m_canvas, &E126Canvas::stopRequested, this, [this]() { finishSession(); });
    }

    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        ExerciseRunnerWidget::paintEvent(event);
    }

    void switchStep(const QString &stepId) override {
        m_stepId = stepId;
        if (m_canvas) {
            m_canvas->switchStep(stepId);
        }
    }

    QString currentAdditionalSnapshot() const override {
        const QString answers = m_canvas ? m_canvas->answersSnapshot() : QString();
        const QString step = m_canvas && !m_canvas->stepId().isEmpty()
            ? m_canvas->stepId()
            : (m_stepId.isEmpty() ? QStringLiteral("1") : m_stepId);
        return step + QLatin1Char(';') + answers;
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
        // Оригинал e126/e1272: pstop Left=970 Top=70
        const double sx = width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
        const double sy = height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
        m_stop->move(qRound(970 * sx), qRound(70 * sy));
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
            const double sx = width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
            const double sy = height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
            m_stop->move(qRound(970 * sx), qRound(70 * sy));
        }
    }

private:
    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        // Для протоколов 1.26/1.272: номер задания + ответы.
        const QString answers = m_canvas ? m_canvas->answersSnapshot() : QString();
        const QString step = m_canvas && !m_canvas->stepId().isEmpty()
            ? m_canvas->stepId()
            : (m_stepId.isEmpty() ? QStringLiteral("1") : m_stepId);
        result.additional = step + QLatin1Char(';') + answers;
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

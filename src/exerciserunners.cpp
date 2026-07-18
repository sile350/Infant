#include "exerciserunnerwidget.h"

#include "e126canvas.h"
#include "e15canvas.h"
#include "exerciseassets.h"
#include "onlypexercise.h"
#include "patientdisplay.h"
#include "puzzlecanvas.h"
#include "puzzlelayout.h"
#include "remembercanvas.h"
#include "wolfrunner.h"

#include "imagebutton.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
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

// Кнопки Стоп/Далее/Показать и т.п. — не зеркалить на экран пациента.
void markPatientControl(QWidget *widget) {
    if (widget) {
        widget->setProperty("dokitPatientControl", true);
    }
}

class TimedSessionRunner : public ExerciseRunnerWidget {
public:
    explicit TimedSessionRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        m_timer = new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

        m_stop = new ClickableLabel(this);
        markPatientControl(m_stop);
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
        markPatientControl(m_continueButton);
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
        markPatientControl(m_showA);
        markPatientControl(m_showB);
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
        markPatientControl(m_next);
        markPatientControl(m_toggle);
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

        if (m_picture) {
            m_picture->hide();
        }
        if (m_group1) {
            m_group1->deleteLater();
            m_group1 = nullptr;
        }
        if (m_group2) {
            m_group2->deleteLater();
            m_group2 = nullptr;
        }

        // digits.Designer.cs: groupBox «1»/«2», колонка А — radio, колонка Б — label (другие ряды).
        static const QStringList kGroup1A = {
            QStringLiteral("9"),
            QStringLiteral("2 4"),
            QStringLiteral("3 8 6"),
            QStringLiteral("1 5 8 5"),
            QStringLiteral("4 6 2 3 9"),
            QStringLiteral("4 8 9 1 7 3"),
            QStringLiteral("5 1 7 4 2 3 8"),
            QStringLiteral("1 4 2 5 9 7 6 3"),
        };
        static const QStringList kGroup1B = {
            QStringLiteral("3"),
            QStringLiteral("7 9"),
            QStringLiteral("1 5 4"),
            QStringLiteral("6 8 5 2"),
            QStringLiteral("3 5 9 6 1"),
            QStringLiteral("7 9 6 4 8 3"),
            QStringLiteral("9 8 5 2 1 6 3"),
            QStringLiteral("4 2 7 0 1 8 9 5"),
        };
        // groupBox2: сверху длинный ряд (radio16=8) → снизу «3» (radio9=1)
        static const QStringList kGroup2A = {
            QStringLiteral("4 9 1 6 3 2 5 8"),
            QStringLiteral("8 5 9 2 3 4 6"),
            QStringLiteral("1 6 5 2 9 8"),
            QStringLiteral("4 1 3 7 2"),
            QStringLiteral("9 2 6 5"),
            QStringLiteral("4 1 7"),
            QStringLiteral("2 5"),
            QStringLiteral("3"),
        };
        static const QStringList kGroup2B = {
            QStringLiteral("4 5 7 1 9 2 8 3"),
            QStringLiteral("1 7 9 5 8 4 6"),
            QStringLiteral("3 1 7 6 9 2"),
            QStringLiteral("2 8 5 9 1"),
            QStringLiteral("4 9 3 7"),
            QStringLiteral("1 5 2"),
            QStringLiteral("8 3"),
            QStringLiteral("6"),
        };
        // Y радиокнопок / подписей Б из Designer
        static const int kRadioY[] = {70, 104, 138, 172, 206, 240, 274, 308};
        static const int kLabelBY[] = {70, 104, 138, 172, 206, 244, 278, 308};

        const QFont groupFont(QStringLiteral("Microsoft Sans Serif"), 16);
        const QFont itemFont(QStringLiteral("Microsoft Sans Serif"), 20);
        const QString boxStyle = QStringLiteral(
            "QGroupBox {"
            "  color:#000000; background-color:#f0f0f0;"
            "  border:1px solid #a0a0a0; margin-top:12px;"
            "}"
            "QGroupBox::title { subcontrol-origin: margin; left:10px; padding:0 4px; }"
            "QRadioButton { color:#000000; background:transparent; spacing:8px; }"
            "QRadioButton::indicator { width:18px; height:18px; }"
            "QLabel { color:#000000; background:transparent; }");

        m_group1 = new QGroupBox(QStringLiteral("1"), this);
        m_group1->setFont(groupFont);
        m_group1->setStyleSheet(boxStyle);
        auto *headerA1 = new QLabel(QStringLiteral("А"), m_group1);
        headerA1->setFont(itemFont);
        headerA1->move(28, 28);
        auto *headerB1 = new QLabel(QStringLiteral("Б"), m_group1);
        headerB1->setFont(itemFont);
        headerB1->move(387, 28);
        auto *excl1 = new QButtonGroup(m_group1);
        for (int i = 0; i < 8; ++i) {
            auto *radio = new QRadioButton(kGroup1A.at(i), m_group1);
            radio->setFont(itemFont);
            radio->move(32, kRadioY[i]);
            radio->adjustSize();
            radio->setProperty("digitValue", i + 1);
            excl1->addButton(radio);
            m_row1 << radio;

            auto *label = new QLabel(kGroup1B.at(i), m_group1);
            label->setFont(itemFont);
            label->move(390, kLabelBY[i]);
            label->adjustSize();
        }

        m_group2 = new QGroupBox(QStringLiteral("2"), this);
        m_group2->setFont(groupFont);
        m_group2->setStyleSheet(boxStyle);
        auto *headerA2 = new QLabel(QStringLiteral("А"), m_group2);
        headerA2->setFont(itemFont);
        headerA2->move(28, 28);
        auto *headerB2 = new QLabel(QStringLiteral("Б"), m_group2);
        headerB2->setFont(itemFont);
        headerB2->move(387, 28);
        auto *excl2 = new QButtonGroup(m_group2);
        for (int i = 0; i < 8; ++i) {
            auto *radio = new QRadioButton(kGroup2A.at(i), m_group2);
            radio->setFont(itemFont);
            radio->move(32, kRadioY[i]);
            radio->adjustSize();
            // radio16=8 … radio9=1
            radio->setProperty("digitValue", 8 - i);
            excl2->addButton(radio);
            m_row2 << radio;

            auto *label = new QLabel(kGroup2B.at(i), m_group2);
            label->setFont(itemFont);
            label->move(368, kLabelBY[i]);
            label->adjustSize();
        }

        setFocusPolicy(Qt::StrongFocus);
        setFocus();
        layoutUi();
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
    }

    void finish() override {
        // digits.cs pstop_Click: results = N; затем results = results + "/" + M
        QString results;
        for (QRadioButton *radio : m_row1) {
            if (radio && radio->isChecked()) {
                results = radio->property("digitValue").toString();
                break;
            }
        }
        for (QRadioButton *radio : m_row2) {
            if (radio && radio->isChecked()) {
                results = results + QLatin1Char('/') + radio->property("digitValue").toString();
                break;
            }
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.additional = results;
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    void layoutUi() override {
        // digits.Designer: groupBox1 (1257,117,571,405), groupBox2 (1257,545,571,405);
        // digits_Load: pstop @ (970,70)
        if (m_group1) {
            m_group1->setGeometry(1257, 117, 571, 405);
            m_group1->show();
            m_group1->raise();
        }
        if (m_group2) {
            m_group2->setGeometry(1257, 545, 571, 405);
            m_group2->show();
            m_group2->raise();
        }
        m_stop->move(970, 70);
        m_stop->raise();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Space) {
            finish();
            return;
        }
        TimedSessionRunner::keyPressEvent(event);
    }

    QList<QRadioButton *> m_row1;
    QList<QRadioButton *> m_row2;
    QGroupBox *m_group1 = nullptr;
    QGroupBox *m_group2 = nullptr;
};

class WordsLearningRunner final : public TimedSessionRunner {
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

        if (m_picture) {
            m_picture->hide();
        }
        if (m_wordsLabel) {
            m_wordsLabel->deleteLater();
            m_wordsLabel = nullptr;
        }
        if (m_table) {
            m_table->deleteLater();
            m_table = nullptr;
        }

        // _4.Designer: label1 @ (984,122), webBrowser1 table2 @ (1306,170,406,334)
        m_wordsLabel = new QLabel(
            QStringLiteral(
                "дерево, кукла, вилка, цветок, телефон, стакан, птица, пальто, лампочка, картина, "
                "человек, книга."),
            this);
        m_wordsLabel->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 14));
        m_wordsLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
        m_wordsLabel->setWordWrap(true);

        m_table = new QTableWidget(6, 1, this);
        m_table->setHorizontalHeaderLabels({QStringLiteral("Кол-во правильно названных слов")});
        m_table->verticalHeader()->setVisible(true);
        m_table->verticalHeader()->setDefaultSectionSize(36);
        m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_table->horizontalHeader()->setFixedHeight(40);
        m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table->setShowGrid(true);
        m_table->setEditTriggers(
            QAbstractItemView::AllEditTriggers);
        m_table->setStyleSheet(QStringLiteral(
            "QTableWidget { background:#f0f0f0; gridline-color:#000000; color:#000000; }"
            "QHeaderView::section { background:#f0f0f0; color:#000000; padding:4px; }"));
        for (int i = 0; i < 6; ++i) {
            m_table->setVerticalHeaderItem(i, new QTableWidgetItem(QString::number(i + 1)));
            auto *item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignCenter);
            item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_table->setItem(i, 0, item);
        }
        // Высота строго по строкам (без пустого растягивания).
        const int tableH = m_table->horizontalHeader()->height()
            + m_table->verticalHeader()->defaultSectionSize() * 6
            + 2 * m_table->frameWidth() + 2;
        m_table->setFixedSize(360, tableH);

        setFocusPolicy(Qt::StrongFocus);
        layoutUi();
        m_table->show();
        m_wordsLabel->show();
        m_stop->move(970, 70);
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
        m_table->setCurrentCell(0, 0);
        m_table->editItem(m_table->item(0, 0));
    }

    void finish() override {
        // Зафиксировать редактор ячейки перед чтением.
        if (m_table) {
            if (QWidget *editor = m_table->indexWidget(m_table->currentIndex())) {
                Q_UNUSED(editor);
            }
            m_table->setCurrentItem(nullptr);
            m_table->clearFocus();
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.additional = collectAdditional();
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    QString collectAdditional() const {
        if (!m_table) {
            return {};
        }
        QStringList parts;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            const QTableWidgetItem *item = m_table->item(r, 0);
            parts << (item ? item->text().trimmed() : QString());
        }
        // Как в оригинале: всегда 6 значений через ';' (включая хвостовой из results += ...+";").
        return parts.join(QLatin1Char(';')) + QLatin1Char(';');
    }

    void layoutUi() override {
        if (m_wordsLabel) {
            m_wordsLabel->setGeometry(984, 122, 886, 48);
            m_wordsLabel->raise();
        }
        if (m_table) {
            // Компактная таблица у правого края, как webBrowser1.
            m_table->move(1306, 170);
            m_table->raise();
        }
        m_stop->move(970, 70);
        m_stop->raise();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Space) {
            finish();
            return;
        }
        TimedSessionRunner::keyPressEvent(event);
    }

    QLabel *m_wordsLabel = nullptr;
    QTableWidget *m_table = nullptr;
};

class HtmlTableRunner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        m_exerciseId = exerciseId;
        m_stepId = stepId.trimmed().isEmpty() ? QStringLiteral("1") : stepId.trimmed();
        m_elapsed = 0;

        if (m_table) {
            m_table->deleteLater();
            m_table = nullptr;
        }
        if (m_stimulusLabel) {
            m_stimulusLabel->deleteLater();
            m_stimulusLabel = nullptr;
        }
        if (m_overtimeLabel) {
            m_overtimeLabel->deleteLater();
            m_overtimeLabel = nullptr;
        }
        if (m_picture) {
            m_picture->hide();
        }

        m_table = new QTableWidget(this);
        m_table->horizontalHeader()->setStretchLastSection(true);
        m_table->verticalHeader()->setVisible(true);
        m_table->setWordWrap(true);
        m_table->setEditTriggers(
            QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked
            | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);

        if (exerciseId == QStringLiteral("5.1.1")) {
            static const QStringList groups = {
                QStringLiteral("Животные"),
                QStringLiteral("Растения"),
                QStringLiteral("Цвета предметов"),
                QStringLiteral("Формы предметов"),
                QStringLiteral("Другие признаки предметов, кроме формы и цвета."),
                QStringLiteral("Действия человека."),
                QStringLiteral("Способы выполнения человеком действий."),
                QStringLiteral("Качества выполняемых человеком действий."),
            };
            m_table->setColumnCount(1);
            m_table->setRowCount(groups.size());
            m_table->setHorizontalHeaderLabels({QStringLiteral("Названные ребенком слова")});
            for (int i = 0; i < groups.size(); ++i) {
                m_table->setVerticalHeaderItem(i, new QTableWidgetItem(groups.at(i)));
                m_table->setItem(i, 0, new QTableWidgetItem);
            }
            m_overtimeLabel = new QLabel(this);
            m_overtimeLabel->setStyleSheet(
                QStringLiteral("color:#cc0000; font-size:16pt; font-weight:700; background:transparent;"));
            m_overtimeLabel->hide();
            if (m_overtimeConnection) {
                disconnect(m_overtimeConnection);
            }
            m_overtimeConnection = connect(m_timer, &QTimer::timeout, this, [this]() {
                if (m_exerciseId == QStringLiteral("5.1.1")) {
                    layoutTableUi();
                }
            });
        } else if (exerciseId == QStringLiteral("5.2.1")) {
            static const QStringList fragments = {
                QStringLiteral("Существительные"),
                QStringLiteral("Глаголы"),
                QStringLiteral("Прилагательные в обычной форме"),
                QStringLiteral("Прилагательные в сравнительной степени"),
                QStringLiteral("Прилагательные в превосходной степени"),
                QStringLiteral("Наречия"),
                QStringLiteral("Местоимения"),
                QStringLiteral("Союзы"),
                QStringLiteral("Предлоги"),
                QStringLiteral("Сложные предложения и конструкции"),
            };
            m_table->setColumnCount(1);
            m_table->setRowCount(fragments.size());
            m_table->setHorizontalHeaderLabels({QStringLiteral("Частота употребления")});
            for (int i = 0; i < fragments.size(); ++i) {
                m_table->setVerticalHeaderItem(i, new QTableWidgetItem(fragments.at(i)));
                m_table->setItem(i, 0, new QTableWidgetItem);
            }
            m_stimulusLabel = new QLabel(this);
            m_stimulusLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            m_stimulusLabel->setStyleSheet(QStringLiteral("background:transparent;"));
            const QString picPath = ExerciseAssets::exerciseFile(
                exerciseId, m_stepId + QStringLiteral(".png"));
            if (!picPath.isEmpty()) {
                m_stimulusLabel->setPixmap(QPixmap(picPath));
            }
            m_stimulusLabel->show();
        } else {
            m_table->setColumnCount(1);
            m_table->setRowCount(1);
            m_table->setItem(0, 0, new QTableWidgetItem);
        }

        m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        layoutTableUi();
        m_table->show();
        m_stop->move(970, 70);
        m_stop->show();
        m_stop->raise();
        m_timer->start();
        show();
        raise();
    }

    void switchStep(const QString &stepId) override {
        const QString next = stepId.trimmed();
        if (next.isEmpty() || next == m_stepId) {
            return;
        }
        m_stepId = next;
        if (m_exerciseId == QStringLiteral("5.2.1") && m_stimulusLabel) {
            const QString picPath =
                ExerciseAssets::exerciseFile(m_exerciseId, m_stepId + QStringLiteral(".png"));
            if (!picPath.isEmpty()) {
                m_stimulusLabel->setPixmap(QPixmap(picPath));
            } else {
                m_stimulusLabel->clear();
            }
            layoutTableUi();
        }
    }

    QString collectAdditional() const {
        if (!m_table) {
            return {};
        }
        QStringList parts;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            const QTableWidgetItem *item = m_table->item(r, 0);
            parts << (item ? item->text().trimmed() : QString());
        }
        if (m_exerciseId == QStringLiteral("5.1.1")) {
            return parts.join(QLatin1Char('['));
        }
        return parts.join(QLatin1Char(';'));
    }

    void finish() override {
        if (m_table) {
            m_table->setCurrentItem(nullptr);
            m_table->clearFocus();
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.additional = collectAdditional();
        m_timer->stop();
        hide();
        emitFinished(result);
    }

    void layoutTableUi() {
        if (!m_table) {
            return;
        }
        if (m_exerciseId == QStringLiteral("5.2.1") && m_stimulusLabel) {
            m_stimulusLabel->setGeometry(40, 120, 520, 700);
            m_table->setGeometry(580, 120, qMax(300, width() - 620), qMax(300, height() - 180));
            m_stimulusLabel->raise();
        } else {
            m_table->setGeometry(100, 120, qMax(400, width() - 200), qMax(300, height() - 200));
        }
        m_table->raise();
        if (m_overtimeLabel && m_exerciseId == QStringLiteral("5.1.1") && m_elapsed > 160) {
            m_overtimeLabel->setText(
                QStringLiteral("%1:%2 сек")
                    .arg(m_elapsed / 60)
                    .arg(m_elapsed % 60, 2, 10, QLatin1Char('0')));
            m_overtimeLabel->adjustSize();
            m_overtimeLabel->move(1200, 70);
            m_overtimeLabel->show();
            m_overtimeLabel->raise();
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        layoutTableUi();
        if (m_stop) {
            m_stop->move(970, 70);
            m_stop->raise();
        }
    }

    void layoutUi() override {
        layoutTableUi();
        m_stop->move(970, 70);
        m_stop->raise();
    }

    QTableWidget *m_table = nullptr;
    QLabel *m_stimulusLabel = nullptr;
    QLabel *m_overtimeLabel = nullptr;
    QMetaObject::Connection m_overtimeConnection;
};

class PuzzlesRunner : public ExerciseRunnerWidget {
public:
    explicit PuzzlesRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new PuzzleCanvas(this);
        m_canvas->hide();

        m_stop = new ClickableLabel(this);
        markPatientControl(m_stop);
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
        setMouseTracking(true);
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
        bool draggable = false;
    };

    int elapsedSeconds() const { return m_elapsed; }

    void setCanvasBackground(const QColor &color) {
        m_background = color;
        update();
    }

    void loadCards(const QString &exerciseId, const QVector<Card> &cards) {
        m_exerciseId = exerciseId;
        m_cards = cards;
        m_elapsed = 0;
        m_dragIndex = -1;
        m_dragging = false;
        m_timer.start();
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), m_background);
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

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        int index = -1;
        if (!hitTestCard(event->pos(), &index)) {
            return;
        }
        m_dragIndex = index;
        m_dragging = false;
        m_pressPos = event->pos();
        const double scale = designScale();
        const int offsetX = (width() - qRound(1920 * scale)) / 2;
        const int offsetY = (height() - qRound(1080 * scale)) / 2;
        m_dragOffsetX = qRound((event->x() - offsetX) / scale) - m_cards[index].x;
        m_dragOffsetY = qRound((event->y() - offsetY) / scale) - m_cards[index].y;
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (m_dragIndex < 0 || m_dragIndex >= m_cards.size()) {
            return;
        }
        Card &card = m_cards[m_dragIndex];
        if (!card.draggable) {
            return;
        }
        if (!m_dragging
            && (event->pos() - m_pressPos).manhattanLength() < 4) {
            return;
        }
        m_dragging = true;
        const double scale = designScale();
        const int offsetX = (width() - qRound(1920 * scale)) / 2;
        const int offsetY = (height() - qRound(1080 * scale)) / 2;
        card.x = qRound((event->x() - offsetX) / scale) - m_dragOffsetX;
        card.y = qRound((event->y() - offsetY) / scale) - m_dragOffsetY;
        update();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        const bool wasDrag = m_dragging;
        const int index = m_dragIndex;
        m_dragging = false;
        m_dragIndex = -1;
        if (wasDrag || index < 0 || index >= m_cards.size()) {
            update();
            return;
        }
        Card &card = m_cards[index];
        if (!card.clickable) {
            return;
        }
        card.closed = !card.closed;
        update();
    }

private:
    double designScale() const {
        return qMin(
            1.0,
            qMin(width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0,
                 height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0));
    }

    bool hitTestCard(const QPoint &pos, int *outIndex) const {
        const double scale = designScale();
        const int offsetX = (width() - qRound(1920 * scale)) / 2;
        const int offsetY = (height() - qRound(1080 * scale)) / 2;
        const int designX = qRound((pos.x() - offsetX) / scale);
        const int designY = qRound((pos.y() - offsetY) / scale);
        for (int i = m_cards.size() - 1; i >= 0; --i) {
            const Card &card = m_cards.at(i);
            const QPixmap &pixmap = card.closed ? card.back : card.front;
            if (pixmap.isNull()) {
                continue;
            }
            if (designX >= card.x && designY >= card.y && designX < card.x + pixmap.width()
                && designY < card.y + pixmap.height()) {
                if (outIndex) {
                    *outIndex = i;
                }
                return true;
            }
        }
        return false;
    }

    QString m_exerciseId;
    QVector<Card> m_cards;
    QTimer m_timer;
    int m_elapsed = 0;
    QColor m_background = Qt::white;
    int m_dragIndex = -1;
    bool m_dragging = false;
    QPoint m_pressPos;
    int m_dragOffsetX = 0;
    int m_dragOffsetY = 0;
};

class CardsRunner final : public ExerciseRunnerWidget {
public:
    explicit CardsRunner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new FlipCardCanvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        markPatientControl(m_stop);
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
        m_wordsHidden = false;

        QVector<FlipCardCanvas::Card> cards;
        const QString zeroPath = ExerciseAssets::exerciseFile(
            exerciseId == QStringLiteral("4.1.8") ? QStringLiteral("4.1.6") : exerciseId,
            QStringLiteral("zero.png"));
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
                card.closed = true;
                card.clickable = true;
                card.draggable = true;
                cards.append(card);
            }
            m_canvas->setCanvasBackground(Qt::white);
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
                    card.closed = true;
                    card.clickable = true;
                    card.draggable = true;
                    cards.append(card);
                    ++count;
                    linex += 250;
                }
                linex = 910;
                liney += 250;
            }
            m_canvas->setCanvasBackground(Qt::white);
        } else if (exerciseId == QStringLiteral("4.1.8")) {
            ensure418Ui();
            m_canvas->setCanvasBackground(QColor(240, 240, 240));

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
                    // pcards.closed = false по умолчанию; clickable = false
                    card.closed = false;
                    card.clickable = false;
                    card.draggable = true;
                    cards.append(card);
                    ++count;
                    linex += 220;
                }
                linex = 1000;
                liney += 210;
            }
        }

        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->loadCards(exerciseId, cards);
        m_canvas->show();
        m_canvas->lower();

        if (exerciseId == QStringLiteral("4.1.8")) {
            layout418Ui();
            if (m_panel418) {
                m_panel418->show();
                m_panel418->raise();
            }
            if (m_hideButton) {
                m_hideButton->show();
                m_hideButton->raise();
            }
        } else {
            if (m_panel418) {
                m_panel418->hide();
            }
            if (m_hideButton) {
                m_hideButton->hide();
            }
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
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_exerciseId == QStringLiteral("4.1.8")) {
            layout418Ui();
        }
        if (m_stop) {
            m_stop->move(80, 72);
            m_stop->raise();
        }
    }

private:
    static const QStringList &stimulusWords() {
        static const QStringList words = {
            QStringLiteral("Школа"),
            QStringLiteral("Обед"),
            QStringLiteral("Утро"),
            QStringLiteral("Красота"),
            QStringLiteral("Прогулка"),
        };
        return words;
    }

    void ensure418Ui() {
        if (!m_panel418) {
            m_panel418 = new QGroupBox(this);
            markPatientControl(m_panel418);
            m_panel418->setTitle(QString());
            m_panel418->setStyleSheet(QStringLiteral(
                "QGroupBox { background-color:#f0f0f0; border:1px solid #a0a0a0; }"
                "QLabel { color:#000000; background:transparent; }"
                "QComboBox { color:#000000; background:#ffffff; }"));

            const QFont labelFont(QStringLiteral("Microsoft Sans Serif"), 8);
            m_helpWordLabel = new QLabel(QStringLiteral("Помощь к слову"), m_panel418);
            m_helpWordLabel->setFont(labelFont);
            m_wordCombo = new QComboBox(m_panel418);
            m_wordCombo->setFont(labelFont);
            m_wordCombo->addItems(stimulusWords());
            m_wordCombo->setCurrentIndex(-1);

            m_helpTypeLabel = new QLabel(QStringLiteral("Виды помощи"), m_panel418);
            m_helpTypeLabel->setFont(labelFont);
            m_helpCombo = new QComboBox(m_panel418);
            m_helpCombo->setFont(labelFont);
            m_helpCombo->addItems({
                QStringLiteral("Повтор более подробной инструкции"),
                QStringLiteral(
                    "Направляющая помощь \"подумай какая карточка сможете тебе напомнить слово\""),
                QStringLiteral("Показ способа выполнения задания с просьбой повторить это действие"),
            });
            m_helpCombo->setCurrentIndex(-1);
            connect(m_wordCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this](int) { m_helpCombo->setCurrentIndex(-1); });
            connect(m_helpCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this](int index) {
                        if (index < 0 || !m_table418 || !m_wordCombo) {
                            return;
                        }
                        const int row = m_wordCombo->currentIndex();
                        if (row < 0 || row >= m_table418->rowCount()) {
                            return;
                        }
                        QTableWidgetItem *item = m_table418->item(row, 3);
                        if (!item) {
                            item = new QTableWidgetItem;
                            m_table418->setItem(row, 3, item);
                        }
                        const QString help = m_helpCombo->currentText().trimmed();
                        if (help.isEmpty()) {
                            return;
                        }
                        const QString prev = item->text().trimmed();
                        item->setText(prev.isEmpty() ? help : (prev + QLatin1Char(' ') + help));
                    });

            m_table418 = new QTableWidget(5, 6, m_panel418);
            m_table418->setHorizontalHeaderLabels({
                QStringLiteral("Выбранная картинка"),
                QStringLiteral("Объяснение выбора"),
                QStringLiteral("Воспроиз. до помощи"),
                QStringLiteral("Виды помощи"),
                QStringLiteral("Воспроиз. после помощи"),
                QStringLiteral("Баллы"),
            });
            m_table418->verticalHeader()->setVisible(true);
            m_table418->verticalHeader()->setMinimumWidth(90);
            m_table418->verticalHeader()->setDefaultSectionSize(56);
            m_table418->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            for (int r = 0; r < 5; ++r) {
                m_table418->setVerticalHeaderItem(r, new QTableWidgetItem(stimulusWords().at(r)));
                for (int c = 0; c < 6; ++c) {
                    m_table418->setItem(r, c, new QTableWidgetItem);
                }
            }
            m_table418->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            m_table418->setWordWrap(true);
        }

        if (!m_hideButton) {
            m_hideButton = new ImageButton(this);
            markPatientControl(m_hideButton);
            connect(m_hideButton, &ImageButton::clicked, this, [this]() { toggle418Words(); });
        }
        m_hideButton->setImagePath(
            ExerciseAssets::exerciseFile(QStringLiteral("4.1.8"), QStringLiteral("hide.png")));
        m_wordsHidden = false;
        apply418WordHeaderVisibility();
    }

    void layout418Ui() {
        if (!m_panel418) {
            return;
        }
        // cards.Designer: groupBox1 @ (12,12,945,421) — опускаем на 200px по ТЗ.
        const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
        const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
        const int panelX = qRound(12 * sx);
        const int panelY = qRound((12 + 200) * sy);
        const int panelW = qRound(945 * sx);
        const int panelH = qRound(421 * sy);
        m_panel418->setGeometry(panelX, panelY, panelW, panelH);

        // Подписи и селекты без наложений (WinForms: gap ~6px при 8pt).
        if (m_helpWordLabel && m_wordCombo && m_helpTypeLabel && m_helpCombo) {
            m_helpWordLabel->setGeometry(qRound(10 * sx), qRound(28 * sy), qRound(120 * sx), qRound(20 * sy));
            m_wordCombo->setGeometry(qRound(135 * sx), qRound(24 * sy), qRound(130 * sx), qRound(24 * sy));
            m_helpTypeLabel->setGeometry(qRound(275 * sx), qRound(28 * sy), qRound(90 * sx), qRound(20 * sy));
            m_helpCombo->setGeometry(qRound(370 * sx), qRound(25 * sy), qRound(560 * sx), qRound(24 * sy));
        }
        if (m_table418) {
            m_table418->setGeometry(
                qRound(6 * sx), qRound(52 * sy), qRound(933 * sx), qRound(354 * sy));
            m_table418->verticalHeader()->setMinimumWidth(qRound(90 * sx));
        }
        if (m_hideButton) {
            m_hideButton->move(qRound(1339 * sx), qRound(29 * sy));
            m_hideButton->raise();
        }
        m_panel418->raise();
    }

    void apply418WordHeaderVisibility() {
        if (!m_table418) {
            return;
        }
        for (int r = 0; r < m_table418->rowCount(); ++r) {
            QTableWidgetItem *header = m_table418->verticalHeaderItem(r);
            if (!header) {
                continue;
            }
            header->setText(m_wordsHidden ? QString() : stimulusWords().value(r));
        }
    }

    void toggle418Words() {
        m_wordsHidden = !m_wordsHidden;
        apply418WordHeaderVisibility();
        if (m_hideButton) {
            m_hideButton->setImagePath(ExerciseAssets::exerciseFile(
                QStringLiteral("4.1.8"),
                m_wordsHidden ? QStringLiteral("show.png") : QStringLiteral("hide.png")));
        }
    }

    QString collect418Additional() const {
        if (!m_table418) {
            return {};
        }
        QStringList rows;
        for (int r = 0; r < m_table418->rowCount(); ++r) {
            QStringList cells;
            for (int c = 0; c < m_table418->columnCount(); ++c) {
                const QTableWidgetItem *item = m_table418->item(r, c);
                cells << (item ? item->text().trimmed() : QString());
            }
            rows << cells.join(QLatin1Char(';'));
        }
        return rows.join(QLatin1Char('|'));
    }

    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        if (m_exerciseId == QStringLiteral("4.1.8")) {
            result.additional = collect418Additional();
        }
        m_canvas->hide();
        if (m_panel418) {
            m_panel418->hide();
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
    ImageButton *m_hideButton = nullptr;
    QGroupBox *m_panel418 = nullptr;
    QLabel *m_helpWordLabel = nullptr;
    QLabel *m_helpTypeLabel = nullptr;
    QComboBox *m_wordCombo = nullptr;
    QComboBox *m_helpCombo = nullptr;
    QTableWidget *m_table418 = nullptr;
    bool m_wordsHidden = false;
};

class E15Runner final : public ExerciseRunnerWidget {
public:
    explicit E15Runner(QWidget *parent = nullptr) : ExerciseRunnerWidget(parent) {
        m_canvas = new E15Canvas(this);
        m_canvas->hide();
        m_stop = new ClickableLabel(this);
        markPatientControl(m_stop);
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
        markPatientControl(m_stop);
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        m_removek = new ClickableLabel(this);
        markPatientControl(m_removek);
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
        markPatientControl(m_stop);
        m_stop->setStyleSheet(QStringLiteral("background-color:#ffffff; background-image:none;"));
        const QString stopPath = ExerciseAssets::sysImage(QStringLiteral("stop.png"));
        if (!stopPath.isEmpty()) {
            m_stop->setPixmap(QPixmap(stopPath));
            m_stop->setFixedSize(QPixmap(stopPath).size());
        }
        m_stop->onClick = [this]() { finishSession(); };
        connect(m_canvas, &E126Canvas::stopRequested, this, [this]() { finishSession(); });
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        if (display && m_canvas) {
            display->attachEmotionsCanvas(m_canvas);
        }
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
        const QString step = m_canvas && !m_canvas->stepId().isEmpty()
            ? m_canvas->stepId()
            : (m_stepId.isEmpty() ? QStringLiteral("1") : m_stepId);
        if (m_exerciseId == QStringLiteral("1.272")) {
            return step;
        }
        const QString answers = m_canvas ? m_canvas->answersSnapshot() : QString();
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
        const QString step = m_canvas && !m_canvas->stepId().isEmpty()
            ? m_canvas->stepId()
            : (m_stepId.isEmpty() ? QStringLiteral("1") : m_stepId);
        if (m_exerciseId == QStringLiteral("1.272")) {
            // Оригинал: createP(..., param1.Text) — только номер задания.
            result.additional = step;
        } else {
            // 1.26: номер задания + ответы.
            const QString answers = m_canvas ? m_canvas->answersSnapshot() : QString();
            result.additional = step + QLatin1Char(';') + answers;
        }
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

void ExerciseRunnerWidget::bindPatientDisplay(PatientDisplay *display) {
    if (display) {
        display->attachMirrorWidget(this);
    }
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
        return new HtmlTableRunner(parent);
    case ExerciseRunnerKind::WordsLearning:
        return new WordsLearningRunner(parent);
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

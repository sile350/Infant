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
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMap>
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

#include <functional>

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
    explicit Remember2Runner(QWidget *parent = nullptr) : TimedSessionRunner(parent) {
        // Как 2.8: непрозрачный белый фон на правой панели (dual) и на оверлее.
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
        setStyleSheet(QStringLiteral("background-color:#ffffff;"));
        // Секундомер во время выполнения (remember2: ltime @ 1380,13) — только на 1-м экране.
        m_liveTimer = new QLabel(this);
        markPatientControl(m_liveTimer);
        m_liveTimer->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 22));
        m_liveTimer->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
        m_liveTimer->hide();
        connect(m_timer, &QTimer::timeout, this, [this]() { updateLiveTimer(); });
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        TimedSessionRunner::startSession(exerciseId, definition, stepId);
        if (m_picture) {
            m_picture->hide();
        }
        ensureCardsUi(exerciseId);
        resetCardsState(exerciseId);
        updateLiveTimer();
        if (m_liveTimer) {
            m_liveTimer->show();
            m_liveTimer->raise();
        }
        layoutRemember2();
    }

    void ensureCardsUi(const QString &exerciseId) {
        // Виджеты один раз — иначе при повторном Start остаются «старые» А/Б (29.4).
        if (!m_showA) {
            m_showA = new ClickableLabel(this);
            m_showA->setCursor(Qt::PointingHandCursor);
            markPatientControl(m_showA);
        }
        if (!m_showB) {
            m_showB = new ClickableLabel(this);
            m_showB->setCursor(Qt::PointingHandCursor);
            markPatientControl(m_showB);
        }
        // Карточки А/Б видны пациенту (не markPatientControl) — иначе зеркало закрашивает их белым.
        if (!m_cardA) {
            m_cardA = new QLabel(this);
        }
        if (!m_cardB) {
            m_cardB = new QLabel(this);
        }
        m_hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
        m_showPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("show.png"));
        m_showA->onClick = [this]() { toggleCardA(); };
        m_showB->onClick = [this]() { toggleCardB(); };
    }

    void resetCardsState(const QString &exerciseId) {
        const QString aPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("a.png"));
        const QString bPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("b.png"));
        m_pixmapA = aPath.isEmpty() ? QPixmap() : QPixmap(aPath);
        m_pixmapB = bPath.isEmpty() ? QPixmap() : QPixmap(bPath);
        if (!m_pixmapA.isNull()) {
            m_cardA->setPixmap(m_pixmapA);
            m_cardA->setFixedSize(m_pixmapA.size());
        }
        if (!m_pixmapB.isNull()) {
            m_cardB->setPixmap(m_pixmapB);
            m_cardB->setFixedSize(m_pixmapB.size());
        }
        // Как remember2: А видна, Б скрыта; кнопки hide/show.
        m_cardAVisible = true;
        m_cardBVisible = false;
        applyCardVisibility();
        if (!m_hidePath.isEmpty()) {
            m_showA->setPixmap(QPixmap(m_hidePath));
            m_showA->setFixedSize(QPixmap(m_hidePath).size());
        }
        if (!m_showPath.isEmpty()) {
            m_showB->setPixmap(QPixmap(m_showPath));
            m_showB->setFixedSize(QPixmap(m_showPath).size());
        }
    }

    void applyCardVisibility() {
        if (m_cardA) {
            if (m_cardAVisible && !m_pixmapA.isNull()) {
                m_cardA->setPixmap(m_pixmapA);
                m_cardA->show();
            } else {
                m_cardA->clear();
                m_cardA->hide();
            }
        }
        if (m_cardB) {
            if (m_cardBVisible && !m_pixmapB.isNull()) {
                m_cardB->setPixmap(m_pixmapB);
                m_cardB->show();
            } else {
                m_cardB->clear();
                m_cardB->hide();
            }
        }
        layoutPatientRemember2();
    }

    void toggleCardA() {
        // remember2.cs pictureBox1: только А; не трогает Б.
        if (m_cardAVisible) {
            m_cardAVisible = false;
            if (!m_showPath.isEmpty()) {
                m_showA->setPixmap(QPixmap(m_showPath));
            }
        } else {
            m_cardAVisible = true;
            if (!m_hidePath.isEmpty()) {
                m_showA->setPixmap(QPixmap(m_hidePath));
            }
        }
        applyCardVisibility();
        layoutRemember2();
    }

    void toggleCardB() {
        // remember2.cs pictureBox2: взаимное переключение А/Б.
        if (!m_cardBVisible) {
            m_cardBVisible = true;
            m_cardAVisible = false;
            if (!m_showPath.isEmpty()) {
                m_showA->setPixmap(QPixmap(m_showPath));
            }
            if (!m_hidePath.isEmpty()) {
                m_showB->setPixmap(QPixmap(m_hidePath));
            }
        } else {
            m_cardBVisible = false;
            m_cardAVisible = true;
            if (!m_hidePath.isEmpty()) {
                m_showA->setPixmap(QPixmap(m_hidePath));
            }
            if (!m_showPath.isEmpty()) {
                m_showB->setPixmap(QPixmap(m_showPath));
            }
        }
        applyCardVisibility();
        layoutRemember2();
    }

    void updateLiveTimer() {
        if (!m_liveTimer) {
            return;
        }
        const int minutes = m_elapsed / 60;
        const int seconds = m_elapsed - minutes * 60;
        m_liveTimer->setText(QStringLiteral("%1:%2 сек").arg(minutes).arg(seconds));
        m_liveTimer->adjustSize();
    }

    void ensurePatientView(PatientDisplay *display) {
        if (!display) {
            return;
        }
        if (!m_patientRoot) {
            m_patientRoot = new QWidget(display);
            m_patientRoot->setAttribute(Qt::WA_StyledBackground, true);
            m_patientRoot->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
            m_patientCardA = new QLabel(m_patientRoot);
            m_patientCardB = new QLabel(m_patientRoot);
            m_patientCardA->setStyleSheet(QStringLiteral("background:transparent;"));
            m_patientCardB->setStyleSheet(QStringLiteral("background:transparent;"));
            m_patientRoot->installEventFilter(this);
        } else if (m_patientRoot->parentWidget() != display) {
            m_patientRoot->setParent(display);
        }
    }

    void layoutPatientRemember2() {
        if (!m_patientRoot || !m_patientCardA || !m_patientCardB) {
            return;
        }
        const int pw = qMax(1, m_patientRoot->width());
        const int ph = qMax(1, m_patientRoot->height());
        // Полный экран пациента: исходный размер PNG, позиции как remember2.Designer.
        const qreal sx = pw / 1920.0;
        const qreal sy = ph / 1080.0;
        constexpr int kNudgeLeft = 40;
        constexpr int kNudgeDown = 40;
        constexpr int kShiftUp = 100;
        const int cardAW = m_pixmapA.isNull() ? 538 : m_pixmapA.width();
        const int cardAH = m_pixmapA.isNull() ? 637 : m_pixmapA.height();
        const int cardBW = m_pixmapB.isNull() ? 530 : m_pixmapB.width();
        const int cardBH = m_pixmapB.isNull() ? 638 : m_pixmapB.height();
        constexpr int kDesignLeft = 708;
        constexpr int kDesignTop = 13;
        const int designRight = qMax(1380 + 104, 1284 + cardBW);
        const int designBottom = qMax(174 + cardAH, 174 + cardBH);
        const int groupW = qMax(1, designRight - kDesignLeft);
        const int groupH = qMax(1, designBottom - kDesignTop);
        const int designCX = kDesignLeft + groupW / 2;
        const int designCY = kDesignTop + groupH / 2;
        const int offsetX = pw / 2 - qRound(designCX * sx) - kNudgeLeft;
        const int offsetY = ph / 2 - qRound(designCY * sy) + kNudgeDown - kShiftUp;
        auto place = [&](int designX, int designY) {
            return QPoint(qRound(designX * sx) + offsetX, qRound(designY * sy) + offsetY);
        };

        if (m_cardAVisible && !m_pixmapA.isNull()) {
            m_patientCardA->setPixmap(m_pixmapA);
            m_patientCardA->setFixedSize(m_pixmapA.size());
            m_patientCardA->move(place(708, 174));
            m_patientCardA->show();
            m_patientCardA->raise();
        } else {
            m_patientCardA->hide();
        }
        if (m_cardBVisible && !m_pixmapB.isNull()) {
            m_patientCardB->setPixmap(m_pixmapB);
            m_patientCardB->setFixedSize(m_pixmapB.size());
            m_patientCardB->move(place(1284, 174));
            m_patientCardB->show();
            m_patientCardB->raise();
        } else {
            m_patientCardB->hide();
        }
        Q_UNUSED(cardAW);
        Q_UNUSED(cardBH);
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        if (!display) {
            return;
        }
        ensurePatientView(display);
        layoutPatientRemember2();
        display->attachContentWidget(m_patientRoot);
    }

    void layoutRemember2Compact() {
        // Dual / узкая панель специалиста: картинки и «Показать/скрыть» на правой половине.
        // Картинки — только native или downscale (без увеличения).
        constexpr int kMargin = 12;
        constexpr int kGap = 16;
        constexpr int kBtnTop = 12;
        constexpr int kGroupDown = 100;
        const int btnH = m_showA ? m_showA->height() : 34;
        const int contentTop = 56 + kGroupDown;
        const int availW = qMax(80, width() - 2 * kMargin);
        const int availH = qMax(80, height() - contentTop - btnH - 8 - kMargin);
        const int halfW = qMax(40, (availW - kGap) / 2);

        if (m_stop) {
            m_stop->move(kMargin, kBtnTop);
            m_stop->show();
            m_stop->raise();
        }
        if (m_liveTimer) {
            const int timerX = m_stop ? (m_stop->x() + m_stop->width() + 24) : (kMargin + 150);
            m_liveTimer->move(timerX, kBtnTop);
            m_liveTimer->show();
            m_liveTimer->raise();
        }

        auto placeColumn = [&](QLabel *card, ClickableLabel *btn, bool visible,
                               const QPixmap &src, int colX) {
            QPixmap disp;
            if (!src.isNull()) {
                disp = pixmapNativeOrDownscale(src, halfW, availH);
            }
            const int picW = disp.isNull() ? 0 : disp.width();
            const int colCenterX = colX + halfW / 2;
            if (btn) {
                btn->move(colCenterX - btn->width() / 2, contentTop);
                btn->show();
                btn->raise();
            }
            if (card) {
                if (visible && !disp.isNull()) {
                    card->setPixmap(disp);
                    card->setFixedSize(disp.size());
                    card->move(colCenterX - picW / 2, contentTop + btnH + 8);
                    card->show();
                    card->raise();
                } else {
                    card->hide();
                }
            }
        };

        placeColumn(m_cardA, m_showA, m_cardAVisible, m_pixmapA, kMargin);
        placeColumn(m_cardB, m_showB, m_cardBVisible, m_pixmapB, kMargin + halfW + kGap);
        if (m_stop) {
            m_stop->raise();
        }
        if (m_showA) {
            m_showA->raise();
        }
        if (m_showB) {
            m_showB->raise();
        }
        layoutPatientRemember2();
    }

    void layoutRemember2() {
        // Dual / узкая панель — компакт на правой половине специалиста.
        if (m_sessionOptions.dualScreen || width() < 1400) {
            layoutRemember2Compact();
            return;
        }
        // remember2.Designer: stop 970@70, кнопки 1120/1380@120, карточки 708/1284@174, ltime 1380@13.
        const qreal sx = width() / 1920.0;
        const qreal sy = height() / 1080.0;
        const int cardAH = m_cardA ? m_cardA->height() : 637;
        const int cardBW = m_cardB ? m_cardB->width() : 530;
        const int cardBH = m_cardB ? m_cardB->height() : 638;
        const int btnW = m_showA ? m_showA->width() : 104;
        const int btnH = m_showA ? m_showA->height() : 34;

        constexpr int kDesignLeft = 708;
        constexpr int kDesignTop = 13;
        const int designRight = qMax(1380 + btnW, 1284 + cardBW);
        const int designBottom = qMax(174 + cardAH, 174 + cardBH);
        const int groupW = qMax(1, designRight - kDesignLeft);
        const int groupH = qMax(1, designBottom - kDesignTop);
        const int designCX = kDesignLeft + groupW / 2;
        const int designCY = kDesignTop + groupH / 2;
        constexpr int kNudgeLeft = 40;
        constexpr int kNudgeDown = 40;
        constexpr int kShiftUp = 100;
        const int offsetX = width() / 2 - qRound(designCX * sx) - kNudgeLeft;
        const int offsetY = height() / 2 - qRound(designCY * sy) + kNudgeDown - kShiftUp;

        auto place = [&](int designX, int designY) {
            return QPoint(qRound(designX * sx) + offsetX, qRound(designY * sy) + offsetY);
        };

        if (m_stop) {
            m_stop->move(place(820, 70));
            m_stop->show();
            m_stop->raise();
        }
        if (m_showA) {
            m_showA->move(place(1120, 120));
            m_showA->show();
            m_showA->raise();
        }
        if (m_showB) {
            m_showB->move(place(1380, 120));
            m_showB->show();
            m_showB->raise();
        }
        if (m_cardA && m_cardAVisible) {
            if (!m_pixmapA.isNull()) {
                m_cardA->setPixmap(m_pixmapA);
                m_cardA->setFixedSize(m_pixmapA.size());
            }
            m_cardA->move(place(708, 174));
            m_cardA->show();
            m_cardA->raise();
        }
        if (m_cardB && m_cardBVisible) {
            if (!m_pixmapB.isNull()) {
                m_cardB->setPixmap(m_pixmapB);
                m_cardB->setFixedSize(m_pixmapB.size());
            }
            m_cardB->move(place(1284, 174));
            m_cardB->show();
            m_cardB->raise();
        }
        if (m_liveTimer) {
            m_liveTimer->move(place(1380, 70));
            m_liveTimer->show();
            m_liveTimer->raise();
        }
        layoutPatientRemember2();
        Q_UNUSED(btnH);
    }

    void finish() override {
        if (m_liveTimer) {
            m_liveTimer->hide();
        }
        if (m_patientRoot) {
            m_patientRoot->hide();
        }
        TimedSessionRunner::finish();
    }

    void layoutUi() override {
        if (m_picture) {
            m_picture->hide();
        }
        layoutRemember2();
    }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        layoutRemember2();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_patientRoot && event->type() == QEvent::Resize) {
            layoutPatientRemember2();
        }
        return TimedSessionRunner::eventFilter(watched, event);
    }

    ClickableLabel *m_showA = nullptr;
    ClickableLabel *m_showB = nullptr;
    QLabel *m_cardA = nullptr;
    QLabel *m_cardB = nullptr;
    QLabel *m_liveTimer = nullptr;
    QWidget *m_patientRoot = nullptr;
    QLabel *m_patientCardA = nullptr;
    QLabel *m_patientCardB = nullptr;
    QPixmap m_pixmapA;
    QPixmap m_pixmapB;
    QString m_showPath;
    QString m_hidePath;
    bool m_cardAVisible = true;
    bool m_cardBVisible = false;
};

class E28Runner final : public TimedSessionRunner {
public:
    explicit E28Runner(QWidget *parent = nullptr) : TimedSessionRunner(parent) {
        setAttribute(Qt::WA_StyledBackground, true);
        setAutoFillBackground(true);
        setStyleSheet(QStringLiteral("background-color:#ffffff;"));
        m_reference = new QLabel(this);
        m_task = new QLabel(this);
        m_next = new ClickableLabel(this);
        m_toggle = new ClickableLabel(this);
        m_reference->setStyleSheet(QStringLiteral("background:transparent;"));
        m_task->setStyleSheet(QStringLiteral("background:transparent;"));
        m_next->setCursor(Qt::PointingHandCursor);
        m_toggle->setCursor(Qt::PointingHandCursor);
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
        m_phaseTask = false;
        m_hintVisible = true;
        m_refSource = QPixmap();
        m_taskSource = QPixmap();
        if (m_picture) {
            m_picture->hide();
        }

        QString refPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.png"));
        if (refPath.isEmpty()) {
            refPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("1.PNG"));
        }
        if (!refPath.isEmpty()) {
            m_refSource = QPixmap(refPath);
            m_reference->setPixmap(m_refSource);
            m_reference->setFixedSize(m_refSource.size());
            m_reference->show();
        } else {
            m_reference->clear();
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
                m_taskSource = QPixmap(taskPath);
                m_task->setPixmap(m_taskSource);
                m_task->setFixedSize(m_taskSource.size());
                m_task->show();
            }
            // Как e28: pictureBox2.Image = null — подсказка скрыта до «Показать».
            m_hintVisible = false;
            m_reference->clear();
            m_reference->hide();
            m_next->hide();
            m_phaseTask = true;
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
            if (!m_hintVisible) {
                m_hintVisible = true;
                if (!m_refSource.isNull()) {
                    m_reference->setPixmap(m_refSource);
                    m_reference->setFixedSize(m_refSource.size());
                    m_reference->show();
                }
                const QString hidePath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("hide.png"));
                if (!hidePath.isEmpty()) {
                    const QPixmap hidePixmap(hidePath);
                    m_toggle->setPixmap(hidePixmap);
                    m_toggle->setFixedSize(hidePixmap.size());
                }
            } else {
                m_hintVisible = false;
                m_reference->clear();
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
        if (m_stop) {
            m_stop->show();
            m_stop->raise();
        }
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        if (!display) {
            return;
        }
        ensurePatientView(display);
        layoutPatientE28();
        display->attachContentWidget(m_patientRoot);
    }

    void layoutUi() override {
        layoutE28();
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_patientRoot && event->type() == QEvent::Resize) {
            layoutPatientE28();
        }
        return TimedSessionRunner::eventFilter(watched, event);
    }

    void finish() override {
        if (m_patientRoot) {
            m_patientRoot->hide();
        }
        TimedSessionRunner::finish();
    }

private:
    void layoutE28() {
        const bool compact = m_sessionOptions.dualScreen || width() < 1400;
        if (compact) {
            layoutE28Compact();
        } else {
            layoutE28Full();
        }
        layoutPatientE28();
    }

    void layoutE28Full() {
        // Оригинал e28.Designer: pictureBox1 (задание/2.png) = (228,246),
        // pictureBox2 (подсказка/1.png) = (1043,246); stop 970@70, show/next = stop+200.
        const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
        const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
        auto place = [&](int x, int y) {
            return QPoint(qRound(x * sx), qRound(y * sy));
        };

        if (m_stop) {
            m_stop->move(place(970, 70));
            m_stop->show();
            m_stop->raise();
        }
        const QPoint btnPos = place(1170, 70);
        if (m_next && m_next->isVisible()) {
            m_next->move(btnPos);
            m_next->raise();
        }
        if (m_toggle && m_toggle->isVisible()) {
            m_toggle->move(btnPos);
            m_toggle->raise();
        }

        if (m_task && m_task->isVisible() && !m_taskSource.isNull()) {
            m_task->setPixmap(m_taskSource);
            m_task->setFixedSize(m_taskSource.size());
            m_task->move(place(228, 246));
            m_task->show();
            m_task->raise();
        }
        if (m_reference && m_hintVisible && !m_refSource.isNull()) {
            m_reference->setPixmap(m_refSource);
            m_reference->setFixedSize(m_refSource.size());
            m_reference->move(place(1043, 246));
            m_reference->show();
            m_reference->raise();
        }
        if (m_stop) {
            m_stop->raise();
        }
        if (m_toggle && m_toggle->isVisible()) {
            m_toggle->raise();
        }
        if (m_next && m_next->isVisible()) {
            m_next->raise();
        }
    }

    void layoutE28Compact() {
        // Dual / узкая панель: картинки и «Показать/скрыть» на правой половине.
        constexpr int kMargin = 12;
        constexpr int kGap = 16;
        constexpr int kBtnTop = 12;
        const int contentTop = 56;
        const int availW = qMax(80, width() - 2 * kMargin);
        const int availH = qMax(80, height() - contentTop - kMargin);

        if (m_stop) {
            m_stop->move(kMargin, kBtnTop);
            m_stop->show();
            m_stop->raise();
        }
        // Далее / Показать-скрыть — на 100px правее «Стоп».
        const int btnX = m_stop ? (m_stop->x() + m_stop->width() + 100) : (kMargin + 150);
        if (m_next && m_next->isVisible()) {
            m_next->move(btnX, kBtnTop);
            m_next->raise();
        }
        if (m_toggle && m_toggle->isVisible()) {
            m_toggle->move(btnX, kBtnTop);
            m_toggle->raise();
        }

        const bool showTask = m_phaseTask && !m_taskSource.isNull();
        const bool showHint = m_hintVisible && !m_refSource.isNull();

        if (showTask && showHint) {
            const int halfW = qMax(40, (availW - kGap) / 2);
            const QPixmap taskDisp =
                pixmapNativeOrDownscale(m_taskSource, halfW, availH);
            const QPixmap refDisp =
                pixmapNativeOrDownscale(m_refSource, halfW, availH);
            const int maxH = qMax(taskDisp.height(), refDisp.height());
            const int picY = contentTop + qMax(0, (availH - maxH) / 2);
            m_task->setPixmap(taskDisp);
            m_task->setFixedSize(taskDisp.size());
            m_task->move(kMargin + qMax(0, (halfW - taskDisp.width()) / 2), picY);
            m_task->show();
            m_task->raise();
            m_reference->setPixmap(refDisp);
            m_reference->setFixedSize(refDisp.size());
            m_reference->move(
                kMargin + halfW + kGap + qMax(0, (halfW - refDisp.width()) / 2), picY);
            m_reference->show();
            m_reference->raise();
        } else if (showTask) {
            const QPixmap taskDisp =
                pixmapNativeOrDownscale(m_taskSource, availW, availH);
            m_task->setPixmap(taskDisp);
            m_task->setFixedSize(taskDisp.size());
            m_task->move(
                kMargin + qMax(0, (availW - taskDisp.width()) / 2),
                contentTop + qMax(0, (availH - taskDisp.height()) / 2));
            m_task->show();
            m_task->raise();
            if (m_reference) {
                m_reference->hide();
            }
        } else if (showHint) {
            const QPixmap refDisp =
                pixmapNativeOrDownscale(m_refSource, availW, availH);
            m_reference->setPixmap(refDisp);
            m_reference->setFixedSize(refDisp.size());
            m_reference->move(
                kMargin + qMax(0, (availW - refDisp.width()) / 2),
                contentTop + qMax(0, (availH - refDisp.height()) / 2));
            m_reference->show();
            m_reference->raise();
            if (m_task) {
                m_task->hide();
            }
        }

        if (m_stop) {
            m_stop->raise();
        }
        if (m_toggle && m_toggle->isVisible()) {
            m_toggle->raise();
        }
        if (m_next && m_next->isVisible()) {
            m_next->raise();
        }
    }

    void ensurePatientView(PatientDisplay *display) {
        if (!m_patientRoot) {
            m_patientRoot = new QWidget(display);
            m_patientRoot->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
            m_patientRoot->installEventFilter(this);
            m_patientTask = new QLabel(m_patientRoot);
            m_patientRef = new QLabel(m_patientRoot);
            m_patientTask->setStyleSheet(QStringLiteral("background:transparent;"));
            m_patientRef->setStyleSheet(QStringLiteral("background:transparent;"));
            m_patientTask->setScaledContents(false);
            m_patientRef->setScaledContents(false);
            m_patientTask->hide();
            m_patientRef->hide();
        } else if (m_patientRoot->parentWidget() != display) {
            m_patientRoot->setParent(display);
        }
    }

    void layoutPatientE28() {
        if (!m_patientRoot || !m_patientTask || !m_patientRef) {
            return;
        }
        const int pw = qMax(1, m_patientRoot->width());
        const int ph = qMax(1, m_patientRoot->height());
        // Как e28.Designer на полном экране: исходный размер, позиции (228,246) / (1043,246).
        const qreal sx = pw / 1920.0;
        const qreal sy = ph / 1080.0;
        const bool showTask = m_phaseTask && !m_taskSource.isNull();
        const bool showHint = m_hintVisible && !m_refSource.isNull();

        if (showTask) {
            m_patientTask->setPixmap(m_taskSource);
            m_patientTask->setFixedSize(m_taskSource.size());
            m_patientTask->move(qRound(228 * sx), qRound(246 * sy));
            m_patientTask->show();
            m_patientTask->raise();
        } else {
            m_patientTask->hide();
        }
        if (showHint) {
            m_patientRef->setPixmap(m_refSource);
            m_patientRef->setFixedSize(m_refSource.size());
            m_patientRef->move(qRound(1043 * sx), qRound(246 * sy));
            m_patientRef->show();
            m_patientRef->raise();
        } else {
            m_patientRef->hide();
        }
    }

    QLabel *m_reference = nullptr;
    QLabel *m_task = nullptr;
    ClickableLabel *m_next = nullptr;
    ClickableLabel *m_toggle = nullptr;
    QPixmap m_refSource;
    QPixmap m_taskSource;
    bool m_phaseTask = false;
    bool m_hintVisible = true;
    QWidget *m_patientRoot = nullptr;
    QLabel *m_patientTask = nullptr;
    QLabel *m_patientRef = nullptr;
};

// Тельняшка 4.2.1: полосы строго по Y строк (как f1.png), без серых прямоугольников под цифрами.
class DigitsStripeBackground final : public QWidget {
public:
    explicit DigitsStripeBackground(QWidget *parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        // Цвета как в f1.png: белый / #f8f8f8.
        painter.fillRect(rect(), Qt::white);
        // Y строк из digits.Designer (radio / label).
        static const int kRowY[] = {70, 104, 138, 172, 206, 240, 274, 308};
        constexpr int kHeaderTop = 22;
        constexpr int kContentBottom = 350;
        // Шапка «А/Б» — серая полоса (как до старта на f1.png).
        painter.fillRect(0, kHeaderTop, width(), kRowY[0] - kHeaderTop - 8, QColor(248, 248, 248));
        for (int i = 0; i < 8; ++i) {
            const int yTop = (i == 0) ? (kRowY[0] - 8) : ((kRowY[i - 1] + kRowY[i]) / 2);
            const int yBot = (i == 7) ? kContentBottom : ((kRowY[i] + kRowY[i + 1]) / 2);
            // 1-я строка цифр — белая, 2-я — серая, далее чередование (как f1.png).
            if (i % 2 == 1) {
                painter.fillRect(0, yTop, width(), qMax(1, yBot - yTop), QColor(248, 248, 248));
            }
        }
    }
};

class DigitsRunner final : public TimedSessionRunner {
public:
    using TimedSessionRunner::TimedSessionRunner;

    void prepareStaticPreview(const QString &exerciseId) override {
        m_exerciseId = exerciseId;
        ensureDigitsUi(exerciseId);
        if (m_timer) {
            m_timer->stop();
        }
        if (m_stop) {
            m_stop->hide();
        }
        // В превью радиокнопки видны, но без секундомера / Стоп.
        setEnabled(true);
        layoutUi();
        show();
        raise();
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_elapsed = 0;
        ensureDigitsUi(exerciseId);
        // Сброс выбора при новом запуске.
        for (QRadioButton *radio : m_row1) {
            if (radio) {
                radio->setAutoExclusive(false);
                radio->setChecked(false);
                radio->setAutoExclusive(true);
            }
        }
        for (QRadioButton *radio : m_row2) {
            if (radio) {
                radio->setAutoExclusive(false);
                radio->setChecked(false);
                radio->setAutoExclusive(true);
            }
        }
        setFocusPolicy(Qt::StrongFocus);
        setFocus();
        if (m_stop) {
            m_stop->show();
        }
        // layoutUi после show: иначе isVisible()=false и Стоп остаётся у y=0
        // (прилипает к вкладкам), а не на высоте «Начать упражнение» (y=12).
        layoutUi();
        m_timer->start();
        show();
        raise();
    }

    void ensureDigitsUi(const QString &exerciseId) {
        Q_UNUSED(exerciseId);
        if (m_picture) {
            m_picture->hide();
        }
        if (m_bgLabel) {
            m_bgLabel->hide();
        }
        if (m_group1 && m_group2) {
            return;
        }
        m_row1.clear();
        m_row2.clear();
        if (m_group1) {
            m_group1->deleteLater();
            m_group1 = nullptr;
        }
        if (m_group2) {
            m_group2->deleteLater();
            m_group2 = nullptr;
        }

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
        static const int kRadioY[] = {70, 104, 138, 172, 206, 240, 274, 308};
        static const int kLabelBY[] = {70, 104, 138, 172, 206, 244, 278, 308};

        const QFont groupFont(QStringLiteral("Microsoft Sans Serif"), 16);
        const QFont itemFont(QStringLiteral("Microsoft Sans Serif"), 20);
        const QString boxStyle = QStringLiteral(
            "QGroupBox {"
            "  color:#000000; background-color:transparent;"
            "  border:2px solid #228B22; margin-top:24px; padding-top:2px;"
            "}"
            "QGroupBox::title {"
            "  subcontrol-origin: margin; subcontrol-position: top left;"
            "  left:12px; top:2px; padding:0 8px;"
            "  color:#000000; background-color:#ffffff;"
            "}"
            "QRadioButton {"
            "  color:#000000; background-color:transparent; border:none; spacing:6px;"
            "}"
            "QRadioButton::indicator { width:14px; height:14px; }"
            "QRadioButton::indicator:unchecked {"
            "  border:1px solid #808080; border-radius:7px; background:#ffffff;"
            "}"
            "QRadioButton::indicator:checked {"
            "  border:1px solid #808080; border-radius:7px;"
            "  background-color:qradialgradient("
            "    cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,"
            "    stop:0 #333333, stop:0.38 #333333, stop:0.39 #ffffff);"
            "}"
            "QLabel { color:#000000; background-color:transparent; border:none; }");

        auto makeTransparent = [](QWidget *w) {
            if (!w) {
                return;
            }
            w->setAttribute(Qt::WA_TranslucentBackground, true);
            w->setAutoFillBackground(false);
            QPalette pal = w->palette();
            pal.setColor(QPalette::Window, Qt::transparent);
            pal.setColor(QPalette::Base, Qt::transparent);
            pal.setColor(QPalette::Button, Qt::transparent);
            w->setPalette(pal);
        };

        auto buildGroup = [&](const QString &title,
                              const QStringList &colA,
                              const QStringList &colB,
                              int labelBX,
                              bool ascendingValues,
                              QList<QRadioButton *> *rowOut) -> QGroupBox * {
            auto *box = new QGroupBox(title, this);
            box->setFont(groupFont);
            box->setStyleSheet(boxStyle);
            makeTransparent(box);

            auto *stripes = new DigitsStripeBackground(box);
            stripes->setGeometry(2, 20, 567, 381);
            stripes->lower();
            stripes->show();

            auto *headerA = new QLabel(QStringLiteral("А"), box);
            headerA->setFont(itemFont);
            makeTransparent(headerA);
            headerA->move(28, 28);
            headerA->adjustSize();
            auto *headerB = new QLabel(QStringLiteral("Б"), box);
            headerB->setFont(itemFont);
            makeTransparent(headerB);
            headerB->move(387, 28);
            headerB->adjustSize();

            auto *excl = new QButtonGroup(box);
            for (int i = 0; i < 8; ++i) {
                auto *radio = new QRadioButton(colA.at(i), box);
                radio->setFont(itemFont);
                radio->move(32, kRadioY[i]);
                radio->adjustSize();
                radio->setProperty("digitValue", ascendingValues ? (i + 1) : (8 - i));
                excl->addButton(radio);
                *rowOut << radio;

                auto *label = new QLabel(colB.at(i), box);
                label->setFont(itemFont);
                makeTransparent(label);
                label->move(labelBX, kLabelBY[i]);
                label->adjustSize();
            }
            return box;
        };

        m_group1 = buildGroup(QStringLiteral("1"), kGroup1A, kGroup1B, 390, true, &m_row1);
        m_group2 = buildGroup(QStringLiteral("2"), kGroup2A, kGroup2B, 368, false, &m_row2);
    }

    void finish() override {
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
        if (m_stop) {
            m_stop->hide();
        }
        // Оставляем UI на правой панели как превью после Стоп.
        emitFinished(result);
    }

    void layoutUi() override {
        // Правая панель: две группы 571×405 друг под другом (фиксированные координаты
        // как в digits.Designer — дочерние радио/лейблы не масштабируем).
        constexpr int kBoxW = 571;
        constexpr int kBoxH = 405;
        constexpr int kGap = 12;
        constexpr int kTop = 48;
        const int x = qMax(8, (width() - kBoxW) / 2);
        int y1 = kTop;
        int y2 = y1 + kBoxH + kGap;
        if (height() > 100 && y2 + kBoxH > height() - 8) {
            y1 = 36;
            y2 = y1 + kBoxH + 4;
        }

        if (m_group1) {
            m_group1->setGeometry(x, y1, kBoxW, kBoxH);
            if (auto *stripes = m_group1->findChild<DigitsStripeBackground *>()) {
                stripes->setGeometry(2, 20, kBoxW - 4, kBoxH - 22);
                stripes->lower();
            }
            m_group1->show();
            m_group1->raise();
        }
        if (m_group2) {
            m_group2->setGeometry(x, y2, kBoxW, kBoxH);
            if (auto *stripes = m_group2->findChild<DigitsStripeBackground *>()) {
                stripes->setGeometry(2, 20, kBoxW - 4, kBoxH - 22);
                stripes->lower();
            }
            m_group2->show();
            m_group2->raise();
        }
        // Как «Начать упражнение» в ExerciseHost (y=12), не у верхнего края / вкладок.
        if (m_stop) {
            m_stop->move(12, 12);
            if (m_stop->isVisible()) {
                m_stop->raise();
            }
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Space) {
            finish();
            return;
        }
        TimedSessionRunner::keyPressEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override {
        TimedSessionRunner::resizeEvent(event);
        layoutUi();
    }

    QList<QRadioButton *> m_row1;
    QList<QRadioButton *> m_row2;
    QGroupBox *m_group1 = nullptr;
    QGroupBox *m_group2 = nullptr;
    QLabel *m_bgLabel = nullptr;
    QPixmap m_bgSource;
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

        // Как table2.html: 2 колонки, без QHeaderView (чёрные заголовки).
        m_table = new QTableWidget(7, 2, this);
        m_table->horizontalHeader()->setVisible(false);
        m_table->verticalHeader()->setVisible(false);
        m_table->setShowGrid(true);
        m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_table->setEditTriggers(QAbstractItemView::AllEditTriggers);
        m_table->setStyleSheet(QStringLiteral(
            "QTableWidget {"
            "  background-color:#f0f0f0; color:#000000; gridline-color:#000000;"
            "  border:1px solid #000000; outline:none;"
            "}"
            "QTableWidget::item {"
            "  background-color:#f0f0f0; color:#000000;"
            "  border:none; padding:2px;"
            "}"
            "QTableWidget::item:selected {"
            "  background-color:#d0e8ff; color:#000000;"
            "}"));
        m_table->setColumnWidth(0, 76);
        m_table->setColumnWidth(1, 284);
        auto makeCell = [](const QString &text, bool bold = false) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QBrush(Qt::black));
            item->setBackground(QBrush(QColor(240, 240, 240)));
            if (bold) {
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            item->setFlags(Qt::ItemIsEnabled);
            return item;
        };
        m_table->setItem(0, 0, makeCell(QStringLiteral("№ попытки"), true));
        m_table->setItem(0, 1, makeCell(QStringLiteral("Кол-во правильно названных слов"), true));
        m_table->setRowHeight(0, 40);
        for (int i = 0; i < 6; ++i) {
            m_table->setItem(i + 1, 0, makeCell(QString::number(i + 1)));
            auto *value = makeCell(QString());
            value->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_table->setItem(i + 1, 1, value);
            m_table->setRowHeight(i + 1, 36);
        }
        const int tableH = 40 + 36 * 6 + 2 * m_table->frameWidth() + 2;
        m_table->setFixedSize(76 + 284 + 2 * m_table->frameWidth() + 2, tableH);
        connect(m_table, &QTableWidget::cellChanged, this, [this](int, int) { syncPatientWordsView(); });

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
        m_table->setCurrentCell(1, 1);
        m_table->editItem(m_table->item(1, 1));
        syncPatientWordsView();
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
        if (m_patientRoot) {
            m_patientRoot->hide();
        }
        hide();
        emitFinished(result);
    }

    QString collectAdditional() const {
        if (!m_table) {
            return {};
        }
        QStringList parts;
        for (int r = 0; r < 6; ++r) {
            const QTableWidgetItem *item = m_table->item(r + 1, 1);
            parts << (item ? item->text().trimmed() : QString());
        }
        // Как в оригинале: всегда 6 значений через ';' (включая хвостовой из results += ...+";").
        return parts.join(QLatin1Char(';')) + QLatin1Char(';');
    }

    void layoutUi() override {
        // Dual: слова и таблица на 100 px ниже (ТЗ 34.4); второго экрана нет.
        const int yOff = m_sessionOptions.dualScreen ? 100 : 0;
        if (m_wordsLabel) {
            m_wordsLabel->setGeometry(984, 122 + yOff, 886, 48);
            m_wordsLabel->raise();
        }
        if (m_table) {
            m_table->move(1306, 170 + yOff);
            m_table->raise();
        }
        m_stop->move(970, 70);
        m_stop->raise();
        syncPatientWordsView();
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        Q_UNUSED(display);
        // 4.2.2: второго экрана нет даже в dual.
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Space) {
            finish();
            return;
        }
        TimedSessionRunner::keyPressEvent(event);
    }

private:
    void syncPatientWordsView() {
        // Пациентский экран отключён для 4.2.2.
    }

    QLabel *m_wordsLabel = nullptr;
    QTableWidget *m_table = nullptr;
    QWidget *m_patientRoot = nullptr;
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
        if (m_liveTimer) {
            m_liveTimer->deleteLater();
            m_liveTimer = nullptr;
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
            // Как table.html: заголовки в первой строке, без QHeaderView.
            m_table->setColumnCount(2);
            m_table->setRowCount(groups.size() + 1);
            m_table->horizontalHeader()->setVisible(false);
            m_table->verticalHeader()->setVisible(false);
            m_table->setShowGrid(true);
            m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_table->setStyleSheet(QStringLiteral(
                "QTableWidget {"
                "  background-color:#f6f6f6; color:#000000; gridline-color:#000000;"
                "  border:1px solid #000000; outline:none;"
                "}"
                "QTableWidget::item {"
                "  background-color:#f6f6f6; color:#000000;"
                "  border:none; padding:4px;"
                "}"
                "QTableWidget::item:selected {"
                "  background-color:#d0e8ff; color:#000000;"
                "}"));
            m_table->setColumnWidth(0, 266);
            m_table->setColumnWidth(1, 464);
            auto makeCell = [](const QString &text, bool header = false) {
                auto *item = new QTableWidgetItem(text);
                item->setForeground(QBrush(Qt::black));
                item->setBackground(QBrush(QColor(246, 246, 246)));
                if (header) {
                    item->setTextAlignment(Qt::AlignCenter);
                    QFont f = item->font();
                    f.setBold(true);
                    item->setFont(f);
                } else {
                    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                }
                item->setFlags(Qt::ItemIsEnabled);
                return item;
            };
            m_table->setItem(0, 0, makeCell(QStringLiteral("Группы"), true));
            m_table->setItem(0, 1, makeCell(QStringLiteral("Названные ребенком слова"), true));
            m_table->setRowHeight(0, 40);
            for (int i = 0; i < groups.size(); ++i) {
                m_table->setItem(i + 1, 0, makeCell(groups.at(i)));
                auto *wordItem = makeCell(QString());
                wordItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                m_table->setItem(i + 1, 1, wordItem);
                m_table->setRowHeight(i + 1, 36);
            }
            const int tableH = 40 + 36 * groups.size() + 2 * m_table->frameWidth() + 2;
            m_table->setFixedSize(266 + 464 + 2 * m_table->frameWidth() + 2, tableH);
            m_liveTimer = new QLabel(this);
            markPatientControl(m_liveTimer);
            m_liveTimer->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 16));
            m_liveTimer->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
            m_liveTimer->show();
            if (m_timerConnection) {
                disconnect(m_timerConnection);
            }
            m_timerConnection = connect(m_timer, &QTimer::timeout, this, [this]() {
                if (m_exerciseId == QStringLiteral("5.1.1")) {
                    updateLiveTimer511();
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
            m_dataByStep.clear();
            m_table->setColumnCount(2);
            m_table->setRowCount(fragments.size());
            m_table->setHorizontalHeaderLabels({
                QStringLiteral("Фрагменты речи, фиксируемые в процессе исследования"),
                QStringLiteral("Частота употребления"),
            });
            m_table->verticalHeader()->setVisible(false);
            m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_table->horizontalHeader()->setFixedHeight(48);
            m_table->verticalHeader()->setDefaultSectionSize(28);
            m_table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
            m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
            m_table->setColumnWidth(0, 500);
            m_table->setColumnWidth(1, 171);
            m_table->setWordWrap(true);
            for (int i = 0; i < fragments.size(); ++i) {
                auto *fragItem = new QTableWidgetItem(fragments.at(i));
                fragItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                m_table->setItem(i, 0, fragItem);
                auto *freqItem = new QTableWidgetItem;
                freqItem->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                freqItem->setTextAlignment(Qt::AlignCenter);
                m_table->setItem(i, 1, freqItem);
            }
            const int tableH = m_table->horizontalHeader()->height()
                + m_table->verticalHeader()->defaultSectionSize() * fragments.size()
                + 2 * m_table->frameWidth() + 2;
            m_table->setFixedSize(671, tableH);
            markPatientControl(m_table);

            m_stimulusLabel = new QLabel(this);
            m_stimulusLabel->setAlignment(Qt::AlignCenter);
            m_stimulusLabel->setStyleSheet(QStringLiteral("background:transparent;"));
            loadStimulusPixmap521();
            m_stimulusLabel->show();

            m_liveTimer = new QLabel(this);
            markPatientControl(m_liveTimer);
            m_liveTimer->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 16));
            m_liveTimer->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
            m_liveTimer->show();
            if (m_timerConnection) {
                disconnect(m_timerConnection);
            }
            m_timerConnection = connect(m_timer, &QTimer::timeout, this, [this]() {
                if (m_exerciseId == QStringLiteral("5.2.1")) {
                    updateLiveTimer521();
                    layoutTableUi();
                }
            });
        } else {
            m_table->setColumnCount(1);
            m_table->setRowCount(1);
            m_table->setItem(0, 0, new QTableWidgetItem);
        }

        if (exerciseId != QStringLiteral("5.1.1") && exerciseId != QStringLiteral("5.2.1")) {
            m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
            m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
        layoutTableUi();
        m_table->show();
        m_stop->move(970, 70);
        m_stop->show();
        m_stop->raise();
        if (m_exerciseId == QStringLiteral("5.1.1")) {
            updateLiveTimer511();
        } else if (m_exerciseId == QStringLiteral("5.2.1")) {
            updateLiveTimer521();
        }
        m_timer->start();
        show();
        raise();
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            syncPatientPicture521();
        }
    }

    void switchStep(const QString &stepId) override {
        const QString next = stepId.trimmed();
        if (next.isEmpty() || next == m_stepId) {
            return;
        }
        // 5.2.1: запомнить таблицу текущего задания, очистить/восстановить для следующего.
        if (m_exerciseId == QStringLiteral("5.2.1") && m_table) {
            m_dataByStep.insert(m_stepId, collectAdditional());
            m_stepId = next;
            applyTableData(m_dataByStep.value(m_stepId));
            loadStimulusPixmap521();
            layoutTableUi();
            syncPatientPicture521();
            return;
        }
        m_stepId = next;
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            loadStimulusPixmap521();
            layoutTableUi();
            syncPatientPicture521();
        }
    }

    QString currentAdditionalSnapshot() const override {
        if (m_exerciseId != QStringLiteral("5.2.1")) {
            return {};
        }
        const QString step = m_stepId.trimmed().isEmpty() ? QStringLiteral("1") : m_stepId.trimmed();
        return step + QLatin1Char(';') + collectAdditional();
    }

    QMap<QString, QString> stepAdditionalMap() const override {
        QMap<QString, QString> out = m_dataByStep;
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            const QString step = m_stepId.trimmed().isEmpty() ? QStringLiteral("1") : m_stepId.trimmed();
            out.insert(step, collectAdditional());
        }
        return out;
    }

    QString collectAdditional() const {
        if (!m_table) {
            return {};
        }
        QStringList parts;
        const int valueCol = (m_exerciseId == QStringLiteral("5.1.1")
                              || m_exerciseId == QStringLiteral("5.2.1"))
            ? 1
            : 0;
        // 5.1.1: строка 0 — заголовок («Группы» / «Названные…»).
        const int startRow = m_exerciseId == QStringLiteral("5.1.1") ? 1 : 0;
        for (int r = startRow; r < m_table->rowCount(); ++r) {
            const QTableWidgetItem *item = m_table->item(r, valueCol);
            parts << (item ? item->text().trimmed() : QString());
        }
        if (m_exerciseId == QStringLiteral("5.1.1")) {
            return parts.join(QLatin1Char('['));
        }
        return parts.join(QLatin1Char(';'));
    }

    void applyTableData(const QString &joined) {
        if (!m_table) {
            return;
        }
        const QStringList parts = joined.split(QLatin1Char(';'));
        const int valueCol = m_exerciseId == QStringLiteral("5.2.1") ? 1 : 0;
        for (int r = 0; r < m_table->rowCount(); ++r) {
            QTableWidgetItem *item = m_table->item(r, valueCol);
            if (!item) {
                item = new QTableWidgetItem;
                m_table->setItem(r, valueCol, item);
            }
            item->setText(r < parts.size() ? parts.at(r) : QString());
        }
    }

    void finish() override {
        if (m_table) {
            m_table->setCurrentItem(nullptr);
            m_table->clearFocus();
        }
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            const QString step = m_stepId.trimmed().isEmpty() ? QStringLiteral("1") : m_stepId.trimmed();
            m_dataByStep.insert(step, collectAdditional());
        }
        ExerciseSessionResult result;
        result.elapsedSeconds = m_elapsed;
        result.additional = collectAdditional();
        m_timer->stop();
        if (m_patientRoot) {
            m_patientRoot->hide();
        }
        hide();
        emitFinished(result);
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        if (!display) {
            return;
        }
        // 5.1.1: второго экрана нет даже в dual.
        if (m_exerciseId == QStringLiteral("5.1.1")) {
            return;
        }
        if (m_exerciseId == QStringLiteral("5.2.1")) {
            ensurePatientPicture521(display);
            syncPatientPicture521();
            display->attachContentWidget(m_patientRoot);
            return;
        }
    }

    void layoutTableUi() {
        if (!m_table) {
            return;
        }
        if (m_exerciseId == QStringLiteral("5.2.1") && m_stimulusLabel) {
            // Как e521.Designer: table @ (51,81), picture @ (1175,106), stop @ (970,70).
            // Dual: тот же полноэкранный вид на первом мониторе, что и при одном экране.
            constexpr int kTableLeft = 51;
            constexpr int kTableTop = 81;
            m_table->move(kTableLeft, kTableTop);
            m_table->raise();

            QPixmap src = m_stimulusPixmap;
            if (!src.isNull()) {
                const int maxW = qMax(80, width() - 1175 - 24);
                const int maxH = qMax(80, height() - 106 - 24);
                if (src.width() > maxW || src.height() > maxH) {
                    src = src.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                m_stimulusLabel->setPixmap(src);
                m_stimulusLabel->setFixedSize(src.size());
                m_stimulusLabel->move(1175, 106);
            }
            m_stimulusLabel->raise();
            syncPatientPicture521();
        } else if (m_exerciseId == QStringLiteral("5.1.1")) {
            // Dual: таблица на 100 px ниже; по центру правой половины.
            const int yOff = m_sessionOptions.dualScreen ? 100 : 0;
            const int rightHalfLeft = width() / 2;
            const int rightHalfW = qMax(m_table->width(), width() - rightHalfLeft);
            const int x = rightHalfLeft + qMax(0, (rightHalfW - m_table->width()) / 2);
            m_table->move(x, 120 + yOff);
            m_table->raise();
        } else if (m_exerciseId != QStringLiteral("5.2.1")) {
            m_table->setGeometry(100, 120, qMax(400, width() - 200), qMax(300, height() - 200));
            m_table->raise();
        }
        if (m_liveTimer && m_stop
            && (m_exerciseId == QStringLiteral("5.1.1")
                || m_exerciseId == QStringLiteral("5.2.1"))) {
            m_liveTimer->adjustSize();
            const int timerX = m_stop->x() + m_stop->width() + 50;
            const int timerY = m_stop->y()
                + qMax(0, (m_stop->height() - m_liveTimer->height()) / 2);
            m_liveTimer->move(timerX, timerY);
            m_liveTimer->raise();
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

private:
    void updateLiveTimer511() {
        if (!m_liveTimer || m_exerciseId != QStringLiteral("5.1.1")) {
            return;
        }
        const QString color = m_elapsed > 160 ? QStringLiteral("#cc0000") : QStringLiteral("#000000");
        m_liveTimer->setStyleSheet(
            QStringLiteral("color:%1; font-size:16pt; font-weight:700; background:transparent;")
                .arg(color));
        m_liveTimer->setText(
            QStringLiteral("%1:%2 сек")
                .arg(m_elapsed / 60)
                .arg(m_elapsed % 60, 2, 10, QLatin1Char('0')));
        m_liveTimer->adjustSize();
    }

    void updateLiveTimer521() {
        if (!m_liveTimer || m_exerciseId != QStringLiteral("5.2.1")) {
            return;
        }
        m_liveTimer->setStyleSheet(
            QStringLiteral("color:#000000; font-size:16pt; font-weight:700; background:transparent;"));
        m_liveTimer->setText(
            QStringLiteral("%1:%2 сек")
                .arg(m_elapsed / 60)
                .arg(m_elapsed % 60, 2, 10, QLatin1Char('0')));
        m_liveTimer->adjustSize();
    }

    void loadStimulusPixmap521() {
        m_stimulusPixmap = QPixmap();
        if (m_exerciseId != QStringLiteral("5.2.1")) {
            return;
        }
        const QStringList candidates = {
            m_stepId + QStringLiteral(".png"),
            QStringLiteral("f") + m_stepId + QStringLiteral(".png"),
        };
        for (const QString &name : candidates) {
            const QString picPath = ExerciseAssets::exerciseFile(m_exerciseId, name);
            if (!picPath.isEmpty() && m_stimulusPixmap.load(picPath)) {
                break;
            }
        }
        if (m_stimulusLabel) {
            if (m_stimulusPixmap.isNull()) {
                m_stimulusLabel->clear();
            } else {
                m_stimulusLabel->setPixmap(m_stimulusPixmap);
            }
        }
    }

    void ensurePatientPicture521(PatientDisplay *display) {
        if (!display) {
            return;
        }
        // 5.1.1 и 5.2.1 делят HtmlTableRunner — сбрасываем чужой patient-view.
        if (m_patientRoot && m_patientTable) {
            m_patientRoot->deleteLater();
            m_patientRoot = nullptr;
            m_patientTable = nullptr;
            m_patientPicture = nullptr;
        }
        if (!m_patientRoot) {
            m_patientRoot = new QWidget(display);
            m_patientRoot->setStyleSheet(QStringLiteral("background:#ffffff;"));
        }
        if (!m_patientPicture) {
            m_patientPicture = new QLabel(m_patientRoot);
            m_patientPicture->setAlignment(Qt::AlignCenter);
            m_patientPicture->setStyleSheet(QStringLiteral("background:transparent;"));
        }
    }

    void syncPatientPicture521() {
        if (!m_patientRoot || !m_patientPicture || m_exerciseId != QStringLiteral("5.2.1")) {
            return;
        }
        const int pw = qMax(100, m_patientRoot->width());
        const int ph = qMax(100, m_patientRoot->height());
        if (m_stimulusPixmap.isNull()) {
            m_patientPicture->clear();
            return;
        }
        QPixmap display = m_stimulusPixmap;
        const int maxW = qMax(40, pw - 40);
        const int maxH = qMax(40, ph - 40);
        if (display.width() > maxW || display.height() > maxH) {
            display = m_stimulusPixmap.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        m_patientPicture->setPixmap(display);
        m_patientPicture->setFixedSize(display.size());
        m_patientPicture->move(qMax(0, (pw - display.width()) / 2), qMax(0, (ph - display.height()) / 2));
        m_patientPicture->show();
        m_patientPicture->raise();
    }

    QTableWidget *m_table = nullptr;
    QLabel *m_stimulusLabel = nullptr;
    QPixmap m_stimulusPixmap;
    QLabel *m_liveTimer = nullptr;
    QLabel *m_patientPicture = nullptr;
    QMetaObject::Connection m_timerConnection;
    QMap<QString, QString> m_dataByStep;
    QWidget *m_patientRoot = nullptr;
    QTableWidget *m_patientTable = nullptr;
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
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            ++m_elapsed;
            if (onTick) {
                onTick(m_elapsed);
            }
        });
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

    std::function<void(int)> onTick;
    std::function<void()> onCardsChanged;

    int elapsedSeconds() const { return m_elapsed; }

    void setCanvasBackground(const QColor &color) {
        m_background = color;
        update();
    }

    const QVector<Card> &cards() const { return m_cards; }

    void setCards(const QVector<Card> &cards, bool emitChange = false) {
        m_cards = cards;
        m_dragIndex = -1;
        m_dragging = false;
        update();
        if (emitChange && onCardsChanged) {
            onCardsChanged();
        }
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

    void stopTimer() { m_timer.stop(); }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), m_background);
        const double scale = designScale();
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
        notifyCardsChanged();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton) {
            return;
        }
        const bool wasDrag = m_dragging;
        const int index = m_dragIndex;
        m_dragging = false;
        m_dragIndex = -1;
        if (wasDrag) {
            notifyCardsChanged();
            update();
            return;
        }
        if (index < 0 || index >= m_cards.size()) {
            update();
            return;
        }
        Card &card = m_cards[index];
        if (!card.clickable) {
            return;
        }
        card.closed = !card.closed;
        update();
        notifyCardsChanged();
    }

private:
    void notifyCardsChanged() {
        if (onCardsChanged) {
            onCardsChanged();
        }
    }

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

        m_wordsLabel = new QLabel(this);
        markPatientControl(m_wordsLabel);
        m_wordsLabel->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 16));
        m_wordsLabel->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
        m_wordsLabel->setText(QStringLiteral("Игра, лес, зима, компот, работа, зубы"));
        m_wordsLabel->hide();

        m_liveTimer = new QLabel(this);
        markPatientControl(m_liveTimer);
        m_liveTimer->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 16));
        m_liveTimer->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
        m_liveTimer->hide();

        m_wordsToggle = new ImageButton(this);
        markPatientControl(m_wordsToggle);
        m_wordsToggle->hide();
        connect(m_wordsToggle, &ImageButton::clicked, this, [this]() { toggle415Words(); });

        m_canvas->onTick = [this](int elapsed) { updateLiveTimer(elapsed); };
        m_canvas->onCardsChanged = [this]() { syncCardsToPatient(); };
    }

    void bindPatientDisplay(PatientDisplay *display) override {
        m_patientDisplay = display;
        if (display && usesPatientCanvas()) {
            ensurePatient415Ui(display);
            display->attachContentWidget(m_patientRoot);
            syncCardsToPatient();
            if (usesWordsChrome()) {
                apply415WordsVisibility();
            } else if (m_patientWords) {
                m_patientWords->hide();
            }
            return;
        }
        teardownPatient415Ui();
        ExerciseRunnerWidget::bindPatientDisplay(display);
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_wordsHidden = false;
        m_syncingCards = false;

        QVector<FlipCardCanvas::Card> cards;
        const QString zeroPath = ExerciseAssets::exerciseFile(
            exerciseId == QStringLiteral("4.1.8") ? QStringLiteral("4.1.6") : exerciseId,
            QStringLiteral("zero.png"));
        QPixmap backPixmap(zeroPath);

        if (exerciseId == QStringLiteral("4.1.5")) {
            // cards.cs: 1,5,4,6,3,2; 30.2/30.4 — ниже на ~90px, чтобы не перекрывала Стоп.
            constexpr int kCardY = 81 + 90;
            const int order[] = {1, 5, 4, 6, 3, 2};
            for (int i = 0; i < 6; ++i) {
                FlipCardCanvas::Card card;
                card.front = QPixmap(ExerciseAssets::exerciseFile(
                    exerciseId, QString::number(order[i]) + QStringLiteral(".png")));
                card.back = backPixmap;
                card.x = 50 + i * 250;
                card.y = kCardY;
                card.closed = true;
                card.clickable = true;
                card.draggable = true;
                cards.append(card);
            }
            m_canvas->setCanvasBackground(Qt::white);
        } else if (exerciseId == QStringLiteral("4.1.6")) {
            // cards.cs: сетка 3×4 @910/100; 31.2 — лицом вверх; 31.3 — ниже на ~90px.
            constexpr int kCardY0 = 100 + 90;
            int count = 1;
            int linex = 910;
            int liney = kCardY0;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 4; ++col) {
                    FlipCardCanvas::Card card;
                    card.front = QPixmap(ExerciseAssets::exerciseFile(
                        exerciseId, QString::number(count) + QStringLiteral(".png")));
                    card.back = backPixmap;
                    card.x = linex;
                    card.y = liney;
                    card.closed = false; // лицевой стороной вверх
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
            clear418Table();
            m_canvas->setCanvasBackground(QColor(240, 240, 240));

            // 32.3: карточки ниже на ~100px (оба экрана).
            constexpr int kCardY0 = 20 + 100;
            int count = 1;
            int linex = 1000;
            int liney = kCardY0;
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
            // 32.5: секундомер на 1-м экране (как label2 в оригинале).
            if (m_wordsLabel) {
                m_wordsLabel->hide();
            }
            if (m_wordsToggle) {
                m_wordsToggle->hide();
            }
            if (m_liveTimer) {
                updateLiveTimer(0);
                m_liveTimer->show();
                m_liveTimer->raise();
            }
            if (m_patientCanvas) {
                m_patientCanvas->setCanvasBackground(QColor(240, 240, 240));
                syncCardsToPatient();
            }
            if (m_patientWords) {
                m_patientWords->hide();
            }
        } else if (usesWordsChrome()) {
            if (m_panel418) {
                m_panel418->hide();
            }
            if (m_hideButton) {
                m_hideButton->hide();
            }
            setup415Chrome();
            layout415Chrome();
        } else {
            if (m_panel418) {
                m_panel418->hide();
            }
            if (m_hideButton) {
                m_hideButton->hide();
            }
            hide415Chrome();
            teardownPatient415Ui();
        }

        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        // После позиции «Стоп» — секундомер на том же уровне, справа +50 px (4.1.8).
        if (exerciseId == QStringLiteral("4.1.8")) {
            layout418Ui();
        }
        show();
        raise();
    }

    void stopSession() override { finishSession(); }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_patientRoot && event->type() == QEvent::Resize) {
            layoutPatient415Ui();
        }
        return ExerciseRunnerWidget::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent *event) override {
        ExerciseRunnerWidget::resizeEvent(event);
        if (m_canvas) {
            m_canvas->setGeometry(0, 0, width(), height());
        }
        if (m_exerciseId == QStringLiteral("4.1.8")) {
            if (m_stop) {
                m_stop->move(80, 72);
            }
            layout418Ui();
            layoutPatient415Ui();
        } else if (usesWordsChrome()) {
            layout415Chrome();
            layoutPatient415Ui();
            if (m_stop) {
                m_stop->move(80, 72);
                m_stop->raise();
            }
        } else if (m_stop) {
            m_stop->move(80, 72);
            m_stop->raise();
        }
    }

private:
    bool usesWordsChrome() const {
        return m_exerciseId == QStringLiteral("4.1.5")
            || m_exerciseId == QStringLiteral("4.1.6");
    }

    bool usesPatientCanvas() const {
        return usesWordsChrome() || m_exerciseId == QStringLiteral("4.1.8");
    }

    QString wordsChromeText() const {
        if (m_exerciseId == QStringLiteral("4.1.6")) {
            return QStringLiteral("Свет, обед, ночь, ученье, дорога, молоко");
        }
        return QStringLiteral("Игра, лес, зима, компот, работа, зубы");
    }

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

    void setup415Chrome() {
        m_wordsHidden = false;
        if (m_wordsLabel) {
            m_wordsLabel->setText(wordsChromeText());
        }
        if (m_wordsToggle) {
            m_wordsToggle->setImagePath(
                ExerciseAssets::exerciseFile(QStringLiteral("4.1.5"), QStringLiteral("hide.png")));
            m_wordsToggle->show();
            m_wordsToggle->raise();
        }
        if (m_wordsLabel) {
            m_wordsLabel->show();
            m_wordsLabel->raise();
        }
        if (m_liveTimer) {
            updateLiveTimer(0);
            m_liveTimer->show();
            m_liveTimer->raise();
        }
        apply415WordsVisibility();
    }

    void hide415Chrome() {
        if (m_wordsLabel) {
            m_wordsLabel->hide();
        }
        if (m_liveTimer) {
            m_liveTimer->hide();
        }
        if (m_wordsToggle) {
            m_wordsToggle->hide();
        }
    }

    void layout415Chrome() {
        const qreal sx = width() > 0 ? width() / 1920.0 : 1.0;
        const qreal sy = height() > 0 ? height() / 1080.0 : 1.0;
        // cards.Designer: words 1192@82, phide 1339@29, timer 1479@31 (+50px вправо)
        if (m_wordsLabel) {
            m_wordsLabel->adjustSize();
            m_wordsLabel->move(qRound(1192 * sx), qRound(82 * sy));
            m_wordsLabel->raise();
        }
        if (m_wordsToggle) {
            m_wordsToggle->move(qRound(1339 * sx), qRound(29 * sy));
            m_wordsToggle->raise();
        }
        if (m_liveTimer) {
            m_liveTimer->adjustSize();
            m_liveTimer->move(qRound(1529 * sx), qRound(31 * sy));
            m_liveTimer->raise();
        }
        if (m_stop) {
            m_stop->raise();
        }
    }

    void updateLiveTimer(int elapsed) {
        if (!m_liveTimer) {
            return;
        }
        const int minutes = elapsed / 60;
        const int seconds = elapsed - minutes * 60;
        m_liveTimer->setText(QStringLiteral("%1:%2 сек").arg(minutes).arg(seconds));
        m_liveTimer->adjustSize();
    }

    void apply415WordsVisibility() {
        // Один экран: кнопка скрывает слова на этом же экране.
        // Dual: слова на 1-м всегда видны; кнопка скрывает только на 2-м (30.4 / 30.5).
        const bool dual = m_patientRoot && m_patientRoot->isVisible();
        if (m_wordsLabel) {
            if (dual) {
                m_wordsLabel->show();
            } else {
                m_wordsLabel->setVisible(!m_wordsHidden);
            }
        }
        if (m_patientWords) {
            m_patientWords->setVisible(!m_wordsHidden);
        }
        if (m_wordsToggle) {
            m_wordsToggle->setImagePath(ExerciseAssets::exerciseFile(
                QStringLiteral("4.1.5"),
                m_wordsHidden ? QStringLiteral("show.png") : QStringLiteral("hide.png")));
        }
    }

    void toggle415Words() {
        m_wordsHidden = !m_wordsHidden;
        apply415WordsVisibility();
        layout415Chrome();
    }

    void ensurePatient415Ui(PatientDisplay *display) {
        if (!display) {
            return;
        }
        if (!m_patientRoot) {
            m_patientRoot = new QWidget(display);
            m_patientRoot->setStyleSheet(QStringLiteral("background-color:#ffffff;"));
            m_patientRoot->installEventFilter(this);
            m_patientCanvas = new FlipCardCanvas(m_patientRoot);
            m_patientCanvas->setCanvasBackground(Qt::white);
            m_patientWords = new QLabel(m_patientRoot);
            m_patientWords->setFont(QFont(QStringLiteral("Microsoft Sans Serif"), 16));
            m_patientWords->setStyleSheet(QStringLiteral("color:#000000; background:transparent;"));
            m_patientWords->setText(QStringLiteral("Игра, лес, зима, компот, работа, зубы"));
            m_patientCanvas->onCardsChanged = [this]() { syncCardsFromPatient(); };
        } else if (m_patientRoot->parentWidget() != display) {
            m_patientRoot->setParent(display);
        }
        if (m_patientCanvas) {
            m_patientCanvas->setCanvasBackground(
                m_exerciseId == QStringLiteral("4.1.8") ? QColor(240, 240, 240) : Qt::white);
        }
        layoutPatient415Ui();
        m_patientRoot->show();
        if (m_patientCanvas) {
            m_patientCanvas->show();
            m_patientCanvas->lower();
        }
        if (m_patientWords) {
            if (usesWordsChrome()) {
                m_patientWords->setText(wordsChromeText());
                m_patientWords->show();
                m_patientWords->raise();
            } else {
                m_patientWords->hide();
            }
        }
    }

    void layoutPatient415Ui() {
        if (!m_patientRoot || !m_patientRoot->parentWidget()) {
            return;
        }
        QWidget *host = m_patientRoot->parentWidget();
        m_patientRoot->setGeometry(0, 0, host->width(), host->height());
        if (m_patientCanvas) {
            m_patientCanvas->setGeometry(0, 0, m_patientRoot->width(), m_patientRoot->height());
        }
        if (m_patientWords) {
            const qreal sx = m_patientRoot->width() > 0 ? m_patientRoot->width() / 1920.0 : 1.0;
            const qreal sy = m_patientRoot->height() > 0 ? m_patientRoot->height() / 1080.0 : 1.0;
            m_patientWords->adjustSize();
            m_patientWords->move(qRound(1192 * sx), qRound(82 * sy));
            m_patientWords->raise();
        }
    }

    void teardownPatient415Ui() {
        if (m_patientDisplay) {
            m_patientDisplay->attachContentWidget(nullptr);
        }
        if (m_patientRoot) {
            m_patientRoot->hide();
        }
    }

    void syncCardsToPatient() {
        if (m_syncingCards || !m_patientCanvas || !m_canvas) {
            return;
        }
        m_syncingCards = true;
        m_patientCanvas->setCards(m_canvas->cards());
        m_syncingCards = false;
    }

    void syncCardsFromPatient() {
        if (m_syncingCards || !m_patientCanvas || !m_canvas) {
            return;
        }
        m_syncingCards = true;
        m_canvas->setCards(m_patientCanvas->cards());
        m_syncingCards = false;
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
                        // Колонка «Виды помощи» (индекс 4 при 7 колонках).
                        QTableWidgetItem *item = m_table418->item(row, 4);
                        if (!item) {
                            item = new QTableWidgetItem;
                            m_table418->setItem(row, 4, item);
                        }
                        const QString help = m_helpCombo->currentText().trimmed();
                        if (help.isEmpty()) {
                            return;
                        }
                        const QString prev = item->text().trimmed();
                        item->setText(prev.isEmpty() ? help : (prev + QLatin1Char(' ') + help));
                    });

            m_table418 = new QTableWidget(5, 7, m_panel418);
            m_table418->setHorizontalHeaderLabels({
                QStringLiteral("Стимульные\nслова"),
                QStringLiteral("Выбранная\nкартинка"),
                QStringLiteral("Объяснение\nвыбора"),
                QStringLiteral("Воспроизведенное\nслово до\nпредъявления помощи"),
                QStringLiteral("Виды\nпомощи"),
                QStringLiteral("Воспроизведенное\nслово после\nпредъявления помощи"),
                QStringLiteral("Баллы"),
            });
            m_table418->verticalHeader()->setVisible(false);
            m_table418->verticalHeader()->setDefaultSectionSize(52);
            m_table418->horizontalHeader()->setMinimumHeight(78);
            m_table418->horizontalHeader()->setFixedHeight(78);
            m_table418->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter | Qt::AlignVCenter);
            m_table418->horizontalHeader()->setTextElideMode(Qt::ElideNone);
            m_table418->horizontalHeader()->setMinimumSectionSize(48);
            m_table418->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            m_table418->setWordWrap(true);
            m_table418->setEditTriggers(QAbstractItemView::AllEditTriggers);
            m_table418->setSelectionBehavior(QAbstractItemView::SelectItems);
            m_table418->setStyleSheet(QStringLiteral(
                "QHeaderView::section {"
                "  background:#f0f0f0; color:#000000; padding:3px 2px;"
                "  border:1px solid #a0a0a0;"
                "}"));
            for (int r = 0; r < 5; ++r) {
                auto *wordItem = new QTableWidgetItem(stimulusWords().at(r));
                wordItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                wordItem->setTextAlignment(Qt::AlignCenter);
                m_table418->setItem(r, 0, wordItem);
                for (int c = 1; c < 7; ++c) {
                    auto *cellItem = new QTableWidgetItem;
                    cellItem->setTextAlignment(Qt::AlignCenter);
                    if (c == 3 || c == 5) {
                        cellItem->setFlags(
                            Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                    }
                    m_table418->setItem(r, c, cellItem);
                }
            }
        }

        if (!m_hideButton) {
            m_hideButton = new ImageButton(m_panel418);
            markPatientControl(m_hideButton);
            connect(m_hideButton, &ImageButton::clicked, this, [this]() { toggle418Words(); });
        } else if (m_hideButton->parentWidget() != m_panel418) {
            m_hideButton->setParent(m_panel418);
        }
        const QString hidePath =
            ExerciseAssets::exerciseFile(QStringLiteral("4.1.8"), QStringLiteral("hide.png"));
        m_hideButton->setImagePath(hidePath);
        const QPixmap hidePm(hidePath);
        if (!hidePm.isNull()) {
            m_hideButton->setFixedSize(hidePm.size());
        }
        m_wordsHidden = false;
        apply418WordHeaderVisibility();
    }

    void clear418Table() {
        if (!m_table418) {
            return;
        }
        for (int r = 0; r < m_table418->rowCount(); ++r) {
            for (int c = 1; c < m_table418->columnCount(); ++c) {
                QTableWidgetItem *item = m_table418->item(r, c);
                if (!item) {
                    item = new QTableWidgetItem;
                    m_table418->setItem(r, c, item);
                }
                if (c == 3 || c == 5) {
                    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                }
                item->setText(QString());
            }
            QTableWidgetItem *wordItem = m_table418->item(r, 0);
            if (!wordItem) {
                wordItem = new QTableWidgetItem;
                wordItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                wordItem->setTextAlignment(Qt::AlignCenter);
                m_table418->setItem(r, 0, wordItem);
            }
            wordItem->setText(stimulusWords().value(r));
        }
        if (m_wordCombo) {
            m_wordCombo->setCurrentIndex(-1);
        }
        if (m_helpCombo) {
            m_helpCombo->setCurrentIndex(-1);
        }
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

        // Кнопка «Скрыть слова» слева от «Помощь к слову» (внутри рамки).
        // Далее как cards.Designer: label3@144, combo2@241, label4@364, combo1@448.
        const int rowY = qRound(24 * sy);
        const int labelY = qRound(28 * sy);
        int cursorX = qRound(8 * sx);
        if (m_hideButton) {
            if (m_hideButton->parentWidget() != m_panel418) {
                m_hideButton->setParent(m_panel418);
            }
            const int hideW = m_hideButton->width() > 0 ? m_hideButton->width() : qRound(120 * sx);
            const int hideH = m_hideButton->height() > 0 ? m_hideButton->height() : qRound(28 * sy);
            m_hideButton->setGeometry(cursorX, rowY, hideW, hideH);
            m_hideButton->show();
            m_hideButton->raise();
            cursorX += hideW + qRound(8 * sx);
        }
        if (m_helpWordLabel && m_wordCombo && m_helpTypeLabel && m_helpCombo) {
            const int helpWordW = qRound(95 * sx);
            m_helpWordLabel->setGeometry(cursorX, labelY, helpWordW, qRound(20 * sy));
            cursorX += helpWordW + qRound(4 * sx);
            const int wordComboW = qRound(110 * sx);
            m_wordCombo->setGeometry(cursorX, rowY, wordComboW, qRound(24 * sy));
            cursorX += wordComboW + qRound(10 * sx);
            const int helpTypeW = qRound(85 * sx);
            m_helpTypeLabel->setGeometry(cursorX, labelY, helpTypeW, qRound(20 * sy));
            cursorX += helpTypeW + qRound(4 * sx);
            const int helpComboW = qMax(qRound(160 * sx), panelW - cursorX - qRound(10 * sx));
            m_helpCombo->setGeometry(cursorX, rowY, helpComboW, qRound(24 * sy));
        }
        if (m_table418) {
            const int tableX = qRound(6 * sx);
            const int tableY = qRound(56 * sy);
            const int tableW = qMax(200, panelW - qRound(12 * sx));
            const int tableH = qMax(120, panelH - tableY - qRound(10 * sy));
            m_table418->setGeometry(tableX, tableY, tableW, tableH);
            // Пропорции table.html (85/66/213/85/246/85/30), чуть шире для полных заголовков.
            const int usableW = qMax(200, tableW - 4);
            const int w0 = qRound(usableW * 0.105);
            const int w1 = qRound(usableW * 0.095);
            const int w2 = qRound(usableW * 0.145);
            const int w3 = qRound(usableW * 0.185);
            const int w5 = qRound(usableW * 0.185);
            const int w6 = qRound(usableW * 0.055);
            const int w4 = qMax(60, usableW - w0 - w1 - w2 - w3 - w5 - w6);
            m_table418->setColumnWidth(0, w0);
            m_table418->setColumnWidth(1, w1);
            m_table418->setColumnWidth(2, w2);
            m_table418->setColumnWidth(3, w3);
            m_table418->setColumnWidth(4, w4);
            m_table418->setColumnWidth(5, w5);
            m_table418->setColumnWidth(6, w6);
        }
        // Секундомер на уровне «Стоп», справа от неё на 50 px.
        if (m_liveTimer && m_liveTimer->isVisible() && m_stop) {
            m_liveTimer->adjustSize();
            const int timerX = m_stop->x() + m_stop->width() + 50;
            const int timerY = m_stop->y()
                + qMax(0, (m_stop->height() - m_liveTimer->height()) / 2);
            m_liveTimer->move(timerX, timerY);
            m_liveTimer->raise();
        }
        m_panel418->raise();
        if (m_stop) {
            m_stop->raise();
        }
        if (m_liveTimer && m_liveTimer->isVisible()) {
            m_liveTimer->raise();
        }
    }

    void apply418WordHeaderVisibility() {
        if (!m_table418) {
            return;
        }
        for (int r = 0; r < m_table418->rowCount(); ++r) {
            QTableWidgetItem *wordItem = m_table418->item(r, 0);
            if (!wordItem) {
                continue;
            }
            wordItem->setText(m_wordsHidden ? QString() : stimulusWords().value(r));
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
            // Колонки 1..6 → sel/ex/re/hlp/rea/b (колонка 0 — стимульное слово).
            for (int c = 1; c < m_table418->columnCount(); ++c) {
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
        if (m_canvas) {
            m_canvas->stopTimer();
            m_canvas->hide();
        }
        if (m_panel418) {
            m_panel418->hide();
        }
        if (m_hideButton) {
            m_hideButton->hide();
        }
        hide415Chrome();
        teardownPatient415Ui();
        m_stop->hide();
        hide();
        emitFinished(result);
    }

    FlipCardCanvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
    ImageButton *m_hideButton = nullptr;
    ImageButton *m_wordsToggle = nullptr;
    QLabel *m_wordsLabel = nullptr;
    QLabel *m_liveTimer = nullptr;
    QGroupBox *m_panel418 = nullptr;
    QLabel *m_helpWordLabel = nullptr;
    QLabel *m_helpTypeLabel = nullptr;
    QComboBox *m_wordCombo = nullptr;
    QComboBox *m_helpCombo = nullptr;
    QTableWidget *m_table418 = nullptr;
    PatientDisplay *m_patientDisplay = nullptr;
    QWidget *m_patientRoot = nullptr;
    FlipCardCanvas *m_patientCanvas = nullptr;
    QLabel *m_patientWords = nullptr;
    bool m_wordsHidden = false;
    bool m_syncingCards = false;
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
        m_stop->onClick = [this]() {
            if (m_canvas) {
                m_canvas->abortSession();
            }
            finishSession();
        };
        connect(m_canvas, &E15Canvas::stopRequested, this, [this]() { finishSession(); });
        connect(m_canvas, &E15Canvas::exerciseCompleted, this, [this]() { finishSession(); });

        // Как shard + groupBox1 на форме e15 (Designer: shard @ 1395,9).
        m_shard = new QPushButton(QStringLiteral("Настройка уровня сложности ▾"), this);
        markPatientControl(m_shard);
        m_shard->setFlat(true);
        m_shard->setCursor(Qt::PointingHandCursor);
        m_shard->setStyleSheet(QStringLiteral(
            "QPushButton { color:#0000ee; font: bold 14px 'Microsoft Sans Serif'; text-decoration: underline;"
            " border:none; background:transparent; text-align:left; padding:0; }"
            "QPushButton:hover { color:#0000cc; }"));
        m_shard->adjustSize();
        m_shard->hide();

        m_modeGroup = new QGroupBox(QStringLiteral("Настройки"), this);
        markPatientControl(m_modeGroup);
        m_modeGroup->setFixedWidth(300);
        m_modeGroup->setStyleSheet(QStringLiteral(
            "QGroupBox { background:#ffffff; border:1px solid #000; margin-top:4px; padding:6px; }"
            "QGroupBox::title { subcontrol-origin: margin; left:6px; padding:0 3px; }"));
        auto *modeLayout = new QVBoxLayout(m_modeGroup);
        modeLayout->setSpacing(8);
        m_highlightRadio = new QRadioButton(
            QStringLiteral("Выделение («подсвечивание») фрагментов при выборе"), m_modeGroup);
        m_moveRadio = new QRadioButton(
            QStringLiteral("Перемещение фрагментов на основной рисунок для визуального сравнения узора"),
            m_modeGroup);
        m_highlightRadio->setChecked(true);
        modeLayout->addWidget(m_highlightRadio);
        modeLayout->addWidget(m_moveRadio);
        m_modeGroup->hide();

        connect(m_shard, &QPushButton::clicked, this, [this]() {
            m_modeOpen = !m_modeOpen;
            m_modeGroup->setVisible(m_modeOpen);
            m_shard->setText(m_modeOpen
                ? QStringLiteral("Настройка уровня сложности ▴")
                : QStringLiteral("Настройка уровня сложности ▾"));
            layoutModeUi();
        });
        auto applyMode = [this](bool highlight) {
            m_sessionOptions.e15SelectMode = highlight;
            if (m_canvas) {
                m_canvas->setSelectOnlyMode(highlight);
            }
        };
        connect(m_highlightRadio, &QRadioButton::toggled, this, [applyMode](bool checked) {
            if (checked) {
                applyMode(true);
            }
        });
        connect(m_moveRadio, &QRadioButton::toggled, this, [applyMode](bool checked) {
            if (checked) {
                applyMode(false);
            }
        });
    }

    void applyE15SelectMode(bool selectOnly) override {
        if (m_highlightRadio && m_moveRadio) {
            m_highlightRadio->blockSignals(true);
            m_moveRadio->blockSignals(true);
            m_highlightRadio->setChecked(selectOnly);
            m_moveRadio->setChecked(!selectOnly);
            m_highlightRadio->blockSignals(false);
            m_moveRadio->blockSignals(false);
        }
        if (m_canvas) {
            m_canvas->setSelectOnlyMode(selectOnly);
        }
    }

    void startSession(
        const QString &exerciseId,
        const ExerciseDefinition &definition,
        const QString &stepId) override {
        Q_UNUSED(definition);
        Q_UNUSED(stepId);
        m_exerciseId = exerciseId;
        m_modeOpen = false;
        if (m_modeGroup) {
            m_modeGroup->hide();
        }
        if (m_shard) {
            m_shard->setText(QStringLiteral("Настройка уровня сложности ▾"));
        }
        if (m_highlightRadio && m_moveRadio) {
            m_highlightRadio->blockSignals(true);
            m_moveRadio->blockSignals(true);
            m_highlightRadio->setChecked(m_sessionOptions.e15SelectMode);
            m_moveRadio->setChecked(!m_sessionOptions.e15SelectMode);
            m_highlightRadio->blockSignals(false);
            m_moveRadio->blockSignals(false);
        }
        m_canvas->setGeometry(0, 0, width(), height());
        m_canvas->startExercise(exerciseId, m_sessionOptions.e15SelectMode);
        m_canvas->show();
        m_canvas->lower();
        m_stop->move(80, 72);
        m_stop->show();
        m_stop->raise();
        layoutModeUi();
        m_shard->show();
        m_shard->raise();
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
        layoutModeUi();
    }

private:
    void layoutModeUi() {
        if (!m_shard) {
            return;
        }
        m_shard->adjustSize();
        // Design 1920×1080 → shard @ (1395, 9); масштабируем под оверлей.
        const double sx = width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
        const double sy = height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
        const double scale = qMin(1.0, qMin(sx, sy));
        const int offsetX = (width() - static_cast<int>(1920 * scale)) / 2;
        const int offsetY = (height() - static_cast<int>(1080 * scale)) / 2;
        const int shardX = offsetX + static_cast<int>(1395 * scale);
        const int shardY = offsetY + static_cast<int>(9 * scale);
        m_shard->move(qMax(8, shardX), qMax(4, shardY));
        if (m_modeGroup) {
            m_modeGroup->adjustSize();
            const int groupH = qMax(m_modeGroup->sizeHint().height(), 120);
            m_modeGroup->setGeometry(m_shard->x(), m_shard->y() + m_shard->height() + 4, 300, groupH);
            if (m_modeOpen) {
                m_modeGroup->raise();
            }
        }
        m_shard->raise();
    }

    void finishSession() {
        ExerciseSessionResult result;
        result.elapsedSeconds = m_canvas ? m_canvas->elapsedSeconds() : 0;
        result.doneState = m_canvas ? m_canvas->doneState() : QString();
        m_canvas->hide();
        m_stop->hide();
        if (m_shard) {
            m_shard->hide();
        }
        if (m_modeGroup) {
            m_modeGroup->hide();
        }
        m_modeOpen = false;
        hide();
        emitFinished(result);
    }

    E15Canvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
    QPushButton *m_shard = nullptr;
    QGroupBox *m_modeGroup = nullptr;
    QRadioButton *m_highlightRadio = nullptr;
    QRadioButton *m_moveRadio = nullptr;
    bool m_modeOpen = false;
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

        // 3.1.21: панель «Ответы ребенка» (как questions.cs поверх remember).
        m_questionsPanel = new QWidget(this);
        markPatientControl(m_questionsPanel);
        m_questionsPanel->setStyleSheet(QStringLiteral(
            "QWidget { background:#f8f8f8; border:1px solid #888; }"
            "QLabel { color:#000; font-family:'Microsoft Sans Serif'; font-size:13px; }"
            "QLineEdit { background:#fff; border:1px solid #666; padding:2px; }"));
        auto *qLayout = new QVBoxLayout(m_questionsPanel);
        qLayout->setContentsMargins(8, 8, 8, 8);
        qLayout->setSpacing(6);
        qLayout->addWidget(new QLabel(QStringLiteral("Ответы ребенка:"), m_questionsPanel));
        for (int i = 0; i < 3; ++i) {
            m_answerEdits[i] = new QLineEdit(m_questionsPanel);
            m_answerEdits[i]->setPlaceholderText(
                QStringLiteral("Вопрос № %1").arg(i + 1));
            qLayout->addWidget(m_answerEdits[i]);
        }
        m_questionsPanel->setFixedWidth(280);
        m_questionsPanel->hide();
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
        if (m_questionsPanel) {
            const bool showQ = exerciseId == QStringLiteral("3.1.21");
            for (QLineEdit *edit : m_answerEdits) {
                if (edit) {
                    edit->clear();
                }
            }
            m_questionsPanel->setVisible(showQ);
            if (showQ) {
                m_questionsPanel->move(12, 120);
                m_questionsPanel->raise();
            }
        }
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
        if (m_questionsPanel && m_questionsPanel->isVisible()) {
            m_questionsPanel->move(12, 120);
            m_questionsPanel->raise();
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
        if (m_exerciseId == QStringLiteral("3.1.21")) {
            const QString step = m_stepId.trimmed().isEmpty() ? QStringLiteral("1") : m_stepId.trimmed();
            QStringList parts;
            parts << step;
            for (QLineEdit *edit : m_answerEdits) {
                parts << (edit ? edit->text().trimmed() : QString());
            }
            while (parts.size() < 4) {
                parts << QString();
            }
            result.additional = parts.join(QLatin1Char(';'));
        } else {
            result.additional = m_canvas ? m_canvas->positionsSnapshot() : QString();
        }
        m_canvas->hide();
        m_stop->hide();
        if (m_removek) {
            m_removek->hide();
        }
        if (m_questionsPanel) {
            m_questionsPanel->hide();
        }
        hide();
        emitFinished(result);
    }

    RememberCanvas *m_canvas = nullptr;
    ClickableLabel *m_stop = nullptr;
    ClickableLabel *m_removek = nullptr;
    QWidget *m_questionsPanel = nullptr;
    QLineEdit *m_answerEdits[3] = {nullptr, nullptr, nullptr};
    QString m_exerciseId;
    QString m_stepId;
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
        // Превью мальчик/девочка → стартовый набор портретов задания 1.
        if (exerciseId == QStringLiteral("1.26") && !m_sessionOptions.genderPrefix.trimmed().isEmpty()) {
            m_canvas->applyGenderPrefix(m_sessionOptions.genderPrefix);
        }
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

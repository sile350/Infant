#ifndef E15CANVAS_H
#define E15CANVAS_H

#include <QPixmap>
#include <QTimer>
#include <QWidget>

class E15Canvas final : public QWidget {
    Q_OBJECT
public:
    explicit E15Canvas(QWidget *parent = nullptr);

    void startExercise(const QString &exerciseId, bool selectOnlyMode);
    void setSelectOnlyMode(bool selectOnlyMode);
    void abortSession();
    // Копия игрового состояния с парного холста (dual 1.5).
    void copyPlayStateFrom(const E15Canvas *peer);
    // 1.6: «Следующее задание» (как f16p1) — переход без проверки ответа.
    void skipToNextTask16();
    int elapsedSeconds() const { return m_elapsed; }
    bool completedSuccessfully() const { return m_completed; }
    bool isFinished() const { return m_finished; }
    QString doneState() const;
    int exerciseNumber() const { return m_exerciseNumber; }

signals:
    void stopRequested();
    void exerciseCompleted();
    void stateChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Sprite {
        QPixmap pixmap;
        QPixmap selectPixmap;
        int x = 0;
        int y = 0;
        int homeX = 0;
        int homeY = 0;
        bool selected = false;
        bool done = false;
    };

    void initExercise15();
    void initExercise16(int number);
    void loadSprites16(int number);
    void updateSnapTarget16();
    bool hitTest(int index, int x, int y) const;
    bool hitReadyButton(int x, int y) const;
    bool hitNextButton(int x, int y) const;
    void clearOtherSelected(int sel);
    void resetSelectionsForModeChange();
    void spriteChosen(int index);
    void snapSpriteHome(int index);
    void snapSpriteToTarget(int index);
    void advanceExercise16();
    void onReadyClicked();
    void setReadyVisual(bool ready);
    void failIncomplete();
    QPoint mapToDesign(const QPoint &pos) const;
    QPoint mapFromDesign(int x, int y) const;
    double scaleFactor() const;
    int targetXForIndex(int index) const;
    int targetYForIndex(int index) const;

    QString m_exerciseId;
    bool m_selectOnly = true;
    bool m_completed = false;
    bool m_finished = false;
    bool m_readyOk = true;
    QVector<Sprite> m_sprites;
    QPixmap m_pole1;
    QPixmap m_pole2;
    QPixmap m_readyPixmap;
    QPixmap m_notReadyPixmap;
    QPixmap m_nextPixmap;
    QTimer m_elapsedTimer;
    QTimer m_timeoutTimer;
    int m_elapsed = 0;
    int m_choose1 = 100;
    int m_choose2 = 100;
    int m_exerciseNumber = 1;
    int m_snapTargetX = 1560;
    int m_snapTargetY = 266;
    // Оригинал bend @ (922,309) 159×152 — между ковриками.
    static constexpr int kReadyX = 922;
    static constexpr int kReadyY = 309;
    static constexpr int kReadyW = 159;
    static constexpr int kReadyH = 152;
    // f16p1 «Следующее задание» @ (898,137).
    static constexpr int kNextX = 898;
    static constexpr int kNextY = 137;
    // f16p2 «Упражнение N из 10» @ (1276,67) — над правой картинкой.
    static constexpr int kLabelX = 1276;
    static constexpr int kLabelY = 67;
    static constexpr int kDeltaY = 100;
    static constexpr int kLine1 = 607;
    static constexpr int kLine2 = 750;
    static constexpr int kTargetX1 = 501;
    static constexpr int kTargetY1 = 399;
    static constexpr int kMaxSeconds = 70;
};

#endif

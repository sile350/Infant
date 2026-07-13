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
    int elapsedSeconds() const { return m_elapsed; }
    bool completedSuccessfully() const { return m_completed; }
    QString doneState() const;

signals:
    void stopRequested();
    void exerciseCompleted();

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
        bool multiForm = true;
    };

    void initExercise15();
    void initExercise16(int number);
    void loadSprites16(int number);
    void redraw();
    bool hitTest(int index, int x, int y) const;
    void clearOtherSelected(int sel);
    void spriteChosen(int index);
    void backChosen();
    void advanceExercise16();
    double slopeForIndex(int index) const;
    QPoint mapToDesign(const QPoint &pos) const;
    QPoint mapFromDesign(int x, int y) const;
    double scaleFactor() const;

    QString m_exerciseId;
    bool m_selectOnly = false;
    bool m_completed = false;
    QVector<Sprite> m_sprites;
    QPixmap m_pole1;
    QPixmap m_pole2;
    QPixmap m_readyPixmap;
    QPixmap m_notReadyPixmap;
    QTimer m_elapsedTimer;
    QTimer m_moveTimer;
    QTimer m_backTimer;
    QTimer m_redrawTimer;
    int m_elapsed = 0;
    int m_selected = -1;
    int m_selBack = -1;
    int m_choose1 = 100;
    int m_choose2 = 100;
    int m_exerciseNumber = 1;
    double m_k = 0;
    double m_kBack = 0;
    int m_b = 0;
    int m_bBack = 0;
    static constexpr int kDeltaY = 100;
    static constexpr int kLine1 = 607;
    static constexpr int kLine2 = 750;
    static constexpr int kTargetX = 1560;
    static constexpr int kTargetY = 266;
    static constexpr int kTargetX1 = 501;
    static constexpr int kTargetY1 = 399;
};

#endif

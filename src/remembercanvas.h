#ifndef REMEMBERCANVAS_H
#define REMEMBERCANVAS_H

#include "puzzlelayout.h"

#include <QPixmap>
#include <QTimer>
#include <QVector>
#include <QWidget>

class RememberCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit RememberCanvas(QWidget *parent = nullptr);

    void startExercise(const QString &exerciseId, const QString &stepId);
    void advanceRemovePhase();
    bool removeButtonVisible() const { return m_removeButtonVisible; }
    QString removeButtonImage() const { return m_removeButtonImage; }
    int elapsedSeconds() const { return m_elapsed; }
    QString positionsSnapshot() const;

signals:
    void stopRequested();
    void removeButtonChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Sprite {
        QPixmap pixmap;
        int x = 0;
        int y = 0;
        int slotIndex = -1;
        int homeSlotX = 0;
        QString name;
        QString spriteId;
    };

    struct HintSprite {
        QPixmap pixmap;
        int x = 0;
        int y = 35;
    };

    bool loadRememberLayout(const QString &exerciseId, const QString &stepId, PuzzleLayout *layout);
    void shuffleSprites(int baseY);
    void updateRemoveButton();
    bool hitTest(const Sprite &sprite, int x, int y) const;
    QPoint mapFromDesign(int x, int y) const;
    double scaleFactor() const;

    QString m_exerciseId;
    QString m_stepId;
    QVector<Sprite> m_sprites;
    QVector<HintSprite> m_hintSprites;
    QPixmap m_template;
    int m_templateX = 0;
    int m_templateY = 0;
    QVector<int> m_slotPositions;
    QTimer m_timer;
    int m_elapsed = 0;
    int m_moving = -1;
    bool m_dragging = false;
    QPoint m_dragOffset;
    bool m_showTemplate = true;
    int m_phase = 0;
    bool m_removeButtonVisible = false;
    QString m_removeButtonImage;
    QVector<QString> m_hintRecords;
};

#endif

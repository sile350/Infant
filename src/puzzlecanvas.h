#ifndef PUZZLECANVAS_H
#define PUZZLECANVAS_H

#include "puzzlelayout.h"
#include "exercisesession.h"

#include <QPixmap>
#include <QTimer>
#include <QWidget>

class PuzzleCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit PuzzleCanvas(QWidget *parent = nullptr);

    void loadExercise(const QString &exerciseId, const QString &stepId, const PuzzleLayout &layout);
    void applySessionOptions(const ExerciseSessionOptions &options);
    void setSelectHighlightMode(bool highlight);
    void setStoryVisible(bool visible);
    void setSpriteVisible(const QString &name, bool visible);
    void setShowTemplate(bool show);
    void setShowHint(bool show);
    void stopElapsedTimer();
    int elapsedSeconds() const { return m_elapsed; }
    QString positionsSnapshot() const;

    struct SpritePose {
        int x = 0;
        int y = 0;
        int rotateState = 0;
        bool selected = false;
        bool hidden = false;
        bool closed = false;
    };
    QVector<SpritePose> spritePoses() const;
    void applySpritePoses(const QVector<SpritePose> &poses);

signals:
    void stopRequested();
    void spritesChanged();

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
        QPixmap selectPixmap;
        int x = 0;
        int y = 0;
        int targetX = -1;
        int targetY = -1;
        int rotateState = 0;
        bool done = false;
        bool selected = false;
        bool hidden = false;
        bool returnable = false;
        bool clickable = true;
        bool closed = false;
        QString name;
        QString closedFile;
        QString openFile;
    };

    void applyRotates(int rotateW, int rotateCW);
    void rotateSpriteC(Sprite &sprite);
    void rotateSpriteTo(Sprite &sprite);
    bool hitTest(const Sprite &sprite, int x, int y) const;
    void redraw();
    void snapSprite(Sprite &sprite);
    QPoint mapFromDesign(int x, int y) const;
    QSize designSize() const;

    QString m_exerciseId;
    PuzzleLayout m_layout;
    QPixmap m_template;
    QPixmap m_template2;
    QPixmap m_hintPixmap;
    int m_hintX = 200;
    int m_hintY = 100;
    bool m_storyVisible = true;
    QPixmap m_background;
    QVector<Sprite> m_sprites;
    QTimer m_timer;
    int m_elapsed = 0;
    int m_moving = -1;
    bool m_dragging = false;
    bool m_movedDuringDrag = false;
    QPoint m_dragOffset;
    QPoint m_pressPos;
    double m_scale = 1.0;
    QPoint m_offset;
    bool m_rotateAllowed = false;
    bool m_selectMode = false;
    bool m_showTemplate = true;
    bool m_showHint = true;
    QString m_stepId;
};

#endif

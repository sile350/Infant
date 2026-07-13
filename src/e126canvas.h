#ifndef E126CANVAS_H
#define E126CANVAS_H

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>
#include <QTimer>
#include <QWidget>

class E126Canvas final : public QWidget {
    Q_OBJECT
public:
    explicit E126Canvas(QWidget *parent = nullptr);

    void startExercise(const QString &exerciseId, const QString &stepId);
    int elapsedSeconds() const { return m_elapsed; }
    QString answersSnapshot() const;

signals:
    void stopRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QRect designRect(int x, int y, int w, int h) const;
    QRect localDesignRect(int x, int y, int w, int h) const;
    void layoutUi();
    void showDemoImage();
    void showStoryImage();
    void toggleEmotionsPanel();

    QString m_exerciseId;
    QString m_stepId;
    QString m_genderPrefix;
    int m_count = 1;
    int m_elapsed = 0;
    QTimer m_timer;
    QPixmap m_mainImage;
    QPixmap m_emotionsImage;
    QLabel *m_questionLabel = nullptr;
    QLabel *m_imageLabel = nullptr;
    QLabel *m_emotionsLabel = nullptr;
    QLineEdit *m_answerEdit = nullptr;
    QComboBox *m_sceneCombo = nullptr;
    QComboBox *m_slideCombo = nullptr;
    QPushButton *m_nextButton = nullptr;
    QPushButton *m_emotionsToggle = nullptr;
    QRadioButton *m_girlRadio = nullptr;
    QRadioButton *m_boyRadio = nullptr;
    QGroupBox *m_groupBox1 = nullptr;
    QGroupBox *m_groupBox2 = nullptr;
    QGroupBox *m_groupBox3 = nullptr;
    QLabel *m_answerCaption = nullptr;
    QLabel *m_storyAnswerCaption = nullptr;
    QStringList m_answers;
    QStringList m_questions;
    bool m_emotionsVisible = false;
};

#endif

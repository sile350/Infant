#ifndef E126CANVAS_H
#define E126CANVAS_H

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

class E126Canvas final : public QWidget {
    Q_OBJECT
public:
    enum class DisplayRole {
        Specialist,
        Patient
    };

    explicit E126Canvas(QWidget *parent = nullptr);

    void setDisplayRole(DisplayRole role);
    DisplayRole displayRole() const { return m_displayRole; }

    void startExercise(const QString &exerciseId, const QString &stepId);
    // Смена задания 1↔2 без полного рестарта таймера с нуля (время продолжается).
    void switchStep(const QString &stepId);
    // Зеркало пациента: скопировать кадр/текст/подсказку со специалиста.
    void syncPatientFrom(const E126Canvas *source);
    int elapsedSeconds() const { return m_elapsed; }
    QString stepId() const { return m_stepId; }
    QString answersSnapshot() const;
    bool emotionsVisible() const { return m_emotionsVisible; }
    QString genderPrefix() const { return m_genderPrefix; }
    int currentCount() const { return m_count; }

signals:
    void stopRequested();
    void patientContentChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    double scaleX() const;
    double scaleY() const;
    QRect designRect(int x, int y, int w, int h) const;
    QRect localRect(int x, int y, int w, int h) const;
    QSize scaledSize(const QSize &native) const;
    void placePixmapLabel(QLabel *label, int designX, int designY, bool localToParent);
    void setScaledPixmap(QLabel *label, const QPixmap &nativePx);
    void clearUi();
    void applyChromeStyles();
    void buildDemoMode();
    void buildStoryMode();
    void build272Mode();
    void buildPatientMode();
    void layoutUi();
    void layoutPatientUi();
    void applyPixmap(QLabel *label, const QString &fileName, bool autoSize = true);
    void showDemoImage();
    void showStoryImage();
    void show272Image();
    void refreshPatientContent();
    void toggleEmotions();
    void advanceDemo();
    void advanceStory();
    void notifyPatientContentChanged();

    DisplayRole m_displayRole = DisplayRole::Specialist;
    QString m_exerciseId;
    QString m_stepId;
    QString m_genderPrefix = QStringLiteral("d");
    int m_count = 1;
    int m_elapsed = 0;
    QTimer m_timer;
    bool m_emotionsVisible = false;

    QGroupBox *m_groupBox1 = nullptr;
    QGroupBox *m_groupBox2 = nullptr;
    QGroupBox *m_groupBox3 = nullptr;

    QLabel *m_imageLabel = nullptr;
    QLabel *m_emotionsLabel = nullptr;
    QLabel *m_nextButton = nullptr;
    QLabel *m_showEmotionsButton = nullptr;
    QLabel *m_answerCaption = nullptr;

    QTextEdit *m_questionEdit = nullptr;
    QLineEdit *m_answerEdit = nullptr;
    QComboBox *m_sceneCombo = nullptr;
    QRadioButton *m_girlRadio = nullptr;
    QRadioButton *m_boyRadio = nullptr;

    QStringList m_answers;
    QStringList m_questions;
};

#endif

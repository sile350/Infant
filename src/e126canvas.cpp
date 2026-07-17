#include "e126canvas.h"

#include "exerciseassets.h"

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QIcon>
#include <QPixmap>

E126Canvas::E126Canvas(QWidget *parent) : QWidget(parent) {
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this]() { ++m_elapsed; });

    m_questions << QStringLiteral(
                       "Мама с Мишей пошли в садик. Миша капризничал. Какое настроение у мамы?")
                << QStringLiteral("Мама готовит ужин, дети балуются. Какое настроение у мамы?")
                << QStringLiteral("Дети сажают деревья. Какое у них настроение?")
                << QStringLiteral("Миша и Саша спорят из-за кораблика. Какое настроение?")
                << QStringLiteral("Птички заговорили с детьми. Что они почувствовали?")
                << QStringLiteral("Мама разрешила взять щенка. Какое настроение у Миши?")
                << QStringLiteral("Маша перебежала на красный свет. Что чувствует мама?")
                << QStringLiteral("Дети играют в песочнице. Какое настроение?")
                << QStringLiteral("Бабушка упала, дети смеются. Что чувствует бабушка?")
                << QStringLiteral("Зайчик увидел гусеницу. Какое настроение у зайца?")
                << QStringLiteral("Дети водят хоровод вокруг ёлки. Какое настроение?")
                << QStringLiteral("Фотографии животных ожили. Что почувствовали дети?");
}

void E126Canvas::startExercise(const QString &exerciseId, const QString &stepId) {
    m_exerciseId = exerciseId;
    m_stepId = stepId;
    m_elapsed = 0;
    m_count = 1;
    m_genderPrefix = QStringLiteral("d");
    m_emotionsVisible = false;
    m_answers = QStringList();
    for (int i = 0; i < 13; ++i) {
        m_answers.append(QString());
    }

    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral(
        "E126Canvas { background-color:#ffffff; }"
        "QGroupBox { background-color:#ffffff; border:1px solid #c0c0c0; margin-top:8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left:8px; padding:0 4px; background:#ffffff; }"
        "QLabel, QLineEdit, QComboBox, QPushButton, QRadioButton { background-color:#ffffff; color:#000000; }"
        "QComboBox QAbstractItemView { background-color:#ffffff; color:#000000; selection-background-color:#cce8ff; }"
        "QLineEdit { border:1px solid #808080; }"
        "QPushButton { border:1px solid #808080; padding:4px 10px; }"));

    qDeleteAll(findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
    m_questionLabel = nullptr;
    m_imageLabel = nullptr;
    m_emotionsLabel = nullptr;
    m_answerEdit = nullptr;
    m_sceneCombo = nullptr;
    m_slideCombo = nullptr;
    m_nextButton = nullptr;
    m_emotionsToggle = nullptr;
    m_girlRadio = nullptr;
    m_boyRadio = nullptr;
    m_groupBox1 = nullptr;
    m_groupBox2 = nullptr;
    m_groupBox3 = nullptr;
    m_answerCaption = nullptr;
    m_storyAnswerCaption = nullptr;

    if (exerciseId == QStringLiteral("1.272")) {
        m_groupBox2 = new QGroupBox(this);
        m_groupBox2->setTitle(QStringLiteral(" "));
        m_questionLabel = new QLabel(m_groupBox2);
        m_questionLabel->setWordWrap(true);
        m_imageLabel = new QLabel(m_groupBox2);
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_sceneCombo = new QComboBox(m_groupBox2);
        for (int i = 1; i <= 6; ++i) {
            m_sceneCombo->addItem(QString::number(i));
        }
        m_sceneCombo->setCurrentText(stepId);
        m_storyAnswerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox2);
        m_storyAnswerCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
        m_answerEdit = new QLineEdit(m_groupBox2);
        m_nextButton = new QPushButton(QStringLiteral("Далее"), m_groupBox2);
        connect(m_sceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_count = index + 1;
            showStoryImage();
        });
        connect(m_nextButton, &QPushButton::clicked, this, [this]() {
            if (m_count < 6 && m_sceneCombo) {
                m_answers[m_count] = m_answerEdit ? m_answerEdit->text() : QString();
                if (m_answerEdit) {
                    m_answerEdit->clear();
                }
                m_sceneCombo->setCurrentIndex(m_count);
            }
        });
        showStoryImage();
    } else if (stepId == QStringLiteral("1")) {
        m_groupBox3 = new QGroupBox(this);
        m_groupBox3->setTitle(QStringLiteral(" "));
        m_girlRadio = new QRadioButton(QStringLiteral("Девочка"), m_groupBox3);
        m_boyRadio = new QRadioButton(QStringLiteral("Мальчик"), m_groupBox3);
        m_girlRadio->setChecked(true);
        m_slideCombo = new QComboBox(m_groupBox3);
        for (int i = 1; i <= 6; ++i) {
            m_slideCombo->addItem(QString::number(i));
        }
        m_emotionsToggle = new QPushButton(QStringLiteral("Показать"), m_groupBox3);
        m_emotionsLabel = new QLabel(this);
        m_imageLabel = new QLabel(m_groupBox3);
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_groupBox1 = new QGroupBox(this);
        m_groupBox1->setTitle(QStringLiteral(" "));
        // Переносим картинку в groupBox1 после создания контейнера.
        m_imageLabel->setParent(m_groupBox1);
        m_answerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox1);
        m_answerCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
        m_answerEdit = new QLineEdit(m_groupBox1);
        m_nextButton = new QPushButton(QStringLiteral("Далее"), m_groupBox1);
        connect(m_girlRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_genderPrefix = QStringLiteral("d");
                showDemoImage();
            }
        });
        connect(m_boyRadio, &QRadioButton::toggled, this, [this](bool checked) {
            if (checked) {
                m_genderPrefix = QStringLiteral("m");
                showDemoImage();
            }
        });
        connect(m_slideCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_count = index + 1;
            showDemoImage();
        });
        connect(m_emotionsToggle, &QPushButton::clicked, this, [this]() { toggleEmotionsPanel(); });
        connect(m_nextButton, &QPushButton::clicked, this, [this]() {
            if (m_count < 6) {
                m_answers[m_count] = m_answerEdit ? m_answerEdit->text() : QString();
                if (m_answerEdit) {
                    m_answerEdit->clear();
                }
                if (m_slideCombo) {
                    m_slideCombo->setCurrentIndex(m_count);
                }
            }
        });
        showDemoImage();
    } else {
        m_groupBox2 = new QGroupBox(this);
        m_groupBox2->setTitle(QStringLiteral(" "));
        m_questionLabel = new QLabel(m_groupBox2);
        m_questionLabel->setWordWrap(true);
        m_questionLabel->setText(m_questions.value(0));
        m_imageLabel = new QLabel(m_groupBox2);
        m_imageLabel->setAlignment(Qt::AlignCenter);
        m_storyAnswerCaption = new QLabel(QStringLiteral("Ответ ребенка"), m_groupBox2);
        m_storyAnswerCaption->setStyleSheet(QStringLiteral("font-weight:bold;"));
        m_answerEdit = new QLineEdit(m_groupBox2);
        m_sceneCombo = new QComboBox(m_groupBox2);
        for (int i = 1; i <= 12; ++i) {
            m_sceneCombo->addItem(QString::number(i));
        }
        m_nextButton = new QPushButton(QStringLiteral("Следующая сцена"), m_groupBox2);
        m_emotionsLabel = new QLabel(this);
        m_emotionsToggle = new QPushButton(this);
        const QString emotionsPath = ExerciseAssets::exerciseFile(exerciseId, QStringLiteral("pemotions.png"));
        if (!emotionsPath.isEmpty()) {
            m_emotionsToggle->setIcon(QIcon(emotionsPath));
            m_emotionsToggle->setFlat(true);
            m_emotionsToggle->setFixedSize(QPixmap(emotionsPath).size());
        } else {
            m_emotionsToggle->setText(QStringLiteral("Эмоции"));
        }
        connect(m_sceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_count = index + 1;
            showStoryImage();
        });
        connect(m_nextButton, &QPushButton::clicked, this, [this]() {
            m_answers[m_count] = m_answerEdit ? m_answerEdit->text() : QString();
            if (m_answerEdit) {
                m_answerEdit->clear();
            }
            if (m_count < 12 && m_sceneCombo) {
                m_sceneCombo->setCurrentIndex(m_count);
            }
        });
        connect(m_emotionsToggle, &QPushButton::clicked, this, [this]() { toggleEmotionsPanel(); });
        showStoryImage();
        toggleEmotionsPanel();
    }

    layoutUi();
    m_timer.start();
}

QRect E126Canvas::designRect(int x, int y, int w, int h) const {
    const double sx = width() > 0 ? static_cast<double>(width()) / 1920.0 : 1.0;
    const double sy = height() > 0 ? static_cast<double>(height()) / 1080.0 : 1.0;
    return QRect(qRound(x * sx), qRound(y * sy), qMax(1, qRound(w * sx)), qMax(1, qRound(h * sy)));
}

QRect E126Canvas::localDesignRect(int x, int y, int w, int h) const {
    return designRect(x, y, w, h);
}

void E126Canvas::layoutUi() {
    if (width() <= 0 || height() <= 0) {
        return;
    }
    if (m_exerciseId == QStringLiteral("1.272")) {
        if (m_groupBox2) {
            m_groupBox2->setGeometry(designRect(38, 488, 901, 788));
        }
        if (m_questionLabel) {
            m_questionLabel->setGeometry(localDesignRect(27, 32, 713, 71));
        }
        if (m_imageLabel) {
            m_imageLabel->setGeometry(localDesignRect(38, 162, 700, 400));
        }
        if (m_storyAnswerCaption) {
            m_storyAnswerCaption->setGeometry(localDesignRect(136, 132, 95, 13));
        }
        if (m_answerEdit) {
            m_answerEdit->setGeometry(localDesignRect(26, 109, 714, 20));
        }
        if (m_nextButton) {
            m_nextButton->setGeometry(localDesignRect(765, 104, 104, 25));
        }
        if (m_sceneCombo) {
            m_sceneCombo->setGeometry(localDesignRect(67, 114, 55, 21));
        }
        return;
    }

    if (m_stepId == QStringLiteral("1")) {
        if (m_groupBox1) {
            m_groupBox1->setGeometry(designRect(47, 12, 943, 453));
        }
        if (m_groupBox3) {
            m_groupBox3->setGeometry(designRect(1396, 12, 445, 147));
        }
        if (m_imageLabel) {
            m_imageLabel->setGeometry(localDesignRect(624, 167, 147, 128));
        }
        if (m_answerCaption) {
            m_answerCaption->setGeometry(localDesignRect(273, 103, 95, 13));
        }
        if (m_answerEdit) {
            m_answerEdit->setGeometry(localDesignRect(93, 81, 516, 20));
        }
        if (m_nextButton) {
            m_nextButton->setGeometry(localDesignRect(681, 75, 104, 25));
        }
        if (m_girlRadio) {
            m_girlRadio->setGeometry(localDesignRect(6, 41, 90, 20));
        }
        if (m_boyRadio) {
            m_boyRadio->setGeometry(localDesignRect(105, 41, 90, 20));
        }
        if (m_slideCombo) {
            m_slideCombo->setGeometry(localDesignRect(67, 114, 55, 21));
        }
        if (m_emotionsToggle) {
            m_emotionsToggle->setGeometry(localDesignRect(180, 36, 104, 25));
        }
        if (m_emotionsLabel) {
            m_emotionsLabel->setGeometry(designRect(1522, 235, 300, 200));
        }
        return;
    }

    if (m_groupBox2) {
        m_groupBox2->setGeometry(designRect(38, 488, 901, 788));
    }
    if (m_questionLabel) {
        m_questionLabel->setGeometry(localDesignRect(27, 32, 713, 71));
    }
    if (m_imageLabel) {
        m_imageLabel->setGeometry(localDesignRect(38, 162, 700, 400));
    }
    if (m_storyAnswerCaption) {
        m_storyAnswerCaption->setGeometry(localDesignRect(136, 132, 95, 13));
    }
    if (m_answerEdit) {
        m_answerEdit->setGeometry(localDesignRect(26, 109, 714, 20));
    }
    if (m_nextButton) {
        m_nextButton->setGeometry(localDesignRect(765, 104, 140, 25));
    }
    if (m_sceneCombo) {
        m_sceneCombo->setGeometry(localDesignRect(67, 114, 55, 21));
    }
    if (m_emotionsToggle) {
        m_emotionsToggle->setGeometry(designRect(1522, 235, 100, 50));
    }
    if (m_emotionsLabel) {
        m_emotionsLabel->setGeometry(designRect(1200, 300, 500, 500));
    }
}

void E126Canvas::showDemoImage() {
    if (!m_imageLabel) {
        return;
    }
    const QString path = ExerciseAssets::exerciseFile(
        m_exerciseId, m_genderPrefix + QString::number(m_count) + QStringLiteral(".png"));
    if (!path.isEmpty()) {
        const QPixmap pixmap(path);
        m_imageLabel->setPixmap(pixmap);
        m_imageLabel->setFixedSize(pixmap.size());
    }
}

void E126Canvas::showStoryImage() {
    if (!m_imageLabel) {
        return;
    }
    const QString path = ExerciseAssets::exerciseFile(
        m_exerciseId, QString::number(m_count) + QStringLiteral(".png"));
    if (!path.isEmpty()) {
        const QPixmap pixmap(path);
        m_imageLabel->setPixmap(pixmap);
        m_imageLabel->setFixedSize(pixmap.size());
    }
    if (m_questionLabel && m_count - 1 < m_questions.size()) {
        m_questionLabel->setText(m_questions.at(m_count - 1));
    }
}

void E126Canvas::toggleEmotionsPanel() {
    if (!m_emotionsLabel) {
        return;
    }
    if (!m_emotionsVisible) {
        const QString file = m_genderPrefix == QStringLiteral("m") ? QStringLiteral("mem.png")
                                                                   : QStringLiteral("dem.png");
        const QString path = ExerciseAssets::exerciseFile(m_exerciseId, file);
        if (!path.isEmpty()) {
            m_emotionsLabel->setPixmap(QPixmap(path));
        }
        if (m_emotionsToggle && m_emotionsToggle->text().size() > 0) {
            m_emotionsToggle->setText(QStringLiteral("Скрыть"));
        }
        m_emotionsVisible = true;
    } else {
        m_emotionsLabel->clear();
        if (m_emotionsToggle && m_emotionsToggle->text().size() > 0) {
            m_emotionsToggle->setText(QStringLiteral("Показать"));
        }
        m_emotionsVisible = false;
    }
    layoutUi();
}

QString E126Canvas::answersSnapshot() const {
  if (m_answerEdit) {
        return m_answers.join(QLatin1Char(';')) + QLatin1Char(';') + m_answerEdit->text();
    }
    return m_answers.join(QLatin1Char(';'));
}

void E126Canvas::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    QWidget::paintEvent(event);
}

void E126Canvas::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        emit stopRequested();
    }
}

void E126Canvas::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    layoutUi();
}

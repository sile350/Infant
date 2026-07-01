#include "custommessagebox.h"

#include "imagebutton.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPixmap>

namespace {

QString locateAsset(const QString &name) {
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + "/../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../assets/sysImages",
        QCoreApplication::applicationDirPath() + "/../../../assets/sysImages",
        QDir::currentPath() + "/assets/sysImages",
        QDir::currentPath() + "/../old_project/serv9 2025/WindowsFormsApp1/bin/Debug/sysImages"
    };
    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

class MessageDialog final : public QDialog {
public:
    enum class Mode { Error, Warning, Info, Confirm };

    MessageDialog(Mode mode, const QString &message, QWidget *parent)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog), m_mode(mode) {
        setModal(true);
        constexpr int kDialogW = 393;
        constexpr int kDialogH = 224;
        setFixedSize(kDialogW, kDialogH);

        const QString background = locateAsset("popup.jpg");
        if (background.isEmpty()) {
            const QString fallback = locateAsset("popup.png");
            if (!fallback.isEmpty()) {
                setStyleSheet(QStringLiteral("QDialog { border: 1px solid #1f3f2f; background-image: url('%1'); background-repeat: no-repeat; }").arg(fallback));
            }
        } else {
            setStyleSheet(QStringLiteral("QDialog { border: 1px solid #1f3f2f; background-image: url('%1'); background-repeat: no-repeat; }").arg(background));
        }

        auto *title = new QLabel(this);
        title->setGeometry(0, 8, kDialogW, 20);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("color: white; font-family: 'Microsoft Sans Serif'; font-size: 10pt; font-style: italic; background: transparent;");

        auto *icon = new QLabel(this);
        icon->setGeometry(17, 40, 45, 41);
        icon->setScaledContents(true);

        auto *heading = new QLabel(this);
        heading->setGeometry(0, 48, kDialogW, 24);
        heading->setAlignment(Qt::AlignCenter);
        heading->setStyleSheet("color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-weight: bold; background: transparent;");

        auto *body = new QLabel(message, this);
        body->setGeometry(20, 86, kDialogW - 40, 78);
        body->setAlignment(Qt::AlignCenter);
        body->setWordWrap(true);
        const QString bodyStyle = mode == Mode::Confirm
            ? "color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; font-style: italic; background: transparent;"
            : "color: black; font-family: 'Microsoft Sans Serif'; font-size: 12pt; background: transparent;";
        body->setStyleSheet(bodyStyle);

        if (mode == Mode::Confirm) {
            title->setText("Внимание");
            heading->setText("Внимание!");
            const QString alertPath = locateAsset("alert.png");
            if (!alertPath.isEmpty()) {
                icon->setPixmap(QPixmap(alertPath));
            }

            auto *continueBtn = new ImageButton(this);
            continueBtn->setGeometry(52, 171, 120, 29);
            const QString continuePath = locateAsset("continue.png");
            if (!continuePath.isEmpty()) {
                continueBtn->setImagePath(continuePath);
            }

            auto *cancelBtn = new ImageButton(this);
            cancelBtn->setGeometry(210, 171, 120, 29);
            const QString cancelPath = locateAsset("cancel.png");
            if (!cancelPath.isEmpty()) {
                cancelBtn->setImagePath(cancelPath);
            }

            connect(continueBtn, &ImageButton::clicked, this, [this]() {
                m_confirmed = true;
                accept();
            });
            connect(cancelBtn, &ImageButton::clicked, this, [this]() {
                m_confirmed = false;
                reject();
            });
            return;
        }

        title->setText(mode == Mode::Info ? "Сообщение" : "Сообщение об ошибке!");
        heading->setText(mode == Mode::Info ? "Сообщение" : "Ошибка!");
        const QString iconPath = locateAsset(mode == Mode::Warning ? "alert.png" : "error.png");
        if (!iconPath.isEmpty()) {
            icon->setPixmap(QPixmap(iconPath));
        }

        auto *okBtn = new ImageButton(this);
        okBtn->setGeometry(160, 175, 66, 29);
        const QString okPath = locateAsset("ok.png");
        if (!okPath.isEmpty()) {
            okBtn->setImagePath(okPath);
        }
        connect(okBtn, &ImageButton::clicked, this, &QDialog::accept);
    }

    bool confirmed() const {
        return m_confirmed;
    }

private:
    Mode m_mode;
    bool m_confirmed = false;
};

} // namespace

QString CustomMessageBox::assetPath(const QString &name) {
    return locateAsset(name);
}

void CustomMessageBox::showError(QWidget *parent, const QString &message) {
    MessageDialog dialog(MessageDialog::Mode::Error, message, parent);
    dialog.exec();
}

void CustomMessageBox::showWarning(QWidget *parent, const QString &message) {
    MessageDialog dialog(MessageDialog::Mode::Warning, message, parent);
    dialog.exec();
}

void CustomMessageBox::showInfo(QWidget *parent, const QString &message) {
    MessageDialog dialog(MessageDialog::Mode::Info, message, parent);
    dialog.exec();
}

bool CustomMessageBox::askConfirm(QWidget *parent, const QString &message) {
    MessageDialog dialog(MessageDialog::Mode::Confirm, message, parent);
    dialog.exec();
    return dialog.confirmed();
}

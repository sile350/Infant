#include "licenseactivationdialog.h"

#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {

class ActivationDialog final : public QDialog {
public:
    explicit ActivationDialog(QWidget *parent)
        : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog) {
        setModal(true);
        setFixedSize(470, 176);

        auto *shadowHost = new QWidget(this);
        shadowHost->setGeometry(8, 8, 454, 160);
        shadowHost->setStyleSheet(
            "background-color: #f0f0f0;"
            "border: 1px solid #b0b0b0;"
        );

        auto *shadow = new QGraphicsDropShadowEffect(shadowHost);
        shadow->setBlurRadius(18);
        shadow->setOffset(4, 4);
        shadow->setColor(QColor(0, 0, 0, 120));
        shadowHost->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(shadowHost);
        layout->setContentsMargins(56, 24, 56, 20);
        layout->setSpacing(12);

        auto *prompt = new QLabel(
            QStringLiteral("Пожалуйста, введите ваш 10-значный ключ"),
            shadowHost
        );
        prompt->setAlignment(Qt::AlignCenter);
        prompt->setWordWrap(true);
        prompt->setStyleSheet(
            "color: black;"
            "font-family: 'Microsoft Sans Serif';"
            "font-size: 14pt;"
            "background: transparent;"
        );
        layout->addWidget(prompt);

        m_keyEdit = new QLineEdit(shadowHost);
        m_keyEdit->setMaxLength(10);
        m_keyEdit->setFixedHeight(24);
        m_keyEdit->setStyleSheet(
            "QLineEdit {"
            "  background: white;"
            "  color: black;"
            "  border: 1px solid #7f9db9;"
            "  padding: 2px 4px;"
            "  font-family: 'Microsoft Sans Serif';"
            "  font-size: 12pt;"
            "}"
        );
        layout->addWidget(m_keyEdit);

        auto *submit = new QPushButton(QStringLiteral("Отправить"), shadowHost);
        submit->setFixedSize(109, 37);
        submit->setCursor(Qt::PointingHandCursor);
        submit->setStyleSheet(
            "QPushButton {"
            "  background-color: #ececec;"
            "  color: black;"
            "  border: 1px solid #7f7f7f;"
            "  font-family: 'Microsoft Sans Serif';"
            "  font-size: 12pt;"
            "  padding: 4px 8px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #e0e0e0;"
            "}"
            "QPushButton:pressed {"
            "  background-color: #d4d4d4;"
            "}"
        );
        layout->addWidget(submit, 0, Qt::AlignHCenter);

        connect(submit, &QPushButton::clicked, this, &QDialog::accept);
        connect(m_keyEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
    }

    QString key() const {
        return m_keyEdit ? m_keyEdit->text().trimmed() : QString{};
    }

protected:
    void showEvent(QShowEvent *event) override {
        QDialog::showEvent(event);
        if (m_keyEdit) {
            m_keyEdit->setFocus();
            m_keyEdit->selectAll();
        }
    }

private:
    QLineEdit *m_keyEdit = nullptr;
};

} // namespace

QString LicenseActivationDialog::promptForKey(QWidget *parent, bool *accepted) {
    ActivationDialog dialog(parent);
    const int result = dialog.exec();
    if (accepted) {
        *accepted = result == QDialog::Accepted;
    }
    if (result != QDialog::Accepted) {
        return {};
    }
    return dialog.key();
}

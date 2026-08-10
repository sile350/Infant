#ifndef PROTOCOLEDITGUARD_H
#define PROTOCOLEDITGUARD_H

#include <functional>

class QTextEdit;
class QString;

namespace ProtocolEditGuard {

enum class Mode {
    // Полный запрет редактирования (шаблон до формирования).
    ReadOnly,
    // После формирования / страница «Протоколы»:
    // OR/HLP/Баллы/Примечание/Результат (если не protocolLockResultEdit).
    LimitedEdit
};

// Обработчик клика по ссылке «Показать изображение».
// Вернуть true, если клик обработан (открытие файла / предупреждение).
using ScanAnchorHandler = std::function<bool(const QString &anchorHref)>;

void install(QTextEdit *editor, Mode mode = Mode::LimitedEdit);
void setMode(QTextEdit *editor, Mode mode);
void setScanAnchorHandler(QTextEdit *editor, ScanAnchorHandler handler);

} // namespace ProtocolEditGuard

#endif

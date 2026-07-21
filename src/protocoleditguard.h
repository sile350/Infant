#ifndef PROTOCOLEDITGUARD_H
#define PROTOCOLEDITGUARD_H

class QTextEdit;

namespace ProtocolEditGuard {

enum class Mode {
    // Полный запрет редактирования (шаблон до формирования).
    ReadOnly,
    // После формирования / страница «Протоколы»:
    // OR/HLP/Баллы/Примечание; строка «Результат» заблокирована.
    LimitedEdit
};

void install(QTextEdit *editor, Mode mode = Mode::LimitedEdit);
void setMode(QTextEdit *editor, Mode mode);

} // namespace ProtocolEditGuard

#endif

#ifndef PROTOCOLEDITGUARD_H
#define PROTOCOLEDITGUARD_H

class QTextEdit;

namespace ProtocolEditGuard {

enum class Mode {
    // Полный запрет редактирования (вкладка «Протоколы», шаблон до формирования).
    ReadOnly,
    // После формирования: только «Результат» и «Примечание».
    LimitedEdit
};

void install(QTextEdit *editor, Mode mode = Mode::LimitedEdit);
void setMode(QTextEdit *editor, Mode mode);

} // namespace ProtocolEditGuard

#endif

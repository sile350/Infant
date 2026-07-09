/****************************************************************************
** Meta object code from reading C++ file 'onlypexercise.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../src/onlypexercise.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'onlypexercise.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_OnlyPExercise_t {
    QByteArrayData data[12];
    char stringdata0[145];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_OnlyPExercise_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_OnlyPExercise_t qt_meta_stringdata_OnlyPExercise = {
    {
QT_MOC_LITERAL(0, 0, 13), // "OnlyPExercise"
QT_MOC_LITERAL(1, 14, 8), // "finished"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 11), // "QList<bool>"
QT_MOC_LITERAL(4, 36, 7), // "answers"
QT_MOC_LITERAL(5, 44, 14), // "elapsedSeconds"
QT_MOC_LITERAL(6, 59, 14), // "pictureChanged"
QT_MOC_LITERAL(7, 74, 5), // "index"
QT_MOC_LITERAL(8, 80, 14), // "answerRecorded"
QT_MOC_LITERAL(9, 95, 7), // "correct"
QT_MOC_LITERAL(10, 103, 21), // "mirrorAnswerRequested"
QT_MOC_LITERAL(11, 125, 19) // "mirrorStopRequested"

    },
    "OnlyPExercise\0finished\0\0QList<bool>\0"
    "answers\0elapsedSeconds\0pictureChanged\0"
    "index\0answerRecorded\0correct\0"
    "mirrorAnswerRequested\0mirrorStopRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_OnlyPExercise[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   39,    2, 0x06 /* Public */,
       6,    1,   44,    2, 0x06 /* Public */,
       8,    2,   47,    2, 0x06 /* Public */,
      10,    1,   52,    2, 0x06 /* Public */,
      11,    0,   55,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,    7,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,

       0        // eod
};

void OnlyPExercise::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OnlyPExercise *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->finished((*reinterpret_cast< const QList<bool>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->pictureChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->answerRecorded((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 3: _t->mirrorAnswerRequested((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->mirrorStopRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<bool> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (OnlyPExercise::*)(const QList<bool> & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnlyPExercise::finished)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (OnlyPExercise::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnlyPExercise::pictureChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (OnlyPExercise::*)(int , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnlyPExercise::answerRecorded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (OnlyPExercise::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnlyPExercise::mirrorAnswerRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (OnlyPExercise::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&OnlyPExercise::mirrorStopRequested)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject OnlyPExercise::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_OnlyPExercise.data,
    qt_meta_data_OnlyPExercise,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *OnlyPExercise::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OnlyPExercise::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_OnlyPExercise.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int OnlyPExercise::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void OnlyPExercise::finished(const QList<bool> & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void OnlyPExercise::pictureChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void OnlyPExercise::answerRecorded(int _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void OnlyPExercise::mirrorAnswerRequested(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void OnlyPExercise::mirrorStopRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

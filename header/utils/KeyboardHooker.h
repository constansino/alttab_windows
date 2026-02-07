#ifndef ALTTAB_WINDOWS_KEYBOARDHOOKER_H
#define ALTTAB_WINDOWS_KEYBOARDHOOKER_H

#include <Windows.h>
#include <QWidget>

class KeyboardHooker {
public:
    explicit KeyboardHooker(QWidget* _receiver);
    ~KeyboardHooker(); // RAII
    inline static QWidget* receiver = nullptr;

private:
    HHOOK h_keyboard = nullptr;
};


#endif // ALTTAB_WINDOWS_KEYBOARDHOOKER_H



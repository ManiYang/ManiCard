#ifndef STYLE_SHEET_UTIL_H
#define STYLE_SHEET_UTIL_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

constexpr char propertyNameStyleClasses[] = "styleClasses";

inline void setStyleClasses(QWidget *widget, const QStringList &styleClasses) {
    widget->setProperty(propertyNameStyleClasses, styleClasses);
}

inline QString styleClassSelector(const QString &styleClassName) {
    return QString("[%1~=\"%2\"]").arg(propertyNameStyleClasses, styleClassName);
}

#endif // STYLE_SHEET_UTIL_H

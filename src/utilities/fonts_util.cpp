#include <QFontDatabase>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include "fonts_util.h"
#include "numbers_util.h"

QRectF boundingRectOfString(
        const QString &str, const QString &fontFamily, const int pointSize,
        const bool bold, const bool italic, const QWidget *widget) {
    QFont font;
    if (widget != nullptr)
        font = widget->font();

    font.setFamily(fontFamily);
    font.setPointSize(pointSize);
    font.setBold(bold);
    font.setItalic(italic);
    font.setCapitalization(QFont::MixedCase);
    font.setHintingPreference(QFont::PreferDefaultHinting);
    font.setStretch(0);

    return QFontMetricsF(font).boundingRect(str);
}

double fontSizeScaleFactor(const QWidget *widget) {
    QString text("____________");
    const double width = boundingRectOfString(text, "Arial", 16, false, false, widget).width();
    const double factor = width / 144.0;
    return nearestInteger(factor / 0.05) * 0.05;
}

QStringList getFontFamilies() {
    QStringList result;

    static const QRegularExpression re(R"(^[^\[\]]+)");
    const QStringList familiesList = QFontDatabase().families();
    for (const QString &family: familiesList) {
        const auto m = re.match(family);
        if (m.hasMatch())
            result << m.captured(0).trimmed();
    }

    return result;
}

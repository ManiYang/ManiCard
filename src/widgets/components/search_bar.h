#ifndef SEARCH_BAR_H
#define SEARCH_BAR_H

#include <QFrame>

namespace Ui {
class SearchBar;
}

class SearchBar : public QFrame
{
    Q_OBJECT
public:
    explicit SearchBar(QWidget *parent = nullptr);
    ~SearchBar();

    void setPlaceholderText(const QString &text);
    void setFontPointSize(const double fontPointSize);

    void setStyleSheet(const QString &) = delete;

    //
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void edited(const QString &text);
    void submitted(const QString &text);

private:
    Ui::SearchBar *ui;

    double fontSize {10.0}; // pt

    std::function<QString (const double fontPointSize)> getStyleSheetForLineEdit;
    void setUpConnections();

    //
    static QPixmap getSearchIconPixmap(
            const bool isDarkTheme, const double fontPointSize, const double fontSizeScaleFactor);
};

#endif // SEARCH_BAR_H

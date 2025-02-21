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

signals:
    void edited(const QString &text);
    void editingFinished(const QString &text);

private:
    Ui::SearchBar *ui;

    void setUpConnections();

    //
    static QPixmap getSearchIconPixmap(const bool isDarkTheme, const double fontSizeScaleFactor);
};

#endif // SEARCH_BAR_H

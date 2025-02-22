#ifndef CUSTOMTEXTBROWSER_H
#define CUSTOMTEXTBROWSER_H

#include <QFrame>
#include <QTextBrowser>

class CustomTextBrowser : public QFrame
{
    Q_OBJECT
public:
    explicit CustomTextBrowser(QWidget *parent = nullptr);

    void clear();
    void setOpenLinks(const bool b);
    void setHorizontalScrollBarPolicy(const Qt::ScrollBarPolicy policy);
    void setVerticalScrollBarPolicy(const Qt::ScrollBarPolicy policy);

    void setTextCursor(const QTextCursor &cursor);
    void setPlainText(const QString &text);

    QTextDocument *document() const;
    QTextCursor textCursor() const;

    //
    QSize sizeHint() const override;

signals:
    void anchorClicked(const QUrl &link);

private:
    QTextBrowser *textBrowser;
};

#endif // CUSTOMTEXTBROWSER_H

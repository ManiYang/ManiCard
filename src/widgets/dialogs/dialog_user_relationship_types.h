#ifndef DIALOG_USER_RELATIONSHIP_TYPES_H
#define DIALOG_USER_RELATIONSHIP_TYPES_H

#include <QDialog>

namespace Ui {
class DialogUserRelationshipTypes;
}

class DialogUserRelationshipTypes : public QDialog
{
    Q_OBJECT
public:
    explicit DialogUserRelationshipTypes(const QStringList relTypes, QWidget *parent = nullptr);
    ~DialogUserRelationshipTypes();

private:
    Ui::DialogUserRelationshipTypes *ui;

    void setUpConnections();
};

#endif // DIALOG_USER_RELATIONSHIP_TYPES_H

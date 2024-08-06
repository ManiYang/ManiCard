#ifndef DIALOG_CREATE_RELATIONSHIP_H
#define DIALOG_CREATE_RELATIONSHIP_H

#include <optional>
#include <QDialog>
#include "models/relationship.h"

namespace Ui {
class DialogCreateRelationship;
}

class DialogCreateRelationship : public QDialog
{
    Q_OBJECT
public:
    explicit DialogCreateRelationship(
            const int cardId, const QString &cardTitle,
            const QStringList relationshipTypesList, QWidget *parent = nullptr);
    ~DialogCreateRelationship();

    std::optional<RelationshipId> getRelationshipId() const;

private:
    Ui::DialogCreateRelationship *ui;
    const int cardId;

    void setUpWidgets(const QString &cardTitle, const QStringList &relTypesList);
    void setUpConnections();

    void setToFromState();
    void setToToState();

    void validate();
};

#endif // DIALOG_CREATE_RELATIONSHIP_H

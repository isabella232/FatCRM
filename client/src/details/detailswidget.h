#ifndef DETAILSWIDGET_H
#define DETAILSWIDGET_H

#include "ui_detailswidget.h"
#include "enums.h"
#include "details.h"

#include <kdcrmdata/sugaraccount.h>
#include <kdcrmdata/sugarlead.h>
#include <kdcrmdata/sugaropportunity.h>
#include <kdcrmdata/sugarcampaign.h>

#include <kabc/addressee.h>

#include <akonadi/item.h>
#include <QtGui/QWidget>

class DetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DetailsWidget(DetailsType type, QWidget *parent = 0);
    ~DetailsWidget();

    inline bool isModified() const
    {
        return mModified;
    }
    void setModificationsIgnored(bool b);

    void setItem(const Akonadi::Item &item);
    void clearFields();
    QMap<QString, QString> data() const;
    Details *details() const { return mDetails; }
    void setData(const QMap<QString, QString> &data);

    static Details *createDetailsForType(DetailsType type);

public Q_SLOTS:
    void saveData();

Q_SIGNALS:
    void createItem();
    void modifyItem();

protected:

private Q_SLOTS:
    void slotModified();
    void slotDiscardData();

private:
    void setModified(bool modified);
    void initialize();
    void reset();
    void setConnections();

private:
    Details *mDetails;

    QMap<QString, QString> mData;

    DetailsType mType;
    bool mModified;
    bool mCreateNew;
    bool mIgnoreModifications;
    Ui_detailswidget mUi;
    Akonadi::Item mItem;
};

#endif /* DETAILSWIDGET_H */


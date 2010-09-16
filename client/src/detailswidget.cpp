#include "detailswidget.h"
#include "sugarclient.h"
#include "accountdetails.h"
#include "leaddetails.h"
#include "contactdetails.h"
#include "opportunitydetails.h"
#include "campaigndetails.h"

#include <akonadi/item.h>

using namespace Akonadi;

DetailsWidget::DetailsWidget( DetailsType type, QWidget* parent )
    : QWidget( parent ), mType( type ), mEditing( false )
{
    mUi.setupUi( this );
    initialize();
}

DetailsWidget::~DetailsWidget()
{

}

void DetailsWidget::initialize()
{
    if ( mUi.detailsContainer->layout() )
        delete mUi.detailsContainer->layout();

    switch( mType ) {
    case Account:
    {
        mDetails = new AccountDetails;
        break;
    }
    case Opportunity:
    {
        mDetails = new OpportunityDetails;
        break;
    }
    case Contact:
    {
        mDetails = new ContactDetails;
        break;
    }
    case Lead:
    {
        mDetails = new LeadDetails;
        break;
    }
    case Campaign:
    {
        mDetails = new CampaignDetails;
        break;
    }
    default:
        return;
    }

    setConnections();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget( mDetails );
    mUi.detailsContainer->setLayout( hlayout );

}

void DetailsWidget::setConnections()
{
    // Pending(michel ) - ToDo CalendarWidget - checkBoxes
    QList<QLineEdit*> lineEdits =  mDetails->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        connect( le, SIGNAL( textChanged( const QString& ) ),
                 this, SLOT( slotEnableSaving() ) );
    QList<QComboBox*> comboBoxes =  mDetails->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes )
        connect( cb, SIGNAL( currentIndexChanged( int ) ),
                    this, SLOT( slotEnableSaving() ) );
    QList<QCheckBox*> checkBoxes =  mDetails->findChildren<QCheckBox*>();
    Q_FOREACH( QCheckBox* cb, checkBoxes )
        connect( cb, SIGNAL( toggled( bool ) ),
                    this, SLOT( slotEnableSaving() ) );
    QList<QTextEdit*> textEdits = mDetails->findChildren<QTextEdit*>();
    Q_FOREACH( QTextEdit* te, textEdits )
        connect( te, SIGNAL( textChanged() ),
                    this, SLOT( slotEnableSaving() ) );
    connect( mUi.description, SIGNAL( textChanged() ),
             this,  SLOT( slotEnableSaving() ) );
    connect( mUi.saveButton, SIGNAL( clicked() ),
             this, SLOT( slotSaveData() ) );
    mUi.saveButton->setEnabled( false );

}

void DetailsWidget::reset()
{
    // Pending(michel ) - ToDo CalendarWidget - checkBoxes
    QList<QLineEdit*> lineEdits =  mDetails->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        disconnect( le, SIGNAL( textChanged( const QString& ) ),
                 this, SLOT( slotEnableSaving() ) );
    QList<QComboBox*> comboBoxes =  mDetails->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes )
        disconnect( cb, SIGNAL( currentIndexChanged( int ) ),
                    this, SLOT( slotEnableSaving() ) );
    QList<QCheckBox*> checkBoxes =  mDetails->findChildren<QCheckBox*>();
    Q_FOREACH( QCheckBox* cb, checkBoxes )
        disconnect( cb, SIGNAL( toggled( bool ) ),
                    this, SLOT( slotEnableSaving() ) );
    QList<QTextEdit*> textEdits = mDetails->findChildren<QTextEdit*>();
    Q_FOREACH( QTextEdit* te, textEdits )
        disconnect( te, SIGNAL( textChanged() ),
                    this, SLOT( slotEnableSaving() ) );
    disconnect( mUi.description, SIGNAL( textChanged() ),
                this,  SLOT( slotEnableSaving() ) );
    mUi.saveButton->setEnabled( false );
}

void DetailsWidget::clearFields ()
{
    clearDetailsWidget();
    // reset this - label and properties
    QList<QLabel*> labels = mUi.informationGB->findChildren<QLabel*>();
    Q_FOREACH( QLabel* lab, labels ) {
        QString value = lab->objectName();
        if ( value == "modifiedBy" ) {
            lab->clear();
            lab->setProperty( "modifiedUserId", qVariantFromValue<QString>( QString() ) );
            lab->setProperty( "modifiedUserName", qVariantFromValue<QString>( QString() ) );
        }
        else if ( value == "dateEntered" ) {
            lab->clear();
            lab->setProperty( "id", qVariantFromValue<QString>( QString() ) );
            lab->setProperty( "deleted", qVariantFromValue<QString>( QString() ) );

            lab->setProperty( "contactId", qVariantFromValue<QString>( QString() ) );
            lab->setProperty( "opportunityRoleFields", qVariantFromValue<QString>( QString() ) );
            lab->setProperty( "cAcceptStatusFields",  qVariantFromValue<QString>( QString() ) );
            lab->setProperty( "mAcceptStatusFields",  qVariantFromValue<QString>( QString() ) );
        }
        else if ( value == "createdBy" ) {
            lab->clear();
            lab->setProperty( "createdBy", qVariantFromValue<QString>( QString() ) );
        }
    }

    // initialize other fields
    mUi.description->setPlainText( QString() );

    // we are creating a new account
    slotSetModifyFlag( false );
}

void DetailsWidget::clearDetailsWidget()
{
    QList<QLineEdit*> lineEdits =  mDetails->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits )
        le->setText( QString() );
    QList<QComboBox*> comboBoxes =  mDetails->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes )
        cb->setCurrentIndex( 0 );
    QList<QCheckBox*> checkBoxes =  mDetails->findChildren<QCheckBox*>();
    Q_FOREACH( QCheckBox* cb, checkBoxes )
        cb->setChecked( false );
    QList<QTextEdit*> textEdits = mDetails->findChildren<QTextEdit*>();
    Q_FOREACH( QTextEdit* te, textEdits )
        te->setPlainText( QString() );
}

void DetailsWidget::setItem (const Item &item )
{

    // new item selected reset flag and saving
    mModifyFlag = true;
    reset();

    // Pending ( michel )
    // We need an Sugar<Type> base class to handle common properties
    // and avoid the following
    QMap<QString, QString> data;

    switch( mType ) {
    case Account:
    {
        SugarAccount account = item.payload<SugarAccount>();
        data = account.data();
        break;
    }
    case Opportunity:
    {
        SugarOpportunity opportunity = item.payload<SugarOpportunity>();
        data = opportunity.data();
        break;
    }
    case Contact:
    {
        KABC::Addressee contact = item.payload<KABC::Addressee>();
        data = contactData( contact ); // Pending(michel) - we need a data method.
        break;
    }
    case Lead:
    {
        SugarLead lead = item.payload<SugarLead>();
        data = lead.data();
        break;
    }
    case Campaign:
    {
        SugarCampaign campaign = item.payload<SugarCampaign>();
        data = campaign.data();
        break;
    }
    default:
        return;
    }
    setData( data );
    setConnections();
}

void DetailsWidget::setData( QMap<QString, QString> data )
{
    QString key;

    QList<QLineEdit*> lineEdits =  mUi.informationGB->findChildren<QLineEdit*>();

    Q_FOREACH( QLineEdit* le, lineEdits ) {
        key = le->objectName();
        le->setText( data.value( key ) );
    }
    QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes ) {
        key = cb->objectName();
        cb->setCurrentIndex( cb->findText( data.value( key ) ) );
        // currency is unique an cannot be changed from the client atm
        if ( key == "currency" ) {
            cb->setCurrentIndex( 0 ); // default
            cb->setProperty( "currencyId",
                               qVariantFromValue<QString>( data.value( "currencyId" ) ) );
            cb->setProperty( "currencyName",
                               qVariantFromValue<QString>( data.value( "currencyName" ) ) );
            cb->setProperty( "currencySymbol",
                               qVariantFromValue<QString>( data.value( "currencySymbol" ) ) );
        }
    }
    QList<QTextEdit*> textEdits = mUi.informationGB->findChildren<QTextEdit*>();
    Q_FOREACH( QTextEdit* te, textEdits ) {
        key = te->objectName();
        te->setPlainText( data.value( key ) );
    }
    QList<QLabel*> labels = mUi.informationGB->findChildren<QLabel*>();
    Q_FOREACH( QLabel* lb, labels ) {
        key = lb->objectName();
        if ( key == "dateEntered" )
            lb->setText( data.value( "dateEntered" ) );
        if ( key == "modifiedBy" ) {
            if ( !data.value( "modifiedByName" ).isEmpty() )
                lb->setText( data.value( "modifiedByName" ) );
            else if ( !data.value( "modifiedBy" ).isEmpty() )
                lb->setText( data.value( "modifiedBy" ) );
            else
                lb->setText( data.value( "modifiedUserName" ) );

            lb->setProperty( "modifiedUserId",
                             qVariantFromValue<QString>( data.value( "modifiedUserId" ) ) );
            lb->setProperty( "modifiedUserName",
                             qVariantFromValue<QString>( data.value( "modifiedUserName" ) ) );
        }
        if ( key == "dateEntered" ) {
            lb->setText( data.value( "dateEntered" ) );
            lb->setProperty( "deleted",
                             qVariantFromValue<QString>( data.value( "deleted" ) ) );
            lb->setProperty( "id",
                             qVariantFromValue<QString>( data.value("id" ) ) );
            lb->setProperty( "contactId",
                             qVariantFromValue<QString>( data.value( "contactId" ) ) );
            lb->setProperty( "opportunityRoleFields",
                             qVariantFromValue<QString>( data.value( "opportunityRoleFields" ) ) );
            lb->setProperty( "cAcceptStatusFields",
                             qVariantFromValue<QString>( data.value( "cAcceptStatusFields" ) ) );
            lb->setProperty( "mAcceptStatusFields",
                             qVariantFromValue<QString>( data.value( "mAcceptStatusFields" ) ) );
        }
        if ( key == "createdBy" ) {
            if ( !data.value( "createdByName" ).isEmpty() )
                lb->setText( data.value( "createdByName" ) );
            else
                lb->setText( data.value( "createdBy" ) );
            lb->setProperty( "createdBy",
                             qVariantFromValue<QString>( data.value("createdBy" ) ) );
            lb->setProperty( "createdById",
                             qVariantFromValue<QString>( data.value( "createdById" ) ) );
        }
    }
    mUi.description->setPlainText( ( mType != Campaign )?
                                   data.value( "description" ):
                                   data.value( "content") );
}

QMap<QString, QString> DetailsWidget::data()
{
    QMap<QString, QString> currentData;
    QString key;

    QList<QLineEdit*> lineEdits =  mUi.informationGB->findChildren<QLineEdit*>();
    Q_FOREACH( QLineEdit* le, lineEdits ) {
        key = le->objectName();
        currentData[key] = le->text();
    }
    QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes ) {
        key = cb->objectName();
        currentData[key] = cb->currentText();
        if ( key == "currency" ) {
            currentData["currencyId"] = cb->property( "currencyId" ).toString();
            currentData["currencyName"] = cb->property( "currencyName" ).toString();
            currentData["currencySymbol"] = cb->property( "currencySymbol" ).toString();
        }
    }
    QList<QTextEdit*> textEdits = mUi.informationGB->findChildren<QTextEdit*>();
    Q_FOREACH( QTextEdit* te, textEdits ) {
        key = te->objectName();
        currentData[key] = te->toPlainText();
    }
    QList<QLabel*> labels = mUi.informationGB->findChildren<QLabel*>();
    Q_FOREACH( QLabel* lb, labels ) {
        key = lb->objectName();
        currentData[key] = lb->text();
        if ( key == "modifiedBy" ) {
            currentData["modifiedUserId"] = lb->property( "modifiedUserId" ).toString();
            currentData["modifiedUserName"] = lb->property( "modifiedUserName" ).toString();
        }
        if ( key == "dateEntered" ) {
            currentData["deleted"] = lb->property( "deleted" ).toString();
            currentData["id"] = lb->property( "id" ).toString();
            currentData["contactId"] = lb->property( "contactId" ).toString();
            currentData["opportunityRoleFields"] =
                lb->property( "opportunityRoleFields" ).toString();
            currentData["cAcceptStatusFields"] =
                lb->property( "opportunityRoleFields" ).toString();
            currentData["mAcceptStatusFields"] =
                lb->property( "mAcceptStatusFields" ).toString();
        }
        if ( key == "createdBy" ) {
            currentData["createdBy"] = lb->property( "createdBy" ).toString();
              currentData["createdById"] = lb->property( "createdById" ).toString();
        }
    }

    currentData["description"] = mUi.description->toPlainText();
    currentData["content"] = mUi.description->toPlainText();

    currentData["parentId"] =  accountsData().value( currentAccountName() );
    currentData["accountId"] = accountsData().value(  currentAccountName() );
    currentData["campaignId"] = campaignsData().value( currentCampaignName() );
    currentData["assignedUserId"] = assignedToData().value( currentAssignedUserName() );
    currentData["assignedToId"] = assignedToData().value( currentAssignedUserName() );
    currentData["reportsToId"] = reportsToData().value( currentReportsToName() );
    return currentData;
}

QMap<QString,QString> DetailsWidget::contactData( KABC::Addressee addressee )
{
    QMap<QString, QString> data;
    data["salutation"] = addressee.custom( "FATCRM", "X-Salutation" );
    data["firstName"] = addressee.givenName();
    data["lastName"] = addressee.familyName();
    data["title"] = addressee.title();
    data["department"] = addressee.department();
    data["accountName"] = addressee.organization();
    data["primaryEmail"] = addressee.preferredEmail();
    data["homePhone"] =addressee.phoneNumber( KABC::PhoneNumber::Home ).number();
    data["mobilePhone"] = addressee.phoneNumber( KABC::PhoneNumber::Cell ).number();
    data["officePhone"] = addressee.phoneNumber( KABC::PhoneNumber::Work ).number();
    data["otherPhone"] = addressee.phoneNumber( KABC::PhoneNumber::Car ).number();
    data["fax"] = addressee.phoneNumber( KABC::PhoneNumber::Fax ).number();

    const KABC::Address address = addressee.address( KABC::Address::Work|KABC::Address::Pref);
    data["primaryAddress"] = address.street();
    data["city"] = address.locality();
    data["state"] = address.region();
    data["postalCode"] = address.postalCode();
    data["country"] = address.country();

    const KABC::Address other = addressee.address( KABC::Address::Home );
    data["otherAddress"] = other.street();
    data["otherCity"] = other.locality();
    data["otherState"] = other.region();
    data["otherPostalCode"] = other.postalCode();
    data["otherCountry"] = other.country();
    data["birthDate"] = QDateTime( addressee.birthday() ).date().toString( QString("yyyy-MM-dd" ) );
    data["assistant"] = addressee.custom( "KADDRESSBOOK", "X-AssistantsName" );
    data["assistantPhone"] = addressee.custom( "FATCRM", "X-AssistantsPhone" );
    data["leadSource"] = addressee.custom( "FATCRM", "X-LeadSourceName" );
    data["campaign"] = addressee.custom( "FATCRM", "X-CampaignName" );
    data["assignedTo"] = addressee.custom( "FATCRM", "X-AssignedUserName" );
    data["reportsTo"] = addressee.custom( "FATCRM", "X-ReportsToUserName" );
    bool donotcall = addressee.custom( "FATCRM", "X-DoNotCall" ).isEmpty() || addressee.custom( "FATCRM", "X-DoNotCall" ) == "0" ? false : true;
    data["doNotCall"] = donotcall;
    data["description"] = addressee.note();
    data["modifiedBy"] = addressee.custom( "FATCRM", "X-ModifiedByName" );
    data["dateModified"] = addressee.custom( "FATCRM", "X-DateModified" );
    data["dateEntered"] = addressee.custom( "FATCRM", "X-DateCreated" );
    data["createdBy"] = addressee.custom( "FATCRM","X-CreatedByName" );
    data["modifiedUserId"] = addressee.custom( "FATCRM", "X-ModifiedUserId" );
    data["modifiedUserName"] = addressee.custom( "FATCRM", "X-ModifiedUserName" );
    data["contactId"] = addressee.custom( "FATCRM", "X-ContactId" );
    data["opportunityRoleFields"] = addressee.custom( "FATCRM", "X-OpportunityRoleFields" );
    data["cAcceptStatusFields"] = addressee.custom( "FATCRM", "X-CacceptStatusFields" );
    data["mAcceptStatusFields"] = addressee.custom( "FATCRM", "X-MacceptStatusFields" );
    data["deleted"] = addressee.custom( "FATCRM", "X-Deleted" );
    data["createdById"] = addressee.custom( "FATCRM", "X-CreatedById" );
    return data;
}

void DetailsWidget::slotSetModifyFlag( bool value )
{
    mModifyFlag = value;
}

void DetailsWidget::slotEnableSaving()
{
    mUi.saveButton->setEnabled( true );
}

void DetailsWidget::slotSaveData()
{
    if ( !mData.empty() )
        mData.clear();

    mData["remoteRevision"] = mUi.dateModified->text();

    QMap<QString, QString> detailsMap = data();
    QMap<QString, QString>::const_iterator i = detailsMap.constBegin();
    while ( i != detailsMap.constEnd() ) {
        mData[i.key()] = mData[i.value()];
        ++i;
    }
    connect( mUi.dateModified, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( slotResetCursor( const QString& ) ) );

    if ( !mModifyFlag )
        emit saveItem();
    else
        emit modifyItem();
}

void DetailsWidget::addAccountData( const QString &name,  const QString &id )
{
    QString dataKey;
    dataKey = mAccountsData.key( id );
    removeAccountData( dataKey );
    mAccountsData.insert( name, id );
    if ( mType != Campaign && mType != Lead ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "parentName" || key == "accountName" ) {
                if ( cb->findText( name ) < 0 )
                    cb->addItem( name );
            }
        }
    }
}

QString DetailsWidget::currentAccountName() const
{
    if ( mType != Campaign && mType != Lead ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "parentName" || key == "accountName" )
                return cb->currentText();
        }
    }
    return QString();
}

void DetailsWidget::removeAccountData( const QString &name )
{
    mAccountsData.remove( name );
    if ( mType != Campaign && mType != Lead ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "parentName" || key == "accountName" ) {
                int index = cb->findText( name );
                if ( index > 0 ) // always leave the first blank field
                    cb->removeItem( index );
            }
        }
    }
}

void DetailsWidget::addCampaignData( const QString &name,  const QString &id )
{
    QString dataKey;
    dataKey = mCampaignsData.key( id );
    removeCampaignData( dataKey );
    mCampaignsData.insert( name, id );
    if ( mType != Campaign ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "campaignName" || key == "campaign" ) {
                if ( cb->findText( name ) < 0 )
                    cb->addItem( name );
            }
        }
    }
}

QString DetailsWidget::currentCampaignName() const
{
    if ( mType != Campaign ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "campaignName" || key == "campaign" )
                return cb->currentText();
        }
    }
    return QString();
}

void DetailsWidget::removeCampaignData( const QString &name )
{
    mCampaignsData.remove( name );
    if ( mType != Campaign ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "campaignName" || key == "campaign" ) {
                int index = cb->findText( name );
                if ( index > 0 ) // always leave the first blank field
                cb->removeItem( index );
            }
        }
    }
}

void DetailsWidget::addAssignedToData( const QString &name, const QString &id )
{
    if ( mAssignedToData.values().contains( id ) )
        mAssignedToData.remove( mAssignedToData.key( id ) );
    mAssignedToData.insert( name, id );
    QString key;
    QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes ) {
        key = cb->objectName();
        if ( key == "assignedUserName" || key == "assignedTo" ) {
            if ( cb->findText( name ) < 0 )
                cb->addItem( name );
        }
    }
}

QString DetailsWidget::currentAssignedUserName() const
{
    QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
    Q_FOREACH( QComboBox* cb, comboBoxes ) {
        QString key;
        key = cb->objectName();
        if ( key == "assignedUserName" || key == "assignedTo" )
            return cb->currentText();
    }
    return QString();
}

void DetailsWidget::addReportsToData( const QString &name, const QString &id )
{
    if ( mReportsToData.values().contains( id ) )
        mReportsToData.remove( mReportsToData.key( id ) );
    mReportsToData.insert( name, id );
    if ( mType == Contact ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "reportsTo" ) {
                if ( cb->findText( name ) < 0 )
                    cb->addItem( name );
            }
        }
    }
}

QString DetailsWidget::currentReportsToName() const
{
    if ( mType == Contact ) {
        QString key;
        QList<QComboBox*> comboBoxes =  mUi.informationGB->findChildren<QComboBox*>();
        Q_FOREACH( QComboBox* cb, comboBoxes ) {
            key = cb->objectName();
            if ( key == "reportsTo" )
                return cb->currentText();
        }
    }
    return QString();
}

void DetailsWidget::slotResetCursor( const QString& text)
{
    SugarClient *w = dynamic_cast<SugarClient*>( window() );
    if ( !w )
        return;
    if ( !text.isEmpty() ) {
        do {
            QApplication::restoreOverrideCursor();
        } while ( QApplication::overrideCursor() != 0 );
        if ( !w->isEnabled() )
            w->setEnabled( true );
    }
}
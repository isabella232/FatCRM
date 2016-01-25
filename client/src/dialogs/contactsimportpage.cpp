/*
  This file is part of FatCRM, a desktop application for SugarCRM written by KDAB.

  Copyright (C) 2015-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Authors: Tobias Koenig <tobias.koenig@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "contactsimportpage.h"
#include "ui_contactsimportpage.h"

#include "filterproxymodel.h"
#include "itemstreemodel.h"

#include <Akonadi/ItemCreateJob>
#include <KJobUiDelegate>
#include <QBoxLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>

namespace {

enum ChangedFlags
{
    IsNormal = 0,
    IsNew = 1,
    IsModified = 2
};

QString markupString(const QString &text, int flags)
{
    if (flags & IsNew)
        return QString::fromLatin1("<b>%1</b>").arg(text);
    else if (flags & IsModified)
        return QString::fromLatin1("<font color=\"#0000ff\"><b>%1</b></font>").arg(text);
    else
        return text;
}

QString formattedContact(const QString &prefix, int prefixFlags,
                         const QString &givenName, int givenNameFlags,
                         const QString &familyName, int familyNameFlags,
                         const QString &emailAddress, int emailAddressFlags,
                         const QString &phoneNumber, int phoneNumberFlags,
                         const QString &address, int addressFlags)
{
    QStringList parts;

    if (!prefix.isEmpty())
        parts << markupString(prefix, prefixFlags);

    if (!givenName.isEmpty())
        parts << markupString(givenName, givenNameFlags);

    if (!familyName.isEmpty())
        parts << markupString(familyName, familyNameFlags);

    if (!emailAddress.isEmpty())
        parts << markupString(QString::fromLatin1("&lt;%1&gt;").arg(emailAddress), emailAddressFlags);

    if (!phoneNumber.isEmpty()) {
        if (!parts.isEmpty())
            parts << "<br/>";
        parts << markupString(phoneNumber, phoneNumberFlags);
    }

    if (!address.isEmpty()) {
        if (!parts.isEmpty())
            parts << "<br/>";
        parts << markupString(address, addressFlags);
    }

    return parts.join(" ");
}

QString formattedContact(const KABC::Addressee &addressee, bool withAddress = false)
{
    QStringList parts;

    if (!addressee.givenName().isEmpty())
        parts << addressee.givenName();

    if (!addressee.familyName().isEmpty())
        parts << addressee.familyName();

    if (!addressee.preferredEmail().isEmpty()) {
        if (parts.isEmpty())
            parts << addressee.preferredEmail();
        parts << QString::fromLatin1("<%1>").arg(addressee.preferredEmail());
    }

    if (withAddress) {
        const QString id = addressee.custom("FATCRM", "X-AccountId");
        const SugarAccount account = AccountRepository::instance()->accountById(id);

        QStringList addressParts;
        if (!account.name().isEmpty())
            addressParts << account.name();

        if (!account.cityForGui().isEmpty())
            addressParts << account.cityForGui();

        if (!account.countryForGui().isEmpty())
            addressParts << account.countryForGui();

        parts << addressParts.join(", ");
    }

    return parts.join(" ");
}

QString formattedAddress(const KABC::Address &address)
{
    QStringList parts;
    if (!address.street().isEmpty())
        parts << address.street();
    if (!address.locality().isEmpty())
        parts << address.locality();
    if (!address.postalCode().isEmpty())
        parts << address.postalCode();
    if (!address.country().isEmpty())
        parts << address.country();

    return parts.join(", ");
}

} // namespace

MergeWidget::UpdateCheckBoxes::UpdateCheckBoxes()
    : prefix(Q_NULLPTR)
    , givenName(Q_NULLPTR)
    , familyName(Q_NULLPTR)
    , emailAddress(Q_NULLPTR)
    , phoneNumber(Q_NULLPTR)
{
}

MergeWidget::MergeWidget(const SugarAccount &account, const KABC::Addressee &importedAddressee,
                         const QVector<MatchPair> &possibleMatches, QWidget *parent)
    : QWidget(parent)
    , mAccount(account)
    , mImportedAddressee(importedAddressee)
    , mPossibleMatches(possibleMatches)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QGroupBox *importedContactBox = new QGroupBox;
    QGroupBox *matchingContactsBox = new QGroupBox;
    QGroupBox *finalContactBox = new QGroupBox;

    mainLayout->addWidget(importedContactBox, 1);
    mainLayout->addWidget(matchingContactsBox, 2);
    mainLayout->addWidget(finalContactBox, 1);

    importedContactBox->setFixedWidth(350);
    finalContactBox->setFixedWidth(350);

    importedContactBox->setTitle(i18n("Imported Contact"));
    matchingContactsBox->setTitle(i18n("Matching Contacts"));
    finalContactBox->setTitle(i18n("Final Contact"));

    // imported contact box content
    QVBoxLayout *importedContactLayout = new QVBoxLayout(importedContactBox);
    mImportedContactLabel = new QLabel;
    mImportedContactLabel->setWordWrap(true);
    importedContactLayout->addWidget(mImportedContactLabel);

    mImportedAddressee.insertCustom("FATCRM", "X-AccountId", mAccount.id());
    mImportedContactLabel->setText(formattedContact(mImportedAddressee, true));

    if (!mImportedAddressee.custom("FATCRM", "X-Salutation").isEmpty()) {
        QCheckBox *checkBox = new QCheckBox;
        checkBox->setText(mImportedAddressee.custom("FATCRM", "X-Salutation"));
        importedContactLayout->addWidget(checkBox);
        mUpdateCheckBoxes.prefix = checkBox;
        checkBox->setChecked(true);
        connect(checkBox, SIGNAL(toggled(bool)), SLOT(updateFinalContact()));
    }

    if (!mImportedAddressee.givenName().isEmpty()) {
        QCheckBox *checkBox = new QCheckBox;
        checkBox->setText(mImportedAddressee.givenName());
        importedContactLayout->addWidget(checkBox);
        mUpdateCheckBoxes.givenName = checkBox;
        checkBox->setChecked(true);
        connect(checkBox, SIGNAL(toggled(bool)), SLOT(updateFinalContact()));
    }

    if (!mImportedAddressee.isEmpty()) {
        QCheckBox *checkBox = new QCheckBox;
        checkBox->setText(mImportedAddressee.familyName());
        importedContactLayout->addWidget(checkBox);
        mUpdateCheckBoxes.familyName = checkBox;
        checkBox->setChecked(true);
        connect(checkBox, SIGNAL(toggled(bool)), SLOT(updateFinalContact()));
    }

    if (!mImportedAddressee.preferredEmail().isEmpty()) {
        QCheckBox *checkBox = new QCheckBox;
        checkBox->setText(mImportedAddressee.preferredEmail());
        importedContactLayout->addWidget(checkBox);
        mUpdateCheckBoxes.emailAddress = checkBox;
        checkBox->setChecked(true);
        connect(checkBox, SIGNAL(toggled(bool)), SLOT(updateFinalContact()));
    }

    if (!mImportedAddressee.phoneNumber(KABC::PhoneNumber::Work).number().isEmpty()) {
        QCheckBox *checkBox = new QCheckBox;
        checkBox->setText(mImportedAddressee.phoneNumber(KABC::PhoneNumber::Work).number());
        importedContactLayout->addWidget(checkBox);
        mUpdateCheckBoxes.phoneNumber = checkBox;
        checkBox->setChecked(true);
        connect(checkBox, SIGNAL(toggled(bool)), SLOT(updateFinalContact()));
    }

    importedContactLayout->addStretch();

    // matching contacts box content
    QVBoxLayout *matchingContactsLayout = new QVBoxLayout(matchingContactsBox);
    mButtonGroup = new QButtonGroup(this);
    mButtonGroup->setExclusive(true);
    connect(mButtonGroup, SIGNAL(buttonClicked(int)), SLOT(updateFinalContact()));

    for (int i = 0; i < mPossibleMatches.count(); ++i) {
        const KABC::Addressee &possibleMatch = mPossibleMatches.at(i).contact;
        QRadioButton *button = new QRadioButton;
        button->setProperty("__index", i);
        button->setText(formattedContact(possibleMatch, true));
        mButtonGroup->addButton(button);
        matchingContactsLayout->addWidget(button);
        button->setChecked(i == 0);

        QFrame *hline = new QFrame;
        hline->setFrameShape(QFrame::HLine);
        matchingContactsLayout->addWidget(hline);
    }

    QRadioButton *button = new QRadioButton;
    button->setProperty("__index", -1);
    button->setText(i18n("Create new contact"));
    button->setChecked(mPossibleMatches.isEmpty());
    mButtonGroup->addButton(button);
    matchingContactsLayout->addWidget(button);
    matchingContactsLayout->addStretch();

    // final contact box content
    QVBoxLayout *finalContactLayout = new QVBoxLayout(finalContactBox);
    mFinalContactLabel = new QLabel;
    mFinalContactLabel->setWordWrap(true);
    finalContactLayout->addWidget(mFinalContactLabel);
    finalContactLayout->addStretch();

    updateFinalContact();
}

Akonadi::Item MergeWidget::finalItem() const
{
    QAbstractButton *button = mButtonGroup->checkedButton();
    const int index = button->property("__index").toInt();

    if (index == -1) {
        Akonadi::Item item;
        item.setMimeType(KABC::Addressee::mimeType());
        item.setPayload(mFinalContact);
        return item;
    } else {
        Akonadi::Item item = mPossibleMatches.at(index).item;
        item.setPayload(mFinalContact);
        return item;
    }
}

void MergeWidget::updateFinalContact()
{
    QAbstractButton *button = mButtonGroup->checkedButton();
    const int index = button->property("__index").toInt();

    int prefixFlags = IsNormal;
    int givenNameFlags = IsNormal;
    int familyNameFlags = IsNormal;
    int emailAddressFlags = IsNormal;
    int phoneNumberFlags = IsNormal;
    int addressFlags = IsNormal;

    if (index == -1) {
        mFinalContact = KABC::Addressee();

        if (mUpdateCheckBoxes.prefix && mUpdateCheckBoxes.prefix->isChecked()) {
            prefixFlags = IsNew;
            mFinalContact.insertCustom("FATCRM", "X-Salutation", mImportedAddressee.custom("FATCRM", "X-Salutation"));
        }

        if (mUpdateCheckBoxes.givenName && mUpdateCheckBoxes.givenName->isChecked()) {
            givenNameFlags = IsNew;
            mFinalContact.setGivenName(mImportedAddressee.givenName());
        }

        if (mUpdateCheckBoxes.familyName && mUpdateCheckBoxes.familyName->isChecked()) {
            familyNameFlags = IsNew;
            mFinalContact.setFamilyName(mImportedAddressee.familyName());
        }

        if (mUpdateCheckBoxes.emailAddress && mUpdateCheckBoxes.emailAddress->isChecked()) {
            emailAddressFlags = IsNew;
            mFinalContact.insertEmail(mImportedAddressee.preferredEmail(), true);
        }

        if (mUpdateCheckBoxes.phoneNumber && mUpdateCheckBoxes.phoneNumber->isChecked()) {
            phoneNumberFlags = IsNew;
            mFinalContact.insertPhoneNumber(mImportedAddressee.phoneNumber(KABC::PhoneNumber::Work));
        }

        KABC::Address address(KABC::Address::Work);
        if (!mAccount.shippingAddressStreet().isEmpty()) {
            address.setStreet(mAccount.shippingAddressStreet());
            address.setLocality(mAccount.shippingAddressCity());
            address.setRegion(mAccount.shippingAddressState());
            address.setPostalCode(mAccount.shippingAddressPostalcode());
            address.setCountry(mAccount.shippingAddressCountry());
        } else {
            address.setStreet(mAccount.billingAddressStreet());
            address.setLocality(mAccount.billingAddressCity());
            address.setRegion(mAccount.billingAddressState());
            address.setPostalCode(mAccount.billingAddressPostalcode());
            address.setCountry(mAccount.billingAddressCountry());
        }

        addressFlags = IsNew;
        mFinalContact.removeAddress(mFinalContact.address(KABC::Address::Work));
        mFinalContact.insertAddress(address);

        mFinalContact.insertCustom("FATCRM", "X-AccountId", mAccount.id());
    } else {
        mFinalContact = mPossibleMatches.at(index).contact;

        if (mUpdateCheckBoxes.prefix && mUpdateCheckBoxes.prefix->isChecked()) {
            if (mFinalContact.custom("FATCRM", "X-Salutation") != mImportedAddressee.custom("FATCRM", "X-Salutation")) {
                prefixFlags = (mFinalContact.prefix().isEmpty() ? IsNew : IsModified);
                mFinalContact.insertCustom("FATCRM", "X-Salutation", mImportedAddressee.custom("FATCRM", "X-Salutation"));
            }
        }

        if (mUpdateCheckBoxes.givenName && mUpdateCheckBoxes.givenName->isChecked()) {
            if (mFinalContact.givenName() != mImportedAddressee.givenName()) {
                givenNameFlags = (mFinalContact.givenName().isEmpty() ? IsNew : IsModified);
                mFinalContact.setGivenName(mImportedAddressee.givenName());
            }
        }

        if (mUpdateCheckBoxes.familyName && mUpdateCheckBoxes.familyName->isChecked()) {
            if (mFinalContact.familyName() != mImportedAddressee.familyName()) {
                familyNameFlags = (mFinalContact.familyName().isEmpty() ? IsNew : IsModified);
                mFinalContact.setFamilyName(mImportedAddressee.familyName());
            }
        }

        if (mUpdateCheckBoxes.emailAddress && mUpdateCheckBoxes.emailAddress->isChecked()) {
            if (mFinalContact.preferredEmail() != mImportedAddressee.preferredEmail()) {
                emailAddressFlags = (mFinalContact.preferredEmail().isEmpty() ? IsNew : IsModified);
                mFinalContact.setEmails(QStringList());
                mFinalContact.insertEmail(mImportedAddressee.preferredEmail(), true);
            }
        }

        if (mUpdateCheckBoxes.phoneNumber && mUpdateCheckBoxes.phoneNumber->isChecked()) {
            if (mFinalContact.phoneNumber(KABC::PhoneNumber::Work).number() != mImportedAddressee.phoneNumber(KABC::PhoneNumber::Work).number()) {
                phoneNumberFlags = (mFinalContact.phoneNumber(KABC::PhoneNumber::Work).number().isEmpty() ? IsNew : IsModified);
                mFinalContact.removePhoneNumber(mFinalContact.phoneNumber(KABC::PhoneNumber::Work));
                mFinalContact.insertPhoneNumber(mImportedAddressee.phoneNumber(KABC::PhoneNumber::Work));
            }
        }
    }

    mFinalContactLabel->setText(formattedContact(mFinalContact.custom("FATCRM", "X-Salutation"), prefixFlags,
                                                 mFinalContact.givenName(), givenNameFlags,
                                                 mFinalContact.familyName(), familyNameFlags,
                                                 mFinalContact.preferredEmail(), emailAddressFlags,
                                                 mFinalContact.phoneNumber(KABC::PhoneNumber::Work).number(), phoneNumberFlags,
                                                 formattedAddress(mFinalContact.address(KABC::Address::Work)), addressFlags));
}


ContactsImportPage::ContactsImportPage(QWidget *parent)
    : QWizardPage(parent)
    , mUi(new Ui::ContactsImportPage)
    , mContactsModel(new FilterProxyModel(Contact, this))
{
    mUi->setupUi(this);
}

ContactsImportPage::~ContactsImportPage()
{
    delete mUi;
}

void ContactsImportPage::setContactsModel(ItemsTreeModel *model)
{
    mContactsModel->setSourceModel(model);
}

void ContactsImportPage::cleanup()
{
}

bool ContactsImportPage::validatePage()
{
    QVector<Akonadi::Item> items;

    foreach (const MergeWidget *mergeWidget, findChildren<MergeWidget*>()) {
        items << mergeWidget->finalItem();
    }

    emit importedItems(items);

    return true;
}

void ContactsImportPage::setChosenContacts(const QVector<ContactsSet> &contacts)
{
    {
        QLayoutItem *layoutItem = Q_NULLPTR;
        while ((layoutItem = mUi->mainLayout->takeAt(0)) != Q_NULLPTR) {
            delete layoutItem->widget();
            delete layoutItem;
        }
    }

    foreach (const ContactsSet &contactsSet, contacts) {
        foreach (const KABC::Addressee &addressee, contactsSet.addressees) {
            QVector<MatchPair> matchByEmail;
            QVector<MatchPair> matchByName;

            for (int row = 0; row < mContactsModel->rowCount(); ++row) {
                const QModelIndex index = mContactsModel->index(row, 0);
                const Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
                const KABC::Addressee possibleMatch = item.payload<KABC::Addressee>();
                if (possibleMatch.preferredEmail() == addressee.preferredEmail()) {
                    MatchPair pair;
                    pair.contact = possibleMatch;
                    pair.item = item;
                    matchByEmail.append(pair);
                } else if (possibleMatch.givenName() == addressee.givenName() &&
                           possibleMatch.familyName() == addressee.familyName()) {
                    MatchPair pair;
                    pair.contact = possibleMatch;
                    pair.item = item;
                    matchByName.append(pair);
                }
            }

            QVector<MatchPair> allMatches;
            allMatches << matchByEmail;
            allMatches << matchByName;

            addMergeWidget(contactsSet.account, addressee, allMatches);
        }
    }
}

void ContactsImportPage::addMergeWidget(const SugarAccount &account, const KABC::Addressee &importedAddressee,
                                        const QVector<MatchPair> &possibleMatches)
{
    mUi->mainLayout->addWidget(new MergeWidget(account, importedAddressee, possibleMatches));
}

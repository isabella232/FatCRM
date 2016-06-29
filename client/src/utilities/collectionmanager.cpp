/*
  This file is part of FatCRM, a desktop application for SugarCRM written by KDAB.

  Copyright (C) 2015-2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Authors: David Faure <david.faure@kdab.com>
           Michel Boyer de la Giroday <michel.giroday@kdab.com>
           Kevin Krammer <kevin.krammer@kdab.com>

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

#include "collectionmanager.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/EntityAnnotationsAttribute>

#include "kdcrmdata/enumdefinitionattribute.h"
#include <QMessageBox>
#include <QApplication>
#include <kabc/addressee.h>
#include <sugaraccount.h>
#include <sugaropportunity.h>
#include <sugarlead.h>
#include <sugarcampaign.h>
#include <klocalizedstring.h>

using namespace Akonadi;

CollectionManager::CollectionManager(QObject *parent) :
    QObject(parent)
{
    mMainCollectionIds.resize(MaxType + 1);
}

void CollectionManager::setResource(const QByteArray &identifier)
{
    /*
     * Look for the wanted collection explicitly by listing all collections
     * of the currently selected resource, filtering by MIME type.
     * include statistics to get the number of items in each collection
     */
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
    job->fetchScope().setResource(identifier);
    job->fetchScope().setIncludeStatistics(true);
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotCollectionFetchResult(KJob*)));
}

Akonadi::Collection::Id CollectionManager::collectionIdForType(DetailsType detailsType) const
{
    return mMainCollectionIds.at(detailsType);
}

QStringList CollectionManager::supportedFields(Akonadi::Collection::Id collectionId) const
{
    return mCollectionData.value(collectionId).supportedFields;
}

EnumDefinitions CollectionManager::enumDefinitions(Akonadi::Collection::Id collectionId) const
{
    return mCollectionData.value(collectionId).enumDefinitions;
}

void CollectionManager::slotCollectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames)
{
    if (attributeNames.contains("entityannotations")) {
        readSupportedFields(collection);
    }
    if (attributeNames.contains("CRM-enumdefinitions")) { // EnumDefinitionAttribute::type()
        readEnumDefinitionAttributes(collection);
    }
}

static int orderForCollection(const QString &contentMimeType)
{
    static const char* s_orderedMimeTypes[] = {
        "application/x-vnd.kdab.crm.account",
        "application/x-vnd.kdab.crm.opportunity",
        "application/x-vnd.kdab.crm.contacts",
        "application/x-vnd.kdab.crm.note",
        "application/x-vnd.kdab.crm.email",
        "application/x-vnd.akonadi.calendar.todo",
        "inode/directory", // root folder
        "text/directory",
        "application/x-vnd.kdab.crm.document"
    };
    for (uint i = 0 ; i < sizeof(s_orderedMimeTypes) / sizeof(*s_orderedMimeTypes); ++i) {
        if (contentMimeType == s_orderedMimeTypes[i])
            return i;
    }
    kDebug() << "unexpected content mimetype for ordering:" << contentMimeType;
    return 20;
}

static bool collectionLessThan(const Collection &c1, const Collection &c2)
{
    // In my tests contentMimeTypes always had exactly one entry.
    const QString contentMimeType1 = c1.contentMimeTypes().at(0);
    const QString contentMimeType2 = c2.contentMimeTypes().at(0);

    // Now emit them in the right order for fast startup.
    // We want to see Accounts first (because required by Opportunities),
    // then Opportunities (because visible on screen), and load Notes/Emails last.

    const int order1 = orderForCollection(contentMimeType1);
    const int order2 = orderForCollection(contentMimeType2);

    return order1 < order2;
}

void CollectionManager::slotCollectionFetchResult(KJob *job)
{
    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob *>(job);

    QVector<Collection> collections;
    Q_FOREACH (const Collection &collection, fetchJob->collections()) {
        collections.append(collection);

        const QString contentMimeType = collection.contentMimeTypes().at(0);
        if (contentMimeType == QLatin1String("inode/directory")) {
            continue;
        } else if (contentMimeType == SugarAccount::mimeType()) {
            mMainCollectionIds[Account] = collection.id();
        } else if (contentMimeType == SugarOpportunity::mimeType()) {
            mMainCollectionIds[Opportunity] = collection.id();
        } else if (contentMimeType == KABC::Addressee::mimeType()) {
            mMainCollectionIds[Contact] = collection.id();
        } else if (contentMimeType == SugarLead::mimeType()) {
            mMainCollectionIds[Lead] = collection.id();
        } else if (contentMimeType == SugarCampaign::mimeType()) {
            mMainCollectionIds[Campaign] = collection.id();
        }

        //qDebug() << collection.contentMimeTypes() << "name" << collection.name();
        readSupportedFields(collection);
        readEnumDefinitionAttributes(collection);
    }

    qSort(collections.begin(), collections.end(), collectionLessThan);

    Q_FOREACH (const Collection &collection, collections) {
        //qDebug() << collection.contentMimeTypes() << "name" << collection.name();
        emit collectionResult(collection.contentMimeTypes().at(0), collection);
    }
}

// duplicated in listentriesjob.cpp
static const char s_supportedFieldsKey[] = "supportedFields";

void CollectionManager::readSupportedFields(const Collection &collection)
{
    if (!mMainCollectionIds.contains(collection.id())) {
        return;
    }
    Akonadi::EntityAnnotationsAttribute *annotationsAttribute =
            collection.attribute<Akonadi::EntityAnnotationsAttribute>();
    const QStringList fields = annotationsAttribute ? annotationsAttribute->value(s_supportedFieldsKey).split(',', QString::SkipEmptyParts) : QStringList();
    if (!fields.isEmpty()) {
        mCollectionData[collection.id()].supportedFields = fields;
    } else {
        kWarning() << "No supported fields for" << collection;
        static bool errorShown = false;
        if (!errorShown) {
            errorShown = true;
            QMessageBox::warning(qApp->activeWindow(), i18n("Internal error"), i18n("The list of fields for '%1' is not available. Creating new items will not work. Try restarting the CRM resource and synchronizing again (then restart FatCRM).", collection.name()));
        }
    }
}

void CollectionManager::readEnumDefinitionAttributes(const Collection &collection)
{
    EnumDefinitionAttribute *enumsAttr = collection.attribute<EnumDefinitionAttribute>();
    if (enumsAttr) {
        mCollectionData[collection.id()].enumDefinitions = EnumDefinitions::fromString(enumsAttr->value());
    } else {
        kWarning() << "No EnumDefinitions in collection attribute for" << collection.id() << collection.name();
        kWarning() << "Collection attributes:";
        foreach (Akonadi::Attribute *attr, collection.attributes()) {
            kWarning() << attr->type();
        }

        static bool errorShown = false;
        if (!errorShown) {
            errorShown = true;
            QMessageBox::warning(qApp->activeWindow(), i18n("Internal error"), i18n("The list of enumeration values for '%1' is not available. Comboboxes will be empty. Try restarting the CRM resource and synchronizing again, making sure at least one update is fetched (then restart FatCRM).", collection.name()));
        }
    }
}

/*
 * Copyright (C) 2010-2014 Jeremy Lainé
 * Contact: https://github.com/jlaine/qdjango
 *
 * This file is part of the QDjango Library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <QStringList>
#include <QDebug>

#include "QDjango.h"
#include "QDjangoWhere.h"
#include "QDjangoWhere_p.h"

static QString escapeLike(const QString &data)
{
    QString escaped = data;
    escaped.replace(QString("%"), QString("\\%"));
    escaped.replace(QString("_"), QString("\\_"));
    return escaped;
}

/// \cond

QDjangoWherePrivate::QDjangoWherePrivate()
    : operation(QDjangoWhere::None)
    , combine(NoCombine)
    , negate(false)
{
}

/// \endcond

/*!
    \enum QDjangoWhere::Operation
    A comparison operation on a database column value.

    \var QDjangoWhere::Operation QDjangoWhere::None
    No comparison, always returns true.

    \var QDjangoWhere::Operation QDjangoWhere::Equals
    Returns true if the column value is equal to the given value.

    \var QDjangoWhere::Operation QDjangoWhere::IEquals
    Returns true if the column value is equal to the given value (case-insensitive)

    \var QDjangoWhere::Operation QDjangoWhere::NotEquals
    Returns true if the column value is not equal to the given value.

    \var QDjangoWhere::Operation QDjangoWhere::INotEquals
    Returns true if the column value is not equal to the given value (case-insensitive).

    \var QDjangoWhere::Operation QDjangoWhere::GreaterThan,
    Returns true if the column value is greater than the given value.

    \var QDjangoWhere::Operation QDjangoWhere::LessThan,
    Returns true if the column value is less than the given value.

    \var QDjangoWhere::Operation QDjangoWhere::GreaterOrEquals,
    Returns true if the column value is greater or equal to the given value.

    \var QDjangoWhere::Operation QDjangoWhere::LessOrEquals,
    Returns true if the column value is less or equal to the given value.

    \var QDjangoWhere::Operation QDjangoWhere::StartsWith,
    Returns true if the column value starts with the given value (strings only).

    \var QDjangoWhere::Operation QDjangoWhere::IStartsWith,
    Returns true if the column value starts with the given value (strings only, case-insensitive).

    \var QDjangoWhere::Operation QDjangoWhere::EndsWith,
    Returns true if the column value ends with the given value (strings only).

    \var QDjangoWhere::Operation QDjangoWhere::IEndsWith,
    Returns true if the column value ends with the given value (strings only, case-insensitive).

    \var QDjangoWhere::Operation QDjangoWhere::Contains,
    Returns true if the column value contains the given value (strings only).

    \var QDjangoWhere::Operation QDjangoWhere::IContains,
    Returns true if the column value contains the given value (strings only, case-insensitive).

    \var QDjangoWhere::Operation QDjangoWhere::IsIn,
    Returns true if the column value is one of the given values.

    \var QDjangoWhere::Operation QDjangoWhere::IsNull
    Returns true if the column value is null.
*/

/** Constructs an empty QDjangoWhere, which expresses no constraint.
 */
QDjangoWhere::QDjangoWhere()
{
    d = new QDjangoWherePrivate;
}

/** Constructs a copy of \a other.
 */
QDjangoWhere::QDjangoWhere(const QDjangoWhere &other)
    : d(other.d)
{
}

/** Constructs a QDjangoWhere expressing a constraint on a database column.
 *
 * \param key
 * \param operation
 * \param value
 */
QDjangoWhere::QDjangoWhere(const QString &key, QDjangoWhere::Operation operation, QVariant value)
{
    d = new QDjangoWherePrivate;
    d->key = key;
    d->operation = operation;
    d->data = value;
}

/** Destroys a QDjangoWhere.
 */
QDjangoWhere::~QDjangoWhere()
{
}

/** Assigns \a other to this QDjangoWhere.
 */
QDjangoWhere& QDjangoWhere::operator=(const QDjangoWhere& other)
{
    d = other.d;
    return *this;
}

/** Negates the current QDjangoWhere.
 */
QDjangoWhere QDjangoWhere::operator!() const
{
    QDjangoWhere result;
    result.d = d;
    if (d->children.isEmpty()) {
        switch (d->operation)
        {
        case None:
        case IsIn:
        case StartsWith:
        case IStartsWith:
        case EndsWith:
        case IEndsWith:
        case Contains:
        case IContains:
            result.d->negate = !d->negate;
            break;
        case IsNull:
            // simplify !(is null) to is not null
            result.d->data = !d->data.toBool();
            break;
        case Equals:
            // simplify !(a = b) to a != b
            result.d->operation = NotEquals;
            break;
        case IEquals:
            // simplify !(a = b) to a != b
            result.d->operation = INotEquals;
            break;
        case NotEquals:
            // simplify !(a != b) to a = b
            result.d->operation = Equals;
            break;
        case INotEquals:
            // simplify !(a != b) to a = b
            result.d->operation = IEquals;
            break;
        case GreaterThan:
            // simplify !(a > b) to a <= b
            result.d->operation = LessOrEquals;
            break;
        case LessThan:
            // simplify !(a < b) to a >= b
            result.d->operation = GreaterOrEquals;
            break;
        case GreaterOrEquals:
            // simplify !(a >= b) to a < b
            result.d->operation = LessThan;
            break;
        case LessOrEquals:
            // simplify !(a <= b) to a > b
            result.d->operation = GreaterThan;
            break;
        }
    } else {
        result.d->negate = !d->negate;
    }

    return result;
}

/** Combines the current QDjangoWhere with the \a other QDjangoWhere using
 *  a logical AND.
 *
 * \param other
 */
QDjangoWhere QDjangoWhere::operator&&(const QDjangoWhere &other) const
{
    if (isAll() || other.isNone())
        return other;
    else if (isNone() || other.isAll())
        return *this;

    if (d->combine == QDjangoWherePrivate::AndCombine) {
        QDjangoWhere result = *this;
        result.d->children << other;
        return result;
    }

    QDjangoWhere result;
    result.d->combine = QDjangoWherePrivate::AndCombine;
    result.d->children << *this << other;
    return result;
}

/** Combines the current QDjangoWhere with the \a other QDjangoWhere using
 *  a logical OR.
 *
 * \param other
 */
QDjangoWhere QDjangoWhere::operator||(const QDjangoWhere &other) const
{
    if (isAll() || other.isNone())
        return *this;
    else if (isNone() || other.isAll())
        return other;

    if (d->combine == QDjangoWherePrivate::OrCombine) {
        QDjangoWhere result = *this;
        result.d->children << other;
        return result;
    }

    QDjangoWhere result;
    result.d->combine = QDjangoWherePrivate::OrCombine;
    result.d->children << *this << other;
    return result;
}

/** Bind the values associated with this QDjangoWhere to the given \a query.
 *
 * \param query
 */
void QDjangoWhere::bindValues(QDjangoQuery &query) const
{
    if (d->operation == QDjangoWhere::IsIn) {
        const QList<QVariant> values = d->data.toList();
        for (int i = 0; i < values.size(); i++)
            query.addBindValue(values[i]);
    } else if (d->operation == QDjangoWhere::IsNull) {
        // no data to bind
    } else if (d->operation == QDjangoWhere::StartsWith || d->operation == QDjangoWhere::IStartsWith) {
        query.addBindValue(escapeLike(d->data.toString()) + QString("%"));
    } else if (d->operation == QDjangoWhere::EndsWith || d->operation == QDjangoWhere::IEndsWith) {
        query.addBindValue(QString("%") + escapeLike(d->data.toString()));
    } else if (d->operation == QDjangoWhere::Contains || d->operation == QDjangoWhere::IContains) {
        query.addBindValue(QString("%") + escapeLike(d->data.toString()) + QString("%"));
    } else if (d->operation != QDjangoWhere::None) {
        query.addBindValue(d->data);
    } else {
        foreach (const QDjangoWhere &child, d->children)
            child.bindValues(query);
    }
}

/** Returns true if the current QDjangoWhere does not express any constraint.
 */
bool QDjangoWhere::isAll() const
{
    return d->combine == QDjangoWherePrivate::NoCombine && d->operation == None && d->negate == false;
}

/** Returns true if the current QDjangoWhere expressed an impossible constraint.
 */
bool QDjangoWhere::isNone() const
{
    return d->combine == QDjangoWherePrivate::NoCombine && d->operation == None && d->negate == true;
}

/** Returns the SQL code corresponding for the current QDjangoWhere.
 */
/* Note - SQLite is always case-insensitive because it can't figure out case when using non-Ascii charcters:
        https://docs.djangoproject.com/en/dev/ref/databases/#sqlite-string-matching
   Note - MySQL is only case-sensitive when the collation is set as such:
        https://code.djangoproject.com/ticket/9682

 */
QString QDjangoWhere::sql(const QSqlDatabase &db) const
{
    QDjangoDatabase::DatabaseType databaseType = QDjangoDatabase::databaseType(db);

    switch (d->operation) {
        case Equals:
            return d->key + QString(" = ?");
        case NotEquals:
            return d->key + QString(" != ?");
        case GreaterThan:
            return d->key + QString(" > ?");
        case LessThan:
            return d->key + QString(" < ?");
        case GreaterOrEquals:
            return d->key + QString(" >= ?");
        case LessOrEquals:
            return d->key + QString(" <= ?");
        case IsIn:
        {
            QStringList bits;
            for (int i = 0; i < d->data.toList().size(); i++)
                bits << QString("?");
            if (d->negate)
                return d->key + QString(" NOT IN (%1)").arg(bits.join(QString(", ")));
            else
                return d->key + QString(" IN (%1)").arg(bits.join(QString(", ")));
        }
        case IsNull:
            return d->key + QString(d->data.toBool() ? " IS NULL" : " IS NOT NULL");
        case StartsWith:
        case EndsWith:
        case Contains:
        {
            QString op;
            if (databaseType == QDjangoDatabase::MySqlServer)
                op = QString(d->negate ? "NOT LIKE BINARY" : "LIKE BINARY");
            else
                op = QString(d->negate ? "NOT LIKE" : "LIKE");
            if (databaseType == QDjangoDatabase::SQLite)
                return d->key + QString(" ") + op + QString(" ? ESCAPE '\\'");
            else
                return d->key + QString(" ") + op + QString(" ?");
        }
        case IStartsWith:
        case IEndsWith:
        case IContains:
        case IEquals:
        {
            const QString op = QString(d->negate ? "NOT LIKE" : "LIKE");
            if (databaseType == QDjangoDatabase::SQLite)
                return d->key + QString(" ") + op + QString(" ? ESCAPE '\\'");
            else if (databaseType == QDjangoDatabase::PostgreSQL)
                return QString("UPPER(") + d->key + QString("::text) ") + op + QString(" UPPER(?)");
            else
                return d->key + QString(" ") + op + QString(" ?");
        }
        case INotEquals:
        {
            const QString op = QString(d->negate ? "LIKE" : "NOT LIKE");
            if (databaseType == QDjangoDatabase::SQLite)
                return d->key + QString(" ") + op + QString(" ? ESCAPE '\\'");
            else if (databaseType == QDjangoDatabase::PostgreSQL)
                return QString("UPPER(") + d->key + QString("::text) ") + op + QString(" UPPER(?)");
            else
                return d->key + QString(" ") + op + QString(" ?");
        }
        case None:
            if (d->combine == QDjangoWherePrivate::NoCombine) {
                return d->negate ? QString("1 != 0") : QString();
            } else {
                QStringList bits;
                foreach (const QDjangoWhere &child, d->children) {
                    QString atom = child.sql(db);
                    if (child.d->children.isEmpty())
                        bits << atom;
                    else
                        bits << QString("(%1)").arg(atom);
                }

                QString combined;
                if (d->combine == QDjangoWherePrivate::AndCombine)
                    combined = bits.join(QString(" AND "));
                else if (d->combine == QDjangoWherePrivate::OrCombine)
                    combined = bits.join(QString(" OR "));
                if (d->negate)
                    combined = QString("NOT (%1)").arg(combined);
                return combined;
            }
    }

    return QString();
}

QString QDjangoWhere::toString() const
{
    if (d->combine == QDjangoWherePrivate::NoCombine) {
        return QString("QDjangoWhere(")
                  + "key=\"" + d->key + "\""
                  + ", operation=\"" + QDjangoWherePrivate::operationToString(d->operation) + "\""
                  + ", value=\"" + d->data.toString() + "\""
                  + ", negate=" + (d->negate ? "true":"false")
                  + ")";
    } else {
        QStringList bits;
        foreach (const QDjangoWhere &childWhere, d->children) {
            bits.append(childWhere.toString());
        }
        if (d->combine == QDjangoWherePrivate::OrCombine) {
            return bits.join(" || ");
        } else {
            return bits.join(" && ");
        }
    }
}
QString QDjangoWherePrivate::operationToString(QDjangoWhere::Operation operation)
{
    switch (operation) {
    case QDjangoWhere::Equals: return QString("Equals");
    case QDjangoWhere::IEquals: return QString("IEquals");
    case QDjangoWhere::NotEquals: return QString("NotEquals");
    case QDjangoWhere::INotEquals: return QString("INotEquals");
    case QDjangoWhere::GreaterThan: return QString("GreaterThan");
    case QDjangoWhere::LessThan: return QString("LessThan");
    case QDjangoWhere::GreaterOrEquals: return QString("GreaterOrEquals");
    case QDjangoWhere::LessOrEquals: return QString("LessOrEquals");
    case QDjangoWhere::StartsWith: return QString("StartsWith");
    case QDjangoWhere::IStartsWith: return QString("IStartsWith");
    case QDjangoWhere::EndsWith: return QString("EndsWith");
    case QDjangoWhere::IEndsWith: return QString("IEndsWith");
    case QDjangoWhere::Contains: return QString("Contains");
    case QDjangoWhere::IContains: return QString("IContains");
    case QDjangoWhere::IsIn: return QString("IsIn");
    case QDjangoWhere::IsNull: return QString("IsNull");
    case QDjangoWhere::None:
    default:
        return QString("");
    }

    return QString();
}

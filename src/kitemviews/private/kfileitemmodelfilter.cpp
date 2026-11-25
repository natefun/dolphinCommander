/*
 * SPDX-FileCopyrightText: 2011 Janardhan Reddy <annapareddyjanardhanreddy@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemmodelfilter.h"

#include "commandersettings.h"

#include <QFileInfo>

#include <QRegularExpression>

#include <algorithm>

#include <KFileItem>

KFileItemModelFilter::KFileItemModelFilter()
    : m_filterMode(Glob)
    , m_caseSensitive(false)
    , m_regExp(nullptr)
    , m_lowerCasePattern()
    , m_pattern()
{
}

KFileItemModelFilter::~KFileItemModelFilter()
{
    delete m_regExp;
    m_regExp = nullptr;
}

void KFileItemModelFilter::setPattern(const QString &filter)
{
    m_pattern = filter;
    m_lowerCasePattern = filter.toLower();

    updateFilter();
}

void KFileItemModelFilter::setFilterMode(FilterMode mode)
{
    m_filterMode = mode;
    updateFilter();
}

KFileItemModelFilter::FilterMode KFileItemModelFilter::filterMode() const
{
    return m_filterMode;
}

void KFileItemModelFilter::setCaseSensitive(bool caseSensitive)
{
    m_caseSensitive = caseSensitive;
    updateFilter();
}

bool KFileItemModelFilter::isCaseSensitive() const
{
    return m_caseSensitive;
}

QString KFileItemModelFilter::pattern() const
{
    return m_pattern;
}

void KFileItemModelFilter::updateFilter()
{
    if (m_filterMode == PlainText) {
        return;
    }

    if (!m_regExp) {
        m_regExp = new QRegularExpression();
    }

    QRegularExpression::PatternOptions options = m_caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
    if (m_filterMode == Regex) {
        m_regExp->setPattern(m_pattern);
        m_regExp->setPatternOptions(options);
    } else if (m_filterMode == Glob) {
        m_regExp->setPattern(QRegularExpression::wildcardToRegularExpression(m_pattern, QRegularExpression::UnanchoredWildcardConversion));
        m_regExp->setPatternOptions(options);
    }
}

void KFileItemModelFilter::setMimeTypes(const QStringList &types)
{
    m_mimeTypes = types;
}

QStringList KFileItemModelFilter::mimeTypes() const
{
    return m_mimeTypes;
}

void KFileItemModelFilter::setExcludeMimeTypes(const QStringList &types)
{
    m_excludeMimeTypes = types;
}

QStringList KFileItemModelFilter::excludeMimeTypes() const
{
    return m_excludeMimeTypes;
}

bool KFileItemModelFilter::hasSetFilters() const
{
    return (!m_pattern.isEmpty() || !m_mimeTypes.isEmpty() || !m_excludeMimeTypes.isEmpty()
            || (m_rulesActive
                && (!m_dotFilesAreHidden || m_hideTemporary || !m_hiddenPrefixes.isEmpty() || !m_hiddenNames.isEmpty() || !m_hiddenExtensions.isEmpty())));
}

void KFileItemModelFilter::setHiddenRulesActive(bool active)
{
    m_rulesActive = active;
}

void KFileItemModelFilter::reloadHiddenRules()
{
    auto list = [](const QString &key) {
        QStringList items = CommanderSettings::stringValue(key).split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (QString &item : items) {
            item = item.trimmed();
        }
        items.removeAll(QString());
        return items;
    };
    m_hiddenPrefixes = list(QStringLiteral("HiddenFilesPrefixList"));
    m_hiddenNames = list(QStringLiteral("HiddenFilesList"));
    m_hiddenExtensions = list(QStringLiteral("HiddenFilesExtList"));
    for (QString &extension : m_hiddenExtensions) {
        extension = extension.startsWith(QLatin1Char('.')) ? extension.mid(1).toLower() : extension.toLower();
    }
    m_dotFilesAreHidden = CommanderSettings::boolValue(QStringLiteral("TreatDotFilesAsHidden"), true);
    m_hideTemporary =
        CommanderSettings::boolValue(QStringLiteral("HideTemporary"), false) && !CommanderSettings::boolValue(QStringLiteral("ShowTemporary"), false);
}

bool KFileItemModelFilter::hiddenByRules(const KFileItem &item) const
{
    const QString name = item.name();
    // The lister lists the files that start with a dot when they are not to be treated as hidden, so the other hidden files are held back here.
    if (!m_dotFilesAreHidden && item.isHidden() && !name.startsWith(QLatin1Char('.'))) {
        return true;
    }
    for (const QString &prefix : m_hiddenPrefixes) {
        if (name.startsWith(prefix)) {
            return true;
        }
    }
    if (m_hiddenNames.contains(name)) {
        return true;
    }
    if (!m_hiddenExtensions.isEmpty() && !item.isDir() && m_hiddenExtensions.contains(QFileInfo(name).suffix().toLower())) {
        return true;
    }
    if (m_hideTemporary && !item.isDir()) {
        const QString suffix = QFileInfo(name).suffix().toLower();
        return suffix == QLatin1String("tmp") || suffix == QLatin1String("temp") || name.startsWith(QLatin1String("~$")) || name.endsWith(QLatin1Char('~'));
    }
    return false;
}

bool KFileItemModelFilter::matches(const KFileItem &item) const
{
    if (m_rulesActive && hiddenByRules(item)) {
        return false;
    }

    const bool hasPatternFilter = !m_pattern.isEmpty();
    const bool hasMimeTypesFilter = !m_mimeTypes.isEmpty() || !m_excludeMimeTypes.isEmpty();

    // If no filter is set, return true.
    if (!hasPatternFilter && !hasMimeTypesFilter) {
        return true;
    }

    // If both filters are set, return true when both filters are matched
    if (hasPatternFilter && hasMimeTypesFilter) {
        return (matchesPattern(item) && matchesType(item));
    }

    // If only one filter is set, return true when that filter is matched
    if (hasPatternFilter) {
        return matchesPattern(item);
    }

    return matchesType(item);
}

bool KFileItemModelFilter::matchesPattern(const KFileItem &item) const
{
    if (m_filterMode == Glob || m_filterMode == Regex) {
        return m_regExp->isValid() && m_regExp->match(item.text()).hasMatch();
    } else if (m_caseSensitive) {
        return item.text().contains(m_pattern);
    } else {
        return item.text().toLower().contains(m_lowerCasePattern);
    }
}

bool KFileItemModelFilter::matchesType(const KFileItem &item) const
{
    bool excluded = std::any_of(m_excludeMimeTypes.constBegin(), m_excludeMimeTypes.constEnd(), [item](const QString &excludeMimetype) {
        // Currently only really used for "temporary" files which are all idenfied by glob.
        // If you use the filter for anything else, this must be revisited.
        return item.currentMimeType().name() == excludeMimetype;
    });
    if (excluded) {
        return false;
    }

    bool included = std::any_of(m_mimeTypes.constBegin(), m_mimeTypes.constEnd(), [item](const QString &mimeType) {
        return item.mimetype() == mimeType;
    });
    if (included) {
        return true;
    }

    return m_mimeTypes.isEmpty();
}

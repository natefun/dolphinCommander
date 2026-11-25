/*
 * SPDX-FileCopyrightText: 2011 Janardhan Reddy <annapareddyjanardhanreddy@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMMODELFILTER_H
#define KFILEITEMMODELFILTER_H

#include "dolphin_export.h"

#include <QStringList>

class KFileItem;
class QRegularExpression;

/**
 * @brief Allows to check whether an item of the KFileItemModel
 *        matches with a set filter-string.
 *
 * Currently the filter is only checked for the KFileItem::text()
 * property of the KFileItem, but this might get extended in
 * future.
 */
class DOLPHIN_EXPORT KFileItemModelFilter
{
public:
    KFileItemModelFilter();
    virtual ~KFileItemModelFilter();

    KFileItemModelFilter(const KFileItemModelFilter &) = delete;
    KFileItemModelFilter &operator=(const KFileItemModelFilter &) = delete;

    /** Filtering modes of KFileItemModelFilter */
    enum FilterMode {
        /** Substring matching. */
        PlainText = 0,
        /** Matching with glob, default. */
        Glob,
        /** Matching with regex. */
        Regex
    };

    /**
     * Sets the pattern that is used for a comparison with the item
     * in KFileItemModelFilter::matches().
     */
    void setPattern(const QString &pattern);
    QString pattern() const;

    /**
     * Sets the filtering mode used in KFileItemModelFilter::matches().
     */
    void setFilterMode(FilterMode mode);
    FilterMode filterMode() const;

    /**
     * Enable or disable the case sensitive filtering.
     */
    void setCaseSensitive(bool caseSensitive);
    bool isCaseSensitive() const;

    /**
     * Set the list of mimetypes that are used for comparison with the
     * item in KFileItemModelFilter::matchesMimeType.
     */
    void setMimeTypes(const QStringList &types);
    QStringList mimeTypes() const;

    /**
     * Set the list of mimetypes that are used for comparison and excluded with the
     * item in KFileItemModelFilter::matchesMimeType.
     */
    void setExcludeMimeTypes(const QStringList &types);
    QStringList excludeMimeTypes() const;

    /**
     * @return True if either the pattern or mimetype filters has been set.
     */
    bool hasSetFilters() const;

    /**
     * @return True if the item matches with the pattern defined by
     *         @ref setPattern() or @ref setMimeTypes
     */
    bool matches(const KFileItem &item) const;

    /**
     * OneCommander's rules for files to treat as hidden (by prefix, by full name, by extension, and temporary files). They apply
     * while hidden files are not shown: the model switches them on and off with that.
     */
    void setHiddenRulesActive(bool active);
    /** Reads the rules from the settings again. */
    void reloadHiddenRules();
    bool hiddenByRules(const KFileItem &item) const;

private:
    /**
     * @return True if item matches pattern set by @ref setPattern.
     */
    bool matchesPattern(const KFileItem &item) const;

    /**
     * @return True if item matches mimetypes set by @ref setMimeTypes.
     */
    bool matchesType(const KFileItem &item) const;

    /**
     * Instantiate and configure m_regExp according to m_filterMode and m_caseSensitive.
     */
    void updateFilter();

    FilterMode m_filterMode; // The current filtering mode.
    bool m_caseSensitive; // If true the matching will be case sensitive.

    QRegularExpression *m_regExp;
    QString m_lowerCasePattern; // Lowercase version of m_filter for
                                // faster comparison in matches().
    QString m_pattern; // Property set by setPattern().
    QStringList m_mimeTypes; // Property set by setMimeTypes()
    QStringList m_excludeMimeTypes; // Property set by setExcludeMimeTypes()
    bool m_rulesActive = false; // Property set by setHiddenRulesActive()
    QStringList m_hiddenPrefixes;
    QStringList m_hiddenNames;
    QStringList m_hiddenExtensions;
    bool m_hideTemporary = false;
    /** Whether a name that starts with a dot makes a file hidden. When not, only the other hidden files are. */
    bool m_dotFilesAreHidden = true;
};
#endif

/***************************************************************************
 *                                                                         *
 *   This file is part of the WordCloud project,                           *
 *       http://www.enricoros.com/opensource/wordcloud                     *
 *                                                                         *
 *   Copyright (C) 2009 by Enrico Ros <enrico.ros@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "WordScanner.h"

#include <QDebug>
#include <QFile>
#include <QHeaderView>
#include <QRegExp>
#include <QStringList>
#include <QTableWidget>
#include <QTextStream>

using namespace WordCloud;

bool Scanner::addFromFile(const QString & fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    // read all the words from file
    QRegExp splitNonWords("\\W");
    QTextStream ts(&file);
    while (!ts.atEnd()) {
        QStringList words = ts.readLine().split(splitNonWords, QString::SkipEmptyParts);
        foreach (const QString & word, words)
            addWord(word);
    }
    return true;
}

bool Scanner::addFromString(const QString & string)
{
    QRegExp splitNonWords("\\W");
    QStringList words = string.split(splitNonWords, QString::SkipEmptyParts);
    foreach (const QString & word, words)
        addWord(word);
    return true;
}

bool Scanner::addFromUrl(const QUrl & url)
{
    qWarning() << "Scanner::addFromUrl(" << url.toString() << ") not implemented";
    return false;
}

bool Scanner::addFromRss(const QUrl & rss)
{
    qWarning() << "Scanner::addFromRss(" << rss.toString() << ") not implemented";
    return false;
}

void Scanner::clear()
{
    m_words.clear();
}

static bool wordFrequencySorter(const Word &w1, const Word &w2)
{
    return w1.count > w2.count;
}

WordList Scanner::takeWords()
{
    // remove common words, single ones, and sort by frequency
    if (m_words.size() >= 100)
        removeWordsBelowCount(2);
    removeWordsByLanguage(QLocale::Italian);
    //qSort(m_words.begin(), m_words.end(), wordFrequencySorter);

    // clear private list and return
    WordList wl = m_words;
    m_words.clear();
    return wl;
}

int Scanner::wordCount() const
{
    return m_words.count();
}

bool Scanner::isEmpty() const
{
    return m_words.isEmpty();
}

void Scanner::dumpOnTable(QTableWidget * table)
{
    // setup the table
    table->clear();
    table->setColumnCount(2);
    table->setRowCount(m_words.size());
    table->horizontalHeader()->setVisible(true);
    table->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Word")));
    table->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("#")));
    table->verticalHeader()->setVisible(false);
    // populate the table
    int row = 0;
    WordList::iterator i = m_words.begin();
    for (; i != m_words.end(); ++i) {
        table->setItem(row, 0, new QTableWidgetItem(i->variants.begin().key()));
        table->setItem(row++, 1, new QTableWidgetItem(QString::number(i->count)));
    }
}

void Scanner::dumpWords() const
{
    QString dumpString;
    bool first = true;
    foreach (const Word & word, m_words) {
        if (first)
            first = false;
        else
            dumpString += ", ";
        dumpString += QString("\"%1\"").arg(word.lowerString);
    }
    qWarning("WordList: %s", qPrintable(dumpString));
}

void Scanner::addWord(const QString & word)
{
    QString lowerWord = word.toLower();

    // update existing entries
    WordList::iterator i = m_words.begin();
    for (; i != m_words.end(); ++i) {
        if (i->lowerString == lowerWord) {
            i->count++;
            if (i->variants.contains(word))
                i->variants[word]++;
            else
                i->variants[word] = 1;
            return;
        }
    }

    // add a new entry
    Word w;
    w.lowerString = lowerWord;
    w.count = 1;
    w.variants[word] = 1;
    m_words.append(w);
}

void Scanner::removeWordsByLanguage(QLocale::Language language)
{
    const char ** regExps;
    int regExpCount = 0;

    switch (language) {
        case QLocale::Italian: {
            const char * r[] = {
                ".", "a.", "all", "alla", "anche", "anzich.", "che", "ci", "cio.",
                "come", "con", "cos.", "cui", "da", "da.", "dall.", "degli", "de.",
                "dell", "della", "delle", "di", "dove", "due", "ed", "far.", "fino",
                "fra", "gli", "i.", "l.", "loro", "nel", "nell", "nella", "nelle",
                "non", "per", "pi.", "poi", "pu.", "quale", "quell.", "quest.", "sar.",
                "s.", "senza", "su.", "sull", "sull.", "tali", "tra", "un", "un.", "uso" };
            regExps = r;
            regExpCount = sizeof(r) / sizeof(const char *);
            } break;

        default:
            qWarning("Scanner::removeWordsByLanguage: language unsupported");
            return;
    }

    // erase all words matching regexps
    WordList::iterator wIt = m_words.begin();
    while (wIt != m_words.end()) {
        bool found = false;
        for (int i = 0; i < regExpCount; i++) {
            if (QRegExp(regExps[i]).exactMatch(wIt->lowerString)) {
                found = true;
                break;
            }
        }
        if (found)
            wIt = m_words.erase(wIt);
        else
            ++wIt;
    }
}

void Scanner::removeWordsBelowCount(int count)
{
    WordList::iterator i = m_words.begin();
    while (i != m_words.end()) {
        if (i->count < count)
            i = m_words.erase(i);
        else
            ++i;
    }
}
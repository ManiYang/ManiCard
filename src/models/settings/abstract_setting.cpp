#include "abstract_setting.h"

QString AbstractWorkspaceOrBoardSetting::removeCommonIndentation(const QString &s) {
    const QStringList lines = s.split('\n');

    QStringList adjustedLines;
    bool gotFirstNonEmptyLine = false;
    int commonIndentation = 0;
    for (const QString &line: lines) {
        if (line.trimmed().isEmpty()) {
            adjustedLines << line;
            continue;
        }

        if (!gotFirstNonEmptyLine) {
            gotFirstNonEmptyLine = true;

            // use first non-empty line's number of preceding spaces as the common indentation depth
            const auto it = std::find_if(line.constBegin(), line.constEnd(), [](QChar c) {
                return c != QChar(' ');
            });
            commonIndentation = it - line.constBegin();
        }

        // remove at most `commonIndentation` spaces from the front of `line`
        int p = 0;
        for (; p < line.count() && p < commonIndentation; ++p) {
            if (line.at(p) != QChar(' '))
                break;
        }
        adjustedLines << line.mid(p);
    }

    if (!gotFirstNonEmptyLine)
        return "";

    // remove empty lines from the head and tail of `adjustedLines`
    int firstNonEmptyLine = -1;
    int lastNonEmptyLine = -1;
    for (int i = 0; i < adjustedLines.count(); ++i) {
        const auto line = adjustedLines.at(i);
        if (!line.trimmed().isEmpty()) {
            if (firstNonEmptyLine == -1)
                firstNonEmptyLine = i;
            lastNonEmptyLine = i;
        }
    }

    return adjustedLines
            .mid(firstNonEmptyLine, lastNonEmptyLine - firstNonEmptyLine + 1)
            .join('\n');
}

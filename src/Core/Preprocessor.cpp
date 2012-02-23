#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <qt4/QtCore/qstring.h>

#include "Preprocessor.h"
#include "SyntaxTreeFactory.h"

#define File FILE
#define null NULL

namespace Core {

    const QString Preprocessor::Directive::IDENTYFIER_STRING = "\\S+";
    const QString Preprocessor::Directive::SPACE_STRING = "\\s+";
    const QString Preprocessor::Directive::END_OF_STRING = ";";
    const Preprocessor::Directive Preprocessor::Directive::IMPORT = Directive("import");

    const QString Preprocessor::FILE_FORMAT_MASK = ".lng";
    
    const QRegExp Preprocessor::TEXT_SPLITTER = QRegExp("\\s*;(\\s|\n)*");
    const QRegExp Preprocessor::LINE_SPLITTER = QRegExp("\\s+");
    const QRegExp Preprocessor::COMMENTS_REMOVER = QRegExp("(\\/\\/[^\n]*\n)|(\\/\\*.*\\*\\/)");
    const QRegExp Preprocessor::SPACES_REMOVER = QRegExp("[\n\t\\s]+");

    Preprocessor::Preprocessor(const QString& pathToLibrary) : pathToLibrary(pathToLibrary) {
    }

    Preprocessor::~Preprocessor() {
    }

    QString Preprocessor::process(const QString& source) {
        QString result = source;

        removeComments(result);

        QStringList lines = result.split(TEXT_SPLITTER, QString::SkipEmptyParts);

        bool ok = true;
        for (int i = 0; i < lines.size(); i++) {
            if (Directive::IMPORT.getMatcherRegExp().exactMatch(lines[i])) {
                try{
                    import(result, lines[i].split(LINE_SPLITTER, QString::SkipEmptyParts)[1]);
                }catch(AnalyzeCrashExeption) {
                    ok = false;
                }
            }
        }
        
        //if(!ok) throw AnalyzeCrashExeption();
        
        return result.replace(SPACES_REMOVER, "");
    }

    void Preprocessor::removeComments(QString& source) {
        source = source.replace(COMMENTS_REMOVER, "");
    }

    void Preprocessor::import(QString& source, const QString& importName) {
        File* fileDescriptor = null;

        if (!fileDescriptor) fileDescriptor = fopen((pathToLibrary + importName + FILE_FORMAT_MASK).toLocal8Bit(), "rt");
        if (!fileDescriptor) fileDescriptor = fopen((importName + FILE_FORMAT_MASK).toLocal8Bit(), "rt");
        if (!fileDescriptor) {
            Events::LibraryFileCannotBeFound event = Events::LibraryFileCannotBeFound(importName);
            event.share(*broadcaster);
            //throws AnalyzeCrashExeption(importName);
        }
        
        QFile file;
        file.open(fileDescriptor, QFile::ReadOnly | QFile::Text);
        
        source = source.replace(Directive::IMPORT.getRemoverRegExp(importName), "");
        source = source + file.readAll();
    }

}
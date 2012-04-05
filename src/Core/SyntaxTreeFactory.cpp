#include <QRegExp>
#include <QLocale>
#include <QString>

#include "../Events/Event.h"
#include "../Core/Analyzer.h"
#include "../Core/SyntaxTreeFactory.h"
#include "../Core/Preprocessor.h"

namespace Core {

    SyntaxTreeFactory::SyntaxTreeFactory(Events::EventBroadcaster* broacaster) : broadcaster(broacaster) { }

    Tree<Symbol> SyntaxTreeFactory::createTree(const QList<Symbol>& text) const throws(AnalyzeCrashExeption) {
        const QString MAIN_SYMBOL = "main";

        QMap<QString, QList<Symbol> > linesMap = createLinesMap(text);

        QSet<QString> notUsedSymbolsSet = createLinesSet(linesMap);
        QSet<QString> notDefinedSymbolsSet = QSet<QString > ();

        Tree<Symbol> tree = Tree<Symbol > ();
        QMap<QString, QList<Symbol> >::Iterator it = linesMap.find(MAIN_SYMBOL);
        QList<Symbol> value = *it;
        linesMap.erase(it);
        notUsedSymbolsSet.erase(notUsedSymbolsSet.find(MAIN_SYMBOL));

        tree = Symbol(MAIN_SYMBOL, Symbol::SymbolType::IDENTYFIER);
        tree.get().setId("a");

        processLine(value, tree);
        TreeProcessor processor = TreeProcessor();

        bool ok = true;
        while (ok) {
            tree.traverse(processor);

            if (processor.isEnd())
                break;

            QList<Tree<Symbol> > nodes = processor.getNodes();

            for (QList<Tree<Symbol> >::Iterator it = nodes.begin(); it != nodes.end(); ++it) {
                QString current = (*it).get().getRepresentation();
                notUsedSymbolsSet.erase(notUsedSymbolsSet.find(current));

                if (!linesMap.contains(current)) {
                    notDefinedSymbolsSet.insert(current);

                    ok = false;
                    continue;
                }

                processLine(linesMap.value(current), *it);
            }

        }

        for (QSet<QString>::ConstIterator it = notDefinedSymbolsSet.begin(); it != notDefinedSymbolsSet.end(); ++it) {
            Events::SymbolIsNotDefinedErrorEvent event = Events::SymbolIsNotDefinedErrorEvent(*it);
            event.share(*broadcaster);
        }

        for (QSet<QString>::ConstIterator it = notUsedSymbolsSet.begin(); it != notUsedSymbolsSet.end(); ++it) {
            Events::SymbolIsNotUsedErrorEvent event = Events::SymbolIsNotUsedErrorEvent(*it);
            event.share(*broadcaster);
        }

        if (!ok) throw AnalyzeCrashExeption();

        return tree;
    }

    QMap<QString, QList<Symbol> > SyntaxTreeFactory::createLinesMap(const QList<Symbol>& text) const {
        QMap<QString, QList<Symbol> > linesMap = QMap<QString, QList<Symbol> >();

        for(QList<Symbol>::ConstIterator it = text.begin(), end = text.end(); it != end;   ++it) {
            Symbol key = *(it++);
            QList<Symbol> defineList;
            
            if(it == end) {
                return linesMap;
            }
            
            if((*(it++)).getType() != Symbol::SymbolType::DEFINE) {
                throw AnalyzeCrashExeption();
            }
            
            while((*it).getType() != Symbol::SymbolType::DEFINE_END) {
                defineList += *(it++);
            }
            
            linesMap.insert(key.getRepresentation(), toPostfixSymbolsList(defineList));
        }
        
        return linesMap;
    }

    QSet<QString> SyntaxTreeFactory::createLinesSet(const QMap<QString, QList<Symbol> >& map) const {
        QSet<QString> set = QSet<QString > ();

        for (QMap<QString, QList<Symbol> >::ConstIterator it = map.begin(); it != map.end(); ++it) {
            set.insert(it.key());
        }

        return set;
    }

    void SyntaxTreeFactory::processLine(const QList<Symbol>& line, Tree<Symbol> tree) const throws(AnalyzeCrashExeption) {
        QStack<unsigned int> positions = QStack<unsigned int>();
        positions.push(tree.get().getType().getArgsNumber());
        
        for (int i = line.size() - 1; i >= 0; i--) {
            while (!tree.isRoot() && positions.top() <= 0) {
                tree = tree.getSupertree();
                positions.pop();
            }
                
            tree[positions.top() - 1] = line[i];
            tree[positions.top() - 1].get().setId(tree.get().getId()+(char) (positions.top() + 98));
            positions.top()--;

            
            if (line[i].getType() == Symbol::SymbolType::AND
                    || line[i].getType() == Symbol::SymbolType::OR) {
                tree = tree[positions.top()];
                positions.push(tree.get().getType().getArgsNumber());
            }
        }
    }

    QList<Symbol> SyntaxTreeFactory::toPostfixSymbolsList(const QList<Symbol>& line) const throws(AnalyzeCrashExeption) {
        QList<Symbol> result = QList<Symbol > ();
        QStack<Symbol> stack = QStack<Symbol > ();

        for (QList<Symbol>::ConstIterator it = line.begin(), end = line.end(); it != end; ++it) {
            if ((*it).getType() == Symbol::SymbolType::LITHERAL
                    || (*it).getType() == Symbol::SymbolType::IDENTYFIER) {
                result.append(*it);
            } else {
                if ((*it).getType() == Symbol::SymbolType::OPEN_BRACKET) {
                    stack.push(*it);
                } else if ((*it).getType() == Symbol::SymbolType::CLOSE_BRACKET) {
                    while (true) {
                        if (stack.empty()) {
                            Events::WrongBracketsNumberErrorEvent event = Events::WrongBracketsNumberErrorEvent();
                            event.share(*broadcaster);
                            throw AnalyzeCrashExeption((*it).getRepresentation());
                        }

                        Symbol top = stack.pop();
                        if (top.getType() == Symbol::SymbolType::OPEN_BRACKET) break;
                        result.append(top);
                    }
                } else {
                    while (!stack.empty() && (*it).getType().getPriority() <= stack.top().getType().getPriority()) {
                        result.append(stack.pop());
                    }
                    stack.push(*it);
                }
            }
        }

        while (!stack.empty()) {
            Symbol top = stack.pop();
            if (top.getType() == Symbol::SymbolType::OPEN_BRACKET
                    || top.getType() == Symbol::SymbolType::CLOSE_BRACKET) {
                Events::WrongBracketsNumberErrorEvent event = Events::WrongBracketsNumberErrorEvent();
                event.share(*broadcaster);
                throw AnalyzeCrashExeption(top.getRepresentation());
            }
            result.append(top);
        }

        return result;
    }

}

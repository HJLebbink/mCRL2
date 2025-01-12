// Author(s): Rimco Boudewijns
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MCRL2_UTILITIES_NUMBEREDTEXTEDIT_H
#define MCRL2_UTILITIES_NUMBEREDTEXTEDIT_H

#include <QTextEdit>


namespace mcrl2
{

namespace gui
{

namespace qt
{

class NumberedTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit NumberedTextEdit(QWidget *parent = nullptr);

    int lineNumberAreaWidth();
    void lineNumberAreaPaintEvent(QPaintEvent* event);

public slots:
    void updateLineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *e);
    void scrollContentsBy(int dx, int dy);

private:
    QWidget* m_numberArea;
};

} // namespace qt

} // namespace gui

} // namespace mcrl2

#endif // MCRL2_UTILITIES_NUMBEREDTEXTEDIT_H

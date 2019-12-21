// enve - 2D animations software
// Copyright (C) 2016-2019 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef BRUSHCONTEXEDWRAPPER_H
#define BRUSHCONTEXEDWRAPPER_H
#include "Paint/simplebrushwrapper.h"
#include <QImage>

class BrushesContext;

struct BrushData {
    QString fName;
    stdsptr<SimpleBrushWrapper> fWrapper;
    QImage fIcon;
    QByteArray fWholeFile;
};

class BrushContexedWrapper : public SelfRef {
    Q_OBJECT
    e_OBJECT
public:
    void setSelected(const bool selected) {
        if(selected == mSelected) return;
        mSelected = selected;
        emit selectionChanged(mSelected);
    }

    bool selected() const {
        return mSelected;
    }

    bool bookmarked() const {
        return mBookmarked;
    }

    const BrushData& getBrushData() const {
        return mBrushData;
    }

    SimpleBrushWrapper * getSimpleBrush() {
        return mBrushData.fWrapper.get();
    }

    BrushesContext* getContext() const {
        return mContext;
    }

    void bookmark();
    void unbookmark();
protected:
    BrushContexedWrapper(BrushesContext* context,
                         const BrushData& brushData) :
        mBrushData(brushData), mContext(context) {}
signals:
    void selectionChanged(bool);
    void bookmarkedChanged(bool);
private:
    bool mBookmarked = false;
    bool mSelected = false;
    const BrushData& mBrushData;
    BrushesContext* const mContext;
};

#endif // BRUSHCONTEXEDWRAPPER_H
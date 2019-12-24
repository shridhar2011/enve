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

#include "boolpropertycontainer.h"

BoolPropertyContainer::BoolPropertyContainer(const QString &name) :
    StaticComplexAnimator(name) {}

bool BoolPropertyContainer::getValue() {
    return mValue;
}

void BoolPropertyContainer::setValue(const bool value) {
    if(mValue == value) return;
    mValue = value;
    prp_afterWholeInfluenceRangeChanged();

    for(const auto& prop : ca_mChildAnimators) {
        prop->SWT_setDisabled(!value);
    }
    emit valueChanged(value);
}


void BoolPropertyContainer::prp_writeProperty(eWriteStream& dst) const {
    dst << mValue;
    StaticComplexAnimator::prp_writeProperty(dst);
}

void BoolPropertyContainer::prp_readProperty(eReadStream& src) {
    bool value; src >> value;
    setValue(value);
    StaticComplexAnimator::prp_readProperty(src);
}

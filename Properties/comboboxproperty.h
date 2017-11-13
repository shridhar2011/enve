#ifndef COMBOBOXPROPERTY_H
#define COMBOBOXPROPERTY_H
#include "property.h"
class ComboBoxProperty : public Property {
    Q_OBJECT
public:
    ComboBoxProperty(const QStringList &list);
    const int &getCurrentValue() {
        return mCurrentValue;
    }

    const QStringList &getValueNames() {
        return mValueNames;
    }

    QString getCurrentValueName() {
        if(mCurrentValue >= mValueNames.count()) return "null";
        return mValueNames.at(mCurrentValue);
    }
    bool SWT_isComboBoxProperty() { return true; }
    void writeProperty(QIODevice *target);
    void readProperty(QIODevice *target);
signals:
    void valueChanged(int);
public slots:
    void setCurrentValue(const int &id) {
        if(mCurrentValue == id) return;
        mCurrentValue = id;
        emit valueChanged(id);
        prp_updateInfluenceRangeAfterChanged();
        prp_callUpdater();
        prp_callFinishUpdater();
    }
private:
    QStringList mValueNames;
    int mCurrentValue = 0;
};

#endif // COMBOBOXPROPERTY_H
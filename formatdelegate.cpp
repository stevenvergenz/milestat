#include "formatdelegate.h"

FormatDelegate::FormatDelegate(QObject *parent, Role r) :
    QStyledItemDelegate(parent), role(r)
{
}

QString FormatDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
	Q_UNUSED(locale);
	
	// return the formatted string based on the role
	if( role == TSeparator ){
		return QString("%L1").arg(value.toInt());
	} else if( role == Currency ){
		return QString("$%L1").arg(value.toDouble(), 0, 'f', 2);
	} else if( role == FixedFloat ){
		return QString("%1").arg(value.toDouble(), 0, 'f', 3);
	} else {
		return value.toString();
	}
}

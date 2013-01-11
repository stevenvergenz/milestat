#ifndef FORMATDELEGATE_H
#define FORMATDELEGATE_H

#include <QStyledItemDelegate>

class FormatDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	typedef enum {TSeparator,Currency,FixedFloat,Default} Role;
	explicit FormatDelegate(QObject *parent = 0, Role r = Default);
	QString displayText ( const QVariant & value, const QLocale & locale ) const;
signals:

public slots:

private:
	Role role;
};

#endif // FORMATDELEGATE_H

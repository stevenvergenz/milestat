#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtSql>
#include <QDateTime>
#include <ctime>
#include <formatdelegate.h>

namespace Ui {
    class MainWindow;
}

struct Command {
	QString type;
	quint32 odometer;
	QString state;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	QStringList names, abbrevs;
	QString dtFormat;
	~MainWindow();

signals:
	void dataUpdated();

public slots:
	void updateCombos();
	void updateStatistics();
	void addCrossing();
	void addRefuel();
	void undoLastCommand();
	void hideUnvisited(bool toggled);
	void resetDatabase();
	void bugReport();
	void aboutMileStat();

protected:
	void changeEvent(QEvent *e);
	void addToLog(QString event);
	void setInitialValues();
	QString validateDatabase();
	QString populateDatabase();

private:
	Ui::MainWindow *ui;
	QSqlDatabase db;
	QSqlTableModel *model;
	QStack<Command> undoStack;
};

#endif // MAINWINDOW_H

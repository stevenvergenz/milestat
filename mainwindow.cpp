#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent), dtFormat("MMM dd | hh:mm"),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    
	abbrevs << "AK" << "AL" << "AR" << "AZ" << "CA" << "CO" << "CT" << "DE" << "FL" << 
	           "GA" << "HI" << "IA" << "ID" << "IL" << "IN" << "KS" << "KY" << "LA" <<
	           "MA" << "MD" << "ME" << "MI" << "MN" << "MO" << "MS" << "MT" << "NC" <<
	           "ND" << "NE" << "NH" << "NJ" << "NM" << "NV" << "NY" << "OH" << "OK" <<
	           "OR" << "PA" << "RI" << "SC" << "SD" << "TN" << "TX" << "UT" << "VA" <<
	           "VT" << "WA" << "WI" << "WV" << "WY";
	names << "Alaska" << "Alabama" << "Arkansas" << "Arizona" <<
	         "California" << "Colorado" << "Connecticut" << "Delaware" <<
	         "Florida" << "Georgia" << "Hawaii" << "Iowa" << "Idaho" <<
	         "Illinois" << "Indiana" << "Kansas" << "Kentucky" << "Louisiana" <<
	         "Massachusetts" << "Maryland" << "Maine" << "Michigan" << "Minnesota" <<
	         "Missouri" << "Mississippi" << "Montana" << "North Carolina" <<
	         "North Dakota" << "Nebraska" << "New Hampshire" << "New Jersey" <<
	         "New Mexico" << "Nevada" << "New York" << "Ohio" << "Oklahoma" <<
	         "Oregon" << "Pennsylvania" << "Rhode Island" << "South Carolina" <<
	         "South Dakota" << "Tennessee" << "Texas" << "Utah" << "Virginia" <<
	         "Vermont" << "Washington" << "Wisconsin" << "West Virginia" << "Wyoming";
	
	// open the database for use
	QString e;
	e = validateDatabase();
	if( !e.isEmpty() ){
		QMessageBox::critical(this, "Critical Error",
			QString("Error %1")
			.arg(e)
		);
		db.close();
		this->deleteLater();
	}
	
	// set up the table model to pass the sql data through
	model = new QSqlTableModel(this,db);
	model->setTable("States");
	model->setEditStrategy(QSqlTableModel::OnManualSubmit);
	model->setFilter("Mileage<>0 OR GasAmt<>0");
	model->setHeaderData(0, Qt::Horizontal, "Name");
	model->setHeaderData(1, Qt::Horizontal, "");
	model->setHeaderData(2, Qt::Horizontal, "Mileage");
	model->setHeaderData(3, Qt::Horizontal, "Fuel\nPurchased");
	model->setHeaderData(4, Qt::Horizontal, "Fuel\nVolume");
	model->setHeaderData(5, Qt::Horizontal, "Refuel\nCount");
	model->setHeaderData(6, Qt::Horizontal, "Visit\nCount");
	
	// set up the table view
	ui->tableView->setModel( model );
	ui->tableView->setItemDelegateForColumn(2,
		new FormatDelegate(0, FormatDelegate::TSeparator));
	ui->tableView->setItemDelegateForColumn(3,
		new FormatDelegate(0, FormatDelegate::Currency));
	ui->tableView->setItemDelegateForColumn(4,
		new FormatDelegate(0, FormatDelegate::FixedFloat));
	ui->tableView->setItemDelegateForColumn(5,
		new FormatDelegate(0));
	ui->tableView->setItemDelegateForColumn(6,
		new FormatDelegate(0));
	ui->tableView->setColumnWidth(0, 125);
	ui->tableView->setColumnWidth(1,  50);
	ui->tableView->sortByColumn(2);
	
	// set the widgets
	updateCombos();
	updateStatistics();
	setInitialValues();

	connect(ui->pushAddCrossing, SIGNAL(clicked()),
			this,                SLOT(addCrossing())
	);
	connect(ui->pushAddFillup, SIGNAL(clicked()),
			this,              SLOT(addRefuel())
	);
	connect(ui->action_Reset_Statistics, SIGNAL(triggered()),
			this,                        SLOT(resetDatabase())
	);
	
	connect(this, SIGNAL(dataUpdated()),
	        this, SLOT(updateStatistics())
	);
	
	connect(ui->actionBugReport, SIGNAL(triggered()),
	        this,                SLOT(bugReport())
	);
	connect(ui->actionAbout_MileStat, SIGNAL(triggered()),
	        this,                      SLOT(aboutMileStat())
	);
	connect(ui->actionAbout_Qt, SIGNAL(triggered()),
	        qApp,                SLOT(aboutQt())
	);
	connect(ui->action_Undo, SIGNAL(triggered()),
	        this,            SLOT(undoLastCommand())
	);
	connect(ui->action_Hide_Unvisited_States, SIGNAL(toggled(bool)),
	        this, SLOT(hideUnvisited(bool))
	);
	
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::updateCombos()
{
	// this function fills all the State combo boxes with the contents of the States table

	// add a blank entry
	ui->comboToState->addItem(" ");
	ui->comboCurState->addItem(" ");

	// run the query
	for(int i=0; i<50; i++){
		ui->comboToState->addItem(abbrevs[i]);
		ui->comboCurState->addItem(abbrevs[i]);
	}
}

void MainWindow::updateStatistics()
{
	// update the contents of the table
	if( !model->select() ){
		addToLog( QString("Error %1: %2 (Model)")
		         .arg(model->lastError().number())
		         .arg(model->lastError().text()) 
		);
	}
	//ui->tableView->resizeColumnsToContents();
}

void MainWindow::addToLog(QString event){
	ui->txtHistory->setPlainText( event +"\n"+ ui->txtHistory->toPlainText());
}

void MainWindow::addCrossing()
{
	QString curState = ui->comboCurState->currentText();
	QString toState = ui->comboToState->currentText();
	quint32 odometer = ui->spinOdometer->value();
	quint32 curtime = ui->dteTime->dateTime().toTime_t();
	QSqlQuery query;
	Command entry = {"Crossings", odometer, curState};
	
	if( ui->comboCurState->currentIndex() == 0 || ui->comboToState->currentIndex() == 0 )
		return;
	
	query.exec("SELECT Odometer,Destination from Crossings ORDER BY Odometer;");
	query.last();
	QSqlRecord prevresult = query.record();

	int dist = 0;
	if( curState == prevresult.value("Destination").toString() ){
		// calculate distance traveled
		dist = odometer - prevresult.value("Odometer").toInt();

	}
	
	// add the entry to Crossings table
	query.prepare("INSERT INTO Crossings (Date, Odometer, Origin, Destination, Distance) "
				  "VALUES (:date, :odo,:orig,:dest,:dist);");
	query.bindValue(":date", curtime);
	query.bindValue(":odo", odometer);
	query.bindValue(":orig", curState);
	query.bindValue(":dest", toState);
	query.bindValue(":dist", dist);
	query.exec();

	// handle the undo entry
	undoStack.push(entry);
	
	// calculate the updated statistics
	query.prepare("UPDATE States SET "
		"Mileage= (SELECT SUM(Distance) FROM Crossings WHERE Origin=:state1), "
		"NumEntries= (SELECT COUNT(*) FROM Crossings WHERE Origin=:state2 AND Distance<>0) "
		"WHERE Abbrev=:state3;");
	query.bindValue(":state1", curState);
	query.bindValue(":state2", curState);
	query.bindValue(":state3", curState);
	query.exec();
	emit dataUpdated();
	
	// update the log
	addToLog(QString("%1 [%L2]: Crossed from %3 to %4, traveling %5 miles in %3.")
		.arg(QDateTime::fromTime_t(curtime).toString(dtFormat))
		.arg(odometer)
	        .arg(curState)
		.arg(toState)
		.arg(dist)
	);

	// update the widgets
	ui->dteTime->setTime( ui->dteTime->time().addSecs(60) );
	ui->comboCurState->setCurrentIndex(ui->comboToState->currentIndex());
	ui->comboToState->setCurrentIndex(0);
	ui->action_Undo->setEnabled(true);
}

void MainWindow::addRefuel()
{
	// assign to locals
	QSqlQuery query;
	quint32 odometer = ui->spinOdometer->value();
	quint32 curtime = ui->dteTime->dateTime().toTime_t();
	QString state = ui->comboCurState->currentText();
	double gal = ui->spinGallons->value();
	double cost = ui->spinCost->value();
	Command entry = {"Refuels", odometer, state};
	
	// obviously, INSERT INTO Refuels table
	query.prepare("INSERT INTO Refuels (Date,Odometer,State,Volume,Cost) "
			   "VALUES (:date, :odo, :state, :vol, :cost);");
	query.bindValue(":date", curtime);
	query.bindValue(":odo", odometer);
	query.bindValue(":state", state);
	query.bindValue(":vol", gal);
	query.bindValue(":cost", cost);
	query.exec();
	
	// add entry to undo stack
	undoStack.push(entry);
	
	// update the states fields with the results of the refuel
	query.prepare("UPDATE States SET "
               "NumRefuels= (SELECT COUNT(*) FROM Refuels WHERE State=:state1), "
               "GasAmt= (SELECT SUM(Volume) FROM Refuels WHERE State=:state2), "
               "PurchaseAmt= (SELECT SUM(Cost) FROM Refuels WHERE State=:state3) "
               "WHERE Abbrev=:state4;");
	query.bindValue(":state1", state);
	query.bindValue(":state2", state);
	query.bindValue(":state3", state);
	query.bindValue(":state4", state);
	query.exec();
	//addToLog( QString("%1: %2").arg(query.lastError().number()).arg(query.lastError().text()));
	
	emit dataUpdated();

	addToLog(QString("%1 [%L2]: You bought %3 gallons of gas for $%4 in %5.")
		.arg(QDateTime::fromTime_t(curtime).toString(dtFormat))
		.arg(odometer)
		.arg(gal, 0, 'f', 2)
		.arg(cost, 0, 'f', 2)
		.arg(state)
	);
	
	// update widgets
	ui->dteTime->setTime( ui->dteTime->time().addSecs(60) );
	ui->action_Undo->setEnabled(true);
}

void MainWindow::undoLastCommand()
{
	// make sure that the stack isn't empty
	if( undoStack.isEmpty() ) return;
	
	// get the most recently executed command
	Command entry = undoStack.pop();
	QSqlQuery query;
	
	/*addToLog( QString("Command retrieved: %1, %2, %3")
	         .arg(entry.type).arg(entry.odometer).arg(entry.state) );*/
	
	// remove the entry from given table at given odometer reading
	if(entry.type=="Crossings") query.prepare("DELETE FROM Crossings WHERE Odometer= :odo");
	else query.prepare("DELETE FROM Refuels WHERE Odometer= :odo");
	query.bindValue( ":odo", entry.odometer );
	if( !query.exec() ){
		addToLog( query.lastQuery() );
		addToLog( QString("Error %1: %2").arg(query.lastError().number()).arg(query.lastError().text()));
		return;
	} else{
		addToLog( QString("Event at odometer %1 was removed from the set of %2.")
		         .arg(entry.odometer).arg(entry.type) );
	}
	query.clear();
	
	// update the states fields with the new state of the tables
	query.prepare("UPDATE States SET "
               "NumRefuels= (SELECT COUNT(*) FROM Refuels WHERE State=:state1), "
               "GasAmt= (SELECT SUM(Volume) FROM Refuels WHERE State=:state2), "
               "PurchaseAmt= (SELECT SUM(Cost) FROM Refuels WHERE State=:state3), "
               "Mileage= (SELECT SUM(Distance) FROM Crossings WHERE Origin=:state4), "
               "NumEntries= (SELECT COUNT(*) FROM Crossings WHERE Origin=:state5 AND Distance<>0) "
               "WHERE Abbrev=:state6;");
	query.bindValue(":state1", entry.state);
	query.bindValue(":state2", entry.state);
	query.bindValue(":state3", entry.state);
	query.bindValue(":state4", entry.state);
	query.bindValue(":state5", entry.state);
	query.bindValue(":state6", entry.state);
	
	if( !query.exec() ){
		addToLog( query.lastQuery() );
		addToLog( QString("Error %1: %2").arg(query.lastError().number()).arg(query.lastError().text()));
		return;
	}
	
	// tell the gui to update the tables
	if( undoStack.isEmpty() ) ui->action_Undo->setEnabled(false);
	emit dataUpdated();
}

void MainWindow::hideUnvisited(bool toggled)
{
	if( toggled )
		model->setFilter("Mileage<>0 OR GasAmt<>0");
	else
		model->setFilter("");
}

void MainWindow::resetDatabase()
{
	// confirm the choice
	QMessageBox::StandardButton ret = QMessageBox::warning(this, "Reset Database",
					"This operation will DELETE ALL DATA entered to this point.\n"
					"This is NOT REVERSABLE. Are you sure you want to do this?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if( ret != QMessageBox::Yes ) return;

	QFile file("mileage.sql");
	if(file.exists()) {
		db.close();
		file.rename("mileage.sql.old");
		ui->txtHistory->setPlainText("");
		validateDatabase();
		setInitialValues();
		emit dataUpdated();
	}
}

void MainWindow::bugReport()
{
	QMessageBox::information(this, "Report a Bug",
		"If you believe that you've found a bug in the program, \n"
		"or if you have an idea to make it better, \n"
		"feel free to email me at <vergenzs@gmail.com>. \n"
		"Please include as detailed a description of the bug as possible, \n"
		"including the error message and any steps I could take to reproduce it, \n"
		"as well as the file \"mileage.sql\", located in the same folder \n"
		"as this program. Your support is what makes this program run, so \n"
		"don't hesitate to contact me. Thank you for your support."
	);
}

void MainWindow::aboutMileStat()
{
	QMessageBox::information(this, "About MileStat",
		"This program, MileStat, was written by Steven Vergenz "
		"and published under the terms of the GNU General Public License "
		"version 3 (GPL). Please see http://www.gnu.org/licenses/gpl-3.0.txt "
		"for the full text of this license. If you would like a copy of the "
		"source code, please email Steven Vergenz at <vergenzs@gmail.com>. "
		"\n\n"
		"This program was designed to help commercial US truckers keep track "
		"of their driving mileage in each state, as well has how much gasoline "
		"they've purchased per state. This makes it easier to calculate "
		"the amount of a percentage-based gasoline tax. "
		"If you have any questions about this program whatsoever, "
		"feel free to contact the author at <vergenzs@gmail.com>."
	);
}

void MainWindow::setInitialValues()
{
	QSqlQuery query;

	query.exec("SELECT Date,Odometer,State FROM ("
	           "SELECT Date,Odometer,Destination AS State FROM Crossings "
	           "UNION "
	           "SELECT Date,Odometer,State FROM Refuels "
	           ") ORDER BY Date DESC LIMIT 1;");
	query.first();
	
	if( query.isValid() ){
		ui->comboCurState->setCurrentIndex(
			abbrevs.indexOf(query.value(2).toString()) +1
		);
		ui->spinOdometer->setValue(query.value(1).toInt());
		ui->dteTime->setDateTime(QDateTime::fromTime_t(query.value(0).toInt() +60));
	}
	else
		ui->dteTime->setDateTime( QDateTime::currentDateTime() );
	
	query.clear();
	
	
	// show the history
	if( !query.exec("SELECT Date,Odometer,Origin,Destination,Distance "
	           "FROM (SELECT Date,Odometer,Origin,Destination,Distance FROM Crossings "
	                "UNION SELECT Date,Odometer,Volume,Cost,State FROM Refuels) "
	           "ORDER BY Date LIMIT 50;") ){
		addToLog(QString("%1: %2 (History)")
		         .arg(query.lastError().number())
		         .arg(query.lastError().text())
		);
		return;
	}
	query.first();
	
	while( query.isValid() ){
		if( !query.value(2).toString().at(0).isNumber() ){ // indicates it is NOT a refuel
			addToLog(QString("%1 [%L2]: You crossed from %3 to %4, traveling %5 miles in %3.")
				.arg(QDateTime::fromTime_t(query.value(0).toInt()).toString(dtFormat))
				.arg(query.value(1).toInt())
				.arg(query.value(2).toString())
				.arg(query.value(3).toString())
				.arg(query.value(4).toInt())
			);
		}
		else{
			addToLog(QString("%1 [%L2]: You bought %3 gallons of gas for $%4 in %5.")
				.arg(QDateTime::fromTime_t(query.value(0).toInt()).toString(dtFormat))
				.arg(query.value(1).toInt())
				.arg(query.value(2).toDouble(), 0, 'f', 3)
				.arg(query.value(3).toDouble(), 0, 'f', 2)
				.arg(query.value(4).toString())
			);
		}
		query.next();
	}
	query.clear();
	addToLog("Values initialized.");
}

QString MainWindow::validateDatabase()
{
	db = QSqlDatabase::addDatabase("QSQLITE");
	QString dbName = qApp->applicationDirPath() + QDir::separator() + "mileage.sql";
	db.setDatabaseName(dbName);

	if( !db.open() ) return QString("Could not open database %1\n%2\n%3")
		.arg(dbName)
		.arg(db.lastError().text())
		.arg(qApp->libraryPaths().join(";"));

	QStringList tables = db.tables(QSql::Tables);
	if( !tables.contains("Refuels") 
	 || !tables.contains("Crossings")
	 || !tables.contains("States") )
		return populateDatabase();

	// finish up
	return QString();
}

QString MainWindow::populateDatabase()
{
	QSqlQuery query;

	// create the nonexistent States table
	if( !query.exec("CREATE TABLE IF NOT EXISTS States ("
			"Name varchar(15) NOT NULL, "
			"Abbrev varchar(5) NOT NULL PRIMARY KEY, "
			"Mileage int, "
			"PurchaseAmt real, "
			"GasAmt real, "
			"NumRefuels int, "
			"NumEntries int "
			")") )
		return QString("%1: %2 (States)")
			.arg(query.lastError().number())
			.arg(query.lastError().text());

	// populate the empty States database
	for(int i=0; i<names.size(); i++)
	{
		query.prepare("INSERT INTO States"
		              "(Name,Abbrev,NumEntries,Mileage,NumRefuels,GasAmt,PurchaseAmt) "
		              "VALUES(?,?,?,?,?,?,?);");
		query.addBindValue(names[i]);
		query.addBindValue(abbrevs[i]);
		query.addBindValue(0);
		query.addBindValue(0);
		query.addBindValue(0);
		query.addBindValue(0);
		query.addBindValue(0);
		if( !query.exec() )
			return QString("%1: %2 (States)")
				.arg(query.lastError().number())
				.arg(query.lastError().text());
	}

	// create the nonexistent Refuels table
	if( !query.exec("CREATE TABLE IF NOT EXISTS Refuels ("
			"Date integer, "
	                "Odometer int, "
			"State varchar(5), "
			"Volume real, "
			"Cost real, "
	                "PRIMARY KEY(Date), "
			"FOREIGN KEY(State) REFERENCES States"
			");") )
		return QString("%1: %2 (States)")
			.arg(query.lastError().number())
			.arg(query.lastError().text());
	
	// create the nonexistent Crossings table
	if( !query.exec("CREATE TABLE IF NOT EXISTS Crossings ("
	               "Date integer, "
	               "Odometer int , "
	               "Origin varchar(5), "
	               "Destination varchar(5), "
	               "Distance int DEFAULT 0, "
	               "PRIMARY KEY(Date), "
	               "FOREIGN KEY(Origin) REFERENCES States, "
	               "FOREIGN KEY(Destination) REFERENCES States"
	               ");") )
		return QString("%1: %2 (States)")
			.arg(query.lastError().number())
			.arg(query.lastError().text());
	
	addToLog("New database initialized.");
	
	return QString();
}

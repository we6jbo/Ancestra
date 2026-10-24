#pragma once
#include <QMainWindow>
#include <QMap>
#include <QSet>
#include <QStringList>
class QTableWidget; class QTreeWidget; class QProgressBar; class QLabel; class QTextEdit; class QPushButton; class QTimer;
namespace Ui { class MainWindow; }
class MainWindow : public QMainWindow {
 Q_OBJECT
public: explicit MainWindow(QWidget *parent=nullptr); ~MainWindow();
private:
 Ui::MainWindow *ui;
 QTreeWidget *tree; QProgressBar *progress; QLabel *summary; QLabel *sources; QTextEdit *detail; QTextEdit *vibeInfo;
 QTimer *timer; QTableWidget *externalTable, *researchedTable; bool syncing=false;
 QStringList baseline; QMap<QString,QString> codes, stories, details; QMap<QString,QSet<QString>> parents,children;
 QSet<QString> people;
 void reload(); void prepareWorkflow(); void showPerson(); void refreshExternal(); void refreshResearched(); void saveResearchStates();
};

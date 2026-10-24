#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "challenge.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonParseError>
#include <QFileDialog>
#include <QDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QSet>
#include <QStatusBar>
#include <algorithm>
static const QString stateFile="/var/lib/kinmap/kinmap_state.json";
static const QString treeFile="/tmp/kinmap_family_tree_26-27.json";
static const QString storyFile="/home/we6jbo/.family_story/real stories.db";
static const QString workflowDir="/home/we6jbo/.treeresearchoct2326";
static const QString scriptFile=workflowDir+"/ancestra_start_vibe.sh";
static const QString externalDir=workflowDir+"/external";
static const QString externalManifest=externalDir+"/manifest.json";
static const QString researchStatus=workflowDir+"/researched_status.json";
static const QString provenance="TG918273,TG654147";
// Persist only transient /tmp sources. Never modify the KinMap state or stories DB.
static QString cacheDirectory(){return QDir::homePath()+"/.cache/ancestra";}
static QString cachePath(const QString &source){return cacheDirectory()+"/"+QFileInfo(source).fileName();}
static bool validData(const QString &source,const QByteArray &data){
 if(data.isEmpty())return false;
 if(source==treeFile){QJsonParseError e;auto d=QJsonDocument::fromJson(data,&e);return e.error==QJsonParseError::NoError&&d.isObject()&&d.object().value("people").isArray();}
 if(source==scriptFile)return data.startsWith("#!/")&&!data.contains('\0');
 return true;
}
static bool saveCache(const QString &path,const QByteArray &data){
 if(!QDir().mkpath(cacheDirectory()))return false;
 QFile old(path);if(old.open(QIODevice::ReadOnly)&&old.readAll()==data)return true;
 QSaveFile out(path);if(!out.open(QIODevice::WriteOnly))return false;
 if(out.write(data)!=data.size()){out.cancelWriting();return false;}
 return out.commit();
}
// Prefer validated live file. Keep last valid cached snapshot if /tmp is missing or incomplete.
static QString resolveTransient(const QString &source){
 QFile live(source);
 if(live.open(QIODevice::ReadOnly)){
  QByteArray contents=live.readAll();
  if(validData(source,contents)){
   saveCache(cachePath(source),contents);
   return source;
  }
 }
 QFile cached(cachePath(source));
 if(cached.open(QIODevice::ReadOnly)&&validData(source,cached.readAll()))return cachePath(source);
 return {};
}

static QJsonObject readJson(QString path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};auto d=QJsonDocument::fromJson(f.readAll());return d.isObject()?d.object():QJsonObject{};}
static void writeIfAbsent(QString path,QByteArray data){if(QFileInfo::exists(path))return;QFile f(path);if(f.open(QIODevice::WriteOnly|QIODevice::NewOnly)){f.write(data);f.close();}}
MainWindow::MainWindow(QWidget *parent):QMainWindow(parent),ui(new Ui::MainWindow){
 ui->setupUi(this);
 auto *r=qobject_cast<QVBoxLayout*>(ui->researchTab->layout());
 auto *head=new QLabel("Family research • read-only source files • TG/AKA story matching",this);r->addWidget(head);
 progress=new QProgressBar(this);progress->setRange(0,100);r->addWidget(progress);
 summary=new QLabel(this);summary->setWordWrap(true);r->addWidget(summary);
 sources=new QLabel(this);sources->setWordWrap(true);r->addWidget(sources);
 auto *refresh=new QPushButton("Refresh records",this);r->addWidget(refresh);connect(refresh,&QPushButton::clicked,this,&MainWindow::reload);
 auto *pair=new QHBoxLayout();r->addLayout(pair);tree=new QTreeWidget(this);tree->setHeaderLabels({"Family group / Person","TG or AKA","Story"});tree->setMinimumWidth(550);tree->header()->setSectionResizeMode(0,QHeaderView::Stretch);pair->addWidget(tree,3);
 detail=new QTextEdit(this);detail->setReadOnly(true);pair->addWidget(detail,2);connect(tree,&QTreeWidget::itemSelectionChanged,this,&MainWindow::showPerson);
 auto *v=qobject_cast<QVBoxLayout*>(ui->vibeTab->layout());
 vibeInfo=new QTextEdit(this);vibeInfo->setReadOnly(true);v->addWidget(vibeInfo);
 auto *copy=new QPushButton("Copy Vibe terminal command to clipboard",this);v->addWidget(copy);
 connect(copy,&QPushButton::clicked,this,[this](){QString selected=resolveTransient(scriptFile);QFile f(selected);if(selected.isEmpty()||!f.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Script unavailable","Neither the live launcher nor its cached copy is available.");return;}QApplication::clipboard()->setText(QStringLiteral("bash ")+QStringLiteral("\"")+selected+QStringLiteral("\""));statusBar()->showMessage("Vibe command copied — paste and run manually in your own terminal",5000);});
 auto *exLayout=qobject_cast<QVBoxLayout*>(ui->externalTab->layout());
 auto *exDescription=new QLabel("External research supplied through ChatGPT or imported locally. Red = no verified use; green = evidence recorded. C is retention/use in research, not proof of a Moltbook post.",this);exDescription->setWordWrap(true);exLayout->addWidget(exDescription);
 externalTable=new QTableWidget(this);externalTable->setColumnCount(3);externalTable->setHorizontalHeaderLabels({"Information / Source", "Vibe read", "Retained & used"});externalTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);externalTable->setSelectionBehavior(QAbstractItemView::SelectRows);externalTable->setEditTriggers(QAbstractItemView::NoEditTriggers);externalTable->setWordWrap(true);externalTable->verticalHeader()->setDefaultSectionSize(38);externalTable->setToolTip("Double-click a source row to read its grouped research notes.");exLayout->addWidget(externalTable);
 auto *exButtons=new QHBoxLayout();exLayout->addLayout(exButtons);
 auto *addExternal=new QPushButton("Import external file",this);exButtons->addWidget(addExternal);
 connect(addExternal,&QPushButton::clicked,this,[this](){auto path=QFileDialog::getOpenFileName(this,"Import external research");if(path.isEmpty())return;QFile f(path);if(!f.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Import failed",f.errorString());return;}auto data=f.readAll();if(data.size()>5*1024*1024){QMessageBox::warning(this,"Too large","Limit is 5 MiB per imported file.");return;}QDir().mkpath(externalDir+"/sources");auto name=QFileInfo(path).fileName();QString dest=externalDir+"/sources/"+name;int index=2;while(QFileInfo::exists(dest)){dest=externalDir+"/sources/"+QString::number(index++)+"_"+name;}QSaveFile out(dest);if(!out.open(QIODevice::WriteOnly)||out.write(data)!=data.size()||!out.commit()){QMessageBox::warning(this,"Import failed","Could not store source.");return;}auto obj=readJson(externalManifest);auto entries=obj.value("entries").toArray();QJsonObject entry{{"id",QFileInfo(dest).baseName()},{"title",name},{"source",dest},{"read_confirmed",false},{"retained_confirmed",false},{"original_name",name}};entries.append(entry);obj.insert("entries",entries);QSaveFile mf(externalManifest);if(mf.open(QIODevice::WriteOnly)){mf.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));mf.commit();}refreshExternal();});
 connect(externalTable,&QTableWidget::cellDoubleClicked,this,[this](int row,int){
  auto arr=readJson(externalManifest).value("entries").toArray();
  if(row<0||row>=arr.size())return;
  auto obj=arr[row].toObject();QFile f(obj.value("source").toString());
  if(!f.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Source unavailable",f.errorString());return;}
  auto *dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle(obj.value("title").toString());dialog->resize(850,640);
  auto *layout=new QVBoxLayout(dialog);auto *viewer=new QTextEdit(dialog);viewer->setReadOnly(true);viewer->setPlainText(QString::fromUtf8(f.readAll()));layout->addWidget(viewer);
  auto *close=new QPushButton("Close",dialog);layout->addWidget(close);connect(close,&QPushButton::clicked,dialog,&QDialog::accept);dialog->show();
 });
 auto *openExternal=new QPushButton("Open external research folder",this);exButtons->addWidget(openExternal);connect(openExternal,&QPushButton::clicked,this,[](){QDesktopServices::openUrl(QUrl::fromLocalFile(externalDir));});
 auto *rsLayout=qobject_cast<QVBoxLayout*>(ui->researchedTab->layout());
 auto *rsHelp=new QLabel("Checked = Vibe has exhausted currently available leads (not absolute completeness). Check Interactive to request a conversation with Vibe; Vibe may clear it only after you say the session is complete. You can manually clear either box.",this);rsHelp->setWordWrap(true);rsLayout->addWidget(rsHelp);
 researchedTable=new QTableWidget(this);researchedTable->setColumnCount(3);researchedTable->setHorizontalHeaderLabels({"Family member", "Research exhausted", "Interactive requested"});researchedTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);rsLayout->addWidget(researchedTable);
 connect(researchedTable,&QTableWidget::itemChanged,this,[this](QTableWidgetItem *item){if(!syncing && item && item->column()>0)saveResearchStates();});
 auto *check=new QPushButton("Verify challenge time",this);v->addWidget(check);connect(check,&QPushButton::clicked,this,[this](){bool ok=verify();QMessageBox::information(this,"Challenge",ok?"Time verified; see terminal output.":"Challenge not verified; see terminal. Research remains available.");});
 prepareWorkflow();reload();timer=new QTimer(this);connect(timer,&QTimer::timeout,this,&MainWindow::reload);timer->start(10000);
}
MainWindow::~MainWindow(){delete ui;}
void MainWindow::prepareWorkflow(){
 QDir().mkpath(workflowDir); QDir().mkpath(externalDir+"/sources");
 writeIfAbsent(workflowDir+"/init.md",R"MD(# Ancestra Vibe Research
Read research_protocol.md first, then work through plan.md, moltbook.md, tree.md and the EXTERNAL manifest in that order. If any is missing, create it interactively with Jeremiah before proceeding. Ask for permission before accessing websites or publishing anything. Preserve source evidence, document citations and uncertainty, distinguish tentative from verified relationships. Do not alter original KinMap files or story database. Work within this research directory and update Ancestra_Manual_Updates.docx every research session, carrying forward all prior people and tasks and explaining exact manual changes to the JSON and story database with source citations and confidence levels. Never invent evidence. Respect privacy of living people and the exact protected birth years 1981, 1960, 1926, 1928 for public publishing; do not upload private family data or access protected services without explicit authorization.
)MD");
 QByteArray shell=R"SH(#!/usr/bin/env bash
set -euo pipefail
cd /home/we6jbo/.treeresearchoct2326
if ! command -v vibe >/dev/null 2>&1; then
  echo 'Mistral Vibe CLI (vibe) not found in PATH.' >&2
  exit 1
fi
if [[ ! -f init.md || ! -f research_protocol.md ]]; then
  echo 'Missing init.md or research_protocol.md. Open Ancestra and reinstall its research files before proceeding.' >&2
  exit 2
fi
printf 'Ancestra research starting in %s\n' "$PWD"
# Positional initial prompt starts the normal INTERACTIVE Vibe UI.
# Do not use -p: that switches Vibe into non-interactive mode.
PROMPT=$(cat <<'ANCESTRA_RESEARCH_PROMPT'
Begin the Ancestra family research workflow NOW; do not stop at a blank prompt.
Read init.md and research_protocol.md first, then explain briefly what you found and begin the first actionable step. Do not skip the interactive approvals.
1. Review plan.md for website permissions, search sources, research plan and evidence. If missing, create it interactively with me; do not replace existing plans.
2. Review moltbook.md for privacy, potential outreach, and my explicit permission before any publishing or other outward sharing. Never publish automatically.
3. Review tree.md for missing relationships and stories, plus the priorities and status checkboxes in researched_status.json. Ask me questions for interactive-requested people and wait for my response.
4. Review external/manifest.json and linked External source files, including Luedtke and grouped family leads. Treat all newly supplied claims as unconfirmed until corroborated. Distinguish Vibe read/retained status from actually posting to Moltbook and record evidence before changing statuses.
5. Read and reconcile unconfirmed.md and confirmed.md; preserve prior work, sources, and uncertainty. Look for missing stories and family connections under plan.md-approved research sources.
6. At the end of every research session, update Ancestra_Manual_Updates.docx (including no-findings sessions). Keep previous people and open tasks and give carefully sourced instructions for ME to manually update the JSON and SQLite source files. If DOCX tooling is unavailable, explain the blocker and preserve a Markdown draft without falsely claiming a DOCX update.
Do not modify /var/lib/kinmap/kinmap_state.json, the family-tree JSON, or the real stories.db database. Preserve and follow the exact privacy restrictions in the research protocol.
Start by opening init.md and research_protocol.md, then proceed interactively with the first unresolved step.
ANCESTRA_RESEARCH_PROMPT
)
exec vibe "$PROMPT"
)SH";
 // Recover the launcher from cache after reboot, or seed it from the template.
 // Do not overwrite an existing customized launcher.
 QString launcher=resolveTransient(scriptFile);
 if(launcher.isEmpty()){
  writeIfAbsent(scriptFile,shell);
  launcher=resolveTransient(scriptFile);
 }
 if(launcher==cachePath(scriptFile)&&!QFileInfo::exists(scriptFile)){
  QFile cached(launcher);
  if(cached.open(QIODevice::ReadOnly))writeIfAbsent(scriptFile,cached.readAll());
  launcher=resolveTransient(scriptFile);
 }
 if(QFileInfo::exists(scriptFile))QFile::setPermissions(scriptFile,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner);
 vibeInfo->setPlainText("Workflow folder: "+workflowDir+"\nLauncher: "+scriptFile+"\n\nAfter you paste the copied command into YOUR terminal and press Enter, the launcher starts interactive Vibe with the initial Ancestra research instruction already submitted. Ancestra never starts Vibe or opens a terminal.\n\nRequired interactive documents (not overwritten):\n1. plan.md — website research permissions, sources and evidence plan\n2. moltbook.md — privacy review and permission before sharing\n3. tree.md — research tasks for missing relationships and stories\n4. EXTERNAL — ingest reviewed ChatGPT uploads and log confirmed use\n5. Ancestra_Manual_Updates.docx — ALWAYS updated by Vibe after research; carry forward open tasks and give exact manual JSON/DB instructions with cited evidence\nResearch rules: research_protocol.md (updated by Ancestra)\n\nThe GUI never changes original KinMap or SQLite records.\nChallenge verification is optional and never blocks startup.");
}
void MainWindow::reload(){
 QString treeSource=resolveTransient(treeFile);
 auto raw=treeSource.isEmpty()?QJsonObject{}:readJson(treeSource),state=readJson(stateFile);
 bool foundState=QFileInfo::exists(stateFile);
 QMap<QString,QString> newCodes,newDetails,newStories; QMap<QString,QStringList> allCodes;QMap<QString,QSet<QString>> newParents,newChildren;QSet<QString> all;QStringList base;
 for(const auto &i:raw.value("people").toArray()){auto o=i.toObject();QString n=o.value("display_name").toString();if(!n.isEmpty()){all.insert(n);base.append(n);newDetails[n]=o.value("details").toString();}}
 for(const auto &i:raw.value("identifiers").toArray()){auto o=i.toObject();QString n=o.value("display_name").toString(),c=o.value("code").toString();if(!n.isEmpty()&&!c.isEmpty()){allCodes[n].append(c);if(!newCodes.contains(n)||c.startsWith("TG"))newCodes[n]=c;}}
 for(const auto &i:state.value("local_people").toArray()){auto o=i.toObject();QString n=o.value("display_name").toString();if(n.isEmpty())continue;all.insert(n);QStringList attr;for(auto key:{"birth_date","birth_place","death_date","death_place","details"}){auto x=o.value(key).toString();if(!x.isEmpty())attr<<QString::fromLatin1(key)+": "+x;}if(!attr.isEmpty())newDetails[n]+="\n"+attr.join("\n");auto cs=o.value("tg_codes").toArray();if(!cs.isEmpty())newCodes[n]=cs.first().toString();}
 auto ingest=[&](QJsonArray arr){for(const auto &v:arr){auto o=v.toObject();auto child=o.value("child").toString(),parent=o.value("parent").toString();if(child.isEmpty()||parent.isEmpty())continue;all.insert(child);all.insert(parent);newParents[child].insert(parent);newChildren[parent].insert(child);}};
 ingest(raw.value("relationships").toArray());ingest(state.value("local_relationships").toArray());
 // Open source DB read-only; never modify or create it.
 if(QFileInfo::exists(storyFile)){QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE","ancestra_readonly");db.setDatabaseName(storyFile);db.setConnectOptions("QSQLITE_OPEN_READONLY");if(db.open()){QSqlQuery q(db);if(q.exec("SELECT code,body FROM real_stories")){while(q.next())newStories[q.value(0).toString()]=q.value(1).toString();}db.close();}db=QSqlDatabase();QSqlDatabase::removeDatabase("ancestra_readonly");}
 for(auto it=allCodes.cbegin();it!=allCodes.cend();++it){for(const auto &c:it.value()){if(newStories.contains(c)){newCodes[it.key()]=c;break;}}}
 baseline=base;codes=newCodes;details=newDetails;parents=newParents;children=newChildren;people=all;stories=newStories;
 int relationshipGoals=0,relationshipDone=0;for(const auto &n:base){if(!newParents.contains(n)||newParents.value(n).isEmpty())++relationshipGoals;else ++relationshipDone; if(!newChildren.contains(n)||newChildren.value(n).isEmpty())++relationshipGoals;else ++relationshipDone;}
 // Count story completion for both baseline and newly appearing persons.
 int storyDone=0;for(const auto &n:all){if(!newStories.value(newCodes.value(n)).trimmed().isEmpty())storyDone++;}
 const int relTotal=relationshipGoals+relationshipDone;
 double relationshipFraction=relTotal?double(relationshipDone)/relTotal:0.0;
 double storyFraction=all.isEmpty()?0.0:double(storyDone)/all.size();
 int percent=qRound(50*relationshipFraction+50*storyFraction);progress->setValue(percent);
 summary->setText(QString("Research coverage: %1%  |  Relationship coverage: %2/%3 (50% weight)  |  Stories: %4/%5 (50% weight)\nBaseline: %6 uploaded tree people. Relationship coverage checks whether each baseline person has at least one recorded parent and child; this is a tracking target, not proof everyone had children. New people count toward story coverage.").arg(percent).arg(relationshipDone).arg(relTotal).arg(storyDone).arg(all.size()).arg(base.size()));
 sources->setText(QString("Tree: %1  |  State: %2  |  Stories DB: %3  •  Refresh every 10 seconds  •  Cache: %4  •  Provenance: %5").arg(treeSource.isEmpty()?"MISSING":(treeSource==treeFile?"LIVE /tmp":"CACHED FALLBACK")).arg(foundState?"available":"MISSING").arg(QFileInfo::exists(storyFile)?"available":"MISSING").arg(cacheDirectory()).arg(provenance));
 QString selected=tree->currentItem()?tree->currentItem()->data(0,Qt::UserRole).toString():QString();tree->clear();
 // grouped by parent relationship, with unlinked members listed separately.
 QMap<QString,QStringList> groups;for(const auto &n:all){QString parent=newParents.value(n).isEmpty()?"Unlinked / root family":"Children of "+QStringList(newParents.value(n).values()).join(" & ");groups[parent].append(n);}
 for(auto it=groups.cbegin();it!=groups.cend();++it){auto *grp=new QTreeWidgetItem(tree,{it.key()});auto ns=it.value();ns.sort(Qt::CaseInsensitive);for(const auto &n:ns){auto *item=new QTreeWidgetItem(grp,{n,newCodes.value(n,"(no code)"),newStories.contains(newCodes.value(n))?"Story recorded":"Story needed"});item->setData(0,Qt::UserRole,n);if(n==selected)tree->setCurrentItem(item);}}
 tree->expandAll();showPerson();refreshExternal();refreshResearched();
}
void MainWindow::showPerson(){auto *it=tree->currentItem();QString n=it?it->data(0,Qt::UserRole).toString():QString();if(n.isEmpty()){detail->setPlainText("Choose a family member to see parents, children, details, and the associated story.");return;}QString code=codes.value(n);detail->setPlainText("Person: "+n+"\nIdentifier: "+code+"\nParents: "+QStringList(parents.value(n).values()).join(", ")+"\nChildren: "+QStringList(children.value(n).values()).join(", ")+"\n\nDETAILS\n"+details.value(n)+"\n\nREAL STORY\n"+stories.value(code,"No story matched to this identifier."));}

void MainWindow::refreshExternal(){
 auto rows=readJson(externalManifest).value("entries").toArray();externalTable->setRowCount(rows.size());
 for(int i=0;i<rows.size();++i){auto o=rows[i].toObject();auto title=o.value("title").toString();auto src=o.value("source").toString();bool present=QFileInfo::exists(src);
 auto *label=new QTableWidgetItem(title+(present?"":" [SOURCE MISSING]"));label->setToolTip(src);externalTable->setItem(i,0,label);
 for(int c=1;c<=2;++c){bool ok=o.value(c==1?"read_confirmed":"retained_confirmed").toBool();auto *item=new QTableWidgetItem(ok?"🟩":"🟥");item->setTextAlignment(Qt::AlignCenter);item->setToolTip(c==1?"Confirmed Vibe successfully read and understood this source":"Confirmed Vibe incorporated this source into durable research notes/report; independent Moltbook posting evidence required");externalTable->setItem(i,c,item);}
 }
}
void MainWindow::refreshResearched(){
 // Preserve user flags by TG code where possible, rather than display name alone.
 auto saved=readJson(researchStatus).value("people").toObject();syncing=true;
 QStringList names=people.values();names.sort(Qt::CaseInsensitive);researchedTable->setRowCount(names.size());
 for(int r=0;r<names.size();++r){const QString n=names[r],key=codes.value(n,n);auto record=saved.value(key).toObject();auto *name=new QTableWidgetItem(n);name->setData(Qt::UserRole,key);name->setFlags(name->flags()&~Qt::ItemIsEditable);researchedTable->setItem(r,0,name);
 for(int c=1;c<=2;++c){auto *cell=new QTableWidgetItem();cell->setFlags((cell->flags()|Qt::ItemIsUserCheckable)&~Qt::ItemIsEditable);cell->setCheckState(record.value(c==1?"exhausted":"interactive_requested").toBool()?Qt::Checked:Qt::Unchecked);researchedTable->setItem(r,c,cell);}
 }
 syncing=false;
}
void MainWindow::saveResearchStates(){
 auto obj=readJson(researchStatus);auto all=obj.value("people").toObject();
 for(int r=0;r<researchedTable->rowCount();++r){auto *n=researchedTable->item(r,0),*a=researchedTable->item(r,1),*b=researchedTable->item(r,2);if(!n||!a||!b)continue;auto key=n->data(Qt::UserRole).toString();auto data=all.value(key).toObject();data.insert("name",n->text());data.insert("exhausted",a->checkState()==Qt::Checked);data.insert("interactive_requested",b->checkState()==Qt::Checked);all.insert(key,data);}
 obj.insert("people",all);QSaveFile f(researchStatus);if(f.open(QIODevice::WriteOnly)){f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));if(!f.commit())statusBar()->showMessage("Failed to save research status",8000);}
}

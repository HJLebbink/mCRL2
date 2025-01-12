// Author(s): Olav Bunte
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "mainwindow.h"

#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QInputDialog>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QStandardItemModel>

MainWindow::MainWindow(const QString& inputFilePath, QWidget* parent)
    : QMainWindow(parent)
{
  specificationEditor = new CodeEditor(this);
  specificationEditor->setPlaceholderText("Type your mCRL2 specification here");
  specificationEditor->setHighlightingRules(true);
  setCentralWidget(specificationEditor);

  settings = new QSettings("mCRL2", "mcrl2ide");

  fileSystem = new FileSystem(specificationEditor, settings, this);
  processSystem = new ProcessSystem(fileSystem);

  setupMenuBar();
  setupToolbar();
  setupDocks();

  processSystem->setConsoleDock(consoleDock);

  findAndReplaceDialog = new FindAndReplaceDialog(specificationEditor, this);
  addPropertyDialog =
      new AddEditPropertyDialog(true, processSystem, fileSystem, this);
  connect(addPropertyDialog, SIGNAL(accepted()), this,
          SLOT(actionAddPropertyResult()));

  /* change the UI whenever a new project has opened */
  connect(fileSystem, SIGNAL(newProjectOpened()), this,
          SLOT(onNewProjectOpened()));
  /* change the UI whenever the IDE enters specification only mode */
  connect(fileSystem, SIGNAL(enterSpecificationOnlyMode()), this,
          SLOT(onEnterSpecificationOnlyMode()));

  /* make saving a project only enabled whenever there are changes */
  saveAction->setEnabled(false);
  connect(specificationEditor, SIGNAL(modificationChanged(bool)), saveAction,
          SLOT(setEnabled(bool)));

  /* change the tool buttons to start or abort a tool depending on whether
   *   processes are running */
  for (ProcessType processType : PROCESSTYPES)
  {
    connect(processSystem->getProcessThread(processType),
            SIGNAL(statusChanged(bool, ProcessType)), this,
            SLOT(changeToolButtons(bool, ProcessType)));
  }

  /* reset the propertiesdock when the specification changes */
  connect(specificationEditor->document(), SIGNAL(modificationChanged(bool)),
          propertiesDock, SLOT(resetAllPropertyWidgets()));

  /* set the title of the main window */
  setWindowTitle("mCRL2 IDE - Unnamed project");
  if (settings->contains("geometry"))
  {
    restoreGeometry(settings->value("geometry").toByteArray());
  }
  else
  {
    resize(QSize(QDesktopWidget().availableGeometry(this).width() * 0.5,
                 QDesktopWidget().availableGeometry(this).height() * 0.75));
  }

  processSystem->testExecutableExistence();

  /* open a project if a project file is given */
  if (!inputFilePath.isEmpty())
  {
    actionOpenProject(inputFilePath);
  }
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupMenuBar()
{
  /* Create the File menu */
  QMenu* fileMenu = menuBar()->addMenu("File");

  newProjectAction =
      fileMenu->addAction(QIcon(":/icons/new_project.png"), "New Project", this,
                          SLOT(actionNewProject(bool)), QKeySequence::New);

  fileMenu->addSeparator();

  openProjectAction =
      fileMenu->addAction(QIcon(":/icons/open_project.png"), "Open Project",
                          this, SLOT(actionOpenProject()), QKeySequence::Open);

  fileMenu->addSeparator();

  saveAction = fileMenu->addAction(saveProjectIcon, saveProjectText, this,
                                   SLOT(actionSave()), QKeySequence::Save);

  saveAsAction =
      fileMenu->addAction(saveProjectAsText, this, SLOT(actionSaveAs()),
                          QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S));

  fileMenu->addSeparator();

  openProjectFolderInExplorerAction =
      fileMenu->addAction("Open Project Folder in Explorer", this,
                          SLOT(actionOpenProjectFolderInExplorer()));
  openProjectFolderInExplorerAction->setEnabled(false);

  fileMenu->addSeparator();

  exitAction = fileMenu->addAction("Exit", this, SLOT(close()),
                                   QKeySequence(Qt::CTRL + Qt::Key_Q));

  /* Create the Edit menu */
  QMenu* editMenu = menuBar()->addMenu("Edit");

  undoAction = editMenu->addAction("Undo", specificationEditor, SLOT(undo()),
                                   QKeySequence::Undo);

  redoAction = editMenu->addAction("Redo", specificationEditor, SLOT(redo()),
                                   QKeySequence::Redo);

  editMenu->addSeparator();

  findAndReplaceAction =
      editMenu->addAction("Find and Replace", this,
                          SLOT(actionFindAndReplace()), QKeySequence::Find);

  editMenu->addSeparator();

  cutAction = editMenu->addAction("Cut", specificationEditor, SLOT(cut()),
                                  QKeySequence::Cut);

  copyAction = editMenu->addAction("Copy", specificationEditor, SLOT(copy()));
  copyAction->setShortcut(QKeySequence::Copy);

  pasteAction = editMenu->addAction("Paste", specificationEditor, SLOT(paste()),
                                    QKeySequence::Paste);

  deleteAction = editMenu->addAction("Delete", specificationEditor,
                                     SLOT(deleteChar()), QKeySequence::Delete);

  selectAllAction =
      editMenu->addAction("Select All", specificationEditor, SLOT(selectAll()),
                          QKeySequence::SelectAll);

  /* Create the View Menu (more actions are added in setupDocks())*/
  viewMenu = menuBar()->addMenu("View");

  zoomInAction =
      viewMenu->addAction("Zoom in", specificationEditor, SLOT(zoomIn()),
                          QKeySequence(Qt::CTRL + Qt::Key_Equal));

  zoomOutAction = viewMenu->addAction("Zoom out", specificationEditor,
                                      SLOT(zoomOut()), QKeySequence::ZoomOut);

  viewMenu->addSeparator();

  /* Create the Tools menu */
  QMenu* toolsMenu = menuBar()->addMenu("Tools");

  parseAction = toolsMenu->addAction(parseStartIcon, parseStartText, this,
                                     SLOT(actionParse()),
                                     QKeySequence(Qt::ALT + Qt::Key_P));

  simulateAction = toolsMenu->addAction(simulateStartIcon, simulateStartText,
                                        this, SLOT(actionSimulate()),
                                        QKeySequence(Qt::ALT + Qt::Key_S));

  toolsMenu->addSeparator();

  showLtsAction = toolsMenu->addAction(showLtsStartIcon, showLtsStartText, this,
                                       SLOT(actionShowLts()),
                                       QKeySequence(Qt::ALT + Qt::Key_T));

  showReducedLtsAction = toolsMenu->addAction(
      showReducedLtsStartIcon, showReducedLtsStartText, this,
      SLOT(actionShowReducedLts()), QKeySequence(Qt::ALT + Qt::Key_R));

  toolsMenu->addSeparator();

  addPropertyAction = toolsMenu->addAction(
      QIcon(":/icons/add_property.png"), "Add Property", this,
      SLOT(actionAddProperty()), QKeySequence(Qt::ALT + Qt::Key_A));

  importPropertyAction = toolsMenu->addAction(
      "Import Property", this, SLOT(actionImportProperty()),
      QKeySequence(Qt::ALT + Qt::Key_I));

  verifyAllPropertiesAction = toolsMenu->addAction(
      verifyAllPropertiesStartIcon, verifyAllPropertiesStartText, this,
      SLOT(actionVerifyAllProperties()), QKeySequence(Qt::ALT + Qt::Key_V));

  /* create the options menu */
  QMenu* optionsMenu = menuBar()->addMenu("Options");

  saveIntermediateFilesMenu =
      optionsMenu->addMenu("Save intermediate files to project");
  saveIntermediateFilesMenu->setEnabled(false);
  saveIntermediateFilesMenu->setToolTipsVisible(true);

  for (std::pair<IntermediateFileType, QString> item :
       INTERMEDIATEFILETYPENAMES)
  {
    QAction* saveFileAction = saveIntermediateFilesMenu->addAction(item.second);
    saveFileAction->setCheckable(true);
    saveFileAction->setProperty("filetype", item.first);
    saveFileAction->setToolTip("Changing this will only have effect on "
                               "processes that have not started yet");
    connect(saveFileAction, SIGNAL(toggled(bool)), fileSystem,
            SLOT(setSaveIntermediateFilesOptions(bool)));
  }
}

void MainWindow::setupToolbar()
{
  toolbar = addToolBar("Tools");
  toolbar->setIconSize(QSize(48, 48));

  /* create each toolbar item by adding the actions */
  toolbar->addAction(newProjectAction);
  toolbar->addAction(openProjectAction);
  toolbar->addAction(saveAction);
  toolbar->addSeparator();
  toolbar->addAction(parseAction);
  toolbar->addAction(simulateAction);
  toolbar->addSeparator();
  toolbar->addAction(showLtsAction);
  toolbar->addAction(showReducedLtsAction);
  toolbar->addSeparator();
  toolbar->addAction(addPropertyAction);
  toolbar->addAction(verifyAllPropertiesAction);
}

void MainWindow::setDocksToDefault()
{
  addDockWidget(propertiesDock->defaultArea, propertiesDock);
  addDockWidget(consoleDock->defaultArea, consoleDock);

  propertiesDock->setFloating(false);
  consoleDock->setFloating(false);

  propertiesDock->show();
  consoleDock->show();

  // Workaround for QTBUG-65592.
  propertiesDock->setObjectName("PropertiesDockObject");
  consoleDock->setObjectName("ConsoleDockObject");
  toolbar->setObjectName("ToolbarObject");
  QByteArray array = saveState();
  restoreState(array);
}

void MainWindow::setupDocks()
{
  /* instantiate the docks */
  propertiesDock = new PropertiesDock(processSystem, fileSystem, this);
  consoleDock = new ConsoleDock(this);

  /* add toggleable option in the view menu for each dock */
  viewMenu->addAction(propertiesDock->toggleViewAction());
  viewMenu->addAction(consoleDock->toggleViewAction());

  /* place the docks in the default dock layout */
  setDocksToDefault();

  /* add option to view menu to put all docks back to their default layout */
  viewMenu->addSeparator();
  viewMenu->addAction("Revert to default layout", this,
                      SLOT(setDocksToDefault()));
}

void MainWindow::onNewProjectOpened()
{
  /* change the title */
  setWindowTitle(QString("mCRL2 IDE - ").append(fileSystem->getProjectName()));

  /* add the properties to the dock */
  propertiesDock->setToNoProperties();
  for (Property property : fileSystem->getProperties())
  {
    propertiesDock->addProperty(property);
  }

  /* change the file buttons */
  changeFileButtons(false);
}

void MainWindow::onEnterSpecificationOnlyMode()
{
  /* change the title */
  setWindowTitle(QString("mCRL2 IDE - Specification only mode - ")
                     .append(fileSystem->getSpecificationFileName()));

  /* change the file buttons */
  changeFileButtons(true);
}

void MainWindow::actionNewProject(bool askToSave)
{
  fileSystem->newProject(askToSave);
}

void MainWindow::actionOpenProject(const QString& inputFilePath)
{
  if (inputFilePath.isEmpty())
  {
    fileSystem->openProject();
  }
  else
  {
    fileSystem->openFromArgument(inputFilePath);
  }
}

void MainWindow::actionSave()
{
  fileSystem->save();
}

void MainWindow::actionSaveAs()
{
  fileSystem->saveAs();
}

void MainWindow::actionOpenProjectFolderInExplorer()
{
  fileSystem->openProjectFolderInExplorer();
}

void MainWindow::actionFindAndReplace()
{
  if (findAndReplaceDialog->isVisible())
  {
    findAndReplaceDialog->setFocus();
    findAndReplaceDialog->activateWindow();
  }
  else
  {
    findAndReplaceDialog->show();
  }
}

bool MainWindow::assertProjectOpened()
{
  if (!fileSystem->projectOpened())
  {
    QMessageBox msgBox(
        QMessageBox::Information, "mCRL2 IDE",
        "To use this tool it is required to create or open a project first",
        QMessageBox::Ok, this, Qt::WindowCloseButtonHint);
    msgBox.exec();
    return false;
  }
  else
  {
    return true;
  }
}

bool MainWindow::assertSpecificationOpened()
{
  if (!fileSystem->getSpecificationFileName().isEmpty())
  {
    return true;
  }
  else
  {
    return assertProjectOpened();
  }
}

void MainWindow::actionParse()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Parsing))
    {
      processSystem->abortAllProcesses(ProcessType::Parsing);
    }
    else
    {
      processSystem->parseSpecification();
    }
  }
}

void MainWindow::actionSimulate()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Simulation))
    {
      processSystem->abortAllProcesses(ProcessType::Simulation);
    }
    else
    {
      processSystem->simulate();
    }
  }
}

void MainWindow::actionShowLts()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::LtsCreation))
    {
      processSystem->abortAllProcesses(ProcessType::LtsCreation);
    }
    else
    {
      lastLtsHasReduction = false;
      processSystem->showLts(mcrl2::lts::lts_eq_none);
    }
  }
}

void MainWindow::actionShowReducedLts()
{
  if (assertSpecificationOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::LtsCreation))
    {
      processSystem->abortAllProcesses(ProcessType::LtsCreation);
    }
    else
    {
      /* create a dialog for asking the user what reduction to use */
      QDialog reductionDialog(this, Qt::WindowCloseButtonHint);
      QVBoxLayout vbox;
      QLabel textLabel("Reduction:");
      EquivalenceComboBox reductionBox(&reductionDialog);
      QDialogButtonBox buttonBox(QDialogButtonBox::Cancel);

      vbox.addWidget(&textLabel);
      vbox.addWidget(&reductionBox);
      vbox.addWidget(&buttonBox);

      reductionDialog.setLayout(&vbox);

      connect(&reductionBox, SIGNAL(activated(int)), &reductionDialog,
              SLOT(accept()));
      connect(&buttonBox, SIGNAL(rejected()), &reductionDialog, SLOT(reject()));

      /* execute the dialog */
      if (reductionDialog.exec())
      {
        mcrl2::lts::lts_equivalence reduction =
            reductionBox.getSelectedEquivalence();

        lastLtsHasReduction = true;
        processSystem->showLts(reduction);
      }
    }
  }
}

void MainWindow::actionAddProperty()
{
  if (assertProjectOpened())
  {
    addPropertyDialog->clearFields();
    addPropertyDialog->resetFocus();
    if (addPropertyDialog->isVisible())
    {
      addPropertyDialog->activateWindow();
      addPropertyDialog->setFocus();
    }
    else
    {
      addPropertyDialog->show();
    }
  }
}

void MainWindow::actionAddPropertyResult()
{
  /* if successful (Add button was pressed), create the new property
   *   we don't need to save to file as this is already done by the dialog */
  Property property = addPropertyDialog->getProperty();
  fileSystem->newProperty(property);
  propertiesDock->addProperty(property);
}

void MainWindow::actionImportProperty()
{
  if (assertProjectOpened())
  {
    std::list<Property> importedProperties = fileSystem->importProperties();
    for (Property property : importedProperties)
    {
      propertiesDock->addProperty(property);
    }
  }
}

void MainWindow::actionVerifyAllProperties()
{
  if (assertProjectOpened())
  {
    if (processSystem->isThreadRunning(ProcessType::Verification))
    {
      processSystem->abortAllProcesses(ProcessType::Verification);
    }
    else
    {
      propertiesDock->verifyAllProperties();
    }
  }
}

void MainWindow::changeFileButtons(bool specificationOnlyMode)
{
  saveIntermediateFilesMenu->setEnabled(true);
  if (specificationOnlyMode)
  {
    saveAction->setText(saveSpecificationText);
    saveAction->setIcon(saveSpecificationIcon);
    saveAsAction->setText(saveSpecificationAsText);
    openProjectFolderInExplorerAction->setEnabled(false);
  }
  else
  {
    saveAction->setText(saveProjectText);
    saveAction->setIcon(saveProjectIcon);
    saveAsAction->setText(saveProjectAsText);
    openProjectFolderInExplorerAction->setEnabled(true);
  }
}

void MainWindow::changeToolButtons(bool toAbort, ProcessType processType)
{
  switch (processType)
  {
  case ProcessType::Parsing:
    if (toAbort)
    {
      parseAction->setText(parseAbortText);
      parseAction->setIcon(parseAbortIcon);
    }
    else
    {
      parseAction->setText(parseStartText);
      parseAction->setIcon(parseStartIcon);
    }
    break;
  case ProcessType::Simulation:
    if (toAbort)
    {
      simulateAction->setText(simulateAbortText);
      simulateAction->setIcon(simulateAbortIcon);
    }
    else
    {
      simulateAction->setText(simulateStartText);
      simulateAction->setIcon(simulateStartIcon);
    }
    break;
  case ProcessType::LtsCreation:
    if (toAbort)
    {
      if (lastLtsHasReduction)
      {
        showLtsAction->setEnabled(false);
        showReducedLtsAction->setText(showReducedLtsAbortText);
        showReducedLtsAction->setIcon(showReducedLtsAbortIcon);
      }
      else
      {
        showReducedLtsAction->setEnabled(false);
        showLtsAction->setText(showLtsAbortText);
        showLtsAction->setIcon(showLtsAbortIcon);
      }
    }
    else
    {
      showLtsAction->setEnabled(true);
      showLtsAction->setText(showLtsStartText);
      showLtsAction->setIcon(showLtsStartIcon);
      showReducedLtsAction->setEnabled(true);
      showReducedLtsAction->setText(showReducedLtsStartText);
      showReducedLtsAction->setIcon(showReducedLtsStartIcon);
    }
    break;
  case ProcessType::Verification:
    if (toAbort)
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesAbortText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesAbortIcon);
    }
    else
    {
      verifyAllPropertiesAction->setText(verifyAllPropertiesStartText);
      verifyAllPropertiesAction->setIcon(verifyAllPropertiesStartIcon);
    }
    break;
  default:
    break;
  }
}

bool MainWindow::event(QEvent* event)
{
  switch (event->type())
  {
  case QEvent::WindowActivate:
    /* if the specification has been modified outside of the IDE, ask to
     *   update the editor */
    if (!reloadIsBeingHandled &&
        (fileSystem->projectOpened() ||
         fileSystem->inSpecificationOnlyMode()) &&
        fileSystem->isSpecificationNewlyModifiedFromOutside())
    {
      reloadIsBeingHandled = true;
      QMessageBox::StandardButton result =
          QMessageBox::question(this, "mCRL2 IDE",
                                "The specification has been modified from "
                                "outside of the IDE, do you want to reload it?",
                                QMessageBox::Yes | QMessageBox::No);
      switch (result)
      {
      case QMessageBox::Yes:
        fileSystem->loadSpecification();
        break;
      case QMessageBox::No:
        specificationEditor->document()->setModified();
        break;
      default:
        break;
      }

      reloadIsBeingHandled = false;
    }

    break;

  case QEvent::Close:
    /* if there are changes, ask the user to save the project first */
    if (fileSystem->isSpecificationModified())
    {
      QMessageBox::StandardButton result = QMessageBox::question(
          this, "mCRL2 IDE",
          "There are changes in the current project, do you want to save?",
          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
      switch (result)
      {
      case QMessageBox::Yes:
        if (!fileSystem->save())
        {
          event->ignore();
          return false;
        }
        break;
      case QMessageBox::Cancel:
        event->ignore();
        return false;
      default:
        break;
      }
    }

    /* save the settings for the main window */
    settings->setValue("geometry", saveGeometry());

    /* remove the temporary folder */
    fileSystem->removeTemporaryFolder();

    /* abort all processes */
    for (ProcessType processType : PROCESSTYPES)
    {
      processSystem->abortAllProcesses(processType);
    }

    break;

  default:
    break;
  }

  return QMainWindow::event(event);
}

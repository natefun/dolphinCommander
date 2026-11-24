/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinmainwindow.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistview.h"
#include "kitemviews/kitemlistwidget.h"
#include "sidessplitter.h"

#include "admin/workerintegration.h"
#include "commandertools.h"
#include "dolphin_generalsettings.h"
#include "dolphinbookmarkhandler.h"
#include "dolphincontextmenu.h"
#include "dolphindockwidget.h"
#include "dolphinmainwindowadaptor.h"
#include "dolphinnavigatorswidgetaction.h"
#include "dolphinnewfilemenu.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphintabbar.h"
#include "dolphintabpage.h"
#include "dolphinurlnavigatorscontroller.h"
#include "dolphinviewcontainer.h"
#include "global.h"
#include "jobqueue.h"
#include "middleclickactioneventfilter.h"
#include "panels/favorites/favoritespanel.h"
#include "panels/places/placespanel.h"
#include "panels/terminal/terminalpanel.h"
#include "search/dolphinquery.h"
#include "selectionmode/actiontexthelper.h"
#include "views/columnsstrip.h"
#include "views/zoomlevelinfo.h"
#if KIO_VERSION >= QT_VERSION_CHECK(6, 24, 0)
#include "servicemenushortcutmanager.h"
#endif
#include "settings/dolphinsettingsdialog.h"
#include "statusbar/diskspaceusagemenu.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/dolphinremoteencoding.h"
#include "views/dolphinviewactionhandler.h"
#include "views/draganddrophelper.h"
#include "views/viewproperties.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KAuthorized>
#include <KColorSchemeManager>
#include <KColorSchemeMenu>
#include <KConfig>
#include <KConfigGui>
#include <KDialogJobUiDelegate>
#include <KDualAction>
#include <KFileItemListProperties>
#include <KFileMetaData/UserMetaData>
#include <KGlobalAccel>
#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>
#include <KIO/CopyJob>
#include <KIO/Global>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KIconLoader>
#include <KIconTheme>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KRecentFilesAction>
#include <KRuntimePlatform>
#include <KService>
#include <KShell>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KSycoca>
#include <KTerminalLauncherJob>
#include <KToggleAction>
#include <KToolBar>
#include <KToolBarPopupAction>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KWindowSystem>
#include <KXMLGUIFactory>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QPixmap>
#ifdef Q_OS_LINUX
#include <sys/xattr.h>
#endif

#include <kwidgetsaddons_version.h>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmapCache>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QScreen>
#include <QSharedPointer>
#include <QShowEvent>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <dolphindebug.h>

#include <algorithm>

#if HAVE_X11
#include <KStartupInfo>
#endif

namespace
{
// Used for GeneralSettings::version() to determine whether
// an updated version of Dolphin is running, so as to migrate
// removed/renamed ...etc config entries; increment it in such
// cases
const int CurrentDolphinVersion = 202;
// The maximum number of entries in the back/forward popup menu
const int MaxNumberOfNavigationentries = 12;
// The maximum number of "Go to Tab" shortcuts
const int MaxActivateTabShortcuts = 9;
}

DolphinMainWindow::DolphinMainWindow()
    : KXmlGuiWindow(nullptr)
    , m_newFileMenu(nullptr)
    , m_tabWidget(nullptr)
    , m_activeViewContainer(nullptr)
    , m_actionHandler(nullptr)
    , m_remoteEncoding(nullptr)
    , m_settingsDialog()
    , m_bookmarkHandler(nullptr)
    , m_disabledActionNotifier(nullptr)
    , m_lastHandleUrlOpenJob(nullptr)
    , m_terminalPanel(nullptr)
    , m_placesPanel(nullptr)
    , m_tearDownFromPlacesRequested(false)
    , m_backAction(nullptr)
    , m_forwardAction(nullptr)
    , m_splitViewAction(nullptr)
    , m_splitViewMenuAction(nullptr)
    , m_diskSpaceUsageMenu(nullptr)
    , m_sessionSaveTimer(nullptr)
    , m_sessionSaveWatcher(nullptr)
    , m_sessionSaveScheduled(false)
{
    Q_INIT_RESOURCE(dolphin);

    new MainWindowAdaptor(this);

#ifndef Q_OS_WIN
    setWindowFlags(Qt::WindowContextHelpButtonHint);
#endif
    setComponentName(QStringLiteral("dolphincommander"), QGuiApplication::applicationDisplayName());
    setObjectName(QStringLiteral("Dolphin#"));

    setStateConfigGroup("State");

#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    new KColorSchemeManager(this); // Sets a sensible color scheme which fixes unreadable icons and text on Windows.
#endif

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage, this, &DolphinMainWindow::showErrorMessage);

    KIO::FileUndoManager *undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, &KIO::FileUndoManager::undoAvailable, this, &DolphinMainWindow::slotUndoAvailable);
    connect(undoManager, &KIO::FileUndoManager::undoTextChanged, this, &DolphinMainWindow::slotUndoTextChanged);
    connect(undoManager, &KIO::FileUndoManager::redoAvailable, this, &DolphinMainWindow::slotRedoAvailable);
    connect(undoManager, &KIO::FileUndoManager::redoTextChanged, this, &DolphinMainWindow::slotRedoTextChanged);

    const bool firstRun = (GeneralSettings::version() < 200);
    if (firstRun) {
        GeneralSettings::setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    // Each side of the window has its own tabs and its own path bar.
    m_leftNavigators = new DolphinNavigatorsWidgetAction(this);
    m_rightNavigators = new DolphinNavigatorsWidgetAction(this);
    m_leftTabs = new DolphinTabWidget(m_leftNavigators, this);
    m_leftTabs->setObjectName("tabWidget");
    m_rightTabs = new DolphinTabWidget(m_rightNavigators, this);
    m_rightTabs->setObjectName("rightTabWidget");
    m_tabWidget = m_leftTabs;
    for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
        connect(tabs, &DolphinTabWidget::tabMoveToOtherSideRequested, this, [this, tabs](int index) {
            DolphinTabWidget *other = tabs == m_leftTabs ? m_rightTabs : m_leftTabs;
            // The slot that moves a tab is private to the tab widget; it is a slot, so it can be called by name.
            QMetaObject::invokeMethod(tabs,
                                      "moveTabToOtherSide",
                                      Q_ARG(int, index),
                                      Q_ARG(DolphinTabBar *, static_cast<DolphinTabBar *>(other->tabBar())),
                                      Q_ARG(int, -1));
        });
        connect(tabs, &DolphinTabWidget::tabCopyToOtherSideRequested, this, [this, tabs](int index) {
            DolphinTabWidget *other = tabs == m_leftTabs ? m_rightTabs : m_leftTabs;
            if (DolphinTabPage *page = tabs->tabPageAt(index)) {
                other->openNewActivatedTab(page->activeViewContainer()->url());
            }
        });
        connect(tabs, &DolphinTabWidget::activeViewChanged, this, [this, tabs](DolphinViewContainer *viewContainer) {
            setActiveSide(tabs);
            activeViewChanged(viewContainer);
        });
        connect(tabs, &DolphinTabWidget::paneActivated, this, [this, tabs]() {
            if (tabs != m_tabWidget) {
                activateSide(tabs);
            }
        });
        connect(tabs, &DolphinTabWidget::lastTabCloseRequested, this, &DolphinMainWindow::slotLastTabCloseRequested);
        connect(tabs, &DolphinTabWidget::tabCountChanged, this, &DolphinMainWindow::tabCountChanged);
        connect(tabs, &DolphinTabWidget::currentUrlChanged, this, &DolphinMainWindow::updateWindowTitle);
    }
    m_sidesSplitter = new SidesSplitter(this);
    m_sidesSplitter->addWidget(m_leftTabs);
    m_sidesSplitter->addWidget(m_rightTabs);
    m_sidesSplitter->setStretchFactor(0, 1);
    m_sidesSplitter->setStretchFactor(1, 1);
    // The active side must always be one that can be seen.
    connect(m_sidesSplitter, &SidesSplitter::hiddenSidesChanged, this, [this]() {
        const int activeIndex = m_tabWidget == m_leftTabs ? 0 : 1;
        if (m_sidesSplitter->isSideHidden(activeIndex)) {
            activateSide(inactiveTabWidget());
        }
    });
    // The menu button is added below the places once the menus exist (see setupGUI() below).
    auto *centralContainer = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(centralContainer);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_sidesSplitter);
    setCentralWidget(centralContainer);

    m_actionTextHelper = new SelectionMode::ActionTextHelper(this);
    setupActions();

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), m_actionTextHelper, this);
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectoryTriggered, this, &DolphinMainWindow::createDirectory);
    connect(m_actionHandler, &DolphinViewActionHandler::createFileTriggered, this, &DolphinMainWindow::createFile);
    connect(m_actionHandler, &DolphinViewActionHandler::selectionModeChangeTriggered, this, &DolphinMainWindow::slotSetSelectionMode);
    connect(m_actionHandler, &DolphinViewActionHandler::viewModeChangeRequested, this, [this](DolphinView::Mode mode) {
        if (m_activeViewContainer) {
            m_activeViewContainer->setViewMode(mode);
        }
    });

    QAction *newDirAction = actionCollection()->action(QStringLiteral("create_dir"));
    Q_ASSERT(newDirAction);
    m_newFileMenu->setNewFolderShortcutAction(newDirAction);

    QAction *newFileAction = actionCollection()->action(QStringLiteral("create_file"));
    Q_ASSERT(newFileAction);
    m_newFileMenu->setNewFileShortcutAction(newFileAction);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinMainWindow::urlChanged, m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    m_disabledActionNotifier = new DisabledActionNotifier(this);
    connect(m_disabledActionNotifier, &DisabledActionNotifier::disabledActionTriggered, this, [this](const QAction *, const QString &reason) {
        m_activeViewContainer->showMessage(reason, KMessageWidget::Warning);
    });

    setupDockWidgets();

    // The first time the program is started, a layout and a theme are asked for (not while tests run).
    if (!QStandardPaths::isTestModeEnabled() && !KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("FirstStartDone", false)) {
        QTimer::singleShot(0, this, &DolphinMainWindow::askFirstStartChoices);
    }

#if KIO_VERSION >= QT_VERSION_CHECK(6, 24, 0)
    m_serviceMenuShortcutManager = new ServiceMenuShortcutManager(actionCollection(), this);
#endif
    setupFileItemActions();

    const bool usePhoneUi{KRuntimePlatform::runtimePlatform().contains(QLatin1String("phone"))};
    setupGUI(Save | Create, usePhoneUi ? QStringLiteral("dolphinuiforphones.rc") : QString() /* load the default dolphincommanderui.rc file */);
    stateChanged(QStringLiteral("new_file"));

    // Entries that lead to KDE's websites are not wanted.
    for (const KStandardAction::StandardAction standardAction : {KStandardAction::AboutKDE, KStandardAction::ReportBug, KStandardAction::Donate}) {
        if (QAction *action = actionCollection()->action(KStandardAction::name(standardAction))) {
            action->setVisible(false);
            action->setEnabled(false);
        }
    }

    // The shortcuts and the language are pages of the settings now, and the button for them is
    // called just "Settings". The language action stays enabled, as the language page uses it.
    if (QAction *keyBindings = actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings))) {
        // Its own dialog is not needed, as there is the page of the settings.
        keyBindings->setVisible(false);
        keyBindings->setEnabled(false);
    }
    if (QAction *switchLanguage = actionCollection()->action(KStandardAction::name(KStandardAction::SwitchApplicationLanguage))) {
        switchLanguage->setVisible(false);
    }
    if (QAction *preferences = actionCollection()->action(KStandardAction::name(KStandardAction::Preferences))) {
        preferences->setText(i18nc("@action", "Settings"));
        preferences->setIconText(i18nc("@action:intoolbar", "Settings"));
    }

    // The toolbar of Dolphin is not used, so there is nothing to show or to configure.
    for (const KStandardAction::StandardAction standardAction : {KStandardAction::ShowToolbar, KStandardAction::ConfigureToolbars}) {
        if (QAction *action = actionCollection()->action(KStandardAction::name(standardAction))) {
            action->setVisible(false);
            action->setEnabled(false);
        }
    }
    toolBar()->hide();
    // Saved window settings must not bring the toolbar back.
    connect(toolBar(), &QToolBar::visibilityChanged, toolBar(), [this](bool visible) {
        if (visible) {
            QTimer::singleShot(0, toolBar(), &QWidget::hide);
        }
    });

    // Both sides are always shown, so there is nothing to split, pop out or move to a tab.
    for (const char *name : {"split_view", "split_view_menu", "split_stash", "popout_split_view", "move_to_tabs_split_view"}) {
        if (QAction *action = actionCollection()->action(QLatin1String(name))) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    QClipboard *clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, &DolphinMainWindow::updatePasteAction);

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(GeneralSettings::filterBar());

    if (firstRun) {
        menuBar()->setVisible(false);

        if (usePhoneUi) {
            m_placesDock->hide();
            auto settings = GeneralSettings::self();
            settings->setShowZoomSlider(false); // Zooming can be done with pinch gestures instead and we are short on horizontal space.
            settings->setRenameInline(false); // This works around inline renaming currently not working well with virtual keyboards.
            settings->save(); // Otherwise the RenameInline setting is not picked up for the first time Dolphin is used.
        }
    }

    QAction *showMenuBarAction = actionCollection()->action(KStandardAction::name(KStandardAction::ShowMenubar));
    showMenuBarAction->setChecked(!menuBar()->isHidden()); // workaround for bug #171080
    QTimer::singleShot(0, this, [this, showMenuBarAction]() {
        menuBar()->setVisible(showMenuBarAction->isChecked());
    });

    auto hamburgerMenu = static_cast<KHamburgerMenu *>(actionCollection()->action(KStandardAction::name(KStandardAction::HamburgerMenu)));
    hamburgerMenu->setMenuBar(menuBar());
    hamburgerMenu->setShowMenuBarAction(showMenuBarAction);
    connect(hamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, &DolphinMainWindow::updateHamburgerMenu);
    hamburgerMenu->hideActionsOf(toolBar());

    // Below the places: the menu that holds everything that is not in the settings or the context menu,
    // and the buttons for refresh and search, in front of the button for the settings.
    auto *topBar = new QToolBar(m_placesDock->widget());
    topBar->setMovable(false);
    topBar->setFloatable(false);
    topBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    topBar->setIconSize(QSize(16, 16));
    topBar->setContextMenuPolicy(Qt::NoContextMenu);
    topBar->addAction(hamburgerMenu);
    for (const QString &name : {KStandardAction::name(KStandardAction::Redisplay), QStringLiteral("toggle_search")}) {
        if (QAction *action = actionCollection()->action(name)) {
            topBar->addAction(action);
        }
    }
    // OneCommander's main toolbar, on a row of its own above that one: layout, file automator (the scripts), new window and about.
    auto *mainBar = new QToolBar(m_placesDock->widget());
    mainBar->setObjectName(QStringLiteral("commanderMainToolbar"));
    mainBar->setMovable(false);
    mainBar->setFloatable(false);
    mainBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    mainBar->setIconSize(QSize(16, 16));
    mainBar->setContextMenuPolicy(Qt::NoContextMenu);
    mainBar->layout()->setSpacing(0);
    mainBar->setStyleSheet(QStringLiteral("QToolButton { padding: 2px; }"));
    for (const QString &name : {QStringLiteral("cycle_layout"),
                                QStringLiteral("scripts_menu"),
                                KStandardAction::name(KStandardAction::New),
                                KStandardAction::name(KStandardAction::AboutApp)}) {
        if (QAction *action = actionCollection()->action(name)) {
            mainBar->addAction(action);
        }
    }
    if (auto *placesBox = qobject_cast<QBoxLayout *>(m_placesDock->widget()->layout())) {
        placesBox->insertWidget(qMax(placesBox->count() - 1, 0), mainBar);
    }
    m_placesButtonRow->insertWidget(0, topBar);

    updateAllowedToolbarAreas();
    updateNavigatorsBackground();

    // enable middle-click on back/forward/up to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotToolBarActionMiddleClicked);
    toolBar()->installEventFilter(middleClickEventFilter);

    setupWhatsThis();

    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);

    QTimer::singleShot(0, this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);

    m_serviceMenuConfigWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kservicemenurc"), KConfig::NoGlobals));
    connect(m_serviceMenuConfigWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList & /*names*/) {
        if (group.name() == QLatin1String("Show")) {
            setupFileItemActions();
        }
    });
    connect(GeneralSettings::self(), &GeneralSettings::splitViewChanged, this, &DolphinMainWindow::slotSplitViewChanged);
    connect(GeneralSettings::self(), &GeneralSettings::tabBarChanged, this, &DolphinMainWindow::slotTabBarChanged);
}

DolphinMainWindow::~DolphinMainWindow()
{
    // This fixes a crash on Wayland when closing the mainwindow while another dialog is open.
    disconnect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &DolphinMainWindow::updatePasteAction);

    // This fixes a crash in dolphinmainwindowtest where the connection below fires even though the KMainWindow destructor of this object is already running.
    Q_ASSERT(m_placesDock);
    disconnect(m_placesDock, &DolphinDockWidget::visibilityChanged, this, &DolphinMainWindow::slotPlacesPanelVisibilityChanged);
}

QVector<DolphinViewContainer *> DolphinMainWindow::viewContainers() const
{
    QVector<DolphinViewContainer *> viewContainers;

    for (const DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
        for (int i = 0; i < tabs->count(); ++i) {
            DolphinTabPage *tabPage = tabs->tabPageAt(i);

            viewContainers << tabPage->primaryViewContainer();
            if (tabPage->splitViewEnabled()) {
                viewContainers << tabPage->secondaryViewContainer();
            }
        }
    }
    return viewContainers;
}

void DolphinMainWindow::openDirectories(const QList<QUrl> &dirs, bool splitView)
{
    DolphinTabWidget *const activeSide = m_tabWidget;
    if (splitView && dirs.size() == 2) {
        // Two directories with the split view requested: one for each side.
        m_leftTabs->openDirectories({dirs.at(0)}, false);
        m_rightTabs->openDirectories({dirs.at(1)}, false);
    } else {
        activeSide->openDirectories(dirs, false);
    }
    ensureTabOnEachSide();
    activateSide(activeSide);
}

void DolphinMainWindow::openDirectories(const QStringList &dirs, bool splitView)
{
    openDirectories(QUrl::fromStringList(dirs), splitView);
}

void DolphinMainWindow::openFiles(const QList<QUrl> &files, bool splitView)
{
    Q_UNUSED(splitView)
    DolphinTabWidget *const activeSide = m_tabWidget;
    activeSide->openFiles(files, false);
    ensureTabOnEachSide();
    activateSide(activeSide);
}

void DolphinMainWindow::ensureTabOnEachSide()
{
    // Both sides always show at least one tab. An empty side starts at the folder of the other one.
    if (m_leftTabs->count() == 0 && m_rightTabs->count() > 0) {
        m_leftTabs->openNewActivatedTab(m_rightTabs->currentTabPage()->activeViewContainer()->url());
    } else if (m_rightTabs->count() == 0 && m_leftTabs->count() > 0) {
        m_rightTabs->openNewActivatedTab(m_leftTabs->currentTabPage()->activeViewContainer()->url());
    }
}

DolphinTabWidget *DolphinMainWindow::inactiveTabWidget() const
{
    return m_tabWidget == m_leftTabs ? m_rightTabs : m_leftTabs;
}

DolphinViewContainer *DolphinMainWindow::inactiveViewContainer() const
{
    return inactiveTabWidget()->currentTabPage()->activeViewContainer();
}

DolphinNavigatorsWidgetAction *DolphinMainWindow::activeNavigators() const
{
    return m_tabWidget == m_leftTabs ? m_leftNavigators : m_rightNavigators;
}

void DolphinMainWindow::setActiveSide(DolphinTabWidget *side)
{
    if (side == m_tabWidget) {
        return;
    }
    DolphinViewContainer *oldViewContainer = m_activeViewContainer;
    m_tabWidget = side;
    if (oldViewContainer) {
        oldViewContainer->setActive(false);
    }
}

void DolphinMainWindow::activateSide(DolphinTabWidget *side)
{
    DolphinTabPage *tabPage = side->currentTabPage();
    if (!tabPage) {
        return;
    }
    DolphinViewContainer *viewContainer = tabPage->activeViewContainer();
    setActiveSide(side);
    if (m_activeViewContainer != viewContainer) {
        activeViewChanged(viewContainer);
    }
    viewContainer->setActive(true);
}

void DolphinMainWindow::slotLastTabCloseRequested()
{
    // A side keeps its last tab. Only if no side has another one, the window is closed.
    if (m_leftTabs->count() < 2 && m_rightTabs->count() < 2) {
        close();
    }
}

void DolphinMainWindow::closeCurrentTab()
{
    m_tabWidget->closeTab();
}

void DolphinMainWindow::activateLastTab()
{
    m_tabWidget->activateLastTab();
}

void DolphinMainWindow::activateNextTab()
{
    m_tabWidget->activateNextTab();
}

void DolphinMainWindow::activatePrevTab()
{
    m_tabWidget->activatePrevTab();
}

bool DolphinMainWindow::isInformationPanelEnabled() const
{
#if HAVE_BALOO
    return actionCollection()->action(QStringLiteral("show_information_panel"))->isChecked();
#else
    return false;
#endif
}

bool DolphinMainWindow::isSplitViewEnabledInCurrentTab() const
{
    return m_tabWidget->currentTabPage()->splitViewEnabled();
}

void DolphinMainWindow::openFiles(const QStringList &files, bool splitView)
{
    openFiles(QUrl::fromStringList(files), splitView);
}

void DolphinMainWindow::activateWindow(const QString &activationToken)
{
    window()->setAttribute(Qt::WA_NativeWindow, true);

    if (KWindowSystem::isPlatformWayland()) {
        KWindowSystem::setCurrentXdgActivationToken(activationToken);
    } else if (KWindowSystem::isPlatformX11()) {
#if HAVE_X11
        KStartupInfo::setNewStartupId(window()->windowHandle(), activationToken.toUtf8());
#endif
    }

    KWindowSystem::activateWindow(window()->windowHandle());
}

bool DolphinMainWindow::isActiveWindow()
{
    return window()->isActiveWindow();
}

void DolphinMainWindow::pasteIntoFolder()
{
    m_activeViewContainer->view()->pasteIntoFolder();
}

void DolphinMainWindow::changeUrl(const QUrl &url)
{
    if (!KProtocolManager::supportsListing(url)) {
        // The URL navigator only checks for validity, not
        // if the URL can be listed. An error message is
        // shown due to DolphinViewContainer::restoreView().
        return;
    }

    m_activeViewContainer->setUrl(url);
    updateFileAndEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();
    m_diskSpaceUsageMenu->setUrl(url);

    // will signal used urls to activities manager, too
    m_recentFiles->addUrl(url, QString(), "inode/directory");

    Q_EMIT urlChanged(url);
}

void DolphinMainWindow::slotTerminalDirectoryChanged(const QUrl &url)
{
    if (m_tearDownFromPlacesRequested && url == QUrl::fromLocalFile(QDir::homePath())) {
        m_placesPanel->proceedWithTearDown();
        m_tearDownFromPlacesRequested = false;
    }

    m_activeViewContainer->setGrabFocusOnUrlChange(false);
    changeUrl(url);
    m_activeViewContainer->setGrabFocusOnUrlChange(true);
}

void DolphinMainWindow::slotEditableStateChanged(bool editable)
{
    KToggleAction *editableLocationAction = static_cast<KToggleAction *>(actionCollection()->action(QStringLiteral("editable_location")));
    editableLocationAction->setChecked(editable);
}

void DolphinMainWindow::slotSelectionChanged(const KFileItemList &selection)
{
    updateFileAndEditActions();

    if (m_fileItemActions) {
        if (selection.count() > 0) {
            m_fileItemActions->setItemListProperties(KFileItemListProperties(selection));
        } else {
            m_fileItemActions->setItemListProperties(KFileItemListProperties(KFileItemList() << m_activeViewContainer->rootItem()));
        }
    }

    const int selectedUrlsCount = m_tabWidget->currentTabPage()->selectedItemsCount();

    QAction *compareFilesAction = actionCollection()->action(QStringLiteral("compare_files"));
    if (selectedUrlsCount == 2) {
        compareFilesAction->setEnabled(isKompareInstalled());
    } else {
        compareFilesAction->setEnabled(false);
    }

    Q_EMIT selectionChanged(selection);
}

void DolphinMainWindow::updateHistory()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex();

    QAction *backAction = actionCollection()->action(KStandardAction::name(KStandardAction::Back));
    if (backAction) {
        backAction->setToolTip(i18nc("@info", "Go back"));
        backAction->setWhatsThis(i18nc("@info:whatsthis go back", "Return to the previously viewed folder."));
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction *forwardAction = actionCollection()->action(KStandardAction::name(KStandardAction::Forward));
    if (forwardAction) {
        forwardAction->setToolTip(i18nc("@info", "Go forward"));
        forwardAction->setWhatsThis(xi18nc("@info:whatsthis go forward", "This undoes a <interface>Go|Back</interface> action."));
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    Dolphin::openNewWindow({m_activeViewContainer->url()}, this);
}

void DolphinMainWindow::openNewActivatedTab()
{
    // keep browsers compatibility, new tab is always after last one
    auto openNewTabAfterLastTabConfigured = GeneralSettings::openNewTabAfterLastTab();
    GeneralSettings::setOpenNewTabAfterLastTab(true);
    m_tabWidget->openNewActivatedTab();
    GeneralSettings::setOpenNewTabAfterLastTab(openNewTabAfterLastTabConfigured);
}

void DolphinMainWindow::addToPlaces()
{
    QUrl url;
    QString name;

    // If nothing is selected, act on the current dir
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        url = m_activeViewContainer->url();
        name = m_activeViewContainer->placesText();
    } else {
        const auto dirToAdd = m_activeViewContainer->view()->selectedItems().first();
        url = dirToAdd.url();
        name = dirToAdd.name();
    }
    if (url.isValid()) {
        QString icon;
        if (isSearchUrl(url)) {
            icon = QStringLiteral("folder-saved-search-symbolic");
        } else {
            icon = KIO::iconNameForUrl(url);
        }
        DolphinPlacesModelSingleton::instance().placesModel()->addPlace(name, url, icon);
    }
}

DolphinTabPage *DolphinMainWindow::openNewTab(const QUrl &url)
{
    return m_tabWidget->openNewTab(url, QUrl());
}

void DolphinMainWindow::openNewTabAndActivate(const QUrl &url)
{
    m_tabWidget->openNewActivatedTab(url, QUrl());
}

void DolphinMainWindow::openNewWindow(const QUrl &url)
{
    Dolphin::openNewWindow({url}, this);
}

void DolphinMainWindow::slotSplitViewChanged()
{
    // The two sides are always shown, so the setting has no effect.
    updateSplitActions();
}

void DolphinMainWindow::slotTabBarChanged()
{
    for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
        tabs->setTabBarAutoHide(false);
        tabs->tabBar()->setTabsClosable(GeneralSettings::showCloseButtonOnTabs());
    }
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList &list = m_activeViewContainer->view()->selectedItems();
    bool tabCreated = false;

    for (const KFileItem &item : list) {
        const QUrl &url = DolphinView::openItemAsFolderUrl(item);
        if (!url.isEmpty()) {
            openNewTab(url);
            tabCreated = true;
        }
    }

    // if no new tab has been created from the selection
    // open the current directory in a new tab
    if (!tabCreated) {
        openNewTab(m_activeViewContainer->url());
    }
}

void DolphinMainWindow::openInNewWindow()
{
    QUrl newWindowUrl;

    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        newWindowUrl = m_activeViewContainer->url();
    } else if (list.count() == 1) {
        const KFileItem &item = list.first();
        newWindowUrl = DolphinView::openItemAsFolderUrl(item);
    }

    if (!newWindowUrl.isEmpty()) {
        Dolphin::openNewWindow({newWindowUrl}, this);
    }
}

void DolphinMainWindow::openInSplitView(const QUrl &url)
{
    QUrl newSplitViewUrl = url;

    if (newSplitViewUrl.isEmpty()) {
        const KFileItemList list = m_activeViewContainer->view()->selectedItems();
        if (list.count() == 1) {
            const KFileItem &item = list.first();
            newSplitViewUrl = DolphinView::openItemAsFolderUrl(item);
        }
    }

    if (newSplitViewUrl.isEmpty()) {
        return;
    }

    // Open the folder in the other pane and move to it.
    m_sidesSplitter->showBothSides();
    DolphinTabWidget *other = inactiveTabWidget();
    other->currentTabPage()->activeViewContainer()->setUrl(newSplitViewUrl);
    activateSide(other);
}

void DolphinMainWindow::showTarget()
{
    const KFileItem link = m_activeViewContainer->view()->selectedItems().at(0);
    const QUrl destinationUrl = link.url().resolved(QUrl(link.linkDest()));

    auto job = KIO::stat(destinationUrl, KIO::StatJob::SourceSide, KIO::StatNoDetails);

    connect(job, &KJob::finished, this, [this, destinationUrl](KJob *job) {
        KIO::StatJob *statJob = static_cast<KIO::StatJob *>(job);

        if (statJob->error()) {
            m_activeViewContainer->showMessage(job->errorString(), KMessageWidget::Error);
        } else {
            KIO::highlightInFileManager({destinationUrl});
        }
    });
}

bool DolphinMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space && m_activeViewContainer->view()->handleSpaceAsNormalKey()) {
            event->accept();
            return true;
        }
    }

    return KXmlGuiWindow::event(event);
}

void DolphinMainWindow::showEvent(QShowEvent *event)
{
    KXmlGuiWindow::showEvent(event);

    if (!event->spontaneous() && m_activeViewContainer) {
        m_activeViewContainer->view()->setFocus();
    }
}

static bool s_keepRunning = false;
static bool s_pretendTray = false;

void DolphinMainWindow::pretendThereIsATray(bool pretend)
{
    s_pretendTray = pretend;
}

bool DolphinMainWindow::isHiddenToTray() const
{
    return m_hiddenToTray;
}

void DolphinMainWindow::closeEvent(QCloseEvent *event)
{
    // Find out if Dolphin is closed directly by the user or
    // by the session manager because the session is closed
    bool closedByUser = true;
    if (qApp->isSavingSession()) {
        closedByUser = false;
    }

    // With "keep running when closed" the window only hides; Quit really closes it.
    if (s_keepRunning && closedByUser && !m_quitting && (s_pretendTray || QSystemTrayIcon::isSystemTrayAvailable())) {
        event->ignore();
        hideToTray();
        return;
    }
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    m_hiddenToTray = false;

    if (m_leftTabs->count() + m_rightTabs->count() > 2 && GeneralSettings::confirmClosingMultipleTabs() && !GeneralSettings::rememberOpenedTabs()
        && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // QDialogButtonBox::Yes    -> Quit
        // QDialogButtonBox::No     -> Close only the current tab
        // QDialogButtonBox::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes),
                         KGuiItem(i18nc("@action:button 'Quit Dolphin' button", "&Quit %1", QGuiApplication::applicationDisplayName()),
                                  QIcon::fromTheme(QStringLiteral("application-exit"))));
        KGuiItem::assign(buttons->button(QDialogButtonBox::No), KGuiItem(i18n("C&lose Current Tab"), QIcon::fromTheme(QStringLiteral("tab-close"))));
        KGuiItem::assign(buttons->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());
        buttons->button(QDialogButtonBox::Yes)->setDefault(true);

        bool doNotAskAgainCheckboxResult = false;

        const auto result = KMessageBox::createKMessageBox(dialog,
                                                           buttons,
                                                           QMessageBox::Warning,
                                                           i18n("You have multiple tabs open in this window, are you sure you want to quit?"),
                                                           QStringList(),
                                                           i18n("Do not ask again"),
                                                           &doNotAskAgainCheckboxResult,
                                                           KMessageBox::Notify);

        if (doNotAskAgainCheckboxResult) {
            GeneralSettings::setConfirmClosingMultipleTabs(false);
        }

        switch (result) {
        case QDialogButtonBox::Yes:
            // Quit
            break;
        case QDialogButtonBox::No:
            // Close only the current tab
            m_tabWidget->closeTab();
            Q_FALLTHROUGH();
        default:
            event->ignore();
            return;
        }
    }

    if (m_terminalPanel && m_terminalPanel->hasProgramRunning() && GeneralSettings::confirmClosingTerminalRunningProgram() && closedByUser) {
        // Ask if the user really wants to quit Dolphin with a program that is still running in the Terminal panel
        // Open a confirmation dialog with 3 buttons:
        // QDialogButtonBox::Yes    -> Quit
        // QDialogButtonBox::No     -> Show Terminal Panel
        // QDialogButtonBox::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        auto standardButtons = QDialogButtonBox::Yes | QDialogButtonBox::Cancel;
        if (!m_terminalPanel->isVisible()) {
            standardButtons |= QDialogButtonBox::No;
        }
        QDialogButtonBox *buttons = new QDialogButtonBox(standardButtons);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes), KStandardGuiItem::quit());
        if (!m_terminalPanel->isVisible()) {
            KGuiItem::assign(buttons->button(QDialogButtonBox::No), KGuiItem(i18n("Show &Terminal Panel"), QIcon::fromTheme(QStringLiteral("dialog-scripts"))));
        }
        KGuiItem::assign(buttons->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());

        bool doNotAskAgainCheckboxResult = false;

        const auto result = KMessageBox::createKMessageBox(
            dialog,
            buttons,
            QMessageBox::Warning,
            i18n("The program '%1' is still running in the Terminal panel. Are you sure you want to quit?", m_terminalPanel->runningProgramName()),
            QStringList(),
            i18n("Do not ask again"),
            &doNotAskAgainCheckboxResult,
            KMessageBox::Notify | KMessageBox::Dangerous);

        if (doNotAskAgainCheckboxResult) {
            GeneralSettings::setConfirmClosingTerminalRunningProgram(false);
        }

        switch (result) {
        case QDialogButtonBox::Yes:
            // Quit
            break;
        case QDialogButtonBox::No:
            actionCollection()->action("show_terminal_panel")->trigger();
            // Do not quit, ignore quit event
            Q_FALLTHROUGH();
        default:
            event->ignore();
            return;
        }
    }

    if (m_sessionSaveTimer && (m_sessionSaveTimer->isActive() || m_sessionSaveWatcher->isRunning())) {
        const bool sessionSaveTimerActive = m_sessionSaveTimer->isActive();

        m_sessionSaveTimer->stop();
        m_sessionSaveWatcher->disconnect();
        m_sessionSaveWatcher->waitForFinished();

        if (sessionSaveTimerActive || m_sessionSaveScheduled) {
            slotSaveSession();
        }
    }

    GeneralSettings::setVersion(CurrentDolphinVersion);
    GeneralSettings::self()->save();

    KXmlGuiWindow::closeEvent(event);
}

void DolphinMainWindow::slotSaveSession()
{
    m_sessionSaveScheduled = false;

    if (m_sessionSaveWatcher->isRunning()) {
        // The previous session is still being saved - schedule another save.
        m_sessionSaveWatcher->disconnect();
        connect(m_sessionSaveWatcher, &QFutureWatcher<void>::finished, this, &DolphinMainWindow::slotSaveSession, Qt::SingleShotConnection);
        m_sessionSaveScheduled = true;
    } else if (!m_sessionSaveTimer->isActive()) {
        // No point in saving the session if the timer is running (since it will save the session again when it times out).
        KConfigGui::setSessionConfig(QStringLiteral("dolphincommander"), QStringLiteral("dolphincommander"));
        KConfig *config = KConfigGui::sessionConfig();
        saveGlobalProperties(config);
        savePropertiesInternal(config, 1);
        KConfigGroup group = config->group(QStringLiteral("Number"));
        group.writeEntry("NumberOfWindows", 1); // Makes session restore aware that there is a window to restore.

        // Copy the config in the main thread so sync() can safely run in the worker.
        QSharedPointer<KConfig> configCopy(config->copyTo(config->name()));
        auto future = QtConcurrent::run([configCopy]() {
            configCopy->sync();
        });
        m_sessionSaveWatcher->setFuture(future);
    }
}

void DolphinMainWindow::slotClickViewBackground()
{
    KUrlNavigator *navigator = activeNavigators()->primaryUrlNavigator();

    if (navigator->isUrlEditable()) {
        navigator->setUrlEditable(false);
    }
}

void DolphinMainWindow::setSessionAutoSaveEnabled(bool enable)
{
    if (enable) {
        if (!m_sessionSaveTimer) {
            m_sessionSaveTimer = new QTimer(this);
            m_sessionSaveWatcher = new QFutureWatcher<void>(this);
            m_sessionSaveTimer->setSingleShot(true);
            m_sessionSaveTimer->setInterval(22000);

            connect(m_sessionSaveTimer, &QTimer::timeout, this, &DolphinMainWindow::slotSaveSession);
        }

        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            connect(tabs, &DolphinTabWidget::urlChanged, m_sessionSaveTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
            connect(tabs, &DolphinTabWidget::tabCountChanged, m_sessionSaveTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
            connect(tabs, &DolphinTabWidget::activeViewChanged, m_sessionSaveTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
            connect(tabs, &DolphinTabWidget::tabPinStateChanged, m_sessionSaveTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
        }
        connect(m_sidesSplitter, &SidesSplitter::hiddenSidesChanged, m_sessionSaveTimer, qOverload<>(&QTimer::start), Qt::UniqueConnection);
    } else if (m_sessionSaveTimer) {
        m_sessionSaveTimer->stop();
        m_sessionSaveWatcher->disconnect();
        m_sessionSaveScheduled = false;

        m_sessionSaveWatcher->waitForFinished();

        m_sessionSaveTimer->deleteLater();
        m_sessionSaveWatcher->deleteLater();
        m_sessionSaveTimer = nullptr;
        m_sessionSaveWatcher = nullptr;
    }
}

void DolphinMainWindow::saveProperties(KConfigGroup &group)
{
    KConfigGroup leftGroup = group.group(QStringLiteral("Left Side"));
    m_leftTabs->saveProperties(leftGroup);
    KConfigGroup rightGroup = group.group(QStringLiteral("Right Side"));
    m_rightTabs->saveProperties(rightGroup);
    group.writeEntry("Active Side", m_tabWidget == m_rightTabs ? 1 : 0);
    // 0 or 1 if the left or the right side is hidden, -1 if both are shown.
    group.writeEntry("Hidden Side", m_sidesSplitter->isSideHidden(0) ? 0 : (m_sidesSplitter->isSideHidden(1) ? 1 : -1));
}

void DolphinMainWindow::readProperties(const KConfigGroup &group)
{
    m_leftTabs->readProperties(group.group(QStringLiteral("Left Side")));
    m_rightTabs->readProperties(group.group(QStringLiteral("Right Side")));
    ensureTabOnEachSide();
    activateSide(group.readEntry("Active Side", 0) == 1 ? m_rightTabs : m_leftTabs);

    // The window has no size yet, so hiding a side has to wait until it is shown.
    const int hiddenSide = group.readEntry("Hidden Side", -1);
    if (hiddenSide == 0 || hiddenSide == 1) {
        QTimer::singleShot(0, this, [this, hiddenSide]() {
            m_sidesSplitter->moveDivider(hiddenSide == 0 ? -1 : 1);
        });
    }
}

void DolphinMainWindow::updateNewMenu()
{
    m_newFileMenu->checkUpToDate();
    m_newFileMenu->setWorkingDirectory(activeViewContainer()->url());
}

void DolphinMainWindow::createDirectory(const QUrl &parent)
{
    // When creating directory, namejob is being run. In network folders,
    // this job can take long time, so instead of starting multiple namejobs,
    // just check if we are already running one. This prevents opening multiple
    // dialogs. BUG:481401
    if (!m_newFileMenu->isCreateDirectoryRunning()) {
        m_newFileMenu->setWorkingDirectory(parent);
        m_newFileMenu->createDirectory();
    }
}

void DolphinMainWindow::createFile()
{
    // Use the same logic as in createDirectory()
    if (!m_newFileMenu->isCreateFileRunning()) {
        m_newFileMenu->setWorkingDirectory(activeViewContainer()->url());
        m_newFileMenu->createFile();
    }
}

bool DolphinMainWindow::keepsRunningWhenClosed()
{
    return s_keepRunning;
}

void DolphinMainWindow::setKeepRunningWhenClosed(bool enabled)
{
    s_keepRunning = enabled;
}

void DolphinMainWindow::hideToTray()
{
    if (!m_trayIcon) {
        m_trayIcon = new QSystemTrayIcon(windowIcon(), this);
        m_trayIcon->setToolTip(QGuiApplication::applicationDisplayName());
        auto *menu = new QMenu(this);
        menu->addAction(QIcon::fromTheme(QStringLiteral("window-restore")), i18nc("@action:inmenu", "Show"), this, &DolphinMainWindow::bringBack);
        menu->addAction(QIcon::fromTheme(QStringLiteral("application-exit")), i18nc("@action:inmenu", "Quit"), this, [this]() {
            m_quitting = true;
            close();
        });
        m_trayIcon->setContextMenu(menu);
        connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger) {
                m_hiddenToTray = false;
                showNormal();
                raise();
                QWidget::activateWindow();
            }
        });
    }
    m_trayIcon->show();
    m_hiddenToTray = true;
    hide();
}

void DolphinMainWindow::dockToBottomHalf()
{
    // OneCommander's Ctrl+`: the window comes to the screen it is on, as wide as the screen and in its lower half; once more, it is minimized.
    if (m_docked && isActiveWindow() && !isMinimized()) {
        m_docked = false;
        showMinimized();
        return;
    }
    m_hiddenToTray = false;
    QScreen *screen = windowHandle() && windowHandle()->screen() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }
    showNormal();
    const QRect area = screen->availableGeometry();
    const QMargins frame(frameGeometry().left() - geometry().left(),
                         frameGeometry().top() - geometry().top(),
                         frameGeometry().right() - geometry().right(),
                         frameGeometry().bottom() - geometry().bottom());
    setGeometry(area.left() - frame.left(),
                area.top() + area.height() / 2 - frame.top(),
                area.width() + frame.left() - frame.right(),
                area.height() / 2 + frame.top() - frame.bottom());
    m_docked = true;
    raise();
    QWidget::activateWindow();
}

void DolphinMainWindow::bringBack()
{
    m_hiddenToTray = false;
    showNormal();
    raise();
    QWidget::activateWindow();
}

void DolphinMainWindow::centerOnMainScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return;
    }
    if (isMaximized() || isFullScreen()) {
        showNormal();
    }
    const QRect area = screen->availableGeometry();
    QRect frame = frameGeometry();
    // A window larger than the screen is made to fit it.
    const QSize size = QSize(qMin(width(), area.width() - (frame.width() - width())), qMin(height(), area.height() - (frame.height() - height())));
    resize(size);
    frame = frameGeometry();
    move(area.center() - QPoint(frame.width() / 2, frame.height() / 2));
}

void DolphinMainWindow::reloadTheme()
{
    const KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    applyTextAndIcons(group.readEntry("FontPointSize", 0), group.readEntry("IconTheme", QString()));
    applyColorTheme(group.readEntry("ColorTheme", QStringLiteral("system")));
    applyStyleSheet(group.readEntry("StyleSheet", QString()));
}

void DolphinMainWindow::quit()
{
    m_quitting = true;
    close();
}

void DolphinMainWindow::showErrorMessage(const QString &message)
{
    m_activeViewContainer->showMessage(message, KMessageWidget::Error);
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction *undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction) {
        undoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString &text)
{
    QAction *undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::undo()
{
    KIO::FileUndoManager::self()->uiInterface()->setParentWidget(this);
    KIO::FileUndoManager::self()->undo();
}

void DolphinMainWindow::slotRedoAvailable(bool available)
{
    QAction *redoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Redo));
    if (redoAction) {
        redoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotRedoTextChanged(const QString &text)
{
    QAction *redoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Redo));
    if (redoAction) {
        redoAction->setText(text);
    }
}

void DolphinMainWindow::redo()
{
    KIO::FileUndoManager::self()->uiInterface()->setParentWidget(this);
    KIO::FileUndoManager::self()->redo();
}

void DolphinMainWindow::cut()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CutContents);
    } else {
        m_activeViewContainer->view()->cutSelectedItemsToClipboard();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::copy()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CopyContents);
    } else {
        m_activeViewContainer->view()->copySelectedItemsToClipboard();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::paste()
{
    m_activeViewContainer->view()->paste();
}

void DolphinMainWindow::find()
{
    m_activeViewContainer->setSearchBarVisible(true);
    m_activeViewContainer->setFocusToSearchBar();
}

void DolphinMainWindow::updateSearchAction()
{
    QAction *toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
    toggleSearchAction->setChecked(m_activeViewContainer->isSearchBarVisible());
}

void DolphinMainWindow::updatePasteAction()
{
    QAction *pasteAction = actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    QPair<bool, QString> pasteInfo = m_activeViewContainer->view()->pasteInfo();
    pasteAction->setEnabled(pasteInfo.first);
    m_disabledActionNotifier->setDisabledReason(pasteAction,
                                                m_activeViewContainer->rootItem().isWritable()
                                                    ? i18nc("@info", "Cannot paste: The clipboard is empty.")
                                                    : i18nc("@info", "Cannot paste: You do not have permission to write into this folder."));
    pasteAction->setText(pasteInfo.second);
}

void DolphinMainWindow::slotDirectoryLoadingCompleted()
{
    updatePasteAction();
}

void DolphinMainWindow::slotToolBarActionMiddleClicked(QAction *action)
{
    if (action == actionCollection()->action(KStandardAction::name(KStandardAction::Back))) {
        goBackInNewTab();
    } else if (action == actionCollection()->action(KStandardAction::name(KStandardAction::Forward))) {
        goForwardInNewTab();
    } else if (action == actionCollection()->action(QStringLiteral("go_up"))) {
        goUpInNewTab();
    } else if (action == actionCollection()->action(QStringLiteral("go_home"))) {
        goHomeInNewTab();
    }
}

QAction *DolphinMainWindow::urlNavigatorHistoryAction(const KUrlNavigator *urlNavigator, int historyIndex, QObject *parent)
{
    const QUrl url = urlNavigator->locationUrl(historyIndex);

    QString text;

    if (isSearchUrl(url)) {
        text = Search::DolphinQuery(url, QUrl{}).title();
    } else if (urlNavigator->showFullPath()) {
        text = url.toDisplayString(QUrl::PreferLocalFile);
    } else {
        const KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();

        const QModelIndex closestIdx = placesModel->closestItem(url);
        if (closestIdx.isValid()) {
            const QUrl placeUrl = placesModel->url(closestIdx);

            text = placesModel->text(closestIdx);

            QString pathInsidePlace = url.path().mid(placeUrl.path().length());

            if (!pathInsidePlace.isEmpty() && !pathInsidePlace.startsWith(QLatin1Char('/'))) {
                pathInsidePlace.prepend(QLatin1Char('/'));
            }

            if (pathInsidePlace != QLatin1Char('/')) {
                text.append(pathInsidePlace);
            }
        }
    }

    QAction *action = new QAction(QIcon::fromTheme(KIO::iconNameForUrl(url)), text, parent);
    action->setData(historyIndex);
    return action;
}

void DolphinMainWindow::slotAboutToShowBackPopupMenu()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    int entries = 0;
    QMenu *menu = m_backAction->popupMenu();
    menu->clear();
    for (int i = urlNavigator->historyIndex() + 1; i < urlNavigator->historySize() && entries < MaxNumberOfNavigationentries; ++i, ++entries) {
        QAction *action = urlNavigatorHistoryAction(urlNavigator, i, menu);
        menu->addAction(action);
    }
}

void DolphinMainWindow::slotGoBack(QAction *action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = gotoIndex - urlNavigator->historyIndex(); i > 0; --i) {
        goBack();
    }
}

void DolphinMainWindow::slotBackForwardActionMiddleClicked(QAction *action)
{
    if (action) {
        const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
        openNewTab(urlNavigator->locationUrl(action->data().value<int>()));
    }
}

void DolphinMainWindow::slotAboutToShowForwardPopupMenu()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    int entries = 0;
    QMenu *menu = m_forwardAction->popupMenu();
    menu->clear();
    for (int i = urlNavigator->historyIndex() - 1; i >= 0 && entries < MaxNumberOfNavigationentries; --i, ++entries) {
        QAction *action = urlNavigatorHistoryAction(urlNavigator, i, menu);
        menu->addAction(action);
    }
}

void DolphinMainWindow::slotGoForward(QAction *action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = urlNavigator->historyIndex() - gotoIndex; i > 0; --i) {
        goForward();
    }
}

void DolphinMainWindow::slotSetSelectionMode(bool enabled, SelectionMode::BottomBar::Contents bottomBarContents)
{
    m_activeViewContainer->setSelectionModeEnabled(enabled, actionCollection(), bottomBarContents);
}

void DolphinMainWindow::selectAll()
{
    // if the URL navigator is editable and focused, select the whole
    // URL instead of all items of the view

    KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigator();
    QLineEdit *lineEdit = urlNavigator->editor()->lineEdit();
    const bool selectUrl = urlNavigator->isUrlEditable() && lineEdit->hasFocus();
    if (selectUrl) {
        lineEdit->selectAll();
    } else {
        m_activeViewContainer->view()->selectAll();
    }
}

void DolphinMainWindow::invertSelection()
{
    m_activeViewContainer->view()->invertSelection();
}

void DolphinMainWindow::toggleSplitView()
{
    QUrl newSplitViewUrl;
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.count() == 1) {
        const KFileItem &item = list.first();
        newSplitViewUrl = DolphinView::openItemAsFolderUrl(item);
    }

    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(!tabPage->splitViewEnabled(), WithAnimation, newSplitViewUrl);
    m_tabWidget->updateTabName(m_tabWidget->indexOf(tabPage));
    updateViewActions();
}

void DolphinMainWindow::popoutSplitView()
{
    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    if (!tabPage->splitViewEnabled())
        return;
    using Choice = GeneralSettings::EnumCloseSplitViewChoice;
    switch (GeneralSettings::closeSplitViewChoice()) {
    case Choice::ActiveView:
        openNewWindow(tabPage->activeViewContainer()->url());
        break;
    case Choice::InactiveView:
        openNewWindow(tabPage->inactiveViewContainer()->url());
        break;
    case Choice::RightView:
        openNewWindow(tabPage->secondaryViewContainer()->url());
        break;
    default:
        Q_UNREACHABLE();
    }

    tabPage->setSplitViewEnabled(false, WithAnimation);
    updateSplitActions();
}

void DolphinMainWindow::moveSplitViewToTabs()
{
    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    if (!tabPage->splitViewEnabled())
        return;

    openNewTab(tabPage->activeViewContainer()->url());

    tabPage->setSplitViewEnabled(false, WithAnimation);
    updateSplitActions();
}

void DolphinMainWindow::toggleSplitStash()
{
    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(false, WithAnimation);
    tabPage->setSplitViewEnabled(true, WithAnimation, QUrl("stash:/"));
}

void DolphinMainWindow::copyToInactiveSplitView()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CopyToOtherViewContents);
    } else {
        DolphinView *const inactiveView = inactiveViewContainer()->view();
        inactiveView->copySelectedItems(m_activeViewContainer->view()->selectedItems(), inactiveView->url());
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::moveToInactiveSplitView()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::MoveToOtherViewContents);
    } else {
        DolphinView *const inactiveView = inactiveViewContainer()->view();
        inactiveView->moveSelectedItems(m_activeViewContainer->view()->selectedItems(), inactiveView->url());
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::reloadView()
{
    m_activeViewContainer->reload();
    Q_EMIT urlRefreshed(m_activeViewContainer->url());
}

void DolphinMainWindow::stopLoading()
{
    m_activeViewContainer->view()->stopLoading();
}

void DolphinMainWindow::enableStopAction()
{
    actionCollection()->action(QStringLiteral("stop"))->setEnabled(true);
}

void DolphinMainWindow::disableStopAction()
{
    actionCollection()->action(QStringLiteral("stop"))->setEnabled(false);
}

void DolphinMainWindow::toggleSelectionMode()
{
    const bool checked = !m_activeViewContainer->isSelectionModeEnabled();

    m_activeViewContainer->setSelectionModeEnabled(checked, actionCollection(), SelectionMode::BottomBar::Contents::GeneralContents);
    actionCollection()->action(QStringLiteral("toggle_selection_mode"))->setChecked(checked);
}

void DolphinMainWindow::showFilterBar()
{
    m_activeViewContainer->setFilterBarVisible(true);
}

void DolphinMainWindow::toggleFilterBar()
{
    const bool checked = !m_activeViewContainer->isFilterBarVisible();
    m_activeViewContainer->setFilterBarVisible(checked);

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(checked);
}

void DolphinMainWindow::toggleEditLocation()
{
    QAction *action = actionCollection()->action(QStringLiteral("editable_location"));
    KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->setUrlEditable(action->isChecked());
}

void DolphinMainWindow::replaceLocation()
{
    KUrlNavigator *navigator = m_activeViewContainer->urlNavigator();
    QLineEdit *lineEdit = navigator->editor()->lineEdit();

    // If the text field currently has focus and everything is selected,
    // pressing the keyboard shortcut returns the whole thing to breadcrumb mode
    // and goes back to the view, just like how it was before this action was triggered the first time.
    if (navigator->isUrlEditable() && lineEdit->hasFocus() && lineEdit->selectedText() == lineEdit->text()) {
        navigator->setUrlEditable(false);
        m_activeViewContainer->view()->setFocus();
    } else {
        navigator->setUrlEditable(true);
        navigator->setFocus();
        lineEdit->selectAll();
    }
}

void DolphinMainWindow::togglePanelLockState()
{
    const bool newLockState = !GeneralSettings::lockPanels();
    const auto childrenObjects = children();
    for (QObject *child : childrenObjects) {
        DolphinDockWidget *dock = qobject_cast<DolphinDockWidget *>(child);
        if (dock) {
            dock->setLocked(newLockState);
        }
    }

    DolphinPlacesModelSingleton::instance().placesModel()->setPanelsLocked(newLockState);

    GeneralSettings::setLockPanels(newLockState);
}

void DolphinMainWindow::slotTerminalPanelVisibilityChanged(bool visible)
{
    if (!visible && m_activeViewContainer) {
        m_activeViewContainer->view()->setFocus();
    }
    // Putting focus to the Terminal is not handled here but in TerminalPanel::showEvent().
}

void DolphinMainWindow::slotPlacesPanelVisibilityChanged(bool visible)
{
    if (!visible && m_activeViewContainer) {
        m_activeViewContainer->view()->setFocus();
        return;
    }
    m_placesPanel->setFocus();
}

void DolphinMainWindow::goBack()
{
    DolphinUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    urlNavigator->goBack();

    if (urlNavigator->locationState().isEmpty()) {
        // An empty location state indicates a redirection URL,
        // which must be skipped too
        urlNavigator->goBack();
    }
}

void DolphinMainWindow::goForward()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goForward();
}

void DolphinMainWindow::goUp()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goUp();
}

void DolphinMainWindow::goHome()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goHome();
}

void DolphinMainWindow::goBackInNewTab()
{
    const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex() + 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goForwardInNewTab()
{
    const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex() - 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goUpInNewTab()
{
    const QUrl currentUrl = activeViewContainer()->urlNavigator()->locationUrl();
    openNewTab(KIO::upUrl(currentUrl));
}

void DolphinMainWindow::goHomeInNewTab()
{
    openNewTab(Dolphin::homeUrl());
}

void DolphinMainWindow::compareFiles()
{
    const KFileItemList items = m_tabWidget->currentTabPage()->selectedItems();
    if (items.count() != 2) {
        // The action is disabled in this case, but it could have been triggered
        // via D-Bus, see https://bugs.kde.org/show_bug.cgi?id=325517
        return;
    }

    QUrl urlA = items.at(0).url();
    QUrl urlB = items.at(1).url();

    const QString program = QStringLiteral("kompare");
    const QStringList args({QStringLiteral("-c"), urlA.toDisplayString(QUrl::PreferLocalFile), urlB.toDisplayString(QUrl::PreferLocalFile)});

    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(program, args, this);
    job->setDesktopName(QStringLiteral("org.kde.kompare"));
    job->start();
}

void DolphinMainWindow::toggleShowMenuBar()
{
    QAction *showMenuBarAction = actionCollection()->action(KStandardAction::name(KStandardAction::ShowMenubar));
    menuBar()->setVisible(showMenuBarAction->isChecked());
    KConfigGroup group = KSharedConfig::openConfig()->group(QStringLiteral("MainWindow"));
    group.writeEntry("MenuBar", showMenuBarAction->isChecked() ? "Enabled" : "Disabled");
    group.sync();
}

QPointer<QAction> DolphinMainWindow::preferredSearchTool()
{
    m_searchTools.clear();

    KService::Ptr kfind = KService::serviceByDesktopName(QStringLiteral("org.kde.kfind"));

    if (!kfind) {
        return nullptr;
    }

    auto *action = new QAction(QIcon::fromTheme(kfind->icon()), kfind->name(), this);

    connect(action, &QAction::triggered, this, [this, kfind] {
        auto *job = new KIO::ApplicationLauncherJob(kfind);
        job->setUrls({m_activeViewContainer->url()});
        job->start();
    });

    return action;
}

void DolphinMainWindow::updateOpenPreferredSearchToolAction()
{
    QAction *openPreferredSearchTool = actionCollection()->action(QStringLiteral("open_preferred_search_tool"));
    if (!openPreferredSearchTool) {
        return;
    }
    QPointer<QAction> tool = preferredSearchTool();
    if (tool) {
        openPreferredSearchTool->setVisible(true);
        openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open %1", tool->text()));
        // Only override with the app icon if it is the default, i.e. the user hasn't configured one manually
        // https://bugs.kde.org/show_bug.cgi?id=442815
        if (openPreferredSearchTool->icon().name() == QLatin1String("search")) {
            openPreferredSearchTool->setIcon(tool->icon());
        }
    } else {
        openPreferredSearchTool->setVisible(false);
        // still visible in Shortcuts configuration window
        openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open Preferred Search Tool"));
        openPreferredSearchTool->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    }
}

void DolphinMainWindow::openPreferredSearchTool()
{
    QPointer<QAction> tool = preferredSearchTool();
    if (tool) {
        tool->trigger();
    }
}

bool DolphinMainWindow::runAsAdministrator(const QString &program, const QStringList &arguments)
{
    // pkexec asks for the password itself; the display variables let a graphical program start as root.
    QStringList args{QStringLiteral("env")};
    for (const char *name : {"DISPLAY", "WAYLAND_DISPLAY", "XDG_RUNTIME_DIR", "XAUTHORITY"}) {
        if (qEnvironmentVariableIsSet(name)) {
            args << QString::fromLatin1(name) + QLatin1Char('=') + qEnvironmentVariable(name);
        }
    }
    args << program << arguments;
    return QProcess::startDetached(QStringLiteral("pkexec"), args);
}

void DolphinMainWindow::openTerminal()
{
    openTerminalJob(m_activeViewContainer->url());
}

void DolphinMainWindow::openTerminalHere()
{
    QList<QUrl> urls = {};

    const auto selectedItems = m_activeViewContainer->view()->selectedItems();
    for (const KFileItem &item : selectedItems) {
        QUrl url = item.targetUrl();
        if (item.isFile()) {
            url.setPath(QFileInfo(url.path()).absolutePath());
        }
        if (!urls.contains(url)) {
            urls << url;
        }
    }

    // No items are selected. Open a terminal window for the current location.
    if (urls.count() == 0) {
        openTerminal();
        return;
    }

    if (urls.count() > 5) {
        QString question = i18np("Are you sure you want to open 1 terminal window?", "Are you sure you want to open %1 terminal windows?", urls.count());
        const int answer = KMessageBox::warningContinueCancel(
            this,
            question,
            {},
            KGuiItem(i18ncp("@action:button", "Open %1 Terminal", "Open %1 Terminals", urls.count()), QStringLiteral("utilities-terminal")),
            KStandardGuiItem::cancel(),
            QStringLiteral("ConfirmOpenManyTerminals"));
        if (answer != KMessageBox::PrimaryAction && answer != KMessageBox::Continue) {
            return;
        }
    }

    for (const QUrl &url : std::as_const(urls)) {
        openTerminalJob(url);
    }
}

void DolphinMainWindow::openTerminalJob(const QUrl &url)
{
    if (url.isLocalFile()) {
        auto job = new KTerminalLauncherJob(QString());
        job->setWorkingDirectory(url.toLocalFile());
        job->start();
        return;
    }

    // Not a local file, with protocol Class ":local", try stat'ing
    if (KProtocolInfo::protocolClass(url.scheme()) == QLatin1String(":local")) {
        KIO::StatJob *job = KIO::mostLocalUrl(url);
        KJobWidgets::setWindow(job, this);
        connect(job, &KJob::result, this, [job]() {
            QUrl statUrl;
            if (!job->error()) {
                statUrl = job->mostLocalUrl();
            }

            auto job = new KTerminalLauncherJob(QString());
            job->setWorkingDirectory(statUrl.isLocalFile() ? statUrl.toLocalFile() : QDir::homePath());
            job->start();
        });

        return;
    }

    // Nothing worked, just use $HOME
    auto job = new KTerminalLauncherJob(QString());
    job->setWorkingDirectory(QDir::homePath());
    job->start();
}

void DolphinMainWindow::editSettings()
{
    if (!m_settingsDialog) {
        DolphinViewContainer *container = activeViewContainer();
        container->view()->writeSettings();

        const QUrl url = container->url();
        DolphinSettingsDialog *settingsDialog = new DolphinSettingsDialog(url, this, actionCollection());
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, this, &DolphinMainWindow::refreshViews);
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, &DolphinUrlNavigatorsController::slotReadSettings);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::handleUrl(const QUrl &url)
{
    delete m_lastHandleUrlOpenJob;
    m_lastHandleUrlOpenJob = nullptr;

    m_lastHandleUrlOpenJob = new KIO::OpenUrlJob(url);
    m_lastHandleUrlOpenJob->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    m_lastHandleUrlOpenJob->setShowOpenOrExecuteDialog(true);

    connect(m_lastHandleUrlOpenJob, &KIO::OpenUrlJob::mimeTypeFound, this, [this, url](const QString &mimetype) {
        if (mimetype == QLatin1String("inode/directory")) {
            // If it's a dir, we'll take it from here
            m_lastHandleUrlOpenJob->kill();
            m_lastHandleUrlOpenJob = nullptr;
            activeViewContainer()->setUrl(url);
        }
    });

    connect(m_lastHandleUrlOpenJob, &KIO::OpenUrlJob::result, this, [this]() {
        m_lastHandleUrlOpenJob = nullptr;
    });

    m_lastHandleUrlOpenJob->start();
}

void DolphinMainWindow::slotWriteStateChanged(bool isFolderWritable)
{
    // trash:/ is writable but we don't want to create new items in it.
    // TODO: remove the trash check once https://phabricator.kde.org/T8234 is implemented
    newFileMenu()->setEnabled(isFolderWritable && m_activeViewContainer->url().scheme() != QLatin1String("trash"));
    // When the menu is disabled, actions in it are disabled later in the event loop, and we need to set the disabled reason after that.
    QTimer::singleShot(0, this, [this]() {
        m_disabledActionNotifier->setDisabledReason(actionCollection()->action(QStringLiteral("create_file")),
                                                    i18nc("@info", "Cannot create new file: You do not have permission to create items in this folder."));
        m_disabledActionNotifier->setDisabledReason(actionCollection()->action(QStringLiteral("create_dir")),
                                                    i18nc("@info", "Cannot create new folder: You do not have permission to create items in this folder."));
        m_disabledActionNotifier->setDisabledReason(actionCollection()->action(QStringLiteral("create_subdir")),
                                                    i18nc("@info", "Cannot create new subfolder: Select a single folder you can write to."));
    });
}

void DolphinMainWindow::openContextMenu(const QPoint &pos, const KFileItem &item, const KFileItemList &selectedItems, const QUrl &url)
{
    // Building the menu queries KFileItemAction plugins, which are free to spin the event loop, so
    // replacing m_fileItemActions has to wait until the menu is gone. Otherwise the plugins queried
    // afterwards read the fresh instance's empty item list. See BUG: 519624
    const bool contextMenuWasOpen = m_contextMenuOpen;
    m_contextMenuOpen = true;
    auto resetContextMenuOpen = qScopeGuard([this, contextMenuWasOpen] {
        m_contextMenuOpen = contextMenuWasOpen;
        if (!m_contextMenuOpen && m_fileItemActionsSetupPending) {
            setupFileItemActions();
        }
    });

    QPointer<DolphinContextMenu> contextMenu = new DolphinContextMenu(this, item, selectedItems, url, m_fileItemActions);
    contextMenu->exec(pos);
    delete contextMenu;
}

QMenu *DolphinMainWindow::createPopupMenu()
{
    QMenu *menu = KXmlGuiWindow::createPopupMenu();

    menu->addSeparator();
    menu->addAction(actionCollection()->action(QStringLiteral("lock_panels")));

    return menu;
}

void DolphinMainWindow::updateHamburgerMenu()
{
    KActionCollection *ac = actionCollection();
    auto hamburgerMenu = static_cast<KHamburgerMenu *>(ac->action(KStandardAction::name(KStandardAction::HamburgerMenu)));
    auto menu = hamburgerMenu->menu();
    if (!menu) {
        menu = new QMenu(this);
        hamburgerMenu->setMenu(menu);
        hamburgerMenu->hideActionsOf(ac->action(QStringLiteral("basic_actions"))->menu());
        hamburgerMenu->hideActionsOf(qobject_cast<KToolBarPopupAction *>(ac->action(QStringLiteral("zoom")))->popupMenu());
    } else {
        menu->clear();
    }
    const QList<QAction *> toolbarActions = toolBar()->actions();

    if (!toolBar()->isVisible()) {
        // If neither the menu bar nor the toolbar are visible, these actions should be available.
        menu->addAction(ac->action(KStandardAction::name(KStandardAction::ShowMenubar)));
        menu->addAction(toolBarMenuAction());
        menu->addSeparator();
    }

    // This group of actions (until the next separator) contains all the most basic actions
    // necessary to use Dolphin effectively.
    menu->addAction(ac->action(QStringLiteral("go_back")));
    menu->addAction(ac->action(QStringLiteral("go_forward")));

    menu->addMenu(m_newFileMenu->menu());
    if (!toolBar()->isVisible() || !toolbarActions.contains(ac->action(QStringLiteral("toggle_selection_mode_tool_bar")))) {
        menu->addAction(ac->action(QStringLiteral("toggle_selection_mode")));
    }
    menu->addAction(ac->action(QStringLiteral("basic_actions")));
    menu->addAction(ac->action(KStandardAction::name(KStandardAction::Undo)));
    menu->addAction(ac->action(KStandardAction::name(KStandardAction::Redo)));
    if (!toolBar()->isVisible()
        || (!toolbarActions.contains(ac->action(QStringLiteral("toggle_search")))
            && !toolbarActions.contains(ac->action(QStringLiteral("open_preferred_search_tool"))))) {
        menu->addAction(ac->action(KStandardAction::name(KStandardAction::Find)));
        // This way a search action will only be added if none of the three available
        // search actions is present on the toolbar.
    }
    if (!toolBar()->isVisible() || !toolbarActions.contains(ac->action(QStringLiteral("toggle_filter")))) {
        menu->addAction(ac->action(QStringLiteral("show_filter_bar")));
        // This way a filter action will only be added if none of the two available
        // filter actions is present on the toolbar.
    }
    menu->addSeparator();

    // The second group of actions (up until the next separator) contains actions for opening
    // additional views to interact with the file system.
    menu->addAction(ac->action(QStringLiteral("file_new")));
    menu->addAction(ac->action(QStringLiteral("new_tab")));
    if (ac->action(QStringLiteral("undo_close_tab"))->isEnabled()) {
        menu->addAction(ac->action(QStringLiteral("closed_tabs")));
    }
    menu->addAction(ac->action(QStringLiteral("open_terminal")));
    menu->addSeparator();

    // The third group contains actions to change what one sees in the view
    // and to change the more general UI.
    if (!toolBar()->isVisible()
        || ((!toolbarActions.contains(ac->action(QStringLiteral("icons"))) && !toolbarActions.contains(ac->action(QStringLiteral("compact")))
             && !toolbarActions.contains(ac->action(QStringLiteral("details"))) && !toolbarActions.contains(ac->action(QStringLiteral("view_mode"))))
            && !toolbarActions.contains(ac->action(QStringLiteral("view_settings"))))) {
        menu->addAction(ac->action(QStringLiteral("view_mode")));
    }
    if (!toolBar()->isVisible() || !toolbarActions.contains(ac->action(QStringLiteral("view_settings")))) {
        menu->addAction(ac->action(QStringLiteral("show_hidden_files")));
        menu->addAction(ac->action(QStringLiteral("sort")));
        menu->addAction(ac->action(QStringLiteral("additional_info")));
        if (!GeneralSettings::showStatusBar() || !GeneralSettings::showZoomSlider()) {
            menu->addAction(ac->action(QStringLiteral("zoom")));
        }
    }
    menu->addAction(ac->action(QStringLiteral("panels")));

    // The "Configure" menu is not added to the actionCollection() because there is hardly
    // a good reason for users to put it on their toolbar.
    auto configureMenu = menu->addMenu(QIcon::fromTheme(QStringLiteral("configure")), i18nc("@action:inmenu menu for configure actions", "Configure"));
    configureMenu->addAction(actionCollection()->action(QStringLiteral("window_color_sheme")));
    configureMenu->addSeparator();
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::SwitchApplicationLanguage)));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::KeyBindings)));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Preferences)));
    hamburgerMenu->hideActionsOf(configureMenu);
}

void DolphinMainWindow::slotPlaceActivated(const QUrl &url)
{
    DolphinViewContainer *view = activeViewContainer();
    view->markNextUrlAsPlace(url);

    if (view->url() == url) {
        view->clearFilterBar(); // Fixes bug 259382.

        // We can end up here if the user clicked a device in the Places Panel
        // which had been unmounted earlier, see https://bugs.kde.org/show_bug.cgi?id=161385.
        reloadView();

        m_activeViewContainer->view()->setFocus(); // We always want the focus on the view after activating a place.
    } else {
        view->disableUrlNavigatorSelectionRequests();
        changeUrl(url);
        view->enableUrlNavigatorSelectionRequests();
    }
}

void DolphinMainWindow::closedTabsCountChanged(unsigned int count)
{
    actionCollection()->action(QStringLiteral("undo_close_tab"))->setEnabled(count > 0);
}

void DolphinMainWindow::activeViewChanged(DolphinViewContainer *viewContainer)
{
    DolphinViewContainer *oldViewContainer = m_activeViewContainer;
    Q_ASSERT(viewContainer);

    m_activeViewContainer = viewContainer;
    m_syncPreviousUrl = viewContainer->url();

    if (oldViewContainer) {
        // Disconnect all signals between the old view container (container,
        // view and url navigator) and main window.
        oldViewContainer->disconnect(this);
        oldViewContainer->view()->disconnect(this);
        oldViewContainer->urlNavigatorInternalWithHistory()->disconnect(this);
        m_leftNavigators->primaryUrlNavigator()->disconnect(this);
        m_rightNavigators->primaryUrlNavigator()->disconnect(this);
        // The two connections below have the menu as their sender, so they are removed
        // this way round. Disconnecting the container as a sender left them in place.
        m_diskSpaceUsageMenu->disconnect(oldViewContainer);

        if (oldViewContainer != viewContainer) {
            // except the requestItemInfo so that on hover the information panel can still be updated
            connect(oldViewContainer->view(), &DolphinView::requestItemInfo, this, &DolphinMainWindow::requestItemInfo);
        }

        // Disconnect other slots.
        disconnect(oldViewContainer,
                   &DolphinViewContainer::selectionModeChanged,
                   actionCollection()->action(QStringLiteral("toggle_selection_mode")),
                   &QAction::setChecked);

        disconnect(oldViewContainer->view(), &DolphinView::viewSettingsChanged, this, nullptr);
    }

    connectViewSignals(viewContainer);

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateFileAndEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();
    updateSearchAction();
    connect(m_diskSpaceUsageMenu,
            &DiskSpaceUsageMenu::showMessage,
            viewContainer,
            [viewContainer](const QString &message, KMessageWidget::MessageType messageType) {
                viewContainer->showMessage(message, messageType);
            });
    connect(m_diskSpaceUsageMenu, &DiskSpaceUsageMenu::showInstallationProgress, viewContainer, &DolphinViewContainer::showProgress);

    const QUrl url = viewContainer->url();
    Q_EMIT urlChanged(url);
    Q_EMIT selectionChanged(m_activeViewContainer->view()->selectedItems());
}

void DolphinMainWindow::rememberRecentFolder(const QUrl &url)
{
    if (!url.isValid() || url.scheme() == QLatin1String("search") || url.scheme() == QLatin1String("timeline")) {
        return;
    }
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    QStringList recent = group.readEntry("RecentFolders", QStringList());
    const QString entry = url.adjusted(QUrl::StripTrailingSlash).toString();
    recent.removeAll(entry);
    recent.prepend(entry);
    group.writeEntry("RecentFolders", recent.mid(0, 20));
}

void DolphinMainWindow::showFolderHotlist()
{
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    const KFilePlacesModel *places = DolphinPlacesModelSingleton::instance().placesModel();
    for (int row = 0; row < places->rowCount(); ++row) {
        const QModelIndex index = places->index(row, 0);
        if (places->isHidden(index) || !places->url(index).isValid()) {
            continue;
        }
        const QUrl url = places->url(index);
        QAction *action = menu->addAction(places->icon(index), places->text(index));
        connect(action, &QAction::triggered, this, [this, url]() {
            changeUrl(url);
        });
    }
    const QStringList recent = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("RecentFolders", QStringList());
    if (!recent.isEmpty()) {
        menu->addSection(i18nc("@title:menu", "Recent Folders"));
        for (const QString &entry : recent) {
            const QUrl url(entry);
            QAction *action = menu->addAction(QIcon::fromTheme(QStringLiteral("folder")), url.isLocalFile() ? url.toLocalFile() : url.toDisplayString());
            connect(action, &QAction::triggered, this, [this, url]() {
                changeUrl(url);
            });
        }
    }
    menu->popup(m_activeViewContainer->mapToGlobal(QPoint(24, 24)));
}

void DolphinMainWindow::showChecksum()
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty() || items.first().isDir()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select a file to calculate its checksum."), KMessageWidget::Information);
        return;
    }
    auto *dialog = new ChecksumDialog(items.first(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DolphinMainWindow::selectByPattern(bool select)
{
    bool ok = false;
    const QString pattern = QInputDialog::getText(this,
                                                  select ? i18nc("@title:window", "Select by Pattern") : i18nc("@title:window", "Deselect by Pattern"),
                                                  i18nc("@label:textbox", "Wildcard pattern, for example *.jpg:"),
                                                  QLineEdit::Normal,
                                                  QStringLiteral("*"),
                                                  &ok);
    if (ok && !pattern.isEmpty()) {
        m_activeViewContainer->view()->selectItems(QRegularExpression(QRegularExpression::wildcardToRegularExpression(pattern)), select);
    }
}

void DolphinMainWindow::mirrorToOtherSide()
{
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    DolphinViewContainer *other = otherPage ? otherPage->activeViewContainer() : nullptr;
    if (!other || !m_activeViewContainer || other == m_activeViewContainer) {
        return;
    }
    m_sidesSplitter->showBothSides();
    other->setUrl(m_activeViewContainer->url());
}

void DolphinMainWindow::swapSides()
{
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    DolphinViewContainer *other = otherPage ? otherPage->activeViewContainer() : nullptr;
    // Both are kept in variables: changing a folder can change which side is the active one.
    DolphinViewContainer *mine = m_activeViewContainer;
    if (!other || !mine || other == mine) {
        return;
    }
    m_sidesSplitter->showBothSides();
    const QUrl mineUrl = mine->url();
    const QUrl otherUrl = other->url();
    mine->setUrl(otherUrl);
    other->setUrl(mineUrl);
    // Going to a folder on the other side moved the focus there: it stays where it was.
    mine->setActive(true);
}

void DolphinMainWindow::setColorLabel(const QString &color)
{
    static const QStringList colors =
        {QStringLiteral("red"), QStringLiteral("orange"), QStringLiteral("yellow"), QStringLiteral("green"), QStringLiteral("blue"), QStringLiteral("purple")};
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select the items to label."), KMessageWidget::Information);
        return;
    }
    for (const KFileItem &item : items) {
        if (!item.isLocalFile()) {
            continue;
        }
        KFileMetaData::UserMetaData data(item.localPath());
        QStringList tags = data.tags();
        tags.removeIf([](const QString &tag) {
            return colors.contains(tag);
        });
        if (!color.isEmpty()) {
            tags.append(color);
        }
        data.setTags(tags);
#ifdef Q_OS_LINUX
        if (tags.isEmpty()) {
            // setTags() with no tags leaves the old ones in place, so the attribute is removed.
            removexattr(QFile::encodeName(item.localPath()).constData(), "user.xdg.tags");
        }
#endif
    }
    // Reloading shows the new color, and would forget the selection.
    m_activeViewContainer->view()->reload();
    m_activeViewContainer->view()->markUrlsAsSelected(items.urlList());
}

void DolphinMainWindow::syncToOtherSide()
{
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    DolphinViewContainer *other = otherPage ? otherPage->activeViewContainer() : nullptr;
    if (!other || !m_activeViewContainer || other == m_activeViewContainer) {
        return;
    }
    QHash<QString, KFileItem> theirs;
    for (const KFileItem &item : other->view()->items()) {
        theirs.insert(item.name(), item);
    }
    QList<QUrl> toCopy;
    for (const KFileItem &item : m_activeViewContainer->view()->items()) {
        const auto match = theirs.constFind(item.name());
        const bool missing = match == theirs.cend();
        const bool newer = !missing && !item.isDir() && !match->isDir() && item.time(KFileItem::ModificationTime) > match->time(KFileItem::ModificationTime);
        if (missing || newer) {
            toCopy.append(item.url());
        }
    }
    if (toCopy.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info:status", "The other side is up to date."), KMessageWidget::Information);
        return;
    }
    // What is copied is missing or older there: it replaces it without asking.
    auto *job = KIO::copy(toCopy, other->url(), KIO::Overwrite);
    JobQueue::add(job);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    KIO::FileUndoManager::self()->recordCopyJob(job);
    m_activeViewContainer->showMessage(i18ncp("@info:status", "Copying %1 item to the other side.", "Copying %1 items to the other side.", toCopy.size()),
                                       KMessageWidget::Information);
}

void DolphinMainWindow::showFolderSizes()
{
    KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        items = m_activeViewContainer->view()->items();
    }
    QStringList folders;
    for (const KFileItem &item : std::as_const(items)) {
        if (item.isDir() && item.isLocalFile()) {
            folders << item.localPath();
        }
    }
    if (folders.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "There are no folders on this computer to measure."), KMessageWidget::Information);
        return;
    }
    auto *dialog = new FolderSizesDialog(folders, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DolphinMainWindow::convertImages()
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select the images to convert."), KMessageWidget::Information);
        return;
    }
    bool ok = false;
    const QString format = QInputDialog::getItem(this,
                                                 i18nc("@title:window", "Convert Images"),
                                                 i18nc("@label:listbox", "Save as:"),
                                                 {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("webp"), QStringLiteral("bmp")},
                                                 0,
                                                 false,
                                                 &ok);
    if (!ok) {
        return;
    }
    int done = 0;
    for (const KFileItem &item : items) {
        if (item.isLocalFile() && !CommanderTools::convertImage(item.localPath(), format).isEmpty()) {
            ++done;
        }
    }
    m_activeViewContainer->showMessage(i18ncp("@info:status", "%1 image converted.", "%1 images converted.", done), KMessageWidget::Information);
}

void DolphinMainWindow::runWithArguments()
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.size() != 1 || !items.first().isLocalFile()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select one program or script on this computer to run."), KMessageWidget::Information);
        return;
    }
    bool ok = false;
    const QString text =
        QInputDialog::getText(this, i18nc("@title:window", "Run with Arguments"), i18nc("@label:textbox", "Arguments:"), QLineEdit::Normal, QString(), &ok);
    if (ok) {
        QProcess::startDetached(items.first().localPath(), QProcess::splitCommand(text), m_activeViewContainer->url().toLocalFile());
    }
}

void DolphinMainWindow::openWithDialog()
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        return;
    }
    auto *job = new KIO::ApplicationLauncherJob(KService::Ptr(), this);
    job->setUrls(items.urlList());
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    job->start();
}

void DolphinMainWindow::newToDo()
{
    const QUrl folder = m_activeViewContainer->url();
    if (!folder.isLocalFile()) {
        m_activeViewContainer->showMessage(i18nc("@info", "To-dos can only be made in folders on this computer."), KMessageWidget::Information);
        return;
    }
    bool ok = false;
    const QString text =
        QInputDialog::getText(this, i18nc("@title:window", "New To-Do"), i18nc("@label:textbox", "What is to be done?"), QLineEdit::Normal, QString(), &ok);
    if (!ok || text.trimmed().isEmpty()) {
        return;
    }
    QString name = text.trimmed();
    name.replace(QRegularExpression(QStringLiteral("[/\\\\:*?\"<>|]")), QStringLiteral("_"));
    QString path = folder.toLocalFile() + QLatin1Char('/') + name + QLatin1String(".2do");
    for (int i = 2; QFile::exists(path); ++i) {
        path = folder.toLocalFile() + QLatin1Char('/') + name + QLatin1Char(' ') + QString::number(i) + QLatin1String(".2do");
    }
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.trimmed().toUtf8());
        file.write("\n");
    }
}

void DolphinMainWindow::toggleToDoDone()
{
    for (const KFileItem &item : m_activeViewContainer->view()->selectedItems()) {
        const QString toggled = CommanderTools::toggledToDoName(item.name());
        if (item.isLocalFile() && toggled != item.name()) {
            QFile::rename(item.localPath(), item.url().adjusted(QUrl::RemoveFilename).toLocalFile() + toggled);
        }
    }
}

void DolphinMainWindow::showRecentNotifications()
{
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    const QStringList messages = DolphinViewContainer::recentMessages();
    if (messages.isEmpty()) {
        menu->addAction(i18nc("@info:status", "No notifications yet."))->setEnabled(false);
    }
    for (const QString &message : messages) {
        menu->addAction(message)->setEnabled(false);
    }
    menu->popup(m_activeViewContainer->mapToGlobal(QPoint(24, 24)));
}

void DolphinMainWindow::setCommanderLayout(CommanderLayout layout)
{
    m_commanderLayout = layout;
    QAction *infoPanel = actionCollection()->action(QStringLiteral("show_information_panel"));
    switch (layout) {
    case CommanderLayout::Standard:
        m_sidesSplitter->showBothSides();
        if (infoPanel && infoPanel->isChecked()) {
            infoPanel->trigger();
        }
        break;
    case CommanderLayout::Columns:
        m_sidesSplitter->showBothSides();
        if (QAction *columns = actionCollection()->action(QStringLiteral("columns"))) {
            columns->trigger();
        }
        break;
    case CommanderLayout::Classic: {
        m_sidesSplitter->showBothSides();
        QWidget *other = inactiveTabWidget();
        const bool otherIsLeft = m_sidesSplitter->widget(0) == other || m_sidesSplitter->widget(0)->isAncestorOf(other);
        m_sidesSplitter->moveDivider(otherIsLeft ? -1 : 1);
        if (infoPanel && !infoPanel->isChecked()) {
            infoPanel->trigger();
        }
        break;
    }
    }
}

void DolphinMainWindow::applyFileView(const QString &name)
{
    DolphinView *view = m_activeViewContainer->view();
    const auto trigger = [this](const char *action) {
        if (QAction *a = actionCollection()->action(QString::fromLatin1(action))) {
            a->trigger();
        }
    };
    if (name == QLatin1String("name-only")) {
        trigger("compact");
        view->setPreviewsShown(false);
    } else if (name == QLatin1String("thumbnail-grid")) {
        trigger("icons");
        view->setPreviewsShown(true);
    } else if (name == QLatin1String("details")) {
        trigger("details");
        view->setPreviewsShown(false);
        view->setVisibleRoles({"text", "size", "modificationtime"});
    } else if (name.startsWith(QLatin1String("saved:"))) {
        const KConfigGroup saved(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews"));
        const QStringList parts = saved.readEntry(name.mid(6), QStringList());
        // mode action, previews (0 or 1), the widths of the columns as "W=" and numbers, then the roles
        if (parts.size() >= 3) {
            trigger(parts.at(0).toLatin1().constData());
            view->setPreviewsShown(parts.at(1) == QLatin1String("1"));
            QList<QByteArray> roles;
            QList<int> widths;
            for (const QString &part : parts.mid(2)) {
                if (part.startsWith(QLatin1String("W="))) {
                    for (const QString &width : part.mid(2).split(QLatin1Char(','), Qt::SkipEmptyParts)) {
                        widths << width.toInt();
                    }
                } else {
                    roles << part.toLatin1();
                }
            }
            view->setVisibleRoles(roles);
            // The widths only count when there is one for each column that is shown.
            if (widths.count() == roles.count() && view->url().isLocalFile()) {
                ViewProperties props(view->url());
                props.setVisibleRoles(roles);
                props.setHeaderColumnWidths(widths);
                props.save();
                view->readSettings();
            }
        }
    } else if (name == QLatin1String("rich")) {
        // OneCommander's rich file view: tall rows with the start of a text and a picture of the other files.
        trigger("details");
        view->setPreviewsShown(true);
        view->setVisibleRoles({"text", "size", "modificationtime", "type"});
        view->setZoomLevel(qMin(ZoomLevelInfo::minimumLevel() + 6, ZoomLevelInfo::maximumLevel()));
    } else if (name == QLatin1String("thumbs-details")) {
        trigger("details");
        view->setPreviewsShown(true);
        view->setVisibleRoles({"text", "size", "modificationtime", "type"});
    }
}

void DolphinMainWindow::designFileView()
{
    DolphinView *view = m_activeViewContainer->view();
    if (!view->url().isLocalFile()) {
        m_activeViewContainer->showMessage(i18nc("@info", "File views can only be designed for folders on this computer."), KMessageWidget::Information);
        return;
    }
    auto *dialog = new FileViewDesignerDialog(view->visibleRoles(), ViewProperties(view->url()).headerColumnWidths(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setHideBelow(DolphinView::columnHideThresholds());
    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        DolphinView *target = m_activeViewContainer->view();
        const QList<QByteArray> roles = dialog->roles();
        const QList<int> widths = dialog->widths();
        if (roles.isEmpty()) {
            return;
        }
        if (dialog->temporaryOnly()) {
            // Only tried on the folder: its settings and the named views stay as they are.
            target->showColumnsTemporarily(roles, widths);
            return;
        }
        // The widths of the view under which the columns are hidden (when that is on) are kept for the whole program.
        QHash<QByteArray, int> thresholds = DolphinView::columnHideThresholds();
        const QList<int> hideBelow = dialog->hideBelow();
        QStringList stored;
        for (int i = 0; i < roles.count() && i < hideBelow.count(); ++i) {
            thresholds.insert(roles.at(i), hideBelow.at(i));
        }
        for (auto it = thresholds.cbegin(); it != thresholds.cend(); ++it) {
            if (it.value() > 0) {
                stored << QString::fromLatin1(it.key()) + QLatin1Char('=') + QString::number(it.value());
            }
        }
        DolphinView::setColumnHideThresholds(thresholds);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ColumnHideThresholds", stored);
        target->updateResponsiveColumns();
        target->setVisibleRoles(roles);
        {
            ViewProperties props(target->url());
            // The view takes its columns from the properties when it reads the settings again.
            props.setVisibleRoles(roles);
            props.setHeaderColumnWidths(widths.count() == roles.count() ? widths : QList<int>());
            props.save();
        }
        target->readSettings();
        if (!dialog->name().isEmpty()) {
            QString modeAction = QStringLiteral("details");
            if (target->viewMode() == DolphinView::IconsView) {
                modeAction = QStringLiteral("icons");
            } else if (target->viewMode() == DolphinView::CompactView) {
                modeAction = QStringLiteral("compact");
            }
            QStringList widthText;
            for (const int width : widths) {
                widthText << QString::number(width);
            }
            QStringList entry{modeAction,
                              target->previewsShown() ? QStringLiteral("1") : QStringLiteral("0"),
                              QLatin1String("W=") + widthText.join(QLatin1Char(','))};
            for (const QByteArray &role : roles) {
                entry << QString::fromLatin1(role);
            }
            KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews")).writeEntry(dialog->name(), entry);
        }
    });
    dialog->open();
}

bool DolphinMainWindow::exportViewDesign(const QString &path)
{
    DolphinView *view = m_activeViewContainer->view();
    QString mode = QStringLiteral("details");
    if (view->viewMode() == DolphinView::IconsView) {
        mode = QStringLiteral("icons");
    } else if (view->viewMode() == DolphinView::CompactView) {
        mode = QStringLiteral("compact");
    }
    QJsonArray roles;
    for (const QByteArray &role : view->visibleRoles()) {
        roles.append(QString::fromLatin1(role));
    }
    QJsonArray widths;
    for (const int width : ViewProperties(view->url()).headerColumnWidths()) {
        widths.append(width);
    }
    QJsonObject design{{QStringLiteral("mode"), mode},
                       {QStringLiteral("previews"), view->previewsShown()},
                       {QStringLiteral("roles"), roles},
                       {QStringLiteral("widths"), widths}};
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(design).toJson());
    return true;
}

bool DolphinMainWindow::importViewDesign(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonObject design = QJsonDocument::fromJson(file.readAll()).object();
    if (!design.contains(QLatin1String("roles")) || !design.contains(QLatin1String("mode"))) {
        return false;
    }
    // The design is applied as a named view that is not kept: the same way saved views are applied.
    QStringList entry{design.value(QLatin1String("mode")).toString(),
                      design.value(QLatin1String("previews")).toBool() ? QStringLiteral("1") : QStringLiteral("0")};
    QStringList widths;
    for (const QJsonValue &width : design.value(QLatin1String("widths")).toArray()) {
        widths << QString::number(width.toInt());
    }
    entry << QLatin1String("W=") + widths.join(QLatin1Char(','));
    for (const QJsonValue &role : design.value(QLatin1String("roles")).toArray()) {
        entry << role.toString();
    }
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews"));
    const QString temporary = QStringLiteral("\x01imported design");
    group.writeEntry(temporary, entry);
    applyFileView(QStringLiteral("saved:") + temporary);
    group.deleteEntry(temporary);
    return true;
}

void DolphinMainWindow::askFirstStartChoices()
{
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    QDialog dialog(this);
    dialog.setWindowTitle(i18nc("@title:window", "Welcome"));
    auto *layout = new QFormLayout(&dialog);
    layout->addRow(new QLabel(i18nc("@info", "Choose how the program starts. Everything can be changed later in the View and Tools menus."), &dialog));
    auto *layoutChoice = new QComboBox(&dialog);
    layoutChoice->setObjectName(QStringLiteral("firstLayout"));
    layoutChoice->addItem(i18nc("@item:inlistbox", "Standard: two sides"), 0);
    layoutChoice->addItem(i18nc("@item:inlistbox", "Columns: Miller columns"), 1);
    layoutChoice->addItem(i18nc("@item:inlistbox", "Classic: one side with information"), 2);
    layout->addRow(i18nc("@label:listbox", "Layout:"), layoutChoice);
    auto *themeChoice = new QComboBox(&dialog);
    themeChoice->setObjectName(QStringLiteral("firstTheme"));
    themeChoice->addItem(i18nc("@item:inlistbox", "Follow the desktop"), QStringLiteral("system"));
    themeChoice->addItem(i18nc("@item:inlistbox", "Light"), QStringLiteral("light"));
    themeChoice->addItem(i18nc("@item:inlistbox", "Dark"), QStringLiteral("dark"));
    themeChoice->addItem(i18nc("@item:inlistbox", "Nord"), QStringLiteral("nord"));
    layout->addRow(i18nc("@label:listbox", "Colors:"), themeChoice);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addRow(buttons);
    dialog.exec();
    setCommanderLayout(static_cast<CommanderLayout>(layoutChoice->currentData().toInt()));
    applyColorTheme(themeChoice->currentData().toString());
    group.writeEntry("FirstStartDone", true);
}

void DolphinMainWindow::saveFileView()
{
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, i18nc("@title:window", "Save File View"), i18nc("@label:textbox", "Name of this view:"), QLineEdit::Normal, QString(), &ok)
            .trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    DolphinView *view = m_activeViewContainer->view();
    QString modeAction = QStringLiteral("details");
    if (view->viewMode() == DolphinView::IconsView) {
        modeAction = QStringLiteral("icons");
    } else if (view->viewMode() == DolphinView::CompactView) {
        modeAction = QStringLiteral("compact");
    }
    QStringList entry{modeAction, view->previewsShown() ? QStringLiteral("1") : QStringLiteral("0")};
    QStringList widths;
    for (const int width : ViewProperties(view->url()).headerColumnWidths()) {
        widths << QString::number(width);
    }
    entry << QLatin1String("W=") + widths.join(QLatin1Char(','));
    for (const QByteArray &role : view->visibleRoles()) {
        entry << QString::fromLatin1(role);
    }
    KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews")).writeEntry(name, entry);
    m_activeViewContainer->showMessage(i18nc("@info:status", "Saved the file view %1.", name), KMessageWidget::Information);
}

void DolphinMainWindow::pasteClipboardAsFile()
{
    const QUrl folder = m_activeViewContainer->url();
    // A web address can be kept as a link file or as the text, or the file it points to can be downloaded (what OneCommander offers).
    const QUrl web = CommanderTools::webLinkIn(QApplication::clipboard()->text());
    QString path;
    if (web.isValid() && folder.isLocalFile()) {
        QMessageBox box(QMessageBox::Question,
                        i18nc("@title:window", "Paste Web Address"),
                        i18nc("@info", "The clipboard has the address %1.", web.toString()),
                        QMessageBox::Cancel,
                        this);
        QPushButton *link = box.addButton(i18nc("@action:button", "Link File"), QMessageBox::AcceptRole);
        QPushButton *text = box.addButton(i18nc("@action:button", "Text File"), QMessageBox::AcceptRole);
        QPushButton *download = box.addButton(i18nc("@action:button", "Download"), QMessageBox::AcceptRole);
        box.exec();
        if (box.clickedButton() == link) {
            path = CommanderTools::saveLinkAsFile(web, folder.toLocalFile());
        } else if (box.clickedButton() == download) {
            KIO::copy({web}, folder)->start();
            return;
        } else if (box.clickedButton() != text) {
            return;
        }
    }
    if (path.isEmpty()) {
        path = folder.isLocalFile() ? CommanderTools::saveClipboardAsFile(QApplication::clipboard()->mimeData(), folder.toLocalFile()) : QString();
    }
    if (path.isEmpty()) {
        m_activeViewContainer->showMessage(
            i18nc("@info", "There is no image or text on the clipboard to make a file of, or this folder is not on this computer."),
            KMessageWidget::Information);
        return;
    }
    m_activeViewContainer->view()->markUrlsAsSelected({QUrl::fromLocalFile(path)});
}

void DolphinMainWindow::newTabFromClipboard()
{
    const QString text = QApplication::clipboard()->text().trimmed();
    const QUrl url = text.startsWith(QLatin1Char('/')) ? QUrl::fromLocalFile(text) : QUrl::fromUserInput(text);
    if (text.isEmpty() || !url.isValid()) {
        m_activeViewContainer->showMessage(i18nc("@info", "The clipboard has no folder path."), KMessageWidget::Information);
        return;
    }
    openNewTab(url);
}

// The icon theme of the desktop, noted before the program changes it.
// Makes the widgets that already have icons take the icons of the theme that is set.
static void iconThemeChanged()
{
    KIconTheme::reconfigure();
    KIconLoader::global()->newIconLoader();
    QPixmapCache::clear();
    for (QWidget *widget : QApplication::allWidgets()) {
        QEvent event(QEvent::StyleChange);
        QApplication::sendEvent(widget, &event);
        widget->update();
    }
}

static QString systemIconThemeName()
{
    static const QString name = QIcon::themeName();
    return name;
}

// Breeze icons are dark on light; on a dark theme the light variant is easier to read, and back.
static QString iconThemeForColors(bool dark)
{
    const QString system = systemIconThemeName();
    const auto installed = [](const QString &theme) {
        for (const QString &path : QIcon::themeSearchPaths()) {
            if (QFile::exists(path + QLatin1Char('/') + theme + QLatin1String("/index.theme"))) {
                return true;
            }
        }
        return false;
    };
    if (dark && system == QLatin1String("breeze") && installed(QStringLiteral("breeze-dark"))) {
        return QStringLiteral("breeze-dark");
    }
    if (!dark && system == QLatin1String("breeze-dark") && installed(QStringLiteral("breeze"))) {
        return QStringLiteral("breeze");
    }
    return system;
}

void DolphinMainWindow::applyColorTheme(const QString &theme)
{
    systemIconThemeName();
    static const QPalette systemPalette = QApplication::palette();
    // The views take their background from the program's palette while one of the fork's themes is on.
    DolphinView::setUsesApplicationPalette(theme == QLatin1String("dark") || theme == QLatin1String("light") || theme == QLatin1String("custom")
                                           || theme == QLatin1String("nord"));

    // Light and Dark are the desktop's own Breeze color schemes when they are installed: their icons are recolored with them.
    const auto installedScheme = [](const QString &id) {
        QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
        dirs << QStringLiteral("/usr/share/color-schemes") << QStringLiteral("/usr/local/share/color-schemes");
        for (const QString &dir : std::as_const(dirs)) {
            if (QFile::exists(dir + QLatin1Char('/') + id + QLatin1String(".colors"))) {
                return true;
            }
        }
        return false;
    };
    const QString schemeId =
        theme == QLatin1String("dark") ? QStringLiteral("BreezeDark") : (theme == QLatin1String("light") ? QStringLiteral("BreezeLight") : QString());
    if (!schemeId.isEmpty() && installedScheme(schemeId)) {
        KColorSchemeManager::instance()->activateSchemeId(schemeId);
        m_schemeThemeActive = true;
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ColorTheme", theme);
        QPixmapCache::clear();
        return;
    }
    if (theme == QLatin1String("system") && m_schemeThemeActive) {
        KColorSchemeManager::instance()->activateSchemeId(QString());
        m_schemeThemeActive = false;
    }
    if (theme == QLatin1String("dark") || theme == QLatin1String("light") || theme == QLatin1String("custom") || theme == QLatin1String("nord")) {
        const bool dark = theme == QLatin1String("dark") || theme == QLatin1String("nord");
        QPalette palette;
        QColor window = dark ? QColor(37, 39, 43) : QColor(244, 244, 246);
        QColor base = dark ? QColor(27, 29, 33) : QColor(255, 255, 255);
        QColor text = dark ? QColor(230, 232, 236) : QColor(30, 32, 36);
        QColor highlight = dark ? QColor(61, 132, 220) : QColor(42, 110, 200);
        if (theme == QLatin1String("nord")) {
            // The Nord palette, as OneCommander's Nord theme uses it: polar night for the windows, snow storm for the text, frost for the highlight.
            window = QColor(59, 66, 82);
            base = QColor(46, 52, 64);
            text = QColor(216, 222, 233);
            highlight = QColor(94, 129, 172);
        }
        if (theme == QLatin1String("custom")) {
            // window, base, text, highlight: the colors the user chose
            const QStringList chosen = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("CustomTheme", QStringList());
            if (chosen.size() == 4) {
                window = QColor(chosen.at(0));
                base = QColor(chosen.at(1));
                text = QColor(chosen.at(2));
                highlight = QColor(chosen.at(3));
            }
        }
        palette.setColor(QPalette::Window, window);
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, base);
        palette.setColor(QPalette::AlternateBase, dark ? QColor(33, 35, 39) : QColor(248, 248, 250));
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::Button, window);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::ToolTipBase, base);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::Highlight, highlight);
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::PlaceholderText, dark ? QColor(140, 144, 150) : QColor(120, 124, 130));
        palette.setColor(QPalette::Link, highlight);
        palette.setColor(QPalette::Disabled, QPalette::Text, dark ? QColor(110, 114, 120) : QColor(160, 164, 170));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, dark ? QColor(110, 114, 120) : QColor(160, 164, 170));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, dark ? QColor(110, 114, 120) : QColor(160, 164, 170));
        QApplication::setPalette(palette);
    } else {
        QApplication::setPalette(systemPalette);
    }
    // Icons drawn before are drawn again with the new colors.
    QPixmapCache::clear();

    KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ColorTheme", theme);
}

void DolphinMainWindow::applyTextAndIcons(int fontPointSize, const QString &iconTheme)
{
    static const QFont systemFont = QApplication::font();
    const QString systemIconTheme = iconThemeForColors(QApplication::palette().color(QPalette::Base).lightnessF() < 0.5);
    QFont font = systemFont;
    if (fontPointSize > 0) {
        font.setPointSize(fontPointSize);
    }
    QApplication::setFont(font);
    QIcon::setThemeName(iconTheme.isEmpty() ? systemIconTheme : iconTheme);
    iconThemeChanged();
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    group.writeEntry("FontPointSize", fontPointSize);
    group.writeEntry("IconTheme", iconTheme);
}

void DolphinMainWindow::editTextAndIcons()
{
    const KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    QDialog dialog(this);
    dialog.setWindowTitle(i18nc("@title:window", "Text and Icons"));
    auto *layout = new QFormLayout(&dialog);
    auto *size = new QSpinBox(&dialog);
    size->setObjectName(QStringLiteral("fontPointSize"));
    size->setRange(0, 32);
    size->setSpecialValueText(i18nc("@item", "The desktop's size"));
    size->setSuffix(i18nc("@item:valuesuffix points", " pt"));
    size->setValue(group.readEntry("FontPointSize", 0));
    layout->addRow(i18nc("@label:spinbox", "Text size:"), size);
    auto *icons = new QComboBox(&dialog);
    icons->setObjectName(QStringLiteral("iconTheme"));
    icons->addItem(i18nc("@item:inlistbox", "The desktop's icons"), QString());
    QStringList themes;
    for (const QString &path : QIcon::themeSearchPaths()) {
        for (const QString &entry : QDir(path).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (QFile::exists(path + QLatin1Char('/') + entry + QLatin1String("/index.theme")) && !themes.contains(entry)) {
                themes << entry;
            }
        }
    }
    themes.sort();
    for (const QString &theme : std::as_const(themes)) {
        icons->addItem(theme, theme);
    }
    icons->setCurrentIndex(qMax(0, icons->findData(group.readEntry("IconTheme", QString()))));
    layout->addRow(i18nc("@label:listbox", "Icons:"), icons);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);
    if (dialog.exec() == QDialog::Accepted) {
        applyTextAndIcons(size->value(), icons->currentData().toString());
    }
}

void DolphinMainWindow::applyStyleSheet(const QString &styleSheet)
{
    // The thin scrollbars are style sheet rules of their own that go after the sheet of the user.
    static const QString thinScrollbars = QStringLiteral(
        "QScrollBar:vertical { width: 7px; background: transparent; margin: 0; }"
        "QScrollBar:horizontal { height: 7px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: palette(mid); border-radius: 3px; min-height: 24px; }"
        "QScrollBar::handle:horizontal { background: palette(mid); border-radius: 3px; min-width: 24px; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background: none; }");
    const bool thin = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ThinScrollbars", false);
    qApp->setStyleSheet(thin ? styleSheet + QLatin1Char('\n') + thinScrollbars : styleSheet);
    KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("StyleSheet", styleSheet);
}

void DolphinMainWindow::editStyleSheet()
{
    QDialog dialog(this);
    dialog.setWindowTitle(i18nc("@title:window", "Style Sheet"));
    dialog.resize(560, 420);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(
        i18nc("@info",
              "A style sheet changes how the widgets of the whole program look, for example:\nQTreeView, QListView { selection-background-color: #c0392b; }"),
        &dialog));
    auto *editor = new QPlainTextEdit(&dialog);
    editor->setObjectName(QStringLiteral("styleSheetEditor"));
    editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor->setPlainText(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("StyleSheet", QString()));
    layout->addWidget(editor);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() == QDialog::Accepted) {
        applyStyleSheet(editor->toPlainText());
    }
}

bool DolphinMainWindow::exportTheme(const QString &path)
{
    const KConfigGroup current(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    KConfig file(path, KConfig::SimpleConfig);
    KConfigGroup theme(&file, QStringLiteral("Theme"));
    theme.writeEntry("ColorTheme", current.readEntry("ColorTheme", QStringLiteral("system")));
    theme.writeEntry("CustomTheme", current.readEntry("CustomTheme", QStringList()));
    theme.writeEntry("FontPointSize", current.readEntry("FontPointSize", 0));
    theme.writeEntry("IconTheme", current.readEntry("IconTheme", QString()));
    theme.writeEntry("StyleSheet", current.readEntry("StyleSheet", QString()));
    return file.sync();
}

bool DolphinMainWindow::importTheme(const QString &path)
{
    if (!QFile::exists(path)) {
        return false;
    }
    KConfig file(path, KConfig::SimpleConfig);
    const KConfigGroup theme(&file, QStringLiteral("Theme"));
    if (!theme.exists()) {
        return false;
    }
    KConfigGroup current(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    current.writeEntry("CustomTheme", theme.readEntry("CustomTheme", QStringList()));
    applyTextAndIcons(theme.readEntry("FontPointSize", 0), theme.readEntry("IconTheme", QString()));
    applyColorTheme(theme.readEntry("ColorTheme", QStringLiteral("system")));
    applyStyleSheet(theme.readEntry("StyleSheet", QString()));
    return true;
}

void DolphinMainWindow::editCustomTheme()
{
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Commander"));
    QStringList colors = group.readEntry("CustomTheme", QStringList());
    if (colors.size() != 4) {
        colors = {QApplication::palette().color(QPalette::Window).name(),
                  QApplication::palette().color(QPalette::Base).name(),
                  QApplication::palette().color(QPalette::Text).name(),
                  QApplication::palette().color(QPalette::Highlight).name()};
    }
    const QStringList titles = {i18nc("@title:window", "Color of the Windows"),
                                i18nc("@title:window", "Color of the File List"),
                                i18nc("@title:window", "Color of the Text"),
                                i18nc("@title:window", "Color of the Selection")};
    for (int i = 0; i < 4; ++i) {
        const QColor chosen = QColorDialog::getColor(QColor(colors.at(i)), this, titles.at(i));
        if (!chosen.isValid()) {
            return; // cancelled: nothing changes
        }
        colors[i] = chosen.name();
    }
    group.writeEntry("CustomTheme", colors);
    applyColorTheme(QStringLiteral("custom"));
}

void DolphinMainWindow::newNote()
{
    const QUrl folder = m_activeViewContainer->url();
    if (!folder.isLocalFile()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Notes can only be made in folders on this computer."), KMessageWidget::Information);
        return;
    }
    const QString base = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH.mm")) + QLatin1String(" note");
    QString path = folder.toLocalFile() + QLatin1Char('/') + base + QLatin1String(".txt");
    for (int i = 2; QFile::exists(path); ++i) {
        path = folder.toLocalFile() + QLatin1Char('/') + base + QLatin1Char(' ') + QString::number(i) + QLatin1String(".txt");
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_activeViewContainer->showMessage(i18nc("@info", "The note could not be made."), KMessageWidget::Error);
        return;
    }
    file.close();
    m_activeViewContainer->view()->markUrlsAsSelected({QUrl::fromLocalFile(path)});
    m_activeViewContainer->view()->renameSelectedItems();
}

void DolphinMainWindow::fillScriptsMenu(QMenu *menu)
{
    menu->clear();
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/scripts");
    const QFileInfoList scripts = QDir(dirPath).entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo &script : scripts) {
        const QString scriptPath = script.absoluteFilePath();
        // A script that is not executable is run by its interpreter: Python, shell, Perl, Ruby, Node.
        static const QHash<QString, QString> interpreters = {{QStringLiteral("py"), QStringLiteral("python3")},
                                                             {QStringLiteral("sh"), QStringLiteral("sh")},
                                                             {QStringLiteral("pl"), QStringLiteral("perl")},
                                                             {QStringLiteral("rb"), QStringLiteral("ruby")},
                                                             {QStringLiteral("js"), QStringLiteral("node")}};
        const QString interpreter = interpreters.value(script.suffix().toLower());
        if (!script.isExecutable() && interpreter.isEmpty()) {
            continue;
        }
        menu->addAction(script.completeBaseName(), this, [this, scriptPath, interpreter, executable = script.isExecutable()]() {
            QStringList args;
            const KFileItemList selected = m_activeViewContainer->view()->selectedItems();
            for (const KFileItem &item : selected) {
                if (item.isLocalFile()) {
                    args << item.localPath();
                }
            }
            if (!executable) {
                args.prepend(scriptPath);
                QProcess::startDetached(interpreter, args, m_activeViewContainer->url().toLocalFile());
            } else {
                QProcess::startDetached(scriptPath, args, m_activeViewContainer->url().toLocalFile());
            }
        });
    }
    if (scripts.isEmpty()) {
        menu->addAction(i18nc("@info:status", "Put executable scripts in %1", dirPath))->setEnabled(false);
    }
}

void DolphinMainWindow::showDuplicateFinder()
{
    const QUrl url = m_activeViewContainer->url();
    if (!url.isLocalFile()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Duplicates can only be found in folders on this computer."), KMessageWidget::Information);
        return;
    }
    auto *dialog = new DuplicateFinderDialog(url.toLocalFile(), this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DolphinMainWindow::showBatchRename()
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select the items to rename."), KMessageWidget::Information);
        return;
    }
    auto *dialog = new BatchRenameDialog(items, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

void DolphinMainWindow::runArk(bool unpack)
{
    const KFileItemList items = m_activeViewContainer->view()->selectedItems();
    if (items.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select the items first."), KMessageWidget::Information);
        return;
    }
    const QString ark = QStandardPaths::findExecutable(QStringLiteral("ark"));
    if (ark.isEmpty()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Ark is needed to pack and unpack, and it is not installed."), KMessageWidget::Warning);
        return;
    }
    QStringList arguments;
    if (unpack) {
        const QUrl destination = inactiveViewContainer() == m_activeViewContainer ? m_activeViewContainer->url() : inactiveViewContainer()->url();
        arguments << QStringLiteral("--batch") << QStringLiteral("--destination") << destination.toLocalFile();
    } else {
        arguments << QStringLiteral("--add") << QStringLiteral("--dialog");
    }
    for (const KFileItem &item : items) {
        arguments << item.url().toString(QUrl::PreferLocalFile);
    }
    QProcess::startDetached(ark, arguments);
}

void DolphinMainWindow::showQuickView()
{
    KFileItemList items;
    DolphinViewContainer *source = m_activeViewContainer;
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    if (otherPage && otherPage->activeViewContainer() != m_activeViewContainer) {
        items = otherPage->activeViewContainer()->view()->selectedItems();
        if (!items.isEmpty()) {
            source = otherPage->activeViewContainer();
        }
    }
    if (items.isEmpty()) {
        items = m_activeViewContainer->view()->selectedItems();
    }
    if (items.isEmpty() || items.first().isDir()) {
        m_activeViewContainer->showMessage(i18nc("@info", "Select a file to look into."), KMessageWidget::Information);
        return;
    }
    if (!m_quickView) {
        m_quickView = new QuickViewDialog(this);
        connect(m_quickView, &QuickViewDialog::stepRequested, this, &DolphinMainWindow::stepQuickView);
    }
    m_quickViewSource = source;
    m_quickView->show(items.first());
}

void DolphinMainWindow::stepQuickView(int delta)
{
    if (!m_quickView || !m_quickViewSource) {
        return;
    }
    DolphinView *view = m_quickViewSource->view();
    const KFileItemList items = view->items();
    int index = -1;
    for (int i = 0; i < items.count(); ++i) {
        if (items.at(i).name() == m_quickView->shownName()) {
            index = i;
            break;
        }
    }
    // The next file in that direction; a folder has nothing to look into, so it is passed over.
    for (int i = index + delta; i >= 0 && i < items.count(); i += delta) {
        if (!items.at(i).isDir()) {
            view->selectOnlyItemsNamed({items.at(i).name()});
            m_quickView->show(items.at(i));
            return;
        }
    }
}

void DolphinMainWindow::fillWorkspacesMenu(QMenu *menu)
{
    menu->clear();
    KConfigGroup group(KSharedConfig::openConfig(), QStringLiteral("Workspaces"));
    QAction *save = menu->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18nc("@action:inmenu", "Save Tabs as Workspace…"));
    connect(save, &QAction::triggered, this, [this]() {
        bool ok = false;
        const QString name =
            QInputDialog::getText(this, i18nc("@title:window", "Save Workspace"), i18nc("@label:textbox", "Name:"), QLineEdit::Normal, QString(), &ok)
                .trimmed();
        if (ok && !name.isEmpty()) {
            saveWorkspace(name);
        }
    });
    const QStringList names = group.groupList();
    if (!names.isEmpty()) {
        menu->addSeparator();
    }
    for (const QString &name : names) {
        QAction *open = menu->addAction(QIcon::fromTheme(QStringLiteral("window-duplicate")), name);
        connect(open, &QAction::triggered, this, [this, name]() {
            openWorkspace(name);
        });
    }
    if (!names.isEmpty()) {
        QMenu *remove = menu->addMenu(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:inmenu", "Remove Workspace"));
        for (const QString &name : names) {
            connect(remove->addAction(name), &QAction::triggered, this, [name]() {
                KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Workspaces")).deleteGroup(name);
                KSharedConfig::openConfig()->sync();
            });
        }
    }
}

void DolphinMainWindow::saveWorkspace(const QString &name)
{
    KConfigGroup parent(KSharedConfig::openConfig(), QStringLiteral("Workspaces"));
    parent.deleteGroup(name);
    KConfigGroup group = parent.group(name);
    const QPair<const char *, DolphinTabWidget *> sides[2] = {{"Left", m_leftTabs}, {"Right", m_rightTabs}};
    for (const auto &side : sides) {
        QList<QUrl> urls;
        for (int i = 0; i < side.second->count(); ++i) {
            urls << side.second->tabPageAt(i)->primaryViewContainer()->url();
        }
        group.writeEntry(side.first, urls);
    }
    KSharedConfig::openConfig()->sync();
}

void DolphinMainWindow::openWorkspace(const QString &name)
{
    const KConfigGroup group = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Workspaces")).group(name);
    const QPair<const char *, DolphinTabWidget *> sides[2] = {{"Left", m_leftTabs}, {"Right", m_rightTabs}};
    for (const auto &side : sides) {
        const QList<QUrl> urls = group.readEntry(side.first, QList<QUrl>());
        for (const QUrl &url : urls) {
            side.second->openNewTab(url, QUrl());
        }
    }
    m_sidesSplitter->showBothSides();
}

void DolphinMainWindow::compareDirectories()
{
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    DolphinViewContainer *other = otherPage ? otherPage->activeViewContainer() : nullptr;
    if (!other || !m_activeViewContainer || other == m_activeViewContainer) {
        return;
    }
    m_sidesSplitter->showBothSides();
    DolphinView *mine = m_activeViewContainer->view();
    DolphinView *theirs = other->view();

    QHash<QString, KFileItem> mineByName;
    QHash<QString, KFileItem> theirsByName;
    for (const KFileItem &item : mine->items()) {
        mineByName.insert(item.name(), item);
    }
    for (const KFileItem &item : theirs->items()) {
        theirsByName.insert(item.name(), item);
    }

    QSet<QString> selectMine;
    QSet<QString> selectTheirs;
    for (auto it = mineByName.cbegin(); it != mineByName.cend(); ++it) {
        const auto match = theirsByName.constFind(it.key());
        if (match == theirsByName.cend()) {
            selectMine.insert(it.key());
            continue;
        }
        const KFileItem &a = it.value();
        const KFileItem &b = match.value();
        if (a.isDir() || b.isDir()) {
            continue;
        }
        const QDateTime timeA = a.time(KFileItem::ModificationTime);
        const QDateTime timeB = b.time(KFileItem::ModificationTime);
        if (timeA != timeB) {
            (timeA > timeB ? selectMine : selectTheirs).insert(it.key());
        } else if (a.size() != b.size()) {
            selectMine.insert(it.key());
            selectTheirs.insert(it.key());
        }
    }
    for (auto it = theirsByName.cbegin(); it != theirsByName.cend(); ++it) {
        if (!mineByName.contains(it.key())) {
            selectTheirs.insert(it.key());
        }
    }
    mine->selectOnlyItemsNamed(selectMine);
    theirs->selectOnlyItemsNamed(selectTheirs);
    m_activeViewContainer->showMessage(i18ncp("@info:status", "%1 item selected here, ", "%1 items selected here, ", selectMine.size())
                                           + i18ncp("@info:status", "%1 on the other side", "%1 on the other side", selectTheirs.size()),
                                       KMessageWidget::Information);
}

void DolphinMainWindow::syncOtherSide(const QUrl &newUrl)
{
    rememberRecentFolder(newUrl);
    const QUrl previous = m_syncPreviousUrl;
    m_syncPreviousUrl = newUrl;
    QAction *syncAction = actionCollection()->action(QStringLiteral("sync_browsing"));
    if (!syncAction || !syncAction->isChecked() || !previous.isValid() || !m_activeViewContainer) {
        return;
    }
    DolphinTabPage *otherPage = inactiveTabWidget()->currentTabPage();
    DolphinViewContainer *other = otherPage ? otherPage->activeViewContainer() : nullptr;
    if (!other || other == m_activeViewContainer) {
        return;
    }
    QUrl from = previous.adjusted(QUrl::StripTrailingSlash);
    QUrl to = newUrl.adjusted(QUrl::StripTrailingSlash);
    QUrl target;
    if (from.isParentOf(to)) {
        // Entered a folder: enter the folder of the same name on the other side, if there is one.
        const QString relative = to.path().mid(from.path().size()).remove(QRegularExpression(QStringLiteral("^/+")));
        target = other->url().adjusted(QUrl::StripTrailingSlash);
        target.setPath(target.path() + QLatin1Char('/') + relative);
        if (target.isLocalFile() && !QFileInfo(target.toLocalFile()).isDir()) {
            return;
        }
    } else if (to.isParentOf(from)) {
        // Went up some levels: go up as many levels on the other side.
        const int levels = from.path().split(QLatin1Char('/'), Qt::SkipEmptyParts).size() - to.path().split(QLatin1Char('/'), Qt::SkipEmptyParts).size();
        target = other->url();
        for (int i = 0; i < levels; ++i) {
            target = KIO::upUrl(target);
        }
    } else {
        return;
    }
    if (target != other->url()) {
        // Following must not take the focus: the side that is browsed stays the active one.
        QPointer<DolphinViewContainer> driver = m_activeViewContainer;
        other->setUrl(target);
        if (driver && m_activeViewContainer != driver) {
            driver->setActive(true);
            driver->view()->setFocus();
        }
    }
}

void DolphinMainWindow::tabCountChanged(int count)
{
    const bool enableTabActions = (count > 1);
    for (int i = 0; i < MaxActivateTabShortcuts; ++i) {
        actionCollection()->action(QStringLiteral("activate_tab_%1").arg(i))->setEnabled(enableTabActions);
    }
    actionCollection()->action(QStringLiteral("activate_last_tab"))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_next_tab"))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_prev_tab"))->setEnabled(enableTabActions);
}

void DolphinMainWindow::updateWindowTitle()
{
    const QString newTitle = m_activeViewContainer->captionWindowTitle();
    if (windowTitle() != newTitle) {
        setWindowTitle(newTitle);
    }
}

void DolphinMainWindow::slotStorageTearDownFromPlacesRequested(const QString &mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectoryIsChildOf(mountPath)) {
        m_tearDownFromPlacesRequested = true;
        m_terminalPanel->goHome();
        // m_placesPanel->proceedWithTearDown() will be called in slotTerminalDirectoryChanged
    } else {
        m_placesPanel->proceedWithTearDown();
    }
}

void DolphinMainWindow::slotStorageTearDownExternallyRequested(const QString &mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectoryIsChildOf(mountPath)) {
        m_tearDownFromPlacesRequested = false;
        m_terminalPanel->goHome();
    }
}

void DolphinMainWindow::slotKeyBindings()
{
#if KIO_VERSION >= QT_VERSION_CHECK(6, 24, 0)
    m_serviceMenuShortcutManager->cleanupStaleShortcuts(this);
#endif

    KShortcutsDialog dialog(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dialog.addCollection(actionCollection());
    if (m_terminalPanel) {
        KActionCollection *konsolePartActionCollection = m_terminalPanel->actionCollection();
        if (konsolePartActionCollection) {
            dialog.addCollection(konsolePartActionCollection, QStringLiteral("KonsolePart"));
        }
    }
    dialog.configure();
}

void DolphinMainWindow::setViewsToHomeIfMountPathOpen(const QString &mountPath)
{
    const QVector<DolphinViewContainer *> theViewContainers = viewContainers();
    for (DolphinViewContainer *viewContainer : theViewContainers) {
        if (!viewContainer) {
            continue;
        }
        const auto viewPath = viewContainer->url().toLocalFile();
        if (viewPath.startsWith(mountPath + QLatin1String("/")) || viewPath == mountPath) {
            viewContainer->setUrl(QUrl::fromLocalFile(QDir::homePath()));
        }
    }
    disconnect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, nullptr, nullptr);
}

void DolphinMainWindow::setupActions()
{
    auto hamburgerMenuAction = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection());

    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(nullptr, nullptr, this);
    actionCollection()->addAction(QStringLiteral("new_menu"), m_newFileMenu);
    QMenu *menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_newFileMenu->setPopupMode(QToolButton::InstantPopup);
    connect(menu, &QMenu::aboutToShow, this, &DolphinMainWindow::updateNewMenu);
    connect(m_newFileMenu, &KNewFileMenu::directoryCreated, this, [this](const QUrl &createdDirectory) {
        activeViewContainer()->view()->expandToUrl(createdDirectory);
    });

    QAction *newWindow = KStandardAction::openNew(this, &DolphinMainWindow::openNewMainWindow, actionCollection());
    // OneCommander keeps Ctrl+N for the new file from a template.
    actionCollection()->setDefaultShortcut(newWindow, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_N);
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));
    newWindow->setToolTip(i18nc("@info", "Open a new Dolphin window"));
    newWindow->setWhatsThis(xi18nc("@info:whatsthis",
                                   "This opens a new "
                                   "window just like this one with the current location."
                                   "<nl/>You can drag and drop items between windows."));
    newWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));

    QAction *newTab = actionCollection()->addAction(QStringLiteral("new_tab"));
    newTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    newTab->setWhatsThis(xi18nc("@info:whatsthis",
                                "This opens a new "
                                "<emphasis>Tab</emphasis> with the current location."
                                "<nl/>Tabs allow you to quickly switch between multiple locations and views within this window. "
                                "You can drag and drop items between tabs."));
    actionCollection()->setDefaultShortcut(newTab, Qt::CTRL | Qt::Key_T);
    connect(newTab, &QAction::triggered, this, &DolphinMainWindow::openNewActivatedTab);

    QAction *addToPlaces = actionCollection()->addAction(QStringLiteral("add_to_places"));
    addToPlaces->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    addToPlaces->setText(i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    addToPlaces->setWhatsThis(xi18nc("@info:whatsthis",
                                     "This adds the selected folder "
                                     "to the Places panel."));
    connect(addToPlaces, &QAction::triggered, this, &DolphinMainWindow::addToPlaces);

    QAction *closeTab = KStandardAction::close(this, &DolphinMainWindow::closeCurrentTab, actionCollection());
    closeTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setToolTip(i18nc("@info", "Close Tab"));
    closeTab->setWhatsThis(i18nc("@info:whatsthis",
                                 "This closes the "
                                 "currently viewed tab. If no more tabs are left, this closes "
                                 "the whole window instead."));

    QAction *quitAction = KStandardAction::quit(this, &DolphinMainWindow::quit, actionCollection());
    quitAction->setWhatsThis(i18nc("@info:whatsthis quit", "This closes this window."));

    // setup 'Edit' menu
    KStandardAction::undo(this, &DolphinMainWindow::undo, actionCollection());
    KStandardAction::redo(this, &DolphinMainWindow::redo, actionCollection());

    // i18n: This will be the last paragraph for the whatsthis for all three:
    // Cut, Copy and Paste
    const QString cutCopyPastePara = xi18nc("@info:whatsthis",
                                            "<para><emphasis>Cut, "
                                            "Copy</emphasis> and <emphasis>Paste</emphasis> work between many "
                                            "applications and are among the most used commands. That's why their "
                                            "<emphasis>keyboard shortcuts</emphasis> are prominently placed right "
                                            "next to each other on the keyboard: <shortcut>Ctrl+X</shortcut>, "
                                            "<shortcut>Ctrl+C</shortcut> and <shortcut>Ctrl+V</shortcut>.</para>");
    QAction *cutAction = KStandardAction::cut(this, &DolphinMainWindow::cut, actionCollection());
    m_actionTextHelper->registerTextWhenNothingIsSelected(cutAction, i18nc("@action", "Cut…"));
    cutAction->setWhatsThis(xi18nc("@info:whatsthis cut",
                                   "This copies the items "
                                   "in your current selection to the <emphasis>clipboard</emphasis>.<nl/>"
                                   "Use the <emphasis>Paste</emphasis> action afterwards to copy them from "
                                   "the clipboard to a new location. The items will be removed from their "
                                   "initial location.")
                            + cutCopyPastePara);
    QAction *copyAction = KStandardAction::copy(this, &DolphinMainWindow::copy, actionCollection());
    m_actionTextHelper->registerTextWhenNothingIsSelected(copyAction, i18nc("@action", "Copy…"));
    copyAction->setWhatsThis(xi18nc("@info:whatsthis copy",
                                    "This copies the "
                                    "items in your current selection to the <emphasis>clipboard</emphasis>."
                                    "<nl/>Use the <emphasis>Paste</emphasis> action afterwards to copy them "
                                    "from the clipboard to a new location.")
                             + cutCopyPastePara);
    QAction *paste = KStandardAction::paste(this, &DolphinMainWindow::paste, actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));
    paste->setWhatsThis(xi18nc("@info:whatsthis paste",
                               "This copies the items from "
                               "your <emphasis>clipboard</emphasis> to the currently viewed folder.<nl/>"
                               "If the items were added to the clipboard by the <emphasis>Cut</emphasis> "
                               "action they are removed from their old location.")
                        + cutCopyPastePara);

    QAction *copyToOtherViewAction = actionCollection()->addAction(QStringLiteral("copy_to_inactive_split_view"));
    copyToOtherViewAction->setText(i18nc("@action:inmenu", "Copy to Other View"));
    m_actionTextHelper->registerTextWhenNothingIsSelected(copyToOtherViewAction, i18nc("@action:inmenu", "Copy to Other View…"));
    copyToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Copy",
                                               "This copies the selected items from "
                                               "the view in focus to the other view. "
                                               "(Only available while in Split View mode.)"));
    copyToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    copyToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Copy to Other View"));
    actionCollection()->setDefaultShortcuts(copyToOtherViewAction, {QKeySequence(Qt::Key_F5), QKeySequence(Qt::ALT | Qt::Key_C)});
    connect(copyToOtherViewAction, &QAction::triggered, this, &DolphinMainWindow::copyToInactiveSplitView);

    QAction *moveToOtherViewAction = actionCollection()->addAction(QStringLiteral("move_to_inactive_split_view"));
    moveToOtherViewAction->setText(i18nc("@action:inmenu", "Move to Other View"));
    m_actionTextHelper->registerTextWhenNothingIsSelected(moveToOtherViewAction, i18nc("@action:inmenu", "Move to Other View…"));
    moveToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Move",
                                               "This moves the selected items from "
                                               "the view in focus to the other view. "
                                               "(Only available while in Split View mode.)"));
    moveToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-cut")));
    moveToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Move to Other View"));
    actionCollection()->setDefaultShortcuts(moveToOtherViewAction, {QKeySequence(Qt::Key_F6), QKeySequence(Qt::ALT | Qt::Key_M)});
    connect(moveToOtherViewAction, &QAction::triggered, this, &DolphinMainWindow::moveToInactiveSplitView);

    QAction *focusOtherViewAction = actionCollection()->addAction(QStringLiteral("focus_inactive_split_view"));
    focusOtherViewAction->setText(i18nc("@action:inmenu", "Focus Other View"));
    focusOtherViewAction->setToolTip(i18nc("@info:tooltip", "Move keyboard focus to the inactive view."));
    focusOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("swap-panels")));
    actionCollection()->setDefaultShortcut(focusOtherViewAction, Qt::CTRL | Qt::Key_F3);
    connect(focusOtherViewAction, &QAction::triggered, this, [this]() {
        m_sidesSplitter->showBothSides();
        activateSide(inactiveTabWidget());
    });

    QAction *syncBrowsing = actionCollection()->addAction(QStringLiteral("sync_browsing"));
    syncBrowsing->setText(i18nc("@action:inmenu", "Synchronized Browsing"));
    syncBrowsing->setToolTip(i18nc("@info:tooltip", "Make the other side follow when you enter a folder or go up."));
    syncBrowsing->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    syncBrowsing->setCheckable(true);
    actionCollection()->setDefaultShortcut(syncBrowsing, Qt::CTRL | Qt::ALT | Qt::Key_S);

    QAction *compareDirectories = actionCollection()->addAction(QStringLiteral("compare_directories"));
    compareDirectories->setText(i18nc("@action:inmenu", "Compare Folders"));
    compareDirectories->setToolTip(i18nc("@info:tooltip", "Select what differs between the two sides."));
    compareDirectories->setWhatsThis(xi18nc("@info:whatsthis",
                                            "This selects the items that only one side has, and of the items "
                                            "that both sides have the newer one if they differ in modification time "
                                            "(both if only their size differs). Folders are compared by name only."));
    compareDirectories->setIcon(QIcon::fromTheme(QStringLiteral("document-edit-decrypt")));
    actionCollection()->setDefaultShortcut(compareDirectories, Qt::CTRL | Qt::SHIFT | Qt::Key_F2);
    connect(compareDirectories, &QAction::triggered, this, &DolphinMainWindow::compareDirectories);

    QAction *hotlist = actionCollection()->addAction(QStringLiteral("folder_hotlist"));
    hotlist->setText(i18nc("@action:inmenu", "Folder Hotlist"));
    hotlist->setToolTip(i18nc("@info:tooltip", "Pick a place to go to from a list."));
    hotlist->setIcon(QIcon::fromTheme(QStringLiteral("bookmarks")));
    actionCollection()->setDefaultShortcut(hotlist, Qt::CTRL | Qt::SHIFT | Qt::Key_F8);
    connect(hotlist, &QAction::triggered, this, &DolphinMainWindow::showFolderHotlist);

    QAction *checksum = actionCollection()->addAction(QStringLiteral("show_checksum"));
    checksum->setText(i18nc("@action:inmenu", "Checksum…"));
    checksum->setToolTip(i18nc("@info:tooltip", "Calculate the checksums of the selected file and check them against one."));
    checksum->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
    actionCollection()->setDefaultShortcut(checksum, Qt::CTRL | Qt::SHIFT | Qt::Key_F7);
    connect(checksum, &QAction::triggered, this, &DolphinMainWindow::showChecksum);

    QAction *selectPattern = actionCollection()->addAction(QStringLiteral("select_by_pattern"));
    selectPattern->setText(i18nc("@action:inmenu", "Select by Pattern…"));
    selectPattern->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-text")));
    actionCollection()->setDefaultShortcut(selectPattern, Qt::ALT | Qt::Key_Plus);
    connect(selectPattern, &QAction::triggered, this, [this]() {
        selectByPattern(true);
    });
    QAction *deselectPattern = actionCollection()->addAction(QStringLiteral("deselect_by_pattern"));
    deselectPattern->setText(i18nc("@action:inmenu", "Deselect by Pattern…"));
    deselectPattern->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-none")));
    actionCollection()->setDefaultShortcut(deselectPattern, Qt::ALT | Qt::Key_Minus);
    connect(deselectPattern, &QAction::triggered, this, [this]() {
        selectByPattern(false);
    });
    QAction *mirror = actionCollection()->addAction(QStringLiteral("mirror_to_other_side"));
    mirror->setText(i18nc("@action:inmenu", "Open This Folder on the Other Side"));
    mirror->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    actionCollection()->setDefaultShortcut(mirror, Qt::CTRL | Qt::ALT | Qt::Key_M);
    connect(mirror, &QAction::triggered, this, &DolphinMainWindow::mirrorToOtherSide);
    QAction *swap = actionCollection()->addAction(QStringLiteral("swap_sides"));
    swap->setText(i18nc("@action:inmenu", "Swap the Folders of the Two Sides"));
    swap->setIcon(QIcon::fromTheme(QStringLiteral("exchange-positions")));
    actionCollection()->setDefaultShortcut(swap, Qt::CTRL | Qt::ALT | Qt::Key_X);
    connect(swap, &QAction::triggered, this, &DolphinMainWindow::swapSides);

    auto *labelMenu = actionCollection()->add<KActionMenu>(QStringLiteral("color_label"));
    labelMenu->setText(i18nc("@action:inmenu", "Color Label"));
    labelMenu->setIcon(QIcon::fromTheme(QStringLiteral("tag")));
    const struct {
        const char *name;
        const char *label;
    } labelColors[] = {{"red", "Red"}, {"orange", "Orange"}, {"yellow", "Yellow"}, {"green", "Green"}, {"blue", "Blue"}, {"purple", "Purple"}};
    for (const auto &entry : labelColors) {
        const QString name = QString::fromLatin1(entry.name);
        QPixmap swatch(16, 16);
        swatch.fill(QColor(name));
        auto *colorAction = new QAction(QIcon(swatch), i18nc("@action:inmenu color label", entry.label), labelMenu);
        labelMenu->addAction(colorAction);
        connect(colorAction, &QAction::triggered, this, [this, name]() {
            setColorLabel(name);
        });
    }
    labelMenu->addSeparator();
    auto *noLabel = new QAction(i18nc("@action:inmenu", "No Color Label"), labelMenu);
    labelMenu->addAction(noLabel);
    connect(noLabel, &QAction::triggered, this, [this]() {
        setColorLabel(QString());
    });

    QAction *syncFolders = actionCollection()->addAction(QStringLiteral("sync_to_other_side"));
    syncFolders->setText(i18nc("@action:inmenu", "Update the Other Side"));
    syncFolders->setToolTip(i18nc("@info:tooltip", "Copy what is missing or older on the other side from this side."));
    syncFolders->setIcon(QIcon::fromTheme(QStringLiteral("folder-sync")));
    actionCollection()->setDefaultShortcut(syncFolders, Qt::CTRL | Qt::ALT | Qt::Key_U);
    connect(syncFolders, &QAction::triggered, this, &DolphinMainWindow::syncToOtherSide);

    QAction *folderSizes = actionCollection()->addAction(QStringLiteral("folder_sizes"));
    folderSizes->setText(i18nc("@action:inmenu", "Folder Sizes…"));
    folderSizes->setToolTip(i18nc("@info:tooltip", "Work out how big the selected folders are, or all folders shown."));
    folderSizes->setIcon(QIcon::fromTheme(QStringLiteral("drive-harddisk")));
    connect(folderSizes, &QAction::triggered, this, &DolphinMainWindow::showFolderSizes);

    QAction *convert = actionCollection()->addAction(QStringLiteral("convert_images"));
    convert->setText(i18nc("@action:inmenu", "Convert Images…"));
    convert->setIcon(QIcon::fromTheme(QStringLiteral("viewimage")));
    connect(convert, &QAction::triggered, this, &DolphinMainWindow::convertImages);

    QAction *openWith = actionCollection()->addAction(QStringLiteral("open_with_dialog"));
    openWith->setText(i18nc("@action:inmenu", "Open With…"));
    openWith->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    actionCollection()->setDefaultShortcut(openWith, Qt::CTRL | Qt::Key_Return);
    connect(openWith, &QAction::triggered, this, &DolphinMainWindow::openWithDialog);

    QAction *newToDoAction = actionCollection()->addAction(QStringLiteral("new_todo"));
    newToDoAction->setText(i18nc("@action:inmenu", "New To-Do…"));
    newToDoAction->setIcon(QIcon::fromTheme(QStringLiteral("view-task")));
    connect(newToDoAction, &QAction::triggered, this, &DolphinMainWindow::newToDo);
    QAction *toggleToDo = actionCollection()->addAction(QStringLiteral("toggle_todo_done"));
    toggleToDo->setText(i18nc("@action:inmenu", "Mark To-Do Done or Open"));
    toggleToDo->setIcon(QIcon::fromTheme(QStringLiteral("checkbox")));
    connect(toggleToDo, &QAction::triggered, this, &DolphinMainWindow::toggleToDoDone);

    QAction *notifications = actionCollection()->addAction(QStringLiteral("recent_notifications"));
    notifications->setText(i18nc("@action:inmenu", "Recent Notifications"));
    notifications->setIcon(QIcon::fromTheme(QStringLiteral("notifications")));
    // OneCommander shows its notifications with ~, a key that type-to-filter needs, so Ctrl is added.
    actionCollection()->setDefaultShortcuts(notifications, {QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_N)});
    connect(notifications, &QAction::triggered, this, &DolphinMainWindow::showRecentNotifications);

    QAction *ageStripes = actionCollection()->addAction(QStringLiteral("age_stripes"));
    ageStripes->setText(i18nc("@action:inmenu", "Color Rows by Age"));
    ageStripes->setToolTip(i18nc("@info:tooltip", "Show a stripe at the left of each row: red for what changed today, blue for what is a year old."));
    ageStripes->setIcon(QIcon::fromTheme(QStringLiteral("color-management")));
    ageStripes->setCheckable(true);
    const bool ageOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("AgeStripes", false);
    ageStripes->setChecked(ageOn);
    KItemListWidget::setAgeStripesShown(ageOn);
    connect(ageStripes, &QAction::toggled, this, [this](bool on) {
        KItemListWidget::setAgeStripesShown(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("AgeStripes", on);
        for (QWidget *w : findChildren<QWidget *>()) {
            w->update();
        }
    });

    QAction *compactRows = actionCollection()->addAction(QStringLiteral("compact_rows"));
    compactRows->setText(i18nc("@action:inmenu", "Compact Rows"));
    compactRows->setToolTip(
        i18nc("@info:tooltip", "Make the rows of the lists as low as they go, as OneCommander's are (the zoom of the views is set to its smallest)."));
    compactRows->setIcon(QIcon::fromTheme(QStringLiteral("zoom-out")));
    connect(compactRows, &QAction::triggered, this, [this]() {
        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            for (int i = 0; i < tabs->count(); ++i) {
                for (DolphinViewContainer *container : {tabs->tabPageAt(i)->primaryViewContainer(), tabs->tabPageAt(i)->secondaryViewContainer()}) {
                    if (container) {
                        container->view()->setZoomLevel(ZoomLevelInfo::minimumLevel());
                    }
                }
            }
        }
    });

    QAction *badges = actionCollection()->addAction(QStringLiteral("colored_badges"));
    badges->setText(i18nc("@action:inmenu", "Colored Badges for Dates and Sizes"));
    badges->setToolTip(i18nc("@info:tooltip", "Draw the dates on a badge by age (red today, blue a year ago) and the sizes on one from green to red."));
    badges->setIcon(QIcon::fromTheme(QStringLiteral("color-management")));
    badges->setCheckable(true);
    const bool badgesOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ColoredBadges", true);
    badges->setChecked(badgesOn);
    KItemListWidget::setBadgesShown(badgesOn);
    connect(badges, &QAction::toggled, this, [this](bool on) {
        KItemListWidget::setBadgesShown(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ColoredBadges", on);
        for (QWidget *w : findChildren<QWidget *>()) {
            w->update();
        }
    });

    auto *layoutMenu = actionCollection()->add<KActionMenu>(QStringLiteral("commander_layouts"));
    layoutMenu->setText(i18nc("@action:inmenu", "Layout"));
    layoutMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    const struct {
        const char *label;
        CommanderLayout layout;
    } layouts[] = {{"Standard: Two Sides", CommanderLayout::Standard},
                   {"Columns: Miller Columns", CommanderLayout::Columns},
                   {"Classic: One Side with Information", CommanderLayout::Classic}};
    for (const auto &entry : layouts) {
        auto *layoutAction = new QAction(i18nc("@action:inmenu", entry.label), layoutMenu);
        layoutMenu->addAction(layoutAction);
        const CommanderLayout chosen = entry.layout;
        connect(layoutAction, &QAction::triggered, this, [this, chosen]() {
            setCommanderLayout(chosen);
        });
    }

    QAction *cycleLayout = actionCollection()->addAction(QStringLiteral("cycle_layout"));
    cycleLayout->setText(i18nc("@action:inmenu", "Change Layout"));
    cycleLayout->setToolTip(i18nc("@info:tooltip", "Switch to the next layout: Standard, Columns, Classic."));
    cycleLayout->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    connect(cycleLayout, &QAction::triggered, this, [this]() {
        setCommanderLayout(CommanderLayout((int(m_commanderLayout) + 1) % 3));
    });

    auto *designFileSave = actionCollection()->addAction(QStringLiteral("export_view_design"));
    designFileSave->setText(i18nc("@action:inmenu", "Save View Design to File…"));
    connect(designFileSave, &QAction::triggered, this, [this]() {
        const QString path =
            QFileDialog::getSaveFileName(this, i18nc("@title:window", "Save View Design"), QString(), i18nc("@info file type filter", "View designs (*.json)"));
        if (!path.isEmpty() && !exportViewDesign(path)) {
            m_activeViewContainer->showMessage(i18nc("@info", "The design could not be saved."), KMessageWidget::Error);
        }
    });
    auto *designFileLoad = actionCollection()->addAction(QStringLiteral("import_view_design"));
    designFileLoad->setText(i18nc("@action:inmenu", "Apply View Design from File…"));
    connect(designFileLoad, &QAction::triggered, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this,
                                                          i18nc("@title:window", "Apply View Design"),
                                                          QString(),
                                                          i18nc("@info file type filter", "View designs (*.json)"));
        if (!path.isEmpty() && !importViewDesign(path)) {
            m_activeViewContainer->showMessage(i18nc("@info", "That file is not a view design."), KMessageWidget::Error);
        }
    });

    auto *fileViewsMenu = actionCollection()->add<KActionMenu>(QStringLiteral("file_views"));
    fileViewsMenu->setText(i18nc("@action:inmenu", "File Views"));
    fileViewsMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details")));
    const struct {
        const char *id;
        const char *label;
    } fileViews[] = {{"name-only", "Name Only"},
                     {"details", "Details"},
                     {"thumbnail-grid", "Thumbnail Grid"},
                     {"thumbs-details", "Thumbs + Details"},
                     {"rich", "Rich (Text and Pictures)"}};
    for (const auto &entry : fileViews) {
        auto *fileViewAction = new QAction(i18nc("@action:inmenu", entry.label), fileViewsMenu);
        fileViewsMenu->addAction(fileViewAction);
        const QString id = QString::fromLatin1(entry.id);
        connect(fileViewAction, &QAction::triggered, this, [this, id]() {
            applyFileView(id);
        });
    }

    {
        auto *saveViewAction = actionCollection()->addAction(QStringLiteral("save_file_view"));
        saveViewAction->setText(i18nc("@action:inmenu", "Save Current View As…"));
        connect(saveViewAction, &QAction::triggered, this, &DolphinMainWindow::saveFileView);
        auto *designViewAction = actionCollection()->addAction(QStringLiteral("design_file_view"));
        designViewAction->setText(i18nc("@action:inmenu", "Design This View…"));
        designViewAction->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
        connect(designViewAction, &QAction::triggered, this, &DolphinMainWindow::designFileView);
        fileViewsMenu->addSeparator();
        fileViewsMenu->addAction(designViewAction);
        fileViewsMenu->addAction(actionCollection()->action(QStringLiteral("export_view_design")));
        fileViewsMenu->addAction(actionCollection()->action(QStringLiteral("import_view_design")));
        fileViewsMenu->addAction(saveViewAction);
        // The saved views are listed each time the menu opens.
        auto *savedSeparator = fileViewsMenu->addSeparator();
        connect(fileViewsMenu->menu(), &QMenu::aboutToShow, this, [this, fileViewsMenu, savedSeparator]() {
            const QList<QAction *> all = fileViewsMenu->menu()->actions();
            const int from = all.indexOf(savedSeparator);
            for (int i = all.size() - 1; i > from; --i) {
                fileViewsMenu->menu()->removeAction(all.at(i));
                all.at(i)->deleteLater();
            }
            const KConfigGroup saved(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews"));
            for (const QString &savedName : saved.keyList()) {
                connect(fileViewsMenu->menu()->addAction(savedName), &QAction::triggered, this, [this, savedName]() {
                    applyFileView(QStringLiteral("saved:") + savedName);
                });
            }
        });
    }

    // More of OneCommander's keys, added next to those the actions already have (a key that is taken is left alone).
    const struct {
        const char *action;
        QKeySequence key;
    } extraKeys[] = {{"duplicate", QKeySequence(Qt::CTRL | Qt::Key_D)},
                     {"create_dir", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N)},
                     {"focus_inactive_split_view", QKeySequence(Qt::ALT | Qt::Key_F)},
                     {"color_label", QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T)}};
    for (const auto &extra : extraKeys) {
        QAction *target = actionCollection()->action(QString::fromLatin1(extra.action));
        if (!target) {
            continue;
        }
        bool taken = false;
        for (QAction *other : actionCollection()->actions()) {
            taken = taken || other->shortcuts().contains(extra.key);
        }
        if (!taken) {
            actionCollection()->setDefaultShortcuts(target, target->shortcuts() + QList<QKeySequence>{extra.key});
        }
    }

    QAction *pasteAsFile = actionCollection()->addAction(QStringLiteral("paste_clipboard_as_file"));
    pasteAsFile->setText(i18nc("@action:inmenu", "Paste Clipboard as File"));
    pasteAsFile->setToolTip(i18nc("@info:tooltip", "Make a dated .png of a copied image, or a dated .txt of copied text."));
    pasteAsFile->setIcon(QIcon::fromTheme(QStringLiteral("edit-paste")));
    actionCollection()->setDefaultShortcut(pasteAsFile, Qt::CTRL | Qt::ALT | Qt::Key_V);
    connect(pasteAsFile, &QAction::triggered, this, &DolphinMainWindow::pasteClipboardAsFile);

    QAction *tabFromClipboard = actionCollection()->addAction(QStringLiteral("new_tab_from_clipboard"));
    tabFromClipboard->setText(i18nc("@action:inmenu", "New Tab from Clipboard Path"));
    tabFromClipboard->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    actionCollection()->setDefaultShortcut(tabFromClipboard, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_T);
    connect(tabFromClipboard, &QAction::triggered, this, &DolphinMainWindow::newTabFromClipboard);

    QAction *runWithArgs = actionCollection()->addAction(QStringLiteral("run_with_arguments"));
    runWithArgs->setText(i18nc("@action:inmenu", "Run with Arguments…"));
    runWithArgs->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    actionCollection()->setDefaultShortcut(runWithArgs, Qt::CTRL | Qt::SHIFT | Qt::Key_Return);
    connect(runWithArgs, &QAction::triggered, this, &DolphinMainWindow::runWithArguments);

    QAction *queueJobs = actionCollection()->addAction(QStringLiteral("queue_jobs"));
    queueJobs->setText(i18nc("@action:inmenu", "Queue Copy and Move Jobs"));
    queueJobs->setToolTip(i18nc("@info:tooltip", "Run copies, moves and pastes one after another instead of all at once."));
    queueJobs->setIcon(QIcon::fromTheme(QStringLiteral("view-process-all")));
    queueJobs->setCheckable(true);
    const bool queueOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("QueueJobs", true);
    queueJobs->setChecked(queueOn);
    JobQueue::setEnabled(queueOn);
    connect(queueJobs, &QAction::toggled, this, [](bool on) {
        JobQueue::setEnabled(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("QueueJobs", on);
    });

    QAction *detailsPane = actionCollection()->addAction(QStringLiteral("show_details_pane"));
    detailsPane->setText(i18nc("@action:inmenu", "Details Pane"));
    detailsPane->setToolTip(i18nc("@info:tooltip", "Show a picture and the details of the selected file under each list."));
    detailsPane->setIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
    detailsPane->setCheckable(true);
    detailsPane->setChecked(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ShowDetailsPane", true));
    connect(detailsPane, &QAction::toggled, this, [this](bool on) {
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ShowDetailsPane", on);
        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            for (int i = 0; i < tabs->count(); ++i) {
                for (DolphinViewContainer *container : {tabs->tabPageAt(i)->primaryViewContainer(), tabs->tabPageAt(i)->secondaryViewContainer()}) {
                    if (container) {
                        container->setDetailsPaneShown(on);
                    }
                }
            }
        }
    });

    QAction *copySelectedPath = actionCollection()->addAction(QStringLiteral("copy_selected_path"));
    copySelectedPath->setText(i18nc("@action:inmenu", "Copy Path of the Selected File"));
    copySelectedPath->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    actionCollection()->setDefaultShortcut(copySelectedPath, Qt::CTRL | Qt::SHIFT | Qt::Key_C);
    connect(copySelectedPath, &QAction::triggered, this, [this]() {
        QStringList paths;
        for (const KFileItem &item : m_activeViewContainer->view()->selectedItems()) {
            paths << (item.isLocalFile() ? item.localPath() : item.url().toDisplayString());
        }
        if (!paths.isEmpty()) {
            QApplication::clipboard()->setText(paths.join(QLatin1Char('\n')));
        }
    });

    QAction *newFromTemplate = actionCollection()->addAction(QStringLiteral("new_file_from_template"));
    newFromTemplate->setText(i18nc("@action:inmenu", "New File from Template…"));
    newFromTemplate->setToolTip(i18nc("@info:tooltip", "Copy a file from the templates folder into this folder, under the name you give it."));
    newFromTemplate->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    actionCollection()->setDefaultShortcut(newFromTemplate, Qt::CTRL | Qt::Key_N);
    connect(newFromTemplate, &QAction::triggered, this, [this]() {
        const QUrl folder = m_activeViewContainer->url();
        if (!folder.isLocalFile()) {
            m_activeViewContainer->showMessage(i18nc("@info", "Files can only be made from templates in folders on this computer."),
                                               KMessageWidget::Information);
            return;
        }
        NewFromTemplateDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        if (dialog.editRequested()) {
            changeUrl(QUrl::fromLocalFile(CommanderTools::templatesFolder()));
            return;
        }
        const QString path = CommanderTools::createFromTemplate(dialog.selectedTemplate(), folder.toLocalFile(), dialog.fileName());
        if (path.isEmpty()) {
            m_activeViewContainer->showMessage(i18nc("@info", "The file could not be made: the name is empty or already used."), KMessageWidget::Error);
            return;
        }
        m_activeViewContainer->view()->markUrlsAsSelected({QUrl::fromLocalFile(path)});
        if (dialog.openAfterwards()) {
            auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(path), this);
            job->start();
        }
    });

    QAction *saveAsTemplate = actionCollection()->addAction(QStringLiteral("save_as_template"));
    saveAsTemplate->setText(i18nc("@action:inmenu", "Save as File Template"));
    saveAsTemplate->setToolTip(i18nc("@info:tooltip", "Copy the selected file into the templates folder, to make new files from it."));
    saveAsTemplate->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
    connect(saveAsTemplate, &QAction::triggered, this, [this]() {
        int saved = 0;
        for (const KFileItem &item : m_activeViewContainer->view()->selectedItems()) {
            saved += CommanderTools::saveAsTemplate(item.localPath()).isEmpty() ? 0 : 1;
        }
        m_activeViewContainer->showMessage(saved > 0 ? i18ncp("@info", "%1 file is now a template.", "%1 files are now templates.", saved)
                                                     : i18nc("@info", "Select a file on this computer to make a template of it."),
                                           saved > 0 ? KMessageWidget::Positive : KMessageWidget::Information);
    });

    auto *copyPathMenu = actionCollection()->add<KActionMenu>(QStringLiteral("copy_path_as"));
    copyPathMenu->setText(i18nc("@action:inmenu", "Copy Path of This Folder As"));
    copyPathMenu->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    const struct {
        const char *label;
        CommanderTools::PathFormat format;
    } pathFormats[] = {{"Plain", CommanderTools::PathFormat::Plain},
                       {"In Quotes", CommanderTools::PathFormat::Quoted},
                       {"Escaped for the Shell", CommanderTools::PathFormat::ShellEscaped},
                       {"As a file:// Address", CommanderTools::PathFormat::FileUrl}};
    for (const auto &entry : pathFormats) {
        auto *formatAction = new QAction(i18nc("@action:inmenu", entry.label), copyPathMenu);
        copyPathMenu->addAction(formatAction);
        const CommanderTools::PathFormat format = entry.format;
        connect(formatAction, &QAction::triggered, this, [this, format]() {
            const QUrl url = m_activeViewContainer->url();
            const QString path = url.isLocalFile() ? url.toLocalFile() : url.toDisplayString();
            QApplication::clipboard()->setText(CommanderTools::formattedPath(path, format));
        });
    }
    // OneCommander's Ctrl+Shift+Alt+C copies the path of the folder at once.
    actionCollection()->setDefaultShortcut(copyPathMenu, Qt::CTRL | Qt::ALT | Qt::SHIFT | Qt::Key_C);
    connect(copyPathMenu, &QAction::triggered, this, [this]() {
        const QUrl url = m_activeViewContainer->url();
        QApplication::clipboard()->setText(url.isLocalFile() ? url.toLocalFile() : url.toDisplayString());
    });

    QAction *lockColumns = actionCollection()->addAction(QStringLiteral("lock_column_resizing"));
    lockColumns->setText(i18nc("@action:inmenu", "Lock Column Resizing"));
    lockColumns->setToolTip(i18nc("@info:tooltip", "Stop the dividers between the Miller columns from being dragged."));
    lockColumns->setIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    lockColumns->setCheckable(true);
    actionCollection()->setDefaultShortcut(lockColumns, Qt::Key_F8);
    connect(lockColumns, &QAction::toggled, this, [](bool on) {
        ColumnsStrip::setResizingLocked(on);
    });

    QAction *terminalAsAdmin = actionCollection()->addAction(QStringLiteral("open_terminal_as_administrator"));
    terminalAsAdmin->setText(i18nc("@action:inmenu", "Open Terminal as Administrator"));
    terminalAsAdmin->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
    actionCollection()->setDefaultShortcut(terminalAsAdmin, Qt::CTRL | Qt::Key_F10);
    connect(terminalAsAdmin, &QAction::triggered, this, [this]() {
        const QString dir = m_activeViewContainer->url().toLocalFile();
        runAsAdministrator(QStringLiteral("konsole"), dir.isEmpty() ? QStringList() : QStringList{QStringLiteral("--workdir"), dir});
    });

    QAction *restartAsAdmin = actionCollection()->addAction(QStringLiteral("restart_as_administrator"));
    restartAsAdmin->setText(i18nc("@action:inmenu", "Open a Copy of This Window as Administrator"));
    restartAsAdmin->setToolTip(i18nc("@info:tooltip", "Starts another dolphinCommander as root for this folder; this window stays open."));
    restartAsAdmin->setIcon(QIcon::fromTheme(QStringLiteral("dialog-password")));
    actionCollection()->setDefaultShortcut(restartAsAdmin, Qt::CTRL | Qt::SHIFT | Qt::Key_F12);
    connect(restartAsAdmin, &QAction::triggered, this, [this]() {
        runAsAdministrator(QCoreApplication::applicationFilePath(), {m_activeViewContainer->url().toString()});
    });

    QAction *toggleColumns = actionCollection()->addAction(QStringLiteral("toggle_columns_view"));
    toggleColumns->setText(i18nc("@action:inmenu", "Toggle Miller Columns"));
    toggleColumns->setToolTip(i18nc("@info:tooltip", "Switch between the Miller columns and the details view."));
    toggleColumns->setIcon(QIcon::fromTheme(QStringLiteral("view-file-columns")));
    actionCollection()->setDefaultShortcut(toggleColumns, Qt::CTRL | Qt::Key_QuoteLeft);
    connect(toggleColumns, &QAction::triggered, this, [this]() {
        const bool inColumns = m_activeViewContainer->view()->viewMode() == DolphinView::ColumnsView;
        if (QAction *target = actionCollection()->action(inColumns ? QStringLiteral("details") : QStringLiteral("columns"))) {
            target->trigger();
        }
    });

    QAction *foldersByName = actionCollection()->addAction(QStringLiteral("folders_sorted_by_name"));
    foldersByName->setText(i18nc("@action:inmenu", "Always Sort Folders by Name"));
    foldersByName->setToolTip(i18nc("@info:tooltip", "Sort the folders by name, whatever the files are sorted by."));
    foldersByName->setIcon(QIcon::fromTheme(QStringLiteral("view-sort-ascending-name")));
    foldersByName->setCheckable(true);
    const bool byNameOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("FoldersSortedByName", false);
    foldersByName->setChecked(byNameOn);
    KFileItemModel::setFoldersAlwaysSortedByName(byNameOn);
    connect(foldersByName, &QAction::toggled, this, [this](bool on) {
        KFileItemModel::setFoldersAlwaysSortedByName(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("FoldersSortedByName", on);
        // The views sort again.
        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            for (int i = 0; i < tabs->count(); ++i) {
                tabs->tabPageAt(i)->refreshViews();
            }
        }
        m_activeViewContainer->view()->reload();
    });

    auto *themeMenu = actionCollection()->add<KActionMenu>(QStringLiteral("color_theme"));
    themeMenu->setText(i18nc("@action:inmenu", "Color Theme"));
    themeMenu->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")));
    auto *themeGroup = new QActionGroup(themeMenu);
    const struct {
        const char *id;
        const char *label;
    } themes[] = {{"system", "Follow the Desktop"}, {"light", "Light"}, {"dark", "Dark"}, {"nord", "Nord"}};
    const QString savedTheme = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ColorTheme", QStringLiteral("system"));
    for (const auto &entry : themes) {
        auto *themeAction = new QAction(i18nc("@action:inmenu", entry.label), themeGroup);
        themeAction->setCheckable(true);
        const QString id = QString::fromLatin1(entry.id);
        themeAction->setChecked(id == savedTheme);
        themeMenu->addAction(themeAction);
        connect(themeAction, &QAction::triggered, this, [this, id]() {
            applyColorTheme(id);
        });
    }
    themeMenu->addSeparator();
    auto *textAndIcons = new QAction(i18nc("@action:inmenu", "Text and Icons…"), themeMenu);
    themeMenu->addAction(textAndIcons);
    connect(textAndIcons, &QAction::triggered, this, &DolphinMainWindow::editTextAndIcons);
    {
        const KConfigGroup saved(KSharedConfig::openConfig(), QStringLiteral("Commander"));
        if (saved.readEntry("FontPointSize", 0) > 0 || !saved.readEntry("IconTheme", QString()).isEmpty()) {
            applyTextAndIcons(saved.readEntry("FontPointSize", 0), saved.readEntry("IconTheme", QString()));
        }
    }
    auto *saveTheme = new QAction(i18nc("@action:inmenu", "Save Theme to File…"), themeMenu);
    themeMenu->addAction(saveTheme);
    connect(saveTheme, &QAction::triggered, this, [this]() {
        const QString path =
            QFileDialog::getSaveFileName(this, i18nc("@title:window", "Save Theme"), QString(), i18nc("@info file type filter", "Theme files (*.theme)"));
        if (!path.isEmpty() && !exportTheme(path)) {
            m_activeViewContainer->showMessage(i18nc("@info", "The theme could not be saved."), KMessageWidget::Error);
        }
    });
    auto *loadTheme = new QAction(i18nc("@action:inmenu", "Load Theme from File…"), themeMenu);
    themeMenu->addAction(loadTheme);
    connect(loadTheme, &QAction::triggered, this, [this]() {
        const QString path =
            QFileDialog::getOpenFileName(this, i18nc("@title:window", "Load Theme"), QString(), i18nc("@info file type filter", "Theme files (*.theme)"));
        if (!path.isEmpty() && !importTheme(path)) {
            m_activeViewContainer->showMessage(i18nc("@info", "That file is not a theme."), KMessageWidget::Error);
        }
    });
    auto *styleSheetAction = new QAction(i18nc("@action:inmenu", "Style Sheet…"), themeMenu);
    themeMenu->addAction(styleSheetAction);
    connect(styleSheetAction, &QAction::triggered, this, &DolphinMainWindow::editStyleSheet);
    {
        const QString savedStyleSheet = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("StyleSheet", QString());
        if (!savedStyleSheet.isEmpty()) {
            qApp->setStyleSheet(savedStyleSheet);
        }
    }
    auto *customTheme = new QAction(i18nc("@action:inmenu", "Custom Colors…"), themeMenu);
    themeMenu->addAction(customTheme);
    connect(customTheme, &QAction::triggered, this, &DolphinMainWindow::editCustomTheme);
    if (savedTheme != QLatin1String("system")) {
        applyColorTheme(savedTheme);
    }

    QAction *itemScrolling = actionCollection()->addAction(QStringLiteral("item_scrolling"));
    itemScrolling->setText(i18nc("@action:inmenu", "Scroll by Whole Items"));
    itemScrolling->setToolTip(i18nc("@info:tooltip", "Make a step of the mouse wheel move by the height of a row."));
    itemScrolling->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    itemScrolling->setCheckable(true);
    const bool itemScrollingOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ItemScrolling", false);
    itemScrolling->setChecked(itemScrollingOn);
    KItemListView::setItemScrolling(itemScrollingOn);
    connect(itemScrolling, &QAction::toggled, this, [this](bool on) {
        KItemListView::setItemScrolling(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ItemScrolling", on);
        // The scroll bars take their step again.
        for (QWidget *w : findChildren<KItemListContainer *>()) {
            w->updateGeometry();
        }
    });

    QAction *hideNarrow = actionCollection()->addAction(QStringLiteral("hide_columns_when_narrow"));
    hideNarrow->setText(i18nc("@action:inmenu", "Hide Columns When Narrow"));
    hideNarrow->setToolTip(i18nc("@info:tooltip", "Show only the columns that fit in the details view, the last ones go first."));
    hideNarrow->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details")));
    hideNarrow->setCheckable(true);
    {
        QHash<QByteArray, int> thresholds;
        for (const QString &entry : KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ColumnHideThresholds", QStringList())) {
            const int equals = entry.lastIndexOf(QLatin1Char('='));
            if (equals > 0) {
                thresholds.insert(entry.left(equals).toLatin1(), entry.mid(equals + 1).toInt());
            }
        }
        DolphinView::setColumnHideThresholds(thresholds);
    }
    const bool hideNarrowOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("HideColumnsWhenNarrow", false);
    hideNarrow->setChecked(hideNarrowOn);
    DolphinView::setHideColumnsWhenNarrow(hideNarrowOn);
    connect(hideNarrow, &QAction::toggled, this, [this](bool on) {
        DolphinView::setHideColumnsWhenNarrow(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("HideColumnsWhenNarrow", on);
        // Every view looks at its width again.
        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            for (int i = 0; i < tabs->count(); ++i) {
                for (DolphinViewContainer *container : {tabs->tabPageAt(i)->primaryViewContainer(), tabs->tabPageAt(i)->secondaryViewContainer()}) {
                    if (container) {
                        container->view()->updateResponsiveColumns();
                    }
                }
            }
        }
    });

    // A key that works from any program, to bring the window back (Meta+Alt+E, as OneCommander's Win+Alt+E).
    QAction *bringBackAction = actionCollection()->addAction(QStringLiteral("bring_window_back"));
    bringBackAction->setText(i18nc("@action:inmenu", "Bring the Window Back"));
    bringBackAction->setIcon(QIcon::fromTheme(QStringLiteral("window-restore")));
    KGlobalAccel::self()->setDefaultShortcut(bringBackAction, {QKeySequence(Qt::META | Qt::ALT | Qt::Key_E)});
    KGlobalAccel::self()->setShortcut(bringBackAction, {QKeySequence(Qt::META | Qt::ALT | Qt::Key_E)});
    connect(bringBackAction, &QAction::triggered, this, &DolphinMainWindow::bringBack);

    QAction *dockAction = actionCollection()->addAction(QStringLiteral("dock_window"));
    dockAction->setText(i18nc("@action:inmenu", "Show the Window in the Lower Half of the Screen"));
    dockAction->setToolTip(i18nc("@info:tooltip", "From any program: brings the window to the lower half of the screen; once more, it is minimized."));
    dockAction->setIcon(QIcon::fromTheme(QStringLiteral("window-duplicate")));
    KGlobalAccel::self()->setDefaultShortcut(dockAction, {QKeySequence(Qt::META | Qt::ALT | Qt::Key_QuoteLeft)});
    KGlobalAccel::self()->setShortcut(dockAction, {QKeySequence(Qt::META | Qt::ALT | Qt::Key_QuoteLeft)});
    connect(dockAction, &QAction::triggered, this, &DolphinMainWindow::dockToBottomHalf);

    QAction *autoThumbs = actionCollection()->addAction(QStringLiteral("auto_thumbs_details"));
    autoThumbs->setText(i18nc("@action:inmenu", "Thumbnails for Folders of Pictures"));
    autoThumbs->setToolTip(i18nc("@info:tooltip", "A folder of mostly pictures that has no style of its own yet is shown with thumbnails when it is opened."));
    autoThumbs->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    autoThumbs->setCheckable(true);
    const bool autoThumbsOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("AutoThumbsDetails", false);
    autoThumbs->setChecked(autoThumbsOn);
    DolphinViewContainer::setAutoThumbsDetails(autoThumbsOn);
    connect(autoThumbs, &QAction::toggled, this, [](bool on) {
        DolphinViewContainer::setAutoThumbsDetails(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("AutoThumbsDetails", on);
    });

    QAction *defaultManager = actionCollection()->addAction(QStringLiteral("make_default_file_manager"));
    defaultManager->setText(i18nc("@action:inmenu", "Make This the Default File Manager"));
    defaultManager->setToolTip(i18nc("@info:tooltip", "Open folders from other programs in dolphinCommander (sets the handler of inode/directory)."));
    defaultManager->setIcon(QIcon::fromTheme(QStringLiteral("system-file-manager")));
    connect(defaultManager, &QAction::triggered, this, [this]() {
        const bool started =
            QProcess::execute(QStringLiteral("xdg-mime"),
                              {QStringLiteral("default"), QStringLiteral("org.kde.dolphincommander.desktop"), QStringLiteral("inode/directory")})
            == 0;
        m_activeViewContainer->showMessage(started ? i18nc("@info", "dolphinCommander now opens the folders that other programs ask for.")
                                                   : i18nc("@info", "The default file manager could not be set (xdg-mime is needed)."),
                                           started ? KMessageWidget::Positive : KMessageWidget::Error);
    });

    QAction *thinScrollbars = actionCollection()->addAction(QStringLiteral("thin_scrollbars"));
    thinScrollbars->setText(i18nc("@action:inmenu", "Thin Scrollbars"));
    thinScrollbars->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details")));
    thinScrollbars->setCheckable(true);
    thinScrollbars->setChecked(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ThinScrollbars", false));
    connect(thinScrollbars, &QAction::toggled, this, [this](bool on) {
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("ThinScrollbars", on);
        applyStyleSheet(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("StyleSheet", QString()));
    });

    QAction *keepRunning = actionCollection()->addAction(QStringLiteral("keep_running_when_closed"));
    keepRunning->setText(i18nc("@action:inmenu", "Keep Running When the Window Is Closed"));
    keepRunning->setToolTip(i18nc("@info:tooltip", "Closing the window hides it and leaves an icon in the system tray; Quit closes it for good."));
    keepRunning->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    keepRunning->setCheckable(true);
    const bool keepRunningOn = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("KeepRunningWhenClosed", false);
    keepRunning->setChecked(keepRunningOn);
    setKeepRunningWhenClosed(keepRunningOn);
    connect(keepRunning, &QAction::toggled, this, [](bool on) {
        setKeepRunningWhenClosed(on);
        KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).writeEntry("KeepRunningWhenClosed", on);
    });

    QAction *namesOnly = actionCollection()->addAction(QStringLiteral("names_only"));
    namesOnly->setText(i18nc("@action:inmenu", "Show Only the Names"));
    namesOnly->setToolTip(i18nc("@info:tooltip", "Hide the other columns so that long names are shown in full."));
    namesOnly->setIcon(QIcon::fromTheme(QStringLiteral("view-list-text")));
    namesOnly->setCheckable(true);
    actionCollection()->setDefaultShortcut(namesOnly, Qt::Key_F7);
    connect(namesOnly, &QAction::toggled, this, [this](bool on) {
        DolphinView::setNamesOnly(on);
        for (DolphinTabWidget *tabs : {m_leftTabs, m_rightTabs}) {
            for (int i = 0; i < tabs->count(); ++i) {
                for (DolphinViewContainer *container : {tabs->tabPageAt(i)->primaryViewContainer(), tabs->tabPageAt(i)->secondaryViewContainer()}) {
                    if (container) {
                        container->view()->updateResponsiveColumns();
                    }
                }
            }
        }
    });

    QAction *centerWindow = actionCollection()->addAction(QStringLiteral("center_window"));
    centerWindow->setText(i18nc("@action:inmenu", "Center the Window on the Main Screen"));
    centerWindow->setIcon(QIcon::fromTheme(QStringLiteral("zoom-fit-best")));
    actionCollection()->setDefaultShortcut(centerWindow, Qt::CTRL | Qt::SHIFT | Qt::Key_F1);
    connect(centerWindow, &QAction::triggered, this, &DolphinMainWindow::centerOnMainScreen);
    QAction *reloadThemeAction = actionCollection()->addAction(QStringLiteral("reload_theme"));
    reloadThemeAction->setText(i18nc("@action:inmenu", "Reload the Theme"));
    reloadThemeAction->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    actionCollection()->setDefaultShortcut(reloadThemeAction, Qt::CTRL | Qt::SHIFT | Qt::Key_F5);
    connect(reloadThemeAction, &QAction::triggered, this, &DolphinMainWindow::reloadTheme);

    QAction *note = actionCollection()->addAction(QStringLiteral("new_note"));
    note->setText(i18nc("@action:inmenu", "New Note"));
    note->setToolTip(i18nc("@info:tooltip", "Make a text file named by the date in this folder."));
    note->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    actionCollection()->setDefaultShortcut(note, Qt::ALT | Qt::Key_N);
    connect(note, &QAction::triggered, this, &DolphinMainWindow::newNote);

    auto *scriptsMenu = actionCollection()->add<KActionMenu>(QStringLiteral("scripts_menu"));
    scriptsMenu->setText(i18nc("@action:inmenu", "Scripts"));
    scriptsMenu->setIcon(QIcon::fromTheme(QStringLiteral("system-run")));
    connect(scriptsMenu->menu(), &QMenu::aboutToShow, this, [this, scriptsMenu]() {
        fillScriptsMenu(scriptsMenu->menu());
    });

    QAction *duplicates = actionCollection()->addAction(QStringLiteral("find_duplicates"));
    duplicates->setText(i18nc("@action:inmenu", "Find Duplicate Files…"));
    duplicates->setToolTip(i18nc("@info:tooltip", "Find files with the same content in this folder and below."));
    duplicates->setIcon(QIcon::fromTheme(QStringLiteral("edit-duplicate")));
    connect(duplicates, &QAction::triggered, this, &DolphinMainWindow::showDuplicateFinder);

    QAction *batchRename = actionCollection()->addAction(QStringLiteral("batch_rename"));
    batchRename->setText(i18nc("@action:inmenu", "Rename by Pattern…"));
    batchRename->setToolTip(i18nc("@info:tooltip", "Rename the selected items by a pattern, with a preview."));
    batchRename->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    actionCollection()->setDefaultShortcut(batchRename, Qt::CTRL | Qt::SHIFT | Qt::Key_F6);
    connect(batchRename, &QAction::triggered, this, &DolphinMainWindow::showBatchRename);

    QAction *quickView = actionCollection()->addAction(QStringLiteral("quick_view"));
    quickView->setText(i18nc("@action:inmenu", "Quick View"));
    quickView->setToolTip(i18nc("@info:tooltip", "Look into the selected file, on the other side first."));
    quickView->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    actionCollection()->setDefaultShortcut(quickView, Qt::ALT | Qt::Key_F3);
    connect(quickView, &QAction::triggered, this, &DolphinMainWindow::showQuickView);

    QAction *packAction = actionCollection()->addAction(QStringLiteral("pack_selection"));
    packAction->setText(i18nc("@action:inmenu", "Pack…"));
    packAction->setToolTip(i18nc("@info:tooltip", "Pack the selected items into an archive with Ark."));
    packAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-insert")));
    actionCollection()->setDefaultShortcut(packAction, Qt::ALT | Qt::Key_F5);
    connect(packAction, &QAction::triggered, this, [this]() {
        runArk(false);
    });

    QAction *unpackAction = actionCollection()->addAction(QStringLiteral("unpack_to_other_side"));
    unpackAction->setText(i18nc("@action:inmenu", "Unpack to Other Side"));
    unpackAction->setToolTip(i18nc("@info:tooltip", "Unpack the selected archives into the folder on the other side."));
    unpackAction->setIcon(QIcon::fromTheme(QStringLiteral("archive-extract")));
    actionCollection()->setDefaultShortcut(unpackAction, Qt::ALT | Qt::Key_F9);
    connect(unpackAction, &QAction::triggered, this, [this]() {
        runArk(true);
    });

    auto *workspaces = new KActionMenu(QIcon::fromTheme(QStringLiteral("window-duplicate")), i18nc("@action:inmenu", "Workspaces"), this);
    actionCollection()->addAction(QStringLiteral("workspaces_menu"), workspaces);
    connect(workspaces->menu(), &QMenu::aboutToShow, this, [this, workspaces]() {
        fillWorkspacesMenu(workspaces->menu());
    });

    QAction *showFilterBar = actionCollection()->addAction(QStringLiteral("show_filter_bar"));
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Filter…"));
    showFilterBar->setToolTip(i18nc("@info:tooltip", "Show Filter Bar"));
    showFilterBar->setWhatsThis(xi18nc("@info:whatsthis",
                                       "This opens the "
                                       "<emphasis>Filter Bar</emphasis> at the bottom of the window.<nl/> "
                                       "There you can enter text to filter the files and folders currently displayed. "
                                       "Only those that contain the text in their name will be kept in view."));
    showFilterBar->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    actionCollection()->setDefaultShortcuts(showFilterBar, {Qt::CTRL | Qt::Key_I, Qt::Key_Slash});
    connect(showFilterBar, &QAction::triggered, this, &DolphinMainWindow::showFilterBar);

    // toggle_filter acts as a copy of the main showFilterBar to be used mainly
    // in the toolbar, with no default shortcut attached, to avoid messing with
    // existing workflows (filter bar always open and Ctrl-I to focus)
    QAction *toggleFilter = actionCollection()->addAction(QStringLiteral("toggle_filter"));
    toggleFilter->setText(i18nc("@action:inmenu", "Toggle Filter Bar"));
    toggleFilter->setIconText(i18nc("@action:intoolbar", "Filter"));
    toggleFilter->setIcon(showFilterBar->icon());
    toggleFilter->setToolTip(showFilterBar->toolTip());
    toggleFilter->setWhatsThis(showFilterBar->whatsThis());
    toggleFilter->setCheckable(true);
    connect(toggleFilter, &QAction::triggered, this, &DolphinMainWindow::toggleFilterBar);

    QAction *searchAction = KStandardAction::find(this, &DolphinMainWindow::find, actionCollection());
    searchAction->setText(i18n("Search…"));
    searchAction->setToolTip(i18nc("@info:tooltip", "Search for files and folders"));
    searchAction->setWhatsThis(xi18nc("@info:whatsthis find",
                                      "<para>This helps you "
                                      "find files and folders by opening a <emphasis>search bar</emphasis>. "
                                      "There you can enter search terms and specify settings to find the "
                                      "items you are looking for.</para>"));

    // toggle_search acts as a copy of the main searchAction to be used mainly
    // in the toolbar, with no default shortcut attached, to avoid messing with
    // existing workflows (search bar always open and Ctrl-F to focus)
    QAction *toggleSearchAction = actionCollection()->addAction(QStringLiteral("toggle_search"));
    toggleSearchAction->setText(i18nc("@action:inmenu", "Toggle Search Bar"));
    toggleSearchAction->setIconText(i18nc("@action:intoolbar", "Search"));
    toggleSearchAction->setIcon(searchAction->icon());
    toggleSearchAction->setToolTip(searchAction->toolTip());
    toggleSearchAction->setWhatsThis(searchAction->whatsThis());
    toggleSearchAction->setCheckable(true);
    connect(toggleSearchAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            find();
        } else {
            m_activeViewContainer->setSearchBarVisible(false);
        }
    });

    QAction *toggleSelectionModeAction = actionCollection()->addAction(QStringLiteral("toggle_selection_mode"));
    // i18n: This action toggles a selection mode.
    toggleSelectionModeAction->setText(i18nc("@action:inmenu", "Select Files and Folders"));
    // i18n: Opens a selection mode for selecting files/folders.
    // The text is kept so unspecific because it will be shown on the toolbar where space is at a premium.
    toggleSelectionModeAction->setIconText(i18nc("@action:intoolbar", "Select"));
    toggleSelectionModeAction->setWhatsThis(xi18nc(
        "@info:whatsthis",
        "<para>This application only knows which files or folders should be acted on if they are"
        " <emphasis>selected</emphasis> first. Press this to toggle a <emphasis>Selection Mode</emphasis> which makes selecting and deselecting as easy as "
        "pressing an item once.</para><para>While in this mode, a quick access bar at the bottom shows available actions for the currently selected items."
        "</para>"));
    toggleSelectionModeAction->setIcon(QIcon::fromTheme(QStringLiteral("quickwizard")));
    toggleSelectionModeAction->setCheckable(true);
    actionCollection()->setDefaultShortcut(toggleSelectionModeAction, Qt::Key_Space);
    connect(toggleSelectionModeAction, &QAction::triggered, this, &DolphinMainWindow::toggleSelectionMode);

    // A special version of the toggleSelectionModeAction for the toolbar that also contains a menu
    // with the selectAllAction and invertSelectionAction.
    auto *toggleSelectionModeToolBarAction =
        new KToolBarPopupAction(toggleSelectionModeAction->icon(), toggleSelectionModeAction->iconText(), actionCollection());
    toggleSelectionModeToolBarAction->setToolTip(toggleSelectionModeAction->text());
    toggleSelectionModeToolBarAction->setWhatsThis(toggleSelectionModeAction->whatsThis());
    actionCollection()->addAction(QStringLiteral("toggle_selection_mode_tool_bar"), toggleSelectionModeToolBarAction);
    toggleSelectionModeToolBarAction->setCheckable(true);
    toggleSelectionModeToolBarAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(toggleSelectionModeToolBarAction, &QAction::triggered, toggleSelectionModeAction, &QAction::trigger);
    connect(toggleSelectionModeAction, &QAction::toggled, toggleSelectionModeToolBarAction, &QAction::setChecked);

    QAction *selectAllAction = KStandardAction::selectAll(this, &DolphinMainWindow::selectAll, actionCollection());
    selectAllAction->setWhatsThis(xi18nc("@info:whatsthis",
                                         "This selects all "
                                         "files and folders in the current location."));

    QAction *invertSelection = actionCollection()->addAction(QStringLiteral("invert_selection"));
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setWhatsThis(xi18nc("@info:whatsthis invert",
                                         "This selects all "
                                         "items that you have currently <emphasis>not</emphasis> selected instead."));
    invertSelection->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-invert")));
    actionCollection()->setDefaultShortcut(invertSelection, Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, &QAction::triggered, this, &DolphinMainWindow::invertSelection);

    QMenu *toggleSelectionModeActionMenu = new QMenu(this);
    toggleSelectionModeActionMenu->addAction(selectAllAction);
    toggleSelectionModeActionMenu->addAction(invertSelection);
    toggleSelectionModeToolBarAction->setMenu(toggleSelectionModeActionMenu);

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    Admin::WorkerIntegration::createActAsAdminAction(actionCollection(), this);

    m_splitViewAction = actionCollection()->add<KActionMenu>(QStringLiteral("split_view"));
    m_splitViewMenuAction = actionCollection()->addAction(QStringLiteral("split_view_menu"));

    m_splitViewAction->setWhatsThis(xi18nc("@info:whatsthis split",
                                           "<para>This presents "
                                           "a second view side-by-side with the current view, so you can see "
                                           "the contents of two folders at once and easily move items between "
                                           "them.</para><para>The view that is not \"in focus\" will be dimmed. "
                                           "</para>Click this button again to close one of the views."));
    m_splitViewMenuAction->setWhatsThis(m_splitViewAction->whatsThis());

    // only set it for the menu version
    actionCollection()->setDefaultShortcut(m_splitViewMenuAction, Qt::Key_F3);

    connect(m_splitViewAction, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);
    connect(m_splitViewMenuAction, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);

    QAction *moveToNewTabsSplit = actionCollection()->addAction(QStringLiteral("move_to_tabs_split_view"));
    moveToNewTabsSplit->setText(i18nc("@action:intoolbar Splits both views into separate tabs", "Split View To Tabs"));
    moveToNewTabsSplit->setWhatsThis(xi18nc("@info:whatsthis", "If the view has been split, this will move both views to tabs and close split view."));
    moveToNewTabsSplit->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    actionCollection()->setDefaultShortcut(moveToNewTabsSplit, Qt::CTRL | Qt::SHIFT | Qt::Key_F3);
    connect(moveToNewTabsSplit, &QAction::triggered, this, &DolphinMainWindow::moveSplitViewToTabs);

    QAction *popoutSplit = actionCollection()->addAction(QStringLiteral("popout_split_view"));
    popoutSplit->setWhatsThis(xi18nc("@info:whatsthis",
                                     "If the view has been split, this will pop the view in focus "
                                     "out into a new window."));
    popoutSplit->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    actionCollection()->setDefaultShortcut(popoutSplit, Qt::SHIFT | Qt::Key_F3);
    connect(popoutSplit, &QAction::triggered, this, &DolphinMainWindow::popoutSplitView);

    QAction *stashSplit = actionCollection()->addAction(QStringLiteral("split_stash"));
    actionCollection()->setDefaultShortcut(stashSplit, Qt::CTRL | Qt::Key_S);
    stashSplit->setText(i18nc("@action:intoolbar Stash", "Stash"));
    stashSplit->setToolTip(i18nc("@info", "Opens the stash virtual directory in a split window"));
    stashSplit->setIcon(QIcon::fromTheme(QStringLiteral("folder-stash")));
    stashSplit->setCheckable(false);
    QDBusConnectionInterface *sessionInterface = QDBusConnection::sessionBus().interface();
    stashSplit->setVisible(sessionInterface && sessionInterface->isServiceRegistered(QStringLiteral("org.kde.kio.StashNotifier")));
    connect(stashSplit, &QAction::triggered, this, &DolphinMainWindow::toggleSplitStash);

    QAction *redisplay = KStandardAction::redisplay(this, &DolphinMainWindow::reloadView, actionCollection());
    redisplay->setAutoRepeat(false);
    actionCollection()->setDefaultShortcut(redisplay, Qt::CTRL | Qt::Key_R);
    redisplay->setToolTip(i18nc("@info:tooltip", "Refresh view"));
    redisplay->setWhatsThis(xi18nc("@info:whatsthis refresh",
                                   "<para>This refreshes "
                                   "the folder view.</para>"
                                   "<para>If the contents of this folder have changed, refreshing will re-scan this folder "
                                   "and show you a newly-updated view of the files and folders contained here.</para>"
                                   "<para>If the view is split, this refreshes the one that is currently in focus.</para>"));

    QAction *stop = actionCollection()->addAction(QStringLiteral("stop"));
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setWhatsThis(i18nc("@info", "This stops the loading of the contents of the current folder."));
    stop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    connect(stop, &QAction::triggered, this, &DolphinMainWindow::stopLoading);

    KToggleAction *editableLocation = actionCollection()->add<KToggleAction>(QStringLiteral("editable_location"));
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    editableLocation->setWhatsThis(xi18nc("@info:whatsthis",
                                          "This toggles the <emphasis>Location Bar</emphasis> to be "
                                          "editable so you can directly enter a location you want to go to.<nl/>"
                                          "You can also switch to editing by clicking to the right of the "
                                          "location and switch back by confirming the edited location."));
    actionCollection()->setDefaultShortcut(editableLocation, QKeySequence());
    connect(editableLocation, &KToggleAction::triggered, this, &DolphinMainWindow::toggleEditLocation);

    QAction *replaceLocation = actionCollection()->addAction(QStringLiteral("replace_location"));
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    // i18n: "enter" is used both in the meaning of "writing" and "going to" a new location here.
    // Both meanings are useful but not necessary to understand the use of "Replace Location".
    // So you might want to be more verbose in your language to convey the meaning but it's up to you.
    replaceLocation->setWhatsThis(xi18nc("@info:whatsthis",
                                         "This switches to editing the location and selects it "
                                         "so you can quickly enter a different location."));
    actionCollection()->setDefaultShortcuts(replaceLocation, {Qt::CTRL | Qt::Key_L, Qt::ALT | Qt::Key_D});
    connect(replaceLocation, &QAction::triggered, this, &DolphinMainWindow::replaceLocation);

    // setup 'Go' menu
    {
        QScopedPointer<QAction> backAction(KStandardAction::back(nullptr, nullptr, nullptr));
        m_backAction = new KToolBarPopupAction(backAction->icon(), backAction->text(), actionCollection());
        m_backAction->setObjectName(backAction->objectName());
        m_backAction->setShortcuts(backAction->shortcuts());
    }
    m_backAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(m_backAction, &QAction::triggered, this, &DolphinMainWindow::goBack);
    connect(m_backAction->popupMenu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowBackPopupMenu);
    connect(m_backAction->popupMenu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoBack);
    actionCollection()->addAction(m_backAction->objectName(), m_backAction);

    auto backShortcuts = m_backAction->shortcuts();
    // Prepend this shortcut, to avoid being hidden by the two-slot UI (#371130)
    backShortcuts.prepend(QKeySequence(Qt::Key_Backspace));
    actionCollection()->setDefaultShortcuts(m_backAction, backShortcuts);

    DolphinRecentTabsMenu *recentTabsMenu = new DolphinRecentTabsMenu(this);
    actionCollection()->addAction(QStringLiteral("closed_tabs"), recentTabsMenu);
    connect(m_leftTabs, &DolphinTabWidget::rememberClosedTab, recentTabsMenu, &DolphinRecentTabsMenu::rememberClosedTab);
    connect(m_rightTabs, &DolphinTabWidget::rememberClosedTab, recentTabsMenu, &DolphinRecentTabsMenu::rememberClosedTab);
    connect(recentTabsMenu, &DolphinRecentTabsMenu::restoreClosedTab, this, [this](const QByteArray &state) {
        m_tabWidget->restoreClosedTab(state);
    });
    connect(recentTabsMenu, &DolphinRecentTabsMenu::closedTabsCountChanged, this, &DolphinMainWindow::closedTabsCountChanged);

    QAction *undoCloseTab = actionCollection()->addAction(QStringLiteral("undo_close_tab"));
    undoCloseTab->setText(i18nc("@action:inmenu File", "Undo close tab"));
    undoCloseTab->setWhatsThis(i18nc("@info:whatsthis undo close tab", "This returns you to the previously closed tab."));
    actionCollection()->setDefaultShortcut(undoCloseTab, Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    undoCloseTab->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    undoCloseTab->setEnabled(false);
    connect(undoCloseTab, &QAction::triggered, recentTabsMenu, &DolphinRecentTabsMenu::undoCloseTab);

    auto undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    undoAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "This undoes "
                                    "the last change you made to files or folders.<nl/>"
                                    "Such changes include <interface>creating</interface>, <interface>renaming</interface> "
                                    "and <interface>moving</interface> them to a different location "
                                    "or to the <filename>Trash</filename>. <nl/>Any changes that cannot be undone "
                                    "will ask for your confirmation beforehand."));
    undoAction->setEnabled(false); // undo should be disabled by default

    auto redoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Redo));
    redoAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "This redoes "
                                    "the last change you undid.<nl/>"
                                    "Such changes include <interface>creating</interface>, <interface>renaming</interface> "
                                    "and <interface>moving</interface> files or folders to a different location "
                                    "or to the <filename>Trash</filename>.<nl/>Any changes that cannot be undone "
                                    "will ask for your confirmation beforehand."));
    redoAction->setEnabled(false); // redo should be disabled by default

    {
        QScopedPointer<QAction> forwardAction(KStandardAction::forward(nullptr, nullptr, nullptr));
        m_forwardAction = new KToolBarPopupAction(forwardAction->icon(), forwardAction->text(), actionCollection());
        m_forwardAction->setObjectName(forwardAction->objectName());
        m_forwardAction->setShortcuts(forwardAction->shortcuts());
    }
    m_forwardAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(m_forwardAction, &QAction::triggered, this, &DolphinMainWindow::goForward);
    connect(m_forwardAction->popupMenu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowForwardPopupMenu);
    connect(m_forwardAction->popupMenu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoForward);
    actionCollection()->addAction(m_forwardAction->objectName(), m_forwardAction);
    actionCollection()->setDefaultShortcuts(m_forwardAction, m_forwardAction->shortcuts());

    // enable middle-click to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotBackForwardActionMiddleClicked);
    m_backAction->popupMenu()->installEventFilter(middleClickEventFilter);
    m_forwardAction->popupMenu()->installEventFilter(middleClickEventFilter);
    KStandardAction::up(this, &DolphinMainWindow::goUp, actionCollection());
    QAction *homeAction = KStandardAction::home(this, &DolphinMainWindow::goHome, actionCollection());
    homeAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "Go to your "
                                    "<filename>Home</filename> folder.<nl/>Every user account "
                                    "has their own <filename>Home</filename> that contains their personal files, "
                                    "as well as hidden folders for their applications' data and configuration files."));

    // setup 'Tools' menu
    QAction *compareFiles = actionCollection()->addAction(QStringLiteral("compare_files"));
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(QIcon::fromTheme(QStringLiteral("kompare")));
    compareFiles->setEnabled(false);
    connect(compareFiles, &QAction::triggered, this, &DolphinMainWindow::compareFiles);

    QAction *manageDiskSpaceUsage = actionCollection()->addAction(QStringLiteral("manage_disk_space"));
    manageDiskSpaceUsage->setText(i18nc("@action:inmenu Tools", "Manage Disk Space Usage"));
    manageDiskSpaceUsage->setIcon(QIcon::fromTheme(QStringLiteral("filelight")));
    m_diskSpaceUsageMenu = new DiskSpaceUsageMenu{this};
    manageDiskSpaceUsage->setMenu(m_diskSpaceUsageMenu);

    QAction *openPreferredSearchTool = actionCollection()->addAction(QStringLiteral("open_preferred_search_tool"));
    openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open Preferred Search Tool"));
    openPreferredSearchTool->setWhatsThis(xi18nc("@info:whatsthis",
                                                 "<para>This opens a preferred search tool for the viewed location.</para>"
                                                 "<para>Use <emphasis>More Search Tools</emphasis> menu to configure it.</para>"));
    openPreferredSearchTool->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    actionCollection()->setDefaultShortcut(openPreferredSearchTool, Qt::CTRL | Qt::SHIFT | Qt::Key_F);
    connect(openPreferredSearchTool, &QAction::triggered, this, &DolphinMainWindow::openPreferredSearchTool);

    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        // Get icon of user default terminal emulator application
        const KConfigGroup group(KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::SimpleConfig), QStringLiteral("General"));
        const QString terminalDesktopFilename = group.readEntry("TerminalService");
        const KService::Ptr terminalService = KService::serviceByStorageId(terminalDesktopFilename);
        const QString terminalIcon = terminalService ? terminalService->icon() : QLatin1String("utilities-terminal");

        QAction *openTerminal = actionCollection()->addAction(QStringLiteral("open_terminal"));
        openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
        openTerminal->setWhatsThis(xi18nc("@info:whatsthis",
                                          "<para>This opens a <emphasis>terminal</emphasis> application for the viewed location.</para>"
                                          "<para>To learn more about terminals use the help features in the terminal application.</para>"));
        openTerminal->setIcon(QIcon::fromTheme(terminalIcon));
        actionCollection()->setDefaultShortcut(openTerminal, Qt::SHIFT | Qt::Key_F4);
        connect(openTerminal, &QAction::triggered, this, &DolphinMainWindow::openTerminal);

        QAction *openTerminalHere = actionCollection()->addAction(QStringLiteral("open_terminal_here"));
        // i18n: "Here" refers to the location(s) of the currently selected item(s) or the currently viewed location if nothing is selected.
        openTerminalHere->setText(i18nc("@action:inmenu Tools", "Open Terminal Here"));
        openTerminalHere->setWhatsThis(xi18nc("@info:whatsthis",
                                              "<para>This opens <emphasis>terminal</emphasis> applications for the selected items' locations.</para>"
                                              "<para>To learn more about terminals use the help features in the terminal application.</para>"));
        openTerminalHere->setIcon(QIcon::fromTheme(terminalIcon));
        actionCollection()->setDefaultShortcut(openTerminalHere, Qt::SHIFT | Qt::ALT | Qt::Key_F4);
        connect(openTerminalHere, &QAction::triggered, this, &DolphinMainWindow::openTerminalHere);
    }

    // setup 'Bookmarks' menu
    KActionMenu *bookmarkMenu = new KActionMenu(i18nc("@title:menu", "&Bookmarks"), this);
    bookmarkMenu->setIcon(QIcon::fromTheme(QStringLiteral("bookmarks")));
    // Make the toolbar button version work properly on click
    bookmarkMenu->setPopupMode(QToolButton::InstantPopup);
    m_bookmarkHandler = new DolphinBookmarkHandler(this, actionCollection(), bookmarkMenu->menu(), this);
    actionCollection()->addAction(QStringLiteral("bookmarks"), bookmarkMenu);

    // setup 'Settings' menu
    KToggleAction *showMenuBar = KStandardAction::showMenubar(nullptr, nullptr, actionCollection());
    showMenuBar->setWhatsThis(xi18nc("@info:whatsthis",
                                     "<para>This switches between having a <emphasis>Menubar</emphasis> "
                                     "and having an <interface>%1</interface> button. Both "
                                     "contain mostly the same actions and configuration options.</para>"
                                     "<para>The Menubar takes up more space but allows for fast and organized access to all "
                                     "actions an application has to offer.</para><para>The %1 button "
                                     "is simpler and small which makes triggering advanced actions more time consuming.</para>",
                                     hamburgerMenuAction->text().replace('&', "")));
    connect(showMenuBar,
            &KToggleAction::triggered, // Fixes #286822
            this,
            &DolphinMainWindow::toggleShowMenuBar,
            Qt::QueuedConnection);

    KStandardAction::keyBindings(this, &DolphinMainWindow::slotKeyBindings, actionCollection());
    KStandardAction::preferences(this, &DolphinMainWindow::editSettings, actionCollection());

    // not in menu actions
    QList<QKeySequence> nextTabKeys = KStandardShortcut::tabNext();
    nextTabKeys.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys = KStandardShortcut::tabPrev();
    prevTabKeys.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    for (int i = 0; i < MaxActivateTabShortcuts; ++i) {
        QAction *activateTab = actionCollection()->addAction(QStringLiteral("activate_tab_%1").arg(i));
        activateTab->setText(i18nc("@action:inmenu", "Go to Tab %1", i + 1));
        activateTab->setEnabled(false);
        connect(activateTab, &QAction::triggered, this, [this, i]() {
            m_tabWidget->activateTab(i);
        });

        // only add default shortcuts for the first 9 tabs regardless of MaxActivateTabShortcuts
        if (i < 9) {
            actionCollection()->setDefaultShortcut(activateTab, QStringLiteral("Alt+%1").arg(i + 1));
        }
    }

    QAction *activateLastTab = actionCollection()->addAction(QStringLiteral("activate_last_tab"));
    activateLastTab->setIconText(i18nc("@action:inmenu", "Last Tab"));
    activateLastTab->setText(i18nc("@action:inmenu", "Go to Last Tab"));
    activateLastTab->setEnabled(false);
    connect(activateLastTab, &QAction::triggered, this, &DolphinMainWindow::activateLastTab);
    actionCollection()->setDefaultShortcut(activateLastTab, Qt::ALT | Qt::Key_0);

    QAction *activateNextTab = actionCollection()->addAction(QStringLiteral("activate_next_tab"));
    activateNextTab->setIconText(i18nc("@action:inmenu", "Next Tab"));
    activateNextTab->setText(i18nc("@action:inmenu", "Go to Next Tab"));
    activateNextTab->setEnabled(false);
    connect(activateNextTab, &QAction::triggered, this, &DolphinMainWindow::activateNextTab);
    actionCollection()->setDefaultShortcuts(activateNextTab, nextTabKeys);

    QAction *activatePrevTab = actionCollection()->addAction(QStringLiteral("activate_prev_tab"));
    activatePrevTab->setIconText(i18nc("@action:inmenu", "Previous Tab"));
    activatePrevTab->setText(i18nc("@action:inmenu", "Go to Previous Tab"));
    activatePrevTab->setEnabled(false);
    connect(activatePrevTab, &QAction::triggered, this, &DolphinMainWindow::activatePrevTab);
    actionCollection()->setDefaultShortcuts(activatePrevTab, prevTabKeys);

    // for context menu
    QAction *showTarget = actionCollection()->addAction(QStringLiteral("show_target"));
    showTarget->setText(i18nc("@action:inmenu", "Show Target"));
    showTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    showTarget->setEnabled(false);
    connect(showTarget, &QAction::triggered, this, &DolphinMainWindow::showTarget);

    QAction *openInNewTab = actionCollection()->addAction(QStringLiteral("open_in_new_tab"));
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTab, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction *openInNewTabs = actionCollection()->addAction(QStringLiteral("open_in_new_tabs"));
    openInNewTabs->setText(i18nc("@action:inmenu", "Open in New Tabs"));
    openInNewTabs->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTabs, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction *openInNewWindow = actionCollection()->addAction(QStringLiteral("open_in_new_window"));
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    connect(openInNewWindow, &QAction::triggered, this, &DolphinMainWindow::openInNewWindow);

    QAction *openInSplitViewAction = actionCollection()->addAction(QStringLiteral("open_in_split_view"));
    openInSplitViewAction->setText(i18nc("@action:inmenu", "Open in Other Pane"));
    openInSplitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    connect(openInSplitViewAction, &QAction::triggered, this, [this]() {
        openInSplitView(QUrl());
    });

    // Window color scheme menu
    auto *manager = KColorSchemeManager::instance();
    KActionMenu *selectionMenu = KColorSchemeMenu::createMenu(manager, this);
    auto windowColorSchemeMenu = new QAction(this);
    windowColorSchemeMenu->setMenu(selectionMenu->menu());
    windowColorSchemeMenu->menu()->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-color")));
    windowColorSchemeMenu->menu()->setTitle(i18n("&Theme"));
    windowColorSchemeMenu->setText(i18n("&Theme"));
    windowColorSchemeMenu->setIconText(i18n("Theme"));
    actionCollection()->addAction(QStringLiteral("window_color_sheme"), windowColorSchemeMenu);

    m_recentFiles = new KRecentFilesAction(this);
}

void DolphinMainWindow::setupDockWidgets()
{
    const bool lock = GeneralSettings::lockPanels();

    DolphinPlacesModelSingleton::instance().placesModel()->setPanelsLocked(lock);

    KDualAction *lockLayoutAction = actionCollection()->add<KDualAction>(QStringLiteral("lock_panels"));
    lockLayoutAction->setActiveText(i18nc("@action:inmenu Panels", "Unlock Panels"));
    lockLayoutAction->setActiveIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    lockLayoutAction->setInactiveText(i18nc("@action:inmenu Panels", "Lock Panels"));
    lockLayoutAction->setInactiveIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    lockLayoutAction->setWhatsThis(xi18nc("@info:whatsthis",
                                          "This "
                                          "switches between having panels <emphasis>locked</emphasis> or "
                                          "<emphasis>unlocked</emphasis>.<nl/>Unlocked panels can be "
                                          "dragged to the other side of the window and have a close "
                                          "button.<nl/>Locked panels are embedded more cleanly."));
    lockLayoutAction->setActive(lock);
    connect(lockLayoutAction, &KDualAction::triggered, this, &DolphinMainWindow::togglePanelLockState);

    // Setup "Information"
    DolphinDockWidget *infoDock = new DolphinDockWidget(i18nc("@title:window", "Information"), this);
    infoDock->setLocked(lock);
    infoDock->setObjectName(QStringLiteral("infoDock"));
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

#if HAVE_BALOO
    InformationPanel *infoPanel = new InformationPanel(infoDock);
    infoPanel->setCustomContextMenuActions({lockLayoutAction});
    connect(infoPanel, &InformationPanel::urlActivated, this, &DolphinMainWindow::handleUrl);
    infoDock->setWidget(infoPanel);

    createPanelAction(QIcon::fromTheme(QStringLiteral("documentinfo")), Qt::Key_F11, infoDock, QStringLiteral("show_information_panel"));

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, &DolphinMainWindow::urlChanged, infoPanel, &InformationPanel::setUrl);
    connect(this, &DolphinMainWindow::selectionChanged, infoPanel, &InformationPanel::setSelection);
    connect(this, &DolphinMainWindow::requestItemInfo, infoPanel, &InformationPanel::requestDelayedItemInfo);
    connect(this, &DolphinMainWindow::fileItemsChanged, infoPanel, &InformationPanel::slotFilesItemChanged);
    connect(this, &DolphinMainWindow::settingsChanged, infoPanel, &InformationPanel::readSettings);
#endif

    // i18n: This is the last paragraph for the "What's This"-texts of all four panels.
    const QString panelWhatsThis = xi18nc("@info:whatsthis",
                                          "<para>To show or "
                                          "hide panels like this go to <interface>Menu|Panels</interface> "
                                          "or <interface>View|Panels</interface>.</para>");
#if HAVE_BALOO
    actionCollection()
        ->action(QStringLiteral("show_information_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis",
                              "<para> This toggles the "
                              "<emphasis>information</emphasis> panel at the right side of the "
                              "window.</para><para>The panel provides in-depth information "
                              "about the items your mouse is hovering over or about the selected "
                              "items. Otherwise it informs you about the currently viewed folder.<nl/>"
                              "For single items a preview of their contents is provided.</para>"));
#endif
    infoDock->setWhatsThis(xi18nc("@info:whatsthis",
                                  "<para>This panel "
                                  "provides in-depth information about the items your mouse is "
                                  "hovering over or about the selected items. Otherwise it informs "
                                  "you about the currently viewed folder.<nl/>For single items a "
                                  "preview of their contents is provided.</para><para>You can configure "
                                  "which and how details are given here by right-clicking.</para>")
                           + panelWhatsThis);

    // Setup "Terminal"
#if HAVE_TERMINAL
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        DolphinDockWidget *terminalDock = new DolphinDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
        terminalDock->setLocked(lock);
        terminalDock->setObjectName(QStringLiteral("terminalDock"));
        terminalDock->setContentsMargins(0, 0, 0, 0);
        m_terminalPanel = new TerminalPanel(terminalDock);
        m_terminalPanel->setCustomContextMenuActions({lockLayoutAction});
        terminalDock->setWidget(m_terminalPanel);

        connect(m_terminalPanel, &TerminalPanel::hideTerminalPanel, terminalDock, &DolphinDockWidget::hide);
        connect(m_terminalPanel, &TerminalPanel::changeUrl, this, &DolphinMainWindow::slotTerminalDirectoryChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged, m_terminalPanel, &TerminalPanel::dockVisibilityChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged, this, &DolphinMainWindow::slotTerminalPanelVisibilityChanged);

        createPanelAction(QIcon::fromTheme(QStringLiteral("dialog-scripts")), Qt::Key_F4, terminalDock, QStringLiteral("show_terminal_panel"));

        addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
        connect(this, &DolphinMainWindow::urlChanged, m_terminalPanel, &TerminalPanel::setUrl);
        connect(this, &DolphinMainWindow::urlRefreshed, m_terminalPanel, &TerminalPanel::refreshUrl);

        if (GeneralSettings::version() < 200) {
            terminalDock->hide();
        }

        actionCollection()
            ->action(QStringLiteral("show_terminal_panel"))
            ->setWhatsThis(xi18nc("@info:whatsthis",
                                  "<para>This toggles the "
                                  "<emphasis>terminal</emphasis> panel at the bottom of the window."
                                  "<nl/>The location in the terminal will always match the folder "
                                  "view so you can navigate using either.</para><para>The terminal "
                                  "panel is not needed for basic computer usage but can be useful "
                                  "for advanced tasks. To learn more about terminals use the help features "
                                  "in a standalone terminal application like Konsole.</para>"));
        terminalDock->setWhatsThis(xi18nc("@info:whatsthis",
                                          "<para>This is "
                                          "the <emphasis>terminal</emphasis> panel. It behaves like a "
                                          "normal terminal but will match the location of the folder view "
                                          "so you can navigate using either.</para><para>The terminal panel "
                                          "is not needed for basic computer usage but can be useful for "
                                          "advanced tasks. To learn more about terminals use the help features in a "
                                          "standalone terminal application like Konsole.</para>")
                                   + panelWhatsThis);

        QAction *focusTerminalPanel = actionCollection()->addAction(QStringLiteral("focus_terminal_panel"));
        focusTerminalPanel->setText(i18nc("@action:inmenu Tools", "Focus Terminal Panel"));
        focusTerminalPanel->setToolTip(i18nc("@info:tooltip", "Move keyboard focus to and from the Terminal panel."));
        focusTerminalPanel->setIcon(QIcon::fromTheme(QStringLiteral("swap-panels")));
        actionCollection()->setDefaultShortcut(focusTerminalPanel, Qt::CTRL | Qt::SHIFT | Qt::Key_F4);
        connect(focusTerminalPanel, &QAction::triggered, this, &DolphinMainWindow::toggleTerminalPanelFocus);

        QAction *switchTerminalUrlSync = actionCollection()->addAction(QStringLiteral("switch_terminal_url_sync"));
        switchTerminalUrlSync->setText(i18nc("@action:inmenu", "Follow Directory Switch"));
        switchTerminalUrlSync->setToolTip(
            i18nc("@info:tooltip", "Determines if current working directory must be kept in sync with terminal whenever directory is changed."));
        switchTerminalUrlSync->setCheckable(true);
        connect(switchTerminalUrlSync, &QAction::toggled, m_terminalPanel, &TerminalPanel::switchSync);
        switchTerminalUrlSync->setChecked(true);

        m_terminalPanel->setSwitchTerminalUrlSyncAction(switchTerminalUrlSync);

    } // endif "shell_access" allowed
#endif // HAVE_TERMINAL

    if (GeneralSettings::version() < 200) {
        infoDock->hide();
    }

    // Setup "Places"
    DolphinDockWidget *placesDock = new DolphinDockWidget(i18nc("@title:window", "Places"));
    placesDock->setLocked(lock);
    placesDock->setObjectName(QStringLiteral("placesDock"));
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_placesDock = placesDock;

    // The places, and the button for the settings below them.
    auto *placesContainer = new QWidget(placesDock);
    auto *placesLayout = new QVBoxLayout(placesContainer);
    placesLayout->setContentsMargins(0, 0, 0, 0);
    placesLayout->setSpacing(0);

    m_placesPanel = new PlacesPanel(placesContainer);
    m_placesPanel->setCustomContextMenuActions({lockLayoutAction});
    placesLayout->addWidget(m_placesPanel, 1);

    auto *settingsButton = new QToolButton(placesContainer);
    settingsButton->setAutoRaise(true);
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsButton->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    settingsButton->setText(i18nc("@action:button", "Settings"));
    settingsButton->setToolTip(i18nc("@info:tooltip", "Open the settings"));
    settingsButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(settingsButton, &QToolButton::clicked, this, [this]() {
        if (QAction *preferences = actionCollection()->action(KStandardAction::name(KStandardAction::Preferences))) {
            preferences->trigger();
        }
    });
    m_placesButtonRow = new QHBoxLayout();
    m_placesButtonRow->setContentsMargins(0, 0, 0, 0);
    m_placesButtonRow->setSpacing(0);
    m_placesButtonRow->addWidget(settingsButton);
    placesLayout->addLayout(m_placesButtonRow);

    placesDock->setWidget(placesContainer);

    createPanelAction(QIcon::fromTheme(QStringLiteral("compass")), Qt::Key_F9, placesDock, QStringLiteral("show_places_panel"));
    // OneCommander toggles its sidebar with Alt+S.
    actionCollection()->setDefaultShortcuts(actionCollection()->action(QStringLiteral("show_places_panel")),
                                            {QKeySequence(Qt::Key_F9), QKeySequence(Qt::ALT | Qt::Key_S)});

    // Setup the notifications: the messages the views showed lately, newest first.
    auto *notificationsDock = new DolphinDockWidget(i18nc("@title:window", "Notifications"));
    notificationsDock->setLocked(lock);
    notificationsDock->setObjectName(QStringLiteral("notificationsDock"));
    notificationsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    auto *notificationsList = new QListWidget(notificationsDock);
    notificationsList->setObjectName(QStringLiteral("notificationsList"));
    notificationsList->addItems(DolphinViewContainer::recentMessages());
    connect(DolphinViewContainer::messageNotifier(), &MessageNotifier::messageShown, notificationsList, [notificationsList](const QString &message) {
        notificationsList->insertItem(0, message);
        while (notificationsList->count() > 50) {
            delete notificationsList->takeItem(notificationsList->count() - 1);
        }
    });
    notificationsDock->setWidget(notificationsList);
    createPanelAction(QIcon::fromTheme(QStringLiteral("notifications")),
                      Qt::CTRL | Qt::SHIFT | Qt::Key_AsciiTilde,
                      notificationsDock,
                      QStringLiteral("show_notifications_panel"));
    addDockWidget(Qt::BottomDockWidgetArea, notificationsDock);
    notificationsDock->hide();

    // The favorites: folders in named groups, under the places.
    auto *favoritesDock = new DolphinDockWidget(i18nc("@title:window", "Favorites"));
    favoritesDock->setLocked(lock);
    favoritesDock->setObjectName(QStringLiteral("favoritesDock"));
    favoritesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_favoritesPanel = new FavoritesPanel(favoritesDock);
    m_favoritesPanel->setObjectName(QStringLiteral("favoritesPanel"));
    favoritesDock->setWidget(m_favoritesPanel);
    connect(m_favoritesPanel, &FavoritesPanel::urlActivated, this, &DolphinMainWindow::slotPlaceActivated);
    connect(this, &DolphinMainWindow::urlChanged, m_favoritesPanel, &FavoritesPanel::addHistory);
    connect(m_favoritesPanel, &FavoritesPanel::urlActivatedInNewTab, this, &DolphinMainWindow::openNewTab);
    createPanelAction(QIcon::fromTheme(QStringLiteral("folder-favorites")),
                      Qt::CTRL | Qt::ALT | Qt::Key_F,
                      favoritesDock,
                      QStringLiteral("show_favorites_panel"));
    addDockWidget(Qt::LeftDockWidgetArea, favoritesDock);
    favoritesDock->hide();

    QAction *addToFavorites = actionCollection()->addAction(QStringLiteral("add_to_favorites"));
    addToFavorites->setText(i18nc("@action:inmenu", "Add This Folder to Favorites"));
    addToFavorites->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    actionCollection()->setDefaultShortcut(addToFavorites, Qt::CTRL | Qt::ALT | Qt::Key_D);
    connect(addToFavorites, &QAction::triggered, this, [this, favoritesDock]() {
        const QStringList groups = m_favoritesPanel->groups();
        m_favoritesPanel->addFavorite(m_activeViewContainer->url(), groups.isEmpty() ? i18nc("@item default group", "Favorites") : groups.first());
        favoritesDock->show();
    });

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(m_placesPanel, &PlacesPanel::placeActivated, this, &DolphinMainWindow::slotPlaceActivated);
    connect(m_placesPanel, &PlacesPanel::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(m_placesPanel, &PlacesPanel::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(m_placesPanel, &PlacesPanel::newWindowRequested, this, [this](const QUrl &url) {
        Dolphin::openNewWindow({url}, this);
    });
    connect(m_placesPanel, &PlacesPanel::openInSplitViewRequested, this, &DolphinMainWindow::openInSplitView);
    connect(m_placesPanel, &PlacesPanel::errorMessage, this, &DolphinMainWindow::showErrorMessage);
    connect(this, &DolphinMainWindow::urlChanged, m_placesPanel, &PlacesPanel::setUrl);
    connect(placesDock, &DolphinDockWidget::visibilityChanged, &DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged);
    connect(placesDock, &DolphinDockWidget::visibilityChanged, this, &DolphinMainWindow::slotPlacesPanelVisibilityChanged);
    connect(this, &DolphinMainWindow::settingsChanged, m_placesPanel, &PlacesPanel::readSettings);
    connect(m_placesPanel, &PlacesPanel::storageTearDownRequested, this, &DolphinMainWindow::slotStorageTearDownFromPlacesRequested);
    connect(m_placesPanel, &PlacesPanel::storageTearDownExternallyRequested, this, &DolphinMainWindow::slotStorageTearDownExternallyRequested);
    DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged(m_placesPanel->isVisible());

    auto actionShowAllPlaces = new QAction(QIcon::fromTheme(QStringLiteral("view-hidden")), i18nc("@item:inmenu", "Show Hidden Places"), this);
    actionShowAllPlaces->setCheckable(true);
    actionShowAllPlaces->setDisabled(true);
    actionShowAllPlaces->setWhatsThis(i18nc("@info:whatsthis",
                                            "This displays "
                                            "all places in the places panel that have been hidden. They will "
                                            "appear semi-transparent and allow you to uncheck their \"Hide\" property."));

    connect(actionShowAllPlaces, &QAction::triggered, this, [this](bool checked) {
        m_placesPanel->setShowAll(checked);
    });
    connect(m_placesPanel, &PlacesPanel::allPlacesShownChanged, actionShowAllPlaces, &QAction::setChecked);

    actionCollection()
        ->action(QStringLiteral("show_places_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis",
                              "<para>This toggles the "
                              "<emphasis>places</emphasis> panel at the left side of the window."
                              "</para><para>It allows you to go to locations you have "
                              "bookmarked and to access disk or media attached to the computer "
                              "or to the network. It also contains sections to find recently "
                              "saved files or files of a certain type.</para>"));
    placesDock->setWhatsThis(xi18nc("@info:whatsthis",
                                    "<para>This is the "
                                    "<emphasis>Places</emphasis> panel. It allows you to go to locations "
                                    "you have bookmarked and to access disk or media attached to the "
                                    "computer or to the network. It also contains sections to find "
                                    "recently saved files or files of a certain type.</para><para>"
                                    "Click on an entry to go there. Click with the right mouse button "
                                    "instead to open any entry in a new tab or new window.</para>"
                                    "<para>New entries can be added by dragging folders onto this panel. "
                                    "Right-click any section or entry to hide it. Right-click an empty "
                                    "space on this panel and select <interface>Show Hidden Places"
                                    "</interface> to display it again.</para>")
                             + panelWhatsThis);

    QAction *focusPlacesPanel = actionCollection()->addAction(QStringLiteral("focus_places_panel"));
    focusPlacesPanel->setText(i18nc("@action:inmenu View", "Focus Places Panel"));
    focusPlacesPanel->setToolTip(i18nc("@info:tooltip", "Move keyboard focus to and from the Places panel."));
    focusPlacesPanel->setIcon(QIcon::fromTheme(QStringLiteral("swap-panels")));
    actionCollection()->setDefaultShortcut(focusPlacesPanel, Qt::CTRL | Qt::Key_P);
    connect(focusPlacesPanel, &QAction::triggered, this, &DolphinMainWindow::togglePlacesPanelFocus);

    // Add actions into the "Panels" menu
    KActionMenu *panelsMenu = new KActionMenu(i18nc("@action:inmenu View", "Show Panels"), this);
    actionCollection()->addAction(QStringLiteral("panels"), panelsMenu);
    panelsMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-sidetree")));
    panelsMenu->setPopupMode(QToolButton::InstantPopup);
    const KActionCollection *ac = actionCollection();
    panelsMenu->addAction(ac->action(QStringLiteral("show_places_panel")));
#if HAVE_BALOO
    panelsMenu->addAction(ac->action(QStringLiteral("show_information_panel")));
#endif
    panelsMenu->addAction(ac->action(QStringLiteral("show_terminal_panel")));
    panelsMenu->addSeparator();
    panelsMenu->addAction(lockLayoutAction);
    panelsMenu->addSeparator();
    panelsMenu->addAction(actionShowAllPlaces);
    panelsMenu->addAction(focusPlacesPanel);
    panelsMenu->addAction(ac->action(QStringLiteral("focus_terminal_panel")));

    connect(panelsMenu->menu(), &QMenu::aboutToShow, this, [actionShowAllPlaces] {
        actionShowAllPlaces->setEnabled(DolphinPlacesModelSingleton::instance().placesModel()->hiddenCount());
    });
}

void DolphinMainWindow::setupFileItemActions()
{
    if (m_contextMenuOpen) {
        m_fileItemActionsSetupPending = true;
        return;
    }
    m_fileItemActionsSetupPending = false;

    if (m_fileItemActions) {
        m_fileItemActions->deleteLater();
    }
    m_fileItemActions = new KFileItemActions(this);
    m_fileItemActions->setParentWidget(this);
    connect(m_fileItemActions, &KFileItemActions::error, this, [this](const QString &errorMessage) {
        showErrorMessage(errorMessage);
    });

#if KIO_VERSION >= QT_VERSION_CHECK(6, 24, 0)
    m_serviceMenuShortcutManager->refresh(m_fileItemActions);
#endif
}

void DolphinMainWindow::updateFileAndEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    const KActionCollection *col = actionCollection();
    KFileItemListProperties capabilitiesSource(list);

    QAction *renameAction = col->action(KStandardAction::name(KStandardAction::RenameFile));
    QAction *moveToTrashAction = col->action(KStandardAction::name(KStandardAction::MoveToTrash));
    QAction *deleteAction = col->action(KStandardAction::name(KStandardAction::DeleteFile));
    QAction *cutAction = col->action(KStandardAction::name(KStandardAction::Cut));
    QAction *duplicateAction = col->action(QStringLiteral("duplicate")); // see DolphinViewActionHandler
    QAction *addToPlacesAction = col->action(QStringLiteral("add_to_places"));
    QAction *copyToOtherViewAction = col->action(QStringLiteral("copy_to_inactive_split_view"));
    QAction *moveToOtherViewAction = col->action(QStringLiteral("move_to_inactive_split_view"));
    QAction *copyLocation = col->action(QStringLiteral("copy_location"));

    if (list.isEmpty()) {
        stateChanged(QStringLiteral("has_no_selection"));

        // All actions that need a selection to function can be enabled because they should trigger selection mode.
        renameAction->setEnabled(true);
        moveToTrashAction->setEnabled(true);
        deleteAction->setEnabled(true);
        cutAction->setEnabled(true);
        duplicateAction->setEnabled(true);
        addToPlacesAction->setEnabled(true);
        copyLocation->setEnabled(true);
        // Them triggering selection mode and not directly acting on selected items is signified by adding "…" to their text.
        m_actionTextHelper->textsWhenNothingIsSelectedEnabled(true);

    } else {
        m_actionTextHelper->textsWhenNothingIsSelectedEnabled(false);
        stateChanged(QStringLiteral("has_selection"));

        QAction *deleteWithTrashShortcut = col->action(QStringLiteral("delete_shortcut")); // see DolphinViewActionHandler
        QAction *showTarget = col->action(QStringLiteral("show_target"));

        if (list.length() == 1 && list.first().isDir()) {
            addToPlacesAction->setEnabled(true);
        } else {
            addToPlacesAction->setEnabled(false);
        }

        const bool enableMoveToTrash = capabilitiesSource.isLocal() && capabilitiesSource.supportsMoving();

        renameAction->setEnabled(capabilitiesSource.supportsMoving());
        m_disabledActionNotifier->setDisabledReason(renameAction, i18nc("@info", "Cannot rename: You do not have permission to rename items in this folder."));
        deleteAction->setEnabled(capabilitiesSource.supportsDeleting());
        m_disabledActionNotifier->setDisabledReason(deleteAction,
                                                    i18nc("@info", "Cannot delete: You do not have permission to remove items from this folder."));
        cutAction->setEnabled(capabilitiesSource.supportsMoving());
        m_disabledActionNotifier->setDisabledReason(cutAction, i18nc("@info", "Cannot cut: You do not have permission to move items from this folder."));
        copyLocation->setEnabled(list.length() == 1);
        showTarget->setEnabled(list.length() == 1 && list.at(0).isLink());
        duplicateAction->setEnabled(capabilitiesSource.supportsWriting());
        m_disabledActionNotifier->setDisabledReason(duplicateAction,
                                                    i18nc("@info", "Cannot duplicate here: You do not have permission to create items in this folder."));

        if (enableMoveToTrash) {
            moveToTrashAction->setEnabled(true);
            deleteWithTrashShortcut->setEnabled(false);
            m_disabledActionNotifier->clearDisabledReason(deleteWithTrashShortcut);
        } else {
            moveToTrashAction->setEnabled(false);
            deleteWithTrashShortcut->setEnabled(capabilitiesSource.supportsDeleting());
            m_disabledActionNotifier->setDisabledReason(deleteWithTrashShortcut,
                                                        i18nc("@info", "Cannot delete: You do not have permission to remove items from this folder."));
        }
    }

    if (list.isEmpty()) {
        copyToOtherViewAction->setEnabled(false);
        m_disabledActionNotifier->setDisabledReason(copyToOtherViewAction, i18nc("@info", "Cannot copy to other view: No files selected."));
        moveToOtherViewAction->setEnabled(false);
        m_disabledActionNotifier->setDisabledReason(moveToOtherViewAction, i18nc("@info", "Cannot move to other view: No files selected."));
    } else {
        const KFileItem capabilitiesDestination = inactiveViewContainer()->rootItem();

        const auto destUrl = capabilitiesDestination.url();
        const bool allNotTargetOrigin = std::all_of(list.cbegin(), list.cend(), [destUrl](const KFileItem &item) {
            return item.url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash) != destUrl;
        });

        if (!allNotTargetOrigin) {
            copyToOtherViewAction->setEnabled(false);
            m_disabledActionNotifier->setDisabledReason(copyToOtherViewAction,
                                                        i18nc("@info", "Cannot copy to other view: The other view already contains these items."));
            moveToOtherViewAction->setEnabled(false);
            m_disabledActionNotifier->setDisabledReason(moveToOtherViewAction,
                                                        i18nc("@info", "Cannot move to other view: The other view already contains these items."));
        } else if (!capabilitiesDestination.isWritable()) {
            copyToOtherViewAction->setEnabled(false);
            m_disabledActionNotifier->setDisabledReason(
                copyToOtherViewAction,
                i18nc("@info", "Cannot copy to other view: You do not have permission to write into the destination folder."));
            moveToOtherViewAction->setEnabled(false);
            m_disabledActionNotifier->setDisabledReason(
                moveToOtherViewAction,
                i18nc("@info", "Cannot move to other view: You do not have permission to write into the destination folder."));
        } else {
            copyToOtherViewAction->setEnabled(true);
            moveToOtherViewAction->setEnabled(capabilitiesSource.supportsMoving());
            m_disabledActionNotifier->setDisabledReason(
                moveToOtherViewAction,
                i18nc("@info", "Cannot move to other view: You do not have permission to move items from this folder."));
        }
    }
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    QAction *restoreViewSettingsToDefault = actionCollection()->action(QStringLiteral("restore_view_settings_default"));
    restoreViewSettingsToDefault->setEnabled(!m_activeViewContainer->view()->isViewSettingsDefaults());

    updateSplitActions();
}

void DolphinMainWindow::updateGoActions()
{
    QAction *goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const QUrl currentUrl = m_activeViewContainer->url();
    // I think this is one of the best places to firstly be confronted
    // with a file system and its hierarchy. Talking about the root
    // directory might seem too much here but it is the question that
    // naturally arises in this context.
    goUpAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "<para>Go to "
                                    "the folder that contains the currently viewed one.</para>"
                                    "<para>All files and folders are organized in a hierarchical "
                                    "<emphasis>file system</emphasis>. At the top of this hierarchy is "
                                    "a directory that contains all data connected to this computer"
                                    "—the <emphasis>root directory</emphasis>.</para>"));
    goUpAction->setEnabled(KIO::upUrl(currentUrl) != currentUrl);
}

void DolphinMainWindow::refreshViews()
{
    setupFileItemActions();
    m_leftTabs->refreshViews();
    m_rightTabs->refreshViews();

    if (GeneralSettings::modifiedStartupSettings()) {
        updateWindowTitle();
    }

    updateSplitActions();

    Q_EMIT settingsChanged();
}

void DolphinMainWindow::connectViewSignals(DolphinViewContainer *container)
{
    connect(container, &DolphinViewContainer::showFilterBarChanged, this, &DolphinMainWindow::updateFilterBarAction);
    connect(container, &DolphinViewContainer::writeStateChanged, this, &DolphinMainWindow::slotWriteStateChanged);
    slotWriteStateChanged(container->view()->isFolderWritable());
    connect(container, &DolphinViewContainer::searchBarVisibilityChanged, this, &DolphinMainWindow::updateSearchAction);
    connect(container, &DolphinViewContainer::captionChanged, this, &DolphinMainWindow::updateWindowTitle);
    connect(container, &DolphinViewContainer::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(container, &DolphinViewContainer::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);

    // A container that replaces its view needs the view signals connected again.
    connect(container, &DolphinViewContainer::viewReplaced, this, [this, container]() {
        if (container == m_activeViewContainer) {
            activeViewChanged(container);
        }
    });

    // Make the toggled state of the selection mode actions visually follow the selection mode state of the view.
    auto toggleSelectionModeAction = actionCollection()->action(QStringLiteral("toggle_selection_mode"));
    toggleSelectionModeAction->setChecked(m_activeViewContainer->isSelectionModeEnabled());
    connect(m_activeViewContainer, &DolphinViewContainer::selectionModeChanged, toggleSelectionModeAction, &QAction::setChecked);

    const DolphinView *view = container->view();
    connect(view, &DolphinView::selectionChanged, this, &DolphinMainWindow::slotSelectionChanged);
    connect(view, &DolphinView::requestItemInfo, this, &DolphinMainWindow::requestItemInfo);
    connect(view, &DolphinView::fileItemsChanged, this, &DolphinMainWindow::fileItemsChanged);
    connect(view, &DolphinView::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(view, &DolphinView::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(view, &DolphinView::windowRequested, this, &DolphinMainWindow::openNewWindow);
    connect(view, &DolphinView::requestContextMenu, this, &DolphinMainWindow::openContextMenu);
    connect(view, &DolphinView::directoryLoadingStarted, this, &DolphinMainWindow::enableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted, this, &DolphinMainWindow::disableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted, this, &DolphinMainWindow::slotDirectoryLoadingCompleted);
    connect(view, &DolphinView::goBackRequested, this, &DolphinMainWindow::goBack);
    connect(view, &DolphinView::goForwardRequested, this, &DolphinMainWindow::goForward);
    connect(view, &DolphinView::urlActivated, this, &DolphinMainWindow::handleUrl);
    connect(view, &DolphinView::urlChanged, this, &DolphinMainWindow::syncOtherSide);
    connect(view, &DolphinView::goUpRequested, this, &DolphinMainWindow::goUp);
    connect(view, &DolphinView::doubleClickViewBackground, this, &DolphinMainWindow::slotDoubleClickViewBackground);
    connect(view, &DolphinView::clickViewBackground, this, &DolphinMainWindow::slotClickViewBackground);

    connect(container->urlNavigatorInternalWithHistory(), &KUrlNavigator::urlChanged, this, &DolphinMainWindow::changeUrl);
    connect(container->urlNavigatorInternalWithHistory(), &KUrlNavigator::historyChanged, this, &DolphinMainWindow::updateHistory);

    const KUrlNavigator *navigator = activeNavigators()->primaryUrlNavigator();

    QAction *editableLocactionAction = actionCollection()->action(QStringLiteral("editable_location"));
    editableLocactionAction->setChecked(navigator->isUrlEditable());
    connect(navigator, &KUrlNavigator::editableStateChanged, this, &DolphinMainWindow::slotEditableStateChanged);
    connect(navigator, &KUrlNavigator::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(navigator, &KUrlNavigator::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(navigator, &KUrlNavigator::newWindowRequested, this, &DolphinMainWindow::openNewWindow);

    connect(view, &DolphinView::viewSettingsChanged, this, [this](bool isDefault) {
        QAction *restoreViewSettingsToDefault = actionCollection()->action(QStringLiteral("restore_view_settings_default"));
        restoreViewSettingsToDefault->setEnabled(!isDefault);
    });
}

void DolphinMainWindow::updateSplitActions()
{
    QAction *moveToTabsAction = actionCollection()->action(QStringLiteral("move_to_tabs_split_view"));
    QAction *popoutSplitAction = actionCollection()->action(QStringLiteral("popout_split_view"));

    auto setActionPopupMode = [this](KActionMenu *action, QToolButton::ToolButtonPopupMode popupMode) {
        action->setPopupMode(popupMode);
        if (auto *buttonForAction = qobject_cast<QToolButton *>(toolBar()->widgetForAction(action))) {
            buttonForAction->setPopupMode(popupMode);
        }
    };

    const DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    if (tabPage->splitViewEnabled()) {
        using Choice = GeneralSettings::EnumCloseSplitViewChoice;
        switch (GeneralSettings::closeSplitViewChoice()) {
        case Choice::ActiveView:
            if (tabPage->primaryViewActive()) {
                m_splitViewAction->setIconText(i18nc("@action:intoolbar Close left view", "Close"));
                m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View: Close Left Pane"));
                m_splitViewAction->setToolTip(i18nc("@info View refer here to split view", "Close Left View"));
                m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-left-close")));
                m_splitViewMenuAction->setText(i18nc("@action:inmenu View refer here to split view", "Close Left View"));

                popoutSplitAction->setText(i18nc("@action:intoolbar Move left split view to a new window", "Pop out Left View"));
                popoutSplitAction->setToolTip(i18nc("@info View refer here to split view", "Move left split view to a new window"));
            } else {
                m_splitViewAction->setIconText(i18nc("@action:intoolbar Close right view", "Close"));
                m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View: Close Right Pane"));
                m_splitViewAction->setToolTip(i18nc("@info View refer here to split view", "Close Right View"));
                m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
                m_splitViewMenuAction->setText(i18nc("@action:inmenu View refer here to split view", "Close Right View"));

                popoutSplitAction->setText(i18nc("@action:intoolbar Move right split view to a new window", "Pop out Right View"));
                popoutSplitAction->setToolTip(i18nc("@info View refer here to split view", "Move right split view to a new window"));
            }
            break;
        case Choice::InactiveView:
            if (!tabPage->primaryViewActive()) {
                m_splitViewAction->setIconText(i18nc("@action:intoolbar Close left view", "Close"));
                m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View: Close Left Pane"));
                m_splitViewAction->setToolTip(i18nc("@info View refer here to split view", "Close Left View"));
                m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-left-close")));
                m_splitViewMenuAction->setText(i18nc("@action:inmenu View refer here to split view", "Close Left View"));

                popoutSplitAction->setText(i18nc("@action:intoolbar Move left split view to a new window", "Pop out Left View"));
                popoutSplitAction->setToolTip(i18nc("@info View refer here to split view", "Move left split view to a new window"));
            } else {
                m_splitViewAction->setIconText(i18nc("@action:intoolbar Close right view", "Close"));
                m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View: Close Right Pane"));
                m_splitViewAction->setToolTip(i18nc("@info View refer here to split view", "Close Right View"));
                m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
                m_splitViewMenuAction->setText(i18nc("@action:inmenu View refer here to split view", "Close Right View"));

                popoutSplitAction->setText(i18nc("@action:intoolbar Move right split view to a new window", "Pop out Right View"));
                popoutSplitAction->setToolTip(i18nc("@info View refer here to split view", "Move right split view to a new window"));
            }
            break;
        case Choice::RightView:
            m_splitViewAction->setIconText(i18nc("@action:intoolbar Close right view", "Close"));
            m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View: Close Right Pane"));
            m_splitViewAction->setToolTip(i18nc("@info View refer here to split view", "Close Right View"));
            m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
            m_splitViewMenuAction->setText(i18nc("@action:inmenu View refer here to split view", "Close Right View"));

            popoutSplitAction->setText(i18nc("@action:intoolbar Move right split view to a new window", "Pop out Right View"));
            popoutSplitAction->setToolTip(i18nc("@info View refer here to split view", "Move right split view to a new window"));
            break;
        default:
            Q_UNREACHABLE();
        }
        // dolphinCommander always has two panes, so a pane cannot be moved out.
        moveToTabsAction->setEnabled(false);
        popoutSplitAction->setEnabled(false);

        if (!m_splitViewAction->menu()) {
            setActionPopupMode(m_splitViewAction, QToolButton::MenuButtonPopup);
            m_splitViewAction->setMenu(new QMenu(this));
            m_splitViewAction->addAction(moveToTabsAction);
            m_splitViewAction->addAction(popoutSplitAction);
        }
    } else {
        m_splitViewAction->setIconText(i18nc("@action:intoolbar Split view", "Split"));
        m_splitViewAction->setText(i18nc("@action Split view name in configuration dialogs", "Split View"));
        m_splitViewMenuAction->setText(m_splitViewAction->iconText());
        m_splitViewAction->setToolTip(i18nc("@info", "Split view"));
        m_splitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
        popoutSplitAction->setText(i18nc("@action:intoolbar Move view in focus to a new window", "Pop out"));
        moveToTabsAction->setEnabled(false);
        popoutSplitAction->setEnabled(false);
        if (m_splitViewAction->menu()) {
            m_splitViewAction->removeAction(popoutSplitAction);
            m_splitViewAction->menu()->deleteLater();
            m_splitViewAction->setMenu(nullptr);
            setActionPopupMode(m_splitViewAction, QToolButton::DelayedPopup);
        }
    }

    // Update state from toolbar action
    m_splitViewMenuAction->setToolTip(m_splitViewAction->toolTip());
    m_splitViewMenuAction->setIcon(m_splitViewAction->icon());
}

void DolphinMainWindow::updateAllowedToolbarAreas()
{
    // The path bars of both sides are part of the tab pages and never in the toolbar.
    toolBar()->setAllowedAreas(Qt::AllToolBarAreas);
}

void DolphinMainWindow::updateNavigatorsBackground()
{
    for (DolphinNavigatorsWidgetAction *navigators : {m_leftNavigators, m_rightNavigators}) {
        navigators->setBackgroundEnabled(navigators->isInToolbar());
    }
}

bool DolphinMainWindow::isKompareInstalled() const
{
    static bool initialized = false;
    static bool installed = false;
    if (!initialized) {
        // TODO: maybe replace this approach later by using a menu
        // plugin like kdiff3plugin.cpp
        installed = !QStandardPaths::findExecutable(QStringLiteral("kompare")).isEmpty();
        initialized = true;
    }
    return installed;
}

void DolphinMainWindow::createPanelAction(const QIcon &icon, const QKeySequence &shortcut, QDockWidget *dockWidget, const QString &actionName)
{
    auto dockAction = dockWidget->toggleViewAction();
    dockAction->setIcon(icon);
    dockAction->setEnabled(true);

    QAction *panelAction = actionCollection()->addAction(actionName, dockAction);
    actionCollection()->setDefaultShortcut(panelAction, shortcut);
}
// clang-format off
void DolphinMainWindow::setupWhatsThis()
{
    // main widgets
    menuBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Menubar</emphasis>. It provides access to commands and "
        "configuration options. Left-click on any of the menus on this "
        "bar to see its contents.</para><para>The Menubar can be hidden "
        "by unchecking <interface>Settings|Show Menubar</interface>. Then "
        "most of its contents become available through a <interface>Menu"
        "</interface> button on the <emphasis>Toolbar</emphasis>.</para>"));
    toolBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Toolbar</emphasis>. It allows quick access to "
        "frequently used actions.</para><para>It is highly customizable. "
        "All items you see in the <interface>Menu</interface> or "
        "in the <interface>Menubar</interface> can be placed on the "
        "Toolbar. Just right-click on it and select <interface>Configure "
        "Toolbars…</interface> or find this action within the <interface>"
        "menu</interface>."
        "</para><para>The location of the bar and the style of its "
        "buttons can also be changed in the right-click menu. Right-click "
        "a button if you want to show or hide its text.</para>"));
    m_tabWidget->setWhatsThis(xi18nc("@info:whatsthis main view",
        "<para>Here you can see the <emphasis>folders</emphasis> and "
        "<emphasis>files</emphasis> that are at the location described in "
        "the <interface>Location Bar</interface> above. This area is the "
        "central part of this application where you navigate to the files "
        "you want to use.</para><para>For explanations of all the "
        "features of this <emphasis>view</emphasis> <link "
        "url='help:/dolphincommander/window.html'>click here</link>. This "
        "will open a page from the <emphasis>Handbook</emphasis>.</para>"));

    // Settings menu
    actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings))
        ->setWhatsThis(xi18nc("@info:whatsthis","<para>This opens a window "
        "that lists the <emphasis>keyboard shortcuts</emphasis>.<nl/>"
        "There you can set up key combinations to trigger an action when "
        "they are pressed simultaneously. All commands in this application can "
        "be triggered this way.</para>"));
    actionCollection()->action(KStandardAction::name(KStandardAction::Preferences))
        ->setWhatsThis(xi18nc("@info:whatsthis","This opens a window where you can "
        "change a multitude of settings for this application. For an explanation "
        "of the various settings go to the chapter <emphasis>Configuring Dolphin"
        "</emphasis> in <interface>Help|Dolphin Handbook</interface>."));

    // Help menu

    auto setStandardActionWhatsThis = [this](KStandardAction::StandardAction actionId,
                                             const QString &whatsThis) {
        // Check for the existence of an action since it can be restricted through the Kiosk system
        if (auto *action = actionCollection()->action(KStandardAction::name(actionId))) {
            action->setWhatsThis(whatsThis);
        }
    };

    // i18n: If the external link isn't available in your language it might make
    // sense to state the external link's language in brackets to not
    // frustrate the user. If there are multiple languages that the user might
    // know with a reasonable chance you might want to have 2 external links.
    // The same might be true for any external link you translate.
    setStandardActionWhatsThis(KStandardAction::HelpContents, xi18nc("@info:whatsthis handbook", "<para>This opens the Handbook for this application. It explains the two sides, the tabs, the Miller columns, the menu and the filter.</para>"));
    // (The i18n call should be completely in the line following the i18n: comment without any line breaks within the i18n call or the comment might not be correctly extracted. See: https://commits.kde.org/kxmlgui/a31135046e1b3335b5d7bbbe6aa9a883ce3284c1 )

    setStandardActionWhatsThis(KStandardAction::WhatsThis,
        xi18nc("@info:whatsthis whatsthis button",
        "<para>This is the button that invokes the help feature you are "
        "using right now! Click it, then click any component of this "
        "application to ask \"What's this?\" about it. The mouse cursor "
        "will change appearance if no help is available for a spot.</para>"
        "<para>There is another way to get help: "
        "The <link url='help:/dolphincommander/index.html'>dolphinCommander Handbook</link>."
        "</para><para>The \"What's this?\" help is "
        "missing in most other windows so don't get too used to this.</para>"));

    setStandardActionWhatsThis(KStandardAction::ReportBug,
        xi18nc("@info:whatsthis","<para>This opens a "
        "window that will guide you through reporting errors or flaws "
        "in this application or in other KDE software.</para>"
        "<para>High-quality bug reports are much appreciated. To learn "
        "how to make your bug report as effective as possible "
        "<link url='https://community.kde.org/Get_Involved/Bug_Reporting'>"
        "click here</link>.</para>"));

    setStandardActionWhatsThis(KStandardAction::Donate,
        xi18nc("@info:whatsthis", "<para>This opens a "
        "<emphasis>web page</emphasis> where you can donate to "
        "support the continued work on this application and many "
        "other projects by the <emphasis>KDE</emphasis> community.</para>"
        "<para>Donating is the easiest and fastest way to efficiently "
        "support KDE and its projects. KDE projects are available for "
        "free therefore your donation is needed to cover things that "
        "require money like servers, contributor meetings, etc.</para>"
        "<para><emphasis>KDE e.V.</emphasis> is the non-profit "
        "organization behind the KDE community.</para>"));

    setStandardActionWhatsThis(KStandardAction::SwitchApplicationLanguage,
        xi18nc("@info:whatsthis",
        "With this you can change the language this application uses."
        "<nl/>You can even set secondary languages which will be used "
        "if texts are not available in your preferred language."));

    setStandardActionWhatsThis(KStandardAction::AboutApp,
        xi18nc("@info:whatsthis","This opens a "
        "window that informs you about the version, license, "
        "used libraries and maintainers of this application."));

    setStandardActionWhatsThis(KStandardAction::AboutKDE,
        xi18nc("@info:whatsthis","This opens a "
        "window with information about <emphasis>KDE</emphasis>. "
        "The KDE community are the people behind this free software."
        "<nl/>If you like using this application but don't know "
        "about KDE or want to see a cute dragon have a look!"));
}
// clang-format on

bool DolphinMainWindow::addHamburgerMenuToToolbar()
{
    QDomDocument domDocument = KXMLGUIClient::domDocument();
    if (domDocument.isNull()) {
        return false;
    }
    QDomNode toolbar = domDocument.elementsByTagName(QStringLiteral("ToolBar")).at(0);
    if (toolbar.isNull()) {
        return false;
    }

    QDomElement hamburgerMenuElement = domDocument.createElement(QStringLiteral("Action"));
    hamburgerMenuElement.setAttribute(QStringLiteral("name"), QStringLiteral("hamburger_menu"));
    toolbar.appendChild(hamburgerMenuElement);

    KXMLGUIFactory::saveConfigFile(domDocument, xmlFile());
    reloadXML();
    createGUI();
    return true;
    // Make sure to also remove the <KXMLGUIFactory> and <QDomDocument> include
    // whenever this method is removed (maybe in the year ~2026).
}

// Set a sane initial window size
QSize DolphinMainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(760, 550));
}

void DolphinMainWindow::saveNewToolbarConfig()
{
    KXmlGuiWindow::saveNewToolbarConfig(); // Applies the new config. This has to be called first
                                           // because the rest of this method decides things
                                           // based on the new config.
    m_leftTabs->currentTabPage()->insertNavigatorsWidget(m_leftNavigators);
    m_rightTabs->currentTabPage()->insertNavigatorsWidget(m_rightNavigators);

    updateAllowedToolbarAreas();
    updateNavigatorsBackground();
    (static_cast<KHamburgerMenu *>(actionCollection()->action(KStandardAction::name(KStandardAction::HamburgerMenu))))->hideActionsOf(toolBar());
}

void DolphinMainWindow::toggleTerminalPanelFocus()
{
    if (!m_terminalPanel->isVisible()) {
        actionCollection()->action(QStringLiteral("show_terminal_panel"))->trigger(); // Also moves focus to the panel.
        actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Defocus Terminal Panel"));
        return;
    }

    if (m_terminalPanel->terminalHasFocus()) {
        m_activeViewContainer->view()->setFocus(Qt::FocusReason::ShortcutFocusReason);
        actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Focus Terminal Panel"));
        return;
    }

    m_terminalPanel->setFocus(Qt::FocusReason::ShortcutFocusReason);
    actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Defocus Terminal Panel"));
}

void DolphinMainWindow::togglePlacesPanelFocus()
{
    if (!m_placesPanel->isVisible()) {
        actionCollection()->action(QStringLiteral("show_places_panel"))->trigger(); // Also moves focus to the panel.
        actionCollection()->action(QStringLiteral("focus_places_panel"))->setText(i18nc("@action:inmenu View", "Defocus Terminal Panel"));
        return;
    }

    if (m_placesPanel->hasFocus()) {
        m_activeViewContainer->view()->setFocus(Qt::FocusReason::ShortcutFocusReason);
        actionCollection()->action(QStringLiteral("focus_places_panel"))->setText(i18nc("@action:inmenu View", "Focus Places Panel"));
        return;
    }

    m_placesPanel->setFocus(Qt::FocusReason::ShortcutFocusReason);
    actionCollection()->action(QStringLiteral("focus_places_panel"))->setText(i18nc("@action:inmenu View", "Defocus Places Panel"));
}

DolphinMainWindow::UndoUiInterface::UndoUiInterface()
    : KIO::FileUndoManager::UiInterface()
{
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface() = default;

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job *job)
{
    DolphinMainWindow *mainWin = qobject_cast<DolphinMainWindow *>(parentWidget());
    if (mainWin) {
        DolphinViewContainer *container = mainWin->activeViewContainer();
        container->showMessage(job->errorString(), KMessageWidget::Error);
    } else {
        KIO::FileUndoManager::UiInterface::jobError(job);
    }
}

bool DolphinMainWindow::isUrlOpen(const QString &url)
{
    return m_leftTabs->isUrlOpen(QUrl::fromUserInput(url)) || m_rightTabs->isUrlOpen(QUrl::fromUserInput(url));
}

bool DolphinMainWindow::isItemVisibleInAnyView(const QString &urlOfItem)
{
    return m_leftTabs->isItemVisibleInAnyView(QUrl::fromUserInput(urlOfItem)) || m_rightTabs->isItemVisibleInAnyView(QUrl::fromUserInput(urlOfItem));
}

void DolphinMainWindow::slotDoubleClickViewBackground(Qt::MouseButton button)
{
    if (button != Qt::MouseButton::LeftButton) {
        // only handle left mouse button for now
        return;
    }

    GeneralSettings *settings = GeneralSettings::self();
    QString clickAction = settings->doubleClickViewAction();

    DolphinView *view = activeViewContainer()->view();
    if (view == nullptr || clickAction == "none") {
        return;
    }

    if (clickAction == customCommand) {
        // run custom command set by the user
        QString clickCustomAction = settings->doubleClickViewCustomAction();
        if (clickCustomAction.isEmpty()) {
            qCDebug(DolphinDebug) << "Double-click view: custom action is empty";
            return;
        }
        // {path} becomes one quoted shell word, also when the command already has it in quotes.
        clickCustomAction.replace(QLatin1String("\"{path}\""), QLatin1String("{path}"));
        clickCustomAction.replace(QLatin1String("'{path}'"), QLatin1String("{path}"));
        clickCustomAction.replace(QLatin1String("{path}"), KShell::quoteArg(view->url().toLocalFile()));

        m_job = new KIO::CommandLauncherJob(clickCustomAction);
        m_job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        m_job->start();

    } else {
        // get the action set by the user and trigger it
        const KActionCollection *actions = actionCollection();
        QAction *action = actions->action(clickAction);
        if (action == nullptr) {
            qCWarning(DolphinDebug) << QStringLiteral("Double-click view: action `%1` was not found").arg(clickAction);
            return;
        }
        action->trigger();
    }
}

#include "moc_dolphinmainwindow.cpp"

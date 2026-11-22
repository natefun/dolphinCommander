/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Tests what dolphinCommander adds to Dolphin or changes in it, so that a merge of a newer
 * Dolphin that breaks one of these features fails here. Dolphin's own tests test Dolphin.
 */

#include "commandertools.h"
#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "dolphinnavigatorswidgetaction.h"
#include "dolphintabbar.h"
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include "jobqueue.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistview.h"
#include "kitemviews/kitemlistwidget.h"
#include "testdir.h"
#include "views/columnsstrip.h"
#include "views/dolphincolumnsview.h"
#include "views/dolphinview.h"
#include "views/viewheader.h"
#include "views/viewproperties.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KCoreDirLister>
#include <KFileMetaData/UserMetaData>

#include <KIO/CopyJob>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

class DolphinCommanderFeaturesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testDetailsIsTheDefaultViewMode();
    void testTheHeaderSwitchesToColumnsAndBack();
    void testTheMillerColumnsFollowASwappedView();
    void testTypingFiltersAndEscapeClosesTheFilter_data();
    void testTypingFiltersAndEscapeClosesTheFilter();
    void testATabMovesToTheOtherSide();
    void testATabDroppedAmongPinnedTabsGoesAfterThem();
    void testRenamePatterns_data();
    void testRenamePatterns();
    void testChecksums();
    void testFindDuplicates();
    void testFolderSize();
    void testToDoNames();
    void testConvertImage();
    void testTheTwoSidesCanBeSwapped();
    void testAFolderCanBeMirroredToTheOtherSide();
    void testTheOtherSideCanBeUpdated();
    void testNotesAndColorLabelsAreMadeFromTheMenus();
    void testToDosFileViewsAndLayoutsWork();
    void testThePasteAndQueueActionsWork();
    void testSelectingByPatternAndTheColumnsToggle();
    void testTheDialogsOfTheToolsShowTheirResults();
    void testFoldersCanAlwaysBeSortedByName();
    void testTheNotificationsPanelListsTheMessages();
    void testAToDoIsCheckedOffByClickingItsCheckbox();
    void testTheColorThemesRecolorTheProgram();
    void testACustomThemeUsesTheColorsThatWereChosen();
    void testTheFileViewDesignerEditsAndAppliesAView();
    void testColumnsCanBeHiddenWhenTheViewIsNarrow();
    void testEachColumnCanHaveItsOwnHideWidth();
    void testItemScrollingStepsByRows();
    void testASavedFileViewKeepsTheColumnWidths();
    void testAFolderImageIsPaintedBehindTheItems();
    void testColorLabelsAndAgeStripesArePainted();
    void testJobsRunOneAfterAnother();
    void testClipboardAsFile();
    void testTheMainWindowHasTheOneCommanderActions();

private:
    void waitForViewReady(DolphinViewContainer *container);
    QToolButton *headerModeButton(DolphinView::Mode mode) const;

    TestDir *m_testDir = nullptr;
    DolphinViewContainer *m_container = nullptr;
};

void DolphinCommanderFeaturesTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    GeneralSettings::setGlobalViewProps(false);
}

void DolphinCommanderFeaturesTest::init()
{
    QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/view_properties")).removeRecursively();

    m_testDir = new TestDir();
    m_testDir->createDir("subdir");
    m_testDir->createFile("subdir/inner.txt");
    m_testDir->createFile("file1.txt");
    m_testDir->createFile("file2.txt");
    KCoreDirLister lister;
    lister.openUrl(m_testDir->url());
    QTRY_VERIFY_WITH_TIMEOUT(lister.isFinished(), 5000);

    m_container = new DolphinViewContainer(m_testDir->url(), nullptr);
    m_container->resize(800, 600);
    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));
    waitForViewReady(m_container);
}

void DolphinCommanderFeaturesTest::cleanup()
{
    delete m_container;
    m_container = nullptr;
    delete m_testDir;
    m_testDir = nullptr;
}

void DolphinCommanderFeaturesTest::waitForViewReady(DolphinViewContainer *container)
{
    QTRY_VERIFY_WITH_TIMEOUT(container->view()->itemsCount() > 0, 5000);
}

QToolButton *DolphinCommanderFeaturesTest::headerModeButton(DolphinView::Mode mode) const
{
    auto *header = m_container->findChild<ViewHeader *>();
    if (!header) {
        return nullptr;
    }
    const auto buttons = header->findChildren<QToolButton *>();
    for (QToolButton *button : buttons) {
        const QVariant buttonMode = button->property("viewMode");
        if (buttonMode.isValid() && buttonMode.toInt() == int(mode)) {
            return button;
        }
    }
    return nullptr;
}

void DolphinCommanderFeaturesTest::testDetailsIsTheDefaultViewMode()
{
    // A folder without view properties of its own opens in the details view.
    QCOMPARE(m_container->view()->viewMode(), DolphinView::DetailsView);
}

void DolphinCommanderFeaturesTest::testTheHeaderSwitchesToColumnsAndBack()
{
    // The columns view is another view, so the header has to ask the container for it and then
    // follow the view that replaced the one it had.
    QToolButton *columns = headerModeButton(DolphinView::ColumnsView);
    QVERIFY(columns);
    columns->click();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()));
    waitForViewReady(m_container);

    QToolButton *details = headerModeButton(DolphinView::DetailsView);
    QVERIFY(details);
    details->click();
    QVERIFY(!qobject_cast<DolphinColumnsView *>(m_container->view()));
    QCOMPARE(m_container->view()->viewMode(), DolphinView::DetailsView);
    QVERIFY(details->isChecked());

    // A switch that does not come from the header shows in it too, which it only can if it
    // follows the view that took the place of the old one.
    m_container->setViewMode(DolphinView::ColumnsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()));
    QTRY_VERIFY(columns->isChecked());
    QVERIFY(!details->isChecked());
}

void DolphinCommanderFeaturesTest::testTheMillerColumnsFollowASwappedView()
{
    m_container->setViewMode(DolphinView::ColumnsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()));
    m_container->setViewMode(DolphinView::DetailsView);
    waitForViewReady(m_container);

    auto *strip = m_container->findChild<ColumnsStrip *>();
    QVERIFY(strip);
    const QUrl subdir = QUrl::fromLocalFile(m_testDir->path() + QStringLiteral("/subdir"));
    Q_EMIT strip->urlRequested(subdir);
    QTRY_COMPARE_WITH_TIMEOUT(m_container->view()->url().adjusted(QUrl::StripTrailingSlash), subdir, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(strip->url().adjusted(QUrl::StripTrailingSlash), subdir, 5000);
}

void DolphinCommanderFeaturesTest::testTypingFiltersAndEscapeClosesTheFilter_data()
{
    QTest::addColumn<DolphinView::Mode>("mode");
    QTest::newRow("details") << DolphinView::DetailsView;
    QTest::newRow("columns") << DolphinView::ColumnsView;
}

void DolphinCommanderFeaturesTest::testTypingFiltersAndEscapeClosesTheFilter()
{
    QFETCH(DolphinView::Mode, mode);
    m_container->setViewMode(mode);
    waitForViewReady(m_container);
    QVERIFY(!m_container->isFilterBarVisible());

    m_container->activateWindow();
    m_container->view()->setFocus();
    QTRY_VERIFY(QApplication::focusWidget());
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_F);
    QTRY_VERIFY_WITH_TIMEOUT(m_container->isFilterBarVisible(), 2000);
    QCOMPARE(m_container->view()->nameFilter(), QStringLiteral("f"));

    m_container->view()->setFocus();
    QTest::keyClick(QApplication::focusWidget(), Qt::Key_Escape);
    QTRY_VERIFY_WITH_TIMEOUT(!m_container->isFilterBarVisible(), 2000);
}

void DolphinCommanderFeaturesTest::testATabMovesToTheOtherSide()
{
    // The two sides of the window, each with its own tabs.
    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    auto *leftNavigators = new DolphinNavigatorsWidgetAction(&window);
    auto *rightNavigators = new DolphinNavigatorsWidgetAction(&window);
    auto *left = new DolphinTabWidget(leftNavigators, &window);
    auto *right = new DolphinTabWidget(rightNavigators, &window);
    layout->addWidget(left);
    layout->addWidget(right);
    window.resize(1200, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    const QUrl subdir = QUrl::fromLocalFile(m_testDir->path() + QStringLiteral("/subdir"));
    left->openNewActivatedTab(m_testDir->url());
    left->openNewActivatedTab(subdir);
    right->openNewActivatedTab(m_testDir->url());
    QCOMPARE(left->count(), 2);
    QCOMPARE(right->count(), 1);
    DolphinTabPage *moved = left->tabPageAt(1);

    QVERIFY(QMetaObject::invokeMethod(left,
                                      "moveTabToOtherSide",
                                      Q_ARG(int, 1),
                                      Q_ARG(DolphinTabBar *, static_cast<DolphinTabBar *>(right->tabBar())),
                                      Q_ARG(int, -1)));

    QCOMPARE(left->count(), 1);
    QCOMPARE(right->count(), 2);
    // The page itself moves, with its folder, and is the current tab of the side it went to.
    QCOMPARE(right->tabPageAt(1), moved);
    QCOMPARE(right->currentIndex(), 1);
    QCOMPARE(moved->activeViewContainer()->url().adjusted(QUrl::StripTrailingSlash), subdir);

    // A change of folder in the moved tab is now reported by the side it is on.
    QSignalSpy leftUrlChanged(left, &DolphinTabWidget::urlChanged);
    QSignalSpy rightUrlChanged(right, &DolphinTabWidget::urlChanged);
    moved->activeViewContainer()->setUrl(m_testDir->url());
    QTRY_VERIFY_WITH_TIMEOUT(rightUrlChanged.count() > 0, 5000);
    QCOMPARE(leftUrlChanged.count(), 0);
}

void DolphinCommanderFeaturesTest::testRenamePatterns_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("counter");
    QTest::addColumn<QString>("expected");

    QTest::newRow("whole name") << "[F]" << "photo.final.jpg" << 1 << "photo.final.jpg";
    QTest::newRow("name and extension") << "[N]-x.[E]" << "photo.final.jpg" << 1 << "photo.final-x.jpg";
    QTest::newRow("no extension") << "[N]|[E]|" << "README" << 1 << "README||";
    QTest::newRow("dotfile has no extension") << "[N]" << ".bashrc" << 1 << ".bashrc";
    QTest::newRow("counter") << "pic[C].[E]" << "a.png" << 7 << "pic7.png";
    QTest::newRow("padded counter") << "pic[C3].[E]" << "a.png" << 7 << "pic007.png";
    QTest::newRow("date") << "[d]_[t]" << "a" << 1 << "2026-11-21_154500";
    QTest::newRow("unknown tokens stay") << "[X][" << "a" << 1 << "[X][";
}

void DolphinCommanderFeaturesTest::testRenamePatterns()
{
    QFETCH(QString, pattern);
    QFETCH(QString, name);
    QFETCH(int, counter);
    QFETCH(QString, expected);
    const QDateTime modified(QDate(2026, 11, 21), QTime(15, 45, 0));
    QCOMPARE(CommanderTools::expandRenamePattern(pattern, name, counter, modified), expected);
}

void DolphinCommanderFeaturesTest::testChecksums()
{
    TestDir dir;
    dir.createFile("f", "abc");
    const QString path = dir.path() + QStringLiteral("/f");
    QCOMPARE(CommanderTools::hashFile(path, QCryptographicHash::Md5), QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    QCOMPARE(CommanderTools::hashFile(path, QCryptographicHash::Sha256), QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    QCOMPARE(CommanderTools::hashFile(dir.path() + QStringLiteral("/missing"), QCryptographicHash::Md5), QString());
    QVERIFY(CommanderTools::checksumsMatch(QStringLiteral("  900150983CD24FB0D6963F7D28E17F72\n"), QStringLiteral("900150983cd24fb0d6963f7d28e17f72")));
    QVERIFY(!CommanderTools::checksumsMatch(QString(), QString()));
    QVERIFY(!CommanderTools::checksumsMatch(QStringLiteral("aa"), QStringLiteral("ab")));
}

void DolphinCommanderFeaturesTest::testTheMainWindowHasTheOneCommanderActions()
{
    // A shortcut that two actions share makes KXmlGuiWindow ask the user about it at startup, which would hang here: close that dialog.
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    KActionCollection *actions = window.actionCollection();
    QMultiHash<QKeySequence, QString> owners;
    for (QAction *action : actions->actions()) {
        for (const QKeySequence &key : action->shortcuts()) {
            owners.insert(key, action->objectName());
        }
    }
    QStringList ambiguous;
    for (const QKeySequence &key : owners.uniqueKeys()) {
        // Dolphin's own Del for "delete_shortcut" and "movetotrash" is not ours.
        if (owners.count(key) > 1 && key != QKeySequence(Qt::Key_Delete)) {
            ambiguous << key.toString() + QLatin1Char('=') + owners.values(key).join(QLatin1Char('+'));
        }
    }
    QVERIFY2(ambiguous.isEmpty(), qPrintable(ambiguous.join(QLatin1String("; "))));
    const QStringList names = {"select_by_pattern",
                               "deselect_by_pattern",
                               "mirror_to_other_side",
                               "swap_sides",
                               "sync_to_other_side",
                               "find_duplicates",
                               "color_label",
                               "new_note",
                               "new_todo",
                               "toggle_todo_done",
                               "scripts_menu",
                               "folder_sizes",
                               "convert_images",
                               "open_with_dialog",
                               "recent_notifications",
                               "age_stripes",
                               "commander_layouts",
                               "file_views",
                               "toggle_columns_view",
                               "folders_sorted_by_name",
                               "hide_columns_when_narrow",
                               "item_scrolling",
                               "color_theme",
                               "show_notifications_panel",
                               "queue_jobs",
                               "paste_clipboard_as_file",
                               "new_tab_from_clipboard",
                               "run_with_arguments"};
    for (const QString &name : names) {
        QVERIFY2(actions->action(name), qPrintable(name));
    }
    const QList<QPair<QString, QKeySequence>> keys = {{"copy_to_inactive_split_view", QKeySequence(Qt::ALT | Qt::Key_C)},
                                                      {"move_to_inactive_split_view", QKeySequence(Qt::ALT | Qt::Key_M)},
                                                      {"show_places_panel", QKeySequence(Qt::ALT | Qt::Key_S)},
                                                      {"duplicate", QKeySequence(Qt::CTRL | Qt::Key_D)},
                                                      {"create_dir", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N)},
                                                      {"focus_inactive_split_view", QKeySequence(Qt::ALT | Qt::Key_F)},
                                                      {"replace_location", QKeySequence(Qt::ALT | Qt::Key_D)}};
    for (const auto &[name, key] : keys) {
        QAction *action = actions->action(name);
        QVERIFY2(action, qPrintable(name));
        QVERIFY2(action->shortcuts().contains(key), qPrintable(name + QStringLiteral(" lacks ") + key.toString()));
    }
}

void DolphinCommanderFeaturesTest::testClipboardAsFile()
{
    TestDir dir;
    QMimeData text;
    text.setText(QStringLiteral("hello"));
    const QString txt = CommanderTools::saveClipboardAsFile(&text, dir.path());
    QVERIFY(txt.endsWith(QStringLiteral(".txt")));
    QFile file(txt);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("hello"));

    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QMimeData picture;
    picture.setImageData(image);
    QVERIFY(CommanderTools::saveClipboardAsFile(&picture, dir.path()).endsWith(QStringLiteral(".png")));

    QMimeData nothing;
    QVERIFY(CommanderTools::saveClipboardAsFile(&nothing, dir.path()).isEmpty());
}

void DolphinCommanderFeaturesTest::testJobsRunOneAfterAnother()
{
    TestDir dir;
    const QByteArray big(8 * 1024 * 1024, 'x');
    dir.createFile("a", big);
    dir.createFile("b", big);
    QDir().mkpath(dir.path() + QStringLiteral("/out1"));
    QDir().mkpath(dir.path() + QStringLiteral("/out2"));
    JobQueue::setEnabled(true);

    auto *first =
        KIO::copy(QUrl::fromLocalFile(dir.path() + QStringLiteral("/a")), QUrl::fromLocalFile(dir.path() + QStringLiteral("/out1")), KIO::HideProgressInfo);
    JobQueue::add(first);
    auto *second =
        KIO::copy(QUrl::fromLocalFile(dir.path() + QStringLiteral("/b")), QUrl::fromLocalFile(dir.path() + QStringLiteral("/out2")), KIO::HideProgressInfo);
    JobQueue::add(second);
    QCOMPARE(JobQueue::count(), 2);

    bool secondStartedBeforeFirstEnded = false;
    bool firstEnded = false;
    connect(first, &KJob::finished, this, [&]() {
        firstEnded = true;
    });
    connect(second, &KIO::CopyJob::copying, this, [&]() {
        secondStartedBeforeFirstEnded = secondStartedBeforeFirstEnded || !firstEnded;
    });
    QSignalSpy firstDone(first, &KJob::finished);
    QSignalSpy secondDone(second, &KJob::finished);
    QTRY_COMPARE_WITH_TIMEOUT(secondDone.count(), 1, 20000);
    QCOMPARE(firstDone.count(), 1);
    QVERIFY(!secondStartedBeforeFirstEnded);
    QVERIFY(QFile::exists(dir.path() + QStringLiteral("/out1/a")));
    QVERIFY(QFile::exists(dir.path() + QStringLiteral("/out2/b")));
    QCOMPARE(JobQueue::count(), 0);
}

void DolphinCommanderFeaturesTest::testColorLabelsAndAgeStripesArePainted()
{
    const KFileItemList items = m_container->view()->items();
    QVERIFY(!items.isEmpty());
    const QString path = items.first().localPath();
    {
        KFileMetaData::UserMetaData probe(path);
        probe.setTags({QStringLiteral("blue")});
        if (probe.tags().isEmpty()) {
            QSKIP("The file system keeps no extended attributes here.");
        }
        probe.setTags({});
    }
    const auto count = [](const QImage &image, bool blue) {
        int found = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x, y);
                if (blue) {
                    found += (c.blue() > c.red() + 25 && c.blue() > c.green() + 5) ? 1 : 0;
                } else {
                    // The stripe of something changed today is red and saturated.
                    found += (c.red() > 200 && c.green() < 90 && c.blue() < 90) ? 1 : 0;
                }
            }
        }
        return found;
    };
    m_container->resize(700, 500);
    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));

    KItemListWidget::setAgeStripesShown(false);
    m_container->view()->reload();
    QTest::qWait(1000);
    const QImage plain = m_container->grab().toImage();

    KItemListWidget::setAgeStripesShown(true);
    KFileMetaData::UserMetaData(path).setTags({QStringLiteral("blue")});
    m_container->view()->reload();
    QTest::qWait(1500);
    const QImage painted = m_container->grab().toImage();
    KItemListWidget::setAgeStripesShown(false);

    QVERIFY2(count(painted, true) > count(plain, true) + 200,
             qPrintable(QStringLiteral("blue pixels %1 before, %2 after").arg(count(plain, true)).arg(count(painted, true))));
    QVERIFY2(count(painted, false) > count(plain, false) + 10,
             qPrintable(QStringLiteral("red pixels %1 before, %2 after").arg(count(plain, false)).arg(count(painted, false))));
}

void DolphinCommanderFeaturesTest::testAFolderImageIsPaintedBehindTheItems()
{
    const auto green = [](const QImage &image) {
        int found = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor c = image.pixelColor(x, y);
                found += (c.green() > c.red() + 15 && c.green() > c.blue() + 15) ? 1 : 0;
            }
        }
        return found;
    };
    m_container->resize(700, 500);
    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));
    QTest::qWait(800);
    const int before = green(m_container->grab().toImage());

    QImage cover(200, 200, QImage::Format_RGB32);
    cover.fill(QColor(0, 200, 0));
    QVERIFY(cover.save(m_testDir->path() + QStringLiteral("/cover.png")));
    m_container->view()->reload();
    QTest::qWait(1500);
    const int after = green(m_container->grab().toImage());
    QVERIFY2(after > before + 2000, qPrintable(QStringLiteral("green pixels %1 before, %2 after").arg(before).arg(after)));
}

void DolphinCommanderFeaturesTest::testTheTwoSidesCanBeSwapped()
{
    TestDir a;
    TestDir b;
    a.createFile("a1");
    b.createFile("b1");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();

    window.openDirectories({a.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    actions->action(QStringLiteral("focus_inactive_split_view"))->trigger();
    window.openDirectories({b.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTest::qWait(300);
    actions->action(QStringLiteral("focus_inactive_split_view"))->trigger();
    QTest::qWait(300);
    // A path is all that matters: the view may report it with or without the file scheme.
    const auto plain = [](const QUrl &url) {
        return url.adjusted(QUrl::StripTrailingSlash).path();
    };
    const QString here = plain(window.activeViewContainer()->url());
    QVERIFY2(here == plain(a.url()) || here == plain(b.url()), qPrintable(here));
    const QString there = here == plain(a.url()) ? plain(b.url()) : plain(a.url());

    // Which side is active can change when a folder is opened on the other one, so the sides are followed by their containers.
    DolphinViewContainer *thisSide = window.activeViewContainer();
    actions->action(QStringLiteral("swap_sides"))->trigger();
    QTRY_COMPARE(plain(thisSide->url()), there);
}

void DolphinCommanderFeaturesTest::testAFolderCanBeMirroredToTheOtherSide()
{
    TestDir a;
    TestDir b;
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();

    window.openDirectories({a.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    DolphinViewContainer *first = window.activeViewContainer();
    actions->action(QStringLiteral("focus_inactive_split_view"))->trigger();
    window.openDirectories({b.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer() && window.activeViewContainer() != first);
    DolphinViewContainer *second = window.activeViewContainer();
    QTRY_COMPARE(second->url().adjusted(QUrl::StripTrailingSlash).path(), b.url().adjusted(QUrl::StripTrailingSlash).path());

    // The active side is the second one: its folder is to appear on the first.
    actions->action(QStringLiteral("mirror_to_other_side"))->trigger();
    QTRY_COMPARE(first->url().adjusted(QUrl::StripTrailingSlash).path(), b.url().adjusted(QUrl::StripTrailingSlash).path());
    QCOMPARE(second->url().adjusted(QUrl::StripTrailingSlash).path(), b.url().adjusted(QUrl::StripTrailingSlash).path());
}

void DolphinCommanderFeaturesTest::testTheOtherSideCanBeUpdated()
{
    TestDir source;
    TestDir target;
    source.createFile("new.txt", "fresh");
    source.createFile("both.txt", "newer");
    target.createFile("both.txt", "older", QDateTime::currentDateTime().addDays(-3));
    target.createFile("only-there.txt", "stays");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();

    window.openDirectories({target.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    DolphinViewContainer *targetSide = window.activeViewContainer();
    actions->action(QStringLiteral("focus_inactive_split_view"))->trigger();
    window.openDirectories({source.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer() && window.activeViewContainer() != targetSide);
    QTRY_VERIFY(window.activeViewContainer()->view()->itemsCount() == 2);
    QTRY_VERIFY(targetSide->view()->itemsCount() == 2);

    actions->action(QStringLiteral("sync_to_other_side"))->trigger();
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(target.path() + QStringLiteral("/new.txt")), 10000);
    QTRY_VERIFY_WITH_TIMEOUT(QFile(target.path() + QStringLiteral("/both.txt")).size() == 5, 10000);
    QVERIFY(QFile::exists(target.path() + QStringLiteral("/only-there.txt")));
}

void DolphinCommanderFeaturesTest::testNotesAndColorLabelsAreMadeFromTheMenus()
{
    TestDir dir;
    dir.createFile("thing.txt", "x");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 1);

    // New Note makes a dated .txt file in the folder.
    actions->action(QStringLiteral("new_note"))->trigger();
    const QStringList notes = QDir(dir.path()).entryList({QStringLiteral("* note.txt")});
    QCOMPARE(notes.size(), 1);

    // A color label from the menu ends up as a tag of the selected file.
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 2);
    // The new note is the selected item now: it is the one to label.
    QTRY_COMPARE(window.activeViewContainer()->view()->selectedItemsCount(), 1);
    const QString labeled = window.activeViewContainer()->view()->selectedItems().first().localPath();
    QVERIFY(labeled.endsWith(QStringLiteral(" note.txt")));
    auto *labels = qobject_cast<KActionMenu *>(actions->action(QStringLiteral("color_label")));
    QVERIFY(labels);
    labels->menu()->actions().first()->trigger();
    const QStringList tags = KFileMetaData::UserMetaData(labeled).tags();
    if (tags.isEmpty()) {
        QSKIP("The file system keeps no extended attributes here.");
    }
    QCOMPARE(tags, QStringList{QStringLiteral("red")});
    // The selection is still there for the next label.
    QTRY_COMPARE(window.activeViewContainer()->view()->selectedItemsCount(), 1);
    // And "No Color Label" takes it off again.
    labels->menu()->actions().last()->trigger();
    QVERIFY(KFileMetaData::UserMetaData(labeled).tags().isEmpty());
}

void DolphinCommanderFeaturesTest::testToDosFileViewsAndLayoutsWork()
{
    TestDir dir;
    dir.createFile("buy milk.2do", "milk");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    DolphinView *view = window.activeViewContainer()->view();
    QTRY_COMPARE(view->itemsCount(), 1);

    // A to-do is marked done with the extension .2dx, and open again with .2do.
    view->selectAll();
    QTRY_COMPARE(view->selectedItemsCount(), 1);
    actions->action(QStringLiteral("toggle_todo_done"))->trigger();
    QVERIFY(QFile::exists(dir.path() + QStringLiteral("/buy milk.2dx")));
    QVERIFY(!QFile::exists(dir.path() + QStringLiteral("/buy milk.2do")));

    // The named file views set the view mode and the previews.
    auto *fileViews = qobject_cast<KActionMenu *>(actions->action(QStringLiteral("file_views")));
    QVERIFY(fileViews);
    const auto applyView = [&](const QString &name) {
        for (QAction *action : fileViews->menu()->actions()) {
            if (action->text() == name) {
                action->trigger();
                return true;
            }
        }
        return false;
    };
    QVERIFY(applyView(QStringLiteral("Name Only")));
    QTRY_COMPARE(window.activeViewContainer()->view()->viewMode(), DolphinView::CompactView);
    QVERIFY(applyView(QStringLiteral("Thumbnail Grid")));
    QTRY_COMPARE(window.activeViewContainer()->view()->viewMode(), DolphinView::IconsView);
    QVERIFY(window.activeViewContainer()->view()->previewsShown());
    QVERIFY(applyView(QStringLiteral("Details")));
    QTRY_COMPARE(window.activeViewContainer()->view()->viewMode(), DolphinView::DetailsView);
    QVERIFY(!window.activeViewContainer()->view()->previewsShown());

    // The layouts: Classic shows the information panel, Standard hides it, Columns switches to the columns view.
    auto *layouts = qobject_cast<KActionMenu *>(actions->action(QStringLiteral("commander_layouts")));
    QVERIFY(layouts);
    // Without Baloo there is no information panel: the layouts then only differ in the sides and the columns.
    QAction *infoPanel = actions->action(QStringLiteral("show_information_panel"));
    const QList<QAction *> layoutActions = layouts->menu()->actions();
    QCOMPARE(layoutActions.size(), 3);
    layoutActions.at(2)->trigger(); // Classic
    QVERIFY(!infoPanel || infoPanel->isChecked());
    layoutActions.at(0)->trigger(); // Standard
    QVERIFY(!infoPanel || !infoPanel->isChecked());
    layoutActions.at(1)->trigger(); // Columns
    QTRY_VERIFY(qobject_cast<DolphinColumnsView *>(window.activeViewContainer()->view()));
}

void DolphinCommanderFeaturesTest::testThePasteAndQueueActionsWork()
{
    TestDir dir;
    dir.createFile("keep");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 1);

    // Paste Clipboard as File makes a dated .txt of the text that was copied.
    QApplication::clipboard()->setText(QStringLiteral("remember this"));
    actions->action(QStringLiteral("paste_clipboard_as_file"))->trigger();
    const QStringList made = QDir(dir.path()).entryList({QStringLiteral("*.txt")});
    QCOMPARE(made.size(), 1);
    QFile file(dir.path() + QLatin1Char('/') + made.first());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("remember this"));

    // The queue switch turns JobQueue on and off.
    QAction *queue = actions->action(QStringLiteral("queue_jobs"));
    QVERIFY(queue->isCheckable());
    queue->setChecked(false);
    QVERIFY(!JobQueue::isEnabled());
    queue->setChecked(true);
    QVERIFY(JobQueue::isEnabled());

    // New Tab from Clipboard Path opens a tab for the folder on the clipboard.
    QTemporaryDir other;
    QVERIFY(other.isValid());
    const int tabsBefore = window.findChildren<DolphinTabWidget *>().first()->count() + window.findChildren<DolphinTabWidget *>().last()->count();
    QApplication::clipboard()->setText(other.path());
    actions->action(QStringLiteral("new_tab_from_clipboard"))->trigger();
    const int tabsAfter = window.findChildren<DolphinTabWidget *>().first()->count() + window.findChildren<DolphinTabWidget *>().last()->count();
    QCOMPARE(tabsAfter, tabsBefore + 1);
}

void DolphinCommanderFeaturesTest::testSelectingByPatternAndTheColumnsToggle()
{
    TestDir dir;
    dir.createFiles({QStringLiteral("a.jpg"), QStringLiteral("b.jpg"), QStringLiteral("c.png")});
    QString pattern;
    QTimer answerer;
    answerer.setInterval(100);
    connect(&answerer, &QTimer::timeout, this, [&]() {
        // The pattern dialog gets its answer; any other dialog is closed.
        if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
            input->setTextValue(pattern);
            input->accept();
        } else if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    answerer.start();
    DolphinMainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 3);

    pattern = QStringLiteral("*.jpg");
    actions->action(QStringLiteral("select_by_pattern"))->trigger();
    QTRY_COMPARE(window.activeViewContainer()->view()->selectedItemsCount(), 2);
    pattern = QStringLiteral("a*");
    actions->action(QStringLiteral("deselect_by_pattern"))->trigger();
    QTRY_COMPARE(window.activeViewContainer()->view()->selectedItemsCount(), 1);
    QCOMPARE(window.activeViewContainer()->view()->selectedItems().first().name(), QStringLiteral("b.jpg"));

    // The toggle goes to the Miller columns and back to the details.
    QCOMPARE(window.activeViewContainer()->view()->viewMode(), DolphinView::DetailsView);
    actions->action(QStringLiteral("toggle_columns_view"))->trigger();
    QTRY_VERIFY(qobject_cast<DolphinColumnsView *>(window.activeViewContainer()->view()));
    actions->action(QStringLiteral("toggle_columns_view"))->trigger();
    QTRY_VERIFY(!qobject_cast<DolphinColumnsView *>(window.activeViewContainer()->view()));
    QCOMPARE(window.activeViewContainer()->view()->viewMode(), DolphinView::DetailsView);
    answerer.stop();
}

void DolphinCommanderFeaturesTest::testTheDialogsOfTheToolsShowTheirResults()
{
    TestDir dir;
    dir.createFile("same1.txt", "twin");
    dir.createFile("same2.txt", "twin");
    dir.createFile("folder/inside.bin", "12345678");
    QImage image(3, 3, QImage::Format_RGB32);
    image.fill(Qt::green);
    QVERIFY(image.save(dir.path() + QStringLiteral("/pic.png")));

    QTimer answerer;
    answerer.setInterval(100);
    connect(&answerer, &QTimer::timeout, this, [&]() {
        if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
            input->setTextValue(QStringLiteral("jpg"));
            input->accept();
        } else if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    answerer.start();
    DolphinMainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    DolphinView *view = window.activeViewContainer()->view();
    QTRY_COMPARE(view->itemsCount(), 4);

    // Folder Sizes: the dialog lists the folder with its size, worked out in the background.
    actions->action(QStringLiteral("folder_sizes"))->trigger();
    FolderSizesDialog *sizes = nullptr;
    QTRY_VERIFY((sizes = [&]() -> FolderSizesDialog * {
        for (QWidget *top : QApplication::topLevelWidgets()) {
            if (auto *found = qobject_cast<FolderSizesDialog *>(top)) {
                return found;
            }
        }
        return nullptr;
    }()));
    auto *table = sizes->findChild<QTableWidget *>();
    QVERIFY(table);
    QTRY_VERIFY(table->rowCount() == 1 && table->item(0, 1) && table->item(0, 1)->data(Qt::UserRole).toLongLong() == 8);
    QCOMPARE(table->item(0, 0)->text(), QStringLiteral("folder"));
    sizes->close();

    // Find Duplicate Files: the dialog lists the two copies as one group.
    actions->action(QStringLiteral("find_duplicates"))->trigger();
    DuplicateFinderDialog *finder = nullptr;
    QTRY_VERIFY((finder = [&]() -> DuplicateFinderDialog * {
        for (QWidget *top : QApplication::topLevelWidgets()) {
            if (auto *found = qobject_cast<DuplicateFinderDialog *>(top)) {
                return found;
            }
        }
        return nullptr;
    }()));
    auto *tree = finder->findChild<QTreeWidget *>();
    QVERIFY(tree);
    QTRY_COMPARE(tree->topLevelItemCount(), 1);
    QCOMPARE(tree->topLevelItem(0)->childCount(), 2);
    finder->close();

    // Convert Images: the selected picture is saved again as a .jpg.
    view->selectAll();
    QTRY_COMPARE(view->selectedItemsCount(), 4);
    actions->action(QStringLiteral("convert_images"))->trigger();
    QTRY_VERIFY(QFile::exists(dir.path() + QStringLiteral("/pic.jpg")));
    answerer.stop();
}

void DolphinCommanderFeaturesTest::testFoldersCanAlwaysBeSortedByName()
{
    TestDir dir;
    const QDateTime now = QDateTime::currentDateTime();
    dir.createDir(QStringLiteral("alpha"), now.addDays(-10));
    dir.createDir(QStringLiteral("zulu"), now.addDays(-5));
    dir.createDir(QStringLiteral("mike"), now.addDays(-1));
    DolphinViewContainer container(dir.url(), nullptr);
    container.show();
    QTRY_COMPARE(container.view()->itemsCount(), 3);
    const auto names = [&]() {
        QStringList result;
        for (const KFileItem &item : container.view()->items()) {
            result << item.name();
        }
        return result;
    };
    container.view()->setSortRole("modificationtime");
    container.view()->setSortOrder(Qt::DescendingOrder);
    QTRY_COMPARE(names(), (QStringList{QStringLiteral("mike"), QStringLiteral("zulu"), QStringLiteral("alpha")}));

    KFileItemModel::setFoldersAlwaysSortedByName(true);
    container.view()->setSortOrder(Qt::AscendingOrder);
    container.view()->setSortOrder(Qt::DescendingOrder);
    QTRY_COMPARE(names(), (QStringList{QStringLiteral("alpha"), QStringLiteral("mike"), QStringLiteral("zulu")}));
    KFileItemModel::setFoldersAlwaysSortedByName(false);
}

void DolphinCommanderFeaturesTest::testTheNotificationsPanelListsTheMessages()
{
    TestDir dir;
    dir.createFile("x");
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    auto *list = window.findChild<QListWidget *>(QStringLiteral("notificationsList"));
    QVERIFY(list);
    QAction *panel = window.actionCollection()->action(QStringLiteral("show_notifications_panel"));
    QVERIFY(panel);
    QVERIFY(!list->isVisible());
    panel->trigger();
    QTRY_VERIFY(list->isVisible());

    const int before = list->count();
    window.activeViewContainer()->showMessage(QStringLiteral("a message for the panel"), KMessageWidget::Information);
    QCOMPARE(list->count(), before + 1);
    QVERIFY(list->item(0)->text().endsWith(QStringLiteral("a message for the panel")));
}

void DolphinCommanderFeaturesTest::testAToDoIsCheckedOffByClickingItsCheckbox()
{
    TestDir dir;
    dir.createFile("walk the dog.2do", "dog");
    DolphinViewContainer container(dir.url(), nullptr);
    container.resize(700, 400);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));
    QTRY_COMPARE(container.view()->itemsCount(), 1);
    QTest::qWait(500);

    auto *list = container.view()->findChild<KItemListContainer *>();
    QVERIFY(list);
    auto *graphicsView = list->findChild<QGraphicsView *>();
    QVERIFY(graphicsView);
    const QRectF row = list->controller()->view()->itemRect(0);
    const QPointF box = KItemListWidget::todoCheckboxRect(row).center();
    // A click beside the checkbox does nothing.
    QTest::mouseClick(graphicsView->viewport(), Qt::LeftButton, Qt::NoModifier, graphicsView->mapFromScene(QPointF(row.left() + 30, row.center().y())));
    QVERIFY(QFile::exists(dir.path() + QStringLiteral("/walk the dog.2do")));
    // A checkbox is painted there: the pixels differ from the row of a file that is not a to-do, and from the checked one after the click.
    const QRect boxRect = graphicsView->mapFromScene(KItemListWidget::todoCheckboxRect(row)).boundingRect();
    const QImage open = graphicsView->viewport()->grab(boxRect).toImage();
    QTest::mouseClick(graphicsView->viewport(), Qt::LeftButton, Qt::NoModifier, graphicsView->mapFromScene(box));
    QTRY_VERIFY(QFile::exists(dir.path() + QStringLiteral("/walk the dog.2dx")));
    QVERIFY(!QFile::exists(dir.path() + QStringLiteral("/walk the dog.2do")));
    QTest::qWait(1000);
    const QImage done = graphicsView->viewport()->grab(boxRect).toImage();
    QVERIFY(open != done);
    // Neither is blank: both show a drawn box.
    const auto colors = [](const QImage &image) {
        QSet<QRgb> found;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                found.insert(image.pixel(x, y));
            }
        }
        return found.size();
    };
    QVERIFY2(colors(open) > 2 && colors(done) > 2, qPrintable(QStringLiteral("colors %1 and %2").arg(colors(open)).arg(colors(done))));
}

void DolphinCommanderFeaturesTest::testTheColorThemesRecolorTheProgram()
{
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    auto *themes = qobject_cast<KActionMenu *>(window.actionCollection()->action(QStringLiteral("color_theme")));
    QVERIFY(themes);
    const QList<QAction *> choices = themes->menu()->actions();
    QCOMPARE(choices.size(), 5); // the three themes, a separator and "Custom Colors…"
    const QColor before = QApplication::palette().color(QPalette::Base);

    choices.at(2)->trigger(); // Dark
    QVERIFY(QApplication::palette().color(QPalette::Base).lightnessF() < 0.3);
    choices.at(1)->trigger(); // Light
    QVERIFY(QApplication::palette().color(QPalette::Base).lightnessF() > 0.9);
    choices.at(0)->trigger(); // Follow the desktop
    QCOMPARE(QApplication::palette().color(QPalette::Base), before);
    QCOMPARE(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ColorTheme"), QStringLiteral("system"));
}

void DolphinCommanderFeaturesTest::testItemScrollingStepsByRows()
{
    TestDir dir;
    for (int i = 0; i < 60; ++i) {
        dir.createFile(QStringLiteral("file %1").arg(i, 2, 10, QLatin1Char('0')));
    }
    DolphinViewContainer container(dir.url(), nullptr);
    container.resize(500, 300);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));
    QTRY_COMPARE(container.view()->itemsCount(), 60);
    auto *list = container.view()->findChild<KItemListContainer *>();
    QVERIFY(list);
    KItemListView *view = list->controller()->view();
    const qreal ofText = view->scrollSingleStep();
    KItemListView::setItemScrolling(true);
    const qreal ofRow = view->scrollSingleStep();
    KItemListView::setItemScrolling(false);
    QCOMPARE(ofRow, view->itemSize().height());
    QVERIFY(ofRow != ofText);
}

void DolphinCommanderFeaturesTest::testASavedFileViewKeepsTheColumnWidths()
{
    TestDir dir;
    dir.createFile("x");
    QTimer answerer;
    answerer.setInterval(100);
    connect(&answerer, &QTimer::timeout, this, [&]() {
        if (auto *input = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
            input->setTextValue(QStringLiteral("wide names"));
            input->accept();
        } else if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    answerer.start();
    DolphinMainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    KActionCollection *actions = window.actionCollection();
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 1);
    // Reading the settings again brings back what is stored: it has to be stored.
    GeneralSettings::setGlobalViewProps(false);
    GeneralSettings::self()->save();

    const int columns = window.activeViewContainer()->view()->visibleRoles().count();
    QVERIFY(columns >= 2);
    QList<int> wide;
    for (int i = 0; i < columns; ++i) {
        wide << 420 - i * 60;
    }
    {
        ViewProperties props(dir.url());
        props.setViewMode(DolphinView::DetailsView);
        props.setHeaderColumnWidths(wide);
        props.save();
    }
    QCOMPARE(ViewProperties(dir.url()).headerColumnWidths(), wide);
    actions->action(QStringLiteral("save_file_view"))->trigger();

    // Another set of widths is replaced by the saved view.
    {
        ViewProperties props(dir.url());
        props.setHeaderColumnWidths(QList<int>(columns, 100));
        props.save();
    }
    auto *fileViews = qobject_cast<KActionMenu *>(actions->action(QStringLiteral("file_views")));
    QVERIFY(fileViews);
    Q_EMIT fileViews->menu()->aboutToShow();
    QAction *saved = nullptr;
    for (QAction *action : fileViews->menu()->actions()) {
        if (action->text() == QStringLiteral("wide names")) {
            saved = action;
        }
    }
    QVERIFY(saved);
    saved->trigger();
    GeneralSettings::setGlobalViewProps(false);
    QCOMPARE(ViewProperties(dir.url()).headerColumnWidths(), wide);
    answerer.stop();
}

void DolphinCommanderFeaturesTest::testACustomThemeUsesTheColorsThatWereChosen()
{
    // Four colors are asked for in turn: the dialogs are answered with these.
    const QList<QColor> answers{QColor(10, 20, 30), QColor(200, 210, 220), QColor(1, 2, 3), QColor(250, 0, 0)};
    int answered = 0;
    QTimer answerer;
    answerer.setInterval(100);
    connect(&answerer, &QTimer::timeout, this, [&]() {
        if (auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget())) {
            dialog->setCurrentColor(answers.at(qMin(answered, 3)));
            ++answered;
            dialog->accept();
        } else if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    answerer.start();
    DolphinMainWindow window;
    auto *themes = qobject_cast<KActionMenu *>(window.actionCollection()->action(QStringLiteral("color_theme")));
    QVERIFY(themes);
    const QColor before = QApplication::palette().color(QPalette::Base);
    QAction *custom = themes->menu()->actions().last();
    QCOMPARE(custom->text(), QStringLiteral("Custom Colors…"));
    custom->trigger();
    QCOMPARE(answered, 4);
    QCOMPARE(QApplication::palette().color(QPalette::Window), answers.at(0));
    QCOMPARE(QApplication::palette().color(QPalette::Base), answers.at(1));
    QCOMPARE(QApplication::palette().color(QPalette::Text), answers.at(2));
    QCOMPARE(QApplication::palette().color(QPalette::Highlight), answers.at(3));
    QCOMPARE(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander")).readEntry("ColorTheme"), QStringLiteral("custom"));
    // Back to the desktop's colors for the tests that follow.
    themes->menu()->actions().first()->trigger();
    QCOMPARE(QApplication::palette().color(QPalette::Base), before);
    answerer.stop();
}

void DolphinCommanderFeaturesTest::testTheFileViewDesignerEditsAndAppliesAView()
{
    // The dialog on its own: the shown columns come first and moving and ticking change what it answers.
    // It is gone before the one of the menu is looked for among the windows of the program.
    {
        FileViewDesignerDialog standalone({"text", "size", "modificationtime"}, {300, 80, 150}, nullptr);
        QCOMPARE(standalone.roles(), (QList<QByteArray>{"text", "size", "modificationtime"}));
        QCOMPARE(standalone.widths(), (QList<int>{300, 80, 150}));
        auto *table = standalone.findChild<QTableWidget *>();
        QVERIFY(table);
        QVERIFY(table->rowCount() > 3);
        // The name stays first: it cannot be moved, nor can a column go above it.
        table->setCurrentCell(0, 1);
        const auto click = [&](QDialog &dialog, const QString &text) {
            for (QPushButton *button : dialog.findChildren<QPushButton *>()) {
                if (button->text() == text) {
                    button->click();
                }
            }
        };
        click(standalone, QStringLiteral("Move Down"));
        QCOMPARE(standalone.roles(), (QList<QByteArray>{"text", "size", "modificationtime"}));
        table->setCurrentCell(1, 1);
        click(standalone, QStringLiteral("Move Up"));
        QCOMPARE(standalone.roles(), (QList<QByteArray>{"text", "size", "modificationtime"}));
        table->setCurrentCell(1, 1);
        click(standalone, QStringLiteral("Move Down"));
        QCOMPARE(standalone.roles(), (QList<QByteArray>{"text", "modificationtime", "size"}));
        QCOMPARE(standalone.widths(), (QList<int>{300, 150, 80})); // the widths go with their columns
        table->item(2, 0)->setCheckState(Qt::Unchecked);
        QCOMPARE(standalone.roles(), (QList<QByteArray>{"text", "modificationtime"}));
        for (QCheckBox *box : standalone.findChildren<QCheckBox *>()) {
            box->setChecked(true);
        }
        QVERIFY(standalone.widths().isEmpty());
    }

    // From the menu: the designer's answer is applied to the view and can be kept under a name.
    TestDir dir;
    dir.createFile("x");
    // The folder has its own view properties, with the usual columns, whatever other runs left behind.
    GeneralSettings::setGlobalViewProps(false);
    GeneralSettings::self()->save();
    {
        ViewProperties props(dir.url());
        props.setViewMode(DolphinView::DetailsView);
        props.setVisibleRoles({"text", "size", "modificationtime"});
        props.save();
    }
    QTimer closer;
    closer.setInterval(200);
    connect(&closer, &QTimer::timeout, this, [&]() {
        if (QWidget *modal = QApplication::activeModalWidget()) {
            modal->close();
        }
    });
    closer.start();
    DolphinMainWindow window;
    closer.stop();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.openDirectories({dir.url().toString()}, false);
    QTRY_VERIFY(window.activeViewContainer());
    QTRY_COMPARE(window.activeViewContainer()->view()->itemsCount(), 1);
    GeneralSettings::setGlobalViewProps(false);
    GeneralSettings::self()->save();
    window.activeViewContainer()->view()->setVisibleRoles({"text", "size", "modificationtime"});
    QTRY_COMPARE(window.activeViewContainer()->view()->visibleRoles(), (QList<QByteArray>{"text", "size", "modificationtime"}));

    window.actionCollection()->action(QStringLiteral("design_file_view"))->trigger();
    FileViewDesignerDialog *designer = nullptr;
    QTRY_VERIFY((designer = [&]() -> FileViewDesignerDialog * {
        for (QWidget *top : QApplication::topLevelWidgets()) {
            if (auto *found = qobject_cast<FileViewDesignerDialog *>(top)) {
                return found;
            }
        }
        return nullptr;
    }()));
    auto *rows = designer->findChild<QTableWidget *>();
    QVERIFY(rows);
    // Swap the two columns after the name and drop the last: the rows start with the columns the view shows.
    const QList<QByteArray> start = designer->roles();
    QVERIFY2(start.size() >= 3,
             qPrintable(QStringLiteral("shown %1; view %2")
                            .arg(QString::fromLatin1(start.join(',')), QString::fromLatin1(window.activeViewContainer()->view()->visibleRoles().join(',')))));
    QCOMPARE(start.first(), QByteArray("text"));
    rows->setCurrentCell(2, 1);
    for (QPushButton *button : designer->findChildren<QPushButton *>()) {
        if (button->text() == QStringLiteral("Move Up")) {
            button->click();
        }
    }
    rows->item(2, 0)->setCheckState(Qt::Unchecked);
    for (QCheckBox *box : designer->findChildren<QCheckBox *>()) {
        box->setChecked(false);
    }
    // The rows are now name, the former third column, the former second one, ...: the one dropped is the former second.
    for (int row = 0; row < rows->rowCount(); ++row) {
        static_cast<QSpinBox *>(rows->cellWidget(row, 2))->setValue(200 + row);
    }
    designer->findChild<QLineEdit *>()->setText(QStringLiteral("size first"));
    const QList<QByteArray> expectedRoles = designer->roles();
    const QList<int> expectedWidths = designer->widths();
    QCOMPARE(expectedRoles, (QList<QByteArray>{start.at(0), start.at(2)}));
    QCOMPARE(expectedWidths.size(), 2);
    designer->accept();

    QTRY_COMPARE(window.activeViewContainer()->view()->visibleRoles(), expectedRoles);
    GeneralSettings::setGlobalViewProps(false);
    QCOMPARE(ViewProperties(dir.url()).headerColumnWidths(), expectedWidths);
    const QStringList saved = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("Commander FileViews")).readEntry("size first", QStringList());
    QVERIFY(!saved.isEmpty());
    QVERIFY(saved.contains(QString::fromLatin1(start.at(2))));
}

void DolphinCommanderFeaturesTest::testColumnsCanBeHiddenWhenTheViewIsNarrow()
{
    TestDir dir;
    dir.createFile("x");
    GeneralSettings::setGlobalViewProps(false);
    GeneralSettings::self()->save();
    {
        ViewProperties props(dir.url());
        props.setViewMode(DolphinView::DetailsView);
        props.setVisibleRoles({"text", "size", "modificationtime", "type"});
        props.save();
    }
    DolphinViewContainer container(dir.url(), nullptr);
    container.resize(1000, 300);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));
    QTRY_COMPARE(container.view()->itemsCount(), 1);
    auto *list = container.view()->findChild<KItemListContainer *>();
    QVERIFY(list);
    const auto shown = [&]() {
        return list->controller()->view()->visibleRoles().count();
    };
    QTRY_COMPARE(shown(), 4);

    DolphinView::setHideColumnsWhenNarrow(true);
    container.view()->updateResponsiveColumns();
    QTRY_COMPARE(shown(), 4); // 1000 pixels are enough for all of them
    container.resize(520, 300);
    QTRY_COMPARE(shown(), 2); // the name and one more
    container.resize(300, 300);
    QTRY_COMPARE(shown(), 1); // only the name
    // The folder's own settings still have all the columns.
    QCOMPARE(ViewProperties(dir.url()).visibleRoles().count(), 4);
    container.resize(1000, 300);
    QTRY_COMPARE(shown(), 4);
    DolphinView::setHideColumnsWhenNarrow(false);
    container.resize(300, 300);
    QTRY_COMPARE(shown(), 4); // and with the setting off, nothing is hidden
}

void DolphinCommanderFeaturesTest::testEachColumnCanHaveItsOwnHideWidth()
{
    TestDir dir;
    dir.createFile("x");
    GeneralSettings::setGlobalViewProps(false);
    GeneralSettings::self()->save();
    {
        ViewProperties props(dir.url());
        props.setViewMode(DolphinView::DetailsView);
        props.setVisibleRoles({"text", "size", "modificationtime", "type"});
        props.save();
    }
    DolphinViewContainer container(dir.url(), nullptr);
    container.resize(1000, 300);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));
    QTRY_COMPARE(container.view()->itemsCount(), 1);
    auto *list = container.view()->findChild<KItemListContainer *>();
    QVERIFY(list);
    const auto shown = [&]() {
        return list->controller()->view()->visibleRoles();
    };
    QTRY_COMPARE(shown().count(), 4);

    // The dialog asks for the thresholds and answers with them by column.
    FileViewDesignerDialog designer({"text", "size", "modificationtime", "type"}, {}, nullptr);
    designer.setHideBelow({{"size", 900}, {"type", 400}});
    QCOMPARE(designer.hideBelow(), (QList<int>{0, 900, 0, 400}));

    // The type is to stay down to 400 pixels, the size is dropped under 900, the date follows the automatic rule.
    DolphinView::setColumnHideThresholds({{"size", 900}, {"type", 400}});
    DolphinView::setHideColumnsWhenNarrow(true);
    container.view()->updateResponsiveColumns();
    QTRY_COMPARE(shown(), (QList<QByteArray>{"text", "size", "modificationtime", "type"}));
    container.resize(700, 300);
    QTRY_COMPARE(shown(), (QList<QByteArray>{"text", "modificationtime", "type"}));
    container.resize(380, 300);
    QTRY_COMPARE(shown(), (QList<QByteArray>{"text"}));
    DolphinView::setColumnHideThresholds({});
    DolphinView::setHideColumnsWhenNarrow(false);
}

void DolphinCommanderFeaturesTest::testConvertImage()
{
    TestDir dir;
    QImage image(4, 4, QImage::Format_RGB32);
    image.fill(Qt::red);
    QVERIFY(image.save(dir.path() + QStringLiteral("/pic.png")));
    const QString jpg = CommanderTools::convertImage(dir.path() + QStringLiteral("/pic.png"), QStringLiteral("jpg"));
    QVERIFY(jpg.endsWith(QStringLiteral("/pic.jpg")));
    QVERIFY(QFile::exists(jpg));
    // Not overwritten, and no image is no conversion.
    QVERIFY(CommanderTools::convertImage(dir.path() + QStringLiteral("/pic.png"), QStringLiteral("jpg")).isEmpty());
    dir.createFile("text", "hello");
    QVERIFY(CommanderTools::convertImage(dir.path() + QStringLiteral("/text"), QStringLiteral("png")).isEmpty());
}

void DolphinCommanderFeaturesTest::testToDoNames()
{
    QCOMPARE(CommanderTools::toggledToDoName(QStringLiteral("buy milk.2do")), QStringLiteral("buy milk.2dx"));
    QCOMPARE(CommanderTools::toggledToDoName(QStringLiteral("buy milk.2dx")), QStringLiteral("buy milk.2do"));
    QCOMPARE(CommanderTools::toggledToDoName(QStringLiteral("notes.txt")), QStringLiteral("notes.txt"));
}

void DolphinCommanderFeaturesTest::testFolderSize()
{
    TestDir dir;
    dir.createFile("a", "12345");
    dir.createFile("sub/deep/b", "123");
    QCOMPARE(CommanderTools::folderSize(dir.path()), qint64(8));
    QCOMPARE(CommanderTools::folderSize(dir.path() + QStringLiteral("/sub")), qint64(3));
}

void DolphinCommanderFeaturesTest::testFindDuplicates()
{
    TestDir dir;
    dir.createFile("a", "same");
    dir.createFile("sub/b", "same");
    dir.createFile("c", "diff");
    dir.createFile("d", "sam3");
    dir.createFile("e", "");
    dir.createFile("f", "");
    const QList<QStringList> groups = CommanderTools::findDuplicates(dir.path());
    QCOMPARE(groups.size(), 1);
    QCOMPARE(groups.first().size(), 2);
    QVERIFY(groups.first().first().endsWith(QStringLiteral("/a")));
    QVERIFY(groups.first().last().endsWith(QStringLiteral("/sub/b")));
}

void DolphinCommanderFeaturesTest::testATabDroppedAmongPinnedTabsGoesAfterThem()
{
    QWidget window;
    auto *layout = new QHBoxLayout(&window);
    auto *left = new DolphinTabWidget(new DolphinNavigatorsWidgetAction(&window), &window);
    auto *right = new DolphinTabWidget(new DolphinNavigatorsWidgetAction(&window), &window);
    layout->addWidget(left);
    layout->addWidget(right);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    left->openNewActivatedTab(m_testDir->url());
    left->openNewActivatedTab(m_testDir->url());
    right->openNewActivatedTab(m_testDir->url());
    right->openNewActivatedTab(m_testDir->url());
    auto *rightBar = static_cast<DolphinTabBar *>(right->tabBar());
    rightBar->setPinned(0, true);
    DolphinTabPage *moved = left->tabPageAt(1);

    QVERIFY(QMetaObject::invokeMethod(left, "moveTabToOtherSide", Q_ARG(int, 1), Q_ARG(DolphinTabBar *, rightBar), Q_ARG(int, 0)));

    QCOMPARE(right->tabPageAt(1), moved);
    QVERIFY(rightBar->isPinned(0));
    QVERIFY(!rightBar->isPinned(1));
}

QTEST_MAIN(DolphinCommanderFeaturesTest)

#include "dolphincommanderfeaturestest.moc"

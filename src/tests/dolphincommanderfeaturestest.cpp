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
#include "kitemviews/kitemlistwidget.h"
#include "testdir.h"
#include "views/columnsstrip.h"
#include "views/dolphincolumnsview.h"
#include "views/dolphinview.h"
#include "views/viewheader.h"

#include <KActionCollection>
#include <KCoreDirLister>
#include <KFileMetaData/UserMetaData>

#include <KIO/CopyJob>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QImage>
#include <QMimeData>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>
#include <QToolButton>

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

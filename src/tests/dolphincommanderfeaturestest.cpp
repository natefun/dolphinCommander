/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Tests what dolphinCommander adds to Dolphin or changes in it, so that a merge of a newer
 * Dolphin that breaks one of these features fails here. Dolphin's own tests test Dolphin.
 */

#include "commandertools.h"
#include "dolphin_generalsettings.h"
#include "dolphinnavigatorswidgetaction.h"
#include "dolphintabbar.h"
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include "testdir.h"
#include "views/columnsstrip.h"
#include "views/dolphincolumnsview.h"
#include "views/dolphinview.h"
#include "views/viewheader.h"

#include <KCoreDirLister>

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
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

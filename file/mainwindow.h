#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QTime>
#include <QFile>
#include <QPixmap>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "shooter.h"
#include "soldier.h"
#include "dog.h"
#include "tank.h"
#include "ground.h"
#include "carrot.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct TowerInfo {
    int id;
    int cost;
};

struct MobSpawnInfo {
    int spawnId;
    int type;
    int hp;
};

struct SpawnPoint {
    QPointF position;
    QList<Checkpoint> checkpoints;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void enterShooterMenu(Shooter* s);
    void exitShooterMenu();
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QTimer *waveIntervalTimer;
    QTimer *spawnTimer;
    QGraphicsScene *scene = nullptr;
    QList<BaseMob*> *mobList;
    QGraphicsView *view;
    bool shooterMenuMode = false;
    Shooter* selectedShooter = nullptr;
    QList<QGraphicsItem*> shooterCircles;
    QGraphicsRectItem* maskItem = nullptr;
    QGraphicsPixmapItem* upIconItem = nullptr;
    QGraphicsSimpleTextItem* upTextItem = nullptr;
    QList<QGraphicsItem*> upgradeHintItems;
    void updateShooterMenuAppearance();
    void clearUpgradeHints();
    void updateUpgradeHints();

    // 塔选择栏
    QList<TowerInfo> availableTowers;
    int selectedTower = -1;
    QList<QGraphicsItem*> towerBarItems;
    QList<QGraphicsItem*> towerCards;
    void updateTowerBarAppearance();

    // 顶部工具栏
    QWidget *topBar;
    QLabel *goldIcon;
    QLabel *goldText;
    QLabel *waveText;
    QLabel *countdownLabel;
    QMovie *countdownMovie;
    QPushButton *speedBtn;
    QPushButton *pauseBtn;
    QPushButton *menuBtn;
    bool doubleSpeed = false;
    int timerInterval = 32;
    int spawnInterval = 1000;
    int waveInterval = 5000;
    int gold = 200;
    int goldPerType[4] = {0, 14, 20, 50}; // 击杀 type 1/2/3 获得的金币
    int totalWaves = 0;
    int currentWaveIndex = -1;
    int spawnIndex = 0;
    Carrot *carrot = nullptr;
    bool gameStarted = false;
    bool gameOver = false;
    bool paused = false;
    int spawnRemaining = -1;   // -1 = 暂停时 spawnTimer 未运行
    int waveRemaining = -1;

    // 开始菜单
    QWidget *menuPage;
    QLabel *menuTitle;
    QPushButton *playBtn;
    QPushButton *exitBtn;
    void showMenuPage();
    void startGame();
    void onMenuReturnClicked();

    // 选关页面
    QWidget *levelSelectPage;
    QList<QPushButton*> levelButtons;
    int unlockedLevelCount = 1;
    void showLevelSelectPage();
    void loadLevel(int idx);
    void clearGameObjects();
    QString progressFilePath() const;
    void loadUnlockProgress();
    void saveUnlockProgress() const;
    void unlockNextLevelIfQualified();
    void updateLevelButtons();

    // 地图数据
    QList<ground*> groundList;
    QList<QGraphicsItem*> routeConnectorItems;
    QList<QGraphicsPixmapItem*> spawnIconList;
    QList<SpawnPoint> spawnPoints;
    int currentLevel = 0;
    QList<QList<MobSpawnInfo>> waves;

    void doPause();
    void cleanupGame();
    void resetAndStart();

    void loadTowers(const QString &path);
    void buildTowerBar();
    void clearRouteConnectors();
    void buildRouteConnectors();
    void startNextWave();
    void spawnNextMonster();
    void updateGoldDisplay();
    void updateWaveDisplay();
    void togglePause();
};
#endif // MAINWINDOW_H

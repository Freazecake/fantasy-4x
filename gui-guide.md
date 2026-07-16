# Гайд по GUI: Qt для фэнтезийной 4X-стратегии

## 0. Общая картина

Вся игровая логика (недели 2-3 по `roadmap.md`) уже готова и покрыта 136 тестами:
`Race`, `Settlement`, `Building`, `Technology`, `Hero`, `Economy`, `Military`,
`PoliticalAction`, `TradeMarket`, `Player` — всё работает в консоли, без единого
окна. GUI ничего не меняет в этой логике — он её только **показывает** и даёт
по ней **кликать**. Это радикально снижает риск: если GUI где-то сломается,
экономика и ИИ не пострадают, потому что находятся в другом слое.

По `roadmap.md` GUI — это две недели:
- **Неделя 1** — Qt-база + кликабельная сетка карты, без игровой логики вообще.
- **Неделя 4** — связка уже готового `core/` с этой GUI (панели поселения,
  дерева технологий, конца хода).

Этот файл разворачивает обе недели подробнее, с реальными сигнатурами из
твоего текущего `core/`, и объясняет Qt-специфику, с которой ты ещё не
сталкивался — сигналы/слоты и модель владения объектами.

**Приоритет при любом выборе** (см. `project-instructions.md`): рабочий
геймплей > простота реализации при твоём уровне Qt > архитектурная чистота >
внешний вид. Ниже везде выбран самый простой вариант, а не самый "красивый".

---

## 1. Минимальный набор Qt — что реально понадобится

| Компонент | Зачем | Важность |
|---|---|---|
| `QApplication`, `QMainWindow`, `QWidget` | скелет приложения и окна | обязательно |
| `QVBoxLayout`/`QHBoxLayout` | расположение виджетов в панелях | обязательно |
| Signals & Slots (`connect(...)`) | единственный способ, которым GUI "узнаёт" о кликах | **самое важное** |
| `QGraphicsView`/`QGraphicsScene`/`QGraphicsItem` | отрисовка карты | обязательно |
| `QString` | только на границе core/gui | обязательно, но по минимуму |
| `QPushButton`, `QLabel`, `QTimer` | кнопки, подписи, таймер хода (если понадобится) | пригодится |
| Qt Containers (`QVector` и т.п.) | — | **не используй**, везде `std::vector` |
| `QThread` | — | **не понадобится**, пошаговая игра не многопоточна |

### 1.1. Скелет приложения

```cpp
// src/main.cpp (GUI-версия; консольный main.cpp можно оставить под #ifdef)
#include <QApplication>
#include "gui/MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
```

`app.exec()` — это бесконечный цикл обработки событий (кликов, перерисовок и
т.д.). Пока он не вызван, ничего не происходит и окно не реагирует ни на что.

### 1.2. Signals & Slots — то, без чего Qt не понять

Это единственный механизм, которым Qt-объекты "разговаривают" друг с другом,
не зная напрямую друг о друге. Аналога в чистом C++/STL нет — ближе всего к
`std::function`-колбэкам, но с проверкой типов на этапе компиляции и
поддержкой множественных подписчиков.

**Как это работает на примере**: `MapView` не должен знать, что клик по клетке
должен открыть панель поселения — это дело `MainWindow`. Поэтому `MapView`
просто **испускает сигнал** "по такой-то клетке кликнули", а кто угодно может
на него подписаться (**слот**).

```cpp
// gui/MapView.hpp
class MapView : public QGraphicsView
{
    Q_OBJECT   // ОБЯЗАТЕЛЬНО для любого класса с сигналами/слотами — без этого
               // макроса код скомпилируется, но упадёт на линковке с ошибкой
               // "undefined reference to vtable for MapView". Это не опечатка,
               // а частая первая ошибка у всех новичков в Qt.
public:
    explicit MapView(QWidget *parent = nullptr);
    void renderMap(const Map &map);   // читает core::Map, ничего не меняет в core

signals:
    void tileClicked(int q, int r);   // сигналы не имеют тела — компилятор его сгенерирует

protected:
    void mousePressEvent(QMouseEvent *event) override;
};
```

```cpp
// gui/MainWindow.hpp
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onTileClicked(int q, int r);   // слот — обычный метод, просто помеченный секцией slots

private:
    MapView *m_mapView;
};
```

```cpp
// gui/MainWindow.cpp
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_mapView = new MapView(this);
    setCentralWidget(m_mapView);

    // Новый (проверяемый компилятором) синтаксис connect — используй ТОЛЬКО его.
    connect(m_mapView, &MapView::tileClicked, this, &MainWindow::onTileClicked);
}

void MainWindow::onTileClicked(int q, int r)
{
    statusBar()->showMessage(QString("Клетка (%1, %2)").arg(q).arg(r));
}
```

Есть старый синтаксис `connect(obj, SIGNAL(clicked()), this, SLOT(onClick()))`
со строками — **не используй его**: опечатка в названии сигнала там не
поймается компилятором, а тихо не сработает в рантайме. `&MapView::tileClicked`
компилятор проверяет как обычный указатель на метод.

### 1.3. QGraphicsView / QGraphicsScene / QGraphicsItem

Три разных вещи, которые новички обычно путают:

- **`QGraphicsScene`** — "модель данных" карты: список фигур (`QGraphicsItem`) с
  координатами. Сама по себе не рисуется на экране.
- **`QGraphicsView`** — виджет-окно, который показывает `Scene` (можно
  приблизить/прокрутить). Это то, что реально стоит в layout'е.
  Обычно достаточно **одного** View на одну Scene.
- **`QGraphicsItem`** (и подклассы вроде `QGraphicsRectItem`) — отдельная
  фигура на сцене: одна клетка карты — один `QGraphicsRectItem`.

**Важный практический момент**: `QGraphicsItem` **не** является `QObject` и
не умеет сигналы/слоты напрямую (есть `QGraphicsObject`, но это лишний вес
ради одной клетки). Проще всего ловить клики не на уровне отдельной клетки, а
переопределив `mousePressEvent` у самого `MapView` и вычислив клетку по
пиксельным координатам клика:

```cpp
// gui/MapView.cpp
namespace {
constexpr int TILE_SIZE = 40;   // размер клетки в пикселях — подбери сам
}

MapView::MapView(QWidget *parent) : QGraphicsView(parent)
{
    setScene(new QGraphicsScene(this));   // parent = this -> Qt сам удалит Scene вместе с View
}

void MapView::renderMap(const Map &map)
{
    scene()->clear();
    for (const Tile &tile : map.getMap())
    {
        Coords c = tile.getCoords();
        auto *rect = new QGraphicsRectItem(c.q * TILE_SIZE, c.r * TILE_SIZE,
                                            TILE_SIZE, TILE_SIZE);
        rect->setBrush(tile.getType() == TileType::Water ? Qt::blue : Qt::green);
        scene()->addItem(rect);   // Scene теперь ВЛАДЕЕТ rect — вручную delete не нужен
                                   // (и НЕЛЬЗЯ, см. раздел 9 про parent-child)
    }
}

void MapView::mousePressEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());   // из пикселей окна в координаты сцены
    int q = static_cast<int>(scenePos.x()) / TILE_SIZE;
    int r = static_cast<int>(scenePos.y()) / TILE_SIZE;
    emit tileClicked(q, r);   // испускаем сигнал — кто угодно может подписаться
    QGraphicsView::mousePressEvent(event);   // не забыть вызвать базовую реализацию
}
```

### 1.4. Граница core/gui: `QString` ↔ `std::string`

Единственное место, где это нужно — когда текст из `core/` (например,
`Building::getName()`, который возвращает `std::string`) нужно показать в
`QLabel`/`QPushButton`:

```cpp
label->setText(QString::fromStdString(building.getName()));
```

В обратную сторону (`QString` → `std::string`) почти не понадобится — core/
не принимает Qt-типы вообще, только id-строки, которые проще собирать как
`std::string` сразу (например, `building.getId()` уже `std::string`, его
никуда конвертировать не нужно).

---

## 2. Как быстро научиться (порядок)

1. Официальный туториал **"Qt Widgets and Layouts"** (~1-2 часа) — собери
   `QMainWindow` с меню и статус-баром руками, не копируя, чтобы разобраться
   в системе layout'ов.
2. Туториал по **QGraphicsView tile-based grid** (в поиске: "Qt QGraphicsView
   tile grid tutorial") — под конкретно твою задачу карты.
3. Signals & Slots — 10-15 минут документации на `doc.qt.io/qt-6/signalsandslots.html`
   (или qt-5, если ставишь Qt5), дальше это база, которой пользуешься
   не думая.

Больше по Qt отдельно учить не нужно — всё остальное в этом файле уже
покрывает то, что реально потребуется.

---

## 3. Настройка окружения: Qt + CMake

Твой текущий `CMakeLists.txt` вообще не знает про Qt (собирает консольную
версию). Вот что нужно добавить:

```cmake
cmake_minimum_required(VERSION 3.14)
project("game")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt-специфика ---
set(CMAKE_AUTOMOC ON)   # автогенерация кода для Q_OBJECT/сигналов-слотов
set(CMAKE_AUTOUIC ON)   # понадобится, только если будешь использовать Qt Designer (.ui файлы) — см. ниже
set(CMAKE_AUTORCC ON)   # понадобится для иконок/ресурсов через .qrc — можно не включать, если их не будет

find_package(Qt6 COMPONENTS Widgets REQUIRED)
# Если у тебя установлен Qt5, а не Qt6 — замени на:
# find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "include/*.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    src
    assets
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
# Для Qt5: target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)

enable_testing()
add_subdirectory(tests)   # tests/ по-прежнему не тянет Qt вообще — это специально
```

**Про `.ui`-файлы (Qt Designer)**: в этом гайде везде используется вариант
"строить интерфейс кодом" (`new QVBoxLayout`, `new QPushButton` и т.д.), а не
визуальный конструктор Qt Designer. Designer быстрее визуально, но прячет
код в сгенерированный файл — для проекта, где приоритет "рабочий геймплей
при слабом знании Qt", код руками проще отлаживать и понятнее на защите. Если
захочешь Designer — это отдельный, не описанный здесь путь, и `CMAKE_AUTOUIC`
тогда обязателен.

**Если CMake не находит Qt**: добавь путь к установке Qt через
`-DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/mingw_64"` (Windows) при конфигурации, или
пропиши этот путь в `CMakeLists.txt` через `set(CMAKE_PREFIX_PATH ...)` перед
`find_package`.

---

## 4. Архитектура `gui/` — файлы и их роли

Из раздела 3 основного гайда (`4x-game-project-guide.md`):

```
src/gui/
├── MainWindow.h/cpp       — владеет Player'ами/GameState, собирает все панели
├── MapView.h/cpp          — QGraphicsView, рисует сетку, шлёт tileClicked(q, r)
├── SettlementPanel.h/cpp  — население по сферам, очередь построек, найм юнитов
├── TechTreePanel.h/cpp    — список технологий, клик = начать исследование
└── TurnPanel.h/cpp        — кнопка "конец хода", текущая фаза, сводка ресурсов
```

**Поток данных** (главное правило): `gui/` только **читает** `core/` через уже
готовый публичный API (`Settlement::allocation()`, `Player::resources()` и
т.д.) и **вызывает** его методы по действиям пользователя
(`settlement.assignFromIdle(...)`). Сам `gui/` никогда не решает, сколько
очков даёт постройка или прошёл ли ход — это уже посчитано в `core/`, GUI
просто дёргает готовый метод и перерисовывает то, что получилось.

```
Клик пользователя
      │
      ▼
 gui/ (сигнал → слот)
      │  вызывает Player::method(...) / Settlement::method(...)
      ▼
   core/ (меняет своё состояние)
      │
      ▼
 gui/ вызывает refresh(): читает новое состояние core/ и перерисовывает виджеты
```

`MainWindow` — единственное место, которое держит `Player`/`GameState` и
раздаёт панелям указатели/ссылки на нужные объекты `core/`. Панели сами по
себе `core/`-объектами не владеют.

---

## 5. Роадмап — неделя 1: Qt-база + кликабельная сетка

Цель недели: окно с сеткой клеток, на клик печатаются координаты. **Ни одного
класса из `core/` в этой неделе не используется** — только `Map::generateMap`
для теста отрисовки, и то без реального игрового состояния.

| Дни | Задача | Результат |
|---|---|---|
| 1 | Туториал "Qt Widgets and Layouts". Собрать пустой `QMainWindow` с меню и статус-баром | окно открывается, видно меню/статус-бар |
| 2 | Туториал по `QGraphicsView`/`QGraphicsScene` на примере grid/tile-игры | понимание Scene/View/Item |
| 3-4 | `gui/MapView.h/cpp` (см. раздел 1.3 этого файла): рисует сетку `N×M` прямоугольников, `mousePressEvent` шлёт `tileClicked(q, r)` | клик по клетке визуально что-то делает (пока просто событие) |
| 5 | В `MainWindow`: `connect(m_mapView, &MapView::tileClicked, ...)`, слот печатает координаты в статус-бар | закреплён механизм сигнал/слот на практике |
| 6 (буфер) | Прогнать `Map::generateMap(10, 10)` через `MapView::renderMap(...)`, проверить, что все клетки кликабельны и координаты верные | сетка реально соответствует `core::Map` |

**Definition of Done недели 1**: окно с сеткой N×M клеток, клик по любой
клетке печатает правильные `(q, r)` в статус-бар. Ни один `Settlement`,
`Player` или `Building` ещё не участвует.

---

## 6. Роадмап — неделя 4: связка core + GUI

Тут уже используется весь `core/`, написанный на неделях 2-3. Порядок задач
важен — панели зависят друг от друга через `MainWindow`.

| Дни | Задача | Какие методы `core/` дёргаются |
|---|---|---|
| 1 | `gui/SettlementPanel.h/cpp`: `QLabel` на каждую из 4 сфер + idle, кнопки/спинбоксы для переброски населения | `Settlement::allocation()`, `assignFromIdle(...)`, `unassignToIdle(...)`, `reallocate(...)` |
| 2 | В `SettlementPanel`: список доступных построек (`QListWidget` или кнопки), кнопка "строить", прогресс-бар (`QProgressBar`) текущей постройки | `Settlement::queueBuilding(...)`, `buildingQueue()`, `builtBuildings()` |
| 3 | В `SettlementPanel`: второй список — найм юнитов, индикация штрафа необученности (есть ли `barracks` среди `builtBuildings()`), заблокированные tier 1+ юниты | `Settlement::queueUnitRecruitment(...)`, `recruitmentQueue()`, `garrison()`, `Military::canRecruit(...)` |
| 4 | `gui/TechTreePanel.h/cpp`: список технологий (доступно/заблокировано/изучено), клик = начать исследование | `Player::technologies()`, `researchedTechIds()`, `setCurrentResearch(...)`, `currentResearchId()` |
| 5 | `gui/TurnPanel.h/cpp`: кнопка "конец хода", текущая фаза, сводка ресурсов игрока | `Player::advanceTurn()`, `Player::resources()` (`Resources::toString()` как черновой вывод, потом заменить на нормальные `QLabel`) |
| 6 | Отрисовка городов на `MapView` — клетки с поселениями подсвечены другим цветом, клик по своей клетке открывает `SettlementPanel` | `Settlement::tiles()`, связка через `tileClicked(q, r)` из недели 1 |
| 7 (буфер/ревью) | Полный проход интерфейса руками: старт партии → распределение населения → постройка → техи → найм → конец хода, без ИИ (один игрок-человек) | всё вместе |

**Definition of Done недели 4**: партия одного игрока-человека полностью
проходима через GUI — распределение населения, постройки, техи, найм юнитов,
конец хода, без единого падения.

### 6.1. Пример: `SettlementPanel` — как выглядит граница core/gui в реальном коде

```cpp
// gui/SettlementPanel.hpp
class SettlementPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettlementPanel(QWidget *parent = nullptr);
    void setSettlement(Settlement *settlement);   // GUI не владеет Settlement, только ссылается
    void refresh();                                 // перечитать core/ и перерисовать лейблы

private slots:
    void onAssignScienceClicked();

private:
    Settlement *m_settlement = nullptr;   // не владеющий указатель — время жизни у Player/GameState
    QLabel *m_scienceLabel;
    QLabel *m_idleLabel;
};
```

```cpp
// gui/SettlementPanel.cpp
void SettlementPanel::refresh()
{
    if (!m_settlement) return;

    const PopulationAllocation &alloc = m_settlement->allocation();
    m_scienceLabel->setText(QString("Наука: %1").arg(alloc.science));
    m_idleLabel->setText(QString("Свободно: %1").arg(alloc.idle));
}

void SettlementPanel::onAssignScienceClicked()
{
    if (!m_settlement) return;

    m_settlement->assignFromIdle(Sphere::Science, 1);   // core/ сам считает, core/ и меняется
    refresh();                                            // GUI просто перечитывает результат
}
```

Обрати внимание: `SettlementPanel` не пересчитывает население сам — он
вызывает готовый метод `core::Settlement` и затем просто перечитывает
получившееся состояние. Если бы `SettlementPanel` сам плюсовал/минусовал
числа населения — это было бы нарушением границы core/gui (игровая логика
внутри GUI).

---

## 7. Частые ошибки новичков в Qt (сэкономит часы отладки)

- **Забыт `Q_OBJECT`** в классе с сигналами/слотами → ошибка линковки
  `undefined reference to vtable for ClassName`. Первое, что проверять при
  такой ошибке.
- **Секции `signals:` / `private slots:`** обязательны — сигнал/слот, не
  объявленный в такой секции, не будет работать через `connect`.
- **Старый синтаксис `SIGNAL()/SLOT()`** — не используй, не проверяется
  компилятором, ошибки видны только в рантайме (и то не всегда).
- **Ручной `delete` Qt-объекта с заданным `parent`** — не делай так. У Qt
  своя модель владения: `new QWidget(this)` означает "этот виджет умрёт
  автоматически, когда умрёт `this`". Двойной `delete` (ручной + автоматический
  от родителя) — падение программы. Именно поэтому в примерах выше везде
  `new QGraphicsScene(this)`, `new QLabel(this)` и т.д. — с явным `parent`.
- **Забыт `setScene(...)`** на `QGraphicsView` — сцена создана, но не
  привязана к View, ничего не рисуется.
- **Путаница scene/view/item координат** — `event->pos()` у `QGraphicsView`
  это пиксели окна, а не координаты сцены; нужен `mapToScene(...)`
  (см. пример `mousePressEvent` выше).
- **Игровая логика заползает в `gui/`** — если в коде `SettlementPanel`
  появляется формула вида `population * 0.1f * raceModifier`, это красный
  флаг: такая формула уже есть в `core/Settlement.cpp`
  (`growPopulation`), GUI должен её вызывать, а не дублировать.

---

## 8. Чек-лист перед началом

- [ ] Qt (Widgets) установлен, `cmake --build .` собирает **пустой** Qt-проект
      (см. `Day 0` в `roadmap.md`, теперь с добавленным `find_package(Qt6 ...)`)
- [ ] Прочитан официальный туториал по Layouts
- [ ] Прочитан туториал по `QGraphicsView` на примере grid-игры
- [ ] Понятен пример `connect(...)` из раздела 1.2 этого файла — если нет,
      перечитать `doc.qt.io/qt-6/signalsandslots.html` ещё раз, не пытаться
      писать GUI без этого
- [ ] Есть чёткое понимание: `gui/` читает `core/` и вызывает его методы,
      но никогда не пересчитывает игровые формулы сама

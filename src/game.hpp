#pragma once
#include "storage.hpp"
#include "swf_data.hpp"

namespace bloons {
constexpr int width = 640, height = 480, columns = 24, rows = 13;
constexpr double frameMilliseconds = 1000.0 / swf::frameRate;
struct Vec { double x = 0, y = 0; };
struct Rect { double x, y, w, h; bool contains(Vec p) const; };
struct Level {
    int number = 0, target = 0, darts = 0;
    LevelTitle title;
    Array<Array<int, rows>, columns> cells{};
};
const Array<Level, 50>& levels();
// Values are the SWF DefineSound character IDs. No pacsong symbol is exported.
enum class Sound { StopAll=-1, Bomb=1, Ice=2, Metal=3, Clink=4, Pop=23,
    PopYellow=29, Boomerang=36, Wood=61, Rubber=73, IcePop=107,
    PopTack=116, ExtraDart=129, PopBlue=142, Throw=153, Menu=311 };
enum class Outcome { Playing, Won, Lost, Complete };
struct Input { Vec mouse; bool up = false, down = false, left = false, right = false; };
struct Balloon {
    Vec position;
    int type = 1, colour = 0, animation = 0, popFrame = -1, symbol = 0;
    bool frozen = false, popped = false, solid = false;
    Array<bool, 4> disabled{}; // left, right, top, bottom
    Rect bounds() const;
    int character() const { return symbol?symbol:swf::balloonSymbol(type,colour); }
};
struct Projectile {
    Vec position, velocity;
    double power = 0, rebounds = 0, alpha = 100, rotation = 0;
    int age = 0, checks = 0;
    bool launched = false, aiming = false, stuck = false, boomerang = false, extra = false, dead = false;
};
struct Profile {
    int unlocked = 1;
    Array<double, 50> best{};
};
class Game {
public:
    explicit Game(uint32_t seed = 1);
    void start(int number);
    void press();
    void release();
    void tick(const Input&, double elapsedMilliseconds = frameMilliseconds);
    void pop(size_t index); // Applies MovieClip.Pop followed by PopGame.PopBalloon.
    const Level* level = nullptr;
    List<Balloon,columns*rows> balloons;
    Array<Projectile, 3> darts{};
    // Each of the 312 cells can produce one burst of eight tacks or one spikey.
    StableList<Projectile,columns*rows*8> tacks;
    StableList<Projectile,columns*rows> spikeys;
    List<Sound,4096> sounds;
    Profile profile;
    Input input;
    Vec monkey, aimTip, pacman{2000, 2000};
    bool easy = false, active = false, gotBoomerang = false, gotMultishot = false, pacmanActive = false;
    int shots = 0, popped = 0, total = 0, pacmanAge = 0, throwFrame = 0;
    double power = 0, powerDirection = 1, sincePop = 0, shake = 0, score = 0, levelScore = 0, aimAngle = 0;
    Outcome outcome = Outcome::Playing;
private:
    uint32_t randomState;
    int random(int range);
    void resetDart(Projectile&);
    void launch(Projectile&, Vec target);
    void updateDart(Projectile&);
    void updateTack(Projectile&);
    void updateSpikey(Projectile&);
    void updatePacman();
    void deactivatePacman();
    void oneLessShot();
    void checkWin();
    void effect(size_t);
    bool clipPop(Balloon&);
    bool contact(const Balloon&, Vec point, double radius = 0) const;
};
}

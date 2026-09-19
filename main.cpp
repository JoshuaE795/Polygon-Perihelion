// Include
// --------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include <cmath>
#include <vector>
#include <algorithm>
#include <array>
#include <string>
#include <mach-o/dyld.h>
// --------------------------------------------------------------------

/*
Run command:

g++ -std=c++11 main.cpp -I/opt/homebrew/Cellar/sfml/2.6.1/include -o prog -L/opt/homebrew/Cellar/sfml/2.6.1/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

*/

// --------------------------------------------------------------------
// Asset paths
// --------------------------------------------------------------------
std::string get_executable_directory() {
    uint32_t size = 1024;
    std::vector<char> path(size);

    if(_NSGetExecutablePath(path.data(), &size) != 0) {
        path.resize(size);
        if(_NSGetExecutablePath(path.data(), &size) != 0) {
            return ".";
        }
    }

    std::string full_path(path.data());
    std::size_t slash = full_path.find_last_of('/');

    if(slash == std::string::npos) {
        return ".";
    }

    return full_path.substr(0, slash);
}

std::string asset_path(const std::string &relative_path) {
    return get_executable_directory() + "/" + relative_path;
}

// constants
// --------------------------------------------------------------------

// Window
const float WINDOW_WIDTH = 800.f;
const float WINDOW_HEIGHT = 800.f;
const int FRAME_RATE_LIMIT = 240;

// Player
const float PLAYER_WIDTH = 30.f;
const float PLAYER_HEIGHT = 15.f;
const float PLAYER_CENTER_OFFSET_Y = 7.5f;
const float PLAYER_OUTLINE_THICKNESS = 2.f;

const float BASE_SPEED = 1.f;
const int BASE_TURN_SPEED = 1;
const int AGILITY_TURN_SPEED = 2;

// Energy
const float MAX_ENERGY = 100.f;
const float ENERGY_REGEN = 0.08f;

// Hull
const int MAX_HEALTH = 3;
const int ONE_SHOT_TOUGHNESS = 1200;
const int DAMAGE_COOLDOWN = 60;

// Shooting
const int BASE_INTERVAL = 50;
const int RAPID_FIRE_INTERVAL = BASE_INTERVAL - 15;
const int HEAVY_COOLDOWN_INTERVAL = BASE_INTERVAL - 20;

const int BASIC_PROJECTILE_POWER = 200;
const float BASIC_PROJECTILE_SPEED = 1.f;
const float BASIC_PROJECTILE_ENERGY = 10.f;
const float BASIC_PROJECTILE_X_SIZE = 10.f;
const float BASIC_PROJECTILE_Y_SIZE = 4.f;

const int HEAVY_PROJECTILE_POWER = 500;
const float HEAVY_PROJECTILE_SPEED = 0.7f;
const float HEAVY_PROJECTILE_ENERGY = 25.f;
const float HEAVY_PROJECTILE_X_SIZE = 14.f;
const float HEAVY_PROJECTILE_Y_SIZE = 7.f;

// Powerups
const int AGILITY_DURATION = 7200;      // 30 seconds at 240 FPS
const int SHIELD_DURATION = 14400;      // 60 seconds at 240 FPS
const int RAPID_FIRE_DURATION = 6000;   // 25 seconds at 240 FPS
const float POWERUP_SPEED = 0.2f;
const float POWERUP_RADIUS = 14.f;

// Asteroids
const int MIN_ASTEROID_TOUGHNESS = 300;
const int MAX_ASTEROID_TOUGHNESS = 2500;

const float MIN_POWERUP_DROP_CHANCE = 10.f;
const float MAX_POWERUP_DROP_CHANCE = 40.f;

// Difficulty progression
const int DIFFICULTY_MAX_SCORE = 10000;
const int MIN_SPAWN_RATE = 100;

// Trail
const sf::Color TRAIL_READY_COLOR = sf::Color::Cyan;
const sf::Color TRAIL_BASIC_COLOR = sf::Color::Green;
const sf::Color TRAIL_HEAVY_COLOR = sf::Color(255, 0, 255, 255);

const float TRAIL_SEGMENT_RADIUS = 7.5f;
const float TRAIL_OUTLINE_THICKNESS = 2.f;
const int TRAIL_INTERVAL = 5;
const int TRAIL_MAX_LENGTH = 10;
const int TRAIL_FADE_ALPHA = 4;

// Player colors
const sf::Color PLAYER_BASE_COLOR = sf::Color(230, 0, 0, 255);
const sf::Color PLAYER_AGILITY_COLOR = sf::Color::Yellow;
const sf::Color PLAYER_RAPID_FIRE_COLOR = sf::Color(255, 150, 0, 255);

// Explosion
const float EXPLOSION_DURATION = 0.5f;
const float EXPLOSION_MAX_RADIUS = 75.f;
const float EXPLOSION_OUTLINE_THICKNESS = 6.f;

// Shield
const float SHIELD_RADIUS = 32.f;
const float SHIELD_OUTLINE_THICKNESS = 3.f;

// Danger border
const float DANGER_BORDER_THICKNESS = 3.f;
const float DANGER_GLOW_SIZE = 22.f;
const float DANGER_PULSE_LENGTH = 100.f;
const float DANGER_PULSE_SPEED = 4.f;

// Other gameplay
const int SPAWN_X = 900;
const int INITIAL_SPAWN_RATE = 500;
const int INITIAL_SWAP_SPEED = 40;
const int SCORE_INTERVAL = 15;

// Audio
const float DEFAULT_SFX_VOLUME = 70.f;
const float DEFAULT_MUSIC_VOLUME = 70.f;

// UI
const float BAR_WIDTH = 140.f;
const float BAR_HEIGHT = 20.f;
// --------------------------------------------------------------------

// global state / setup
// --------------------------------------------------------------------
enum GameState {
    MAIN_MENU,
    OPTIONS,
    PLAYING,
    PAUSED_MENU,
    GAME_OVER,
};

GameState current_state = MAIN_MENU;
bool wait_for_enter_release = false;

sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Polygon Perihelion", sf::Style::Close);
sf::ConvexShape player(3UL);
sf::ConvexShape player_hitbox(3UL);
std::vector<sf::CircleShape> trail;
sf::Vector2f center(window.getSize().x / 2, window.getSize().y / 2);
sf::Font font;

float sfx_volume = DEFAULT_SFX_VOLUME;
float music_volume = DEFAULT_MUSIC_VOLUME;
bool rocket_idle_hum = true;
bool show_keybinds = false;
bool waiting_for_keybind = false;
int keybind_selection = 0;

struct Keybinds {
    sf::Keyboard::Key fire = sf::Keyboard::Up;
    sf::Keyboard::Key heavy_modifier = sf::Keyboard::LShift;
    sf::Keyboard::Key turn_left = sf::Keyboard::Left;
    sf::Keyboard::Key turn_right = sf::Keyboard::Right;
    sf::Keyboard::Key toggle_hitboxes = sf::Keyboard::Grave;
    sf::Keyboard::Key pause = sf::Keyboard::Escape;
};

Keybinds keybinds;

class AudioSystem {
public:
    enum SoundId {
        AGILITY_ACTIVATE,
        AGILITY_DEACTIVATE,
        BASIC_SHOT,
        GAME_OVER,
        HEAVY_SHOT,
        HIGH_TOUGHNESS_ASTEROID_BREAK,
        HULL_DAMAGED,
        HULL_REGEN,
        LOW_TOUGHNESS_ASTEROID_BREAK,
        MID_TOUGHNESS_ASTEROID_BREAK,
        PLAYER_DEATH,
        POWERUP_SPAWN,
        RAPID_FIRE_ACTIVATE,
        RAPID_FIRE_DEACTIVATE,
        ROCKET_TRAIL_HUM,
        SETTING_ADJUSTMENT,
        SHIELD_DOWN,
        SHIELD_UP,
        SOUND_COUNT
    };

private:
    std::array<sf::SoundBuffer, SOUND_COUNT> buffers;
    std::array<sf::Sound, 16> sound_pool;
    sf::Sound trail_hum;
    sf::Music ambient_music;
    bool loaded = false;

    const char *paths[SOUND_COUNT] = {
        "sounds/agility_activate.mp3",
        "sounds/agility_deactivate.mp3",
        "sounds/basic_shot.mp3",
        "sounds/game_over.mp3",
        "sounds/heavy_shot.mp3",
        "sounds/high_toughness_asteroid_break.mp3",
        "sounds/hull_damaged.mp3",
        "sounds/hull_regen.mp3",
        "sounds/low_toughness_asteroid_break.mp3",
        "sounds/mid_toughness_asteroid_break.mp3",
        "sounds/player_death.mp3",
        "sounds/powerup_spawn.mp3",
        "sounds/rapidfire_activate.mp3",
        "sounds/rapidfire_deactivate.mp3",
        "sounds/rocket_trail_hum.mp3",
        "sounds/setting_adjustment.mp3",
        "sounds/shield_down.mp3",
        "sounds/shield_up.mp3"
    };

public:
    bool setup() {
        loaded = true;

        for(int i = 0; i < SOUND_COUNT; i++) {
            if(!buffers[i].loadFromFile(asset_path(paths[i]))) {
                std::cerr << "Error loading sound: " << paths[i] << std::endl;
                loaded = false;
            }
        }

        for(sf::Sound &sound : sound_pool) {
            sound.setVolume(sfx_volume);
        }

        trail_hum.setVolume(sfx_volume);
        trail_hum.setLoop(true);

        if(!ambient_music.openFromFile(asset_path("sounds/ambient_track.mp3"))) {
            std::cerr << "Error loading music: sounds/ambient_track.mp3" << std::endl;
            loaded = false;
        }
        else {
            ambient_music.setVolume(music_volume);
            ambient_music.setLoop(true);
        }

        return loaded;
    }

    void set_volume(float volume) {
        for(sf::Sound &sound : sound_pool) {
            sound.setVolume(volume);
        }

        trail_hum.setVolume(volume);
    }

    void set_music_volume(float volume) {
        ambient_music.setVolume(volume);
    }

    void start_ambient_music() {
        if(ambient_music.getStatus() != sf::Music::Playing) {
            ambient_music.play();
        }
    }

    void stop_ambient_music() {
        ambient_music.stop();
    }

    void play(SoundId id) {
        if(!loaded) {
            return;
        }

        for(sf::Sound &sound : sound_pool) {
            if(sound.getStatus() == sf::Sound::Stopped) {
                sound.setBuffer(buffers[id]);
                sound.setVolume(sfx_volume);
                sound.play();
                return;
            }
        }

        sound_pool[0].stop();
        sound_pool[0].setBuffer(buffers[id]);
        sound_pool[0].setVolume(sfx_volume);
        sound_pool[0].play();
    }

    void start_trail_hum() {
        if(!loaded || trail_hum.getStatus() == sf::Sound::Playing) {
            return;
        }

        trail_hum.setBuffer(buffers[ROCKET_TRAIL_HUM]);
        trail_hum.setVolume(sfx_volume);
        trail_hum.play();
    }

    void stop_trail_hum() {
        trail_hum.stop();
    }
};

AudioSystem audio;

int pause_selection = 0;

int orientation = 0;

int score = 0;
int high_score = 0;

int cooldown = 0;
int interval = BASE_INTERVAL;
int shot_time = interval;
int spawn_rate = INITIAL_SPAWN_RATE;
int spawn_chance = spawn_rate;
int swap_speed = INITIAL_SWAP_SPEED;
int swap_count = swap_speed;
float speed = BASE_SPEED;
int turn_speed = BASE_TURN_SPEED;
bool show_hitboxes = false;
int interval_count = 0;

// Trail cooldown indicator
sf::Color trail_target_color = TRAIL_READY_COLOR;
int last_shot_type = 0;
int heavy_cooldown_start = BASE_INTERVAL;

// Powerup timers
int agility_timer = 0;
int rapid_fire_timer = 0;
int shield_timer = 0;
bool shield_active = false;

// Energy
float energy = MAX_ENERGY;

// Hull
int health = MAX_HEALTH;
int damage_cooldown = 0;

// Explosion
sf::CircleShape explosion;
sf::Clock explosion_clock;
bool explosion_active = false;
sf::Color explosion_color = sf::Color::White;
float explosion_start_radius = 5.f;

// Shield visual
sf::CircleShape shield_visual;

// Danger border
sf::Clock danger_border_clock;
// --------------------------------------------------------------------

// function declarations
// --------------------------------------------------------------------
int randint(int min, int max);

void rotate(sf::ConvexShape &cs, int p_count, double degrees, int &o = orientation);

int normalize_angle(int angle);

float get_difficulty_progress();

void update_difficulty();

void draw_danger_border();

bool projections_overlap(const sf::ConvexShape &a, const sf::ConvexShape &b);
bool projections_overlap(const sf::CircleShape &a, const sf::ConvexShape &b);

bool check_collision(const sf::ConvexShape &a, const sf::ConvexShape &b);
bool check_collision(const sf::CircleShape &a, const sf::ConvexShape &b);
bool check_collision(const sf::ConvexShape &a, const sf::CircleShape &b);
// --------------------------------------------------------------------

// classes
// --------------------------------------------------------------------
class Projectile : public sf::ConvexShape {
public:
    sf::ConvexShape creator;
    int direction;
    int p_type;
    int power;
    float travel_speed;
    float energy_cost;
    sf::ConvexShape hitbox;

    Projectile(sf::ConvexShape creator, int p_type) {
        this->creator = creator;
        this->direction = orientation;
        this->p_type = p_type;

        this->setPointCount(4UL);
        this->hitbox.setPointCount(4UL);

        float x_size;
        float y_size;
        sf::Color color;

        switch(p_type) {
            case 1:
                x_size = BASIC_PROJECTILE_X_SIZE;
                y_size = BASIC_PROJECTILE_Y_SIZE;
                color = sf::Color::Green;
                this->power = BASIC_PROJECTILE_POWER;
                this->travel_speed = BASIC_PROJECTILE_SPEED;
                this->energy_cost = BASIC_PROJECTILE_ENERGY;
                break;

            case 2:
                x_size = HEAVY_PROJECTILE_X_SIZE;
                y_size = HEAVY_PROJECTILE_Y_SIZE;
                color = TRAIL_HEAVY_COLOR;
                this->power = HEAVY_PROJECTILE_POWER;
                this->travel_speed = HEAVY_PROJECTILE_SPEED;
                this->energy_cost = HEAVY_PROJECTILE_ENERGY;
                break;

            default:
                x_size = BASIC_PROJECTILE_X_SIZE;
                y_size = BASIC_PROJECTILE_Y_SIZE;
                color = sf::Color::Green;
                this->power = BASIC_PROJECTILE_POWER;
                this->travel_speed = BASIC_PROJECTILE_SPEED;
                this->energy_cost = BASIC_PROJECTILE_ENERGY;
                break;
        }

        this->setPoint(0, sf::Vector2f(0.f, 0.f));
        this->setPoint(1, sf::Vector2f(x_size, y_size));
        this->setPoint(2, sf::Vector2f(x_size * 2, 0.f));
        this->setPoint(3, sf::Vector2f(x_size, -y_size));
        this->setFillColor(color);

        this->hitbox.setPoint(0, sf::Vector2f(0.f, -y_size));
        this->hitbox.setPoint(1, sf::Vector2f(x_size * 2, -y_size));
        this->hitbox.setPoint(2, sf::Vector2f(x_size * 2, y_size));
        this->hitbox.setPoint(3, sf::Vector2f(0.f, y_size));
        this->hitbox.setFillColor(sf::Color::Transparent);
        this->hitbox.setOutlineThickness(2.f);
        this->hitbox.setOutlineColor(sf::Color::Red);

        this->setOrigin(x_size, 0.f);
        this->hitbox.setOrigin(x_size, 0.f);
        this->setRotation(direction);
        this->hitbox.setRotation(direction);

        sf::Vector2f tip = player.getPoint(1) + player.getPosition();
        this->setPosition(tip);
        this->hitbox.setPosition(tip);
    }

    void travel(float speed) {
        float radians = direction * (M_PI / 180);
        this->move((speed * 2) * this->travel_speed * cos(radians), (speed * 2) * this->travel_speed * sin(radians));
        this->hitbox.move((speed * 2) * this->travel_speed * cos(radians), (speed * 2) * this->travel_speed * sin(radians));
    }
};

class Asteroid : public sf::ConvexShape {
public:
    int durability;
    float spawny;
    float radius;
    float speed;
    float density_multiplier;
    int toughness;
    sf::CircleShape hitbox;

    Asteroid(float x, float y) {
        setPointCount(randint(9, 12));

        int spawn = randint(0, 200);
        this->spawny = y;

        if(spawn < 80) {
            this->radius = randint(30, 40);
        }
        else if(spawn < 160) {
            this->radius = randint(60, 75);
        }
        else {
            this->radius = randint(90, 100);
        }

        // --------------------------------------------------------
        // Asteroid density progression
        // --------------------------------------------------------
        float difficulty_progress = get_difficulty_progress();
        float density_roll = randint(0, 10000) / 100.f;

        // Starting distribution:
        // 75% -> 1x
        // 20% -> 1.5x
        // 5%  -> 2x
        // 0%  -> 2.5x
        //
        // Maximum distribution:
        // 25% -> 1x
        // 40% -> 1.5x
        // 25% -> 2x
        // 10% -> 2.5x
        //
        // The probabilities smoothly transition between the two.
        float chance_1 = 75.f + (25.f - 75.f) * difficulty_progress;
        float chance_15 = 95.f + (65.f - 95.f) * difficulty_progress;
        float chance_2 = 100.f - (10.f * difficulty_progress);

        if(density_roll < chance_1) {
            this->density_multiplier = 1.f;
        }
        else if(density_roll < chance_15) {
            this->density_multiplier = 1.5f;
        }
        else if(density_roll < chance_2) {
            this->density_multiplier = 2.f;
        }
        else {
            this->density_multiplier = 2.5f;
        }

        float hitbox_radius = radius * 0.88;
        this->hitbox.setRadius(hitbox_radius);
        this->hitbox.setPosition(this->getPosition().x + SPAWN_X - hitbox_radius, this->getPosition().y + this->spawny - hitbox_radius);
        this->hitbox.setFillColor(sf::Color::Transparent);
        this->hitbox.setOutlineThickness(2.f);
        this->hitbox.setOutlineColor(sf::Color::Red);

        toughness = static_cast<int>(this->radius * 10 * this->density_multiplier);
        durability = toughness;
        speed = 10.f / radius;

        for(int i = 0; i < getPointCount(); ++i) {
            float angle = (i * 2 * M_PI) / getPointCount();
            float offsetX = cos(angle) * radius;
            float offsetY = sin(angle) * radius;
            setPoint(i, sf::Vector2f(x + offsetX, y + offsetY));
        }

        int grey_shade;

        if(this->density_multiplier == 1.f) {
            grey_shade = randint(150, 211);
        }
        else if(this->density_multiplier == 1.5f) {
            grey_shade = randint(110, 149);
        }
        else if(this->density_multiplier == 2.f) {
            grey_shade = randint(80, 109);
        }
        else {
            grey_shade = randint(50, 79);
        }

        setFillColor(sf::Color(grey_shade, grey_shade, grey_shade, 255));
    }

    void move(float x, float y) {
        sf::ConvexShape::move(x, y);
        hitbox.move(x, y);
    }

    float get_drop_chance() const {
        float toughness_progress = static_cast<float>(toughness - MIN_ASTEROID_TOUGHNESS) / static_cast<float>(MAX_ASTEROID_TOUGHNESS - MIN_ASTEROID_TOUGHNESS);

        if(toughness_progress < 0.f) {
            toughness_progress = 0.f;
        }

        if(toughness_progress > 1.f) {
            toughness_progress = 1.f;
        }

        return MIN_POWERUP_DROP_CHANCE + toughness_progress * (MAX_POWERUP_DROP_CHANCE - MIN_POWERUP_DROP_CHANCE);
    }
};

class Powerup : public sf::CircleShape {
public:
    int p_type;
    sf::CircleShape hitbox;

    Powerup(float x, float y, int p_type) {
        this->p_type = p_type;

        this->setRadius(POWERUP_RADIUS);
        this->setOrigin(POWERUP_RADIUS, POWERUP_RADIUS);
        this->setPosition(x, y);

        this->hitbox.setRadius(POWERUP_RADIUS);
        this->hitbox.setOrigin(POWERUP_RADIUS, POWERUP_RADIUS);
        this->hitbox.setPosition(x, y);
        this->hitbox.setFillColor(sf::Color::Transparent);
        this->hitbox.setOutlineThickness(2.f);
        this->hitbox.setOutlineColor(sf::Color::Red);

        switch(p_type) {
            case 1:
                this->setFillColor(sf::Color::Red);
                this->setOutlineThickness(2.f);
                this->setOutlineColor(sf::Color::White);
                break;

            case 2:
                this->setFillColor(sf::Color::Cyan);
                this->setOutlineThickness(2.f);
                this->setOutlineColor(sf::Color::White);
                break;

            case 3:
                this->setFillColor(sf::Color::Yellow);
                this->setOutlineThickness(2.f);
                this->setOutlineColor(sf::Color::White);
                break;

            case 4:
                this->setFillColor(PLAYER_RAPID_FIRE_COLOR);
                this->setOutlineThickness(2.f);
                this->setOutlineColor(sf::Color::White);
                break;

            default:
                this->setFillColor(sf::Color::White);
                this->setOutlineThickness(2.f);
                this->setOutlineColor(sf::Color::White);
                break;
        }
    }

    void move(float x, float y) {
        sf::CircleShape::move(x, y);
        hitbox.move(x, y);
    }

    void activate(float &energy, int &health, int &turn_speed, int &interval, int &agility_timer, int &rapid_fire_timer, int &shield_timer, bool &shield_active) {
        switch(p_type) {
            case 1:
                health++;

                if(health > MAX_HEALTH) {
                    health = MAX_HEALTH;
                }
                break;

            case 2:
                shield_active = true;
                shield_timer = SHIELD_DURATION;
                break;

            case 3:
                turn_speed = AGILITY_TURN_SPEED;
                agility_timer = AGILITY_DURATION;
                break;

            case 4:
                interval = RAPID_FIRE_INTERVAL;
                rapid_fire_timer = RAPID_FIRE_DURATION;
                break;
        }
    }
};

// Object storage
// --------------------------------------------------------------------
std::vector<Projectile> projs;
std::vector<Asteroid> belt;
std::vector<Powerup> powerups;
// --------------------------------------------------------------------

// utility functions
// --------------------------------------------------------------------
int randint(int min, int max) {
    static std::random_device dev;
    static std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);
    return dist(rng);
}

void rotate(sf::ConvexShape &cs, int p_count, double degrees, int &o) {
    std::vector<float> x_coords;
    std::vector<float> y_coords;
    double h = 0;
    double k = 0;

    for(int i = 0; i < p_count; i++) {
        x_coords.push_back(cs.getPoint(i).x);
        h += cs.getPoint(i).x;
        y_coords.push_back(cs.getPoint(i).y);
        k += cs.getPoint(i).y;
    }

    h /= p_count;
    k /= p_count;

    double radians = degrees * (M_PI / 180);

    for(int i = 0; i < p_count; i++) {
        float x = x_coords[i];
        float y = y_coords[i];

        cs.setPoint(i, sf::Vector2f(h + cos(radians) * (x - h) - sin(radians) * (y - k), k + sin(radians) * (x - h) + cos(radians) * (y - k)));
    }

    if(&cs == &player) {
        o += degrees;
    }
}

int normalize_angle(int angle) {
    angle %= 360;

    if(angle < 0) {
        angle += 360;
    }

    return angle;
}

float get_difficulty_progress() {
    float progress = static_cast<float>(score) / static_cast<float>(DIFFICULTY_MAX_SCORE);

    if(progress < 0.f) {
        progress = 0.f;
    }

    if(progress > 1.f) {
        progress = 1.f;
    }

    return progress;
}

void update_difficulty() {
    float progress = get_difficulty_progress();

    float difficulty_curve = progress * progress * (3.f - 2.f * progress);

    float current_spawn_rate = INITIAL_SPAWN_RATE + (MIN_SPAWN_RATE - INITIAL_SPAWN_RATE) * difficulty_curve;

    spawn_rate = static_cast<int>(current_spawn_rate);

    if(spawn_rate < MIN_SPAWN_RATE) {
        spawn_rate = MIN_SPAWN_RATE;
    }
}

void draw_danger_border() {
    float progress = get_difficulty_progress();
    float time = danger_border_clock.getElapsedTime().asSeconds();

    // Slow pulse makes the entire perimeter breathe like an energy field.
    float pulse = (sin(time * 2.5f) + 1.f) * 0.5f;

    // A slightly stronger field as difficulty rises.
    float base_alpha = 45.f + progress * 35.f;
    float bright_alpha = 125.f + progress * 70.f;

    // ------------------------------------------------------------
    // Outer atmospheric glow
    // ------------------------------------------------------------
    sf::RectangleShape top_glow(sf::Vector2f(WINDOW_WIDTH, DANGER_GLOW_SIZE));
    sf::RectangleShape bottom_glow(sf::Vector2f(WINDOW_WIDTH, DANGER_GLOW_SIZE));
    sf::RectangleShape left_glow(sf::Vector2f(DANGER_GLOW_SIZE, WINDOW_HEIGHT));
    sf::RectangleShape right_glow(sf::Vector2f(DANGER_GLOW_SIZE, WINDOW_HEIGHT));

    top_glow.setPosition(0.f, 0.f);
    bottom_glow.setPosition(0.f, WINDOW_HEIGHT - DANGER_GLOW_SIZE);
    left_glow.setPosition(0.f, 0.f);
    right_glow.setPosition(WINDOW_WIDTH - DANGER_GLOW_SIZE, 0.f);

    sf::Uint8 glow_alpha = static_cast<sf::Uint8>(base_alpha + pulse * 25.f);

    top_glow.setFillColor(sf::Color(255, 0, 0, glow_alpha));
    bottom_glow.setFillColor(sf::Color(255, 0, 0, glow_alpha));
    left_glow.setFillColor(sf::Color(255, 0, 0, glow_alpha));
    right_glow.setFillColor(sf::Color(255, 0, 0, glow_alpha));

    window.draw(top_glow);
    window.draw(bottom_glow);
    window.draw(left_glow);
    window.draw(right_glow);

    // ------------------------------------------------------------
    // Main plasma perimeter
    // ------------------------------------------------------------
    sf::RectangleShape top_edge(sf::Vector2f(WINDOW_WIDTH, DANGER_BORDER_THICKNESS));
    sf::RectangleShape bottom_edge(sf::Vector2f(WINDOW_WIDTH, DANGER_BORDER_THICKNESS));
    sf::RectangleShape left_edge(sf::Vector2f(DANGER_BORDER_THICKNESS, WINDOW_HEIGHT));
    sf::RectangleShape right_edge(sf::Vector2f(DANGER_BORDER_THICKNESS, WINDOW_HEIGHT));

    top_edge.setPosition(0.f, 0.f);
    bottom_edge.setPosition(0.f, WINDOW_HEIGHT - DANGER_BORDER_THICKNESS);
    left_edge.setPosition(0.f, 0.f);
    right_edge.setPosition(WINDOW_WIDTH - DANGER_BORDER_THICKNESS, 0.f);

    sf::Uint8 edge_alpha = static_cast<sf::Uint8>(bright_alpha + pulse * 60.f);

    top_edge.setFillColor(sf::Color(255, 20, 20, edge_alpha));
    bottom_edge.setFillColor(sf::Color(255, 20, 20, edge_alpha));
    left_edge.setFillColor(sf::Color(255, 20, 20, edge_alpha));
    right_edge.setFillColor(sf::Color(255, 20, 20, edge_alpha));

    window.draw(top_edge);
    window.draw(bottom_edge);
    window.draw(left_edge);
    window.draw(right_edge);

    // ------------------------------------------------------------
    // Moving energy pulses
    // ------------------------------------------------------------
    float pulse_position = fmod(time * DANGER_PULSE_SPEED * 60.f, DANGER_PULSE_LENGTH * 8.f);

    sf::RectangleShape horizontal_pulse(sf::Vector2f(DANGER_PULSE_LENGTH, 3.f));
    sf::RectangleShape vertical_pulse(sf::Vector2f(3.f, DANGER_PULSE_LENGTH));

    sf::Uint8 pulse_alpha = static_cast<sf::Uint8>(180.f + pulse * 75.f);

    horizontal_pulse.setFillColor(sf::Color(255, 90, 90, pulse_alpha));
    vertical_pulse.setFillColor(sf::Color(255, 90, 90, pulse_alpha));

    // Top and bottom pulses move left -> right.
    horizontal_pulse.setPosition(pulse_position - DANGER_PULSE_LENGTH, 0.f);
    window.draw(horizontal_pulse);

    horizontal_pulse.setPosition(WINDOW_WIDTH - pulse_position, WINDOW_HEIGHT - 3.f);
    window.draw(horizontal_pulse);

    // Left and right pulses move top -> bottom.
    vertical_pulse.setPosition(0.f, pulse_position - DANGER_PULSE_LENGTH);
    window.draw(vertical_pulse);

    vertical_pulse.setPosition(WINDOW_WIDTH - 3.f, WINDOW_HEIGHT - pulse_position);
    window.draw(vertical_pulse);

    // ------------------------------------------------------------
    // Bright corner emitters
    // ------------------------------------------------------------
    sf::RectangleShape corner_horizontal(sf::Vector2f(28.f, 3.f));
    sf::RectangleShape corner_vertical(sf::Vector2f(3.f, 28.f));

    sf::Uint8 corner_alpha = static_cast<sf::Uint8>(210.f + pulse * 45.f);

    corner_horizontal.setFillColor(sf::Color(255, 120, 120, corner_alpha));
    corner_vertical.setFillColor(sf::Color(255, 120, 120, corner_alpha));

    // Top-left
    corner_horizontal.setPosition(0.f, 0.f);
    corner_vertical.setPosition(0.f, 0.f);
    window.draw(corner_horizontal);
    window.draw(corner_vertical);

    // Top-right
    corner_horizontal.setPosition(WINDOW_WIDTH - 28.f, 0.f);
    corner_vertical.setPosition(WINDOW_WIDTH - 3.f, 0.f);
    window.draw(corner_horizontal);
    window.draw(corner_vertical);

    // Bottom-left
    corner_horizontal.setPosition(0.f, WINDOW_HEIGHT - 3.f);
    corner_vertical.setPosition(0.f, WINDOW_HEIGHT - 28.f);
    window.draw(corner_horizontal);
    window.draw(corner_vertical);

    // Bottom-right
    corner_horizontal.setPosition(WINDOW_WIDTH - 28.f, WINDOW_HEIGHT - 3.f);
    corner_vertical.setPosition(WINDOW_WIDTH - 3.f, WINDOW_HEIGHT - 28.f);
    window.draw(corner_horizontal);
    window.draw(corner_vertical);
}

bool projections_overlap(const sf::ConvexShape &a, const sf::ConvexShape &b) {
    for(int i = 0; i < a.getPointCount(); i++) {
        sf::Vector2f point1 = a.getTransform().transformPoint(a.getPoint(i));
        sf::Vector2f point2 = a.getTransform().transformPoint(a.getPoint((i + 1) % a.getPointCount()));
        sf::Vector2f edge = point2 - point1;
        sf::Vector2f axis(-edge.y, edge.x);

        float a_min_projection = INFINITY;
        float a_max_projection = -INFINITY;

        for(int j = 0; j < a.getPointCount(); j++) {
            sf::Vector2f point = a.getTransform().transformPoint(a.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            a_min_projection = std::min(a_min_projection, projection);
            a_max_projection = std::max(a_max_projection, projection);
        }

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int j = 0; j < b.getPointCount(); j++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    return true;
}

bool projections_overlap(const sf::CircleShape &a, const sf::ConvexShape &b) {
    sf::Vector2f circle_center = a.getPosition() + sf::Vector2f(a.getRadius(), a.getRadius());

    for(int i = 0; i < b.getPointCount(); i++) {
        sf::Vector2f point1 = b.getTransform().transformPoint(b.getPoint(i));
        sf::Vector2f point2 = b.getTransform().transformPoint(b.getPoint((i + 1) % b.getPointCount()));
        sf::Vector2f edge = point2 - point1;
        sf::Vector2f axis(-edge.y, edge.x);

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int j = 0; j < b.getPointCount(); j++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(j));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        float center_projection = circle_center.x * axis.x + circle_center.y * axis.y;
        float radius_projection = a.getRadius() * sqrt(axis.x * axis.x + axis.y * axis.y);
        float a_min_projection = center_projection - radius_projection;
        float a_max_projection = center_projection + radius_projection;

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    float closest_distance = INFINITY;
    sf::Vector2f closest_vertex;

    for(int i = 0; i < b.getPointCount(); i++) {
        sf::Vector2f vertex = b.getTransform().transformPoint(b.getPoint(i));
        float distance = sqrt(pow(vertex.x - circle_center.x, 2) + pow(vertex.y - circle_center.y, 2));

        if(distance < closest_distance) {
            closest_distance = distance;
            closest_vertex = vertex;
        }
    }

    sf::Vector2f axis = closest_vertex - circle_center;
    float axis_length = sqrt(axis.x * axis.x + axis.y * axis.y);

    if(axis_length > 0.f) {
        axis /= axis_length;

        float circle_projection = circle_center.x * axis.x + circle_center.y * axis.y;
        float a_min_projection = circle_projection - a.getRadius();
        float a_max_projection = circle_projection + a.getRadius();

        float b_min_projection = INFINITY;
        float b_max_projection = -INFINITY;

        for(int i = 0; i < b.getPointCount(); i++) {
            sf::Vector2f point = b.getTransform().transformPoint(b.getPoint(i));
            float projection = point.x * axis.x + point.y * axis.y;
            b_min_projection = std::min(b_min_projection, projection);
            b_max_projection = std::max(b_max_projection, projection);
        }

        if(a_max_projection < b_min_projection || b_max_projection < a_min_projection) {
            return false;
        }
    }

    return true;
}

bool check_collision(const sf::ConvexShape &a, const sf::ConvexShape &b) {
    return projections_overlap(a, b) && projections_overlap(b, a);
}

bool check_collision(const sf::CircleShape &a, const sf::ConvexShape &b) {
    return projections_overlap(a, b);
}

bool check_collision(const sf::ConvexShape &a, const sf::CircleShape &b) {
    return projections_overlap(b, a);
}
// --------------------------------------------------------------------

// menu / game functions
// --------------------------------------------------------------------
void update_sfx_volume() {
    audio.set_volume(sfx_volume);
}

void reset_player();

void reset_run() {
    current_state = MAIN_MENU;
    audio.stop_trail_hum();
    reset_player();
    projs.clear();
    belt.clear();
    powerups.clear();
    trail.clear();
    explosion_active = false;

    score = 0;
    interval_count = 0;
    energy = MAX_ENERGY;
    health = MAX_HEALTH;
    damage_cooldown = 0;

    cooldown = 0;
    interval = BASE_INTERVAL;
    shot_time = interval;
    spawn_rate = INITIAL_SPAWN_RATE;
    spawn_chance = spawn_rate;
    swap_speed = INITIAL_SWAP_SPEED;
    swap_count = swap_speed;
    speed = BASE_SPEED;
    turn_speed = BASE_TURN_SPEED;

    agility_timer = 0;
    rapid_fire_timer = 0;
    shield_timer = 0;
    shield_active = false;

    trail_target_color = TRAIL_READY_COLOR;
    last_shot_type = 0;
    heavy_cooldown_start = interval;
    pause_selection = 0;
    show_keybinds = false;
    waiting_for_keybind = false;
    keybind_selection = 0;

    player.setFillColor(PLAYER_BASE_COLOR);
}

std::string key_name(sf::Keyboard::Key key) {
    if(key >= sf::Keyboard::A && key <= sf::Keyboard::Z) {
        return std::string(1, static_cast<char>('A' + (key - sf::Keyboard::A)));
    }

    if(key >= sf::Keyboard::Num0 && key <= sf::Keyboard::Num9) {
        return std::string(1, static_cast<char>('0' + (key - sf::Keyboard::Num0)));
    }

    switch(key) {
        case sf::Keyboard::Space: return "SPACE";
        case sf::Keyboard::Enter: return "ENTER";
        case sf::Keyboard::Escape: return "ESC";
        case sf::Keyboard::Tab: return "TAB";
        case sf::Keyboard::Backspace: return "BACKSPACE";
        case sf::Keyboard::LShift: return "L-SHIFT";
        case sf::Keyboard::RShift: return "R-SHIFT";
        case sf::Keyboard::LControl: return "L-CTRL";
        case sf::Keyboard::RControl: return "R-CTRL";
        case sf::Keyboard::LAlt: return "L-ALT";
        case sf::Keyboard::RAlt: return "R-ALT";
        case sf::Keyboard::Up: return "UP";
        case sf::Keyboard::Down: return "DOWN";
        case sf::Keyboard::Left: return "LEFT";
        case sf::Keyboard::Right: return "RIGHT";
        case sf::Keyboard::Grave: return "GRAVE";
        case sf::Keyboard::Comma: return "COMMA";
        case sf::Keyboard::Period: return "PERIOD";
        case sf::Keyboard::Slash: return "SLASH";
        case sf::Keyboard::SemiColon: return "SEMICOLON";
        case sf::Keyboard::Quote: return "QUOTE";
        case sf::Keyboard::LBracket: return "L-BRACKET";
        case sf::Keyboard::RBracket: return "R-BRACKET";
        case sf::Keyboard::Backslash: return "BACKSLASH";
        case sf::Keyboard::Equal: return "EQUAL";
        case sf::Keyboard::Dash: return "HYPHEN";
        case sf::Keyboard::Add: return "NUMPAD +";
        case sf::Keyboard::Subtract: return "NUMPAD -";
        case sf::Keyboard::Multiply: return "NUMPAD *";
        case sf::Keyboard::Divide: return "NUMPAD /";
        case sf::Keyboard::F1: return "F1";
        case sf::Keyboard::F2: return "F2";
        case sf::Keyboard::F3: return "F3";
        case sf::Keyboard::F4: return "F4";
        case sf::Keyboard::F5: return "F5";
        case sf::Keyboard::F6: return "F6";
        case sf::Keyboard::F7: return "F7";
        case sf::Keyboard::F8: return "F8";
        case sf::Keyboard::F9: return "F9";
        case sf::Keyboard::F10: return "F10";
        case sf::Keyboard::F11: return "F11";
        case sf::Keyboard::F12: return "F12";
        default: return "UNKNOWN";
    }
}

void draw_keybinds_menu() {
    window.clear();

    sf::RectangleShape panel(sf::Vector2f(WINDOW_WIDTH - 100.f, WINDOW_HEIGHT - 100.f));
    panel.setPosition(50.f, 50.f);
    panel.setFillColor(sf::Color(10, 12, 25, 255));
    panel.setOutlineThickness(3.f);
    panel.setOutlineColor(sf::Color::Cyan);
    window.draw(panel);

    sf::Text title("KEYBINDS", font, 44);
    title.setFillColor(sf::Color::Cyan);
    title.setPosition(WINDOW_WIDTH / 2.f - title.getLocalBounds().width / 2.f, 75.f);
    window.draw(title);

    std::vector<std::string> bindings = {
        "FIRE",
        "HEAVY SHOT MODIFIER",
        "TURN LEFT",
        "TURN RIGHT",
        "TOGGLE HITBOXES",
        "PAUSE / RESUME"
    };

    std::vector<sf::Keyboard::Key> keys = {
        keybinds.fire,
        keybinds.heavy_modifier,
        keybinds.turn_left,
        keybinds.turn_right,
        keybinds.toggle_hitboxes,
        keybinds.pause
    };

    for(int i = 0; i < static_cast<int>(bindings.size()); i++) {
        float y = 145.f + i * 65.f;

        sf::Text binding(bindings[i], font, 21);
        binding.setFillColor(i == keybind_selection ? sf::Color::Yellow : sf::Color::White);
        binding.setPosition(135.f, y);
        window.draw(binding);

        sf::RectangleShape key_box(sf::Vector2f(190.f, 38.f));
        key_box.setPosition(475.f, y - 5.f);
        key_box.setFillColor(sf::Color(30, 35, 50, 255));
        key_box.setOutlineThickness(2.f);
        key_box.setOutlineColor(i == keybind_selection ? sf::Color::Yellow : sf::Color::Cyan);
        window.draw(key_box);

        sf::Text key_text(
            waiting_for_keybind && i == keybind_selection ? "PRESS KEY" : key_name(keys[i]),
            font,
            18
        );
        key_text.setFillColor(waiting_for_keybind && i == keybind_selection ? sf::Color::Yellow : sf::Color::Cyan);
        key_text.setPosition(
            570.f - key_text.getLocalBounds().width / 2.f,
            y + 2.f
        );
        window.draw(key_text);
    }

    sf::Text instructions(
        waiting_for_keybind
            ? "PRESS A KEY TO REBIND    ESC: CANCEL"
            : "UP/DOWN: SELECT    ENTER: REBIND    ESC: BACK",
        font,
        14
    );
    instructions.setFillColor(sf::Color(130, 130, 150, 255));
    instructions.setPosition(
        WINDOW_WIDTH / 2.f - instructions.getLocalBounds().width / 2.f,
        635.f
    );
    window.draw(instructions);

    window.display();
}

void draw_pause_menu() {
    if(show_keybinds) {
        draw_keybinds_menu();
        return;
    }

    window.clear();

    sf::RectangleShape panel(sf::Vector2f(WINDOW_WIDTH - 100.f, WINDOW_HEIGHT - 100.f));
    panel.setPosition(50.f, 50.f);
    panel.setFillColor(sf::Color(10, 12, 25, 255));
    panel.setOutlineThickness(3.f);
    panel.setOutlineColor(sf::Color(0, 220, 255, 220));
    window.draw(panel);

    sf::Text title("GAME PAUSED", font, 46);
    title.setFillColor(sf::Color::Cyan);
    title.setPosition(WINDOW_WIDTH / 2.f - title.getLocalBounds().width / 2.f, 70.f);
    window.draw(title);

    std::vector<std::string> options = {
        "Resume",
        "Restart Run",
        "Exit to Main Menu",
        "Rocket Idle Hum: " + std::string(rocket_idle_hum ? "ON" : "OFF"),
        "Keybinds"
    };

    for(int i = 0; i < static_cast<int>(options.size()); i++) {
        float y = 170.f + i * 55.f;

        sf::Text option(options[i], font, 22);
        option.setFillColor(i == pause_selection ? sf::Color::Yellow : sf::Color::White);
        option.setPosition(
            WINDOW_WIDTH / 2.f - option.getLocalBounds().width / 2.f,
            y
        );

        window.draw(option);
    }

    // SFX slider gets its own row below the menu options.
    // SFX slider sits on its own row, leaving room for a future music slider below it.
    sf::Text volume_label("SFX", font, 20);
    volume_label.setFillColor(pause_selection == 5 ? sf::Color::Yellow : sf::Color::White);
    volume_label.setPosition(135.f, 485.f);
    window.draw(volume_label);

    sf::RectangleShape volume_background(sf::Vector2f(300.f, 14.f));
    volume_background.setPosition((WINDOW_WIDTH - 300.f) / 2.f, 490.f);
    volume_background.setFillColor(sf::Color(45, 45, 55, 255));
    volume_background.setOutlineThickness(2.f);
    volume_background.setOutlineColor(pause_selection == 5 ? sf::Color::Yellow : sf::Color::White);
    window.draw(volume_background);

    sf::RectangleShape volume_bar(sf::Vector2f(300.f * (sfx_volume / 100.f), 14.f));
    volume_bar.setPosition((WINDOW_WIDTH - 300.f) / 2.f, 490.f);
    volume_bar.setFillColor(sf::Color::Cyan);
    window.draw(volume_bar);

    sf::Text volume_value(std::to_string(static_cast<int>(sfx_volume)) + "%", font, 18);
    volume_value.setFillColor(sf::Color::Cyan);
    volume_value.setPosition((WINDOW_WIDTH - 300.f) / 2.f + 320.f, 484.f);
    window.draw(volume_value);

    sf::Text music_label("MUSIC", font, 20);
    music_label.setFillColor(pause_selection == 6 ? sf::Color::Yellow : sf::Color::White);
    music_label.setPosition(120.f, 530.f);
    window.draw(music_label);

    sf::RectangleShape music_background(sf::Vector2f(300.f, 14.f));
    music_background.setPosition((WINDOW_WIDTH - 300.f) / 2.f, 535.f);
    music_background.setFillColor(sf::Color(45, 45, 55, 255));
    music_background.setOutlineThickness(2.f);
    music_background.setOutlineColor(pause_selection == 6 ? sf::Color::Yellow : sf::Color::White);
    window.draw(music_background);

    sf::RectangleShape music_bar(sf::Vector2f(300.f * (music_volume / 100.f), 14.f));
    music_bar.setPosition((WINDOW_WIDTH - 300.f) / 2.f, 535.f);
    music_bar.setFillColor(sf::Color::Cyan);
    window.draw(music_bar);

    sf::Text music_value(std::to_string(static_cast<int>(music_volume)) + "%", font, 18);
    music_value.setFillColor(sf::Color::Cyan);
    music_value.setPosition((WINDOW_WIDTH - 300.f) / 2.f + 320.f, 529.f);
    window.draw(music_value);

    sf::Text navigation(
        "UP/DOWN: SELECT    LEFT/RIGHT: ADJUST    ENTER: CONFIRM    ESC: RESUME",
        font,
        13
    );
    navigation.setFillColor(sf::Color(120, 120, 140, 255));
    navigation.setPosition(
        WINDOW_WIDTH / 2.f - navigation.getLocalBounds().width / 2.f,
        675.f
    );
    window.draw(navigation);

    window.display();
}

void menu_mode() {
    window.clear();

    sf::Text title("Polygon Perihelion", font, 50);
    title.setFillColor(sf::Color::White);
    title.setPosition(window.getSize().x / 2 - title.getLocalBounds().width / 2, 100);
    window.draw(title);

    sf::Text instruction("Press Enter to Start", font, 30);
    instruction.setFillColor(sf::Color::Green);
    instruction.setPosition(window.getSize().x / 2 - instruction.getLocalBounds().width / 2, window.getSize().y / 2);
    window.draw(instruction);

    window.display();
}

void display_explosion(sf::Vector2f position, sf::Color color, float radius) {
    explosion.setRadius(radius);
    explosion.setOrigin(radius, radius);
    explosion.setPosition(position);
    explosion.setFillColor(sf::Color::Transparent);
    explosion.setOutlineThickness(EXPLOSION_OUTLINE_THICKNESS);
    explosion.setOutlineColor(color);

    explosion_color = color;
    explosion_start_radius = radius;
    explosion_active = true;
    explosion_clock.restart();
}

void reset_player() {
    player.setPoint(0, center);
    player.setPoint(1, sf::Vector2f(player.getPoint(0).x + PLAYER_WIDTH, player.getPoint(0).y + PLAYER_CENTER_OFFSET_Y));
    player.setPoint(2, sf::Vector2f(player.getPoint(0).x, player.getPoint(0).y + PLAYER_HEIGHT));

    player_hitbox.setPoint(0, player.getPoint(0));
    player_hitbox.setPoint(1, player.getPoint(1));
    player_hitbox.setPoint(2, player.getPoint(2));

    player.setPosition(0.f, 0.f);
    player_hitbox.setPosition(0.f, 0.f);
    orientation = 0;
}

void game_over() {
    if(score > high_score) {
        high_score = score;
    }

    if(explosion_active) {
        float elapsed = explosion_clock.getElapsedTime().asSeconds();
        float progress = elapsed / EXPLOSION_DURATION;

        if(progress >= 1.f) {
            explosion_active = false;
        }
        else {
            float radius = explosion_start_radius + (EXPLOSION_MAX_RADIUS - explosion_start_radius) * progress;
            explosion.setRadius(radius);
            explosion.setOrigin(radius, radius);

            sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));

            explosion.setOutlineColor(sf::Color(explosion_color.r, explosion_color.g, explosion_color.b, alpha));
            window.draw(explosion);
        }
    }

    sf::Text game_over_text("Game Over", font, 50);
    game_over_text.setFillColor(sf::Color::White);
    game_over_text.setPosition(window.getSize().x / 2 - game_over_text.getLocalBounds().width / 2, window.getSize().y / 4);
    window.draw(game_over_text);

    sf::Text restart_text("Press Enter to Restart", font, 30);
    restart_text.setFillColor(sf::Color::White);
    restart_text.setPosition(window.getSize().x / 2 - restart_text.getLocalBounds().width / 2, window.getSize().y / 2);
    window.draw(restart_text);

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
        reset_run();
    }

    window.display();
}

void player_death() {
    current_state = GAME_OVER;
    audio.stop_trail_hum();
    audio.stop_ambient_music();
    audio.play(AudioSystem::PLAYER_DEATH);
    audio.play(AudioSystem::GAME_OVER);

    display_explosion(
        sf::Vector2f(
            player.getPosition().x + center.x,
            player.getPosition().y + center.y
        ),
        sf::Color::Red,
        2.f
    );

    display_explosion(
        sf::Vector2f(
            player.getPosition().x + center.x,
            player.getPosition().y + center.y
        ),
        sf::Color::Cyan,
        5.f
    );
}
// --------------------------------------------------------------------

int main() {
    // setup player & window
    // --------------------------------------------------------------------
    window.setFramerateLimit(FRAME_RATE_LIMIT);

    if(!font.loadFromFile(asset_path("Orbitron-VariableFont_wght.ttf"))) {
        std::cerr << "Error loading font" << std::endl;
        return -1;
    }

    if(!audio.setup()) {
        std::cerr << "Some audio files could not be loaded." << std::endl;
    }

    player.setFillColor(PLAYER_BASE_COLOR);
    player.setOutlineThickness(PLAYER_OUTLINE_THICKNESS);
    player.setOutlineColor(sf::Color(211, 211, 211, 255));

    player.setPoint(0, center);
    player.setPoint(1, sf::Vector2f(player.getPoint(0).x + PLAYER_WIDTH, player.getPoint(0).y + PLAYER_CENTER_OFFSET_Y));
    player.setPoint(2, sf::Vector2f(player.getPoint(0).x, player.getPoint(0).y + PLAYER_HEIGHT));

    player_hitbox.setFillColor(sf::Color::Transparent);
    player_hitbox.setOutlineThickness(2.f);
    player_hitbox.setOutlineColor(sf::Color::Red);
    player_hitbox.setPoint(0, player.getPoint(0));
    player_hitbox.setPoint(1, player.getPoint(1));
    player_hitbox.setPoint(2, player.getPoint(2));

    shield_visual.setRadius(SHIELD_RADIUS);
    shield_visual.setOrigin(SHIELD_RADIUS, SHIELD_RADIUS);
    shield_visual.setFillColor(sf::Color::Transparent);
    shield_visual.setOutlineThickness(SHIELD_OUTLINE_THICKNESS);
    shield_visual.setOutlineColor(sf::Color::Cyan);
    // --------------------------------------------------------------------

    // score
    // --------------------------------------------------------------------
    sf::Text score_text("Score: " + std::to_string(score), font, 20);
    sf::Text high_score_text("High Score: " + std::to_string(high_score), font, 20);
    sf::Text health_text("Hull", font, 20);
    sf::Text energy_text("Energy", font, 20);

    score_text.setPosition(10.f, score_text.getLocalBounds().height / 2);
    high_score_text.setPosition(10.f, score_text.getLocalBounds().height * 2);
    health_text.setPosition(555.f, score_text.getLocalBounds().height / 2 + 3.f);
    energy_text.setPosition(555.f, score_text.getLocalBounds().height / 2 + 30.f);

    sf::RectangleShape health_background(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
    sf::RectangleShape health_bar(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
    sf::RectangleShape energy_background(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
    sf::RectangleShape energy_bar(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));

    health_background.setPosition(650.f, score_text.getLocalBounds().height / 2 + 3.f);
    health_background.setFillColor(sf::Color(50, 50, 50, 255));
    health_background.setOutlineThickness(2.f);
    health_background.setOutlineColor(sf::Color::White);

    health_bar.setPosition(650.f, score_text.getLocalBounds().height / 2 + 3.f);
    health_bar.setFillColor(sf::Color::Red);

    energy_background.setPosition(650.f, score_text.getLocalBounds().height / 2 + 30.f);
    energy_background.setFillColor(sf::Color(50, 50, 50, 255));
    energy_background.setOutlineThickness(2.f);
    energy_background.setOutlineColor(sf::Color::White);

    energy_bar.setPosition(650.f, score_text.getLocalBounds().height / 2 + 30.f);
    energy_bar.setFillColor(sf::Color::Cyan);
    // --------------------------------------------------------------------

    // main
    // --------------------------------------------------------------------
    while(window.isOpen()) {
        sf::Event event;

        while(window.pollEvent(event)) {
            if(event.type == sf::Event::Closed) {
                window.close();
            }

            if(event.type == sf::Event::KeyPressed) {
                if(current_state == PLAYING && event.key.code == keybinds.pause) {
                    current_state = PAUSED_MENU;
                    audio.stop_trail_hum();
                    audio.stop_ambient_music();
                    pause_selection = 0;
                    audio.play(AudioSystem::SETTING_ADJUSTMENT);
                }
                else if(current_state == PAUSED_MENU) {
                    if(show_keybinds) {
                        if(waiting_for_keybind) {
                            if(event.key.code == sf::Keyboard::Escape) {
                                waiting_for_keybind = false;
                            }
                            else {
                                switch(keybind_selection) {
                                    case 0:
                                        keybinds.fire = event.key.code;
                                        break;
                                    case 1:
                                        keybinds.heavy_modifier = event.key.code;
                                        break;
                                    case 2:
                                        keybinds.turn_left = event.key.code;
                                        break;
                                    case 3:
                                        keybinds.turn_right = event.key.code;
                                        break;
                                    case 4:
                                        keybinds.toggle_hitboxes = event.key.code;
                                        break;
                                    case 5:
                                        keybinds.pause = event.key.code;
                                        break;
                                }

                                waiting_for_keybind = false;
                                audio.play(AudioSystem::SETTING_ADJUSTMENT);
                            }
                        }
                        else if(event.key.code == sf::Keyboard::Escape) {
                            show_keybinds = false;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(event.key.code == sf::Keyboard::Up) {
                            keybind_selection = (keybind_selection + 5) % 6;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(event.key.code == sf::Keyboard::Down) {
                            keybind_selection = (keybind_selection + 1) % 6;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(event.key.code == sf::Keyboard::Enter) {
                            waiting_for_keybind = true;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                    }
                    else if(event.key.code == sf::Keyboard::Escape) {
                        current_state = PLAYING;
                        if(rocket_idle_hum) {
                            audio.start_trail_hum();
                        }
                        audio.start_ambient_music();
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Up) {
                        pause_selection = (pause_selection + 6) % 7;
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Down) {
                        pause_selection = (pause_selection + 1) % 7;
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Left && pause_selection == 5) {
                        sfx_volume = std::max(0.f, sfx_volume - 10.f);
                        update_sfx_volume();
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Right && pause_selection == 5) {
                        sfx_volume = std::min(100.f, sfx_volume + 10.f);
                        update_sfx_volume();
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Left && pause_selection == 6) {
                        music_volume = std::max(0.f, music_volume - 10.f);
                        audio.set_music_volume(music_volume);
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Right && pause_selection == 6) {
                        music_volume = std::min(100.f, music_volume + 10.f);
                        audio.set_music_volume(music_volume);
                        audio.play(AudioSystem::SETTING_ADJUSTMENT);
                    }
                    else if(event.key.code == sf::Keyboard::Enter) {
                        if(pause_selection == 0) {
                            current_state = PLAYING;
                            if(rocket_idle_hum) {
                                audio.start_trail_hum();
                            }
                            audio.start_ambient_music();
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(pause_selection == 1) {
                            reset_run();
                            current_state = PLAYING;
                            if(rocket_idle_hum) {
                                audio.start_trail_hum();
                            }
                            audio.start_ambient_music();
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(pause_selection == 2) {
                            reset_run();
                            wait_for_enter_release = true;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(pause_selection == 3) {
                            rocket_idle_hum = !rocket_idle_hum;
                            audio.stop_trail_hum();
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                        else if(pause_selection == 4) {
                            show_keybinds = true;
                            keybind_selection = 0;
                            waiting_for_keybind = false;
                            audio.play(AudioSystem::SETTING_ADJUSTMENT);
                        }
                    }
                }
            }
        }

        // menu handling
        // --------------------------------------------------------------------
        if(current_state == MAIN_MENU) {
            menu_mode();

            if(!wait_for_enter_release && sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                current_state = PLAYING;
                audio.start_ambient_music();
                audio.play(AudioSystem::SETTING_ADJUSTMENT);
            }

            if(!sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                wait_for_enter_release = false;
            }
        }

        else if(current_state == PLAYING) {
            cooldown++;
            shot_time++;
            spawn_chance++;
            swap_count++;

            // ------------------------------------------------------------
            // Difficulty progression
            // ------------------------------------------------------------
            update_difficulty();

            // ------------------------------------------------------------
            // Powerup timers
            // ------------------------------------------------------------
            if(agility_timer > 0) {
                agility_timer--;

                if(agility_timer == 0) {
                    turn_speed = BASE_TURN_SPEED;
                    audio.play(AudioSystem::AGILITY_DEACTIVATE);

                    if(rapid_fire_timer > 0) {
                        player.setFillColor(PLAYER_RAPID_FIRE_COLOR);
                    }
                    else {
                        player.setFillColor(PLAYER_BASE_COLOR);
                    }
                }
            }

            if(rapid_fire_timer > 0) {
                rapid_fire_timer--;

                if(rapid_fire_timer == 0) {
                    interval = BASE_INTERVAL;
                    audio.play(AudioSystem::RAPID_FIRE_DEACTIVATE);

                    if(agility_timer > 0) {
                        player.setFillColor(PLAYER_AGILITY_COLOR);
                    }
                    else {
                        player.setFillColor(PLAYER_BASE_COLOR);
                    }
                }
            }

            if(shield_timer > 0) {
                shield_timer--;

                if(shield_timer == 0) {
                    shield_active = false;
                    audio.play(AudioSystem::SHIELD_DOWN);
                }
            }

            if(damage_cooldown > 0) {
                damage_cooldown--;
            }

            if(sf::Keyboard::isKeyPressed(keybinds.toggle_hitboxes) && swap_count > swap_speed) {
                show_hitboxes = !show_hitboxes;
                swap_count = 0;
            }

            // ------------------------------------------------------------
            // Manual turning
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(keybinds.turn_left)) {
                rotate(player, 3, -turn_speed);
                rotate(player_hitbox, 3, -turn_speed);
            }

            if(sf::Keyboard::isKeyPressed(keybinds.turn_right)) {
                rotate(player, 3, turn_speed);
                rotate(player_hitbox, 3, turn_speed);
            }

            // ------------------------------------------------------------
            // Energy
            // ------------------------------------------------------------
            bool firing = sf::Keyboard::isKeyPressed(keybinds.fire);

            if(!firing) {
                energy += ENERGY_REGEN;

                if(energy > MAX_ENERGY) {
                    energy = MAX_ENERGY;
                }
            }

            if(rocket_idle_hum) {
                audio.start_trail_hum();
            }
            else {
                audio.stop_trail_hum();
            }

            // Shooting
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(keybinds.fire) && sf::Keyboard::isKeyPressed(keybinds.heavy_modifier) && shot_time >= interval && energy >= HEAVY_PROJECTILE_ENERGY) {
                Projectile projectile(player, 2);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;

                heavy_cooldown_start = shot_time;
                shot_time = -shot_time;

                trail_target_color = TRAIL_HEAVY_COLOR;
                last_shot_type = 2;
                audio.play(AudioSystem::HEAVY_SHOT);
            }
            else if(sf::Keyboard::isKeyPressed(keybinds.fire) && shot_time >= interval && energy >= BASIC_PROJECTILE_ENERGY) {
                Projectile projectile(player, 1);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;

                shot_time = 0;

                trail_target_color = TRAIL_BASIC_COLOR;
                last_shot_type = 1;
                audio.play(AudioSystem::BASIC_SHOT);
            }

            // ------------------------------------------------------------
            // Spawn asteroids
            // ------------------------------------------------------------
            if(randint(0, 100) == 50 && spawn_chance > spawn_rate) {
                belt.push_back(Asteroid(SPAWN_X, randint(0, static_cast<int>(WINDOW_HEIGHT))));
                spawn_chance = 0;
            }

            orientation %= 360;

            // ------------------------------------------------------------
            // Movement
            // ------------------------------------------------------------
            float radians = orientation * (M_PI / 180);
            float move_x = speed * cos(radians);
            float move_y = speed * sin(radians);

            player.move(move_x, move_y);
            player_hitbox.move(move_x, move_y);

            // ------------------------------------------------------------
            // Wall collision
            // ------------------------------------------------------------
            sf::FloatRect bounds = player.getGlobalBounds();

            bool hit_left = bounds.left <= 0.f;
            bool hit_right = bounds.left + bounds.width >= window.getSize().x;
            bool hit_top = bounds.top <= 0.f;
            bool hit_bottom = bounds.top + bounds.height >= window.getSize().y;

            if(hit_left || hit_right || hit_top || hit_bottom) {
                player_death();
            }

            if(current_state == GAME_OVER) {
                window.clear();

                if(explosion_active) {
                    float elapsed = explosion_clock.getElapsedTime().asSeconds();
                    float progress = elapsed / EXPLOSION_DURATION;

                    if(progress < 1.f) {
                        float explosion_radius = explosion_start_radius + (EXPLOSION_MAX_RADIUS - explosion_start_radius) * progress;
                        explosion.setRadius(explosion_radius);
                        explosion.setOrigin(explosion_radius, explosion_radius);

                        sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
                        explosion.setOutlineColor(sf::Color(explosion_color.r, explosion_color.g, explosion_color.b, alpha));
                        window.draw(explosion);
                    }
                }

                window.display();
                continue;
            }

            // ------------------------------------------------------------
            // Projectile movement
            // ------------------------------------------------------------
            for(Projectile &i : projs) {
                i.travel(speed);
            }

            // ------------------------------------------------------------
            // Trail
            // ------------------------------------------------------------
            if(cooldown > TRAIL_INTERVAL) {
                cooldown = 0;

                float cooldown_progress = 1.f;

                if(last_shot_type == 1) {
                    cooldown_progress = static_cast<float>(shot_time) / static_cast<float>(interval);
                }
                else if(last_shot_type == 2) {
                    cooldown_progress = static_cast<float>(shot_time + heavy_cooldown_start) / static_cast<float>(heavy_cooldown_start + HEAVY_COOLDOWN_INTERVAL);
                }

                cooldown_progress = std::max(0.f, std::min(1.f, cooldown_progress));

                float red = trail_target_color.r + (TRAIL_READY_COLOR.r - trail_target_color.r) * cooldown_progress;
                float green = trail_target_color.g + (TRAIL_READY_COLOR.g - trail_target_color.g) * cooldown_progress;
                float blue = trail_target_color.b + (TRAIL_READY_COLOR.b - trail_target_color.b) * cooldown_progress;

                sf::Color current_trail_color(
                    static_cast<sf::Uint8>(red),
                    static_cast<sf::Uint8>(green),
                    static_cast<sf::Uint8>(blue),
                    255
                );

                sf::CircleShape segment(TRAIL_SEGMENT_RADIUS);
                segment.setFillColor(sf::Color::Transparent);
                segment.setOutlineThickness(TRAIL_OUTLINE_THICKNESS);
                segment.setOutlineColor(current_trail_color);

                float offsetX = -25 * cos(radians);
                float offsetY = -25 * sin(radians);

                segment.setPosition(player.getPosition().x + center.x + offsetX, player.getPosition().y + center.y + offsetY);
                trail.push_back(segment);

                if(trail.size() > TRAIL_MAX_LENGTH) {
                    trail.erase(trail.begin());
                }
            }

            window.clear();

            // ------------------------------------------------------------
            // Deadly perimeter
            // ------------------------------------------------------------
            draw_danger_border();

            // ------------------------------------------------------------
            // Update player trail
            // ------------------------------------------------------------
            for(int i = 0; i < trail.size(); i++) {
                sf::Color trail_color = trail[i].getOutlineColor();
                sf::Uint8 new_alpha = trail_color.a > TRAIL_FADE_ALPHA ? trail_color.a - TRAIL_FADE_ALPHA : 0;

                trail[i].setOutlineColor(sf::Color(trail_color.r, trail_color.g, trail_color.b, new_alpha));
                window.draw(trail[i]);
            }

            // ------------------------------------------------------------
            // Update projectiles
            // ------------------------------------------------------------
            for(int i = 0; i < projs.size(); i++) {
                window.draw(projs[i]);

                if(show_hitboxes) {
                    window.draw(projs[i].hitbox);
                }

                sf::FloatRect bounds = projs[i].getGlobalBounds();

                if(bounds.left + bounds.width < 0 || bounds.top + bounds.height < 0 || bounds.left > window.getSize().x || bounds.top > window.getSize().y) {
                    projs.erase(projs.begin() + i);
                    i--;
                }
            }

            // ------------------------------------------------------------
            // Update asteroids
            // ------------------------------------------------------------
            for(int i = 0; i < belt.size(); i++) {
                belt[i].move(-belt[i].speed, 0.f);
                window.draw(belt[i]);

                if(show_hitboxes) {
                    window.draw(belt[i].hitbox);
                }

                if(check_collision(belt[i].hitbox, player_hitbox) && damage_cooldown == 0) {
                    // ----------------------------------------------------
                    // Shield absorbs the entire collision
                    // ----------------------------------------------------
                    if(shield_active) {
                        shield_active = false;
                        shield_timer = 0;
                        damage_cooldown = DAMAGE_COOLDOWN;

                        display_explosion(
                            sf::Vector2f(
                                player.getPosition().x + center.x,
                                player.getPosition().y + center.y
                            ),
                            sf::Color::Cyan,
                            10.f
                        );

                        audio.play(AudioSystem::HULL_DAMAGED);

                        belt[i].durability = 0;
                    }
                    else if(belt[i].toughness >= ONE_SHOT_TOUGHNESS) {
                        health = 0;
                        player_death();
                    }
                    else {
                        int hull_damage = (belt[i].toughness + 399) / 400;

                        if(hull_damage > MAX_HEALTH) {
                            hull_damage = MAX_HEALTH;
                        }

                        health -= hull_damage;
                        damage_cooldown = DAMAGE_COOLDOWN;
                        audio.play(AudioSystem::HULL_DAMAGED);

                        if(health <= 0) {
                            player_death();
                        }
                        else {
                            belt[i].durability = 0;
                        }
                    }
                }

                if(current_state == GAME_OVER) {
                    continue;
                }

                for(int j = 0; j < projs.size(); j++) {
                    if(check_collision(belt[i].hitbox, projs[j].hitbox)) {
                        belt[i].durability -= projs[j].power;

                        display_explosion(
                            sf::Vector2f(
                                projs[j].getPosition().x,
                                projs[j].getPosition().y
                            ),
                            projs[j].getFillColor(),
                            2.5f
                        );

                        projs.erase(projs.begin() + j);
                        j--;
                    }
                }

                if(belt[i].durability <= 0) {
                    // Asteroid destruction explosion
                    display_explosion(
                        sf::Vector2f(
                            belt[i].getGlobalBounds().left + belt[i].getGlobalBounds().width / 2.f,
                            belt[i].getGlobalBounds().top + belt[i].getGlobalBounds().height / 2.f
                        ),
                        belt[i].getFillColor(),
                        belt[i].radius * 0.5f
                    );

                    // ----------------------------------------------------
                    // Asteroid destruction sound
                    // ----------------------------------------------------
                    if(belt[i].toughness >= 2000) {
                        audio.play(AudioSystem::HIGH_TOUGHNESS_ASTEROID_BREAK);
                    }
                    else if(belt[i].toughness >= 1000) {
                        audio.play(AudioSystem::MID_TOUGHNESS_ASTEROID_BREAK);
                    }
                    else {
                        audio.play(AudioSystem::LOW_TOUGHNESS_ASTEROID_BREAK);
                    }

                    // ----------------------------------------------------
                    // Powerup drop
                    // ----------------------------------------------------
                    float drop_chance = belt[i].get_drop_chance();

                    if(randint(1, 10000) <= static_cast<int>(drop_chance * 100.f)) {
                        int powerup_type = randint(1, 4);

                        powerups.push_back(
                            Powerup(
                                belt[i].getGlobalBounds().left + belt[i].getGlobalBounds().width / 2.f,
                                belt[i].getGlobalBounds().top + belt[i].getGlobalBounds().height / 2.f,
                                powerup_type
                            )
                        );

                        audio.play(AudioSystem::POWERUP_SPAWN);
                    }

                    belt.erase(belt.begin() + i);
                    i--;
                    continue;
                }

                sf::FloatRect bounds = belt[i].getGlobalBounds();

                if(bounds.left + bounds.width < 0) {
                    belt.erase(belt.begin() + i);
                    i--;
                    continue;
                }
            }

            // ------------------------------------------------------------
            // Update powerups
            // ------------------------------------------------------------
            for(int i = 0; i < powerups.size(); i++) {
                powerups[i].move(-POWERUP_SPEED, 0.f);

                if(check_collision(powerups[i].hitbox, player_hitbox)) {
                    powerups[i].activate(energy, health, turn_speed, interval, agility_timer, rapid_fire_timer, shield_timer, shield_active);

                    if(powerups[i].p_type == 3) {
                        player.setFillColor(PLAYER_AGILITY_COLOR);
                    }
                    else if(powerups[i].p_type == 4) {
                        player.setFillColor(PLAYER_RAPID_FIRE_COLOR);
                    }

                    display_explosion(
                        sf::Vector2f(
                            powerups[i].getPosition().x,
                            powerups[i].getPosition().y
                        ),
                        powerups[i].getFillColor(),
                        5.f
                    );

                    if(powerups[i].p_type == 1) {
                        audio.play(AudioSystem::HULL_REGEN);
                    }
                    else if(powerups[i].p_type == 2) {
                        audio.play(AudioSystem::SHIELD_UP);
                    }
                    else if(powerups[i].p_type == 3) {
                        audio.play(AudioSystem::AGILITY_ACTIVATE);
                    }
                    else if(powerups[i].p_type == 4) {
                        audio.play(AudioSystem::RAPID_FIRE_ACTIVATE);
                    }

                    powerups.erase(powerups.begin() + i);
                    i--;
                    continue;
                }

                window.draw(powerups[i]);

                if(show_hitboxes) {
                    window.draw(powerups[i].hitbox);
                }

                sf::FloatRect bounds = powerups[i].getGlobalBounds();

                if(bounds.left + bounds.width < 0) {
                    powerups.erase(powerups.begin() + i);
                    i--;
                    continue;
                }
            }

            // ------------------------------------------------------------
            // Shield visual
            // ------------------------------------------------------------
            if(shield_active) {
                shield_visual.setPosition(
                    player.getPosition().x + center.x + PLAYER_CENTER_OFFSET_Y * 2.f,
                    player.getPosition().y + center.y + PLAYER_CENTER_OFFSET_Y
                );

                window.draw(shield_visual);
            }

            // ------------------------------------------------------------
            // Explosion
            // ------------------------------------------------------------
            if(explosion_active) {
                float elapsed = explosion_clock.getElapsedTime().asSeconds();
                float progress = elapsed / EXPLOSION_DURATION;

                if(progress >= 1.f) {
                    explosion_active = false;
                }
                else {
                    float radius = explosion_start_radius + (EXPLOSION_MAX_RADIUS - explosion_start_radius) * progress;
                    explosion.setRadius(radius);
                    explosion.setOrigin(radius, radius);

                    sf::Uint8 alpha = static_cast<sf::Uint8>(255.f * (1.f - progress));
                    explosion.setOutlineColor(sf::Color(explosion_color.r, explosion_color.g, explosion_color.b, alpha));
                    window.draw(explosion);
                }
            }

            // ------------------------------------------------------------
            // Draw player
            // ------------------------------------------------------------
            window.draw(player);

            if(show_hitboxes) {
                window.draw(player_hitbox);
            }

            // ------------------------------------------------------------
            // Hull & energy bars
            // ------------------------------------------------------------
            health_bar.setSize(sf::Vector2f(BAR_WIDTH * (static_cast<float>(health) / MAX_HEALTH), BAR_HEIGHT));
            energy_bar.setSize(sf::Vector2f(BAR_WIDTH * (energy / MAX_ENERGY), BAR_HEIGHT));

            window.draw(score_text);
            window.draw(high_score_text);
            window.draw(health_text);
            window.draw(health_background);
            window.draw(health_bar);
            window.draw(energy_text);
            window.draw(energy_background);
            window.draw(energy_bar);

            // ------------------------------------------------------------
            // Update score
            // ------------------------------------------------------------
            interval_count++;

            if(interval_count >= SCORE_INTERVAL) {
                score++;
                interval_count = 0;
            }

            score_text.setString("Score: " + std::to_string(score));
            high_score_text.setString("High Score: " + std::to_string(high_score));

            // ------------------------------------------------------------
            // Display
            // ------------------------------------------------------------
            window.display();
        }

        else if(current_state == PAUSED_MENU) {
            draw_pause_menu();
        }

        else if(current_state == GAME_OVER) {
            window.clear();
            game_over();
        }
    }

    return 0;
}
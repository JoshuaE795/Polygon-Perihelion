// include
// --------------------------------------------------------------------
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include <cmath>
#include <vector>
#include <algorithm>
// --------------------------------------------------------------------

/*
Run command:

g++ main.cpp -I/opt/homebrew/Cellar/sfml/2.6.1/include -o prog -L/opt/homebrew/Cellar/sfml/2.6.1/lib -lsfml-graphics -lsfml-window -lsfml-system

*/

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

sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Polygon Perihelion", sf::Style::Close);
sf::ConvexShape player(3UL);
sf::ConvexShape player_hitbox(3UL);
std::vector<sf::CircleShape> trail;
sf::Vector2f center(window.getSize().x / 2, window.getSize().y / 2);
sf::Font font;

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
        current_state = MAIN_MENU;
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

        player.setFillColor(PLAYER_BASE_COLOR);
    }

    window.display();
}

void player_death() {
    current_state = GAME_OVER;

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

    if(!font.loadFromFile("Orbitron-VariableFont_wght.ttf")) {
        std::cerr << "Error loading font" << std::endl;
        return -1;
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
        }

        // menu handling
        // --------------------------------------------------------------------
        if(current_state == MAIN_MENU) {
            menu_mode();

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
                current_state = PLAYING;
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
                }
            }

            if(damage_cooldown > 0) {
                damage_cooldown--;
            }

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Grave) && swap_count > swap_speed) {
                show_hitboxes = !show_hitboxes;
                swap_count = 0;
            }

            // ------------------------------------------------------------
            // Manual turning
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                rotate(player, 3, -turn_speed);
                rotate(player_hitbox, 3, -turn_speed);
            }

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                rotate(player, 3, turn_speed);
                rotate(player_hitbox, 3, turn_speed);
            }

            // ------------------------------------------------------------
            // Energy
            // ------------------------------------------------------------
            bool firing = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);

            if(!firing) {
                energy += ENERGY_REGEN;

                if(energy > MAX_ENERGY) {
                    energy = MAX_ENERGY;
                }
            }

            // ------------------------------------------------------------
            // Shooting
            // ------------------------------------------------------------
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) && shot_time >= interval && energy >= HEAVY_PROJECTILE_ENERGY) {
                Projectile projectile(player, 2);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;

                heavy_cooldown_start = shot_time;
                shot_time = -shot_time;

                trail_target_color = TRAIL_HEAVY_COLOR;
                last_shot_type = 2;
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && shot_time >= interval && energy >= BASIC_PROJECTILE_ENERGY) {
                Projectile projectile(player, 1);
                projs.push_back(projectile);
                energy -= projectile.energy_cost;

                shot_time = 0;

                trail_target_color = TRAIL_BASIC_COLOR;
                last_shot_type = 1;
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

        else if(current_state == GAME_OVER) {
            window.clear();
            game_over();
        }
    }

    return 0;
}